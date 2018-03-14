// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "WebBrowserTextureResource.h"

#include "DeviceProfiles/DeviceProfile.h"
#include "DeviceProfiles/DeviceProfileManager.h"
#include "ExternalTexture.h"
#include "PipelineStateCache.h"
#include "SceneUtils.h"
#include "Shader.h"
#include "StaticBoundShaderState.h"
#include "RenderUtils.h"
#include "RHIStaticStates.h"
#include "ExternalTexture.h"
#include "WebBrowserTexture.h"


#define WebBrowserTextureRESOURCE_TRACE_RENDER 0


/* FWebBrowserTextureResource structors
 *****************************************************************************/

FWebBrowserTextureResource::FWebBrowserTextureResource(UWebBrowserTexture& InOwner, FIntPoint& InOwnerDim, SIZE_T& InOwnerSize)
	: Cleared(false)
	, CurrentClearColor(FLinearColor::Transparent)
	, Owner(InOwner)
	, OwnerDim(InOwnerDim)
	, OwnerSize(InOwnerSize)
{ 
	FPlatformMisc::LowLevelOutputDebugStringf(TEXT("FWebBrowserTextureResource:FWebBrowserTextureResource %d %d"), OwnerDim.X, OwnerDim.Y);

}


/* FWebBrowserTextureResource interface
 *****************************************************************************/

