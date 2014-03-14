// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "WindowsApplication.h"
#include "WindowsWindow.h"
#include "WindowsCursor.h"
#include "GenericApplicationMessageHandler.h"
#include "XInputInterface.h"
#include "IInputDeviceModule.h"
#include "IInputDevice.h"

#if WITH_EDITOR
#include "ModuleManager.h"
#include "Developer/Windows/VSAccessor/Public/VSAccessorModule.h"
#endif

// Allow Windows Platform types in the entire file.
#include "AllowWindowsPlatformTypes.h"
	#include "Ole2.h"
	#include <shlobj.h>
	#include <objbase.h>
	#include <SetupApi.h>

// This might not be defined by Windows when maintaining backwards-compatibility to pre-Vista builds
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL                  0x020E
#endif

DEFINE_LOG_CATEGORY(LogWindowsDesktop);

const FIntPoint FWindowsApplication::MinimizedWindowPosition(-32000,-32000);

FWindowsApplication* WindowApplication = NULL;

FWindowsApplication* FWindowsApplication::CreateWindowsApplication( const HINSTANCE InstanceHandle, const HICON IconHandle )
{
	WindowApplication = new FWindowsApplication( InstanceHandle, IconHandle );
	return WindowApplication;
}

FWindowsApplication::FWindowsApplication( const HINSTANCE HInstance, const HICON IconHandle )
	: GenericApplication( MakeShareable( new FWindowsCursor() ) )
	, InstanceHandle( HInstance )
	, bUsingHighPrecisionMouseInput( false )
	, XInput( XInputInterface::Create( MessageHandler ) )
	, CVarDeferMessageProcessing( 
		TEXT( "Slate.DeferWindowsMessageProcessing" ),
		bAllowedToDeferMessageProcessing,
		TEXT( "Whether windows message processing is deferred until tick or if they are processed immediately" ) )
	, bAllowedToDeferMessageProcessing( true )
	, bHasLoadedInputPlugins( false )
	, bInModalSizeLoop( false )

{
	// Disable the process from being showing "ghosted" not responding messages during slow tasks
	// This is a hack.  A more permanent solution is to make our slow tasks not block the editor for so long
	// that message pumping doesn't occur (which causes these messages).
	::DisableProcessWindowsGhosting();

	// Register the Win32 class for Slate windows and assign the application instance and icon
	const bool bClassRegistered = RegisterClass( InstanceHandle, IconHandle );

	// Initialize OLE for Drag and Drop support.
	OleInitialize( NULL );

	TextInputMethodSystem = MakeShareable( new FWindowsTextInputMethodSystem );
	if(!TextInputMethodSystem->Initialize())
	{
		TextInputMethodSystem.Reset();
	}

	// Get initial display metrics. (display information for existing desktop, before we start changing resolutions)
	GetDisplayMetrics(InitialDisplayMetrics);
}

bool FWindowsApplication::RegisterClass( const HINSTANCE HInstance, const HICON HIcon )
{
	WNDCLASS wc;
	FMemory::Memzero( &wc, sizeof(wc) );
	wc.style = CS_DBLCLKS; // We want to receive double clicks
	wc.lpfnWndProc = AppWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = HInstance;
	wc.hIcon = HIcon;
	wc.hCursor = NULL;	// We manage the cursor ourselves
	wc.hbrBackground = NULL; // Transparent
	wc.lpszMenuName = NULL;
	wc.lpszClassName = FWindowsWindow::AppWindowClass;

	if( !::RegisterClass( &wc ) )
	{
		//ShowLastError();

		// @todo Slate: Error message should be localized!
		MessageBox(NULL, TEXT("Window Registration Failed!"), TEXT("Error!"), MB_ICONEXCLAMATION | MB_OK);

		return false;
	}

	return true;
}

FWindowsApplication::~FWindowsApplication()
{
	TextInputMethodSystem->Terminate();

	::CoUninitialize();
	OleUninitialize();
}

TSharedRef< FGenericWindow > FWindowsApplication::MakeWindow() 
{ 
	return FWindowsWindow::Make(); 
}

void FWindowsApplication::InitializeWindow( const TSharedRef< FGenericWindow >& InWindow, const TSharedRef< FGenericWindowDefinition >& InDefinition, const TSharedPtr< FGenericWindow >& InParent, const bool bShowImmediately )
{
	const TSharedRef< FWindowsWindow > Window = StaticCastSharedRef< FWindowsWindow >( InWindow );
	const TSharedPtr< FWindowsWindow > ParentWindow = StaticCastSharedPtr< FWindowsWindow >( InParent );

	Windows.Add( Window );
	Window->Initialize( this, InDefinition, InstanceHandle, ParentWindow, bShowImmediately );
}

void FWindowsApplication::SetMessageHandler( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler )
{
	GenericApplication::SetMessageHandler(InMessageHandler);
	XInput->SetMessageHandler( InMessageHandler );

	TArray<IInputDeviceModule*> PluginImplementations = IModularFeatures::Get().GetModularFeatureImplementations<IInputDeviceModule>( IInputDeviceModule::GetModularFeatureName() );
	for( auto DeviceIt = ExternalInputDevices.CreateIterator(); DeviceIt; ++DeviceIt )
	{
		(*DeviceIt)->SetMessageHandler(InMessageHandler);
	}

}

FModifierKeysState FWindowsApplication::GetModifierKeys() const
{
	const bool bIsLeftShiftDown = ( ::GetAsyncKeyState( VK_LSHIFT ) & 0x8000 ) != 0;
	const bool bIsRightShiftDown = ( ::GetAsyncKeyState( VK_RSHIFT ) & 0x8000 ) != 0;
	const bool bIsLeftControlDown = ( ::GetAsyncKeyState( VK_LCONTROL ) & 0x8000 ) != 0;
	const bool bIsRightControlDown = ( ::GetAsyncKeyState( VK_RCONTROL ) & 0x8000 ) != 0;
	const bool bIsLeftAltDown = ( ::GetAsyncKeyState( VK_LMENU ) & 0x8000 ) != 0;
	const bool bIsRightAltDown = ( ::GetAsyncKeyState( VK_RMENU ) & 0x8000 ) != 0;

	return FModifierKeysState( bIsLeftShiftDown, bIsRightShiftDown, bIsLeftControlDown, bIsRightControlDown, bIsLeftAltDown, bIsRightAltDown );
}

void FWindowsApplication::SetCapture( const TSharedPtr< FGenericWindow >& InWindow )
{
	if ( InWindow.IsValid() )
	{
		::SetCapture( (HWND)InWindow->GetOSWindowHandle() );
	}
	else
	{
		::SetCapture( NULL );
	}
}

void* FWindowsApplication::GetCapture( void ) const
{
	return ::GetCapture();
}

