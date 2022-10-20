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
HRESULT SwitchJoystick(HWND hDlg, GUID guidInstance);
HRESULT UpdateInputState(HWND hDlg, TCHAR* state, BUTTONSTATE* buttonState, KEYCONFIG* keymap, const bool isKeymapWithHoldDown);
VOID makeKeyBoardOutput(const TCHAR* masconText, const TCHAR* buttonText, const long x_axis, TCHAR* state, BUTTONSTATE* buttonState, KEYCONFIG* keymap, const bool isKeymapWithHoldDown);
VOID makeMasconKeyBoardOutput(INPUT* inputs, INPUT* release, int* idx_inputs, int* idx_release, bool* isHoldDown, const TCHAR* strText, const long x_axis, TCHAR* state, KEYCONFIG* keymap, const bool isKeymapWithHoldDown);
VOID makeButtonKeyBoardOutput(INPUT* inputs, INPUT* release, int* idx_inputs, int* idx_release, const TCHAR* strText, BUTTONSTATE* buttonState, KEYCONFIG* keymap);

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
WPARAM index; // Selection item index
WPARAM idx_currentDevice; // Selection item index for devices




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

            // Set a timer to go off 30 times a second. At every timer message
            // the input device will be read
            SetTimer( hDlg, 0, 1000 / 30, NULL );
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
            if( FAILED( UpdateInputState( hDlg, state, &buttonState, &keyinfo[index].KEY, wcscmp( keyinfo[index].NAME, L"HMMSIM METRO" ) == 0 ) ) )
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
HRESULT UpdateInputState( HWND hDlg, TCHAR* state, BUTTONSTATE* buttonState, KEYCONFIG* keymap, const bool isKeymapWithHoldDown)
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
    for( int i = 0; i < 128; i++ )
    {
        if( js.rgbButtons[i] & 0x80 )
        {
            TCHAR sz[128];
            StringCchPrintf( sz, 128, TEXT( "%02d " ), i );
            StringCchCat( strText, 512, sz );
			if ( i == 0 || i == 4 || i == 5 || i == 6 || i == 7 )
			{
				TCHAR sz_m[128];
				StringCchPrintf(sz_m, 128, TEXT("%02d "), i);
				StringCchCat(masconText, 512, sz_m);
			}
			else if ( i == 1 || i == 2 || i == 3 || i == 8 || i == 9 )
			{
				TCHAR sz_b[128];
				StringCchPrintf(sz_b, 128, TEXT("%02d "), i);
				StringCchCat(buttonText, 512, sz_b);
			}
        }
    }
    SetWindowText( GetDlgItem( hDlg, IDC_BUTTONS ), strText );

	makeKeyBoardOutput( masconText, buttonText, js.lX, state, buttonState, keymap, isKeymapWithHoldDown );

    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: makeKeyBoardOutput()
