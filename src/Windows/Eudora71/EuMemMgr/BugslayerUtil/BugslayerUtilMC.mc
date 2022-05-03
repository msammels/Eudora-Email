;//-----------------------------------------------------------------------
;// Debugging Applications for Microsoft .NET and Microsoft Windows
;// Copyright � 1997-2003 John Robbins -- All rights reserved.
;//-----------------------------------------------------------------------

MessageIdTypedef=DWORD
SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )
FacilityNames=(System=0x0:FACILITY_SYSTEM
               Runtime=0x2:FACILITY_RUNTIME
               Stubs=0x3:FACILITY_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
              )
LanguageNames=(English=0x409:MSG00409)

MessageId=0x10
Severity=Success
Facility=Runtime
SymbolicName=MSG_BSU_ERROR_SUCCESS
Language=English
%1
.

MessageId=0x20
Severity=Informational
Facility=Runtime
SymbolicName=MSG_BSU_ERROR_INFORMATIONAL
Language=English
%1
.

MessageId=0x30
Severity=Warning
Facility=Runtime
SymbolicName=MSG_BSU_ERROR_WARNING
Language=English
%1
.

MessageId=0x40
Severity=Error
Facility=Runtime
SymbolicName=MSG_BSU_ERROR_ERROR
Language=English
%1
.
