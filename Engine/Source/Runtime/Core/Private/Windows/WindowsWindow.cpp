// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "WindowsWindow.h"
#include "WindowsApplication.h"

#if WINVER > 0x502	// Windows Vista or better required for DWM
#include "AllowWindowsPlatformTypes.h"
#include "Dwmapi.h"
#include "HideWindowsPlatformTypes.h"
#endif

FWindowsWindow::~FWindowsWindow()
{
	// NOTE: The HWnd is invalid here!
	//       Doing stuff with HWnds will fail silently.
	//       Use Destroy() instead.
}

const TCHAR FWindowsWindow::AppWindowClass[] = TEXT("UnrealWindow");

// This is a hard-coded constant for Windows' WS_THICKFRAME
// There does not seem to be a way to alter this
static int32 WindowsAeroBorderSize = 8;
static int32 WindowsStandardBorderSize = 4;

TSharedRef< FWindowsWindow > FWindowsWindow::Make()
{
	// First, allocate the new native window object.  This doesn't actually create a native window or anything,
	// we're simply instantiating the object so that we can keep shared references to it.
	return MakeShareable( new FWindowsWindow() );
}

void FWindowsWindow::Initialize( FWindowsApplication* const Application, const TSharedRef< FGenericWindowDefinition >& InDefinition, HINSTANCE InHInstance, const TSharedPtr< FWindowsWindow >& InParent, const bool bShowImmediately )
{
	Definition = InDefinition;
	OwningApplication = Application;

	// Finally, let's initialize the new native window object.  Calling this function will often cause OS
	// window messages to be sent! (such as activation messages)
	uint32 WindowExStyle = 0;
	uint32 WindowStyle = 0;

	RegionWidth = RegionHeight = INDEX_NONE;

	const float XInitialRect = Definition->XDesiredPositionOnScreen;
	const float YInitialRect = Definition->YDesiredPositionOnScreen;

	const float WidthInitial = Definition->WidthDesiredOnScreen;
	const float HeightInitial = Definition->HeightDesiredOnScreen;

	int32 X = FMath::Trunc( XInitialRect );
	int32 Y = FMath::Trunc( YInitialRect );
	int32 ClientWidth = FMath::Trunc( WidthInitial );
	int32 ClientHeight = FMath::Trunc( HeightInitial );
	int32 WindowWidth = ClientWidth;
	int32 WindowHeight = ClientHeight;

	if( !Definition->HasOSWindowBorder )
	{
		WindowExStyle = WS_EX_WINDOWEDGE;
		if (Definition->SupportsTransparency)
		{
			WindowExStyle |= WS_EX_LAYERED;
		}

		WindowStyle = WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
		if ( Definition->AppearsInTaskbar )
		{
			WindowExStyle |= WS_EX_APPWINDOW;
		}
		else
		{
			WindowExStyle |= WS_EX_TOOLWINDOW;
		}

		if( Definition->IsTopmostWindow )
		{
			// Tool tips are always top most windows
			WindowExStyle |= WS_EX_TOPMOST;
		}

		if ( !Definition->AcceptsInput )
		{
			// Window should never get input
			WindowExStyle |= WS_EX_TRANSPARENT;
		}
	}
	else
	{
		// OS Window border setup
		WindowExStyle = WS_EX_APPWINDOW;
		WindowStyle = WS_POPUP | WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_BORDER | WS_CAPTION;

		// Note SizeX and SizeY should be the size of the client area.  We need to get the actual window size by adjusting the client size to account for standard windows border around the window
		RECT WindowRect = { 0, 0, ClientWidth, ClientWidth };
		::AdjustWindowRectEx(&WindowRect,WindowStyle,0,WindowExStyle);

		X += WindowRect.left;
		Y += WindowRect.top;
		WindowWidth = WindowRect.right - WindowRect.left;
		WindowHeight = WindowRect.bottom - WindowRect.top;
	}


	// Creating the Window
	HWnd = CreateWindowEx(
		WindowExStyle,
		AppWindowClass,
		*Definition->Title,
		WindowStyle,
		X, Y, WindowWidth, WindowHeight,
		( InParent.IsValid() ) ? static_cast<HWND>( InParent->HWnd ) : NULL,
		NULL, InHInstance, NULL);

	VirtualWidth = ClientWidth;
	VirtualHeight = ClientHeight;

	// We call reshape window here because we didn't take into account the non-client area
	// in the initial creation of the window. Slate should only pass client area dimensions.
	// Reshape window may resize the window if the non-client area is encroaching on our
	// desired client area space.
	ReshapeWindow( X, Y, ClientWidth, ClientHeight );

	if( HWnd == NULL )
	{
		// @todo Error message should be localized!
		MessageBox(NULL, TEXT("Window Creation Failed!"), TEXT("Error!"), MB_ICONEXCLAMATION | MB_OK);
		checkf(0, TEXT("Window Creation Failed (%d)"), ::GetLastError() );
		return;
	}

	if ( Definition->SupportsTransparency )
	{
		SetOpacity( Definition->Opacity );
	}

#if WINVER > 0x502	// Windows Vista or better required for DWM
	// Disable DWM Rendering and Nonclient Area painting if not showing the os window border
	// This prevents the standard windows frame from ever being drawn
	if( !Definition->HasOSWindowBorder )
	{
		const DWMNCRENDERINGPOLICY RenderingPolicy = DWMNCRP_DISABLED;
		check(SUCCEEDED(DwmSetWindowAttribute(HWnd, DWMWA_NCRENDERING_POLICY, &RenderingPolicy, sizeof(RenderingPolicy))));

		const BOOL bEnableNCPaint = false;
		check(SUCCEEDED(DwmSetWindowAttribute(HWnd, DWMWA_ALLOW_NCPAINT, &bEnableNCPaint, sizeof(bEnableNCPaint))));
	}
#endif	// WINVER

	// No region for non regular windows or windows displaying the os window border
	if ( IsRegularWindow() && !Definition->HasOSWindowBorder )
	{
		WindowStyle |= WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
		if ( Definition->SupportsMaximize )
		{
			WindowStyle |= WS_MAXIMIZEBOX;
		}
		if ( Definition->SupportsMinimize )
		{
			WindowStyle |= WS_MINIMIZEBOX;
		}
		if ( Definition->HasSizingFrame )
		{
			WindowStyle |= WS_THICKFRAME;
		}

		check(SetWindowLong(HWnd, GWL_STYLE, WindowStyle));

		AdjustWindowRegion( ClientWidth, ClientHeight );
	}

	if ( IsRegularWindow() )
	{
		// Tell OLE that we are opting into drag and drop.
		// Only makes sense for regular windows (windows that last a while.)
		RegisterDragDrop( HWnd, this );
	}
}

