// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "Core.h"
#include "ImageCore.h"
#include "ModuleInterface.h"
#include "ModuleManager.h"
#include "TargetPlatform.h"
#include "TextureCompressorModule.h"
#include "PixelFormat.h"
#include "IConsoleManager.h"
#include "TaskGraphInterfaces.h"

#include "ispc_texcomp.h"

DEFINE_LOG_CATEGORY_STATIC(LogTextureFormatIntelISPCTexComp, Log, All);

/**
 * Macro trickery for supported format names.
 */
#define ENUM_SUPPORTED_FORMATS(op) \
	op(BC6H) \
	op(BC7)

#define DECL_FORMAT_NAME(FormatName) static FName GTextureFormatName##FormatName = FName(TEXT(#FormatName));
ENUM_SUPPORTED_FORMATS(DECL_FORMAT_NAME);
#undef DECL_FORMAT_NAME

#define DECL_FORMAT_NAME_ENTRY(FormatName) GTextureFormatName##FormatName ,
static FName GSupportedTextureFormatNames[] =
{
	ENUM_SUPPORTED_FORMATS(DECL_FORMAT_NAME_ENTRY)
};
#undef DECL_FORMAT_NAME_ENTRY

#undef ENUM_SUPPORTED_FORMATS

/**
 * BC6H Compression function
 */
static void IntelBC6HCompressScans(bc6h_enc_settings* pEncSettings, FImage* pInImage, FCompressedImage2D* pOutImage, int yStart, int yEnd, int OutSliceSize)
{
	check(pInImage->Format == ERawImageFormat::RGBA16F);
	check((yStart % 4) == 0);
	check((yStart >= 0) && (yStart <= pInImage->SizeY));
	check((yEnd   >= 0) && (yEnd   <= pInImage->SizeY));

	uint8* pInTexels = reinterpret_cast<uint8*>(&pInImage->RawData[0]);
	const int iInStride = pInImage->SizeX * 8;
	uint8* pOutTexels = reinterpret_cast<uint8*>(&pOutImage->RawData[0]);

	rgba_surface insurface;
	insurface.ptr		= pInTexels + (yStart * iInStride);
	insurface.width		= pInImage->SizeX;
	insurface.height	= yEnd - yStart;
	insurface.stride	= pInImage->SizeX * 8;

	pOutTexels += ((yStart + 3) / 4) * ((pInImage->SizeX + 3) / 4) * 16;
	for (int Slice = 0 ; Slice < pInImage->NumSlices ; Slice++)
	{
		CompressBlocksBC6H(&insurface, pOutTexels, pEncSettings);
		pOutTexels += OutSliceSize;
		insurface.ptr += 8 * pInImage->SizeX * pInImage->SizeY;
	}
}

/**
 * BC7 Compression function
 */
static void IntelBC7CompressScans(bc7_enc_settings* pEncSettings, FImage* pInImage, FCompressedImage2D* pOutImage, int yStart, int yEnd)
{
	check(pInImage->Format == ERawImageFormat::BGRA8);
	check((yStart % 4) == 0);
	check((yStart >= 0) && (yStart <= pInImage->SizeY));
	check((yEnd   >= 0) && (yEnd   <= pInImage->SizeY));

	uint8* pInTexels = reinterpret_cast<uint8*>(&pInImage->RawData[0]);
	const int iInStride = pInImage->SizeX * 4;
	uint8* pOutTexels = reinterpret_cast<uint8*>(&pOutImage->RawData[0]);

	// Switch byte order for compressors input
	for ( int y=yStart; y < yEnd; ++y )
	{
		uint8* pInTexelsSwap = pInTexels + (y * iInStride);
		for ( int x=0; x < pInImage->SizeX; ++x )
		{
			const uint8 r = pInTexelsSwap[0];
			pInTexelsSwap[0] = pInTexelsSwap[2];
			pInTexelsSwap[2] = r;

			pInTexelsSwap += 4;
		}
	}

	rgba_surface insurface;
	insurface.ptr		= pInTexels + (yStart * iInStride);
	insurface.width		= pInImage->SizeX;
	insurface.height	= yEnd - yStart;
	insurface.stride	= pInImage->SizeX * 4;

	pOutTexels += ((yStart + 3) / 4) * ((pInImage->SizeX + 3) / 4) * 16;
	CompressBlocksBC7(&insurface, pOutTexels, pEncSettings);
}


