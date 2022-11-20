//-----------------------------------------------------------------------------
// File: DgoConverter.cpp
//
// Edit by FSMUSEUM.
// Copyright (c) 2022 FSMUSEUM Some rights reserved.
//
// Original Copyright Joystick.cpp: Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#define STRICT
#define DIRECTINPUT_VERSION 0x0800
#define _CRT_SECURE_NO_DEPRECATE
#ifndef _WIN32_DCOM
#define _WIN32_DCOM
#endif

#include <windows.h>
#include <commctrl.h>
#include <basetsd.h>
#include <dinput.h>
#include <dinputd.h>
#include <assert.h>
#include <oleauto.h>
#include <shellapi.h>

#pragma warning( disable : 4996 ) // disable deprecated warning 
#include <strsafe.h>
#pragma warning( default : 4996 )
#include "resource.h"

#include "DgoConverter.h"




//-----------------------------------------------------------------------------
// Function-prototypes
//-----------------------------------------------------------------------------
INT_PTR CALLBACK MainDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK    EnumObjectsCallback( const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext );
BOOL CALLBACK    EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance, VOID* pContext );
HRESULT InitDirectInput( HWND hDlg );
VOID FreeDirectInput();
HRESULT SwitchJoystick( HWND hDlg, GUID guidInstance );
HRESULT UpdateInputState( HWND hDlg, TCHAR* state, BUTTONCONFIG_BOOL* buttonState, KEYCONFIG* keymap, KEYTRIGGERINFO* triggermap, const bool isKeymapWithHoldDown );
VOID makeKeyBoardOutput( const TCHAR* masconText, const TCHAR* buttonText, const DIJOYSTATE2 js, TCHAR* state, BUTTONCONFIG_BOOL* buttonState, KEYTRIGGERINFO* triggermap, KEYCONFIG* keymap, const bool isKeymapWithHoldDown );
VOID makeMasconKeyBoardOutput( INPUT* inputs, INPUT* release, int* idx_inputs, int* idx_release, bool* isHoldDown, const TCHAR* strText, const DIJOYSTATE2 js, TCHAR* state, KEYTRIGGERINFO* triggermap, KEYCONFIG* keymap, const bool isKeymapWithHoldDown );
VOID makeButtonKeyBoardOutput( INPUT* inputs, INPUT* release, int* idx_inputs, int* idx_release, const TCHAR* strText, const DIJOYSTATE2 js, BUTTONCONFIG_BOOL* buttonState, KEYTRIGGERINFO* triggermap, KEYCONFIG* keymap );
VOID makeButtonInputArray( INPUT* inputs, INPUT* release, int* idx_inputs, int* idx_release, WORD wVk, bool* thisButtonState, bool isHoldButton, bool isValidThisButton );
bool validateMasconState( WCHAR* validateState, const TCHAR* buttonStrText, const DIJOYSTATE2 js, KEYTRIGGERINFO* triggermap );
bool validateButtonState( WCHAR* validateState, const TCHAR* buttonStrText, const DIJOYSTATE2 js, KEYTRIGGERINFO* triggermap );
bool validateMasconInputs( const DIJOYSTATE2 js, const TCHAR* buttonStrText, TRIGGERVALUES triggerValues );
bool validateButtonInputs( const DIJOYSTATE2 js, const TCHAR* buttonStrText, TRIGGERVALUES triggerValues );

// Stuff to filter out XInput devices
#include <wbemidl.h>
HRESULT SetupForIsXInputDevice();
bool IsXInputDevice( const GUID* pGuidProductFromDirectInput );
void CleanupForIsXInputDevice();

struct XINPUT_DEVICE_NODE
{
    DWORD dwVidPid;
    XINPUT_DEVICE_NODE* pNext;
};

struct DI_ENUM_CONTEXT
{
    DIJOYCONFIG* pPreferredJoyCfg;
    bool bPreferredJoyCfgValid;
};

bool                    g_bFilterOutXinputDevices = false;
XINPUT_DEVICE_NODE*     g_pXInputDeviceList = NULL;




//-----------------------------------------------------------------------------
// Defines, constants, and global variables
//-----------------------------------------------------------------------------
#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

LPDIRECTINPUT8          g_pDI = NULL;
LPDIRECTINPUTDEVICE8    g_pJoystick = NULL;

const int				num_maxDeviceInfo = 10; // Maximun number of stored device info is currently set to 10
int						idx_deviceInfo = 0;
GAMECTRLDEVICEINFO		g_deviceInfo[num_maxDeviceInfo];

TCHAR					state[4] = { 'N', 'A', '\0', '\0' }; // Set current state of controller
BUTTONSTATE				buttonState = { FALSE, FALSE, FALSE, FALSE, FALSE }; // Set current state of buttons

