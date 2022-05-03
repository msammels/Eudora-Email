/******************************************************************************/
/*																										*/
/*	Name		:	ISCHED.H			  																*/
/* Date     :  5/28/1997                                                      */
/* Author   :  Jim Susoy                                                      */
/* Notice   :  (C) 1997 Qualcomm, Inc. - All Rights Reserved                  */
/* Copyright (c) 2016, Computer History Museum 
All rights reserved. 
Redistribution and use in source and binary forms, with or without modification, are permitted (subject to 
the limitations in the disclaimer below) provided that the following conditions are met: 
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. 
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following 
   disclaimer in the documentation and/or other materials provided with the distribution. 
 * Neither the name of Computer History Museum nor the names of its contributors may be used to endorse or promote products 
   derived from this software without specific prior written permission. 
NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE 
COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH 
DAMAGE. */

/*	Desc.		:	ISched class definitions	 												*/
/*																										*/
/******************************************************************************/
#ifndef __ISCHED_H__
#define __ISCHED_H__

typedef void (*CONTINUATION)(LPVOID pCtx);

typedef enum {
	SCHED_ASAP,
	SCHED_LATER,
} ISCHED_PROPS;

typedef class ISchedule * LPISCHEDULE;
class ISchedule : public IUnknown
{
public:
	/* Schedule the execution of the specified continuation routine				*/
	virtual BOOL	Schedule(CONTINUATION pCB,LPVOID pCxt,DWORD dwDelay=0,ISCHED_PROPS ipWhen=SCHED_ASAP) PURE;
	
	/* Get number of continuations/timers scheduled on this object.				*/
	virtual DWORD	Count(void) PURE;
	
	/* Cancel any scheduled continuation routines for this object					*/
	virtual void	Cancel(void) PURE;
};

/* 4c8661f0-d79c-11d0-8d3d-00a02471d0b1 (Generated by uuidgen.exe)				*/
#define ISCHED_CLASS_STR		L"{4c8661f0-d79c-11d0-8d3d-00a02471d0b1}"
DEFINE_GUID(CLSID_ISchedule	,0x4c8661f0,0xd79c,0x11d0,0x8d,0x3d,0x00,0xa0,0x24,0x71,0xd0,0xb1);

#define ISCHED_IID_STR			ISCHED_CLASS_STR
DEFINE_GUID(IID_ISchedule 		,0x4c8661f0,0xd79c,0x11d0,0x8d,0x3d,0x00,0xa0,0x24,0x71,0xd0,0xb1);

#endif