/** Expands any dimension to multiple of 4.  Copies the nearest row/column */
static bool ExpandTo4PxMultiple(const FImage& InImage, FImage& OutImage)
{	
	// Calculate how many times to duplicate each row or column
	const int32 FillX = InImage.SizeX % 4 > 0 ? 4 - InImage.SizeX % 4 : 0;
	const int32 FillY = InImage.SizeY % 4 > 0 ? 4 - InImage.SizeY % 4 : 0;
	
	// Early out
	if (FillX == 0 && FillY == 0)
	{
		return false;
	}

	const int32 NewSizeX = InImage.SizeX + FillX;
	const int32 NewSizeY = InImage.SizeY + FillY;
		
	// Allocate room to fill out into
	int32 BytesPerPixel = InImage.GetBytesPerPixel();
	TArray<uint8> ResizedRawData;
	ResizedRawData.SetNumUninitialized(NewSizeX * NewSizeY * InImage.NumSlices * BytesPerPixel);
	
	int32 SourceSliceSize = InImage.SizeX * InImage.SizeY * BytesPerPixel;
	int32 DestSliceSize = NewSizeX * NewSizeY * BytesPerPixel;
	for ( int32 SliceIndex=0; SliceIndex < InImage.NumSlices; ++SliceIndex )
	{
		uint8* SourceData = ((uint8*)InImage.RawData.GetData()) + SliceIndex * SourceSliceSize;
		uint8* DestData = ((uint8*)ResizedRawData.GetData()) + SliceIndex * DestSliceSize;
		for ( int32 Y = 0; Y < NewSizeY; ++Y )
		{
			for ( int32 X = 0; X < NewSizeX; ++X )
			{
				uint8* DestColor = DestData + Y*BytesPerPixel * NewSizeX + X*BytesPerPixel;
				int32 SourceX = FMath::Min(X, InImage.SizeX-1);
				int32 SourceY = FMath::Min(Y, InImage.SizeY-1);
				uint8* SourceColor = SourceData + SourceY*BytesPerPixel * InImage.SizeX + SourceX*BytesPerPixel;
				FMemory::Memcpy(DestColor, SourceColor, BytesPerPixel);
			}
		}
	}

	// Put the new image data into the OutImage
	OutImage.Init(NewSizeX, NewSizeY, InImage.NumSlices, InImage.Format, InImage.GammaSpace);
	FMemory::Memcpy(OutImage.RawData.GetData(), ResizedRawData.GetData(), NewSizeX * NewSizeY * OutImage.NumSlices * BytesPerPixel);

	return true;
}

/**
 * Intel BC texture format handler.
 */
class FTextureFormatIntelISPCTexComp : public ITextureFormat
{
	virtual bool AllowParallelBuild() const override
	{
		return true;
	}

	virtual uint16 GetVersion(FName Format) const override
	{
		return 1;
	}

	virtual void GetSupportedFormats(TArray<FName>& OutFormats) const override
	{
		for (int32 i = 0; i < ARRAY_COUNT(GSupportedTextureFormatNames); ++i)
		{
			OutFormats.Add(GSupportedTextureFormatNames[i]);
		}
	}

	virtual FTextureFormatCompressorCaps GetFormatCapabilities() const override
	{
		return FTextureFormatCompressorCaps(); // Default capabilities.
	}

