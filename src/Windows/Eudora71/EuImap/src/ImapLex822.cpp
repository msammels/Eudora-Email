// IMAPLEX822.CPP
//
// Routines for parsing RFC 822 messages (non-global-pop version!!).
//

#include "stdafx.h"


#ifdef IMAP4 // Only for IMAP.


#include "ImapLex822.h"

#include "resource.h"
#include "Base64.h"
#include "mime.h"

#include "ImapExports.h"
#include "imap.h"
#include "imapgets.h"
#include "ImapConnection.h"
#include "ImapMailbox.h"
#include "ImapDownload.h"
#include "rs.h"
#include "utils.h"


#include "DebugNewHelpers.h"

// Statics ////////////////////////////////
//
static BOOL Translate2047(char* text, const char* charset, char encoding);
static void PseudoQP(char* text);
static int DecodeB64String(char* s);
//
/////////////////////////////////////////////

// When the analyzer is in CollectLine, this table tells it what state to
// enter based on character class
//
static const char LineTable[CHARTYPE_LIMIT] =
{
	CollectAtom, 	CollectAtom, 	CollectSpecial, CollectSpecial,
	CollectSpecial,	CollectLWSP,	CollectLWSP,	CollectQText,
	CollectComment,	CollectSpecial,	CollectSpecial,	CollectSpecial,
	CollectSpecial,	CollectSpecial,	CollectSpecial,	CollectSpecial,
	CollectDL,		CollectSpecial,	CollectAtom,	CollectSpecial,
	CollectSpecial,	CollectSpecial
};

