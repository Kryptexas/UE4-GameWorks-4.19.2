// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "WindowsPlatformApplicationMisc.h"
#include "WindowsApplication.h"
#include "Misc/App.h"
#include "Math/Color.h"
#include "WindowsHWrapper.h"

// Resource includes.
#include "Runtime/Launch/Resources/Windows/Resource.h"

typedef HRESULT(STDAPICALLTYPE *GetDpiForMonitorProc)(HMONITOR Monitor, int32 DPIType, uint32 *DPIX, uint32 *DPIY);
extern CORE_API GetDpiForMonitorProc GetDpiForMonitor;

GenericApplication* FWindowsPlatformApplicationMisc::CreateApplication()
{
	HICON AppIconHandle = LoadIcon( hInstance, MAKEINTRESOURCE( GetAppIcon() ) );
	if( AppIconHandle == NULL )
	{
		AppIconHandle = LoadIcon( (HINSTANCE)NULL, IDI_APPLICATION ); 
	}

	return FWindowsApplication::CreateWindowsApplication( hInstance, AppIconHandle );
}

int32 FWindowsPlatformApplicationMisc::GetAppIcon()
{
	return IDICON_UE4Game;
}

static void WinPumpMessages()
{
	{
		MSG Msg;
		while( PeekMessage(&Msg,NULL,0,0,PM_REMOVE) )
		{
			TranslateMessage( &Msg );
			DispatchMessage( &Msg );
		}
	}
}

static void WinPumpSentMessages()
{
	MSG Msg;
	PeekMessage(&Msg,NULL,0,0,PM_NOREMOVE | PM_QS_SENDMESSAGE);
}

void FWindowsPlatformApplicationMisc::PumpMessages(bool bFromMainLoop)
{
	if (!bFromMainLoop)
	{
		TGuardValue<bool> PumpMessageGuard( GPumpingMessagesOutsideOfMainLoop, true );
		// Process pending windows messages, which is necessary to the rendering thread in some rare cases where D3D
		// sends window messages (from IDXGISwapChain::Present) to the main thread owned viewport window.
		WinPumpSentMessages();
		return;
	}

	GPumpingMessagesOutsideOfMainLoop = false;
	WinPumpMessages();

	// Determine if application has focus
	bool HasFocus = FApp::UseVRFocus() ? FApp::HasVRFocus() : FWindowsPlatformProcess::IsThisApplicationForeground();

	// If editor thread doesn't have the focus, don't suck up too much CPU time.
	if( GIsEditor )
	{
		static bool HadFocus=1;
		if( HadFocus && !HasFocus )
		{
			// Drop our priority to speed up whatever is in the foreground.
			SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL );
		}
		else if( HasFocus && !HadFocus )
		{
			// Boost our priority back to normal.
			SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_NORMAL );
		}
		if( !HasFocus )
		{
			// Sleep for a bit to not eat up all CPU time.
			FPlatformProcess::Sleep(0.005f);
		}
		HadFocus = HasFocus;
	}

	// if its our window, allow sound, otherwise apply multiplier
	FApp::SetVolumeMultiplier( HasFocus ? 1.0f : FApp::GetUnfocusedVolumeMultiplier() );
}

void FWindowsPlatformApplicationMisc::PreventScreenSaver()
{
	INPUT Input = { 0 };
	Input.type = INPUT_MOUSE;
	Input.mi.dx = 0;
	Input.mi.dy = 0;	
	Input.mi.mouseData = 0;
	Input.mi.dwFlags = MOUSEEVENTF_MOVE;
	Input.mi.time = 0;
	Input.mi.dwExtraInfo = 0; 	
	SendInput(1,&Input,sizeof(INPUT));
}

uint32 FWindowsPlatformApplicationMisc::GetCharKeyMap(uint32* KeyCodes, FString* KeyNames, uint32 MaxMappings)
{
	return FGenericPlatformApplicationMisc::GetStandardPrintableKeyMap(KeyCodes, KeyNames, MaxMappings, true, false);
}