FWindowsWindow::FWindowsWindow()
	: HWnd(NULL)
	, WindowMode( EWindowMode::Windowed )
	, OLEReferenceCount(0)
	, bIsVisible( false )
{
	FMemory::MemZero(PreFullscreenWindowPlacement);
	PreFullscreenWindowPlacement.length = sizeof(WINDOWPLACEMENT);
}

HWND FWindowsWindow::GetHWnd() const
{
	return HWnd;
}

HRGN FWindowsWindow::MakeWindowRegionObject() const
{
	HRGN Region;
	if ( RegionWidth != INDEX_NONE && RegionHeight != INDEX_NONE )
	{
		if (IsMaximized())
		{
			int32 WindowBorderSize = GetWindowBorderSize();
			Region = CreateRectRgn( WindowBorderSize, WindowBorderSize, RegionWidth - WindowBorderSize, RegionHeight - WindowBorderSize );
		}
		else
		{
			if( Definition->CornerRadius > 0 )
			{
				// @todo mac: Corner radius not applied on Mac platform yet
				Region = CreateRoundRectRgn( 0, 0, RegionWidth, RegionHeight, Definition->CornerRadius, Definition->CornerRadius );
			}
			else
			{
				Region = CreateRectRgn( 0, 0, RegionWidth, RegionHeight );
			}
		}
	}
	else
	{
		RECT rcWnd;
		GetWindowRect( HWnd, &rcWnd );
		Region = CreateRectRgn( 0, 0, rcWnd.right - rcWnd.left, rcWnd.bottom - rcWnd.top );
	}

	return Region;
}