void FWindowsApplication::SetHighPrecisionMouseMode( const bool Enable, const TSharedPtr< FGenericWindow >& InWindow )
{
	HWND hwnd = NULL;
	DWORD flags = RIDEV_REMOVE;
	bUsingHighPrecisionMouseInput = Enable;

	if ( Enable )
	{
		flags = 0;

		if ( InWindow.IsValid() )
		{
			hwnd = (HWND)InWindow->GetOSWindowHandle(); 
		}
	}

	// NOTE: Currently has to be created every time due to conflicts with Direct8 Input used by the wx unrealed
	RAWINPUTDEVICE RawInputDevice;

	//The HID standard for mouse
	const uint16 StandardMouse = 0x02;

	RawInputDevice.usUsagePage = 0x01; 
	RawInputDevice.usUsage = StandardMouse;
	RawInputDevice.dwFlags = flags;

	// Process input for just the window that requested it.  NOTE: If we pass NULL here events are routed to the window with keyboard focus
	// which is not always known at the HWND level with Slate
	RawInputDevice.hwndTarget = hwnd; 

	// Register the raw input device
	::RegisterRawInputDevices( &RawInputDevice, 1, sizeof( RAWINPUTDEVICE ) );
}

bool FWindowsApplication::TryCalculatePopupWindowPosition( const FPlatformRect& InAnchor, const FVector2D& InSize, const EPopUpOrientation::Type Orientation, /*OUT*/ FVector2D* const CalculatedPopUpPosition ) const
{
#if(_WIN32_WINNT >= 0x0601) 
	POINT AnchorPoint = { FMath::Trunc(InAnchor.Left), FMath::Trunc(InAnchor.Top) };
	SIZE WindowSize = { FMath::Trunc(InSize.X), FMath::Trunc(InSize.Y) };

	RECT WindowPos;
	::CalculatePopupWindowPosition( &AnchorPoint, &WindowSize, TPM_CENTERALIGN | TPM_VCENTERALIGN, NULL, &WindowPos );

	CalculatedPopUpPosition->Set( WindowPos.left, WindowPos.right );
	return true;
#else
	return false;
#endif
}

FPlatformRect FWindowsApplication::GetWorkArea( const FPlatformRect& CurrentWindow ) const
{
	RECT WindowsWindowDim;
	WindowsWindowDim.left = CurrentWindow.Left;
	WindowsWindowDim.top = CurrentWindow.Top;
	WindowsWindowDim.right = CurrentWindow.Right;
	WindowsWindowDim.bottom = CurrentWindow.Bottom;

	// ... figure out the best monitor for that window.
	HMONITOR hBestMonitor = MonitorFromRect( &WindowsWindowDim, MONITOR_DEFAULTTONEAREST );

	// Get information about that monitor...
	MONITORINFO MonitorInfo;
	MonitorInfo.cbSize = sizeof(MonitorInfo);
	GetMonitorInfo( hBestMonitor, &MonitorInfo);

	// ... so that we can figure out the work area (are not covered by taskbar)
	MonitorInfo.rcWork;

	FPlatformRect WorkArea;
	WorkArea.Left = MonitorInfo.rcWork.left;
	WorkArea.Top = MonitorInfo.rcWork.top;
	WorkArea.Right = MonitorInfo.rcWork.right;
	WorkArea.Bottom = MonitorInfo.rcWork.bottom;

	return WorkArea;
}

/**
 * Extracts EDID data from the given registry key and reads out native display with and height
 * @param hDevRegKey - Registry key where EDID is stored
 * @param OutWidth - Reference to output variable for monitor native width
 * @param OutHeight - Reference to output variable for monitor native height
 * @returns 'true' if data was extracted successfully, 'false' otherwise
 **/
static bool GetMonitorSizeFromEDID(const HKEY hDevRegKey, int32& OutWidth, int32& OutHeight)
{	
	static const uint32 NameSize = 512;
	static TCHAR ValueName[NameSize];

	DWORD Type;
	DWORD ActualValueNameLength = NameSize;

	BYTE EDIDData[1024];
	DWORD EDIDSize = sizeof(EDIDData);

	for (LONG i = 0, RetValue = ERROR_SUCCESS; RetValue != ERROR_NO_MORE_ITEMS; ++i)
	{
		RetValue = RegEnumValue ( hDevRegKey, 
			i, 
			&ValueName[0],
			&ActualValueNameLength, NULL, &Type,
			EDIDData,
			&EDIDSize);

		if (RetValue != ERROR_SUCCESS || (FCString::Strcmp(ValueName, TEXT("EDID")) != 0))
		{
			continue;
		}

		// EDID data format documented here:
		// http://en.wikipedia.org/wiki/EDID

		int DetailTimingDescriptorStartIndex = 54;
		OutWidth = ((EDIDData[DetailTimingDescriptorStartIndex+4] >> 4) << 8) | EDIDData[DetailTimingDescriptorStartIndex+2];
		OutHeight = ((EDIDData[DetailTimingDescriptorStartIndex+7] >> 4) << 8) | EDIDData[DetailTimingDescriptorStartIndex+5];

		return true; // valid EDID found
	}

	return false; // EDID not found
}

/**
 * Locate registry record for the given display device ID and extract native size information
 * @param TargetDevID - Name of taret device
 * @praam OutWidth - Reference to output variable for monitor native width
 * @praam OutHeight - Reference to output variable for monitor native height
 * @returns TRUE if data was extracted successfully, FALSE otherwise
 **/
inline bool GetSizeForDevID(const FString& TargetDevID, int32& Width, int32& Height)
{
	static const GUID ClassMonitorGuid = {0x4d36e96e, 0xe325, 0x11ce, {0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18}};

	HDEVINFO DevInfo = SetupDiGetClassDevsEx(
		&ClassMonitorGuid, //class GUID
		NULL,
		NULL,
		DIGCF_PRESENT,
		NULL,
		NULL,
		NULL);

	if (NULL == DevInfo)
	{
		return false;
	}

	bool bRes = false;

	for (ULONG MonitorIndex = 0; ERROR_NO_MORE_ITEMS != GetLastError(); ++MonitorIndex)
	{ 
		SP_DEVINFO_DATA DevInfoData;
		ZeroMemory(&DevInfoData, sizeof(DevInfoData));
		DevInfoData.cbSize = sizeof(DevInfoData);

		if (SetupDiEnumDeviceInfo(DevInfo, MonitorIndex, &DevInfoData) == TRUE)
		{
			HKEY hDevRegKey = SetupDiOpenDevRegKey(DevInfo, &DevInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);

			if(!hDevRegKey || (hDevRegKey == INVALID_HANDLE_VALUE))
			{
				continue;
			}

			bRes = GetMonitorSizeFromEDID(hDevRegKey, Width, Height);

			RegCloseKey(hDevRegKey);
		}
	}
	
	if (SetupDiDestroyDeviceInfoList(DevInfo) == FALSE)
	{
		bRes = false;
	}

	return bRes;
}

