// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	CubemapUnwapUtils.cpp: Pixel and Vertex shader to render a cube map as 2D texture
=============================================================================*/

#include "UnrealEd.h"
#include "CubemapUnwrapUtils.h" 

IMPLEMENT_SHADER_TYPE(,FCubemapTexturePropertiesVS,TEXT("SimpleElementVertexShader"),TEXT("Main"),SF_Vertex);
IMPLEMENT_SHADER_TYPE(template<>,FCubemapTexturePropertiesPS<false>,TEXT("SimpleElementPixelShader"),TEXT("CubemapTextureProperties"),SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>,FCubemapTexturePropertiesPS<true>,TEXT("SimpleElementPixelShader"),TEXT("CubemapTextureProperties"),SF_Pixel);
IMPLEMENT_SHADER_TYPE(,FIESLightProfilePS,TEXT("SimpleElementPixelShader"),TEXT("IESLightProfileMain"),SF_Pixel);

namespace CubemapHelpers
{
	/**
	* Helper function to create an unwrapped 2D image of the cube map ( longitude/latitude )
	* This version takes explicitly passed properties of the source object, as the sources have different APIs.
	* @param	TextureResource		Source FTextureResource object.
	* @param	AxisDimenion		axis length of the cube.
	* @param	SourcePixelFormat	pixel format of the source.
	* @param	BitsOUT				Raw bits of the 2D image bitmap.
	* @param	SizeXOUT			Filled with the X dimension of the output bitmap.
	* @param	SizeYOUT			Filled with the Y dimension of the output bitmap.
	* @return						true on success.
	* @param	FormatOUT			Filled with the pixel format of the output bitmap.
	*/
	bool GenerateLongLatUnwrap(const FTextureResource* TextureResource, const uint32 AxisDimenion, const EPixelFormat SourcePixelFormat, TArray<uint8>& BitsOUT, FIntPoint& SizeOUT, EPixelFormat& FormatOUT)
	{
		TRefCountPtr<FBatchedElementParameters> BatchedElementParameters;
		BatchedElementParameters = new FMipLevelBatchedElementParameters((float)0, true);
		const FIntPoint LongLatDimensions(AxisDimenion * 2, AxisDimenion);

		// If the source format is 8 bit per channel or less then select a LDR target format.
		const EPixelFormat TargetPixelFormat = CalculateImageBytes(1, 1, 0, SourcePixelFormat) <= 4 ? PF_B8G8R8A8 : PF_FloatRGBA;

		UTextureRenderTarget2D* RenderTargetLongLat = new UTextureRenderTarget2D(FPostConstructInitializeProperties());
		check(RenderTargetLongLat);
		RenderTargetLongLat->AddToRoot();
		RenderTargetLongLat->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
		RenderTargetLongLat->InitCustomFormat(LongLatDimensions.X, LongLatDimensions.Y, TargetPixelFormat, false);
		RenderTargetLongLat->TargetGamma = 0;
		FRenderTarget* RenderTarget = RenderTargetLongLat->GameThread_GetRenderTargetResource();

		FCanvas* Canvas = new FCanvas(RenderTarget, NULL, 0, 0, 0);
		Canvas->SetRenderTarget(RenderTarget);

		ENQUEUE_UNIQUE_RENDER_COMMAND(
			LongLatUnwrapBeginSceneCommand,
		{
			RHIBeginScene();
		});

		// Clear the render target to black
		Canvas->Clear(FLinearColor(0, 0, 0, 0));

		FCanvasTileItem TileItem(FVector2D(0.0f, 0.0f), TextureResource, FVector2D(LongLatDimensions.X, LongLatDimensions.Y), FLinearColor::White);
		TileItem.BatchedElementParameters = BatchedElementParameters;
		TileItem.BlendMode = SE_BLEND_Opaque;
		Canvas->DrawItem(TileItem);

		Canvas->Flush();
		FlushRenderingCommands();
		Canvas->SetRenderTarget(NULL);
		FlushRenderingCommands();
		ENQUEUE_UNIQUE_RENDER_COMMAND(
			LongLatUnwrapEndSceneCommand,
		{
			RHIEndScene();
		});
		
		int32 ImageBytes = CalculateImageBytes(LongLatDimensions.X, LongLatDimensions.Y, 0, TargetPixelFormat);

		BitsOUT.AddUninitialized(ImageBytes);

		bool bReadSuccess = false;
		switch (TargetPixelFormat)
		{
			case PF_B8G8R8A8:
				bReadSuccess = RenderTarget->ReadPixelsPtr((FColor*)BitsOUT.GetTypedData());
			break;
			case PF_FloatRGBA:
				{
					TArray<FFloat16Color> FloatColors;
					bReadSuccess = RenderTarget->ReadFloat16Pixels(FloatColors);
					FMemory::Memcpy(BitsOUT.GetTypedData(), FloatColors.GetTypedData(), ImageBytes);
				}
			break;
		}
		// Clean up.
		RenderTargetLongLat->RemoveFromRoot();
		RenderTargetLongLat = NULL;
		delete Canvas;

		SizeOUT = LongLatDimensions;
		FormatOUT = TargetPixelFormat;
		if (bReadSuccess == false)
		{
			// Reading has failed clear output buffer.
			BitsOUT.Empty();
		}

		return bReadSuccess;
	}

	bool GenerateLongLatUnwrap(const UTextureCube* CubeTexture, TArray<uint8>& BitsOUT, FIntPoint& SizeOUT, EPixelFormat& FormatOUT)
	{
		check(CubeTexture != NULL);
		return GenerateLongLatUnwrap(CubeTexture->Resource, CubeTexture->GetSizeX(), CubeTexture->GetPixelFormat(), BitsOUT, SizeOUT, FormatOUT);
	}

	bool GenerateLongLatUnwrap(const UTextureRenderTargetCube* CubeTarget, TArray<uint8>& BitsOUT, FIntPoint& SizeOUT, EPixelFormat& FormatOUT)
	{
		check(CubeTarget != NULL);
		return GenerateLongLatUnwrap(CubeTarget->Resource, CubeTarget->SizeX, CubeTarget->GetFormat(), BitsOUT, SizeOUT, FormatOUT);
	}
}
