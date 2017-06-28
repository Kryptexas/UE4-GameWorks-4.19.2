// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "OculusHMDPrivate.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS
#include "OculusHMD.h"
#include "RenderingThread.h"
#include "DefaultSpectatorScreenController.h"
#include "ClearQuad.h"


namespace OculusHMD
{
	// 
	class FOculusRiftSpectatorScreenController : public FDefaultSpectatorScreenController
	{
	public:
		FOculusRiftSpectatorScreenController(FOculusHMD* InOculusHMDDevice)
			: FDefaultSpectatorScreenController(InOculusHMDDevice)
			, OculusHMDDevice(InOculusHMDDevice)
		{
		}

		void RenderSpectatorScreen_RenderThread(FRHICommandListImmediate& RHICmdList, FRHITexture2D* BackBuffer, FTexture2DRHIRef SrcTexture) const
		{
			FCustomPresent* CustomPresent = OculusHMDDevice->GetCustomPresent_Internal();
			if (!CustomPresent)
			{
				return;
			}

			FDefaultSpectatorScreenController::RenderSpectatorScreen_RenderThread(RHICmdList, BackBuffer, SrcTexture);
		}

		void RenderSpectatorModeUndistorted(FRHICommandListImmediate& RHICmdList, FTexture2DRHIRef TargetTexture, FTexture2DRHIRef EyeTexture, FTexture2DRHIRef OtherTexture)
		{
			FSettings* Settings = OculusHMDDevice->GetSettings_RenderThread();
			check(Settings);
			FIntRect DestRect(0, 0, TargetTexture->GetSizeX() / 2, TargetTexture->GetSizeY());
			for (int i = 0; i < 2; ++i)
			{
				OculusHMDDevice->CopyTexture_RenderThread(RHICmdList, EyeTexture, Settings->EyeRenderViewport[i], TargetTexture, DestRect, false);
				DestRect.Min.X += TargetTexture->GetSizeX() / 2;
				DestRect.Max.X += TargetTexture->GetSizeX() / 2;
			}
		}

		void RenderSpectatorModeDistorted(FRHICommandListImmediate& RHICmdList, FTexture2DRHIRef TargetTexture, FTexture2DRHIRef EyeTexture, FTexture2DRHIRef OtherTexture)
		{
			FCustomPresent* CustomPresent = OculusHMDDevice->GetCustomPresent_Internal();
			check(CustomPresent);
			FTexture2DRHIRef MirrorTexture = CustomPresent->GetMirrorTexture();
			if (MirrorTexture)
			{
				FIntRect SrcRect(0, 0, MirrorTexture->GetSizeX(), MirrorTexture->GetSizeY());
				FIntRect DestRect(0, 0, TargetTexture->GetSizeX(), TargetTexture->GetSizeY());
				OculusHMDDevice->CopyTexture_RenderThread(RHICmdList, MirrorTexture, SrcRect, TargetTexture, DestRect, false);
			}
		}

		void RenderSpectatorModeSingleEye(FRHICommandListImmediate& RHICmdList, FTexture2DRHIRef TargetTexture, FTexture2DRHIRef EyeTexture, FTexture2DRHIRef OtherTexture)
		{
			FSettings* Settings = OculusHMDDevice->GetSettings_RenderThread();
			check(Settings);
			const FIntRect SrcRect= Settings->EyeRenderViewport[0];
			const FIntRect DstRect(0, 0, TargetTexture->GetSizeX(), TargetTexture->GetSizeY());

			OculusHMDDevice->CopyTexture_RenderThread(RHICmdList, EyeTexture, SrcRect, TargetTexture, DstRect, false);
		}

	private:
		FOculusHMD* OculusHMDDevice;
	};

	void FOculusHMD::CreateSpectatorScreenController()
	{
		SpectatorScreenController = MakeUnique<FOculusRiftSpectatorScreenController>(this);
	}

	bool FOculusHMD::ShouldDisableHiddenAndVisibileAreaMeshForSpectatorScreen_RenderThread() const
	{
		check(IsInRenderingThread());
		return false;

		// If you really need the eye corners to look nice, and can't just crop more, 
		// and are willing to suffer a frametime hit... you could do this:
		//if (!SpectatorScreenController)
		//{
		//	return false;
		//}
		//const ESpectatorScreenMode Mode = SpectatorScreenController->GetSpectatorScreenMode();
		//return Mode == ESpectatorScreenMode::SingleEyeLetterboxed ||
		//	Mode == ESpectatorScreenMode::SingleEyeCroppedToFill ||
		//	Mode == ESpectatorScreenMode::TexturePlusEye;
	}

	ESpectatorScreenMode FOculusHMD::GetSpectatorScreenMode() const
	{
		return SpectatorScreenController ? SpectatorScreenController->GetSpectatorScreenMode() : ESpectatorScreenMode::Disabled;
	}

	FIntRect FOculusHMD::GetFullFlatEyeRect(FTexture2DRHIRef EyeTexture) const
	{
		check(IsInRenderingThread());
		// Rift does this differently than other platforms, it already has an idea of what rectangle it wants to use stored.
		FIntRect& EyeRect = Settings_RenderThread->EyeRenderViewport[0];

		// But the rectangle rift specifies has corners cut off, so we will crop a little more.
		static FVector2D SrcNormRectMin(0.05f, 0.0f);
		static FVector2D SrcNormRectMax(0.95f, 1.0f);
		const int32 SizeX = EyeRect.Max.X - EyeRect.Min.X;
		const int32 SizeY = EyeRect.Max.Y - EyeRect.Min.Y;
		return FIntRect(EyeRect.Min.X + SizeX * SrcNormRectMin.X, EyeRect.Min.Y + SizeY * SrcNormRectMin.Y, EyeRect.Min.X + SizeX * SrcNormRectMax.X, EyeRect.Min.Y + SizeY * SrcNormRectMax.Y);
	}

	void FOculusHMD::CopyTexture_RenderThread(FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef SrcTexture, FIntRect SrcRect, FTexture2DRHIParamRef DstTexture, FIntRect DstRect, bool bClearBlack) const
	{
		if (bClearBlack)
		{
			SetRenderTarget(RHICmdList, DstTexture, FTextureRHIRef());
			const FIntRect ClearRect(0, 0, DstTexture->GetSizeX(), DstTexture->GetSizeY());
			RHICmdList.SetViewport(ClearRect.Min.X, ClearRect.Min.Y, 0, ClearRect.Max.X, ClearRect.Max.Y, 1.0f);
			DrawClearQuad(RHICmdList, FLinearColor::Black);
		}

		check(CustomPresent);
		CustomPresent->CopyTexture_RenderThread(RHICmdList, DstTexture, SrcTexture, DstRect, SrcRect);
	}
} // namespace OculusHMD

#endif // OCULUS_HMD_SUPPORTED_PLATFORMS