void FWebBrowserTextureResource::Render(const FRenderParams& Params)
{
	check(IsInRenderingThread());

	TSharedPtr<FWebBrowserTextureSampleQueue, ESPMode::ThreadSafe> SampleSource = Params.SampleSource.Pin();

	if (SampleSource.IsValid())
	{
		// get the most current sample to be rendered
		TSharedPtr<FWebBrowserTextureSample, ESPMode::ThreadSafe> Sample;
		bool SampleValid = false;
		
		while (SampleSource->Peek(Sample) && Sample.IsValid())
		{
			SampleValid = SampleSource->Dequeue(Sample);
		}

		if (!SampleValid)
		{
			return; // no sample to render
		}

		check(Sample.IsValid());

		// render the sample
		CopySample(Sample, Params.ClearColor);

		if (!GSupportsImageExternal && Params.PlayerGuid.IsValid())
		{
			FTextureRHIRef VideoTexture = (FTextureRHIRef)Owner.TextureReference.TextureReferenceRHI;
			FExternalTextureRegistry::Get().RegisterExternalTexture(Params.PlayerGuid, VideoTexture, SamplerStateRHI, Sample->GetScaleRotation(), Sample->GetOffset());
		}
	}
	else if (!Cleared)
	{
		ClearTexture(Params.ClearColor);

		if (!GSupportsImageExternal && Params.PlayerGuid.IsValid())
		{
			FTextureRHIRef VideoTexture = (FTextureRHIRef)Owner.TextureReference.TextureReferenceRHI;
			FExternalTextureRegistry::Get().RegisterExternalTexture(Params.PlayerGuid, VideoTexture, SamplerStateRHI, FLinearColor(1.0f, 0.0f, 0.0f, 1.0f), FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
		}
	}
}


/* FRenderTarget interface
 *****************************************************************************/

FIntPoint FWebBrowserTextureResource::GetSizeXY() const
{
	return FIntPoint(Owner.GetWidth(), Owner.GetHeight());
}


/* FTextureResource interface
 *****************************************************************************/

FString FWebBrowserTextureResource::GetFriendlyName() const
{
	return Owner.GetPathName();
}


uint32 FWebBrowserTextureResource::GetSizeX() const
{
	return Owner.GetWidth();
}


uint32 FWebBrowserTextureResource::GetSizeY() const
{
	return Owner.GetHeight();
}


void FWebBrowserTextureResource::InitDynamicRHI()
{
	// create the sampler state
	FSamplerStateInitializerRHI SamplerStateInitializer(
		(ESamplerFilter)UDeviceProfileManager::Get().GetActiveProfile()->GetTextureLODSettings()->GetSamplerFilter(&Owner),
		(Owner.AddressX == TA_Wrap) ? AM_Wrap : ((Owner.AddressX == TA_Clamp) ? AM_Clamp : AM_Mirror),
		(Owner.AddressY == TA_Wrap) ? AM_Wrap : ((Owner.AddressY == TA_Clamp) ? AM_Clamp : AM_Mirror),
		AM_Wrap
	);

	SamplerStateRHI = RHICreateSamplerState(SamplerStateInitializer);
}


void FWebBrowserTextureResource::ReleaseDynamicRHI()
{
	Cleared = false;

	InputTarget.SafeRelease();
	OutputTarget.SafeRelease();
	RenderTargetTextureRHI.SafeRelease();
	TextureRHI.SafeRelease();

	UpdateTextureReference(nullptr);
}


/* FWebBrowserTextureResource implementation
 *****************************************************************************/

void FWebBrowserTextureResource::ClearTexture(const FLinearColor& ClearColor)
{
	FPlatformMisc::LowLevelOutputDebugStringf(TEXT("FWebBrowserTextureResource:ClearTexture"));
	// create output render target if we don't have one yet
	const uint32 OutputCreateFlags = TexCreate_Dynamic | TexCreate_SRGB;

	if ((ClearColor != CurrentClearColor) || !OutputTarget.IsValid() || ((OutputTarget->GetFlags() & OutputCreateFlags) != OutputCreateFlags))
	{
		FRHIResourceCreateInfo CreateInfo = {
			FClearValueBinding(ClearColor)
		};

		TRefCountPtr<FRHITexture2D> DummyTexture2DRHI;

		RHICreateTargetableShaderResource2D(
			2,
			2,
			PF_B8G8R8A8,
			1,
			OutputCreateFlags,
			TexCreate_RenderTargetable,
			false,
			CreateInfo,
			OutputTarget,
			DummyTexture2DRHI
		);

		CurrentClearColor = ClearColor;
		UpdateResourceSize();
	}

	if (RenderTargetTextureRHI != OutputTarget)
	{
		UpdateTextureReference(OutputTarget);
	}

	// draw the clear color
	FRHICommandListImmediate& CommandList = FRHICommandListExecutor::GetImmediateCommandList();
	{
		FRHIRenderTargetView View = FRHIRenderTargetView(RenderTargetTextureRHI, ERenderTargetLoadAction::EClear);
		FRHISetRenderTargetsInfo Info(1, &View, FRHIDepthRenderTargetView());
		CommandList.SetRenderTargetsAndClear(Info);
		CommandList.SetViewport(0, 0, 0.0f, RenderTargetTextureRHI->GetSizeX(), RenderTargetTextureRHI->GetSizeY(), 1.0f);
		CommandList.TransitionResource(EResourceTransitionAccess::EReadable, RenderTargetTextureRHI);
	}

	Cleared = true;
}

void FWebBrowserTextureResource::CopySample(const TSharedPtr<FWebBrowserTextureSample, ESPMode::ThreadSafe>& Sample, const FLinearColor& ClearColor)
{
	FRHITexture* SampleTexture = Sample->GetTexture();
	FRHITexture2D* SampleTexture2D = (SampleTexture != nullptr) ? SampleTexture->GetTexture2D() : nullptr;
	// If the sample already provides a texture resource, we simply use that
	// as the output render target. If the sample only provides raw data, then
	// we create our own output render target and copy the data into it.
	if (SampleTexture2D != nullptr)
	{
	FPlatformMisc::LowLevelOutputDebugStringf(TEXT("FWebBrowserTextureResource:CopySample 1"));
		// use sample's texture as the new render target.
		if (TextureRHI != SampleTexture2D)
		{
			FPlatformMisc::LowLevelOutputDebugStringf(TEXT("FWebBrowserTextureResource:CopySample 11"));
			UpdateTextureReference(SampleTexture2D);

			OutputTarget.SafeRelease();
			UpdateResourceSize();
		}
	}
	else
	{
		FPlatformMisc::LowLevelOutputDebugStringf(TEXT("FWebBrowserTextureResource:CopySample 2"));
		// create a new output render target if necessary
		const uint32 OutputCreateFlags = TexCreate_Dynamic | TexCreate_SRGB;
		const FIntPoint SampleDim = Sample->GetDim();

		if ((ClearColor != CurrentClearColor) || !OutputTarget.IsValid() || (OutputTarget->GetSizeXY() != SampleDim) || ((OutputTarget->GetFlags() & OutputCreateFlags) != OutputCreateFlags))
		{
			FPlatformMisc::LowLevelOutputDebugStringf(TEXT("FWebBrowserTextureResource:CopySample 1"));
			TRefCountPtr<FRHITexture2D> DummyTexture2DRHI;

			FRHIResourceCreateInfo CreateInfo = {
				FClearValueBinding(ClearColor)
			};

			RHICreateTargetableShaderResource2D(
				SampleDim.X,
				SampleDim.Y,
				PF_B8G8R8A8,
				1,
				OutputCreateFlags,
				TexCreate_RenderTargetable,
				false,
				CreateInfo,
				OutputTarget,
				DummyTexture2DRHI
			);

			CurrentClearColor = ClearColor;
			UpdateResourceSize();
		}

		if (RenderTargetTextureRHI != OutputTarget)
		{
			UpdateTextureReference(OutputTarget);
		}

		// copy sample data to output render target
		FUpdateTextureRegion2D Region(0, 0, 0, 0, SampleDim.X, SampleDim.Y);
		RHIUpdateTexture2D(RenderTargetTextureRHI.GetReference(), 0, Region, Sample->GetStride(), (uint8*)Sample->GetBuffer());
	}
	Cleared = false;
}


void FWebBrowserTextureResource::UpdateResourceSize()
{
	FPlatformMisc::LowLevelOutputDebugStringf(TEXT("FWebBrowserTextureResource:UpdateResourceSize"));

	SIZE_T ResourceSize = 0;

	if (InputTarget.IsValid())
	{
		ResourceSize += CalcTextureSize(InputTarget->GetSizeX(), InputTarget->GetSizeY(), InputTarget->GetFormat(), 1);
	}

	if (OutputTarget.IsValid())
	{
		ResourceSize += CalcTextureSize(OutputTarget->GetSizeX(), OutputTarget->GetSizeY(), OutputTarget->GetFormat(), 1);
	}

	OwnerSize = ResourceSize;
}


void FWebBrowserTextureResource::UpdateTextureReference(FRHITexture2D* NewTexture)
{
	FPlatformMisc::LowLevelOutputDebugStringf(TEXT("FWebBrowserTextureResource:UpdateTextureReference"));
	TextureRHI = NewTexture;
	RenderTargetTextureRHI = NewTexture;

	RHIUpdateTextureReference(Owner.TextureReference.TextureReferenceRHI, NewTexture);

	if (RenderTargetTextureRHI != nullptr)
	{
		OwnerDim = FIntPoint(RenderTargetTextureRHI->GetSizeX(), RenderTargetTextureRHI->GetSizeY());
	}
	else
	{
		OwnerDim = FIntPoint::ZeroValue;
	}
}
