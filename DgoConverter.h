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