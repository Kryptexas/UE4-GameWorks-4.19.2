// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Framework/Application/HardwareCursor.h"
#include "Logging/LogMacros.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "ModuleManager.h"
#include "Interfaces/IImageWrapperModule.h"
#include "SlateApplication.h"

#if PLATFORM_LINUX
#include "SDL.h"
#include "SDL_surface.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogHardwareCursor, Log, All);

FHardwareCursor::FHardwareCursor(const FString& InPathToCursorWithoutExtension, FVector2D InHotSpot)
#if PLATFORM_WINDOWS
	: CursorHandle(NULL)
#elif PLATFORM_MAC
	: CursorHandle(nullptr)
#endif
{
	// The hotspot should be in normalized coordinates, because we multiply it by the width & height of the
	// actual image we load into memory, which may vary per platform, to get the actual Pixel Hotspot.
	check(InHotSpot.X >= 0.0f && InHotSpot.X <= 1.0f);
	check(InHotSpot.Y >= 0.0f && InHotSpot.Y <= 1.0f);

	// NOTE:
	// The reason we don't include the file extension is so we can support prioritization
	// of the different formats supported on each platform.  So on Windows we can support .ani -> .cur -> .png.
	//
	// Additionally - we load all the files into memory and then find ways to then convert them into
	// OS cursors.  Several of the OSes have functions for loading a cursor from a file,
	// which works great, unless your game uses Pak files.

#if PLATFORM_WINDOWS

	const FString AniCursor = InPathToCursorWithoutExtension + TEXT(".ani");
	const FString CurCursor = InPathToCursorWithoutExtension + TEXT(".cur");

	TArray<uint8> CursorFileData;

	if (FFileHelper::LoadFileToArray(CursorFileData, *AniCursor, FILEREAD_Silent) || FFileHelper::LoadFileToArray(CursorFileData, *CurCursor, FILEREAD_Silent))
	{
		//TODO Would be nice to find a way to do this that doesn't involve the temp file copy.

		// The cursors may be in a pak file, if that's the case we need to write it to a temporary file
		// and then load that file as the cursor.  It's a workaround because there doesn't appear to be
		// a good way to load a cursor from anything other than a loose file or a resource.
		FString TempCursorFile = FPaths::CreateTempFilename(FPlatformProcess::UserTempDir(), TEXT("Cursor-"), TEXT(".temp"));
		if (FFileHelper::SaveArrayToFile(CursorFileData, *TempCursorFile))
		{
			CursorHandle = (HCURSOR)LoadImage(NULL,
				*TempCursorFile,
				IMAGE_CURSOR,
				0,
				0,
				LR_LOADFROMFILE);

			IFileManager::Get().Delete(*TempCursorFile);
		}
	}
	else if (FindAndLoadCorrectResolutionPng(CursorFileData, InPathToCursorWithoutExtension))
	{
		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));

		IImageWrapperPtr PngImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
		if (PngImageWrapper.IsValid() && PngImageWrapper->SetCompressed(CursorFileData.GetData(), CursorFileData.Num()))
		{
			const TArray<uint8>* RawImageData = nullptr;
			if (PngImageWrapper->GetRaw(ERGBFormat::RGBA, 8, RawImageData))
			{
				const int32 Width = PngImageWrapper->GetWidth();
				const int32 Height = PngImageWrapper->GetHeight();

				CreateCursorFromRGBABuffer((FColor*)RawImageData->GetData(), Width, Height, InHotSpot);
			}
		}
	}

#elif PLATFORM_MAC

	const FString TiffCursor = InPathToCursorWithoutExtension + TEXT(".tiff");

	TArray<uint8> CursorFileData;
	if (FFileHelper::LoadFileToArray(CursorFileData, *TiffCursor, FILEREAD_Silent) || FindAndLoadCorrectResolutionPng(CursorFileData, InPathToCursorWithoutExtension))
	{
		NSData* CursorData = [[NSData alloc] initWithBytes:CursorFileData.GetData() length : CursorFileData.Num()];
		NSImage* CursorImage = [[NSImage alloc] initWithData:CursorData];
		NSImageRep* CursorImageRep = [[CursorImage representations] objectAtIndex:0];

		const int32 PixelHotspotX = FMath::RoundToInt(InHotSpot.X * CursorImageRep.pixelsWide);
		const int32 PixelHotspotY = FMath::RoundToInt(InHotSpot.Y * CursorImageRep.pixelsHigh);

		CursorHandle = [[NSCursor alloc] initWithImage:CursorImage hotSpot : NSMakePoint(PixelHotspotX, PixelHotspotY)];
		[CursorImage release];
		[CursorData release];
	}

