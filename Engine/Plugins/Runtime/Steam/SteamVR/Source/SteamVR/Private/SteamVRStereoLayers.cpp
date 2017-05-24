// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
//
#include "CoreMinimal.h"
#include "SteamVRPrivate.h"
#include "StereoLayerManager.h"
#include "SteamVRHMD.h"
#include "Misc/ScopeLock.h"

#if STEAMVR_SUPPORTED_PLATFORMS

// enable to ensure all openvr calls succeed
#if 0
static vr::EVROverlayError sOvrError;
#define OVR_VERIFY(x) sOvrError = x; ensure(sOvrError == vr::VROverlayError_None)
#else
#define OVR_VERIFY(x) x
#endif

/*=============================================================================
*
* Helper functions
*
*/

//=============================================================================
static void TransformToSteamSpace(const FTransform& In, vr::HmdMatrix34_t& Out, float WorldToMeterScale)
{
	const FRotator InRot = In.Rotator();
	FRotator OutRot(InRot.Yaw, -InRot.Roll, -InRot.Pitch);

	const FVector InPos = In.GetTranslation();
	FVector OutPos(InPos.Y, InPos.Z, -InPos.X);
	OutPos /= WorldToMeterScale;

	const FVector InScale = In.GetScale3D();
	FVector OutScale(InScale.Y, InScale.Z, InScale.X);

	Out = FSteamVRHMD::ToHmdMatrix34(FTransform(OutRot, OutPos, OutScale).ToMatrixWithScale());
}


/*=============================================================================
*
* FSteamVRHMD's IStereoLayer implementation via TStereLayerManager<>
*
*/

//=============================================================================
void SetLayerDescMember(FSteamVRLayer& Layer, const IStereoLayers::FLayerDesc& InLayerDesc)
{
	if (InLayerDesc.Texture != Layer.LayerDesc.Texture)
	{
		Layer.bUpdateTexture = true;
	}
	Layer.LayerDesc = InLayerDesc;
}

//=============================================================================
bool GetLayerDescMember(const FSteamVRLayer& Layer, IStereoLayers::FLayerDesc& OutLayerDesc)
{
	OutLayerDesc = Layer.LayerDesc;
	return true;
}

//=============================================================================
void MarkLayerTextureForUpdate(FSteamVRLayer& Layer)
{
	Layer.bUpdateTexture = true;
}

//=============================================================================
void FSteamVRHMD::UpdateSplashScreen()
{
	FTexture2DRHIRef Texture = (bSplashShowMovie && SplashMovie.IsValid()) ? SplashMovie : SplashTexture;
	if (bSplashIsShown && Texture.IsValid())
	{
		FLayerDesc LayerDesc;
		LayerDesc.Flags = ELayerFlags::LAYER_FLAG_TEX_NO_ALPHA_CHANNEL;
		LayerDesc.PositionType = ELayerType::TrackerLocked;
		LayerDesc.Texture = Texture;
		LayerDesc.UVRect = FBox2D(SplashOffset, SplashOffset + SplashScale);
		
		FTransform Translation(FVector(500.0f, 0.0f, 100.0f));
		FRotator Rotation(LastHmdOrientation);
		Rotation.Pitch = 0.0f;
		Rotation.Roll = 0.0f;
		LayerDesc.Transform = Translation * FTransform(Rotation.Quaternion());

		LayerDesc.QuadSize = FVector2D(800.0f, 450.0f);

		if (SplashLayerHandle)
		{
			SetLayerDesc(SplashLayerHandle, LayerDesc);
		}
		else
		{
			SplashLayerHandle = CreateLayer(LayerDesc);
		}
	}
	else
	{
		if (SplashLayerHandle)
		{
			DestroyLayer(SplashLayerHandle);
			SplashLayerHandle = 0;
		}
	}
}