// Key mapping
KEYINFO keyinfo[] = { 
	{ L"JRE SIM ONE HANDLE", { 0x31, 0x53, 0x53, 0x51, 0x51, 0x5a, 0x5a, NULL, 0x00, VK_BACK, VK_RETURN, 0x00, 0x00 }},
	{ L"JRE SIM TWO HANDLE", { VK_OEM_2, 0x4d, 0x53, VK_OEM_PERIOD, 0x41, VK_OEM_COMMA, 0x5a, NULL, 0x00, VK_BACK, VK_RETURN, 0x00, 0x00 }},
	{ L"HMMSIM METRO", { VK_OEM_PERIOD, VK_OEM_COMMA, 0x41, VK_OEM_PERIOD, 0x41, VK_OEM_COMMA, 0x5a, NULL, 0x41, 0x00, VK_RETURN, 0x00, VK_SPACE }},
};
const int keyinfo_len = sizeof(keyinfo) / sizeof(KEYINFO);
// Key trigger conditions
KEYTRIGGERINFO triggermap[] = {
	{ L"JC-PS101U", 
	  { L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"-1000", L"1000", L"1000", L"-1000", L"-1000", L"24", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A" },
	  { L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A" },
	  { L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A" },
	  { L"", L"04 07 ", L"04 06 07 ", L"05 ", L"05 06 ", L"04 05 ", L"04 05 06 ", L"05 07 ", L"05 06 07 ", L"04 05 07 ", L"00 04 05 07 ", L"04 05 07 ", L"00 04 05 07 ", L"04 05 07 ", L"00 04 05 07 ", L"3", L"2", L"1", L"8", L"9" },
      { TRUE, FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE }
	},
	{ L"JC-PS101U - late",
	  { L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"-1000", L"1000", L"1000", L"-1000", L"-1000", L"-8", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A" },
	  { L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A" },
	  { L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A" },
	  { L"", L"04 07 ", L"04 06 07 ", L"05 ", L"05 06 ", L"04 05 ", L"04 05 06 ", L"05 07 ", L"05 06 07 ", L"04 05 07 ", L"00 04 05 07 ", L"04 05 07 ", L"00 04 05 07 ", L"04 05 07 ", L"00 04 05 07 ", L"3", L"2", L"1", L"8", L"9" },
	  { TRUE, FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE }
	},
	{ L"ZUIKI One Handle Mascon",
	  { L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A" },
	  { L"-1000", L"-961", L"-852", L"-750", L"-641", L"-531", L"-430", L"-320", L"-211", L"0", L"239", L"429", L"612", L"802", L"1000", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A" },
	  { L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A" },
	  { L"06 ", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"0", L"1", L"2", L"9", L"8" },
	  { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE }
	}
	/*
	{ L"DEBUG",
	  { L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A" },
	  { L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A" },
	  { L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A" },
	  { L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A", L"N/A" },
	  { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE }
	}
	*/
};
const int triggermap_len = sizeof(triggermap) / sizeof(KEYTRIGGERINFO);
WPARAM index; // Selected item index for key mapping
WPARAM idx_currentDevice; // Selected item index for devices
WPARAM idx_currentTriggerMap; // Selected item index for key trigger map




//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point for the application.  Since we use a simple dialog for 
//       user interaction we don't need to pump messages.
//-----------------------------------------------------------------------------
int APIENTRY WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, int )
{
    InitCommonControls();

    WCHAR* strCmdLine;
    int nNumArgs;
    LPWSTR* pstrArgList = CommandLineToArgvW( GetCommandLineW(), &nNumArgs );
    for( int iArg = 1; iArg < nNumArgs; iArg++ )
    {
        strCmdLine = pstrArgList[iArg];

        // Handle flag args
        if( *strCmdLine == L'/' || *strCmdLine == L'-' )
        {
            strCmdLine++;

            int nArgLen = ( int )wcslen( L"noxinput" );
            if( _wcsnicmp( strCmdLine, L"noxinput", nArgLen ) == 0 && strCmdLine[nArgLen] == 0 )
            {
                g_bFilterOutXinputDevices = true;
                continue;
            }
        }
    }
    LocalFree( pstrArgList );

    // Display the main dialog box.
    DialogBox( hInst, MAKEINTRESOURCE( IDD_JOYST_IMM ), NULL, MainDlgProc );

    return 0;
}




//-----------------------------------------------------------------------------
// Name: MainDialogProc
// Desc: Handles dialog messages
//-----------------------------------------------------------------------------
INT_PTR CALLBACK MainDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED_PARAMETER( lParam );

    switch( msg )
    {
        case WM_INITDIALOG:
            if( FAILED( InitDirectInput( hDlg ) ) )
            {
                MessageBox( NULL, TEXT( "Error Initializing DirectInput" ),
                            TEXT( "DgoConverter" ), MB_ICONERROR | MB_OK );
                EndDialog( hDlg, 0 );
            }

			// Add items: target keymapping
			for (int i = 0; i < keyinfo_len; i++) {
				WPARAM current_index = SendMessage( GetDlgItem(hDlg, IDC_COMBOBOX_KEYMAP), CB_ADDSTRING, 0, (LPARAM)keyinfo[i].NAME );
				if (i == 0)
					index = SendMessage( GetDlgItem(hDlg, IDC_COMBOBOX_KEYMAP), CB_SETCURSEL, current_index, 0 ); // Select default
			}
			// Add items: devices
			for (int i = 0; i < idx_deviceInfo; i++) {
				WPARAM current_index = SendMessage(GetDlgItem(hDlg, IDC_COMBOBOX_DEVICE_LIST), CB_ADDSTRING, 0, (LPARAM)g_deviceInfo[i].productName);
				if (i == idx_deviceInfo - 1)
					idx_currentDevice = SendMessage(GetDlgItem(hDlg, IDC_COMBOBOX_DEVICE_LIST), CB_SETCURSEL, current_index, 0); // Select default
			}
			// Add items: key trigger map
			for (int i = 0; i < triggermap_len; i++) {
				WPARAM current_index = SendMessage(GetDlgItem(hDlg, IDC_COMBOBOX_TRIGGERMAP), CB_ADDSTRING, 0, (LPARAM)triggermap[i].NAME);
				if (i == 0)
					idx_currentTriggerMap = SendMessage(GetDlgItem(hDlg, IDC_COMBOBOX_TRIGGERMAP), CB_SETCURSEL, current_index, 0); // Select default
			}

            // Set a timer to go off 30 times a second. At every timer message
            // the input device will be read
            SetTimer( hDlg, 0, 1000 / 100, NULL );
            return TRUE;

        case WM_ACTIVATE:
            if( WA_INACTIVE != wParam && g_pJoystick )
            {
                // Make sure the device is acquired, if we are gaining focus.
				g_pJoystick->Acquire();
            }
            return TRUE;

        case WM_TIMER:
            // Update the input device every timer message
            if( FAILED( UpdateInputState( hDlg, state, &buttonState, &keyinfo[index].KEY, &triggermap[idx_currentTriggerMap], wcscmp( keyinfo[index].NAME, L"HMMSIM METRO" ) == 0 ) ) )
            {
                KillTimer( hDlg, 0 );
                MessageBox( NULL, TEXT( "Error Reading Input State. " ) \
                            TEXT( "The program will now exit." ), TEXT( "DgoConverter" ),
                            MB_ICONERROR | MB_OK );
                EndDialog( hDlg, TRUE );
            }
            return TRUE;

        case WM_COMMAND:
            switch( LOWORD( wParam ) )
            {
				// Get which item is selected
				case IDC_COMBOBOX_KEYMAP:
					if ( HIWORD( wParam ) == CBN_SELCHANGE )
						index = SendMessage( GetDlgItem(hDlg, IDC_COMBOBOX_KEYMAP), CB_GETCURSEL, 0, 0 );
					return TRUE;
				case IDC_COMBOBOX_TRIGGERMAP:
					if (HIWORD(wParam) == CBN_SELCHANGE)
						idx_currentTriggerMap = SendMessage(GetDlgItem(hDlg, IDC_COMBOBOX_TRIGGERMAP), CB_GETCURSEL, 0, 0);
					return TRUE;
				case IDC_COMBOBOX_DEVICE_LIST:
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						idx_currentDevice = SendMessage(GetDlgItem(hDlg, IDC_COMBOBOX_DEVICE_LIST), CB_GETCURSEL, 0, 0);
						SwitchJoystick(hDlg, g_deviceInfo[idx_currentDevice].guidInstance);
					}
					return TRUE;
                case IDCANCEL:
                    EndDialog( hDlg, 0 );
                    return TRUE;
            }

        case WM_DESTROY:
            // Cleanup everything
            KillTimer( hDlg, 0 );
            FreeDirectInput();
            return TRUE;
    }

    return FALSE; // Message not handled 
}




//-----------------------------------------------------------------------------
// Name: InitDirectInput()
// Desc: Initialize the DirectInput variables.
//-----------------------------------------------------------------------------
HRESULT InitDirectInput( HWND hDlg )
{
    HRESULT hr;

    // Register with the DirectInput subsystem and get a pointer
    // to a IDirectInput interface we can use.
    // Create a DInput object
    if( FAILED( hr = DirectInput8Create( GetModuleHandle( NULL ), DIRECTINPUT_VERSION,
                                         IID_IDirectInput8, ( VOID** )&g_pDI, NULL ) ) )
        return hr;


    if( g_bFilterOutXinputDevices )
        SetupForIsXInputDevice();

    DIJOYCONFIG PreferredJoyCfg = { NULL };
	//PreferredJoyCfg.guidInstance = { 2017556176,42687,4583, {128, '\x1', 'D', 'E', 'S', 'T', '\0', '\0'} }; // Designate GUID of JC-PS101UBK
    DI_ENUM_CONTEXT enumContext;
    enumContext.pPreferredJoyCfg = &PreferredJoyCfg;
    enumContext.bPreferredJoyCfgValid = false; // Set true for using a device where PreferredJoyCfg.guidInstance = { 2017556176,42687,4583, {128, '\x1', 'D', 'E', 'S', 'T', '\0', '\0'}.

    IDirectInputJoyConfig8* pJoyConfig = NULL;
    if( FAILED( hr = g_pDI->QueryInterface( IID_IDirectInputJoyConfig8, ( void** )&pJoyConfig ) ) )
        return hr;

    PreferredJoyCfg.dwSize = sizeof( PreferredJoyCfg );
    //if( SUCCEEDED( pJoyConfig->GetConfig( 0, &PreferredJoyCfg, DIJC_GUIDINSTANCE ) ) ) // This function is expected to fail if no Joystick is attached
    //    enumContext.bPreferredJoyCfgValid = true;
    SAFE_RELEASE( pJoyConfig );


    // Look for a simple Joystick we can use for this program.
    if( FAILED( hr = g_pDI->EnumDevices( DI8DEVCLASS_GAMECTRL,
                                         EnumJoysticksCallback,
                                         &enumContext, DIEDFL_ATTACHEDONLY ) ) )
        return hr;

    if( g_bFilterOutXinputDevices )
        CleanupForIsXInputDevice();

    // Make sure we got a Joystick
    if( NULL == g_pJoystick )
    {
        MessageBox( NULL, TEXT( "Joystick not found. The program will now exit." ),
                    TEXT( "DgoConverter" ),
                    MB_ICONERROR | MB_OK );
        EndDialog( hDlg, 0 );
        return S_OK;
    }

    // Set the data format to "simple Joystick" - a predefined data format 
    //
    // A data format specifies which controls on a device we are interested in,
    // and how they should be reported. This tells DInput that we will be
    // passing a DIJOYSTATE2 structure to IDirectInputDevice::GetDeviceState().
    if( FAILED( hr = g_pJoystick->SetDataFormat( &c_dfDIJoystick2 ) ) )
        return hr;

    // Set the cooperative level to let DInput know how this device should
    // interact with the system and with other DInput applications.
	// MUST BE "DISCL_NONEXCLUSIVE" and "DISCL_BACKGROUND" mode
	if( FAILED( hr = g_pJoystick->SetCooperativeLevel( hDlg, DISCL_NONEXCLUSIVE |
		                                               DISCL_BACKGROUND ) ) )
        return hr;

    // Enumerate the Joystick objects. The callback function enabled user
    // interface elements for objects that are found, and sets the min/max
    // values property for discovered axes.
    if( FAILED( hr = g_pJoystick->EnumObjects( EnumObjectsCallback,
                                               ( VOID* )hDlg, DIDFT_ALL ) ) )
        return hr;

    return S_OK;
}


//-----------------------------------------------------------------------------
// Enum each PNP device using WMI and check each device ID to see if it contains 
// "IG_" (ex. "VID_045E&PID_028E&IG_00").  If it does, then it’s an XInput device
// Unfortunately this information can not be found by just using DirectInput.
// Checking against a VID/PID of 0x028E/0x045E won't find 3rd party or future 
// XInput devices.
//
// This function stores the list of xinput devices in a linked list 
// at g_pXInputDeviceList, and IsXInputDevice() searchs that linked list
//-----------------------------------------------------------------------------
HRESULT SetupForIsXInputDevice()
{
    IWbemServices* pIWbemServices = NULL;
    IEnumWbemClassObject* pEnumDevices = NULL;
    IWbemLocator* pIWbemLocator = NULL;
    IWbemClassObject* pDevices[20] = {0};
    BSTR bstrDeviceID = NULL;
    BSTR bstrClassName = NULL;
    BSTR bstrNamespace = NULL;
    DWORD uReturned = 0;
    bool bCleanupCOM = false;
    UINT iDevice = 0;
    VARIANT var;
    HRESULT hr;

    // CoInit if needed
    hr = CoInitialize( NULL );
    bCleanupCOM = SUCCEEDED( hr );

    // Create WMI
    hr = CoCreateInstance( __uuidof( WbemLocator ),
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           __uuidof( IWbemLocator ),
                           ( LPVOID* )&pIWbemLocator );
    if( FAILED( hr ) || pIWbemLocator == NULL )
        goto LCleanup;

    // Create BSTRs for WMI
    bstrNamespace = SysAllocString( L"\\\\.\\root\\cimv2" ); if( bstrNamespace == NULL ) goto LCleanup;
    bstrDeviceID = SysAllocString( L"DeviceID" );           if( bstrDeviceID == NULL )  goto LCleanup;
    bstrClassName = SysAllocString( L"Win32_PNPEntity" );    if( bstrClassName == NULL ) goto LCleanup;

    // Connect to WMI 
    hr = pIWbemLocator->ConnectServer( bstrNamespace, NULL, NULL, 0L,
                                       0L, NULL, NULL, &pIWbemServices );
    if( FAILED( hr ) || pIWbemServices == NULL )
        goto LCleanup;

    // Switch security level to IMPERSONATE
    CoSetProxyBlanket( pIWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                       RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0 );

    // Get list of Win32_PNPEntity devices
    hr = pIWbemServices->CreateInstanceEnum( bstrClassName, 0, NULL, &pEnumDevices );
    if( FAILED( hr ) || pEnumDevices == NULL )
        goto LCleanup;

    // Loop over all devices
    for(; ; )
    {
        // Get 20 at a time
        hr = pEnumDevices->Next( 10000, 20, pDevices, &uReturned );
        if( FAILED( hr ) )
            goto LCleanup;
        if( uReturned == 0 )
            break;

        for( iDevice = 0; iDevice < uReturned; iDevice++ )
        {
            // For each device, get its device ID
            hr = pDevices[iDevice]->Get( bstrDeviceID, 0L, &var, NULL, NULL );
            if( SUCCEEDED( hr ) && var.vt == VT_BSTR && var.bstrVal != NULL )
            {
                // Check if the device ID contains "IG_".  If it does, then it’s an XInput device
                // Unfortunately this information can not be found by just using DirectInput 
                if( wcsstr( var.bstrVal, L"IG_" ) )
                {
                    // If it does, then get the VID/PID from var.bstrVal
                    DWORD dwPid = 0, dwVid = 0;
                    WCHAR* strVid = wcsstr( var.bstrVal, L"VID_" );
                    if( strVid && swscanf( strVid, L"VID_%4X", &dwVid ) != 1 )
                        dwVid = 0;
                    WCHAR* strPid = wcsstr( var.bstrVal, L"PID_" );
                    if( strPid && swscanf( strPid, L"PID_%4X", &dwPid ) != 1 )
                        dwPid = 0;

                    DWORD dwVidPid = MAKELONG( dwVid, dwPid );

                    // Add the VID/PID to a linked list
                    XINPUT_DEVICE_NODE* pNewNode = new XINPUT_DEVICE_NODE;
                    if( pNewNode )
                    {
                        pNewNode->dwVidPid = dwVidPid;
                        pNewNode->pNext = g_pXInputDeviceList;
                        g_pXInputDeviceList = pNewNode;
                    }
                }
            }
            SAFE_RELEASE( pDevices[iDevice] );
        }
    }

LCleanup:
    if( bstrNamespace )
        SysFreeString( bstrNamespace );
    if( bstrDeviceID )
        SysFreeString( bstrDeviceID );
    if( bstrClassName )
        SysFreeString( bstrClassName );
    for( iDevice = 0; iDevice < 20; iDevice++ )
    SAFE_RELEASE( pDevices[iDevice] );
    SAFE_RELEASE( pEnumDevices );
    SAFE_RELEASE( pIWbemLocator );
    SAFE_RELEASE( pIWbemServices );

    return hr;
}


//-----------------------------------------------------------------------------
// Returns true if the DirectInput device is also an XInput device.
// Call SetupForIsXInputDevice() before, and CleanupForIsXInputDevice() after
//-----------------------------------------------------------------------------
bool IsXInputDevice( const GUID* pGuidProductFromDirectInput )
{
    // Check each xinput device to see if this device's vid/pid matches
    XINPUT_DEVICE_NODE* pNode = g_pXInputDeviceList;
    while( pNode )
    {
        if( pNode->dwVidPid == pGuidProductFromDirectInput->Data1 )
            return true;
        pNode = pNode->pNext;
    }

    return false;
}


//-----------------------------------------------------------------------------
// Cleanup needed for IsXInputDevice()
//-----------------------------------------------------------------------------
void CleanupForIsXInputDevice()
{
    // Cleanup linked list
    XINPUT_DEVICE_NODE* pNode = g_pXInputDeviceList;
    while( pNode )
    {
        XINPUT_DEVICE_NODE* pDelete = pNode;
        pNode = pNode->pNext;
        SAFE_DELETE( pDelete );
    }
}



//-----------------------------------------------------------------------------
// Name: EnumJoysticksCallback()
// Desc: Called once for each enumerated Joystick. If we find one, create a
//       device interface on it so we can play with it.
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance,
                                     VOID* pContext )
{
    DI_ENUM_CONTEXT* pEnumContext = ( DI_ENUM_CONTEXT* )pContext;
    HRESULT hr;

    if( g_bFilterOutXinputDevices && IsXInputDevice( &pdidInstance->guidProduct ) )
        return DIENUM_CONTINUE;

    // Skip anything other than the perferred Joystick device as defined by the control panel.  
    // Instead you could store all the enumerated Joysticks and let the user pick.
    if( pEnumContext->bPreferredJoyCfgValid &&
        !IsEqualGUID( pdidInstance->guidInstance, pEnumContext->pPreferredJoyCfg->guidInstance ) )
        return DIENUM_CONTINUE; // CONTINUE: search the desired controller repeatedly.

    // Obtain an interface to the enumerated Joystick.
    hr = g_pDI->CreateDevice( pdidInstance->guidInstance, &g_pJoystick, NULL );

    // If it failed, then we can't use this Joystick. (Maybe the user unplugged
    // it while we were in the middle of enumerating it.)
    if( FAILED( hr ) )
        return DIENUM_CONTINUE;

	// Store detected Joystick.
	if (idx_deviceInfo < num_maxDeviceInfo)
	{
		wcscpy(g_deviceInfo[idx_deviceInfo].productName, pdidInstance->tszProductName);
		g_deviceInfo[idx_deviceInfo++].guidInstance = pdidInstance->guidInstance;
		return DIENUM_CONTINUE;
	}
	else
		return DIENUM_STOP; // Stop enumeration
}




//-----------------------------------------------------------------------------
// Name: EnumObjectsCallback()
// Desc: Callback function for enumerating objects (axes, buttons, POVs) on a 
//       Joystick. This function enables user interface elements for objects
//       that are found to exist, and scales axes min/max values.
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumObjectsCallback( const DIDEVICEOBJECTINSTANCE* pdidoi,
                                   VOID* pContext )
{
    HWND hDlg = ( HWND )pContext;

    static int nSliderCount = 0;  // Number of returned slider controls
    static int nPOVCount = 0;     // Number of returned POV controls

    // For axes that are returned, set the DIPROP_RANGE property for the
    // enumerated axis in order to scale min/max values.
    if( pdidoi->dwType & DIDFT_AXIS )
    {
        DIPROPRANGE diprg;
        diprg.diph.dwSize = sizeof( DIPROPRANGE );
        diprg.diph.dwHeaderSize = sizeof( DIPROPHEADER );
        diprg.diph.dwHow = DIPH_BYID;
        diprg.diph.dwObj = pdidoi->dwType; // Specify the enumerated axis
        diprg.lMin = -1000;
        diprg.lMax = +1000;

        // Set the range for the axis
        if( FAILED( g_pJoystick->SetProperty( DIPROP_RANGE, &diprg.diph ) ) )
            return DIENUM_STOP;

    }


    // Set the UI to reflect what objects the Joystick supports
    if( pdidoi->guidType == GUID_XAxis )
    {
        EnableWindow( GetDlgItem( hDlg, IDC_X_AXIS ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDC_X_AXIS_TEXT ), TRUE );
    }
    if( pdidoi->guidType == GUID_YAxis )
    {
        EnableWindow( GetDlgItem( hDlg, IDC_Y_AXIS ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDC_Y_AXIS_TEXT ), TRUE );
    }
    if( pdidoi->guidType == GUID_ZAxis )
    {
        EnableWindow( GetDlgItem( hDlg, IDC_Z_AXIS ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDC_Z_AXIS_TEXT ), TRUE );
    }
    if( pdidoi->guidType == GUID_RxAxis )
    {
        EnableWindow( GetDlgItem( hDlg, IDC_X_ROT ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDC_X_ROT_TEXT ), TRUE );
    }
    if( pdidoi->guidType == GUID_RyAxis )
    {
        EnableWindow( GetDlgItem( hDlg, IDC_Y_ROT ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDC_Y_ROT_TEXT ), TRUE );
    }
    if( pdidoi->guidType == GUID_RzAxis )
    {
        EnableWindow( GetDlgItem( hDlg, IDC_Z_ROT ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDC_Z_ROT_TEXT ), TRUE );
    }
    if( pdidoi->guidType == GUID_Slider )
    {
        switch( nSliderCount++ )
        {
            case 0 :
                EnableWindow( GetDlgItem( hDlg, IDC_SLIDER0 ), TRUE );
                EnableWindow( GetDlgItem( hDlg, IDC_SLIDER0_TEXT ), TRUE );
                break;

            case 1 :
                EnableWindow( GetDlgItem( hDlg, IDC_SLIDER1 ), TRUE );
                EnableWindow( GetDlgItem( hDlg, IDC_SLIDER1_TEXT ), TRUE );
                break;
        }
    }
    if( pdidoi->guidType == GUID_POV )
    {
        switch( nPOVCount++ )
        {
            case 0 :
                EnableWindow( GetDlgItem( hDlg, IDC_POV0 ), TRUE );
                EnableWindow( GetDlgItem( hDlg, IDC_POV0_TEXT ), TRUE );
                break;

            case 1 :
                EnableWindow( GetDlgItem( hDlg, IDC_POV1 ), TRUE );
                EnableWindow( GetDlgItem( hDlg, IDC_POV1_TEXT ), TRUE );
                break;

            case 2 :
                EnableWindow( GetDlgItem( hDlg, IDC_POV2 ), TRUE );
                EnableWindow( GetDlgItem( hDlg, IDC_POV2_TEXT ), TRUE );
                break;

            case 3 :
                EnableWindow( GetDlgItem( hDlg, IDC_POV3 ), TRUE );
                EnableWindow( GetDlgItem( hDlg, IDC_POV3_TEXT ), TRUE );
                break;
        }
    }

    return DIENUM_CONTINUE;
}


//-----------------------------------------------------------------------------
// Name: UpdateInputState()
// Desc: Get the input device's state and display it.
//-----------------------------------------------------------------------------
HRESULT SwitchJoystick( HWND hDlg, GUID guidInstance )
{
	HRESULT hr;

	// Obtain an interface to the enumerated Joystick.
	hr = g_pDI->CreateDevice( guidInstance , &g_pJoystick, NULL );

	// Make sure we got a Joystick
	if (NULL == g_pJoystick || FAILED(hr))
	{
		MessageBox(NULL, TEXT("Joystick not found or failed to initialize. The program will now exit."),
			TEXT("DgoConverter"),
			MB_ICONERROR | MB_OK);
		EndDialog(hDlg, 0);
		return S_OK;
	}

	// Set the data format to "simple Joystick" - a predefined data format 
	//
	// A data format specifies which controls on a device we are interested in,
	// and how they should be reported. This tells DInput that we will be
	// passing a DIJOYSTATE2 structure to IDirectInputDevice::GetDeviceState().
	if (FAILED(hr = g_pJoystick->SetDataFormat(&c_dfDIJoystick2)))
		return hr;

	// Set the cooperative level to let DInput know how this device should
	// interact with the system and with other DInput applications.
	// MUST BE "DISCL_NONEXCLUSIVE" and "DISCL_BACKGROUND" mode
	if (FAILED(hr = g_pJoystick->SetCooperativeLevel(hDlg, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND)))
		return hr;

	// Enumerate the Joystick objects. The callback function enabled user
	// interface elements for objects that are found, and sets the min/max
	// values property for discovered axes.
	if (FAILED(hr = g_pJoystick->EnumObjects(EnumObjectsCallback,(VOID*)hDlg, DIDFT_ALL)))
		return hr;

	return S_OK;
}


//-----------------------------------------------------------------------------
// Name: UpdateInputState()
// Desc: Get the input device's state and display it.
//-----------------------------------------------------------------------------
HRESULT UpdateInputState( HWND hDlg, TCHAR* state, BUTTONCONFIG_BOOL* buttonState, KEYCONFIG* keymap, KEYTRIGGERINFO* triggermap, const bool isKeymapWithHoldDown )
{
    HRESULT hr;
    TCHAR strText[512] = {0}; // Device state text
	TCHAR masconText[512] = {0}; // Mascon state
	TCHAR buttonText[512] = {0}; // Other buttons state
    DIJOYSTATE2 js;           // DInput Joystick state 

    if( NULL == g_pJoystick )
        return S_OK;

    // Poll the device to read the current state
    hr = g_pJoystick->Poll();
    if( FAILED( hr ) )
    {
        // DInput is telling us that the input stream has been
        // interrupted. We aren't tracking any state between polls, so
        // we don't have any special reset that needs to be done. We
        // just re-acquire and try again.
        hr = g_pJoystick->Acquire();
        while( hr == DIERR_INPUTLOST )
            hr = g_pJoystick->Acquire();

        // hr may be DIERR_OTHERAPPHASPRIO or other errors.  This
        // may occur when the app is minimized or in the process of 
        // switching, so just try again later 
        return S_OK;
    }

    // Get the input's device state
    if( FAILED( hr = g_pJoystick->GetDeviceState( sizeof( DIJOYSTATE2 ), &js ) ) )
        return hr; // The device should have been acquired during the Poll()

    // Display Joystick state to dialog

    // Axes
    StringCchPrintf( strText, 512, TEXT( "%ld" ), js.lX );
    SetWindowText( GetDlgItem( hDlg, IDC_X_AXIS ), strText );
    StringCchPrintf( strText, 512, TEXT( "%ld" ), js.lY );
    SetWindowText( GetDlgItem( hDlg, IDC_Y_AXIS ), strText );
    StringCchPrintf( strText, 512, TEXT( "%ld" ), js.lZ );
    SetWindowText( GetDlgItem( hDlg, IDC_Z_AXIS ), strText );
    StringCchPrintf( strText, 512, TEXT( "%ld" ), js.lRx );
    SetWindowText( GetDlgItem( hDlg, IDC_X_ROT ), strText );
    StringCchPrintf( strText, 512, TEXT( "%ld" ), js.lRy );
    SetWindowText( GetDlgItem( hDlg, IDC_Y_ROT ), strText );
    StringCchPrintf( strText, 512, TEXT( "%ld" ), js.lRz );
    SetWindowText( GetDlgItem( hDlg, IDC_Z_ROT ), strText );

    // Slider controls
    StringCchPrintf( strText, 512, TEXT( "%ld" ), js.rglSlider[0] );
    SetWindowText( GetDlgItem( hDlg, IDC_SLIDER0 ), strText );
    StringCchPrintf( strText, 512, TEXT( "%ld" ), js.rglSlider[1] );
    SetWindowText( GetDlgItem( hDlg, IDC_SLIDER1 ), strText );

    // Points of view
    StringCchPrintf( strText, 512, TEXT( "%ld" ), js.rgdwPOV[0] );
    SetWindowText( GetDlgItem( hDlg, IDC_POV0 ), strText );
    StringCchPrintf( strText, 512, TEXT( "%ld" ), js.rgdwPOV[1] );
    SetWindowText( GetDlgItem( hDlg, IDC_POV1 ), strText );
    StringCchPrintf( strText, 512, TEXT( "%ld" ), js.rgdwPOV[2] );
    SetWindowText( GetDlgItem( hDlg, IDC_POV2 ), strText );
    StringCchPrintf( strText, 512, TEXT( "%ld" ), js.rgdwPOV[3] );
    SetWindowText( GetDlgItem( hDlg, IDC_POV3 ), strText );

	// Fill up text with expected master controller state
	StringCchPrintf(strText, 512, TEXT("%s"), state);
	SetWindowText(GetDlgItem(hDlg, IDC_MASCON_STATE), strText);

    // Fill up text with which buttons are pressed
    StringCchCopy( strText, 512, TEXT( "" ) );
    for( int i = 0; i < NUM_BUTTONS; i++ )
    {
        if( js.rgbButtons[i] & 0x80 )
        {
            TCHAR sz[128];
            StringCchPrintf( sz, 128, TEXT( "%02d " ), i );
            StringCchCat( strText, 512, sz );
			if ( triggermap->isExclusiveButton[i] )
			{
				TCHAR sz_m[128];
				StringCchPrintf(sz_m, 128, TEXT("%02d "), i);
				StringCchCat(masconText, 512, sz_m);
			}
			else
			{
				TCHAR sz_b[128];
				StringCchPrintf(sz_b, 128, TEXT("%02d "), i);
				StringCchCat(buttonText, 512, sz_b);
			}
        }
    }
    SetWindowText( GetDlgItem( hDlg, IDC_BUTTONS ), strText );

	makeKeyBoardOutput( masconText, buttonText, js, state, buttonState, triggermap, keymap, isKeymapWithHoldDown );

    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: makeKeyBoardOutput()
// Desc: Convert Joystick/Button Inputs to Key board outputs.
//-----------------------------------------------------------------------------
VOID makeKeyBoardOutput( const TCHAR* masconText, const TCHAR* buttonText, const DIJOYSTATE2 js, TCHAR* state, BUTTONCONFIG_BOOL* buttonState, KEYTRIGGERINFO* triggermap, KEYCONFIG* keymap, const bool isKeymapWithHoldDown )
{
	// KeyBoard Output Struct
	const int num_inputs = 6;
	INPUT inputs[num_inputs] = {};
	INPUT release[num_inputs] = {};
	ZeroMemory(inputs, sizeof(inputs));
	ZeroMemory(release, sizeof(release));
	for (int i = 0; i < num_inputs; i++)
	{
		inputs[i].ki.wVk = keymap->NUL;
		release[i].ki.wVk = keymap->NUL;
	}
	// inputs array index
	int idx_inputs = 0;
	int idx_release = 0;
	// is hold down
	bool isHoldDown = FALSE;
	DWORD holdDownTime = 70;

	makeMasconKeyBoardOutput( inputs, release, &idx_inputs, &idx_release, &isHoldDown, masconText, js, state, triggermap, keymap, isKeymapWithHoldDown );
	makeButtonKeyBoardOutput( inputs, release, &idx_inputs, &idx_release, buttonText, js, buttonState, triggermap, keymap );

	if (inputs[0].ki.wVk != keymap->NUL)
	{
		SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
		if (isHoldDown)
			Sleep(holdDownTime);
			// isHoldDown = FALSE;
		SendInput(ARRAYSIZE(release), release, sizeof(INPUT));
	}
}

//-----------------------------------------------------------------------------
// Name: makeMasconKeyBoardOutput()
// Desc: Convert Joystick Inputs to Key board outputs.
//-----------------------------------------------------------------------------
VOID makeMasconKeyBoardOutput( INPUT* inputs, INPUT* release, int* idx_inputs, int* idx_release, bool* isHoldDown, const TCHAR* strText, const DIJOYSTATE2 js, TCHAR* state, KEYTRIGGERINFO* triggermap, KEYCONFIG* keymap, const bool isKeymapWithHoldDown)
{
	const long x_axis = js.lX;
	// EB
	if ( validateMasconState( L"EB", strText, js, triggermap ) )
	{
		if (wcscmp(state, L"EB") != 0)
			inputs[*idx_inputs].ki.wVk = keymap->EB;
		*(state + 0) = 'E';
		*(state + 1) = 'B';
	}
	// B8
	else if ( validateMasconState( L"B8", strText, js, triggermap ) )
	{
		if (wcscmp(state, L"EB") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BDN;
		else if (wcscmp(state, L"B7") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BUP;
		*(state + 0) = 'B';
		*(state + 1) = '8';
	}
	// B7
	else if ( validateMasconState( L"B7", strText, js, triggermap ) )
	{
		if (wcscmp(state, L"B8") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BDN;
		else if (wcscmp(state, L"B6") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BUP;
		*(state + 0) = 'B';
		*(state + 1) = '7';
	}
	// B6
	else if ( validateMasconState( L"B6", strText, js, triggermap ) )
	{
		if (wcscmp(state, L"B7") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BDN;
		else if (wcscmp(state, L"B5") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BUP;
		*(state + 0) = 'B';
		*(state + 1) = '6';
	}
	// B5
	else if ( validateMasconState( L"B5", strText, js, triggermap ) )
	{
		if (wcscmp(state, L"B6") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BDN;
		else if (wcscmp(state, L"B4") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BUP;
		*(state + 0) = 'B';
		*(state + 1) = '5';
	}
	// B4
	else if ( validateMasconState( L"B4", strText, js, triggermap ) )
	{
		if (wcscmp(state, L"B5") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BDN;
		else if (wcscmp(state, L"B3") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BUP;
		*(state + 0) = 'B';
		*(state + 1) = '4';
	}
	// B3
	else if ( validateMasconState( L"B3", strText, js, triggermap ) )
	{
		if (wcscmp(state, L"B4") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BDN;
		else if (wcscmp(state, L"B2") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BUP;
		*(state + 0) = 'B';
		*(state + 1) = '3';
	}
	// B2
	else if ( validateMasconState( L"B2", strText, js, triggermap ) )
	{
		if (wcscmp(state, L"B3") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BDN;
		else if (wcscmp(state, L"B1") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BUP;
		*(state + 0) = 'B';
		*(state + 1) = '2';
	}
	// B1
	else if ( validateMasconState( L"B1", strText, js, triggermap ) )
	{
		if (wcscmp(state, L"B2") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BDN;
		else if (wcscmp(state, L"NT") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BUP;
		*(state + 0) = 'B';
		*(state + 1) = '1';
	}
	// NT
	else if ( validateMasconState( L"NT", strText, js, triggermap ) )
	{
		// NT
		if (wcscmp(state, L"B1") == 0)
		{
			inputs[*idx_inputs].ki.wVk = keymap->BNT;
			*(state + 0) = 'N';
			*(state + 1) = 'T';
		}
		else if (wcscmp(state, L"P1") == 0)
		{
			inputs[*idx_inputs].ki.wVk = keymap->PNT;
			*(state + 0) = 'N';
			*(state + 1) = 'T';
			if (isKeymapWithHoldDown)
				*isHoldDown = TRUE;
		}
		// P4 for JC-PS101U series where NT and P4 have the same trigger
		// TODO: adress this complex problem if there are solutions
		else if (wcscmp(state, L"P3") == 0)
		{
			inputs[*idx_inputs].ki.wVk = keymap->PDN;
			*(state + 0) = 'P';
			*(state + 1) = '4';
			if (isKeymapWithHoldDown)
				*isHoldDown = TRUE;
		}
		else if (wcscmp(state, L"P5") == 0)
		{
			if (!isKeymapWithHoldDown)
				inputs[*idx_inputs].ki.wVk = keymap->PUP;
			*(state + 0) = 'P';
			*(state + 1) = '4';
		}
	}
	// P1
	else if ( validateMasconState( L"P1", strText, js, triggermap ) )
	{
		if (wcscmp(state, L"NT") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->PDN;
		else if ((wcscmp(state, L"P2") == 0))
		{
			inputs[*idx_inputs].ki.wVk = keymap->PUP;
			if (isKeymapWithHoldDown)
				*isHoldDown = TRUE;
		}
		*(state + 0) = 'P';
		*(state + 1) = '1';
	}
	// P2
	else if ( validateMasconState( L"P2", strText, js, triggermap ) )
	{
		if (wcscmp(state, L"P1") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->PDN;
		else if (wcscmp(state, L"P3") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->PUP;
		*(state + 0) = 'P';
		*(state + 1) = '2';
		if (isKeymapWithHoldDown)
			*isHoldDown = TRUE;
	}
	// P3
	else if ( validateMasconState( L"P3", strText, js, triggermap ) )
	{
		if (wcscmp(state, L"P2") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->PDN;
		else if (wcscmp(state, L"P4") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->PUP;
		*(state + 0) = 'P';
		*(state + 1) = '3';
		if (isKeymapWithHoldDown)
			*isHoldDown = TRUE;
	}
	// P4
	else if ( validateMasconState( L"P4", strText, js, triggermap ) )
	{
		if (wcscmp(state, L"P3") == 0)
		{
			inputs[*idx_inputs].ki.wVk = keymap->PDN;
		    *(state + 0) = 'P';
			*(state + 1) = '4';
			if (isKeymapWithHoldDown)
				*isHoldDown = TRUE;
		}
		else if (wcscmp(state, L"P5") == 0)
		{
			if (!isKeymapWithHoldDown)
				inputs[*idx_inputs].ki.wVk = keymap->PUP;
			*(state + 0) = 'P';
			*(state + 1) = '4';
		}
	}
	// P5
	else if ( validateMasconState( L"P5", strText, js, triggermap ) )
	{
		if (!isKeymapWithHoldDown && wcscmp(state, L"P4") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->PDN;
		*(state + 0) = 'P';
		*(state + 1) = '5';
	}


	// if the sate of mascon is changed 
	if ( inputs[*idx_inputs].ki.wVk != keymap->NUL )
	{
		// input[0]
		inputs[*idx_inputs].type = INPUT_KEYBOARD;
		// input[1]
		release[*idx_release].ki.wVk = inputs[(*idx_inputs)++].ki.wVk;
		release[*idx_release].type = INPUT_KEYBOARD;
		release[(*idx_release)++].ki.dwFlags = KEYEVENTF_KEYUP; // *idx++ cause a bug (*idx++ regards as *(idx++), not (*idx)++), so use (*idx)++
	}
}


//-----------------------------------------------------------------------------
// Name: makeButtonKeyBoardOutput()
// Desc: Convert Joystick Button Inputs to Key board outputs.
//-----------------------------------------------------------------------------
VOID makeButtonKeyBoardOutput( INPUT* inputs, INPUT* release, int* idx_inputs, int* idx_release, const TCHAR* strText, const DIJOYSTATE2 js, BUTTONCONFIG_BOOL* buttonState, KEYTRIGGERINFO* triggermap, KEYCONFIG* keymap )
{
	// C button
	if ( validateMasconState( L"C", strText, js, triggermap ) )
	{
		if ( !(buttonState->C) )
		{
			inputs[*idx_inputs].type = INPUT_KEYBOARD;
			inputs[(*idx_inputs)++].ki.wVk = keymap->C;
			buttonState->C = TRUE;
		}
	}
	else if ( buttonState->C == TRUE )
	{
		inputs[*idx_inputs].type = INPUT_KEYBOARD;
		inputs[(*idx_inputs)++].ki.wVk = keymap->C; // Press a key first, or the key will not be released.

		release[*idx_release].type = INPUT_KEYBOARD;
		release[*idx_release].ki.wVk = keymap->C;
		release[(*idx_release)++].ki.dwFlags = KEYEVENTF_KEYUP;
		buttonState->C = FALSE;
	}

//-----------------------------------------------------------------------------
// Name: makeButtonInputArray()
// Desc: Make input array for buttons.
//-----------------------------------------------------------------------------
VOID makeButtonInputArray( INPUT* inputs, INPUT* release, int* idx_inputs, int* idx_release, WORD wVk, bool* thisButtonState, bool isHoldButton, bool isValidThisButton ) {
	if ( isHoldButton ) {
		if ( isValidThisButton )
		{
			if ( !(*thisButtonState) )
			{
				inputs[*idx_inputs].type = INPUT_KEYBOARD;
				inputs[(*idx_inputs)++].ki.wVk = wVk;
				*thisButtonState = TRUE;
			}
		}
		else if ( (*thisButtonState) == TRUE )
		{
			//inputs[*idx_inputs].type = INPUT_KEYBOARD;
			inputs[(*idx_inputs)++].ki.wVk = wVk; // Press a key first, or the key will not be released.

			release[*idx_release].type = INPUT_KEYBOARD;
			release[*idx_release].ki.wVk = wVk;
			release[(*idx_release)++].ki.dwFlags = KEYEVENTF_KEYUP;
			*thisButtonState = FALSE;
		}
	}
	else {
		// isHoldButton == FALSE will be deprecated.
		if ( isValidThisButton )
		{
			inputs[*idx_inputs].type = INPUT_KEYBOARD;
			inputs[*idx_inputs].ki.wVk = wVk;

			release[*idx_release].ki.wVk = inputs[(*idx_inputs)++].ki.wVk;
			release[*idx_release].type = INPUT_KEYBOARD;
			release[(*idx_release)++].ki.dwFlags = KEYEVENTF_KEYUP; // *idx++ cause a bug (*idx++ regards as *(idx++), not (*idx)++), so use (*idx)++
			*thisButtonState = FALSE;
		}
	}
}


//-----------------------------------------------------------------------------
// Name: validateMasconState()
// Desc: Validate Mascon Inputs based on a trigger map.
//-----------------------------------------------------------------------------
bool validateMasconState(WCHAR* validateState, const TCHAR* buttonStrText, const DIJOYSTATE2 js, KEYTRIGGERINFO* triggermap)
{
	if ( wcscmp( validateState, L"EB" ) == 0 )
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.EB, triggermap->AX_Y.EB, triggermap->AX_Z.EB, triggermap->BT.EB };
		return validateMasconInputs(js, buttonStrText, triggerValues);
	}
	if ( wcscmp( validateState, L"B8" ) == 0 )
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.B8, triggermap->AX_Y.B8, triggermap->AX_Z.B8, triggermap->BT.B8 };
		return validateMasconInputs(js, buttonStrText, triggerValues);
	}
	if ( wcscmp( validateState, L"B7" ) == 0 )
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.B7, triggermap->AX_Y.B7, triggermap->AX_Z.B7, triggermap->BT.B7 };
		return validateMasconInputs(js, buttonStrText, triggerValues);
	}
	if ( wcscmp( validateState, L"B6" ) == 0 )
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.B6, triggermap->AX_Y.B6, triggermap->AX_Z.B6, triggermap->BT.B6 };
		return validateMasconInputs(js, buttonStrText, triggerValues);
	}
	if ( wcscmp( validateState, L"B5" ) == 0 )
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.B5, triggermap->AX_Y.B5, triggermap->AX_Z.B5, triggermap->BT.B5 };
		return validateMasconInputs(js, buttonStrText, triggerValues);
	}
	if ( wcscmp( validateState, L"B4" ) == 0 )
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.B4, triggermap->AX_Y.B4, triggermap->AX_Z.B4, triggermap->BT.B4 };
		return validateMasconInputs(js, buttonStrText, triggerValues);
	}
	if ( wcscmp( validateState, L"B3" ) == 0 )
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.B3, triggermap->AX_Y.B3, triggermap->AX_Z.B3, triggermap->BT.B3 };
		return validateMasconInputs(js, buttonStrText, triggerValues);
	}
	if ( wcscmp( validateState, L"B2" ) == 0 )
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.B2, triggermap->AX_Y.B2, triggermap->AX_Z.B2, triggermap->BT.B2 };
		return validateMasconInputs(js, buttonStrText, triggerValues);
	}
	if ( wcscmp( validateState, L"B1" ) == 0 )
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.B1, triggermap->AX_Y.B1, triggermap->AX_Z.B1, triggermap->BT.B1 };
		return validateMasconInputs(js, buttonStrText, triggerValues);
	}
	if ( wcscmp( validateState, L"NT" ) == 0 )
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.NT, triggermap->AX_Y.NT, triggermap->AX_Z.NT, triggermap->BT.NT };
		return validateMasconInputs(js, buttonStrText, triggerValues);
	}
	if ( wcscmp( validateState, L"P1" ) == 0 )
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.P1, triggermap->AX_Y.P1, triggermap->AX_Z.P1, triggermap->BT.P1 };
		return validateMasconInputs(js, buttonStrText, triggerValues);
	}
	if ( wcscmp( validateState, L"P2" ) == 0 )
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.P2, triggermap->AX_Y.P2, triggermap->AX_Z.P2, triggermap->BT.P2 };
		return validateMasconInputs(js, buttonStrText, triggerValues);
	}
	if ( wcscmp( validateState, L"P3" ) == 0 )
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.P3, triggermap->AX_Y.P3, triggermap->AX_Z.P3, triggermap->BT.P3 };
		return validateMasconInputs(js, buttonStrText, triggerValues);
	}
	if ( wcscmp( validateState, L"P4" ) == 0 )
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.P4, triggermap->AX_Y.P4, triggermap->AX_Z.P4, triggermap->BT.P4 };
		return validateMasconInputs(js, buttonStrText, triggerValues);
	}
	if ( wcscmp( validateState, L"P5" ) == 0 )
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.P5, triggermap->AX_Y.P5, triggermap->AX_Z.P5, triggermap->BT.P5 };
		return validateMasconInputs(js, buttonStrText, triggerValues);
	}
	if ( wcscmp( validateState, L"A" ) == 0 )
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.A, triggermap->AX_Y.A, triggermap->AX_Z.A, triggermap->BT.A };
		return validateNonMasconInputs(js, buttonStrText, triggerValues);
	}
	if ( wcscmp( validateState, L"B" ) == 0 )
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.B, triggermap->AX_Y.B, triggermap->AX_Z.B, triggermap->BT.B };
		return validateNonMasconInputs(js, buttonStrText, triggerValues);
	}
	if ( wcscmp( validateState, L"C" ) == 0 )
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.C, triggermap->AX_Y.C, triggermap->AX_Z.C, triggermap->BT.C };
		return validateNonMasconInputs(js, buttonStrText, triggerValues);
	}
	if ( wcscmp( validateState, L"START" ) == 0 )
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.START, triggermap->AX_Y.START, triggermap->AX_Z.START, triggermap->BT.START };
		return validateNonMasconInputs(js, buttonStrText, triggerValues);
	}
	if ( wcscmp( validateState, L"SELECT" ) == 0 )
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.SELECT, triggermap->AX_Y.SELECT, triggermap->AX_Z.SELECT, triggermap->BT.SELECT };
		return validateNonMasconInputs(js, buttonStrText, triggerValues);
	}
	return FALSE;
}