#elif PLATFORM_LINUX

	TArray<uint8> CursorFileData;
	if (FindAndLoadCorrectResolutionPng(CursorFileData, InPathToCursorWithoutExtension))
	{
		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));

		IImageWrapperPtr PngImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
		if (PngImageWrapper.IsValid() && PngImageWrapper->SetCompressed(CursorFileData.GetData(), CursorFileData.Num()))
		{
			const TArray<uint8>* RawImageData = nullptr;
			if (PngImageWrapper->GetRaw(ERGBFormat::RGBA, 8, RawImageData))
			{
				const int32 Width = PngImageWrapper->GetWidth();
				const int32 Height = PngImageWrapper->GetHeight();

				CreateCursorFromRGBABuffer((FColor*)RawImageData->GetData(), Width, Height, InHotSpot);
			}
		}
	}

#endif

#if PLATFORM_WINDOWS || PLATFORM_MAC || PLATFORM_LINUX

	if (!GetHandle())
	{
		UE_LOG(LogHardwareCursor, Error, TEXT("Failed to load cursor '%s'.  If you included a file extension or the size information '_WidthxHeight' remove them."), *InPathToCursorWithoutExtension);
	}

#endif
}

FHardwareCursor::FHardwareCursor(const TArray<FColor>& Pixels, FIntPoint InSize, FVector2D InHotSpot)
#if PLATFORM_WINDOWS
	: CursorHandle(NULL)
#elif PLATFORM_MAC
	: CursorHandle(nullptr)
#endif
{
	// The hotspot should be in normalized coordinates, because we multiply it by the width & height of the
	// actual image we load into memory, which may vary per platform, to get the actual Pixel Hotspot.
	check(InHotSpot.X >= 0.0f && InHotSpot.X <= 1.0f);
	check(InHotSpot.Y >= 0.0f && InHotSpot.Y <= 1.0f);

	const int32 Width = InSize.X;
	const int32 Height = InSize.Y;

	CreateCursorFromRGBABuffer((FColor*)Pixels.GetData(), Width, Height, InHotSpot);
}

void FHardwareCursor::CreateCursorFromRGBABuffer(const FColor* Pixels, int32 Width, int32 Height, FVector2D InHotSpot)
{
#if PLATFORM_WINDOWS

	TArray<FColor> BGRAPixels;
	BGRAPixels.AddUninitialized(Width * Height);

	for (int32 Index = 0; Index < BGRAPixels.Num(); Index++)
	{
		const FColor& SrcPixel = Pixels[Index];
		BGRAPixels[Index] = FColor(SrcPixel.B, SrcPixel.G, SrcPixel.R, SrcPixel.A);
	}

	// The bitmap created is already in BGRA format, so we can just hand over the buffer.
	HBITMAP CursorColor = ::CreateBitmap(Width, Height, 1, 32, BGRAPixels.GetData());
	// Just create a dummy mask, we're making a full 32bit bitmap with support for alpha, so no need to worry about the mask.
	HBITMAP CursorMask = ::CreateCompatibleBitmap(::GetDC(NULL), Width, Height);

	ICONINFO IconInfo = { 0 };
	IconInfo.fIcon = FALSE;
	IconInfo.xHotspot = FMath::RoundToInt(InHotSpot.X * Width);
	IconInfo.yHotspot = FMath::RoundToInt(InHotSpot.Y * Height);
	IconInfo.hbmColor = CursorColor;
	IconInfo.hbmMask = CursorMask;

	CursorHandle = ::CreateIconIndirect(&IconInfo);

	::DeleteObject(CursorColor);
	::DeleteObject(CursorMask);

#elif PLATFORM_MAC

	NSBitmapImageRep* CursorImageRep = [
		[NSBitmapImageRep alloc]
			initWithBitmapDataPlanes:NULL
			pixelsWide : Width
			pixelsHigh : Height
			bitsPerSample : 8
			samplesPerPixel : 4
			hasAlpha : YES
			isPlanar : NO
			colorSpaceName : NSCalibratedRGBColorSpace
			bitmapFormat : NSAlphaFirstBitmapFormat
			bytesPerRow : Width * 4
			bitsPerPixel : 32];

	uint8* CursorPixels = [CursorImageRep bitmapData];
	for (int32 X = 0; X < Width; X++)
	{
		for (int32 Y = 0; Y < Height; Y++)
		{
			uint8* TargetPixel = (uint8*)CursorPixels + Y * Width + X * 4;
			*(uint32*)TargetPixel = *(((uint32*)Pixels) + (Y * Width) + X);
		}
	}

	NSImage* CursorImage = [[NSImage alloc] initWithSize:NSMakeSize(Width, Height)];
	[CursorImage addRepresentation : CursorImageRep];

	const int32 PixelHotspotX = FMath::RoundToInt(InHotSpot.X * Width);
	const int32 PixelHotspotY = FMath::RoundToInt(InHotSpot.Y * Height);

	CursorHandle = [[NSCursor alloc] initWithImage:CursorImage hotSpot : NSMakePoint(PixelHotspotX, PixelHotspotY)];
	[CursorImage release];
	[CursorImageRep release];

#elif PLATFORM_LINUX

	uint32 Rmask = 0x000000ff;
	uint32 Gmask = 0x0000ff00;
	uint32 Bmask = 0x00ff0000;
	uint32 Amask = 0xff000000;

	SDL_Surface* Surface = SDL_CreateRGBSurfaceFrom((void*)Pixels, Width, Height, 32, Width * 4, Rmask, Gmask, Bmask, Amask);
	if (Surface)
	{
		CursorHandle = SDL_CreateColorCursor(Surface, FMath::RoundToInt(InHotSpot.X * Width), FMath::RoundToInt(InHotSpot.Y * Height));

		SDL_FreeSurface(Surface);
	}

#endif
}