uint32 FWindowsPlatformApplicationMisc::GetKeyMap( uint32* KeyCodes, FString* KeyNames, uint32 MaxMappings )
{
#define ADDKEYMAP(KeyCode, KeyName)		if (NumMappings<MaxMappings) { KeyCodes[NumMappings]=KeyCode; KeyNames[NumMappings]=KeyName; ++NumMappings; };

	uint32 NumMappings = 0;

	if ( KeyCodes && KeyNames && (MaxMappings > 0) )
	{
		ADDKEYMAP( VK_LBUTTON, TEXT("LeftMouseButton") );
		ADDKEYMAP( VK_RBUTTON, TEXT("RightMouseButton") );
		ADDKEYMAP( VK_MBUTTON, TEXT("MiddleMouseButton") );

		ADDKEYMAP( VK_XBUTTON1, TEXT("ThumbMouseButton") );
		ADDKEYMAP( VK_XBUTTON2, TEXT("ThumbMouseButton2") );

		ADDKEYMAP( VK_BACK, TEXT("BackSpace") );
		ADDKEYMAP( VK_TAB, TEXT("Tab") );
		ADDKEYMAP( VK_RETURN, TEXT("Enter") );
		ADDKEYMAP( VK_PAUSE, TEXT("Pause") );

		ADDKEYMAP( VK_CAPITAL, TEXT("CapsLock") );
		ADDKEYMAP( VK_ESCAPE, TEXT("Escape") );
		ADDKEYMAP( VK_SPACE, TEXT("SpaceBar") );
		ADDKEYMAP( VK_PRIOR, TEXT("PageUp") );
		ADDKEYMAP( VK_NEXT, TEXT("PageDown") );
		ADDKEYMAP( VK_END, TEXT("End") );
		ADDKEYMAP( VK_HOME, TEXT("Home") );

		ADDKEYMAP( VK_LEFT, TEXT("Left") );
		ADDKEYMAP( VK_UP, TEXT("Up") );
		ADDKEYMAP( VK_RIGHT, TEXT("Right") );
		ADDKEYMAP( VK_DOWN, TEXT("Down") );

		ADDKEYMAP( VK_INSERT, TEXT("Insert") );
		ADDKEYMAP( VK_DELETE, TEXT("Delete") );

		ADDKEYMAP( VK_NUMPAD0, TEXT("NumPadZero") );
		ADDKEYMAP( VK_NUMPAD1, TEXT("NumPadOne") );
		ADDKEYMAP( VK_NUMPAD2, TEXT("NumPadTwo") );
		ADDKEYMAP( VK_NUMPAD3, TEXT("NumPadThree") );
		ADDKEYMAP( VK_NUMPAD4, TEXT("NumPadFour") );
		ADDKEYMAP( VK_NUMPAD5, TEXT("NumPadFive") );
		ADDKEYMAP( VK_NUMPAD6, TEXT("NumPadSix") );
		ADDKEYMAP( VK_NUMPAD7, TEXT("NumPadSeven") );
		ADDKEYMAP( VK_NUMPAD8, TEXT("NumPadEight") );
		ADDKEYMAP( VK_NUMPAD9, TEXT("NumPadNine") );

		ADDKEYMAP( VK_MULTIPLY, TEXT("Multiply") );
		ADDKEYMAP( VK_ADD, TEXT("Add") );
		ADDKEYMAP( VK_SUBTRACT, TEXT("Subtract") );
		ADDKEYMAP( VK_DECIMAL, TEXT("Decimal") );
		ADDKEYMAP( VK_DIVIDE, TEXT("Divide") );

		ADDKEYMAP( VK_F1, TEXT("F1") );
		ADDKEYMAP( VK_F2, TEXT("F2") );
		ADDKEYMAP( VK_F3, TEXT("F3") );
		ADDKEYMAP( VK_F4, TEXT("F4") );
		ADDKEYMAP( VK_F5, TEXT("F5") );
		ADDKEYMAP( VK_F6, TEXT("F6") );
		ADDKEYMAP( VK_F7, TEXT("F7") );
		ADDKEYMAP( VK_F8, TEXT("F8") );
		ADDKEYMAP( VK_F9, TEXT("F9") );
		ADDKEYMAP( VK_F10, TEXT("F10") );
		ADDKEYMAP( VK_F11, TEXT("F11") );
		ADDKEYMAP( VK_F12, TEXT("F12") );

		ADDKEYMAP( VK_NUMLOCK, TEXT("NumLock") );

		ADDKEYMAP( VK_SCROLL, TEXT("ScrollLock") );

		ADDKEYMAP( VK_LSHIFT, TEXT("LeftShift") );
		ADDKEYMAP( VK_RSHIFT, TEXT("RightShift") );
		ADDKEYMAP( VK_LCONTROL, TEXT("LeftControl") );
		ADDKEYMAP( VK_RCONTROL, TEXT("RightControl") );
		ADDKEYMAP( VK_LMENU, TEXT("LeftAlt") );
		ADDKEYMAP( VK_RMENU, TEXT("RightAlt") );
		ADDKEYMAP( VK_LWIN, TEXT("LeftCommand") );
		ADDKEYMAP( VK_RWIN, TEXT("RightCommand") );

		TMap<uint32, uint32> ScanToVKMap;
#define MAP_OEM_VK_TO_SCAN(KeyCode) { const uint32 CharCode = MapVirtualKey(KeyCode,2); if (CharCode != 0) { ScanToVKMap.Add(CharCode,KeyCode); } }
		MAP_OEM_VK_TO_SCAN(VK_OEM_1);
		MAP_OEM_VK_TO_SCAN(VK_OEM_2);
		MAP_OEM_VK_TO_SCAN(VK_OEM_3);
		MAP_OEM_VK_TO_SCAN(VK_OEM_4);
		MAP_OEM_VK_TO_SCAN(VK_OEM_5);
		MAP_OEM_VK_TO_SCAN(VK_OEM_6);
		MAP_OEM_VK_TO_SCAN(VK_OEM_7);
		MAP_OEM_VK_TO_SCAN(VK_OEM_8);
		MAP_OEM_VK_TO_SCAN(VK_OEM_PLUS);
		MAP_OEM_VK_TO_SCAN(VK_OEM_COMMA);
		MAP_OEM_VK_TO_SCAN(VK_OEM_MINUS);
		MAP_OEM_VK_TO_SCAN(VK_OEM_PERIOD);
		MAP_OEM_VK_TO_SCAN(VK_OEM_102);
#undef  MAP_OEM_VK_TO_SCAN

		static const uint32 MAX_KEY_MAPPINGS(256);
		uint32 CharCodes[MAX_KEY_MAPPINGS];
		FString CharKeyNames[MAX_KEY_MAPPINGS];
		const int32 CharMappings = GetCharKeyMap(CharCodes, CharKeyNames, MAX_KEY_MAPPINGS);

		for (int32 MappingIndex = 0; MappingIndex < CharMappings; ++MappingIndex)
		{
			ScanToVKMap.Remove(CharCodes[MappingIndex]);
		}

		for (auto It(ScanToVKMap.CreateConstIterator()); It; ++It)
		{
			ADDKEYMAP(It.Value(), FString::Chr(It.Key()));
		}
	}

	check(NumMappings < MaxMappings);
	return NumMappings;

#undef ADDKEYMAP
}

