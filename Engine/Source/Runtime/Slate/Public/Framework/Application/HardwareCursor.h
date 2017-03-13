// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ICursor.h"

#if PLATFORM_WINDOWS
#include "WindowsHWrapper.h"
#endif

struct SDL_Cursor;

/**
 * Provides a cross desktop platform solution for providing hardware cursors.  These cursors
 * generally require platform specific formats.  To try and combat this, this class standardizes
 * on .png files.  However, on different platforms that support it, it searches for platform specific
 * formats first if you want to take advantage of those capabilities.
 *
 * Windows:
 *   .ani -> .cur -> .png
 *
 * Mac:
 *   .tiff -> .png
 *
 * Linux:
 *   .png
 *
 * Additionally, you have the option to include different resolutions.  On Windows the typical size
 * cursors are 32x32 or 64x64.  On Macs, a multi-resolution tiff can be provided.
 *
 * Mac: https://developer.apple.com/library/content/documentation/GraphicsAnimation/Conceptual/HighResolutionOSX/Optimizing/Optimizing.html
 *
 * In order to deal with this, your cursor path should include the base cursor with no size information
 * however, you should name them like this, Pointer_32x32.png and Pointer_64x64.png.  This only applies for
 * .png, since .cur|.ani|.tiff already support multiple resolutions.
 *
 * If no exact size is found, the closest cursor in size is used.
 */
class SLATE_API FHardwareCursor
{
public:
	/**
	 * HotSpot needs to be in normalized UV coordinates since it may apply to different resolution images. 
	 */
	FHardwareCursor(const FString& InPathToCursorWithoutExtension, FVector2D HotSpot);
	FHardwareCursor(const TArray<FColor>& Pixels, FIntPoint Size, FVector2D HotSpot);

	~FHardwareCursor();

	/** Gets the platform specific handle to the cursor that was allocated.  If loading the cursor failed, this value will be null. */
	void* GetHandle();

	/**
	 * Replaces the shape for the given cursor for this platform.  So if you wanted to replace the default
	 * arrow cursor on the platform with this custom hardware cursor, you would pass in EMouseCursor::Default.
	 */
	void ReplaceTheShapeFor(EMouseCursor::Type InCursorType);

private:
	void CreateCursorFromRGBABuffer(const FColor* Pixels, int32 Width, int32 Height, FVector2D InHotSpot);

	bool FindAndLoadCorrectResolutionPng(TArray<uint8>& Result, const FString& InPathToCursorWithoutExtension);

private:
#if PLATFORM_WINDOWS
	HCURSOR CursorHandle;
#elif PLATFORM_MAC
	NSCursor* CursorHandle;
#elif PLATFORM_LINUX
	SDL_Cursor* CursorHandle;
#endif
};