bool FHardwareCursor::FindAndLoadCorrectResolutionPng(TArray<uint8>& Result, const FString& InPathToCursorWithoutExtension)
{
#if PLATFORM_WINDOWS
	const int32 SystemCursorWidth = ::GetSystemMetrics(SM_CXCURSOR);
	const int32 SystemCursorHeight = ::GetSystemMetrics(SM_CYCURSOR);
#elif PLATFORM_MAC
	// TODO: Is there a way to tell the current desired cursor size on OSX?
	const int32 SystemCursorWidth = 32;
	const int32 SystemCursorHeight = 32;
#elif PLATFORM_LINUX
	// TODO: Is there a way to tell the current desired cursor size on Linux ?
	const int32 SystemCursorWidth = 32;
	const int32 SystemCursorHeight = 32;
#endif

#if PLATFORM_DESKTOP

	FString CursorsWithSizeSearch = FPaths::GetCleanFilename(InPathToCursorWithoutExtension) + TEXT("_*x*.png");

	TArray<FString> PngCursorFiles;
	IFileManager::Get().FindFilesRecursive(PngCursorFiles, *FPaths::GetPath(InPathToCursorWithoutExtension), *CursorsWithSizeSearch, true, false, false);

	FString PngCursorPath = InPathToCursorWithoutExtension + TEXT(".png");
	int32 SizeDistance = INT_MAX;
	int32 CursorWidth = 0;
	int32 CursorHeight = 0;

	for (const FString& CursorFile : PngCursorFiles)
	{
		FString Dummy;
		FString SizeAndExtension;
		FString Size;
		FString Width, Height;

		if (!CursorFile.Split(TEXT("_"), &Dummy, &SizeAndExtension, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
		{
			continue;
		}

		if (!SizeAndExtension.Split(TEXT("."), &Size, &Dummy))
		{
			continue;
		}

		if (!Size.Split(TEXT("x"), &Width, &Height))
		{
			continue;
		}

		if (FCString::IsPureAnsi(*Width) && FCString::IsPureAnsi(*Height))
		{
			int32 TempCursorWidth = FCString::Atoi(*Width);
			int32 TempCursorHeight = FCString::Atoi(*Height);

			int32 Distance = FMath::Abs(SystemCursorWidth - CursorWidth);
			if (Distance < SizeDistance)
			{
				PngCursorPath = CursorFile;
				CursorWidth = TempCursorWidth;
				CursorHeight = TempCursorHeight;
				SizeDistance = Distance;

				if (SizeDistance == 0)
				{
					break;
				}
			}
		}
	}

	if (FFileHelper::LoadFileToArray(Result, *PngCursorPath, FILEREAD_Silent))
	{
		UE_LOG(LogHardwareCursor, Log, TEXT("Loading Cursor '%s'."), *PngCursorPath);

		if ( SizeDistance != 0 )
		{
			FString PngCursorFileName = FPaths::GetCleanFilename(PngCursorPath);
			UE_LOG(LogHardwareCursor, Warning, TEXT("The cursor '%s' is %dx%d, but the OS requested a cursor of size %dx%d.  You should add another variation to get a pixel perfect cursor."), 
				*PngCursorFileName, CursorWidth, CursorHeight, SystemCursorWidth, SystemCursorHeight);
		}

		return true;
	}
	else
	{
		UE_LOG(LogHardwareCursor, Error, TEXT("Failed to load cursor '%s', couldn't find any matching cursors."), *PngCursorPath);
	}

#endif

	return false;
}

FHardwareCursor::~FHardwareCursor()
{
#if PLATFORM_WINDOWS
	if (CursorHandle)
	{
		DestroyCursor(CursorHandle);
	}
#elif PLATFORM_MAC
	if (CursorHandle)
	{
		[CursorHandle release];
	}
#elif PLATFORM_LINUX
	if (CursorHandle)
	{
		SDL_FreeCursor(CursorHandle);
	}
#endif
}

void* FHardwareCursor::GetHandle()
{
#if PLATFORM_WINDOWS
	return CursorHandle;
#elif PLATFORM_MAC
	return CursorHandle;
#elif PLATFORM_LINUX
	return CursorHandle;
#else
	return nullptr;
#endif
}

void FHardwareCursor::ReplaceTheShapeFor(EMouseCursor::Type InCursorType)
{
	TSharedPtr<ICursor> PlatformCursor = FSlateApplication::Get().GetPlatformCursor();

	if (ICursor* Cursor = PlatformCursor.Get())
	{
		Cursor->SetTypeShape(InCursorType, GetHandle());
	}
}