// Classify characters.
static const BYTE Class822[256] =
{
/*   0 (.) */ CTL822,  /*   1 (.) */ CTL822,  /*   2 (.) */ CTL822,
/*   3 (.) */ CTL822,  /*   4 (.) */ CTL822,  /*   5 (.) */ CTL822,
/*   6 (.) */ CTL822,  /*   7 (.) */ CTL822,  /*   8 (.) */ CTL822,
/*   9 (.) */ HTAB822, /*  10 (.) */ LF822,   /*  11 (.) */ CTL822,
/*  12 (.) */ CTL822,  /*  13 (.) */ CR822,   /*  14 (.) */ CTL822,
/*  15 (.) */ CTL822,  /*  16 (.) */ CTL822,  /*  17 (.) */ CTL822,
/*  18 (.) */ CTL822,  /*  19 (.) */ CTL822,  /*  20 (.) */ CTL822,
/*  21 (.) */ CTL822,  /*  22 (.) */ CTL822,  /*  23 (.) */ CTL822,
/*  24 (.) */ CTL822,  /*  25 (.) */ CTL822,  /*  26 (.) */ CTL822,
/*  27 (.) */ CTL822,  /*  28 (.) */ CTL822,  /*  29 (.) */ CTL822,
/*  30 (.) */ CTL822,  /*  31 (.) */ CTL822,  /*  32 ( ) */ SPACE822,
/*  33 (!) */ CHAR822, /*  34 (") */ QUOTE822,/*  35 (#) */ CHAR822,
/*  36 ($) */ CHAR822, /*  37 (%) */ CHAR822,
/*  38 (&) */ CHAR822, /*  39 (') */ CHAR822, /*  40 (() */ LPAR822,
/*  41 ()) */ RPAR822, /*  42 (*) */ CHAR822, /*  43 (+) */ CHAR822,
/*  44 (,) */ COMMA822,/*  45 (-) */ CHAR822, /*  46 (.) */ DOT822,
/*  47 (/) */ SLASH822,/*  48 (0) */ DIGIT822,/*  49 (1) */ DIGIT822,
/*  50 (2) */ DIGIT822,/*  51 (3) */ DIGIT822,/*  52 (4) */ DIGIT822,
/*  53 (5) */ DIGIT822,/*  54 (6) */ DIGIT822,/*  55 (7) */ DIGIT822,
/*  56 (8) */ DIGIT822,/*  57 (9) */ DIGIT822,/*  58 (:) */ COLON822,
/*  59 (;) */ SEM822,  /*  60 (<) */ CHAR822, /*  61 (=) */ EQUAL822,
/*  62 (>) */ CHAR822, /*  63 (?) */ QMARK822,/*  64 (@) */ AT822,
/*  65 (A) */ ALPHA822,/*  66 (B) */ ALPHA822,/*  67 (C) */ ALPHA822,
/*  68 (D) */ ALPHA822,/*  69 (E) */ ALPHA822,/*  70 (F) */ ALPHA822,
/*  71 (G) */ ALPHA822,/*  72 (H) */ ALPHA822,/*  73 (I) */ ALPHA822,
/*  74 (J) */ ALPHA822,/*  75 (K) */ ALPHA822,/*  76 (L) */ ALPHA822,
/*  77 (M) */ ALPHA822,/*  78 (N) */ ALPHA822,/*  79 (O) */ ALPHA822,
/*  80 (P) */ ALPHA822,/*  81 (Q) */ ALPHA822,/*  82 (R) */ ALPHA822,
/*  83 (S) */ ALPHA822,/*  84 (T) */ ALPHA822,/*  85 (U) */ ALPHA822,
/*  86 (V) */ ALPHA822,/*  87 (W) */ ALPHA822,/*  88 (X) */ ALPHA822,
/*  89 (Y) */ ALPHA822,/*  90 (Z) */ ALPHA822,/*  91 ([) */ LSQU822,
/*  92 (\) */ BSLSH822,/*  93 (]) */ RSQU822, /*  94 (^) */ CHAR822,
/*  95 (_) */ CHAR822, /*  96 (`) */ CHAR822, /*  97 (a) */ ALPHA822,
/*  98 (b) */ ALPHA822,/*  99 (c) */ ALPHA822,/* 100 (d) */ ALPHA822,
/* 101 (e) */ ALPHA822,/* 102 (f) */ ALPHA822,/* 103 (g) */ ALPHA822,
/* 104 (h) */ ALPHA822,/* 105 (i) */ ALPHA822,/* 106 (j) */ ALPHA822,
/* 107 (k) */ ALPHA822,/* 108 (l) */ ALPHA822,/* 109 (m) */ ALPHA822,
/* 110 (n) */ ALPHA822,/* 111 (o) */ ALPHA822,/* 112 (p) */ ALPHA822,
/* 113 (q) */ ALPHA822,/* 114 (r) */ ALPHA822,/* 115 (s) */ ALPHA822,
/* 116 (t) */ ALPHA822,/* 117 (u) */ ALPHA822,/* 118 (v) */ ALPHA822,
/* 119 (w) */ ALPHA822,/* 120 (x) */ ALPHA822,/* 121 (y) */ ALPHA822,
/* 122 (z) */ ALPHA822,/* 123 ({) */ CHAR822, /* 124 (|) */ CHAR822,
/* 125 (}) */ CHAR822, /* 126 (~) */ CHAR822, /* 127 (.) */ CHAR822,
/* 128 (.) */ CHAR822, /* 129 (.) */ CHAR822, /* 130 (.) */ CHAR822,
/* 131 (.) */ CHAR822, /* 132 (.) */ CHAR822, /* 133 (.) */ CHAR822,
/* 134 (.) */ CHAR822, /* 135 (.) */ CHAR822, /* 136 (.) */ CHAR822,
/* 137 (.) */ CHAR822, /* 138 (.) */ CHAR822, /* 139 (.) */ CHAR822,
/* 140 (.) */ CHAR822, /* 141 (.) */ CHAR822, /* 142 (.) */ CHAR822,
/* 143 (.) */ CHAR822, /* 144 (.) */ CHAR822, /* 145 (.) */ CHAR822,
/* 146 (.) */ CHAR822, /* 147 (.) */ CHAR822, /* 148 (.) */ CHAR822,
/* 149 (.) */ CHAR822, /* 150 (.) */ CHAR822, /* 151 (.) */ CHAR822,
/* 152 (.) */ CHAR822, /* 153 (.) */ CHAR822, /* 154 (.) */ CHAR822,
/* 155 (.) */ CHAR822, /* 156 (.) */ CHAR822, /* 157 (.) */ CHAR822,
/* 158 (.) */ CHAR822, /* 159 (.) */ CHAR822, /* 160 (.) */ CHAR822,
/* 161 (.) */ CHAR822, /* 162 (.) */ CHAR822, /* 163 (.) */ CHAR822,
/* 164 (.) */ CHAR822, /* 165 (.) */ CHAR822, /* 166 (.) */ CHAR822,
/* 167 (.) */ CHAR822, /* 168 (.) */ CHAR822, /* 169 (.) */ CHAR822,
/* 170 (.) */ CHAR822, /* 171 (.) */ CHAR822, /* 172 (.) */ CHAR822,
/* 173 (.) */ CHAR822, /* 174 (.) */ CHAR822, /* 175 (.) */ CHAR822,
/* 176 (.) */ CHAR822, /* 177 (.) */ CHAR822, /* 178 (.) */ CHAR822,
/* 179 (.) */ CHAR822, /* 180 (.) */ CHAR822, /* 181 (.) */ CHAR822,
/* 182 (.) */ CHAR822, /* 183 (.) */ CHAR822, /* 184 (.) */ CHAR822,
/* 185 (.) */ CHAR822, /* 186 (.) */ CHAR822, /* 187 (.) */ CHAR822,
/* 188 (.) */ CHAR822, /* 189 (.) */ CHAR822, /* 190 (.) */ CHAR822,
/* 191 (.) */ CHAR822, /* 192 (.) */ CHAR822, /* 193 (.) */ CHAR822,
/* 194 (.) */ CHAR822, /* 195 (.) */ CHAR822, /* 196 (.) */ CHAR822,
/* 197 (.) */ CHAR822, /* 198 (.) */ CHAR822, /* 199 (.) */ CHAR822,
/* 200 (.) */ CHAR822, /* 201 (.) */ CHAR822, /* 202 (.) */ CHAR822,
/* 203 (.) */ CHAR822, /* 204 (.) */ CHAR822, /* 205 (.) */ CHAR822,
/* 206 (.) */ CHAR822, /* 207 (.) */ CHAR822, /* 208 (.) */ CHAR822,
/* 209 (.) */ CHAR822, /* 210 (.) */ CHAR822, /* 211 (.) */ CHAR822,
/* 212 (.) */ CHAR822, /* 213 (.) */ CHAR822, /* 214 (.) */ CHAR822,
/* 215 (.) */ CHAR822, /* 216 (.) */ CHAR822, /* 217 (.) */ CHAR822,
/* 218 (.) */ CHAR822, /* 219 (.) */ CHAR822, /* 220 (.) */ CHAR822,
/* 221 (.) */ CHAR822, /* 222 (.) */ CHAR822, /* 223 (.) */ CHAR822,
/* 224 (.) */ CHAR822, /* 225 (.) */ CHAR822, /* 226 (.) */ CHAR822,
/* 227 (.) */ CHAR822, /* 228 (.) */ CHAR822, /* 229 (.) */ CHAR822,
/* 230 (.) */ CHAR822, /* 231 (.) */ CHAR822, /* 232 (.) */ CHAR822,
/* 233 (.) */ CHAR822, /* 234 (.) */ CHAR822, /* 235 (.) */ CHAR822,
/* 236 (.) */ CHAR822, /* 237 (.) */ CHAR822, /* 238 (.) */ CHAR822,
/* 239 (.) */ CHAR822, /* 240 (.) */ CHAR822, /* 241 (.) */ CHAR822,
/* 242 (.) */ CHAR822, /* 243 (.) */ CHAR822, /* 244 (.) */ CHAR822,
/* 245 (.) */ CHAR822, /* 246 (.) */ CHAR822, /* 247 (.) */ CHAR822,
/* 248 (.) */ CHAR822, /* 249 (.) */ CHAR822, /* 250 (.) */ CHAR822,
/* 251 (.) */ CHAR822, /* 252 (.) */ CHAR822, /* 253 (.) */ CHAR822,
/* 254 (.) */ CHAR822, /* 255 (.) */ CHAR822
};