/**
 * Extract hardware information about connect monitors
 * @param OutMonitorInfo - Reference to an array for holding records about each detected monitor
 **/
void GetMonitorInfo(TArray<FMonitorInfo>& OutMonitorInfo)
{
	DISPLAY_DEVICE DisplayDevice;
	DisplayDevice.cb = sizeof(DisplayDevice);
	DWORD DeviceIndex = 0; // device index

	FMonitorInfo* PrimaryDevice = NULL;
	OutMonitorInfo.Empty(2); // Reserve two slots, as that will be the most common maximum

	FString DeviceID;
	while (EnumDisplayDevices(0, DeviceIndex, &DisplayDevice, 0))
	{
		if ((DisplayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) > 0)
		{
			DISPLAY_DEVICE Monitor;
			ZeroMemory(&Monitor, sizeof(Monitor));
			Monitor.cb = sizeof(Monitor);
			DWORD MonitorIndex = 0;

			while (EnumDisplayDevices(DisplayDevice.DeviceName, MonitorIndex, &Monitor, 0))
			{
				if (Monitor.StateFlags & DISPLAY_DEVICE_ACTIVE &&
					!(Monitor.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER))
				{
					FMonitorInfo Info;

					Info.ID = FString::Printf(TEXT("%s"), Monitor.DeviceID);
					Info.Name = Info.ID.Mid (8, Info.ID.Find (TEXT("\\"), ESearchCase::IgnoreCase, ESearchDir::FromStart, 9) - 8);

					if (GetSizeForDevID(Info.Name, Info.NativeWidth, Info.NativeHeight))
					{
						Info.ID = Monitor.DeviceID;
						Info.bIsPrimary = (DisplayDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) > 0;
						OutMonitorInfo.Add(Info);

						if (PrimaryDevice == NULL && Info.bIsPrimary)
						{
							PrimaryDevice = &OutMonitorInfo.Last();
						}
					}
				}
				MonitorIndex++;

				ZeroMemory(&Monitor, sizeof(Monitor));
				Monitor.cb = sizeof(Monitor);
			}
		}

		ZeroMemory(&DisplayDevice, sizeof(DisplayDevice));
		DisplayDevice.cb = sizeof(DisplayDevice);
		DeviceIndex++;
	}
}

void FWindowsApplication::GetDisplayMetrics( FDisplayMetrics& OutDisplayMetrics ) const
{
	// Total screen size of the primary monitor
	OutDisplayMetrics.PrimaryDisplayWidth = ::GetSystemMetrics( SM_CXSCREEN );
	OutDisplayMetrics.PrimaryDisplayHeight = ::GetSystemMetrics( SM_CYSCREEN );

	// Get the screen rect of the primary monitor, excluding taskbar etc.
	RECT WorkAreaRect;
	WorkAreaRect.top = WorkAreaRect.bottom = WorkAreaRect.left = WorkAreaRect.right = 0;
	SystemParametersInfo( SPI_GETWORKAREA, 0, &WorkAreaRect, 0 );
	OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Left = WorkAreaRect.left;
	OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Top = WorkAreaRect.top;
	OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Right = 1.0f + WorkAreaRect.right;
	OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Bottom = 1.0f + WorkAreaRect.bottom;

	// Virtual desktop area
	OutDisplayMetrics.VirtualDisplayRect.Left = ::GetSystemMetrics( SM_XVIRTUALSCREEN );
	OutDisplayMetrics.VirtualDisplayRect.Top = ::GetSystemMetrics( SM_YVIRTUALSCREEN );
	OutDisplayMetrics.VirtualDisplayRect.Right = 1.0f + ::GetSystemMetrics( SM_CXVIRTUALSCREEN );
	OutDisplayMetrics.VirtualDisplayRect.Bottom = 1.0f + ::GetSystemMetrics( SM_CYVIRTUALSCREEN );

	// Get connected monitor information
	GetMonitorInfo(OutDisplayMetrics.MonitorInfo);
}

void FWindowsApplication::GetInitialDisplayMetrics( FDisplayMetrics& OutDisplayMetrics ) const
{
	OutDisplayMetrics = InitialDisplayMetrics;
}

void FWindowsApplication::DestroyApplication()
{

}

static TSharedPtr< FWindowsWindow > FindWindowByHWND( const TArray< TSharedRef< FWindowsWindow > >& WindowsToSearch, HWND HandleToFind )
{
	for (int32 WindowIndex=0; WindowIndex < WindowsToSearch.Num(); ++WindowIndex)
	{
		TSharedRef< FWindowsWindow > Window = WindowsToSearch[ WindowIndex ];
		if ( Window->GetHWnd() == HandleToFind )
		{
			return Window;
		}
	}

	return TSharedPtr< FWindowsWindow >( NULL );
}


/** All WIN32 messages sent to our app go here; this method simply passes them on */
LRESULT CALLBACK FWindowsApplication::AppWndProc(HWND hwnd, uint32 msg, WPARAM wParam, LPARAM lParam)
{
	ensure( IsInGameThread() );

	return WindowApplication->ProcessMessage( hwnd, msg, wParam, lParam );
}