// Desc: Convert Joystick/Button Inputs to Key board outputs.
//-----------------------------------------------------------------------------
VOID makeKeyBoardOutput(const TCHAR* masconText, const TCHAR* buttonText, const long x_axis, TCHAR* state, BUTTONSTATE* buttonState, KEYCONFIG* keymap, const bool isKeymapWithHoldDown)
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

	makeMasconKeyBoardOutput( inputs, release, &idx_inputs, &idx_release, &isHoldDown, masconText, x_axis, state, keymap, isKeymapWithHoldDown );
	makeButtonKeyBoardOutput( inputs, release, &idx_inputs, &idx_release, buttonText, buttonState, keymap );

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
VOID makeMasconKeyBoardOutput( INPUT* inputs, INPUT* release, int* idx_inputs, int* idx_release, bool* isHoldDown, const TCHAR* strText, const long x_axis, TCHAR* state, KEYCONFIG* keymap, const bool isKeymapWithHoldDown)
{
	// EB
	if (wcscmp(strText, L"") == 0)
	{
		if (wcscmp(state, L"EB") != 0)
			inputs[*idx_inputs].ki.wVk = keymap->EB;
		*(state + 0) = 'E';
		*(state + 1) = 'B';
	}
	// B8
	else if ( wcscmp(strText, L"04 07 ") == 0 )
	{
		if (wcscmp(state, L"EB") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BDN;
		else if (wcscmp(state, L"B7") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BUP;
		*(state + 0) = 'B';
		*(state + 1) = '8';
	}
	// B7
	else if ( wcscmp(strText, L"04 06 07 ") == 0 )
	{
		if (wcscmp(state, L"B8") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BDN;
		else if (wcscmp(state, L"B6") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BUP;
		*(state + 0) = 'B';
		*(state + 1) = '7';
	}
	// B6
	else if (wcscmp(strText, L"05 ") == 0)
	{
		if (wcscmp(state, L"B7") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BDN;
		else if (wcscmp(state, L"B5") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BUP;
		*(state + 0) = 'B';
		*(state + 1) = '6';
	}
	// B5
	else if (wcscmp(strText, L"05 06 ") == 0)
	{
		if (wcscmp(state, L"B6") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BDN;
		else if (wcscmp(state, L"B4") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BUP;
		*(state + 0) = 'B';
		*(state + 1) = '5';
	}
	// B4
	else if (wcscmp(strText, L"04 05 ") == 0)
	{
		if (wcscmp(state, L"B5") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BDN;
		else if (wcscmp(state, L"B3") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BUP;
		*(state + 0) = 'B';
		*(state + 1) = '4';
	}
	// B3
	else if (wcscmp(strText, L"04 05 06 ") == 0)
	{
		if (wcscmp(state, L"B4") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BDN;
		else if (wcscmp(state, L"B2") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BUP;
		*(state + 0) = 'B';
		*(state + 1) = '3';
	}
	// B2
	else if (wcscmp(strText, L"05 07 ") == 0)
	{
		if (wcscmp(state, L"B3") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BDN;
		else if (wcscmp(state, L"B1") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BUP;
		*(state + 0) = 'B';
		*(state + 1) = '2';
	}
	// B1
	else if (wcscmp(strText, L"05 06 07 ") == 0)
	{
		if (wcscmp(state, L"B2") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BDN;
		else if (wcscmp(state, L"NT") == 0)
			inputs[*idx_inputs].ki.wVk = keymap->BUP;
		*(state + 0) = 'B';
		*(state + 1) = '1';
	}
	// NT, P2, P4
	else if (wcscmp(strText, L"04 05 07 ") == 0)
	{
		// NT, P4 (x_axis = -1000)
		if ( x_axis == -1000 )
		{
			// NT
			if (wcscmp(state, L"B1") == 0 )
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
			// P4
			else if (wcscmp(state, L"P3") == 0)
			{
				inputs[*idx_inputs].ki.wVk = keymap->PDN;
				*(state + 0) = 'P';
				*(state + 1) = '4';
				if (isKeymapWithHoldDown)
					*isHoldDown = TRUE;
			}
			else if ( wcscmp(state, L"P5") == 0 )
			{
				if ( !isKeymapWithHoldDown )
					inputs[*idx_inputs].ki.wVk = keymap->PUP;
				*(state + 0) = 'P';
				*(state + 1) = '4';
			}
				
		}
		// P2 (x_axis = 1000)
		else if ((x_axis == 1000) )
		{
			if ( wcscmp(state, L"P1") == 0 )
				inputs[*idx_inputs].ki.wVk = keymap->PDN;
			else if ( wcscmp(state, L"P3") == 0 )
				inputs[*idx_inputs].ki.wVk = keymap->PUP;
			*(state + 0) = 'P';
			*(state + 1) = '2';
			if (isKeymapWithHoldDown)
				*isHoldDown = TRUE;
		}
	}
	// P1, P3
	else if (wcscmp(strText, L"00 04 05 07 ") == 0)
	{
		// P1 (x_axis = 1000)
		if ( (x_axis == 1000) )
		{
			if ( wcscmp(state, L"NT") == 0 )
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
		// P3 (x_axis = -1000)
		else if ( (x_axis == -1000)  )
		{
			if ( wcscmp(state, L"P2") == 0 )
				inputs[*idx_inputs].ki.wVk = keymap->PDN;
			else if ( wcscmp(state, L"P4") == 0 )
				inputs[*idx_inputs].ki.wVk = keymap->PUP;
			*(state + 0) = 'P';
			*(state + 1) = '3';
			if (isKeymapWithHoldDown)
				*isHoldDown = TRUE;
		}
		// P5 (x_axis = 24)
		else if ( x_axis == 24 || x_axis == -8) // some JC-PS101U returns values where x_axis == -8
		{
			if ( !isKeymapWithHoldDown && wcscmp(state, L"P4") == 0 )
				inputs[*idx_inputs].ki.wVk = keymap->PDN;
			*(state + 0) = 'P';
			*(state + 1) = '5';
		}
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
VOID makeButtonKeyBoardOutput(INPUT* inputs, INPUT* release, int* idx_inputs, int* idx_release, const TCHAR* strText, BUTTONSTATE* buttonState, KEYCONFIG* keymap)
{
	// C button
	if ( wcschr(strText, L'1') != NULL )
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

	// B button
	if ( wcschr(strText, L'2') != NULL )
	{
		if ( !(buttonState->B) )
		{
			inputs[*idx_inputs].type = INPUT_KEYBOARD;
			inputs[(*idx_inputs)++].ki.wVk = keymap->B;
			buttonState->B = TRUE;
		}
		
	}
	else if ( buttonState->B == TRUE )
	{
		inputs[*idx_inputs].type = INPUT_KEYBOARD;
		inputs[(*idx_inputs)++].ki.wVk = keymap->B; // Press a key first, or the key will not be released.

		release[*idx_release].type = INPUT_KEYBOARD;
		release[*idx_release].ki.wVk = keymap->B;
		release[(*idx_release)++].ki.dwFlags = KEYEVENTF_KEYUP;
		buttonState->B = FALSE;
	}

	// A button
	if ( wcschr(strText, L'3') != NULL )
	{
		if ( !(buttonState->A) )
		{
			inputs[*idx_inputs].type = INPUT_KEYBOARD;
			inputs[(*idx_inputs)++].ki.wVk = keymap->A;
			buttonState->A = TRUE;
		}
	}
	else if ( buttonState->A == TRUE )
	{
		inputs[*idx_inputs].type = INPUT_KEYBOARD;
		inputs[(*idx_inputs)++].ki.wVk = keymap->A; // Press a key first, or the key will not be released.

		release[*idx_release].type = INPUT_KEYBOARD;
		release[*idx_release].ki.wVk = keymap->A;
		release[(*idx_release)++].ki.dwFlags = KEYEVENTF_KEYUP;
		buttonState->A = FALSE;
	}

	// START button
	if ( wcschr(strText, L'8') != NULL)
	{
		if ( !(buttonState->START) )
		{
			inputs[*idx_inputs].type = INPUT_KEYBOARD;
			inputs[(*idx_inputs)++].ki.wVk = keymap->START;
			buttonState->START = TRUE;
		}
	}
	else if ( buttonState->START == TRUE )
	{
		inputs[*idx_inputs].type = INPUT_KEYBOARD;
		inputs[(*idx_inputs)++].ki.wVk = keymap->START; // Press a key first, or the key will not be released.

		release[*idx_release].type = INPUT_KEYBOARD;
		release[*idx_release].ki.wVk = keymap->START;
		release[(*idx_release)++].ki.dwFlags = KEYEVENTF_KEYUP;
		buttonState->START = FALSE;
	}

	// SELECT button
	if ( wcschr(strText, L'9') != NULL )
	{
		if ( !(buttonState->SELECT) )
		{
			inputs[*idx_inputs].type = INPUT_KEYBOARD;
			inputs[(*idx_inputs)++].ki.wVk = keymap->SELECT;
			buttonState->SELECT = TRUE;
		}
	}
	else if ( buttonState->SELECT == TRUE )
	{
		inputs[*idx_inputs].type = INPUT_KEYBOARD;
		inputs[(*idx_inputs)++].ki.wVk = keymap->SELECT; // Press a key first, or the key will not be released.

		release[*idx_release].type = INPUT_KEYBOARD;
		release[*idx_release].ki.wVk = keymap->SELECT;
		release[(*idx_release)++].ki.dwFlags = KEYEVENTF_KEYUP;
		buttonState->SELECT = FALSE;
	}

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