//=============================================================================
void FSteamVRHMD::UpdateLayer(struct FSteamVRLayer& Layer, uint32 LayerId, bool bIsValid) const
{
	if (bIsValid && Layer.OverlayHandle == vr::k_ulOverlayHandleInvalid)
	{
		FString OverlayName = FString::Printf(TEXT("StereoLayer:%u"), LayerId);
		const char* OverlayNameAnsiStr = TCHAR_TO_ANSI(*OverlayName);
		OVR_VERIFY(VROverlay->CreateOverlay(OverlayNameAnsiStr, OverlayNameAnsiStr, &Layer.OverlayHandle));
		OVR_VERIFY(VROverlay->HideOverlay(Layer.OverlayHandle));
		Layer.bUpdateTexture = true;
	}
	else if (!bIsValid && Layer.OverlayHandle != vr::k_ulOverlayHandleInvalid)
	{
		OVR_VERIFY(VROverlay->DestroyOverlay(Layer.OverlayHandle));
		Layer.OverlayHandle = vr::k_ulOverlayHandleInvalid;
	}

	if (Layer.OverlayHandle == vr::k_ulOverlayHandleInvalid)
	{
		return;
	}

	// UVs
	vr::VRTextureBounds_t TextureBounds;
    TextureBounds.uMin = Layer.LayerDesc.UVRect.Min.X;
    TextureBounds.uMax = Layer.LayerDesc.UVRect.Max.X;
    TextureBounds.vMin = Layer.LayerDesc.UVRect.Min.Y;
    TextureBounds.vMax = Layer.LayerDesc.UVRect.Max.Y;
	OVR_VERIFY(VROverlay->SetOverlayTextureBounds(Layer.OverlayHandle, &TextureBounds));
	const float WorldToMeterScale = FMath::Max(GetWorldToMetersScale(), 0.1f);
	OVR_VERIFY(VROverlay->SetOverlayWidthInMeters(Layer.OverlayHandle, Layer.LayerDesc.QuadSize.X / WorldToMeterScale));
	OVR_VERIFY(VROverlay->SetOverlayTexelAspect(Layer.OverlayHandle, Layer.LayerDesc.QuadSize.X / Layer.LayerDesc.QuadSize.Y));

	// Shift layer priority values up by -INT32_MIN, as SteamVR uses unsigned integers for the layer order where UE uses signed integers.
	// This will preserve the correct order between layers with negative and positive priorities
	OVR_VERIFY(VROverlay->SetOverlaySortOrder(Layer.OverlayHandle, Layer.LayerDesc.Priority-INT32_MIN)); 

	// Transform
	switch (Layer.LayerDesc.PositionType)
	{
	case ELayerType::WorldLocked:
#if 0
	{
		// needs final implementation

		FQuat PlayerOrientation = BaseOrientation.Inverse() * PlayerOrientation;
		FTransform PlayerTorso(PlayerOrientation, PlayerPosition);

		FTransform Transform = PlayerTorso.Inverse() * Layer.LayerDesc.Transform;

		vr::HmdMatrix34_t HmdTransform;
		TransformToSteamSpace(Transform, HmdTransform, WorldToMeterScale);
		OVR_VERIFY(VROverlay->SetOverlayTransformTrackedDeviceRelative(Layer.OverlayHandle, VRCompositor->GetTrackingSpace(), &HmdTransform));
		break;
	}
#endif
	case ELayerType::TrackerLocked:
	{
		vr::HmdMatrix34_t HmdTransform;
		TransformToSteamSpace(Layer.LayerDesc.Transform, HmdTransform, WorldToMeterScale);
		OVR_VERIFY(VROverlay->SetOverlayTransformAbsolute(Layer.OverlayHandle, VRCompositor->GetTrackingSpace(), &HmdTransform));
		break;
	}
	case ELayerType::FaceLocked:
	default:
	{
		vr::HmdMatrix34_t HmdTransform;
		TransformToSteamSpace(Layer.LayerDesc.Transform, HmdTransform, WorldToMeterScale);
		OVR_VERIFY(VROverlay->SetOverlayTransformTrackedDeviceRelative(Layer.OverlayHandle, 0, &HmdTransform));
	}
	};
}


//=============================================================================
void FSteamVRHMD::UpdateLayerTextures()
{
	// If there have been no layer changes since last frame we can return
	// Additionally, if we don't have a valid tracking position, the calls to ShowOverlay/SetOverlayTexture below will not have any effect.
	if (!GetStereoLayersDirty() || !HasValidTrackingPosition())
	{
		return;
	}

	ForEachLayer([=](uint32 /* unused */, FSteamVRLayer& Layer)
	{
		if (Layer.OverlayHandle != vr::k_ulOverlayHandleInvalid)
		{

			if (Layer.bUpdateTexture || (Layer.LayerDesc.Flags & LAYER_FLAG_TEX_CONTINUOUS_UPDATE))
			{
				vr::Texture_t Texture;
				Texture.handle = Layer.LayerDesc.Texture->GetNativeResource();
#if PLATFORM_LINUX
#if STEAMVR_USE_VULKAN_RHI
				Texture.eType = vr::TextureType_Vulkan;
#else
				Texture.eType = vr::TextureType_OpenGL;
#endif
#else
				Texture.eType = vr::TextureType_DirectX;
#endif
				Texture.eColorSpace = vr::ColorSpace_Auto;
				OVR_VERIFY(VROverlay->SetOverlayTexture(Layer.OverlayHandle, &Texture));
				OVR_VERIFY(VROverlay->ShowOverlay(Layer.OverlayHandle));

				Layer.bUpdateTexture = false;
			}
		}
	});
}

//=============================================================================
IStereoLayers* FSteamVRHMD::GetStereoLayers ()
{
	check(VROverlay);
	return this;
}

#endif //STEAMVR_SUPPORTED_PLATFORMS