int32 FWindowsApplication::ProcessMessage( HWND hwnd, uint32 msg, WPARAM wParam, LPARAM lParam )
{
	TSharedPtr< FWindowsWindow > CurrentNativeEventWindowPtr = FindWindowByHWND( Windows, hwnd );

	if( Windows.Num() && CurrentNativeEventWindowPtr.IsValid() )
	{
		TSharedRef< FWindowsWindow > CurrentNativeEventWindow = CurrentNativeEventWindowPtr.ToSharedRef();

		switch(msg)
		{
			// Character
		case WM_CHAR:
			DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam );
			return 0;
		case WM_SYSCHAR:
			{
				if( ( HIWORD(lParam) & 0x2000 ) != 0 && wParam == VK_SPACE )
				{
					// Do not handle Alt+Space so that it passes through and opens the window system menu
					break;
				}
				else
				{
					return 0;
				}
			}
			
			break;

		case WM_SYSKEYDOWN:
			{
				// Alt-F4 or Alt+Space was pressed. 
				// Allow alt+f4 to close the window and alt+space to open the window menu
				if( wParam != VK_F4 && wParam != VK_SPACE)
				{
					DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam );
				}
			}
			break;

		case WM_KEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYUP:
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_XBUTTONDBLCLK:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
		case WM_NCMOUSEMOVE:
		case WM_MOUSEMOVE:
		case WM_MOUSEWHEEL:
		case WM_SETCURSOR:
			{
				DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam );
				// Handled
				return 0;
			}
			break;

		// Mouse Movement
		case WM_INPUT:
			{
				uint32 Size = 0;
				TArray<uint8> RawData;

				::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &Size, sizeof(RAWINPUTHEADER));
				RawData.AddZeroed( Size );

				RAWINPUT* Raw = NULL;
				if (::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, RawData.GetData(), &Size, sizeof(RAWINPUTHEADER)) == Size )
				{
					Raw = (RAWINPUT*)RawData.GetData();
				}

				if (Raw->header.dwType == RIM_TYPEMOUSE) 
				{
					const bool IsAbsoluteInput = (Raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == MOUSE_MOVE_ABSOLUTE;
					if( IsAbsoluteInput )
					{
						// Since the raw input is coming in as absolute it is likely the user is using a tablet
						// or perhaps is interacting through a virtual desktop
						DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam, 0, 0, MOUSE_MOVE_ABSOLUTE );
						return 1;
					}

					// Since raw input is coming in as relative it is likely a traditional mouse device
					const int xPosRelative = Raw->data.mouse.lLastX;
					const int yPosRelative = Raw->data.mouse.lLastY;

					DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam, xPosRelative, yPosRelative, MOUSE_MOVE_RELATIVE );
					return 1;
				} 
				
			}
			break;


		case WM_NCCALCSIZE:
			{
				// Let windows absorb this message if using the standard border
				if ( wParam && !CurrentNativeEventWindow->GetDefinition().HasOSWindowBorder )
				{
					return 0;
				}
			}
			break;

		case WM_SIZE:
			{
				DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam );

				return 0;
			}
			break;

		case WM_SIZING:
			{
				DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam, 0, 0 );
			}
			break;
		case WM_ENTERSIZEMOVE:
			{
				bInModalSizeLoop = true;
				DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam, 0, 0 );
			}
			break;
		case WM_EXITSIZEMOVE:
			{
				bInModalSizeLoop = false;
				DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam, 0, 0 );
			}
			break;


		case WM_MOVE:
			{
				// client area position
				const int32 NewX = (int)(short)(LOWORD(lParam));
				const int32 NewY = (int)(short)(HIWORD(lParam));
				FIntPoint NewPosition(NewX,NewY);

				// Only cache the screen position if its not minimized
				if ( FWindowsApplication::MinimizedWindowPosition != NewPosition )
				{
					MessageHandler->OnMovedWindow( CurrentNativeEventWindow, NewX, NewY );

					return 0;
				}
			}
			break;

		case WM_NCHITTEST:
			{
				// Only needed if not using the os window border as this is determined automatically
				if( !CurrentNativeEventWindow->GetDefinition().HasOSWindowBorder )
				{
					RECT rcWindow;
					GetWindowRect(hwnd, &rcWindow);

					const int32 LocalMouseX = (int)(short)(LOWORD(lParam)) - rcWindow.left;
					const int32 LocalMouseY = (int)(short)(HIWORD(lParam)) - rcWindow.top;
					if ( CurrentNativeEventWindow->IsRegularWindow() )
					{
						EWindowZone::Type Zone;
						// Assumes this is not allowed to leave Slate or touch rendering
						{
							Zone = MessageHandler->GetWindowZoneForPoint( CurrentNativeEventWindow, LocalMouseX, LocalMouseY );
						}

						static const LRESULT Results[] = {HTNOWHERE, HTTOPLEFT, HTTOP, HTTOPRIGHT, HTLEFT, HTCLIENT,
							HTRIGHT, HTBOTTOMLEFT, HTBOTTOM, HTBOTTOMRIGHT,
							HTCAPTION, HTMINBUTTON, HTMAXBUTTON, HTCLOSE, HTSYSMENU};

						return Results[Zone];
					}
				}
			}
			break;

			// Window focus and activation
		case WM_ACTIVATE:
			{
				DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam );
			}
			break;

		case WM_ACTIVATEAPP:
			{
				// When window activation changes we are not in a modal size loop
				bInModalSizeLoop = false;
				DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam );
			}
			break;

		case WM_PAINT:
			{
				if( bInModalSizeLoop && IsInGameThread() )
				{
					MessageHandler->OnOSPaint(CurrentNativeEventWindowPtr.ToSharedRef() );
				}
			}
			break;

		case WM_ERASEBKGND:
			{
				// Intercept background erasing to eliminate flicker.
				// Return non-zero to indicate that we'll handle the erasing ourselves
				return 1;
			}
			break;

		case WM_NCACTIVATE:
			{
				if( !CurrentNativeEventWindow->GetDefinition().HasOSWindowBorder )
				{
					// Unless using the OS window border, intercept calls to prevent non-client area drawing a border upon activation or deactivation
					// Return true to ensure standard activation happens
					return true;
				}
			}
			break;

		case WM_NCPAINT:
			{
				if( !CurrentNativeEventWindow->GetDefinition().HasOSWindowBorder )
				{
					// Unless using the OS window border, intercept calls to draw the non-client area - we do this ourselves
					return 0;
				}
			}
			break;

		case WM_DESTROY:
			{
				Windows.Remove( CurrentNativeEventWindow );
				return 0;
			}
			break;

		case WM_CLOSE:
			{
				DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam );
				return 0;
			}
			break;

		case WM_SYSCOMMAND:
			{
				switch( wParam & 0xfff0 )
				{
				case SC_RESTORE:
					// Checks to see if the window is minimized.
					if( IsIconic(hwnd) )
					{
						// This is required for restoring a minimized fullscreen window
						::ShowWindow(hwnd,SW_RESTORE);
						return 0;
					}
					else
					{
						if(!MessageHandler->OnWindowAction( CurrentNativeEventWindow, EWindowAction::Restore))
						{
							return 1;
						}
					}
					break;
				case SC_MAXIMIZE:
					{
						if(!MessageHandler->OnWindowAction( CurrentNativeEventWindow, EWindowAction::Maximize))
						{
							return 1;
						}
					}
					break;
				default:
					if( !( MessageHandler->ShouldProcessUserInputMessages( CurrentNativeEventWindow ) && IsInputMessage( msg ) ) )
					{
						return 0;
					}
					break;
				}
			}
			break;
			
		case WM_NCLBUTTONDOWN:
		case WM_NCRBUTTONDOWN:
		case WM_NCMBUTTONDOWN:
			{
				switch( wParam )
				{
				case HTMINBUTTON:
					{
						if(!MessageHandler->OnWindowAction( CurrentNativeEventWindow, EWindowAction::ClickedNonClientArea))
						{
							return 1;
						}
					}
					break;
				case HTMAXBUTTON:
					{
						if(!MessageHandler->OnWindowAction( CurrentNativeEventWindow, EWindowAction::ClickedNonClientArea))
						{
							return 1;
						}
					}
					break;
				case HTCLOSE:
					{
						if(!MessageHandler->OnWindowAction( CurrentNativeEventWindow, EWindowAction::ClickedNonClientArea))
						{
							return 1;
						}
					}
					break;
				case HTCAPTION:
					{
						if(!MessageHandler->OnWindowAction( CurrentNativeEventWindow, EWindowAction::ClickedNonClientArea))
						{
							return 1;
						}
					}
					break;
				}
			}
			break;

		case WM_GETDLGCODE:
			{
				// Slate wants all keys and messages.
				return DLGC_WANTALLKEYS;
			}
			break;
		
		case WM_CREATE:
			{
				return 0;
			}
			break;

		case WM_DEVICECHANGE:
			{
				XInput->SetNeedsControllerStateUpdate(); 
			}
		}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void FWindowsApplication::CheckForShiftUpEvents(const int32 KeyCode)
{
	// Since VK_SHIFT doesn't get an up message if the other shift key is held we need to poll for it
	if (PressedModifierKeys.Contains(KeyCode) && ((::GetKeyState(KeyCode) & 0x8000) == 0) )
	{
		PressedModifierKeys.Remove(KeyCode);
		MessageHandler->OnKeyUp( KeyCode, 0, false );
	}
}