	virtual bool CompressImage(
		const FImage& InImage,
		const struct FTextureBuildSettings& BuildSettings,
		bool bImageHasAlphaChannel,
		FCompressedImage2D& OutCompressedImage
		) const override
	{
		bool bCompressionSucceeded = false;

		const int iWidthInBlocks	= ((InImage.SizeX + 3) & ~ 3) / 4;
		const int iHeightInBlocks	= ((InImage.SizeY + 3) & ~ 3) / 4;
		const int iOutputSliceSizeBytes = iWidthInBlocks * iHeightInBlocks * 16;
		const int iOutputBytes		= iOutputSliceSizeBytes * InImage.NumSlices;
		OutCompressedImage.RawData.AddUninitialized(iOutputBytes);

		// When we allow async tasks to execute we do so with 4 lines of the image per task
		// This isn't optimal for long thin textures, but works well with how ISPC works
		const int iScansPerTask = 4;
		const int iNumTasks = FMath::Max((InImage.SizeY / iScansPerTask) - 1, 0);
		const bool bUseTasks = true;

		EPixelFormat CompressedPixelFormat = PF_Unknown;
		if ( BuildSettings.TextureFormatName == GTextureFormatNameBC6H )
		{
			FImage Image;
			// Expand any dimension less than 4 to 4.  Intel compressor doens't seem to do this automatically.
			FImage ResizedImage;
			if (ExpandTo4PxMultiple(InImage, ResizedImage))
			{
				ResizedImage.CopyTo(Image, ERawImageFormat::RGBA16F, EGammaSpace::Linear);
			}
			else
			{
				InImage.CopyTo(Image, ERawImageFormat::RGBA16F, EGammaSpace::Linear);
			}

			bc6h_enc_settings settings;
			GetProfile_bc6h_basic(&settings);

			if ( bUseTasks )
			{
				class FIntelCompressWorker : public FNonAbandonableTask
				{
				public:
					FIntelCompressWorker(bc6h_enc_settings* pEncSettings, FImage* pInImage, FCompressedImage2D* pOutImage, int yStart, int yEnd, int iOutputSliceSizeBytes)
						: mpEncSettings(pEncSettings)
						, mpInImage(pInImage)
						, mpOutImage(pOutImage)
						, mYStart(yStart)
						, mYEnd(yEnd)
						, mOutputSliceSizeBytes(iOutputSliceSizeBytes)
					{
					}

					void DoWork()
					{
						IntelBC6HCompressScans(mpEncSettings, mpInImage, mpOutImage, mYStart, mYEnd, mOutputSliceSizeBytes);
					}

					FORCEINLINE TStatId GetStatId() const
					{
						RETURN_QUICK_DECLARE_CYCLE_STAT(FIntelCompressWorker, STATGROUP_ThreadPoolAsyncTasks);
					}

					bc6h_enc_settings*	mpEncSettings;
					FImage*				mpInImage;
					FCompressedImage2D*	mpOutImage;
					int					mYStart;
					int					mYEnd;
					int					mOutputSliceSizeBytes;
				};
				typedef FAsyncTask<FIntelCompressWorker> FIntelCompressTask;

				// One less task because we'll do the final + non multiple of 4 inside this task
				TIndirectArray<FIntelCompressTask> CompressionTasks;
				CompressionTasks.Reserve(iNumTasks);
				for ( int iTask=0; iTask < iNumTasks; ++iTask )
				{
					auto* AsyncTask = new(CompressionTasks) FIntelCompressTask(&settings, &Image, &OutCompressedImage, iTask * iScansPerTask, (iTask + 1) * iScansPerTask, iOutputSliceSizeBytes);
					AsyncTask->StartBackgroundTask();
				}
				IntelBC6HCompressScans(&settings, &Image, &OutCompressedImage, iScansPerTask * iNumTasks, Image.SizeY, iOutputSliceSizeBytes);
				// Wait completion
				for (int32 TaskIndex = 0; TaskIndex < CompressionTasks.Num(); ++TaskIndex)
				{
					CompressionTasks[TaskIndex].EnsureCompletion();
				}
			}
			else
			{
				IntelBC6HCompressScans(&settings, &Image, &OutCompressedImage, 0, Image.SizeY, iOutputSliceSizeBytes);
			}

			CompressedPixelFormat = PF_BC6H;
			bCompressionSucceeded = true;
		}
		else if ( BuildSettings.TextureFormatName == GTextureFormatNameBC7 )
		{
			check(InImage.NumSlices == 1);
			FImage Image;
			// Expand any dimension less than 4 to 4.  Intel compressor doesn't seem to do this automatically.
			FImage ResizedImage;
			if (ExpandTo4PxMultiple(InImage, ResizedImage))
			{
				ResizedImage.CopyTo(Image, ERawImageFormat::BGRA8, BuildSettings.GetGammaSpace());
			}
			else
			{
				InImage.CopyTo(Image, ERawImageFormat::BGRA8, BuildSettings.GetGammaSpace());
			}

			bc7_enc_settings settings;
			if ( bImageHasAlphaChannel )
			{
				GetProfile_alpha_basic(&settings);
			}
			else
			{
				GetProfile_basic(&settings);
			}

			if ( bUseTasks )
			{
				class FIntelCompressWorker : public FNonAbandonableTask
				{
				public:
					FIntelCompressWorker(bc7_enc_settings* pEncSettings, FImage* pInImage, FCompressedImage2D* pOutImage, int yStart, int yEnd)
						: mpEncSettings(pEncSettings)
						, mpInImage(pInImage)
						, mpOutImage(pOutImage)
						, mYStart(yStart)
						, mYEnd(yEnd)
					{
					}

					void DoWork()
					{
						IntelBC7CompressScans(mpEncSettings, mpInImage, mpOutImage, mYStart, mYEnd);
					}

					FORCEINLINE TStatId GetStatId() const
					{
						RETURN_QUICK_DECLARE_CYCLE_STAT(FIntelCompressWorker, STATGROUP_ThreadPoolAsyncTasks);
					}

					bc7_enc_settings*	mpEncSettings;
					FImage*				mpInImage;
					FCompressedImage2D*	mpOutImage;
					int					mYStart;
					int					mYEnd;
				};
				typedef FAsyncTask<FIntelCompressWorker> FIntelCompressTask;

				// One less task because we'll do the final + non multiple of 4 inside this task
				TIndirectArray<FIntelCompressTask> CompressionTasks;
				CompressionTasks.Reserve(iNumTasks);
				for ( int iTask=0; iTask < iNumTasks; ++iTask )
				{
					auto* AsyncTask = new(CompressionTasks) FIntelCompressTask(&settings, &Image, &OutCompressedImage, iTask * iScansPerTask, (iTask + 1) * iScansPerTask);
					AsyncTask->StartBackgroundTask();
				}

				IntelBC7CompressScans(&settings, &Image, &OutCompressedImage, iScansPerTask * iNumTasks, Image.SizeY);

				// Wait completion
				for (int32 TaskIndex = 0; TaskIndex < CompressionTasks.Num(); ++TaskIndex)
				{
					CompressionTasks[TaskIndex].EnsureCompletion();
				}
			}
			else
			{
				IntelBC7CompressScans(&settings, &Image, &OutCompressedImage, 0, Image.SizeY);
			}

			CompressedPixelFormat = PF_BC7;
			bCompressionSucceeded = true;
		}

		if ( bCompressionSucceeded )
		{
			OutCompressedImage.SizeX = FMath::Max(InImage.SizeX, 4);
			OutCompressedImage.SizeY = FMath::Max(InImage.SizeY, 4);
			OutCompressedImage.PixelFormat = CompressedPixelFormat;
		}
		return bCompressionSucceeded;
	}
};