void FWindowsWindow::AdjustWindowRegion( int32 Width, int32 Height )
{
	RegionWidth = Width;
	RegionHeight = Height;

	HRGN Region = MakeWindowRegionObject();

	// NOTE: We explicitly don't delete the Region object, because the OS deletes the handle when it no longer needed after
	// a call to SetWindowRgn.
	check( SetWindowRgn( HWnd, Region, false ) );
}

void FWindowsWindow::ReshapeWindow( int32 NewX, int32 NewY, int32 NewWidth, int32 NewHeight )
{
	WINDOWINFO WindowInfo;
	FMemory::MemZero( WindowInfo );
	WindowInfo.cbSize = sizeof( WindowInfo );
	::GetWindowInfo( HWnd, &WindowInfo );

	// X,Y, Width, Height defines the pixel left of the client area on the screen
	RECT ClientRect = { 0, 0, NewWidth, NewHeight };
	RECT WindowRect = { 0, 0, NewWidth, NewHeight };
	if( Definition->HasOSWindowBorder )
	{
		// Note SizeX and SizeY should be the size of the client area.  We need to get the actual window size by adjusting the client size to account for standard windows border around the window
		::AdjustWindowRectEx(&WindowRect,WindowInfo.dwStyle,0,WindowInfo.dwExStyle);

		NewWidth = WindowRect.right - WindowRect.left;
		NewHeight = WindowRect.bottom - WindowRect.top;
	}

	// the window position is the requested position
	int32 WindowX = NewX;
	int32 WindowY = NewY;
	
	// If the window size changes often, only grow the window, never shrink it
	const bool bVirtualSizeChanged = NewWidth != VirtualWidth || NewHeight != VirtualHeight;
	VirtualWidth = NewWidth;
	VirtualHeight = NewHeight;

	if( Definition->SizeWillChangeOften )
	{
		// When SizeWillChangeOften is set, we set a minimum width and height window size that we'll keep allocated
		// even when the requested actual window size is smaller.  This just avoids constantly resizing the window
		// and associated GPU buffers, which can be very slow on some platforms.

		const RECT OldWindowRect = WindowInfo.rcWindow;
		const int32 OldWidth = OldWindowRect.right - OldWindowRect.left;
		const int32 OldHeight = OldWindowRect.bottom - OldWindowRect.top;

		const int32 MinRetainedWidth = Definition->ExpectedMaxWidth != INDEX_NONE ? Definition->ExpectedMaxWidth : OldWidth;
		const int32 MinRetainedHeight = Definition->ExpectedMaxHeight != INDEX_NONE ? Definition->ExpectedMaxHeight : OldHeight;

		NewWidth = FMath::Max( NewWidth, FMath::Min( OldWidth, MinRetainedWidth ) );
		NewHeight = FMath::Max( NewHeight, FMath::Min( OldHeight, MinRetainedHeight ) );
	}


	// NOTE: MoveWindow will trigger a WM_SIZE and our SWindow's cached size will be updated
	const bool bRepaint = true;
	::MoveWindow( HWnd, WindowX, WindowY, NewWidth, NewHeight, bRepaint );

	if( Definition->SizeWillChangeOften && bVirtualSizeChanged )
	{
		AdjustWindowRegion( VirtualWidth, VirtualHeight );
	}
}

bool FWindowsWindow::GetFullScreenInfo( int32& X, int32& Y, int32& Width, int32& Height ) const
{
	bool bTrueFullscreen = ( WindowMode == EWindowMode::Fullscreen );

	// Grab current monitor data for sizing
	HMONITOR Monitor = MonitorFromWindow( HWnd, bTrueFullscreen ? MONITOR_DEFAULTTOPRIMARY : MONITOR_DEFAULTTONEAREST );
	MONITORINFO MonitorInfo;
	MonitorInfo.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo( Monitor, &MonitorInfo );

	X = MonitorInfo.rcMonitor.left;
	Y = MonitorInfo.rcMonitor.top;
	Width = MonitorInfo.rcMonitor.right - X;
	Height = MonitorInfo.rcMonitor.bottom - Y;

	return true;
}