int32 FWindowsApplication::ProcessDeferredMessage( const FDeferredWindowsMessage& DeferredMessage )
{
	if ( Windows.Num() && DeferredMessage.NativeWindow.IsValid() )
	{
		HWND hwnd = DeferredMessage.hWND;
		uint32 msg = DeferredMessage.Message;
		WPARAM wParam = DeferredMessage.wParam;
		LPARAM lParam = DeferredMessage.lParam;

		TSharedPtr< FWindowsWindow > CurrentNativeEventWindowPtr = DeferredMessage.NativeWindow.Pin();

		// This effectively disables a window without actually disabling it natively with the OS.
		// This allows us to continue receiving messages for it.
		if ( !MessageHandler->ShouldProcessUserInputMessages( CurrentNativeEventWindowPtr ) && IsInputMessage( msg ) )
		{
			return 0;	// consume input messages
		}

		switch(msg)
		{
			// Character
		case WM_CHAR:
			{
				// Character code is stored in WPARAM
				const TCHAR Character = wParam;

				// LPARAM bit 30 will be ZERO for new presses, or ONE if this is a repeat
				const bool bIsRepeat = ( lParam & 0x40000000 ) != 0;

				MessageHandler->OnKeyChar( Character, bIsRepeat );

				// Note: always return 0 to handle the message.  Win32 beeps if WM_CHAR is not handled...
				return 0;
			}
			break;


			// Key down
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
			{
				// Character code is stored in WPARAM
				const int32 Win32Key = wParam;

				// The actual key to use.  Some keys will be translated into other keys. 
				// I.E VK_CONTROL will be translated to either VK_LCONTROL or VK_RCONTROL as these
				// keys are never sent on their own
				int32 ActualKey = Win32Key;

				// LPARAM bit 30 will be ZERO for new presses, or ONE if this is a repeat
				bool bIsRepeat = ( lParam & 0x40000000 ) != 0;

				switch( Win32Key )
				{
				case VK_MENU:
					// Differentiate between left and right alt
					if( (lParam & 0x1000000) == 0 )
					{
						ActualKey = VK_LMENU;
						if ( (bIsRepeat = PressedModifierKeys.Contains( VK_LMENU )) == false)
						{
							PressedModifierKeys.Add( VK_LMENU );
						}
					}
					else
					{
						ActualKey = VK_RMENU;
						if ( (bIsRepeat = PressedModifierKeys.Contains( VK_RMENU )) == false)
						{
							PressedModifierKeys.Add( VK_RMENU );
						}
					}
					break;
				case VK_CONTROL:
					// Differentiate between left and right control
					if( (lParam & 0x1000000) == 0 )
					{
						ActualKey = VK_LCONTROL;
						if ( (bIsRepeat = PressedModifierKeys.Contains( VK_LCONTROL )) == false)
						{
							PressedModifierKeys.Add( VK_LCONTROL );
						}
					}
					else
					{
						ActualKey = VK_RCONTROL;
						if ( (bIsRepeat = PressedModifierKeys.Contains( VK_RCONTROL )) == false)
						{
							PressedModifierKeys.Add( VK_RCONTROL );
						}
					}

					break;
				case VK_SHIFT:
					// Differentiate between left and right shift
					ActualKey = MapVirtualKey( (lParam & 0x00ff0000) >> 16, MAPVK_VSC_TO_VK_EX);
					if ( (bIsRepeat = PressedModifierKeys.Contains( ActualKey )) == false)
					{
						PressedModifierKeys.Add( ActualKey );
					}
					break;
				default:
					// No translation needed
					break;
				}

				// Get the character code from the virtual key pressed.  If 0, no translation from virtual key to character exists
				uint32 CharCode = ::MapVirtualKey( Win32Key, MAPVK_VK_TO_CHAR );

				const bool Result = MessageHandler->OnKeyDown( ActualKey, CharCode, bIsRepeat );

				// Always return 0 to handle the message or else windows will beep
				if( Result || msg != WM_SYSKEYDOWN )
				{
					// Handled
					return 0;
				}
			}
			break;


			// Key up
		case WM_SYSKEYUP:
		case WM_KEYUP:
			{
				// Character code is stored in WPARAM
				int32 Win32Key = wParam;

				// The actual key to use.  Some keys will be translated into other keys. 
				// I.E VK_CONTROL will be translated to either VK_LCONTROL or VK_RCONTROL as these
				// keys are never sent on their own
				int32 ActualKey = Win32Key;

				bool bModifierKeyReleased = false;
				switch( Win32Key )
				{
				case VK_MENU:
					// Differentiate between left and right alt
					if( (lParam & 0x1000000) == 0 )
					{
						ActualKey = VK_LMENU;
					}
					else
					{
						ActualKey = VK_RMENU;
					}
					PressedModifierKeys.Remove( ActualKey );
					break;
				case VK_CONTROL:
					// Differentiate between left and right control
					if( (lParam & 0x1000000) == 0 )
					{
						ActualKey = VK_LCONTROL;
					}
					else
					{
						ActualKey = VK_RCONTROL;
					}
					PressedModifierKeys.Remove( ActualKey );
					break;
				case VK_SHIFT:
					// Differentiate between left and right shift
					ActualKey = MapVirtualKey( (lParam & 0x00ff0000) >> 16, MAPVK_VSC_TO_VK_EX);
					PressedModifierKeys.Remove( ActualKey );
					break;
				default:
					// No translation needed
					break;
				}

				// Get the character code from the virtual key pressed.  If 0, no translation from virtual key to character exists
				uint32 CharCode = ::MapVirtualKey( Win32Key, MAPVK_VK_TO_CHAR );

				// Key up events are never repeats
				const bool bIsRepeat = false;

				const bool Result = MessageHandler->OnKeyUp( ActualKey, CharCode, bIsRepeat );

				// Note that we allow system keys to pass through to DefWndProc here, so that core features
				// like Alt+F4 to close a window work.
				if( Result || msg != WM_SYSKEYUP )
				{
					// Handled
					return 0;
				}
			}
			break;

			// Mouse Button Down
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
			{
				if( msg == WM_LBUTTONDOWN )
				{
					MessageHandler->OnMouseDown( CurrentNativeEventWindowPtr, EMouseButtons::Left );
				}
				else
				{
					MessageHandler->OnMouseDoubleClick( CurrentNativeEventWindowPtr, EMouseButtons::Left );
				}
				return 0;
			}
			break;

		case WM_MBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
			{
				if( msg == WM_MBUTTONDOWN )
				{
					MessageHandler->OnMouseDown( CurrentNativeEventWindowPtr, EMouseButtons::Middle );
				}
				else
				{
					MessageHandler->OnMouseDoubleClick( CurrentNativeEventWindowPtr, EMouseButtons::Middle );
				}
				return 0;
			}
			break;

		case WM_RBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
			{
				if( msg == WM_RBUTTONDOWN )
				{
					MessageHandler->OnMouseDown( CurrentNativeEventWindowPtr, EMouseButtons::Right );
				}
				else
				{
					MessageHandler->OnMouseDoubleClick( CurrentNativeEventWindowPtr, EMouseButtons::Right );
				}
				return 0;
			}
			break;

		case WM_XBUTTONDBLCLK:
		case WM_XBUTTONDOWN:
			{
				EMouseButtons::Type MouseButton = ( HIWORD(wParam) & XBUTTON1 ) ? EMouseButtons::Thumb01  : EMouseButtons::Thumb02;

				BOOL Result = false;
				if( msg == WM_XBUTTONDOWN )
				{
					Result = MessageHandler->OnMouseDown( CurrentNativeEventWindowPtr, MouseButton );
				}
				else
				{
					Result = MessageHandler->OnMouseDoubleClick( CurrentNativeEventWindowPtr, MouseButton );
				}

				return Result ? 0 : 1;
			}
			break;

			// Mouse Button Up
		case WM_LBUTTONUP:
			{
				return MessageHandler->OnMouseUp( EMouseButtons::Left ) ? 0 : 1;
			}
			break;

		case WM_MBUTTONUP:
			{
				return MessageHandler->OnMouseUp( EMouseButtons::Middle ) ? 0 : 1;
			}
			break;

		case WM_RBUTTONUP:
			{
				return MessageHandler->OnMouseUp( EMouseButtons::Right ) ? 0 : 1;
			}
			break;

		case WM_XBUTTONUP:
			{
				EMouseButtons::Type MouseButton = ( HIWORD(wParam) & XBUTTON1 ) ? EMouseButtons::Thumb01  : EMouseButtons::Thumb02;
				return MessageHandler->OnMouseUp( MouseButton ) ? 0 : 1;
			}
			break;

		// Mouse Movement
		case WM_INPUT:
			{
				if( DeferredMessage.RawInputFlags == MOUSE_MOVE_RELATIVE )
				{
					MessageHandler->OnRawMouseMove( DeferredMessage.X, DeferredMessage.Y );
				}
				else
				{
					// Absolute coordinates given through raw input are simulated using MouseMove to get relative coordinates
					MessageHandler->OnMouseMove();
				}

				return 0;
			}
			break;

		// Mouse Movement
		case WM_NCMOUSEMOVE:
		case WM_MOUSEMOVE:
			{
				BOOL Result = false;
				if( !bUsingHighPrecisionMouseInput )
				{
					Result = MessageHandler->OnMouseMove();
				}

				return Result ? 0 : 1;
			}
			break;
			// Mouse Wheel
		case WM_MOUSEWHEEL:
			{
				const float SpinFactor = 1 / 120.0f;
				const SHORT WheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);

				const BOOL Result = MessageHandler->OnMouseWheel( static_cast<float>( WheelDelta ) * SpinFactor );
				return Result ? 0 : 1;
			}
			break;

			// Mouse Cursor
		case WM_SETCURSOR:
			{
				return MessageHandler->OnCursorSet() ? 1 : 0;
			}
			break;

			// Window focus and activation
		case WM_ACTIVATE:
			{
				EWindowActivation::Type ActivationType;

				if (LOWORD(wParam) & WA_ACTIVE)
				{
					ActivationType = EWindowActivation::Activate;
				}
				else if (LOWORD(wParam) & WA_CLICKACTIVE)
				{
					ActivationType = EWindowActivation::ActivateByMouse;
				}
				else
				{
					ActivationType = EWindowActivation::Deactivate;
				}

				if ( CurrentNativeEventWindowPtr.IsValid() )
				{
					BOOL Result = false;
					Result = MessageHandler->OnWindowActivationChanged( CurrentNativeEventWindowPtr.ToSharedRef(), ActivationType );
					return Result ? 0 : 1;
				}

				return 1;
			}
			break;

		case WM_ACTIVATEAPP:
			MessageHandler->OnApplicationActivationChanged( !!wParam );
			break;

	
		case WM_NCACTIVATE:
			{
				if( CurrentNativeEventWindowPtr.IsValid() && !CurrentNativeEventWindowPtr->GetDefinition().HasOSWindowBorder )
				{
					// Unless using the OS window border, intercept calls to prevent non-client area drawing a border upon activation or deactivation
					// Return true to ensure standard activation happens
					return true;
				}
			}
			break;

		case WM_NCPAINT:
			{
				if( CurrentNativeEventWindowPtr.IsValid() && !CurrentNativeEventWindowPtr->GetDefinition().HasOSWindowBorder )
				{
					// Unless using the OS window border, intercept calls to draw the non-client area - we do this ourselves
					return 0;
				}
			}
			break;

		case WM_CLOSE:
			{
				if ( CurrentNativeEventWindowPtr.IsValid() )
				{
					// Called when the OS close button is pressed
					MessageHandler->OnWindowClose( CurrentNativeEventWindowPtr.ToSharedRef() );
				}
				return 0;
			}
			break;
		case WM_SIZE:
			{
				if( CurrentNativeEventWindowPtr.IsValid() )
				{
					// @todo Fullscreen - Perform deferred resize
					// Note WM_SIZE provides the client dimension which is not equal to the window dimension if there is a windows border 
					const int32 NewWidth = (int)(short)(LOWORD(lParam));
					const int32 NewHeight = (int)(short)(HIWORD(lParam));

					const FGenericWindowDefinition& Definition = CurrentNativeEventWindowPtr->GetDefinition();
					if ( Definition.IsRegularWindow && !Definition.HasOSWindowBorder )
					{
						CurrentNativeEventWindowPtr->AdjustWindowRegion(NewWidth, NewHeight);
					}

					const bool bWasMinimized = (wParam == SIZE_MINIMIZED);

					const bool Result = MessageHandler->OnSizeChanged( CurrentNativeEventWindowPtr.ToSharedRef(), NewWidth, NewHeight, bWasMinimized );
				}
			}
			break;
		case WM_SIZING:
			{
				if( CurrentNativeEventWindowPtr.IsValid() )
				{
					MessageHandler->OnResizingWindow( CurrentNativeEventWindowPtr.ToSharedRef() );
				}
			}
			break;
		case WM_ENTERSIZEMOVE:
			{
				if( CurrentNativeEventWindowPtr.IsValid() )
				{
					MessageHandler->BeginReshapingWindow( CurrentNativeEventWindowPtr.ToSharedRef() );
				}
			}
			break;
		case WM_EXITSIZEMOVE:
			{
				if( CurrentNativeEventWindowPtr.IsValid() )
				{
					MessageHandler->FinishedReshapingWindow( CurrentNativeEventWindowPtr.ToSharedRef() );
				}
			}
			break;
		}
	}

	return 0;
}

