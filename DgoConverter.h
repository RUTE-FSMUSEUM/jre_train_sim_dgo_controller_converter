#pragma once

//-----------------------------------------------------------------------------
// File: DgoConverter.h
//
// Copyright (c) 2022 FSMUSEUM All rights reserved.
//-----------------------------------------------------------------------------

#include <windows.h>
#include <dinputd.h>

const int NAME_SIZE = 128;

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

typedef struct TriggerCondition {
	WORD EB;
	WORD B8;
	WORD B7;
	WORD B6;
	WORD B5;
	WORD B4;
	WORD B3;
	WORD B2;
	WORD B1;
	WORD NT;
	WORD P1;
	WORD P2;
	WORD P3;
	WORD P4;
	WORD P5;
	WORD A;
	WORD B;
	WORD C;
	WORD START;
	WORD SELECT;
}TRIGCOND;

typedef struct ButtonForMascon {
	bool BT0;
	bool BT1;
	bool BT2;
	bool BT3;
	bool BT4;
	bool BT5;
	bool BT6;
	bool BT7;
	bool BT8;
	bool BT9;
}ISMASCONBT;

typedef struct KeyTriggerInfo {
	WCHAR NAME[NAME_SIZE];
	TRIGCOND AX_X;
	TRIGCOND AX_Y;
	TRIGCOND AX_Z;
	TRIGCOND BT;
	ISMASCONBT ISMASCON; // マスコンの位置がAXISではなくボタンでも表現されるJC-PS101Uのためだけに作ったconfigで、もっと良い方法はないのか模索中
}KEYTRIGGERINFO;

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