FLinearColor FWindowsPlatformApplicationMisc::GetScreenPixelColor(const FVector2D& InScreenPos, float /*InGamma*/)
{
	COLORREF PixelColorRef = GetPixel(GetDC(HWND_DESKTOP), InScreenPos.X, InScreenPos.Y);

	FColor sRGBScreenColor(
		(PixelColorRef & 0xFF),
		((PixelColorRef & 0xFF00) >> 8),
		((PixelColorRef & 0xFF0000) >> 16),
		255);

	// Assume the screen color is coming in as sRGB space
	return FLinearColor(sRGBScreenColor);
}

bool FWindowsPlatformApplicationMisc::GetWindowTitleMatchingText(const TCHAR* TitleStartsWith, FString& OutTitle)
{
	bool bWasFound = false;
	WCHAR Buffer[8192];
	// Get the first window so we can start walking the window chain
	HWND hWnd = FindWindowW(NULL,NULL);
	if (hWnd != NULL)
	{
		size_t TitleStartsWithLen = _tcslen(TitleStartsWith);
		do
		{
			GetWindowText(hWnd,Buffer,8192);
			// If this matches, then grab the full text
			if (_tcsnccmp(TitleStartsWith, Buffer, TitleStartsWithLen) == 0)
			{
				OutTitle = Buffer;
				hWnd = NULL;
				bWasFound = true;
			}
			else
			{
				// Get the next window to interrogate
				hWnd = GetWindow(hWnd, GW_HWNDNEXT);
			}
		}
		while (hWnd != NULL);
	}
	return bWasFound;
}