bool FWindowsApplication::IsInputMessage( uint32 msg )
{
	switch(msg)
	{
	// Keyboard input notification messages...
	case WM_CHAR:
	case WM_SYSCHAR:
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYUP:
	case WM_SYSCOMMAND:
	// Mouse input notification messages...
	case WM_MOUSEHWHEEL:
	case WM_MOUSEWHEEL:
	case WM_MOUSEHOVER:
	case WM_MOUSELEAVE:
	case WM_MOUSEMOVE:
	case WM_NCMOUSEHOVER:
	case WM_NCMOUSELEAVE:
	case WM_NCMOUSEMOVE:
	case WM_NCMBUTTONDBLCLK:
	case WM_NCMBUTTONDOWN:
	case WM_NCMBUTTONUP:
	case WM_NCRBUTTONDBLCLK:
	case WM_NCRBUTTONDOWN:
	case WM_NCRBUTTONUP:
	case WM_NCXBUTTONDBLCLK:
	case WM_NCXBUTTONDOWN:
	case WM_NCXBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_XBUTTONDBLCLK:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	// Raw input notification messages...
	case WM_INPUT:
	case WM_INPUT_DEVICE_CHANGE:
		return true;
	}
	return false;
}

void FWindowsApplication::DeferMessage( TSharedPtr<FWindowsWindow>& NativeWindow, HWND InHWnd, uint32 InMessage, WPARAM InWParam, LPARAM InLParam, int32 MouseX, int32 MouseY, uint32 RawInputFlags )
{
	if( GPumpingMessagesOutsideOfMainLoop && bAllowedToDeferMessageProcessing )
	{
		DeferredMessages.Add( FDeferredWindowsMessage( NativeWindow, InHWnd, InMessage, InWParam, InLParam, MouseX, MouseY, RawInputFlags ) );
	}
	else
	{
		// When not deferring messages, process them immediately
		ProcessDeferredMessage( FDeferredWindowsMessage( NativeWindow, InHWnd, InMessage, InWParam, InLParam, MouseX, MouseY, RawInputFlags ) );
	}
}