// For incomplete tokens, we must return a token based on what
// state the analyzer is in
//
static const BYTE State2Token[] =
{
	ErrorToken,		// Init822
	ErrorToken,		// CollectLine
	LinearWhite,	// CollectLWSP
	Atom,			// CollectAtom
	Comment,		// CollectComment
	QText,			// CollectQText
	DomainLit,		// CollectDL
	Special,		// CollectSpecial
	RegText,		// CollectText
	ErrorToken		// ReceiveError
};


ImapLex822State::ImapLex822State(CImapDownloader *pRW) : m_pRW(pRW)
{
	m_State = Init822;
}

// Lex822 - lexical analyzer for RFC 822 header fields
//
Token822Enum ImapLex822State::Lex822()
{
	Char822Enum charClass, nextClass;
	Token822Enum returnToken = Continue822;
	State822Enum origState = m_State;

	// initialize
	if (m_State == Init822)
	{
		m_State		= CollectLine;
		m_TokenSpot = 0;
		m_pSpot		= m_pEnd = m_Buffer;
		m_InStructure = 0;
		m_UhOh		= m_Has2047 = FALSE;
	}
	m_ReinitToken = TRUE;
	
	// main processing loop
	do
	{
		// grab chars if need be
		if (m_pSpot >= m_pEnd) LexFill();
		
		// size up the situation
		charClass = (Char822Enum)Class822[*m_pSpot];
		
		// empty header?
		if (charClass == CR822 && origState == Init822) return (EndOfHeader);
		
		nextClass = (Char822Enum)((m_pSpot < m_pEnd - 1)? Class822[m_pSpot[1]] : LexFill());
		
		// errors fetching chars?
		if (m_State == ReceiveError)
			return(ErrorToken);
		
		// early termination?
		if (m_UhOh && nextClass==CR822 || origState == Init822 && charClass == DOT822 && nextClass == CR822)
		{
			// we did, we did tee a putty tat
			m_State = Init822;
			return (EndOfMessage);
		}
		m_UhOh = (charClass == LF822 && nextClass == DOT822);
		
		// Do we need to clear the token buffer?
		if (m_ReinitToken)
		{
			m_InStructure = 0;
			m_TokenSpot = 0;
			m_ReinitToken = FALSE;
		}
		
		// are we going to overrun the token buffer?
		if (m_TokenSpot > sizeof(m_Token)-3)
		{
			m_ReinitToken = TRUE;
			returnToken = (Token822Enum)State2Token[m_State];
			break;
		}
		
		// Is this a folded CRLF?
		if (charClass == LF822 && (nextClass == SPACE822 || nextClass == HTAB822))
			charClass = SPACE822;   // treat it like a space
		
		// ok, process the character
		switch (m_State)
		{
			/*********************************/
			case CollectAtom:
				switch (charClass)
				{
					case ALPHA822:
					case DIGIT822:
					case CHAR822:
					case DOT822:   // RFC 822 has this as special, MIME does not
						ADD(*m_pSpot++);   // add char to atom
						break;

					default:
						returnToken = Atom;
						m_State = CollectLine;
						break;
				}
				break;

			/*********************************/             
			case CollectLWSP:
				switch (charClass)
				{
					case SPACE822:
					case HTAB822:
						ADD(*m_pSpot++);   // add char to lwsp
						break;
					
					default:
						returnToken = LinearWhite;
						m_State = CollectLine;
						break;
				}
				break;
			
			/*********************************/
			case CollectComment:
				if (m_Token[m_TokenSpot] == '\\')
					ADD(*m_pSpot++);
				else
					switch(charClass)
					{
						case RPAR822:
							ADD(*m_pSpot++);
							if (!--m_InStructure)
							{
								returnToken = Comment;
								m_State = CollectLine;
							}
							break;

						case LPAR822:
							ADD(*m_pSpot++);
							m_InStructure++;
							break;
						
						case CR822:
							m_InStructure = 0;
							returnToken = Comment;
							m_State = CollectLine;
							break;
						
						default:
							ADD(*m_pSpot++);
							break;
					}
				break;
			
			/*********************************/
			case CollectQText:
				if (m_Token[m_TokenSpot] == '\\')
					ADD(*m_pSpot++);
				else
					switch (charClass)
					{
						case QUOTE822:
							m_pSpot++; // skip the quote
							if (!m_InStructure)                           
							{   
								m_InStructure++;	// we weren't in a quote, but we are now
								break;
							}
							// quote has ended; fall-through is deliberate
							
						case CR822:
							returnToken = QText;
							m_State = CollectLine;
							break;
						
						default:
							ADD(*m_pSpot++);
							break;
					}
				break;
		
			/*********************************/
			case CollectDL:
				if (m_Token[m_TokenSpot] == '\\')
					ADD(*m_pSpot++);
				else
					switch (charClass)
					{
						case LSQU822:
							m_pSpot++;         // skip the [
							break;
						
						case RSQU822:
							m_pSpot++;         // skip the ], and fall through
							
						case CR822:
							returnToken = DomainLit;
							m_State = CollectLine;
							break;
						
						default:
							ADD(*m_pSpot++);
							break;
					}
				break;
			
			/*********************************/
			case CollectSpecial:
				// are we looking at the end of the world as we know it?
				if (charClass == LF822 && nextClass == CR822)
				{
					// CR has already been written, so let's add the LF
					ADD(*m_pSpot++);
					if ((returnToken = WriteHeaderToken(Special)) != ErrorToken)
						returnToken = EndOfHeader;
					m_State = Init822;
				}
				else if (charClass == QUOTE822)
					m_State = CollectQText;
				else if (charClass == LPAR822)
					m_State = CollectComment;
				else
				{
					ADD(*m_pSpot++);
					returnToken = Special;
					m_State = CollectLine;
				}
				break;
			
			/*********************************/
			case CollectText:
				switch (charClass)
				{
					case LF822:
						returnToken = RegText;
						m_State = CollectLine;
						break;

//					// This folds continuation header lines into a space
//					case SPACE822:
//						if (TokenSpot && Token[TokenSpot - 1] == '\r')
//						{
//							Token[TokenSpot - 1] = ' ';
//							charClass = nextClass;
//							Spot += 2;
//							break;
//						}
//						// fall through
					
					default:
						ADD(*m_pSpot++);
						break;
				}
				break;
				
			/*********************************/
			case CollectLine:
				m_State = (State822Enum)LineTable[charClass];
				m_ReinitToken = TRUE;
				break;
		}
	}
	while (returnToken == Continue822);

	return (returnToken);
}