//-----------------------------------------------------------------------------
// Name: validateMasconInputs()
// Desc: Validate Input (AX_X, AX_Y, AX_Z, Buttons etc.) for Mascon.
//-----------------------------------------------------------------------------
bool validateMasconInputs(const DIJOYSTATE2 js, const TCHAR* buttonStrText, TRIGGERVALUES triggerValues)
{
	validateInputResult result[NUM_VALIDATE] = { NOT_VALIDATED, NOT_VALIDATED, NOT_VALIDATED, NOT_VALIDATED }; // { AX_X, AX_Y, AX_Z, BT }

	// TODO: Œ^•ÏŠ·‰Â”\‚©Šm”F
	// Validate AX_X
	if ( wcscmp( triggerValues.AX_X , NOT_IN_USE ) == 0 )
		result[0] = NOT_ASSIGNED;
	else if ( js.lX == _wtol(triggerValues.AX_X) )
		result[0] = VALIDATED;

	// Validate AX_Y
	if ( wcscmp( triggerValues.AX_Y, NOT_IN_USE ) == 0 )
		result[1] = NOT_ASSIGNED;
	else if ( js.lY == _wtol(triggerValues.AX_Y) )
		result[1] = VALIDATED;

	// Validate AX_Z
	if ( wcscmp( triggerValues.AX_Z, NOT_IN_USE ) == 0 )
		result[2] = NOT_ASSIGNED;
	else if ( js.lZ == _wtol(triggerValues.AX_Z) )
		result[2] = VALIDATED;

	// Validate BT (Buttons)
	if ( wcscmp( triggerValues.BT, NOT_IN_USE ) == 0 )
		result[3] = NOT_ASSIGNED;
	else if ( wcscmp( buttonStrText, triggerValues.BT ) == 0 )
		result[3] = VALIDATED;

	// Judgement
	int NUM_NOT_ASSIGNED = 0;
	for (int i = 0; i < NUM_VALIDATE; i++)
	{
		if (result[i] == NOT_VALIDATED)
			return FALSE;
		if ( result[i] == NOT_ASSIGNED )
			NUM_NOT_ASSIGNED++;
		if (NUM_NOT_ASSIGNED == NUM_VALIDATE)
			return FALSE;
	}
	return TRUE;
}


