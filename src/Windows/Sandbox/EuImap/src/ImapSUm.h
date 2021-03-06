// IMAPSUM.H
//
// Routines for message summaries

#ifndef _IMAPSUM_H_
#define _IMAPSUM_H_

class CSummary;

class CImapSum : public CObject
{
public:
	CImapSum();
	~CImapSum();

	void SetOffset(long Offset) { m_Offset = Offset; }
	void SetFlag(WORD Flag) { m_Flags |= Flag; }
	void UnsetFlag(WORD Flag) { m_Flags &= ~Flag; }
	void SetFlagEx(WORD Flag) { m_FlagsEx |= Flag; }
	void UnsetFlagEx(WORD Flag) { m_FlagsEx &= ~Flag; }

	void SetState(char State)	{ m_State = State; }

	void SetPriority(int Priority) { m_Priority = (short)Priority; }

	void GleanPriority(const char* HeaderContents);
	void SetFrom(const char* NewFrom);
	void SetSubject(const char* NewSubject);

	const char* GetFrom();
	CString GetPriority() const;
	const char* GetSubject();

	unsigned long GetHash()
		{ return (m_Hash); }
	unsigned long SetHash(unsigned long Hash)
		{ return (m_Hash = Hash); }

	// Copying between a CSummary.
	//
	void CopyFromCSummary (CSummary* pSum);
	void CopyToCSummary (CSummary* pSum);

// Implementation
public:
#ifdef _DEBUG
	void AssertValid() const;
	void Dump(CDumpContext& dc) const;
#endif

public:
	long m_Offset;
	long m_Length;
	short m_State;
	short m_Priority;

	long m_Seconds;

	char m_From[64];
	char m_Subject[64];

	// Stores the raw date header value so "FormatDate()" can extract the
	// timezone.
	//
	CString m_RawDateString;

	// Important: m_SummaryDate is NOT filled in for a newly downloaded message.
	// It is NEVER copied from a CImapSum to a CSummary!!! It's used only to pass
	// CSummary's m_Date value to the "WriteOfflineMessage" routine.
	//
	CString m_SummaryDate;

	// Public.
	unsigned long m_Imflags;		// IMAP message flags - 4 bytes.
	unsigned short m_MsgSize;

	int m_nUndownloadedAttachments;

private:
	WORD			m_Flags;
	WORD			m_FlagsEx;

	unsigned long	m_Hash;

};


class CImapSumList : public CObList
{
public:
	CImapSumList() {}
	~CImapSumList();

	int		GetIndex(const CImapSum* Sum) const;

	void	DeleteAll ();

};


#endif