void FWindowsApplication::PumpMessages( const float TimeDelta )
{
	MSG Message;

	// standard Windows message handling
	while(PeekMessage(&Message, NULL, 0, 0, PM_REMOVE))
	{ 
		TranslateMessage(&Message);
		DispatchMessage(&Message); 
	}
}

void FWindowsApplication::ProcessDeferredEvents( const float TimeDelta )
{
	// This function can be reentered when entering a modal tick loop.
	// We need to make a copy of the events that need to be processed or we may end up processing the same messages twice 
	TArray<FDeferredWindowsMessage> EventsToProcess( DeferredMessages );

	DeferredMessages.Empty();
	for( int32 MessageIndex = 0; MessageIndex < EventsToProcess.Num(); ++MessageIndex )
	{
		const FDeferredWindowsMessage& DeferredMessage = EventsToProcess[MessageIndex];
		ProcessDeferredMessage( DeferredMessage );
	}

	CheckForShiftUpEvents(VK_LSHIFT);
	CheckForShiftUpEvents(VK_RSHIFT);
}

void FWindowsApplication::PollGameDeviceState( const float TimeDelta )
{
	// initialize any externally-implemented input devices (we delay load initialize the array so any plugins have had time to load)
	if (!bHasLoadedInputPlugins)
	{
		TArray<IInputDeviceModule*> PluginImplementations = IModularFeatures::Get().GetModularFeatureImplementations<IInputDeviceModule>( IInputDeviceModule::GetModularFeatureName() );
		for( auto InputPluginIt = PluginImplementations.CreateIterator(); InputPluginIt; ++InputPluginIt )
		{
			TSharedPtr<IInputDevice> Device = (*InputPluginIt)->CreateInputDevice(MessageHandler);
			if ( Device.IsValid() )
			{
				ExternalInputDevices.Add(Device);
			}
		}

		bHasLoadedInputPlugins = true;
	}

	// Poll game device states and send new events
	XInput->SendControllerEvents();

	// Poll externally-implemented devices
	for( auto DeviceIt = ExternalInputDevices.CreateIterator(); DeviceIt; ++DeviceIt )
	{
		(*DeviceIt)->Tick( TimeDelta );
		(*DeviceIt)->SendControllerEvents();
	}
}

void FWindowsApplication::SetChannelValue (int32 ControllerId, FForceFeedbackChannelType ChannelType, float Value)
{
	// send vibration to externally-implemented devices
	for( auto DeviceIt = ExternalInputDevices.CreateIterator(); DeviceIt; ++DeviceIt )
	{
		(*DeviceIt)->SetChannelValue(ControllerId, ChannelType, Value);
	}
}

void FWindowsApplication::SetChannelValues (int32 ControllerId, const FForceFeedbackValues &Values)
{
	// send vibration to externally-implemented devices
	for( auto DeviceIt = ExternalInputDevices.CreateIterator(); DeviceIt; ++DeviceIt )
	{
		(*DeviceIt)->SetChannelValues(ControllerId, Values);
	}
}