//-----------------------------------------------------------------------------
// Name: validateButtonState()
// Desc: Validate Joystick Button Inputs based on a trigger map.
//-----------------------------------------------------------------------------
bool validateButtonState(WCHAR* validateState, const TCHAR* buttonStrText, const DIJOYSTATE2 js, KEYTRIGGERINFO* triggermap)
{
	if (wcscmp(validateState, L"ESC") == 0)
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.ESC, triggermap->AX_Y.ESC, triggermap->AX_Z.ESC, triggermap->POV.ESC, triggermap->BT.ESC };
		return validateButtonInputs(js, buttonStrText, triggerValues);
	}
	if (wcscmp(validateState, L"ENTER") == 0)
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.ENTER, triggermap->AX_Y.ENTER, triggermap->AX_Z.ENTER, triggermap->POV.ENTER, triggermap->BT.ENTER };
		return validateButtonInputs(js, buttonStrText, triggerValues);
	}
	if (wcscmp(validateState, L"UP") == 0)
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.UP, triggermap->AX_Y.UP, triggermap->AX_Z.UP, triggermap->POV.UP, triggermap->BT.UP };
		return validateButtonInputs(js, buttonStrText, triggerValues);
	}
	if (wcscmp(validateState, L"DOWN") == 0)
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.DOWN, triggermap->AX_Y.DOWN, triggermap->AX_Z.DOWN, triggermap->POV.DOWN, triggermap->BT.DOWN };
		return validateButtonInputs(js, buttonStrText, triggerValues);
	}
	if (wcscmp(validateState, L"LEFT") == 0)
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.LEFT, triggermap->AX_Y.LEFT, triggermap->AX_Z.LEFT, triggermap->POV.LEFT, triggermap->BT.LEFT };
		return validateButtonInputs(js, buttonStrText, triggerValues);
	}
	if (wcscmp(validateState, L"RIGHT") == 0)
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.RIGHT, triggermap->AX_Y.RIGHT, triggermap->AX_Z.RIGHT, triggermap->POV.RIGHT, triggermap->BT.RIGHT };
		return validateButtonInputs(js, buttonStrText, triggerValues);
	}
	if (wcscmp(validateState, L"ELECHORN") == 0)
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.ELECHORN, triggermap->AX_Y.ELECHORN, triggermap->AX_Z.ELECHORN, triggermap->POV.ELECHORN, triggermap->BT.ELECHORN };
		return validateButtonInputs(js, buttonStrText, triggerValues);
	}
	if (wcscmp(validateState, L"HORN") == 0)
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.HORN, triggermap->AX_Y.HORN, triggermap->AX_Z.HORN, triggermap->POV.HORN, triggermap->BT.HORN };
		return validateButtonInputs(js, buttonStrText, triggerValues);
	}
	if (wcscmp(validateState, L"BUZZER") == 0)
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.BUZZER, triggermap->AX_Y.BUZZER, triggermap->AX_Z.BUZZER, triggermap->POV.BUZZER, triggermap->BT.BUZZER };
		return validateButtonInputs(js, buttonStrText, triggerValues);
	}
	if (wcscmp(validateState, L"EBREST") == 0)
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.EBREST, triggermap->AX_Y.EBREST, triggermap->AX_Z.EBREST, triggermap->POV.EBREST, triggermap->BT.EBREST };
		return validateButtonInputs(js, buttonStrText, triggerValues);
	}

	if (wcscmp(validateState, L"ATS.CONF") == 0)
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.ATS.CONF, triggermap->AX_Y.ATS.CONF, triggermap->AX_Z.ATS.CONF, triggermap->POV.ATS.CONF, triggermap->BT.ATS.CONF };
		return validateButtonInputs(js, buttonStrText, triggerValues);
	}
	if (wcscmp(validateState, L"ATS.RESNORM") == 0)
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.ATS.RESNORM, triggermap->AX_Y.ATS.RESNORM, triggermap->AX_Z.ATS.RESNORM, triggermap->POV.ATS.RESNORM, triggermap->BT.ATS.RESNORM };
		return validateButtonInputs(js, buttonStrText, triggerValues);
	}
	if (wcscmp(validateState, L"ATS.RESEMER") == 0)
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.ATS.RESEMER, triggermap->AX_Y.ATS.RESEMER, triggermap->AX_Z.ATS.RESEMER, triggermap->POV.ATS.RESEMER, triggermap->BT.ATS.RESEMER };
		return validateButtonInputs(js, buttonStrText, triggerValues);
	}

	if (wcscmp(validateState, L"TASC") == 0)
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.TASC, triggermap->AX_Y.TASC, triggermap->AX_Z.TASC, triggermap->POV.TASC, triggermap->BT.TASC };
		return validateButtonInputs(js, buttonStrText, triggerValues);
	}
	if (wcscmp(validateState, L"INCHING") == 0)
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.INCHING, triggermap->AX_Y.INCHING, triggermap->AX_Z.INCHING, triggermap->POV.INCHING, triggermap->BT.INCHING };
		return validateButtonInputs(js, buttonStrText, triggerValues);
	}
	if (wcscmp(validateState, L"CRUISE") == 0)
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.CRUISE, triggermap->AX_Y.CRUISE, triggermap->AX_Z.CRUISE, triggermap->POV.CRUISE, triggermap->BT.CRUISE };
		return validateButtonInputs(js, buttonStrText, triggerValues);
	}
	if (wcscmp(validateState, L"SUPB") == 0)
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.SUPB, triggermap->AX_Y.SUPB, triggermap->AX_Z.SUPB, triggermap->POV.SUPB, triggermap->BT.SUPB };
		return validateButtonInputs(js, buttonStrText, triggerValues);
	}
	if (wcscmp(validateState, L"SLOPESTAT") == 0)
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.SLOPESTAT, triggermap->AX_Y.SLOPESTAT, triggermap->AX_Z.SLOPESTAT, triggermap->POV.SLOPESTAT, triggermap->BT.SLOPESTAT };
		return validateButtonInputs(js, buttonStrText, triggerValues);
	}
	if (wcscmp(validateState, L"INFO") == 0)
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.INFO, triggermap->AX_Y.INFO, triggermap->AX_Z.INFO, triggermap->POV.INFO, triggermap->BT.INFO };
		return validateButtonInputs(js, buttonStrText, triggerValues);
	}
	if (wcscmp(validateState, L"NEXTVIEW") == 0)
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.NEXTVIEW, triggermap->AX_Y.NEXTVIEW, triggermap->AX_Z.NEXTVIEW, triggermap->POV.NEXTVIEW, triggermap->BT.NEXTVIEW };
		return validateButtonInputs(js, buttonStrText, triggerValues);
	}
	if (wcscmp(validateState, L"BUP") == 0)
	{
		TRIGGERVALUES triggerValues = { triggermap->AX_X.BUP, triggermap->AX_Y.BUP, triggermap->AX_Z.BUP, triggermap->POV.BUP, triggermap->BT.BUP };
		return validateButtonInputs(js, buttonStrText, triggerValues);
	}
	return FALSE;
}