/** Native windows should implement MoveWindowTo by relocating the platform-specific window to (X,Y). */
void FWindowsWindow::MoveWindowTo( int32 X, int32 Y )
{
	RECT WindowRect;
	::GetWindowRect(HWnd, &WindowRect);

	POINT ClientPoint;
	ClientPoint.x = 0;
	ClientPoint.y = 0;
	::ClientToScreen( HWnd, &ClientPoint );

	const int32 XMoveDistance = X - ClientPoint.x;
	const int32 YMoveDistance = Y - ClientPoint.y;

	X = WindowRect.left + XMoveDistance;
	Y = WindowRect.top + YMoveDistance;

	::MoveWindow( HWnd, X, Y, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top, true );
}

/** Native windows should implement BringToFront by making this window the top-most window (i.e. focused).
 *
 * @param bForce	Forces the window to the top of the Z order, even if that means stealing focus from other windows
 *					In general do not pass true for this.  It is really only useful for some windows, like game windows where not forcing it to the front
 *					could cause mouse capture and mouse lock to happen but without the window visible
 */
void FWindowsWindow::BringToFront( bool bForce )
{
	if ( IsRegularWindow() )
	{
		if (::IsIconic(HWnd))
		{
			::ShowWindow(HWnd, SW_RESTORE);
		}
		else
		{
			::SetActiveWindow(HWnd);
		}
	}
	else
	{
		HWND HWndInsertAfter = HWND_TOP;
		// By default we activate the window or it isn't actually brought to the front 
		uint32 Flags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER;

		if( !bForce )
		{
			Flags |= SWP_NOACTIVATE;
		}

		if( Definition->IsTopmostWindow )
		{
			HWndInsertAfter = HWND_TOPMOST;
		}

		::SetWindowPos( HWnd, HWndInsertAfter, 0, 0, 0, 0, Flags );
	}
}

void FWindowsWindow::HACK_ForceToFront()
{
	::SetForegroundWindow(HWnd);
}

/** Native windows should implement this function by asking the OS to destroy OS-specific resource associated with the window (e.g. Win32 window handle) */
void FWindowsWindow::Destroy()
{
	if (OLEReferenceCount > 0)
	{
		RevokeDragDrop( HWnd );
		check( OLEReferenceCount == 0 );
	}

	::DestroyWindow( HWnd );
}

/** Native window should implement this function by performing the equivalent of the Win32 minimize-to-taskbar operation */
void FWindowsWindow::Minimize()
{
	::ShowWindow(HWnd, SW_MINIMIZE);
}

/** Native window should implement this function by performing the equivalent of the Win32 maximize operation */
void FWindowsWindow::Maximize()
{
	::ShowWindow(HWnd, SW_MAXIMIZE);
}

/** Native window should implement this function by performing the equivalent of the Win32 maximize operation */
void FWindowsWindow::Restore()
{
	::ShowWindow(HWnd, SW_RESTORE);
}

/** Native window should make itself visible */
void FWindowsWindow::Show()
{
	if (!bIsVisible)
	{
		bIsVisible = true;

		// Do not activate windows that do not take input; e.g. tool-tips and cursor decorators
		// Also dont activate if a window wants to appear but not activate itself
		const bool bShouldActivate = Definition->AcceptsInput && Definition->ActivateWhenFirstShown;
		::ShowWindow(HWnd, bShouldActivate ? SW_SHOW : SW_SHOWNA);
	}
}

/** Native window should hide itself */
void FWindowsWindow::Hide()
{
	if (bIsVisible)
	{
		bIsVisible = false;
		::ShowWindow(HWnd, SW_HIDE);
	}
}

