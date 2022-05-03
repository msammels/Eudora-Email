/* Copyright (c) 2017, Computer History Museum All rights reserved. Redistribution and use in source and binary forms, with or without modification, are permitted (subject to the limitations in the disclaimer below) provided that the following conditions are met:  * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.  * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following    disclaimer in the documentation and/or other materials provided with the distribution.  * Neither the name of Computer History Museum nor the names of its contributors may be used to endorse or promote products    derived from this software without specific prior written permission. NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. *//* * Program:	Mac newline routines * * Author:	Mark Crispin *		6158 Lariat Loop NE *		Bainbridge Island, WA  98110-2098 *		Internet: MRC@Panda.COM * * Date:	26 January 1992 * Last Edited:	6 April 1996 * * Copyright 1996 by Mark Crispin * *  Permission to use, copy, modify, and distribute this software and its * documentation for any purpose and without fee is hereby granted, provided * that the above copyright notice appears in all copies and that both the * above copyright notices and this permission notice appear in supporting * documentation, and that the name of Mark Crispin not be used in advertising * or publicity pertaining to distribution of the software without specific, * written prior permission.  This software is made available "as is", and * MARK CRISPIN DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD TO * THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED WARRANTIES OF * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN NO EVENT SHALL * MARK CRISPIN BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, * WHETHER IN AN ACTION OF CONTRACT, TORT (INCLUDING NEGLIGENCE) OR STRICT * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF * THIS SOFTWARE. * *//* Copy string with CRLF newlines * Accepts: destination string *	    pointer to size of destination string buffer *	    source string *	    length of source string * Returns: length of copied string */unsigned long strcrlfcpy (char **dst,unsigned long *dstl,char *src,			  unsigned long srcl){  long i,j;  char c,*d = src;				/* count number of CR's in source string(s) */  for (i = srcl,j = 0; j < srcl; j++) if (*d++ == '\015') i++;				/* flush destination buffer if too small */  if (*dst && (i > *dstl)) fs_give ((void **) dst);  if (!*dst) {			/* make a new buffer if needed */    *dst = (char *) fs_get ((*dstl = i) + 1);    if (dstl) *dstl = i;	/* return new buffer length to main program */  }  d = *dst;			/* destination string */  while (srcl--) {		/* copy strings */    c = *d++ = *src++;		/* copy character */				/* append line feed to bare CR */    if ((c == '\015') && (*src != '\012')) *d++ = '\012';  }  *d = '\0';			/* tie off destination */  return d - *dst;		/* return length */}/* Length of string after strcrlfcpy applied * Accepts: source string * Returns: length of string */unsigned long strcrlflen (STRING *s){  unsigned long pos = GETPOS (s);  unsigned long i = SIZE (s);  unsigned long j = i;  while (j--) if ((SNX (s) == '\015') && ((CHR (s) != '\012') || !j)) i++;  SETPOS (s,pos);		/* restore old position */  return i;}