//-----------------------------------------------------------------------------
// Name: validateButtonInputs()
// Desc: Validate Input (AX_X, AX_Y, AX_Z, Buttons etc.) for buttons.
//-----------------------------------------------------------------------------
bool validateButtonInputs(const DIJOYSTATE2 js, const TCHAR* buttonStrText, TRIGGERVALUES triggerValues)
{
	validateInputResult result[NUM_VALIDATE] = { NOT_VALIDATED, NOT_VALIDATED, NOT_VALIDATED, NOT_VALIDATED }; // { AX_X, AX_Y, AX_Z, BT }

	// TODO: Œ^•ÏŠ·‰Â”\‚©Šm”F
	// Validate AX_X
	if ( wcscmp( triggerValues.AX_X, NOT_IN_USE ) == 0 )
		result[0] = NOT_ASSIGNED;
	else if (js.lX == _wtol(triggerValues.AX_X))
		result[0] = VALIDATED;

	// Validate AX_Y
	if ( wcscmp( triggerValues.AX_Y, NOT_IN_USE ) == 0 )
		result[1] = NOT_ASSIGNED;
	else if (js.lY == _wtol(triggerValues.AX_Y))
		result[1] = VALIDATED;

	// Validate AX_Z
	if ( wcscmp( triggerValues.AX_Z, NOT_IN_USE ) == 0 )
		result[2] = NOT_ASSIGNED;
	else if (js.lZ == _wtol(triggerValues.AX_Z))
		result[2] = VALIDATED;

	// Validate POV
	if (wcscmp(triggerValues.POV, NOT_IN_USE) == 0)
		result[3] = NOT_ASSIGNED;
	else if (js.rgdwPOV[0] == _wtol(triggerValues.POV))
		result[3] = VALIDATED;

	// Validate BT (Buttons)
	if ( wcscmp( triggerValues.BT, NOT_IN_USE ) == 0 )
		result[4] = NOT_ASSIGNED;
	else if ( wcschr( buttonStrText, *(triggerValues.BT) ) != NULL )
		result[4] = VALIDATED;

	// Judgement
	int NUM_NOT_ASSIGNED = 0;
	for (int i = 0; i < NUM_VALIDATE; i++)
	{
		if (result[i] == NOT_VALIDATED)
			return FALSE;
		if (result[i] == NOT_ASSIGNED)
			NUM_NOT_ASSIGNED++;
		if (NUM_NOT_ASSIGNED == NUM_VALIDATE)
			return FALSE;
	}
	return TRUE;
}


//-----------------------------------------------------------------------------
// Name: FreeDirectInput()
// Desc: Initialize the DirectInput variables.
//-----------------------------------------------------------------------------
VOID FreeDirectInput()
{
    // Unacquire the device one last time just in case 
    // the app tried to exit while the device is still acquired.
    if( g_pJoystick )
        g_pJoystick->Unacquire();

    // Release any DirectInput objects.
    SAFE_RELEASE( g_pJoystick );
    SAFE_RELEASE( g_pDI );

	// Delete any DgoConverter structs.
}



