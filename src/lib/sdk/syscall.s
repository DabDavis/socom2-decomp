# sc: compiler=ee-gcc2.9-991111
# libkernl's syscall stubs (MAIN 0x1A3700-0x1A3FA0): Sony's SDK 2.7, the one assembler object in
# libkernl but the TLB handlers. The demo links the same object -- 138 stubs at 0x15B100-0x15B9A0,
# byte-identical slot for slot -- and its symbols name every one (the slot pairing names the eight
# whose numbers repeat: AddIntcHandler2, RFU085..089, RFU091, RFU116). Its `.text` section symbol
# and its lack of a `gcc2_compiled.` marker say the object was assembled, not compiled (README "The
# GCC front"). A stub loads the BIOS call number into v1 and traps: `li v1,N; syscall; jr ra; nop`.
# Numbers repeat where the kernel serves two entry points with one call (AddIntcHandler2, the
# event-flag RFUs); negative numbers are the interrupt-context (`i`) variants' calls.

	.set	noreorder

	.macro	STUB name, num
	.globl	\name
	.type	\name,@function
	.ent	\name
\name:
	li	$3, \num
	syscall
	jr	$31
	nop
	.end	\name
	.size	\name,.-\name
	.endm

	.text
	.align	4
	STUB	RFU000_FullReset, 0
	STUB	ResetEE, 1
	STUB	SetGsCrt, 2
	STUB	RFU003, 3
	STUB	_Exit, 4
	STUB	RFU005, 5
	STUB	_LoadExecPS2, 6
	STUB	_ExecPS2, 7
	STUB	RFU008, 8
	STUB	RFU009, 9
	STUB	AddSbusIntcHandler, 10
	STUB	RemoveSbusIntcHandler, 11
	STUB	Interrupt2Iop, 12
	STUB	SetVTLBRefillHandler, 13
	STUB	SetVCommonHandler, 14
	STUB	SetVInterruptHandler, 15
	STUB	AddIntcHandler, 16
	STUB	AddIntcHandler2, 16
	STUB	RemoveIntcHandler, 17
	STUB	AddDmacHandler, 18
	STUB	AddDmacHandler2, 18
	STUB	RemoveDmacHandler, 19
	STUB	_EnableIntc, 20
	STUB	_DisableIntc, 21
	STUB	_EnableDmac, 22
	STUB	_DisableDmac, 23
	STUB	SetAlarm, 252
	STUB	ReleaseAlarm, 253
	STUB	_iEnableIntc, -26
	STUB	_iDisableIntc, -27
	STUB	_iEnableDmac, -28
	STUB	_iDisableDmac, -29
	STUB	iSetAlarm, -254
	STUB	iReleaseAlarm, -255
	STUB	CreateThread, 32
	STUB	DeleteThread, 33
	STUB	StartThread, 34
	STUB	ExitThread, 35
	STUB	ExitDeleteThread, 36
	STUB	TerminateThread, 37
	STUB	iTerminateThread, -38
	STUB	DisableDispatchThread, 39
	STUB	EnableDispatchThread, 40
	STUB	ChangeThreadPriority, 41
	STUB	iChangeThreadPriority, -42
	STUB	RotateThreadReadyQueue, 43
	STUB	_iRotateThreadReadyQueue, -44
	STUB	ReleaseWaitThread, 45
	STUB	iReleaseWaitThread, -46
	STUB	GetThreadId, 47
	STUB	ReferThreadStatus, 48
	STUB	iReferThreadStatus, -49
	STUB	SleepThread, 50
	STUB	WakeupThread, 51
	STUB	_iWakeupThread, -52
	STUB	CancelWakeupThread, 53
	STUB	iCancelWakeupThread, -54
	STUB	SuspendThread, 55
	STUB	_iSuspendThread, -56
	STUB	ResumeThread, 57
	STUB	iResumeThread, -58
	STUB	JoinThread, 59
	STUB	RFU060, 60
	STUB	RFU061, 61
	STUB	EndOfHeap, 62
	STUB	RFU063, 63
	STUB	CreateSema, 64
	STUB	DeleteSema, 65
	STUB	SignalSema, 66
	STUB	iSignalSema, -67
	STUB	WaitSema, 68
	STUB	PollSema, 69
	STUB	iPollSema, -70
	STUB	ReferSemaStatus, 71
	STUB	iReferSemaStatus, -72
	STUB	RFU073, 73
	STUB	SetOsdConfigParam, 74
	STUB	GetOsdConfigParam, 75
	STUB	GetGsHParam, 76
	STUB	GetGsVParam, 77
	STUB	SetGsHParam, 78
	STUB	SetGsVParam, 79
	STUB	RFU080_CreateEventFlag, 80
	STUB	RFU081_DeleteEventFlag, 81
	STUB	RFU082_SetEventFlag, 82
	STUB	RFU083_iSetEventFlag, -83
	STUB	RFU084_ClearEventFlag, 84
	STUB	RFU085_iClearEventFlag, -85
	STUB	RFU086_WaitEvnetFlag, 86
	STUB	RFU087_PollEvnetFlag, 87
	STUB	RFU088_iPollEvnetFlag, -88
	STUB	RFU089_ReferEventFlagStatus, 89
	STUB	RFU090_iReferEventFlagStatus, -90
	STUB	RFU091, 91
	STUB	EnableIntcHandler, 92
	STUB	iEnableIntcHandler, -92
	STUB	DisableIntcHandler, 93
	STUB	iDisableIntcHandler, -93
	STUB	EnableDmacHandler, 94
	STUB	iEnableDmacHandler, -94
	STUB	DisableDmacHandler, 95
	STUB	iDisableDmacHandler, -95
	STUB	KSeg0, 96
	STUB	EnableCache, 97
	STUB	DisableCache, 98
	STUB	GetCop0, 99
	STUB	FlushCache, 100
	STUB	CpuConfig, 102
	STUB	iGetCop0, -103
	STUB	iFlushCache, -104
	STUB	iCpuConfig, -106
	STUB	sceSifStopDma, 107
	STUB	SetCPUTimerHandler, 108
	STUB	SetCPUTimer, 109
	STUB	SetOsdConfigParam2, 110
	STUB	GetOsdConfigParam2, 111
	STUB	GsGetIMR, 112
	STUB	iGsGetIMR, -112
	STUB	GsPutIMR, 113
	STUB	iGsPutIMR, -113
	STUB	SetPgifHandler, 114
	STUB	SetVSyncFlag, 115
	STUB	RFU116, 116
	STUB	_print, 117
	STUB	sceSifDmaStat, 118
	STUB	isceSifDmaStat, -118
	STUB	sceSifSetDma, 119
	STUB	isceSifSetDma, -119
	STUB	sceSifSetDChain, 120
	STUB	isceSifSetDChain, -120
	STUB	sceSifSetReg, 121
	STUB	sceSifGetReg, 122
	STUB	_ExecOSD, 123
	STUB	Deci2Call, 124
	STUB	PSMode, 125
	STUB	MachineType, 126
	STUB	GetMemorySize, 127
	STUB	_InitTLB, 130
