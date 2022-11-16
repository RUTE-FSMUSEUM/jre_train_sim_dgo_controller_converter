#pragma once

//-----------------------------------------------------------------------------
// File: DgoConverter.h
//
// Copyright (c) 2022 FSMUSEUM All rights reserved.
//-----------------------------------------------------------------------------

#include <windows.h>
#include <dinputd.h>

const int NAME_SIZE = 128;
const int NUM_BUTTONS = 10;
const WCHAR* NOT_IN_USE = L"N/A";
const int NUM_VALIDATE = 4;

enum validateInputResult { NOT_VALIDATED, VALIDATED, NOT_ASSIGNED };

typedef struct KeyConfig {
	WORD EB;
	WORD BNT;
	WORD PNT;
	WORD BUP;
	WORD PUP;
	WORD BDN;
	WORD PDN;
	WORD NUL;
	WORD A;
	WORD B;
	WORD C;
	WORD START;
	WORD SELECT;
}KEYCONFIG;

typedef struct KeyInfo {
	WCHAR NAME[NAME_SIZE];
	KEYCONFIG KEY;
}KEYINFO;

typedef struct WcharTriggerCondition {
	WCHAR EB[NAME_SIZE];
	WCHAR B8[NAME_SIZE];
	WCHAR B7[NAME_SIZE];
	WCHAR B6[NAME_SIZE];
	WCHAR B5[NAME_SIZE];
	WCHAR B4[NAME_SIZE];
	WCHAR B3[NAME_SIZE];
	WCHAR B2[NAME_SIZE];
	WCHAR B1[NAME_SIZE];
	WCHAR NT[NAME_SIZE];
	WCHAR P1[NAME_SIZE];
	WCHAR P2[NAME_SIZE];
	WCHAR P3[NAME_SIZE];
	WCHAR P4[NAME_SIZE];
	WCHAR P5[NAME_SIZE];
	WCHAR A[NAME_SIZE];
	WCHAR B[NAME_SIZE];
	WCHAR C[NAME_SIZE];
	WCHAR START[NAME_SIZE];
	WCHAR SELECT[NAME_SIZE];
}TRIGCOND_WCHAR;

typedef struct KeyTriggerInfo {
	WCHAR NAME[NAME_SIZE];
	TRIGCOND_WCHAR AX_X;
	TRIGCOND_WCHAR AX_Y;
	TRIGCOND_WCHAR AX_Z;
	TRIGCOND_WCHAR BT;
	bool isExclusiveButton[NUM_BUTTONS]; // マスコンの位置がAXISではなくボタンでも表現されるJC-PS101Uのためだけに作ったconfigで、もっと良い方法はないのか模索中
}KEYTRIGGERINFO;

typedef struct TriggerInfoForOneKey {
	const WCHAR* AX_X;
	const WCHAR* AX_Y;
	const WCHAR* AX_Z;
	const WCHAR* BT;
}TRIGGERVALUES;

typedef struct ButtonState {
	bool A;
	bool B;
	bool C;
	bool START;
	bool SELECT;
}BUTTONSTATE;

typedef struct GameControllerDeviceInfo {
	WCHAR productName[260];
	_GUID guidInstance;
}GAMECTRLDEVICEINFO;