float FWindowsPlatformApplicationMisc::GetDPIScaleFactorAtPoint(float X, float Y)
{
	if (FParse::Param(FCommandLine::Get(), TEXT("enablehighdpi")))
	{
		if (GetDpiForMonitor)
		{
			POINT Position = { X, Y };
			HMONITOR Monitor = MonitorFromPoint(Position, MONITOR_DEFAULTTONEAREST);
			if (Monitor)
			{
				uint32 DPIX = 0;
				uint32 DPIY = 0;
				return SUCCEEDED(GetDpiForMonitor(Monitor, 0/*MDT_EFFECTIVE_DPI_VALUE*/, &DPIX, &DPIY)) ? DPIX / 96.0f : 1.0f;
			}
		}
		else
		{
			HDC Context = GetDC(nullptr);
			const float DPIScaleFactor = GetDeviceCaps(Context, LOGPIXELSX) / 96.0f;
			ReleaseDC(nullptr, Context);
			return DPIScaleFactor;
		}
	}
	return 1.0f;
}

// Disabling optimizations helps to reduce the frequency of OpenClipboard failing with error code 0. It still happens
// though only with really large text buffers and we worked around this by changing the editor to use an intermediate
// text buffer for internal operations.
PRAGMA_DISABLE_OPTIMIZATION 

void FWindowsPlatformApplicationMisc::ClipboardCopy(const TCHAR* Str)
{
	if( OpenClipboard(GetActiveWindow()) )
	{
		verify(EmptyClipboard());
		HGLOBAL GlobalMem;
		int32 StrLen = FCString::Strlen(Str);
		GlobalMem = GlobalAlloc( GMEM_MOVEABLE, sizeof(TCHAR)*(StrLen+1) );
		check(GlobalMem);
		TCHAR* Data = (TCHAR*) GlobalLock( GlobalMem );
		FCString::Strcpy( Data, (StrLen+1), Str );
		GlobalUnlock( GlobalMem );
		if( SetClipboardData( CF_UNICODETEXT, GlobalMem ) == NULL )
			UE_LOG(LogWindows, Fatal,TEXT("SetClipboardData failed with error code %i"), (uint32)GetLastError() );
		verify(CloseClipboard());
	}
}

void FWindowsPlatformApplicationMisc::ClipboardPaste(class FString& Result)
{
	if( OpenClipboard(GetActiveWindow()) )
	{
		HGLOBAL GlobalMem = NULL;
		bool Unicode = 0;
		GlobalMem = GetClipboardData( CF_UNICODETEXT );
		Unicode = 1;
		if( !GlobalMem )
		{
			GlobalMem = GetClipboardData( CF_TEXT );
			Unicode = 0;
		}
		if( !GlobalMem )
		{
			Result = TEXT("");
		}
		else
		{
			void* Data = GlobalLock( GlobalMem );
			check( Data );	
			if( Unicode )
				Result = (TCHAR*) Data;
			else
			{
				ANSICHAR* ACh = (ANSICHAR*) Data;
				int32 i;
				for( i=0; ACh[i]; i++ );
				TArray<TCHAR> Ch;
				Ch.AddUninitialized(i+1);
				for( i=0; i<Ch.Num(); i++ )
					Ch[i]=CharCast<TCHAR>(ACh[i]);
				Result.GetCharArray() = MoveTemp(Ch);
			}
			GlobalUnlock( GlobalMem );
		}
		verify(CloseClipboard());
	}
	else 
	{
		Result=TEXT("");
	}
}

PRAGMA_ENABLE_OPTIMIZATION 