/** Toggle native window between fullscreen and normal mode */
void FWindowsWindow::SetWindowMode( EWindowMode::Type NewWindowMode )
{
	if( NewWindowMode != WindowMode )
	{
		bool bTrueFullscreen = NewWindowMode == EWindowMode::Fullscreen;

		// Setup Win32 Flags to be used for Fullscreen mode
		LONG WindowFlags = GetWindowLong(HWnd, GWL_STYLE);
		const LONG FullscreenFlags = WS_POPUP;
		const LONG RestoredFlags = WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CAPTION | WS_SYSMENU | WS_OVERLAPPED | WS_BORDER;

		// If we're not in fullscreen, make it so
		if( NewWindowMode == EWindowMode::WindowedFullscreen || NewWindowMode == EWindowMode::Fullscreen )
		{
			::GetWindowPlacement(HWnd, &PreFullscreenWindowPlacement);

			// Setup Win32 flags for fullscreen window
			WindowFlags &= ~RestoredFlags;
			WindowFlags |= FullscreenFlags;
			SetWindowLong(HWnd, GWL_STYLE, WindowFlags);

			if (!bTrueFullscreen)
			{
				// Ensure the window is restored if we are going for WindowedFullscreen
				ShowWindow(HWnd, SW_RESTORE);
			}

			// Get the current window position.
			RECT ClientRect;
			GetClientRect(HWnd, &ClientRect);

			// Grab current monitor data for sizing
			HMONITOR Monitor = MonitorFromWindow( HWnd, bTrueFullscreen ? MONITOR_DEFAULTTOPRIMARY : MONITOR_DEFAULTTONEAREST );
			MONITORINFO MonitorInfo;
			MonitorInfo.cbSize = sizeof(MONITORINFO);
			GetMonitorInfo( Monitor, &MonitorInfo );

			// Get the target client width to send to ReshapeWindow.
			// Preserve the current res if going to true fullscreen and the monitor supports it and allow the calling code
			// to resize if required.
			// Else, use the monitor's res for windowed fullscreen.
			LONG MonitorWidth  = MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left;
			LONG TargetClientWidth = bTrueFullscreen ?
				FMath::Min(MonitorWidth, ClientRect.right - ClientRect.left) :
				MonitorWidth;

			LONG MonitorHeight = MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top;
			LONG TargetClientHeight = bTrueFullscreen ?
				FMath::Min(MonitorHeight, ClientRect.bottom - ClientRect.top) :
				MonitorHeight;


			// Resize and position fullscreen window
			ReshapeWindow(
				MonitorInfo.rcMonitor.left,
				MonitorInfo.rcMonitor.top,
				TargetClientWidth,
				TargetClientHeight);
		}
		else
		{
			// Windowed:

			// Setup Win32 flags for restored window
			WindowFlags &= ~FullscreenFlags;
			WindowFlags |= RestoredFlags;
			SetWindowLong(HWnd, GWL_STYLE, WindowFlags);

			::SetWindowPlacement(HWnd, &PreFullscreenWindowPlacement);
		}

		WindowMode = NewWindowMode;
	}
}

/** @return true if the native window is maximized, false otherwise */
bool FWindowsWindow::IsMaximized() const
{
	bool bIsMaximized = !!::IsZoomed(HWnd);
	return bIsMaximized;
}

/** @return true if the native window is visible, false otherwise */
bool FWindowsWindow::IsVisible() const
{
	return bIsVisible;
}

/** Returns the size and location of the window when it is restored */
bool FWindowsWindow::GetRestoredDimensions(int32& X, int32& Y, int32& Width, int32& Height)
{
	WINDOWPLACEMENT WindowPlacement;
	WindowPlacement.length = sizeof( WINDOWPLACEMENT );

	if( ::GetWindowPlacement( HWnd, &WindowPlacement) != 0 )
	{
		// This window may be minimized.  Get the position when it is restored.
		const RECT Restored = WindowPlacement.rcNormalPosition;

		X = Restored.left;
		Y = Restored.top;
		Width = Restored.right - Restored.left;
		Height = Restored.bottom - Restored.top;
		return true;
	}
	else
	{
		return false;
	}
}


void FWindowsWindow::AdjustCachedSize( FVector2D& Size ) const
{
	if( Definition.IsValid() && Definition->SizeWillChangeOften )
	{
		Size = FVector2D( VirtualWidth, VirtualHeight );
	}
	else if(HWnd)
	{
		RECT ClientRect;
		::GetClientRect(HWnd,&ClientRect);
		Size.X = ClientRect.right - ClientRect.left;
		Size.Y = ClientRect.bottom - ClientRect.top;
	}
}

/** Sets focus on the native window */
void FWindowsWindow::SetWindowFocus()
{
	if( GetFocus() != HWnd )
	{
		::SetFocus( HWnd );
	}
}

/**
 * Sets the opacity of this window
 *
 * @param	InOpacity	The new window opacity represented as a floating point scalar
 */