/**
 * Module for DXT texture compression.
 */
static ITextureFormat* Singleton = NULL;

class FTextureFormatIntelISPCTexCompModule : public ITextureFormatModule
{
public:
	FTextureFormatIntelISPCTexCompModule()
	{
		mDllHandle = nullptr;
	}

	virtual ~FTextureFormatIntelISPCTexCompModule()
	{
		delete Singleton;
		Singleton = NULL;

		if ( mDllHandle != nullptr )
		{
			FPlatformProcess::FreeDllHandle(mDllHandle);
			mDllHandle = nullptr;
		}
	}

	virtual ITextureFormat* GetTextureFormat()
	{
		if (!Singleton)
		{
#if PLATFORM_WINDOWS
	#if PLATFORM_64BITS
			mDllHandle = FPlatformProcess::GetDllHandle(TEXT("../../../Engine/Binaries/ThirdParty/IntelISPCTexComp/Win64-Release/ispc_texcomp.dll"));
	#else	//32-bit platform
			mDllHandle = FPlatformProcess::GetDllHandle(TEXT("../../../Engine/Binaries/ThirdParty/IntelISPCTexComp/Win32-Release/ispc_texcomp.dll"));
	#endif
#endif	//PLATFORM_WINDOWS

			Singleton = new FTextureFormatIntelISPCTexComp();
		}
		return Singleton;
	}

	void* mDllHandle;
};

IMPLEMENT_MODULE(FTextureFormatIntelISPCTexCompModule, TextureFormatIntelISPCTexComp);
