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

typedef struct ats {
	WORD CONF;
	WORD RESNORM;
	WORD RESEMER;
}ATSINFO;

typedef struct AtsBool {
	bool CONF;
	bool RESNORM;
	bool RESEMER;
}ATSBOOL;

typedef struct AtsWchar {
	WCHAR CONF[NAME_SIZE];
	WCHAR RESNORM[NAME_SIZE];
	WCHAR RESEMER[NAME_SIZE];
}ATSWCHAR;

typedef struct MasconConfig {
	WORD EB;
	WORD BNT;
	WORD PNT;
	WORD BUP;
	WORD PUP;
	WORD BDN;
	WORD PDN;
}MASCONCONFIG;

typedef struct ButtonConfig {
	WORD ESC;
	WORD ENTER;
	WORD UP;
	WORD DOWN;
	WORD LEFT;
	WORD RIGHT;
	WORD ELECHORN;
	WORD HORN;
	WORD BUZZER;
	WORD EBREST;
	ATSINFO ATS;
	WORD TASC;
	WORD INCHING;
	WORD CRUISE;
	WORD SUPB;
	WORD SLOPESTAT;
	WORD INFO;
	WORD NEXTVIEW;
	WORD BUP;
}BUTTONCONFIG;

typedef struct ButtonBoolConfig {
	bool ESC;
	bool ENTER;
	bool UP;
	bool DOWN;
	bool LEFT;
	bool RIGHT;
	bool ELECHORN;
	bool HORN;
	bool BUZZER;
	bool EBREST;
	ATSBOOL ATS;
	bool TASC;
	bool INCHING;
	bool CRUISE;
	bool SUPB;
	bool SLOPESTAT;
	bool INFO;
	bool NEXTVIEW;
	bool BUP;
}BUTTONCONFIG_BOOL;

typedef struct KeyConfig {
	WORD NUL;
	MASCONCONFIG MASCON;
	BUTTONCONFIG BTN;
	BUTTONCONFIG_BOOL isHoldButton;
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
	WCHAR ESC[NAME_SIZE];
	WCHAR ENTER[NAME_SIZE];
	WCHAR UP[NAME_SIZE];
	WCHAR DOWN[NAME_SIZE];
	WCHAR LEFT[NAME_SIZE];
	WCHAR RIGHT[NAME_SIZE];
	WCHAR ELECHORN[NAME_SIZE];
	WCHAR HORN[NAME_SIZE];
	WCHAR BUZZER[NAME_SIZE];
	WCHAR EBREST[NAME_SIZE];
	ATSWCHAR ATS;
	WCHAR TASC[NAME_SIZE];
	WCHAR INCHING[NAME_SIZE];
	WCHAR CRUISE[NAME_SIZE];
	WCHAR SUPB[NAME_SIZE];
	WCHAR SLOPESTAT[NAME_SIZE];
	WCHAR INFO[NAME_SIZE];
	WCHAR NEXTVIEW[NAME_SIZE];
	WCHAR BUP[NAME_SIZE];
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

typedef struct GameControllerDeviceInfo {
	WCHAR productName[260];
	_GUID guidInstance;
}GAMECTRLDEVICEINFO;