// WriteHeaderToken - write a header token to disk
//
Token822Enum ImapLex822State::WriteHeaderToken(Token822Enum tokenType)
{
	long err = 1;

	// This must have already been set.
	if ( ! m_pRW )
	{
		ASSERT (0);
		return ErrorToken;
	}
	
	switch (tokenType)
	{
		// Either of these get written as two returns
		case EndOfMessage:
			if ( ( !m_pRW->WriteTextLine() ) || ( !m_pRW->WriteTextLine() ) )
			{
				err = -1;
			}
			break;

		// One CR-LF has already been written, so only write out one more
		case EndOfHeader:

			// JOK: If the current header is the top-level header, this will
			// return a non-zero value. Write an X-UID header line.
			m_pRW->WriteXHEADERLine ();

			// Now write the blank line.
			if ( !m_pRW->WriteTextLine() )
				err = -1;
			break;
		
		// These don't get written
		case ErrorToken:
			break;
		
		case QText:
			if ( !m_pRW->Put ('"') ||
				 !m_pRW->Put ((char*)m_Token, m_TokenSpot) ||
				 !m_pRW->Put('"') )
			{
				err = -1;
			}
			break;
		
		// write out the token
		default:
			if (!m_pRW->Put ((char*)m_Token, m_TokenSpot) )
				err = -1;
			break;
	}
	
	return (err < 0L? ErrorToken : tokenType);
}


				
// LexFill - grab some chars for the lexical analyzer
//  returns the character class of the first of the new chars
//  if no chars, returns CR (since this will eventually terminate the analyzer)
//    and sets state to ReceiveError.
//
Char822Enum ImapLex822State::LexFill()
{
	BYTE* readHere;
	int size;
	
	 // Once an error, always an error
	if (m_State == ReceiveError)
		return (CR822);
	
	// If we have a leftover char, move it to front of buffer and put new
	// BYTEs after it.  Else, just read into entire buffer
	if (m_pSpot < m_pEnd)
	{
		*m_Buffer = *m_pSpot;
		m_pSpot = m_Buffer;
		readHere = m_pSpot + 1;
	}
	else
		readHere = m_pSpot = m_Buffer;
	
	// Receive chars
	size = sizeof(m_Buffer) - (m_pSpot-m_Buffer) - 2;
	if ((size = m_pRW->ReadLine((char*)readHere, size)) <= 0)
	{
		// ReadPOPLine() returns an empty buffer when .<CR><LF> is seen,
		// but the parser expects to see it, so add it back in
		if (size == 0)
		{
			strcpy((char*)readHere, ".\r\n");
			size = 3;
		}
		else
		{
			m_State = ReceiveError;     // Abort.  Things are bad.
			*readHere = '\n';         // back out of current token.
			size = 0;                 // ignore anything we might have gotten
		}
	}
	// ok, this is a bit non-standard--treat a line with a single space
	// as though it were a blank line.
	else if (size == 3 && readHere[0] == ' ' && readHere[1] == '\r')
	{
		readHere[0] = '\r';
		readHere[1] = '\n';
		size--;
	}
	
	// This isn't strictly kosher, but we're going to do it here anyway,
	// because I can't imagine how to do this the "right" way, which is
	// to integrate it with the lexing & grammar.  So shoot me.
	m_Has2047 = ImapFix2047((char*)readHere, size) || m_Has2047;
	
	m_pEnd = readHere + size;
	return ((Char822Enum)Class822[*readHere]);
}