HRESULT FWindowsApplication::OnOLEDragEnter( const HWND HWnd, IDataObject *DataObjectPointer, DWORD KeyState, POINTL CursorPosition, DWORD *CursorEffect)
{
	const TSharedPtr< FWindowsWindow > Window = FindWindowByHWND( Windows, HWnd );

	if ( !Window.IsValid() )
	{
		return 0;
	}

	// Decipher the OLE data

	// Utility to ensure resource release
	struct FOLEResourceGuard
	{
		STGMEDIUM& StorageMedium;
		LPVOID DataPointer;

		FOLEResourceGuard( STGMEDIUM& InStorage )
		: StorageMedium( InStorage )
		, DataPointer( GlobalLock(InStorage.hGlobal) ) 
		{
		}

		~FOLEResourceGuard()
		{
			GlobalUnlock( StorageMedium.hGlobal );
			ReleaseStgMedium( &StorageMedium );
		}
	};

	// Attempt to get plain text or unicode text from the data being dragged in

	FORMATETC FormatEtc_Ansii = { CF_TEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	const bool bHaveAnsiText = (DataObjectPointer->QueryGetData(&FormatEtc_Ansii) == S_OK)
		? true
		: false;

	FORMATETC FormatEtc_UNICODE = { CF_UNICODETEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	const bool bHaveUnicodeText = (DataObjectPointer->QueryGetData(&FormatEtc_UNICODE) == S_OK)
		? true
		: false;

	FORMATETC FormatEtc_File = { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	const bool bHaveFiles = (DataObjectPointer->QueryGetData(&FormatEtc_File) == S_OK)
		? true
		: false;

	STGMEDIUM StorageMedium;

	if ( bHaveUnicodeText && S_OK == DataObjectPointer->GetData( &FormatEtc_UNICODE, &StorageMedium ) )
	{
		FOLEResourceGuard ResourceGuard(StorageMedium);
		const TCHAR* TextData = static_cast<TCHAR*>( ResourceGuard.DataPointer );
		*CursorEffect = MessageHandler->OnDragEnterText( Window.ToSharedRef(), FString(TextData) );
	}
	else if ( bHaveAnsiText && S_OK == DataObjectPointer->GetData( &FormatEtc_Ansii, &StorageMedium ) )
	{
		FOLEResourceGuard ResourceGuard(StorageMedium);
		const ANSICHAR* TextData = static_cast<ANSICHAR*>( ResourceGuard.DataPointer );
		*CursorEffect = MessageHandler->OnDragEnterText( Window.ToSharedRef(), FString(TextData) );
	}
	else if ( bHaveFiles && S_OK == DataObjectPointer->GetData( &FormatEtc_File, &StorageMedium ) )
	{
		FOLEResourceGuard ResourceGuard(StorageMedium);
		const DROPFILES* DropFiles = static_cast<DROPFILES*>( ResourceGuard.DataPointer );
			
		// pFiles is the offset to the beginning of the file list, in bytes
		LPVOID FileListStart = (BYTE*)ResourceGuard.DataPointer + DropFiles->pFiles;

		TArray<FString> FileList;
		if ( DropFiles->fWide )
		{
			// Unicode filenames
			// The file list is NULL delimited with an extra NULL character at the end.
			TCHAR* Pos = static_cast<TCHAR*>( FileListStart );
			while (Pos[0] != 0)
			{
				const FString ListElement = FString(Pos);
				FileList.Add(ListElement);
				Pos += ListElement.Len() + 1;
			}
		}
		else
		{
			// Ansi filenames
			// The file list is NULL delimited with an extra NULL character at the end.
			ANSICHAR* Pos = static_cast<ANSICHAR*>( FileListStart );
			while (Pos[0] != 0)
			{
				const FString ListElement = FString(Pos);
				FileList.Add(ListElement);
				Pos += ListElement.Len() + 1;
			}
		}

		*CursorEffect = MessageHandler->OnDragEnterFiles( Window.ToSharedRef(), FileList );
	}

	return 0;
}

HRESULT FWindowsApplication::OnOLEDragOver( const HWND HWnd, DWORD KeyState, POINTL CursorPosition, DWORD *CursorEffect)
{
	const TSharedPtr< FWindowsWindow > Window = FindWindowByHWND( Windows, HWnd );

	if ( Window.IsValid() )
	{
		*CursorEffect = MessageHandler->OnDragOver( Window.ToSharedRef() );
	}

	return 0;
}

HRESULT FWindowsApplication::OnOLEDragOut( const HWND HWnd )
{
	const TSharedPtr< FWindowsWindow > Window = FindWindowByHWND( Windows, HWnd );

	if ( Window.IsValid() )
	{
		// User dragged out of a Slate window. We must tell Slate it is no longer in drag and drop mode.
		// Note that this also gets triggered when the user hits ESC to cancel a drag and drop.
		MessageHandler->OnDragLeave( Window.ToSharedRef() );
	}

	return 0;
}

HRESULT FWindowsApplication::OnOLEDrop( const HWND HWnd, IDataObject *DataObjectPointer, DWORD KeyState, POINTL CursorPosition, DWORD *CursorEffect)
{
	const TSharedPtr< FWindowsWindow > Window = FindWindowByHWND( Windows, HWnd );

	if ( Window.IsValid() )
	{
		*CursorEffect = MessageHandler->OnDragDrop( Window.ToSharedRef() );
	}

	return 0;
}

#if WITH_EDITOR
bool FWindowsApplication::SupportsSourceAccess() const 
{
	return true;
}

void FWindowsApplication::GotoLineInSource(const FString& FileAndLineNumber)
{
	FString FullPath, LineNumberWithColumnString;
	if (FileAndLineNumber.Split(TEXT("|"), &FullPath, &LineNumberWithColumnString))
	{
		FString LineNumberString;
		FString ColumnNumberString;
		if ( !LineNumberWithColumnString.Split(TEXT(":"), &LineNumberString, &ColumnNumberString, ESearchCase::CaseSensitive, ESearchDir::FromEnd) )
		{
			// The column was not in the string
			LineNumberString = LineNumberWithColumnString;
			ColumnNumberString = TEXT("");
		}
		
		int32 LineNumber = FCString::Strtoi(*LineNumberString, NULL, 10);
		int32 ColumnNumber = FCString::Strtoi(*ColumnNumberString, NULL, 10);

		FVSAccessorModule& VSAccessorModule = FModuleManager::LoadModuleChecked<FVSAccessorModule>(TEXT("VSAccessor"));
		VSAccessorModule.OpenVisualStudioFileAtLine(FullPath, LineNumber, ColumnNumber);
	}
}
#endif

#include "HideWindowsPlatformTypes.h"
