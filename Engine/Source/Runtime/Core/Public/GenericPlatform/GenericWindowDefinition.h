// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/UnrealString.h"
#include "HAL/Platform.h"
#include "GenericApplicationMessageHandler.h"


#ifndef ALPHA_BLENDED_WINDOWS
	#define ALPHA_BLENDED_WINDOWS IS_PROGRAM || WITH_EDITOR
#endif


/** Enumeration to specify different window types for SWindows */
enum class EWindowType
{
	/** Value indicating that this is a standard, general-purpose window */
	Normal,
	/** Value indicating that this is a window used for a popup menu */
	Menu,
	/** Value indicating that this is a window used for a tooltip */
	ToolTip,
	/** Value indicating that this is a window used for a notification toast */
	Notification,
	/** Value indicating that this is a window used for a cursor decorator */
	CursorDecorator
};


/** Enumeration to specify different transparency options for SWindows */
enum class EWindowTransparency
{
	/** Value indicating that a window does not support transparency */
	None,

	/** Value indicating that a window supports transparency at the window level (one opacity applies to the entire window) */
	PerWindow,

#if ALPHA_BLENDED_WINDOWS
	/** Value indicating that a window supports per-pixel alpha blended transparency */
	PerPixel,
#endif
};


struct CORE_API FGenericWindowDefinition
{
	/** Window type */
	EWindowType Type;
	
	/** The initially desired horizontal screen position */
	float XDesiredPositionOnScreen;
	/** The initially desired vertical screen position */
	float YDesiredPositionOnScreen;

	/** The initially desired width */
	float WidthDesiredOnScreen;
	/** The initially desired height */
	float HeightDesiredOnScreen;

	/** the level of transparency supported by this window */
	EWindowTransparency TransparencySupport;

	/** true if the window is using the os window border instead of a slate created one */
	bool HasOSWindowBorder;
	/** should this window show up in the taskbar */
	bool AppearsInTaskbar;
	/** true if the window should be on top of all other windows; false otherwise */
	bool IsTopmostWindow;
	/** true if the window accepts input; false if the window is non-interactive */
	bool AcceptsInput;
	/** true if this window will be activated when it is first shown */
	bool ActivateWhenFirstShown;
	/** true if this window will be focused when it is first shown */
	bool FocusWhenFirstShown;
	/** true if this window displays an enabled close button on the toolbar area */
	bool HasCloseButton;
	/** true if this window displays an enabled minimize button on the toolbar area */
	bool SupportsMinimize;
	/** true if this window displays an enabled maximize button on the toolbar area */
	bool SupportsMaximize;

	/** true if the window is modal (prevents interacting with its parent) */
	bool IsModalWindow;
	/** true if this is a vanilla window, or one being used for some special purpose: e.g. tooltip or menu */
	bool IsRegularWindow;
	/** true if this is a user-sized window with a thick edge */
	bool HasSizingFrame;
	/** true if we expect the size of this window to change often, such as if its animated, or if it recycled for tool-tips. */
	bool SizeWillChangeOften;
	/** The expected maximum width of the window.  May be used for performance optimization when SizeWillChangeOften is set. */
	int32 ExpectedMaxWidth;
	/** The expected maximum height of the window.  May be used for performance optimization when SizeWillChangeOften is set. */
	int32 ExpectedMaxHeight;

	/** the title of the window */
	FString Title;
	/** opacity of the window (0-1) */
	float Opacity;
	/** the radius of the corner rounding of the window */
	int32 CornerRadius;

	FWindowSizeLimits SizeLimits;
};

// video driver details
struct FGPUDriverInfo
{
	FGPUDriverInfo()
		: VendorId(0)
	{
	}

	// DirectX vendor Id, use functions below
	uint32 VendorId;
	// e.g. "NVIDIA GeForce GTX 680" or "AMD Radeon R9 200 / HD 7900 Series"
	FString DeviceDescription;
	// e.g. "NVIDIA" or "Advanced Micro Devices, Inc."
	FString ProviderName;
	// e.g. "15.200.1062.1004"(AMD)
	// e.g. "9.18.13.4788"(NVIDIA) first number is Windows version (e.g. 7:Vista, 6:XP, 4:Me, 9:Win8(1), 10:Win7), last 5 have the UserDriver version encoded
	FString InternalDriverVersion;	
	// e.g. "Catalyst 15.7.1"(AMD) or "347.88"(NVIDIA)
	FString UserDriverVersion;
	// e.g. 3-13-2015
	FString DriverDate;

	bool IsValid() const
	{
		return !DeviceDescription.IsEmpty();
	}
	
	bool IsAMD() const { return VendorId == 0x1002; }

	bool IsIntel() const { return VendorId == 0x8086; }

	bool IsNVIDIA() const { return VendorId == 0x10DE; }
};