/************************************************************************
 * Translate2047 - translate a string according to the 2047 rules
 *  returns whether or not the string was successfully translated
 ************************************************************************/
static BOOL Translate2047(char* text, const char* charset, char encoding)
{
	int			 iCharsetIdx = FindMIMECharset(charset);

	// A charset index of -1 indicates we were not able to identify a
	// charset so don't bother doing anything.
	if (iCharsetIdx == -1)
		return (FALSE);
	
	// first, we undo the encoding
	switch (encoding)
	{
	case 'Q':
	case 'q':
		PseudoQP(text);
		break;
	case 'B':
	case 'b':
		if (DecodeB64String(text)) return(FALSE);	// decode errors abort
		break;
	default:
		return(FALSE);
		break;
	}
	
	// iCharsetIdx = 0 is Windows, 1 is US ASCII and 2 is Latin1 which are
	// not translated so only translate if the index is greater than 2.
	if (iCharsetIdx > 2)
	{
		// Translate the text from the specified charset.
		ISOTranslate(text, strlen(text), iCharsetIdx);
	}
	
	// there.  that didn't hurt.  all that much.  at least not that you can see.
	return (TRUE);
}




/************************************************************************
 * Fix2047 - translate RFC 2047 stuff
 ************************************************************************/
BOOL ImapFix2047(char* chars, int& len)
{
	char *equal, *lastEqual;
	char* q[4];
	char** thisQ;
	char* end= chars + len;
	char* spot = chars;
	char text[1024];
	char charset[32];
	int numChars;
	char encoding;
	char* nextEqual;
	BOOL found = FALSE;
	
	while (spot < end)
	{
		// scan for first =
		while (end - spot > 8 && *spot != '=')
			spot++;
		if (end - spot <= 8)
			return(found);		// done
		equal = spot;
		
		// is next char a ?
		if (spot[1] != '?')
		{
			spot++;
			continue;
		}
		q[0] = spot + 1;
		
		// find the remaining three ?'s
		for (thisQ = q + 1; thisQ < q + 4; thisQ++)
		{
			for (thisQ[0] = thisQ[-1] + 1; **thisQ != '?'; ++*thisQ)
				if (end - *thisQ < 2)
					return(found);
		}
		
		// is the character after the last ? an '='?
		if (q[3][1] != '=')
		{
			spot += 2;
			continue;
		}
		lastEqual = q[3] + 1;
		
		// is the encoding method a single char?
		if (q[2] - q[1] != 2)
		{
			spot += 2;
			continue;
		}
		
		// They're here.
		found = TRUE;
		encoding = q[1][1];
		numChars = min(q[1] - q[0] - 1, sizeof(charset));
		strncpy(charset, q[0] + 1, numChars);
		charset[numChars] = 0;
		numChars = min(q[3] - q[2] - 1, sizeof(text));
		strncpy(text, q[2] + 1, numChars);
		text[numChars] = 0;

		if (Translate2047(text, charset, encoding))
		{
			// move new chars into place
			strcpy(equal, text);
			// check for stuff we should or should not remove
			if (lastEqual[1] == '\r' && lastEqual[2] == '\n') ;//lastEqual += 2; // toast a trailing return
			else
			{	// if the next thing is an encoded word, toast intervening
				for (nextEqual = lastEqual + 1; nextEqual < end; nextEqual++)
				{
					if (nextEqual[0] == '=' && nextEqual[1] == '?')
					{
						lastEqual = nextEqual - 1;
						break;
					}
					else if (*nextEqual != ' ' && *nextEqual != '\t')
						break;
				}
			}
			// move chars from after encoded text to after decoded text
			// lastEqual points to the original buffer, which is not necessarily
			// NULL terminated, so we have to do a little mumbo-jumbo to
			// get the strcat() to work correctly
			char SaveChar = *end;
			*end = 0;
			strcat(equal, lastEqual + 1);
			*end = SaveChar;

			// adjust end to account for deleted chars
			end = equal + strlen(equal);
			len = strlen(chars);
			// adjust lastEqual to point to last decoded char
			lastEqual = equal + strlen(text) - 1;
#ifdef NEVER
			if (*lastEqual == ' ' || *lastEqual == '\012') lastEqual++;
#endif
		}
		spot = lastEqual + 1;
	}
	
	return (found);
}



/************************************************************************
 * PseudoQP - quoted-printable, kind of
 ************************************************************************/
static void PseudoQP(char* text)
{
	char* spot = text;
	char* end = spot + strlen(text);
	char* copySpot = spot;
	
	while (spot < end)
	{
		switch (*spot)
		{
		case '_':
			*copySpot++ = ' ';
			break;
		case '=':
			if (end - spot < 3) *copySpot++ = *spot;
			else
			{
				HexToString(copySpot++, spot + 1, 2);
				spot += 2;
			}
			break;
		default:
			*copySpot++ = *spot;
			break;
		}
		spot++;
	}
	
	*copySpot = 0;
}



/************************************************************************
 * DecodeB64String - decode a base64 string
 ************************************************************************/
static int DecodeB64String(char* s)
{
	Base64Decoder	d64;
	LONG			InLen = strlen(s);
	LONG			OutLen;
	int				result;
	
	d64.Init();
	result = d64.Decode(s, InLen, s, OutLen);
	if (OutLen >= 0 && OutLen <= InLen)
		s[OutLen] = 0;
//	if ((d64.decoderState+d64.padCount)%4) result++;
	
	return (result);
}

#endif // IMAP4