void FWindowsWindow::SetOpacity( const float InOpacity )
{
	SetLayeredWindowAttributes( HWnd, 0, FMath::Trunc( InOpacity * 255.0f ), LWA_ALPHA );
}

/**
 * Enables or disables this window.  When disabled, a window will receive no input.       
 *
 * @param bEnable	true to enable the window, false to disable it.
 */
void FWindowsWindow::Enable( bool bEnable )
{
	::EnableWindow( HWnd, bEnable );
}

/** @return true if native window exists underneath the coordinates */
bool FWindowsWindow::IsPointInWindow( int32 X, int32 Y ) const
{
	bool Result = false;

	HRGN Region = MakeWindowRegionObject();
	Result = !!PtInRegion( Region, X, Y );
	DeleteObject( Region );

	return Result;
}

int32 FWindowsWindow::GetWindowBorderSize() const
{
	WINDOWINFO WindowInfo;
	FMemory::MemZero( WindowInfo );
	WindowInfo.cbSize = sizeof( WindowInfo );
	::GetWindowInfo( HWnd, &WindowInfo );

	return WindowInfo.cxWindowBorders;
}

bool FWindowsWindow::IsForegroundWindow() const
{
	return ::GetForegroundWindow() == HWnd;
}

void FWindowsWindow::SetText( const TCHAR* const Text )
{
	SetWindowText(HWnd, Text);
}


bool FWindowsWindow::IsRegularWindow() const
{
	return Definition->IsRegularWindow;
}

HRESULT STDCALL FWindowsWindow::QueryInterface( REFIID iid, void ** ppvObject )
{
	if ( IID_IDropTarget == iid || IID_IUnknown == iid )
	{
		AddRef();
		*ppvObject = this;
		return S_OK;
	}
	else
	{
		*ppvObject = NULL;
		return E_NOINTERFACE;
	}
}

ULONG STDCALL FWindowsWindow::AddRef( void )
{
	// We only do this for correctness checking
	FPlatformAtomics::InterlockedIncrement( &OLEReferenceCount );
	return OLEReferenceCount;
}

ULONG STDCALL FWindowsWindow::Release( void )
{
	// We only do this for correctness checking
	FPlatformAtomics::InterlockedDecrement( &OLEReferenceCount );
	return OLEReferenceCount;
}

HRESULT STDCALL FWindowsWindow::DragEnter( __RPC__in_opt IDataObject *DataObjectPointer, ::DWORD KeyState, POINTL CursorPosition, __RPC__inout ::DWORD *CursorEffect )
{
	if (IsInGameThread())
	{
		return OwningApplication->OnOLEDragEnter(HWnd, DataObjectPointer, KeyState, CursorPosition, CursorEffect);
	}
	else
	{
		UE_LOG(LogWindowsDesktop, Log, TEXT("DragEnter invoked from another thread"));
		return 0;
	}
}

HRESULT STDCALL FWindowsWindow::DragOver( ::DWORD KeyState, POINTL CursorPosition, __RPC__inout ::DWORD *CursorEffect )
{
	if (IsInGameThread())
	{
		return OwningApplication->OnOLEDragOver(HWnd, KeyState, CursorPosition, CursorEffect);
	}
	else
	{
		UE_LOG(LogWindowsDesktop, Log, TEXT("DragOver invoked from another thread"));
		return 0;
	}
}

HRESULT STDCALL FWindowsWindow::DragLeave( void )
{
	if (IsInGameThread())
	{
		return OwningApplication->OnOLEDragOut(HWnd);
	}
	else
	{
		UE_LOG(LogWindowsDesktop, Log, TEXT("DragLeave invoked from another thread"));
		return 0;
	}
}

HRESULT STDCALL FWindowsWindow::Drop( __RPC__in_opt IDataObject *DataObjectPointer, ::DWORD KeyState, POINTL CursorPosition, __RPC__inout ::DWORD *CursorEffect )
{
	if (IsInGameThread())
	{
		return OwningApplication->OnOLEDrop(HWnd, DataObjectPointer, KeyState, CursorPosition, CursorEffect);
	}
	else
	{
		UE_LOG(LogWindowsDesktop, Log, TEXT("OLE Drop invoked from another thread"));
		return 0;
	}
}
