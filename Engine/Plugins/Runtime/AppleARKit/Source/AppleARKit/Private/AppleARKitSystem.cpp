// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "AppleARKitSystem.h"
#include "DefaultXRCamera.h"
#include "AppleARKitSessionDelegate.h"
#include "ScopeLock.h"
#include "AppleARKitModule.h"
#include "AppleARKitTransform.h"
#include "AppleARKitVideoOverlay.h"
#include "AppleARKitFrame.h"
#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000
#include "IOSAppDelegate.h"
#endif
#include "AppleARKitAnchor.h"
#include "AppleARKitPlaneAnchor.h"
#include "AppleARKitConfiguration.h"
#include "GeneralProjectSettings.h"
#include "IOSRuntimeSettings.h"
#include "AppleARKitFaceMeshConversion.h"
#include "ARSessionConfig.h"
#include "AppleARKitSettings.h"
#include "ARTrackable.h"
#include "ARLightEstimate.h"
#include "ARTraceResult.h"
#include "ARPin.h"

// For orientation changed
#include "Misc/CoreDelegates.h"

namespace ARKitUtil
{

static UARPin* PinFromComponent( const USceneComponent* Component, const TArray<UARPin*>& InPins )
{
	for (UARPin* Pin : InPins)
	{
		if (Pin->GetPinnedComponent() == Component)
		{
			return Pin;
		}
	}
	
	return nullptr;
}


static TArray<UARPin*> PinsFromGeometry( const UARTrackedGeometry* Geometry, const TArray<UARPin*>& InPins )
{
	TArray<UARPin*> OutPins;
	for (UARPin* Pin : InPins)
	{
		if (Pin->GetTrackedGeometry() == Geometry)
		{
			OutPins.Add(Pin);
		}
	}
	
	return OutPins;
}

	
}


//
//  FAppleARKitXRCamera
//

class FAppleARKitXRCamera : public FDefaultXRCamera
{
public:
	FAppleARKitXRCamera(const FAutoRegister& AutoRegister, FAppleARKitSystem& InTrackingSystem, int32 InDeviceId)
	: FDefaultXRCamera( AutoRegister, &InTrackingSystem, InDeviceId )
	, ARKitSystem( InTrackingSystem )
	{}
	
private:
	//~ FDefaultXRCamera
	void OverrideFOV(float& InOutFOV)
	{
		// @todo arkit : is it safe not to lock here? Theoretically this should only be called on the game thread.
		ensure(IsInGameThread());
		const bool bShouldOverrideFOV = ARKitSystem.GetSessionConfig().ShouldRenderCameraOverlay();
		if (bShouldOverrideFOV && ARKitSystem.GameThreadFrame.IsValid())
		{
			if (ARKitSystem.DeviceOrientation == EScreenOrientation::Portrait || ARKitSystem.DeviceOrientation == EScreenOrientation::PortraitUpsideDown)
			{
				// Portrait
				InOutFOV = ARKitSystem.GameThreadFrame->Camera.GetVerticalFieldOfViewForScreen(EAppleARKitBackgroundFitMode::Fill);
			}
			else
			{
				// Landscape
				InOutFOV = ARKitSystem.GameThreadFrame->Camera.GetHorizontalFieldOfViewForScreen(EAppleARKitBackgroundFitMode::Fill);
			}
		}
	}
	
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override
	{
		FDefaultXRCamera::SetupView(InViewFamily, InView);
	}
	
	virtual void SetupViewProjectionMatrix(FSceneViewProjectionData& InOutProjectionData) override
	{
		FDefaultXRCamera::SetupViewProjectionMatrix(InOutProjectionData);
	}
	
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override
	{
		FDefaultXRCamera::BeginRenderViewFamily(InViewFamily);
	}
	
	virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override
	{
		FDefaultXRCamera::PreRenderView_RenderThread(RHICmdList, InView);
	}
	
	virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override
	{
		// Grab the latest frame from ARKit
		{
			FScopeLock ScopeLock(&ARKitSystem.FrameLock);
			ARKitSystem.RenderThreadFrame = ARKitSystem.LastReceivedFrame;
		}
		
		// @todo arkit: Camera late update? 
		
		if (ARKitSystem.RenderThreadFrame.IsValid())
		{
			VideoOverlay.UpdateVideoTexture_RenderThread(RHICmdList, *ARKitSystem.RenderThreadFrame, InViewFamily);
		}
		
		FDefaultXRCamera::PreRenderViewFamily_RenderThread(RHICmdList, InViewFamily);
	}
	
	virtual void PostRenderBasePass_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override
	{
		VideoOverlay.RenderVideoOverlay_RenderThread(RHICmdList, InView, ARKitSystem.DeviceOrientation);
	}
	
	virtual bool IsActiveThisFrame(class FViewport* InViewport) const override
	{
		// Base implementation needs this call as it updates bCurrentFrameIsStereoRendering as a side effect.
		// We'll ignore the result however.
		FDefaultXRCamera::IsActiveThisFrame(InViewport);

		// Check to see if they have disabled the automatic rendering or not
		// Most Face AR apps that are driving other meshes using the face capture (animoji style) will disable this.
		const bool bRenderOverlay =
			ARKitSystem.OnGetARSessionStatus().Status == EARSessionStatus::Running &&
			ARKitSystem.GetSessionConfig().ShouldRenderCameraOverlay();

#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000
		if ([IOSAppDelegate GetDelegate].OSVersion >= 11.0f)
		{
			return bRenderOverlay;
		}
		else
		{
			return false;
		}
#else
		return false;
#endif //ARKIT_SUPPORT
	}
	//~ FDefaultXRCamera
	
private:
	FAppleARKitSystem& ARKitSystem;
	FAppleARKitVideoOverlay VideoOverlay;
};



#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000
static FORCEINLINE FGuid ToFGuid( uuid_t UUID )
{
	FGuid AsGUID(
		*(uint32*)UUID,
		*((uint32*)UUID)+1,
		*((uint32*)UUID)+2,
		*((uint32*)UUID)+3);
	return AsGUID;
}

static FORCEINLINE FGuid ToFGuid( NSUUID* Identifier )
{
	// Get bytes
	uuid_t UUID;
	[Identifier getUUIDBytes:UUID];
	
	// Set FGuid parts
	return ToFGuid( UUID );
}

FARBlendShapeMap ToBlendShapeMap(NSDictionary<ARBlendShapeLocation,NSNumber *>* BlendShapes)
{
	FARBlendShapeMap BlendShapeMap;

#define SET_BLEND_SHAPE(AppleShape, UE4Shape) \
	if (BlendShapes[AppleShape]) \
	{ \
		BlendShapeMap.Add(UE4Shape, [BlendShapes[AppleShape] floatValue]); \
	}

	// Do we want to capture face performance or look at the face as if in a mirror
	if (GetDefault<UAppleARKitSettings>()->DefaultFaceTrackingDirection == EARFaceTrackingDirection::FaceRelative)
	{
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeBlinkLeft, EARFaceBlendShape::EyeBlinkLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeLookDownLeft, EARFaceBlendShape::EyeLookDownLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeLookInLeft, EARFaceBlendShape::EyeLookInLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeLookOutLeft, EARFaceBlendShape::EyeLookOutLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeLookUpLeft, EARFaceBlendShape::EyeLookUpLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeSquintLeft, EARFaceBlendShape::EyeSquintLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeWideLeft, EARFaceBlendShape::EyeWideLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeBlinkRight, EARFaceBlendShape::EyeBlinkRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeLookDownRight, EARFaceBlendShape::EyeLookDownRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeLookInRight, EARFaceBlendShape::EyeLookInRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeLookOutRight, EARFaceBlendShape::EyeLookOutRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeLookUpRight, EARFaceBlendShape::EyeLookUpRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeSquintRight, EARFaceBlendShape::EyeSquintRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeWideRight, EARFaceBlendShape::EyeWideRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationJawForward, EARFaceBlendShape::JawForward);
		SET_BLEND_SHAPE(ARBlendShapeLocationJawLeft, EARFaceBlendShape::JawLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationJawRight, EARFaceBlendShape::JawRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthLeft, EARFaceBlendShape::MouthLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthRight, EARFaceBlendShape::MouthRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthSmileLeft, EARFaceBlendShape::MouthSmileLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthSmileRight, EARFaceBlendShape::MouthSmileRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthFrownLeft, EARFaceBlendShape::MouthFrownLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthFrownRight, EARFaceBlendShape::MouthFrownRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthDimpleLeft, EARFaceBlendShape::MouthDimpleLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthDimpleRight, EARFaceBlendShape::MouthDimpleRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthStretchLeft, EARFaceBlendShape::MouthStretchLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthStretchRight, EARFaceBlendShape::MouthStretchRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthPressLeft, EARFaceBlendShape::MouthPressLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthPressRight, EARFaceBlendShape::MouthPressRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthLowerDownLeft, EARFaceBlendShape::MouthLowerDownLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthLowerDownRight, EARFaceBlendShape::MouthLowerDownRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthUpperUpLeft, EARFaceBlendShape::MouthUpperUpLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthUpperUpRight, EARFaceBlendShape::MouthUpperUpRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationBrowDownLeft, EARFaceBlendShape::BrowDownLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationBrowDownRight, EARFaceBlendShape::BrowDownRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationBrowOuterUpLeft, EARFaceBlendShape::BrowOuterUpLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationBrowOuterUpRight, EARFaceBlendShape::BrowOuterUpRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationCheekSquintLeft, EARFaceBlendShape::CheekSquintLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationCheekSquintRight, EARFaceBlendShape::CheekSquintRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationNoseSneerLeft, EARFaceBlendShape::NoseSneerLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationNoseSneerRight, EARFaceBlendShape::NoseSneerRight);
	}
	else
	{
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeBlinkLeft, EARFaceBlendShape::EyeBlinkRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeLookDownLeft, EARFaceBlendShape::EyeLookDownRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeLookInLeft, EARFaceBlendShape::EyeLookInRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeLookOutLeft, EARFaceBlendShape::EyeLookOutRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeLookUpLeft, EARFaceBlendShape::EyeLookUpRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeSquintLeft, EARFaceBlendShape::EyeSquintRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeWideLeft, EARFaceBlendShape::EyeWideRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeBlinkRight, EARFaceBlendShape::EyeBlinkLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeLookDownRight, EARFaceBlendShape::EyeLookDownLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeLookInRight, EARFaceBlendShape::EyeLookInLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeLookOutRight, EARFaceBlendShape::EyeLookOutLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeLookUpRight, EARFaceBlendShape::EyeLookUpLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeSquintRight, EARFaceBlendShape::EyeSquintLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationEyeWideRight, EARFaceBlendShape::EyeWideLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationJawForward, EARFaceBlendShape::JawForward);
		SET_BLEND_SHAPE(ARBlendShapeLocationJawLeft, EARFaceBlendShape::JawRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationJawRight, EARFaceBlendShape::JawLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthLeft, EARFaceBlendShape::MouthRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthRight, EARFaceBlendShape::MouthLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthSmileLeft, EARFaceBlendShape::MouthSmileRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthSmileRight, EARFaceBlendShape::MouthSmileLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthFrownLeft, EARFaceBlendShape::MouthFrownRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthFrownRight, EARFaceBlendShape::MouthFrownLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthDimpleLeft, EARFaceBlendShape::MouthDimpleRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthDimpleRight, EARFaceBlendShape::MouthDimpleLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthStretchLeft, EARFaceBlendShape::MouthStretchRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthStretchRight, EARFaceBlendShape::MouthStretchLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthPressLeft, EARFaceBlendShape::MouthPressRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthPressRight, EARFaceBlendShape::MouthPressLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthLowerDownLeft, EARFaceBlendShape::MouthLowerDownRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthLowerDownRight, EARFaceBlendShape::MouthLowerDownLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthUpperUpLeft, EARFaceBlendShape::MouthUpperUpRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationMouthUpperUpRight, EARFaceBlendShape::MouthUpperUpLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationBrowDownLeft, EARFaceBlendShape::BrowDownRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationBrowDownRight, EARFaceBlendShape::BrowDownLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationBrowOuterUpLeft, EARFaceBlendShape::BrowOuterUpRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationBrowOuterUpRight, EARFaceBlendShape::BrowOuterUpLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationCheekSquintLeft, EARFaceBlendShape::CheekSquintRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationCheekSquintRight, EARFaceBlendShape::CheekSquintLeft);
		SET_BLEND_SHAPE(ARBlendShapeLocationNoseSneerLeft, EARFaceBlendShape::NoseSneerRight);
		SET_BLEND_SHAPE(ARBlendShapeLocationNoseSneerRight, EARFaceBlendShape::NoseSneerLeft);
	}

	// These are the same mirrored or not
	SET_BLEND_SHAPE(ARBlendShapeLocationJawOpen, EARFaceBlendShape::JawOpen);
	SET_BLEND_SHAPE(ARBlendShapeLocationMouthClose, EARFaceBlendShape::MouthClose);
	SET_BLEND_SHAPE(ARBlendShapeLocationMouthFunnel, EARFaceBlendShape::MouthFunnel);
	SET_BLEND_SHAPE(ARBlendShapeLocationMouthPucker, EARFaceBlendShape::MouthPucker);
	SET_BLEND_SHAPE(ARBlendShapeLocationMouthRollLower, EARFaceBlendShape::MouthRollLower);
	SET_BLEND_SHAPE(ARBlendShapeLocationMouthRollUpper, EARFaceBlendShape::MouthRollUpper);
	SET_BLEND_SHAPE(ARBlendShapeLocationMouthShrugLower, EARFaceBlendShape::MouthShrugLower);
	SET_BLEND_SHAPE(ARBlendShapeLocationMouthShrugUpper, EARFaceBlendShape::MouthShrugUpper);
	SET_BLEND_SHAPE(ARBlendShapeLocationBrowInnerUp, EARFaceBlendShape::BrowInnerUp);
	SET_BLEND_SHAPE(ARBlendShapeLocationCheekPuff, EARFaceBlendShape::CheekPuff);


#undef SET_BLEND_SHAPE

	return BlendShapeMap;
}

enum class EAppleAnchorType : uint8
{
	Anchor,
	PlaneAnchor,
	FaceAnchor,
	MAX
};

struct FAppleARKitAnchorData
{
	FAppleARKitAnchorData(FGuid InAnchorGuid, FTransform InTransform)
	: Transform( InTransform )
	, AnchorType( EAppleAnchorType::Anchor )
	, AnchorGUID( InAnchorGuid )
	{
	}

	FAppleARKitAnchorData(FGuid InAnchorGuid, FTransform InTransform, FVector InCenter, FVector InExtent)
	: Transform( InTransform )
	, AnchorType( EAppleAnchorType::PlaneAnchor )
	, AnchorGUID( InAnchorGuid )
	, Center(InCenter)
	, Extent(InExtent)
	{
	}

	FAppleARKitAnchorData(FGuid InAnchorGuid, FTransform InTransform, FARBlendShapeMap InBlendShapes, TArray<FVector> InFaceVerts)
	: Transform( InTransform )
	, AnchorType( EAppleAnchorType::FaceAnchor )
	, AnchorGUID( InAnchorGuid )
	, BlendShapes( MoveTemp(InBlendShapes) )
	, FaceVerts( MoveTemp(InFaceVerts) )
	{
	}

	FTransform Transform;
	EAppleAnchorType AnchorType;
	FGuid AnchorGUID;
	FVector Center;
	FVector Extent;
	FARBlendShapeMap BlendShapes;
	TArray<FVector> FaceVerts;
	// Note: the index buffer never changes so can be safely read once
	static TArray<int32> FaceIndices;
};

TArray<int32> FAppleARKitAnchorData::FaceIndices;

#endif//ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000






//
//  FAppleARKitSystem
//

FAppleARKitSystem::FAppleARKitSystem()
: FARSystemBase()
, DeviceOrientation(EScreenOrientation::Unknown)
, DerivedTrackingToUnrealRotation(FRotator::ZeroRotator)
, LightEstimate(nullptr)
, GameThreadFrameNumber(0)
, GameThreadTimestamp(0.0)
, LastTrackedGeometry_DebugId(0)
{
	// See Initialize(), as we need access to SharedThis()

	// Create our LiveLink provider if the project setting is enabled
	if (GetDefault<UAppleARKitSettings>()->bEnableLiveLinkForFaceTracking)
	{
		FaceTrackingLiveLinkSubjectName = GetDefault<UAppleARKitSettings>()->DefaultFaceTrackingLiveLinkSubjectName;
#if PLATFORM_IOS
		LiveLinkSource = FAppleARKitLiveLinkSourceFactory::CreateLiveLinkSource(true);
#else
		// This should be started already, but just in case
		FAppleARKitLiveLinkSourceFactory::CreateLiveLinkRemoteListener();
#endif
	}
}

FAppleARKitSystem::~FAppleARKitSystem()
{
	// Unregister our ability to hit-test in AR with Unreal
}


FName FAppleARKitSystem::GetSystemName() const
{
	static const FName AppleARKitSystemName(TEXT("AppleARKit"));
	return AppleARKitSystemName;
}


bool FAppleARKitSystem::GetCurrentPose(int32 DeviceId, FQuat& OutOrientation, FVector& OutPosition)
{
	if (DeviceId == IXRTrackingSystem::HMDDeviceId && GameThreadFrame.IsValid())
	{
		// Do not have to lock here, because we are on the game
		// thread and GameThreadFrame is only written to from the game thread.
		
		
		// Apply alignment transform if there is one.
		FTransform CurrentTransform(GameThreadFrame->Camera.Orientation, GameThreadFrame->Camera.Translation);
		CurrentTransform = FTransform(DerivedTrackingToUnrealRotation) * CurrentTransform;
		CurrentTransform *= GetAlignmentTransform();
		
		
		// Apply counter-rotation to compensate for mobile device orientation
		OutOrientation = CurrentTransform.GetRotation();
		OutPosition = CurrentTransform.GetLocation();
		
		return true;
	}
	else
	{
		return false;
	}
}

FString FAppleARKitSystem::GetVersionString() const
{
	return TEXT("AppleARKit - V1.0");
}


bool FAppleARKitSystem::EnumerateTrackedDevices(TArray<int32>& OutDevices, EXRTrackedDeviceType Type)
{
	if (Type == EXRTrackedDeviceType::Any || Type == EXRTrackedDeviceType::HeadMountedDisplay)
	{
		static const int32 DeviceId = IXRTrackingSystem::HMDDeviceId;
		OutDevices.Add(DeviceId);
		return true;
	}
	return false;
}

FRotator DeriveTrackingToWorldRotation( EScreenOrientation::Type DeviceOrientation )
{
	// We rotate the camera to counteract the portrait vs. landscape viewport rotation
	FRotator DeviceRot = FRotator::ZeroRotator;
	switch (DeviceOrientation)
	{
		case EScreenOrientation::Portrait:
			DeviceRot = FRotator(0.0f, 0.0f, -90.0f);
			break;
			
		case EScreenOrientation::PortraitUpsideDown:
			DeviceRot = FRotator(0.0f, 0.0f, 90.0f);
			break;
			
		default:
		case EScreenOrientation::LandscapeLeft:
			break;
			
		case EScreenOrientation::LandscapeRight:
			DeviceRot = FRotator(0.0f, 0.0f, 180.0f);
			break;
	};
	
	return DeviceRot;
}

void FAppleARKitSystem::UpdateFrame()
{
	FScopeLock ScopeLock( &FrameLock );
	// This might get called multiple times per frame so only update if delegate version is newer
	if (!GameThreadFrame.IsValid() || !LastReceivedFrame.IsValid() ||
		GameThreadFrame->Timestamp < LastReceivedFrame->Timestamp)
	{
		GameThreadFrame = LastReceivedFrame;
		if (GameThreadFrame.IsValid())
		{
			// Used to mark the time at which tracked geometry was updated
			GameThreadFrameNumber++;
			GameThreadTimestamp = GameThreadFrame->Timestamp;
		}
	}
}

void FAppleARKitSystem::UpdatePoses()
{
	if (DeviceOrientation == EScreenOrientation::Unknown)
	{
		SetDeviceOrientation( static_cast<EScreenOrientation::Type>(FPlatformMisc::GetDeviceOrientation()) );
	}
	UpdateFrame();
}


void FAppleARKitSystem::ResetOrientationAndPosition(float Yaw)
{
	// @todo arkit implement FAppleARKitSystem::ResetOrientationAndPosition
}

bool FAppleARKitSystem::IsHeadTrackingAllowed() const
{
	// Check to see if they have disabled the automatic camera tracking or not
	// For face AR tracking movements of the device most likely won't want to be tracked
	const bool bEnableCameraTracking =
		OnGetARSessionStatus().Status == EARSessionStatus::Running &&
		GetSessionConfig().ShouldEnableCameraTracking();

#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000
	if ([IOSAppDelegate GetDelegate].OSVersion >= 11.0f)
	{
		return bEnableCameraTracking;
	}
	else
	{
		return false;
	}
#else
	return false;
#endif //ARKIT_SUPPORT
}

TSharedPtr<class IXRCamera, ESPMode::ThreadSafe> FAppleARKitSystem::GetXRCamera(int32 DeviceId)
{
	if (!XRCamera.IsValid())
	{
		TSharedRef<FAppleARKitXRCamera, ESPMode::ThreadSafe> NewCamera = FSceneViewExtensions::NewExtension<FAppleARKitXRCamera>(*this, DeviceId);
		XRCamera = NewCamera;
	}
	
	return XRCamera;
}

float FAppleARKitSystem::GetWorldToMetersScale() const
{
	// @todo arkit FAppleARKitSystem::GetWorldToMetersScale needs a real scale somehow
	return 100.0f;
}

void FAppleARKitSystem::OnBeginRendering_GameThread()
{
	UpdatePoses();
}

bool FAppleARKitSystem::OnStartGameFrame(FWorldContext& WorldContext)
{
	FXRTrackingSystemBase::OnStartGameFrame(WorldContext);
	
	CachedTrackingToWorld = ComputeTrackingToWorldTransform(WorldContext);
	
	if (GameThreadFrame.IsValid())
	{
		if (GameThreadFrame->LightEstimate.bIsValid)
		{
			UARBasicLightEstimate* NewLightEstimate = NewObject<UARBasicLightEstimate>();
			NewLightEstimate->SetLightEstimate( GameThreadFrame->LightEstimate.AmbientIntensity,  GameThreadFrame->LightEstimate.AmbientColorTemperatureKelvin);
			LightEstimate = NewLightEstimate;
		}
		else
		{
			LightEstimate = nullptr;
		}
		
	}
	
	return true;
}

//bool FAppleARKitSystem::ARLineTraceFromScreenPoint(const FVector2D ScreenPosition, TArray<FARTraceResult>& OutHitResults)
//{
//	const bool bSuccess = HitTestAtScreenPosition(ScreenPosition, EAppleARKitHitTestResultType::ExistingPlaneUsingExtent, OutHitResults);
//	return bSuccess;
//}

void FAppleARKitSystem::OnARSystemInitialized()
{
	// Register for device orientation changes
	FCoreDelegates::ApplicationReceivedScreenOrientationChangedNotificationDelegate.AddThreadSafeSP(this, &FAppleARKitSystem::OrientationChanged);
}

EARTrackingQuality FAppleARKitSystem::OnGetTrackingQuality() const
{
	return GameThreadFrame.IsValid()
		? GameThreadFrame->Camera.TrackingQuality
		: EARTrackingQuality::NotTracking;
}

void FAppleARKitSystem::OnStartARSession(UARSessionConfig* SessionConfig)
{
	Run(SessionConfig);
}

void FAppleARKitSystem::OnPauseARSession()
{
	ensureAlwaysMsgf(false, TEXT("FAppleARKitSystem::OnPauseARSession() is unimplemented."));
}

void FAppleARKitSystem::OnStopARSession()
{
	Pause();
}

FARSessionStatus FAppleARKitSystem::OnGetARSessionStatus() const
{
	return IsRunning()
		? FARSessionStatus(EARSessionStatus::Running)
		: FARSessionStatus(EARSessionStatus::NotStarted);
}

void FAppleARKitSystem::OnSetAlignmentTransform(const FTransform& InAlignmentTransform)
{
	const FTransform& NewAlignmentTransform = InAlignmentTransform;
	
	// Update transform for all geometries
	for (auto GeoIt=TrackedGeometries.CreateIterator(); GeoIt; ++GeoIt)
	{
		GeoIt.Value()->UpdateAlignmentTransform(NewAlignmentTransform);
	}
	
	// Update transform for all Pins
	for (UARPin* Pin : Pins)
	{
		Pin->UpdateAlignmentTransform(NewAlignmentTransform);
	}
	
	SetAlignmentTransform_Internal(InAlignmentTransform);
}

static bool IsHitInRange( float UnrealHitDistance )
{
    // Skip results further than 5m or closer that 20cm from camera
	return 20.0f < UnrealHitDistance && UnrealHitDistance < 500.0f;
}

#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000

static UARTrackedGeometry* FindGeometryFromAnchor( ARAnchor* InAnchor, TMap<FGuid, UARTrackedGeometry*>& Geometries )
{
	if (InAnchor != NULL)
	{
		const FGuid AnchorGUID = ToFGuid( InAnchor.identifier );
		UARTrackedGeometry** Result = Geometries.Find(AnchorGUID);
		if (Result != nullptr)
		{
			return *Result;
		}
	}
	
	return nullptr;
}

#endif//ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000

TArray<FARTraceResult> FAppleARKitSystem::OnLineTraceTrackedObjects( const FVector2D ScreenCoord, EARLineTraceChannels TraceChannels )
{
	const float WorldToMetersScale = GetWorldToMetersScale();
	TArray<FARTraceResult> Results;
	
	// Sanity check
	if (IsRunning())
	{
#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000
		
		TSharedRef<FARSystemBase, ESPMode::ThreadSafe> This = SharedThis(this);
		
		@autoreleasepool
		{
			// Perform a hit test on the Session's last frame
			ARFrame* HitTestFrame = Session.currentFrame;
			if (HitTestFrame)
			{
				Results.Reserve(8);
				
				// Convert the screen position to normalised coordinates in the capture image space
				FVector2D NormalizedImagePosition = FAppleARKitCamera( HitTestFrame.camera ).GetImageCoordinateForScreenPosition( ScreenCoord, EAppleARKitBackgroundFitMode::Fill );
				switch (DeviceOrientation)
				{
					case EScreenOrientation::Portrait:
						NormalizedImagePosition = FVector2D( NormalizedImagePosition.Y, 1.0f - NormalizedImagePosition.X );
						break;
						
					case EScreenOrientation::PortraitUpsideDown:
						NormalizedImagePosition = FVector2D( 1.0f - NormalizedImagePosition.Y, NormalizedImagePosition.X );
						break;
						
					default:
					case EScreenOrientation::LandscapeLeft:
						break;
						
					case EScreenOrientation::LandscapeRight:
						NormalizedImagePosition = FVector2D(1.0f, 1.0f) - NormalizedImagePosition;
						break;
				};
				
				// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("Hit Test At Screen Position: x: %f, y: %f"), NormalizedImagePosition.X, NormalizedImagePosition.Y));
				
				// First run hit test against existing planes with extents (converting & filtering results as we go)
				if (!!(TraceChannels & EARLineTraceChannels::PlaneUsingExtent) || !!(TraceChannels & EARLineTraceChannels::PlaneUsingBoundaryPolygon))
				{
					// First run hit test against existing planes with extents (converting & filtering results as we go)
					NSArray< ARHitTestResult* >* PlaneHitTestResults = [HitTestFrame hitTest:CGPointMake(NormalizedImagePosition.X, NormalizedImagePosition.Y) types:ARHitTestResultTypeExistingPlaneUsingExtent];
					for ( ARHitTestResult* HitTestResult in PlaneHitTestResults )
					{
						const float UnrealHitDistance = HitTestResult.distance * WorldToMetersScale;
						if ( IsHitInRange( UnrealHitDistance ) )
						{
							// Hit result has passed and above filtering, add it to the list
							// Convert to Unreal's Hit Test result format
							Results.Add( FARTraceResult( This, UnrealHitDistance, EARLineTraceChannels::PlaneUsingExtent, FAppleARKitTransform::ToFTransform( HitTestResult.worldTransform, WorldToMetersScale )*GetAlignmentTransform(), FindGeometryFromAnchor(HitTestResult.anchor, TrackedGeometries) ) );
						}
					}
				}
				
				// If there were no valid results, fall back to hit testing against one shot plane
				if (!!(TraceChannels & EARLineTraceChannels::GroundPlane))
				{
					NSArray< ARHitTestResult* >* PlaneHitTestResults = [HitTestFrame hitTest:CGPointMake(NormalizedImagePosition.X, NormalizedImagePosition.Y) types:ARHitTestResultTypeEstimatedHorizontalPlane];
					for ( ARHitTestResult* HitTestResult in PlaneHitTestResults )
					{
						const float UnrealHitDistance = HitTestResult.distance * WorldToMetersScale;
						if ( IsHitInRange( UnrealHitDistance ) )
						{
							// Hit result has passed and above filtering, add it to the list
							// Convert to Unreal's Hit Test result format
							Results.Add( FARTraceResult( This, UnrealHitDistance, EARLineTraceChannels::GroundPlane, FAppleARKitTransform::ToFTransform( HitTestResult.worldTransform, WorldToMetersScale )*GetAlignmentTransform(), FindGeometryFromAnchor(HitTestResult.anchor, TrackedGeometries) ) );
						}
					}
				}
				
				// If there were no valid results, fall back further to hit testing against feature points
				if (!!(TraceChannels & EARLineTraceChannels::FeaturePoint))
				{
					// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("No results for plane hit test - reverting to feature points"), NormalizedImagePosition.X, NormalizedImagePosition.Y));
					
					NSArray< ARHitTestResult* >* FeatureHitTestResults = [HitTestFrame hitTest:CGPointMake(NormalizedImagePosition.X, NormalizedImagePosition.Y) types:ARHitTestResultTypeFeaturePoint];
					for ( ARHitTestResult* HitTestResult in FeatureHitTestResults )
					{
						const float UnrealHitDistance = HitTestResult.distance * WorldToMetersScale;
						if ( IsHitInRange( UnrealHitDistance ) )
						{
							// Hit result has passed and above filtering, add it to the list
							// Convert to Unreal's Hit Test result format
							Results.Add( FARTraceResult( This, UnrealHitDistance, EARLineTraceChannels::FeaturePoint, FAppleARKitTransform::ToFTransform( HitTestResult.worldTransform, WorldToMetersScale )*GetAlignmentTransform(), FindGeometryFromAnchor(HitTestResult.anchor, TrackedGeometries) ) );
						}
					}
				}
			}
		}
#endif // ARKIT_SUPPORT
	}
	
	if (Results.Num() > 1)
	{
		Results.Sort([](const FARTraceResult& A, const FARTraceResult& B)
		{
			return A.GetDistanceFromCamera() < B.GetDistanceFromCamera();
		});
	}
	
	return Results;
}

TArray<UARTrackedGeometry*> FAppleARKitSystem::OnGetAllTrackedGeometries() const
{
	TArray<UARTrackedGeometry*> Geometries;
	TrackedGeometries.GenerateValueArray(Geometries);
	return Geometries;
}

TArray<UARPin*> FAppleARKitSystem::OnGetAllPins() const
{
	return Pins;
}

UARLightEstimate* FAppleARKitSystem::OnGetCurrentLightEstimate() const
{
	return LightEstimate;
}

UARPin* FAppleARKitSystem::OnPinComponent( USceneComponent* ComponentToPin, const FTransform& PinToWorldTransform, UARTrackedGeometry* TrackedGeometry, const FName DebugName )
{
	if ( ensureMsgf(ComponentToPin != nullptr, TEXT("Cannot pin component.")) )
	{
		if (UARPin* FindResult = ARKitUtil::PinFromComponent(ComponentToPin, Pins))
		{
			UE_LOG(LogAppleARKit, Warning, TEXT("Component %s is already pinned. Unpin it first."), *ComponentToPin->GetReadableName());
			OnRemovePin(FindResult);
		}

		// PinToWorld * AlignedTrackingToWorld(-1) * TrackingToAlignedTracking(-1) = PinToWorld * WorldToAlignedTracking * AlignedTrackingToTracking
		// The Worlds and AlignedTracking cancel out, and we get PinToTracking
		// But we must translate this logic into Unreal's transform API
		const FTransform& TrackingToAlignedTracking = GetAlignmentTransform();
		const FTransform PinToTrackingTransform = PinToWorldTransform.GetRelativeTransform(GetTrackingToWorldTransform()).GetRelativeTransform(TrackingToAlignedTracking);

		// If the user did not provide a TrackedGeometry, create the simplest TrackedGeometry for this pin.
		UARTrackedGeometry* GeometryToPinTo = TrackedGeometry;
		if (GeometryToPinTo == nullptr)
		{
			GeometryToPinTo = NewObject<UARTrackedPoint>();
			GeometryToPinTo->UpdateTrackedGeometry(SharedThis(this), GameThreadFrameNumber, GameThreadTimestamp, PinToTrackingTransform, GetAlignmentTransform());
		}
		
		UARPin* NewPin = NewObject<UARPin>();
		NewPin->InitARPin(SharedThis(this), ComponentToPin, PinToTrackingTransform, GeometryToPinTo, DebugName);
		
		Pins.Add(NewPin);
		
		return NewPin;
	}
	else
	{
		return nullptr;
	}
}

void FAppleARKitSystem::OnRemovePin(UARPin* PinToRemove)
{
	Pins.RemoveSingleSwap(PinToRemove);
}

bool FAppleARKitSystem::GetCurrentFrame(FAppleARKitFrame& OutCurrentFrame) const
{
	if( GameThreadFrame.IsValid() )
	{
		OutCurrentFrame = *GameThreadFrame;
		return true;
	}
	else
	{
		return false;
	}
}

bool FAppleARKitSystem::OnIsTrackingTypeSupported(EARSessionType SessionType) const
{
#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000
	switch (SessionType)
	{
		case EARSessionType::Orientation:
		{
			return AROrientationTrackingConfiguration.isSupported == TRUE;
		}
		case EARSessionType::World:
		{
			return ARWorldTrackingConfiguration.isSupported == TRUE;
		}
		case EARSessionType::Face:
		{
			return ARFaceTrackingConfiguration.isSupported == TRUE;
		}
	}
#endif
	return false;
}

void FAppleARKitSystem::AddReferencedObjects( FReferenceCollector& Collector )
{
	FARSystemBase::AddReferencedObjects(Collector);

	Collector.AddReferencedObjects( TrackedGeometries );
	Collector.AddReferencedObjects( Pins );
	
	if(LightEstimate)
	{
		Collector.AddReferencedObject(LightEstimate);
	}
}

bool FAppleARKitSystem::HitTestAtScreenPosition(const FVector2D ScreenPosition, EAppleARKitHitTestResultType InTypes, TArray< FAppleARKitHitTestResult >& OutResults)
{
	ensureMsgf(false,TEXT("UNIMPLEMENTED; see OnLineTraceTrackedObjects()"));
	return false;
}

static TOptional<EScreenOrientation::Type> PickAllowedDeviceOrientation( EScreenOrientation::Type InOrientation )
{
#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000
	const UIOSRuntimeSettings* IOSSettings = GetDefault<UIOSRuntimeSettings>();
	
	const bool bOrientationSupported[] =
	{
		true, // Unknown
		IOSSettings->bSupportsPortraitOrientation != 0, // Portait
		IOSSettings->bSupportsUpsideDownOrientation != 0, // PortraitUpsideDown
		IOSSettings->bSupportsLandscapeRightOrientation != 0, // LandscapeLeft; These are flipped vs the enum name?
		IOSSettings->bSupportsLandscapeLeftOrientation != 0, // LandscapeRight; These are flipped vs the enum name?
		false, // FaceUp
		false // FaceDown
	};
	
	if (bOrientationSupported[static_cast<int32>(InOrientation)])
	{
		return InOrientation;
	}
	else
	{
		return TOptional<EScreenOrientation::Type>();
	}
#else
	return TOptional<EScreenOrientation::Type>();
#endif
}

void FAppleARKitSystem::SetDeviceOrientation( EScreenOrientation::Type InOrientation )
{
	TOptional<EScreenOrientation::Type> NewOrientation = PickAllowedDeviceOrientation(InOrientation);

	if (!NewOrientation.IsSet() && DeviceOrientation == EScreenOrientation::Unknown)
	{
		// We do not currently have a valid orientation, nor did the device provide one.
		// So pick ANY ALLOWED default.
		// This only realy happens if the device is face down on something or
		// in another "useless" state for AR.
		
		if (!NewOrientation.IsSet())
		{
			NewOrientation = PickAllowedDeviceOrientation(EScreenOrientation::Portrait);
		}
		
		if (!NewOrientation.IsSet())
		{
			NewOrientation = PickAllowedDeviceOrientation(EScreenOrientation::LandscapeLeft);
		}
		
		if (!NewOrientation.IsSet())
		{
			NewOrientation = PickAllowedDeviceOrientation(EScreenOrientation::PortraitUpsideDown);
		}
		
		if (!NewOrientation.IsSet())
		{
			NewOrientation = PickAllowedDeviceOrientation(EScreenOrientation::LandscapeRight);
		}
		
		check(NewOrientation.IsSet());
	}
	
	if (NewOrientation.IsSet() && DeviceOrientation != NewOrientation.GetValue())
	{
		DeviceOrientation = NewOrientation.GetValue();
		DerivedTrackingToUnrealRotation = DeriveTrackingToWorldRotation( DeviceOrientation );
	}
}


bool FAppleARKitSystem::Run(UARSessionConfig* SessionConfig)
{
// @todo - JoeG & NickA talk about incompatible session types and what to do here

	if (IsRunning())
	{
		UE_LOG(LogAppleARKit, Log, TEXT("Session already running"), this);

		return true;
	}

	// @todo arkit FAppleARKitSystem::GetWorldToMetersScale needs a real scale somehow
	FAppleARKitConfiguration Config;
#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000
	if ([IOSAppDelegate GetDelegate].OSVersion >= 11.0f)
	{
		ARSessionRunOptions options = 0;

		// Convert to native ARWorldTrackingSessionConfiguration
		ARConfiguration* Configuration = ToARConfiguration(SessionConfig, Config);
		// Not all session types are supported by all devices
		if (Configuration == nullptr)
		{
			UE_LOG(LogAppleARKit, Log, TEXT("The requested session type is not supported by this device"));
			return false;
		}

		// Create our ARSessionDelegate
		if (Delegate == nullptr)
		{
			Delegate = [[FAppleARKitSessionDelegate alloc] initWithAppleARKitSystem:this];
		}

		if (Session == nullptr)
		{
			// Start a new ARSession
			Session = [ARSession new];
			Session.delegate = Delegate;
			Session.delegateQueue = dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0);
		}
		else
		{
			// pause and start with new options
			options = ARSessionRunOptionResetTracking | ARSessionRunOptionRemoveExistingAnchors;
			[Session pause];
		}

		// Create MetalTextureCache
		if (IsMetalPlatform(GMaxRHIShaderPlatform))
		{
			id<MTLDevice> Device = (id<MTLDevice>)GDynamicRHI->RHIGetNativeDevice();
			check(Device);

			CVReturn Return = CVMetalTextureCacheCreate(nullptr, nullptr, Device, nullptr, &MetalTextureCache);
			check(Return == kCVReturnSuccess);
			check(MetalTextureCache);

			// Pass to session delegate to use for Metal texture creation
			[Delegate setMetalTextureCache : MetalTextureCache];
		}

		UE_LOG(LogAppleARKit, Log, TEXT("Starting session: %p with options %d"), this, options);

		// Start the session with the configuration
		[Session runWithConfiguration : Configuration options : options];
	}
	
#endif // ARKIT_SUPPORT
	
	// @todo arkit Add support for relocating ARKit space to Unreal World Origin? BaseTransform = FTransform::Identity;
	
	// Set running state
	bIsRunning = true;
	
	return true;
}

bool FAppleARKitSystem::IsRunning() const
{
	return bIsRunning;
}

bool FAppleARKitSystem::Pause()
{
	// Already stopped?
	if (!IsRunning())
	{
		return true;
	}
	
	UE_LOG(LogAppleARKit, Log, TEXT("Stopping session: %p"), this);
	
#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000
	if ([IOSAppDelegate GetDelegate].OSVersion >= 11.0f)
	{
		// Suspend the session
		[Session pause];
	
		// Release MetalTextureCache created in Start
		if (MetalTextureCache)
		{
			// Tell delegate to release it
			[Delegate setMetalTextureCache:nullptr];
		
			CFRelease(MetalTextureCache);
			MetalTextureCache = nullptr;
		}
	}
	
#endif // ARKIT_SUPPORT
	
	// Set running state
	bIsRunning = false;
	
	return true;
}

void FAppleARKitSystem::OrientationChanged(const int32 NewOrientationRaw)
{
	const EScreenOrientation::Type NewOrientation = static_cast<EScreenOrientation::Type>(NewOrientationRaw);
	SetDeviceOrientation(NewOrientation);
}
						
void FAppleARKitSystem::SessionDidUpdateFrame_DelegateThread(TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > Frame)
{
	// Thread safe swap buffered frame
	DECLARE_CYCLE_STAT(TEXT("FAppleARKitSystem::SessionDidUpdateFrame_DelegateThread"),
					   STAT_FAppleARKitSystem_SessionUpdateFrame,
					   STATGROUP_APPLEARKIT);
	
	auto UpdateFrameTask = FSimpleDelegateGraphTask::FDelegate::CreateThreadSafeSP( this, &FAppleARKitSystem::SessionDidUpdateFrame_Internal, Frame.ToSharedRef() );
	FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(UpdateFrameTask, GET_STATID(STAT_FAppleARKitSystem_SessionUpdateFrame), nullptr, ENamedThreads::GameThread);
}
			
void FAppleARKitSystem::SessionDidFailWithError_DelegateThread(const FString& Error)
{
	UE_LOG(LogAppleARKit, Warning, TEXT("Session failed with error: %s"), *Error);
}

#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000

static TSharedPtr<FAppleARKitAnchorData> MakeAnchorData( ARAnchor* Anchor, const float WorldToMeterScale )
{
	// Construct appropriate UAppleARKitAnchor subclass
	TSharedPtr<FAppleARKitAnchorData> NewAnchor;
	if ([Anchor isKindOfClass:[ARPlaneAnchor class]])
	{
		ARPlaneAnchor* PlaneAnchor = (ARPlaneAnchor*)Anchor;
		NewAnchor = MakeShared<FAppleARKitAnchorData>(
			ToFGuid(PlaneAnchor.identifier),
			FAppleARKitTransform::ToFTransform(PlaneAnchor.transform, WorldToMeterScale),
			FAppleARKitTransform::ToFVector(PlaneAnchor.center, WorldToMeterScale),
			// @todo use World Settings WorldToMetersScale
			0.5f*FAppleARKitTransform::ToFVector(PlaneAnchor.extent, WorldToMeterScale).GetAbs()
		);
	}
	else if ([Anchor isKindOfClass:[ARFaceAnchor class]])
	{
		ARFaceAnchor* FaceAnchor = (ARFaceAnchor*)Anchor;
		NewAnchor = MakeShared<FAppleARKitAnchorData>(
			ToFGuid(FaceAnchor.identifier),
			FAppleARKitTransform::ToFTransform(FaceAnchor.transform, WorldToMeterScale),
			ToBlendShapeMap(FaceAnchor.blendShapes),
			ToVertexBuffer(FaceAnchor.geometry.vertices, FaceAnchor.geometry.vertexCount)
		);
		// Only convert from 16bit to 32bit once
		if (FAppleARKitAnchorData::FaceIndices.Num() == 0)
		{
			FAppleARKitAnchorData::FaceIndices = To32BitIndexBuffer(FaceAnchor.geometry.triangleIndices, FaceAnchor.geometry.triangleCount * 3);
		}
	}
	else
	{
		NewAnchor = MakeShared<FAppleARKitAnchorData>(
			ToFGuid(Anchor.identifier),
			FAppleARKitTransform::ToFTransform(Anchor.transform, WorldToMeterScale));
	}
	
	return NewAnchor;
}

void FAppleARKitSystem::SessionDidAddAnchors_DelegateThread( NSArray<ARAnchor*>* anchors )
{
	DECLARE_CYCLE_STAT(TEXT("FAppleARKitSystem::SessionDidAddAnchors_DelegateThread"),
					   STAT_FAppleARKitSystem_SessionDidAddAnchors,
					   STATGROUP_APPLEARKIT);

	for (ARAnchor* anchor in anchors)
	{
		TSharedPtr<FAppleARKitAnchorData> NewAnchorData = MakeAnchorData(anchor, GetWorldToMetersScale());
		if (ensure(NewAnchorData.IsValid()))
		{
			auto AddAnchorTask = FSimpleDelegateGraphTask::FDelegate::CreateSP(this, &FAppleARKitSystem::SessionDidAddAnchors_Internal, NewAnchorData.ToSharedRef());
			FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(AddAnchorTask, GET_STATID(STAT_FAppleARKitSystem_SessionDidAddAnchors), nullptr, ENamedThreads::GameThread);
		}
	}
}

void FAppleARKitSystem::SessionDidUpdateAnchors_DelegateThread( NSArray<ARAnchor*>* anchors )
{
	DECLARE_CYCLE_STAT(TEXT("FAppleARKitSystem::SessionDidUpdateAnchors_DelegateThread"),
					   STAT_FAppleARKitSystem_SessionDidUpdateAnchors,
					   STATGROUP_APPLEARKIT);
	
	for (ARAnchor* anchor in anchors)
	{
		TSharedPtr<FAppleARKitAnchorData> NewAnchorData = MakeAnchorData(anchor, GetWorldToMetersScale());
		if (ensure(NewAnchorData.IsValid()))
		{
			auto UpdateAnchorTask = FSimpleDelegateGraphTask::FDelegate::CreateSP(this, &FAppleARKitSystem::SessionDidUpdateAnchors_Internal, NewAnchorData.ToSharedRef());
			FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(UpdateAnchorTask, GET_STATID(STAT_FAppleARKitSystem_SessionDidUpdateAnchors), nullptr, ENamedThreads::GameThread);
		}
	}
}

void FAppleARKitSystem::SessionDidRemoveAnchors_DelegateThread( NSArray<ARAnchor*>* anchors )
{
	DECLARE_CYCLE_STAT(TEXT("FAppleARKitSystem::SessionDidRemoveAnchors_DelegateThread"),
					   STAT_FAppleARKitSystem_SessionDidRemoveAnchors,
					   STATGROUP_APPLEARKIT);
	
	for (ARAnchor* anchor in anchors)
	{
		// Convert to FGuid
		const FGuid AnchorGuid = ToFGuid( anchor.identifier );

		auto RemoveAnchorTask = FSimpleDelegateGraphTask::FDelegate::CreateSP(this, &FAppleARKitSystem::SessionDidRemoveAnchors_Internal, AnchorGuid);
		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(RemoveAnchorTask, GET_STATID(STAT_FAppleARKitSystem_SessionDidRemoveAnchors), nullptr, ENamedThreads::GameThread);
	}
}


void FAppleARKitSystem::SessionDidAddAnchors_Internal( TSharedRef<FAppleARKitAnchorData> AnchorData )
{
	// In case we have camera tracking turned off, we still need to update the frame
	if (!GetSessionConfig().ShouldEnableCameraTracking())
	{
		UpdateFrame();
	}

	UARTrackedGeometry* NewGeometry = nullptr;
	switch (AnchorData->AnchorType)
	{
		case EAppleAnchorType::Anchor:
		{
			NewGeometry = NewObject<UARTrackedGeometry>();
			NewGeometry->UpdateTrackedGeometry(SharedThis(this), GameThreadFrameNumber, GameThreadTimestamp, AnchorData->Transform, GetAlignmentTransform());
			break;
		}
		case EAppleAnchorType::PlaneAnchor:
		{
			UARPlaneGeometry* NewGeo = NewObject<UARPlaneGeometry>();
			NewGeo->UpdateTrackedGeometry(SharedThis(this), GameThreadFrameNumber, GameThreadTimestamp, AnchorData->Transform, GetAlignmentTransform(), AnchorData->Center, AnchorData->Extent);
			NewGeometry = NewGeo;
			break;
		}
		case EAppleAnchorType::FaceAnchor:
		{
			// Update LiveLink first, because the other updates use MoveTemp for efficiency
			if (LiveLinkSource.IsValid())
			{
				LiveLinkSource->PublishBlendShapes(FaceTrackingLiveLinkSubjectName, GameThreadTimestamp, GameThreadFrameNumber, AnchorData->BlendShapes);
			}

			UARFaceGeometry* NewGeo = NewObject<UARFaceGeometry>();
			NewGeo->UpdateTrackedGeometry(SharedThis(this), GameThreadFrameNumber, GameThreadTimestamp, AnchorData->Transform, GetAlignmentTransform(), AnchorData->BlendShapes, AnchorData->FaceVerts, AnchorData->FaceIndices);
			NewGeometry = NewGeo;
			break;
		}
	}
	check(NewGeometry != nullptr);

	UARTrackedGeometry* NewTrackedGeometry = TrackedGeometries.Add( AnchorData->AnchorGUID, NewGeometry );
	
	const FString NewAnchorDebugName = FString::Printf(TEXT("APPL-%02d"), LastTrackedGeometry_DebugId++);
	NewTrackedGeometry->SetDebugName( FName(*NewAnchorDebugName) );
}

void FAppleARKitSystem::SessionDidUpdateAnchors_Internal( TSharedRef<FAppleARKitAnchorData> AnchorData )
{
	// In case we have camera tracking turned off, we still need to update the frame
	if (!GetSessionConfig().ShouldEnableCameraTracking())
	{
		UpdateFrame();
	}

	UARTrackedGeometry** GeometrySearchResult = TrackedGeometries.Find(AnchorData->AnchorGUID);
	if (ensure(GeometrySearchResult != nullptr))
	{
		UARTrackedGeometry* FoundGeometry = *GeometrySearchResult;
		TArray<UARPin*> PinsToUpdate = ARKitUtil::PinsFromGeometry(FoundGeometry, Pins);
		
		
		// We figure out the delta transform for the Anchor (aka. TrackedGeometry in ARKit) and apply that
		// delta to figure out the new ARPin transform.
		const FTransform Anchor_LocalToTrackingTransform_PreUpdate = FoundGeometry->GetLocalToTrackingTransform_NoAlignment();
		const FTransform& Anchor_LocalToTrackingTransform_PostUpdate = AnchorData->Transform;
		
		const FTransform AnchorDeltaTransform = Anchor_LocalToTrackingTransform_PreUpdate.GetRelativeTransform(Anchor_LocalToTrackingTransform_PostUpdate);
		
		switch (AnchorData->AnchorType)
		{
			case EAppleAnchorType::Anchor:
			{
				FoundGeometry->UpdateTrackedGeometry(SharedThis(this), GameThreadFrameNumber, GameThreadTimestamp, AnchorData->Transform, GetAlignmentTransform());
				for (UARPin* Pin : PinsToUpdate)
				{
					const FTransform Pin_LocalToTrackingTransform_PostUpdate = Pin->GetLocalToTrackingTransform_NoAlignment() * AnchorDeltaTransform;
					Pin->OnTransformUpdated(Pin_LocalToTrackingTransform_PostUpdate);
				}
				
				break;
			}
			case EAppleAnchorType::PlaneAnchor:
			{
				if (UARPlaneGeometry* PlaneGeo = Cast<UARPlaneGeometry>(FoundGeometry))
				{
					PlaneGeo->UpdateTrackedGeometry(SharedThis(this), GameThreadFrameNumber, GameThreadTimestamp, AnchorData->Transform, GetAlignmentTransform(), AnchorData->Center, AnchorData->Extent);
					for (UARPin* Pin : PinsToUpdate)
					{
						const FTransform Pin_LocalToTrackingTransform_PostUpdate = Pin->GetLocalToTrackingTransform_NoAlignment() * AnchorDeltaTransform;
						Pin->OnTransformUpdated(Pin_LocalToTrackingTransform_PostUpdate);
					}
				}
				break;
			}
			case EAppleAnchorType::FaceAnchor:
			{
				if (UARFaceGeometry* FaceGeo = Cast<UARFaceGeometry>(FoundGeometry))
				{
					// Update LiveLink first, because the other updates use MoveTemp for efficiency
					if (LiveLinkSource.IsValid())
					{
						LiveLinkSource->PublishBlendShapes(FaceTrackingLiveLinkSubjectName, GameThreadTimestamp, GameThreadFrameNumber, AnchorData->BlendShapes);
					}

					FaceGeo->UpdateTrackedGeometry(SharedThis(this), GameThreadFrameNumber, GameThreadTimestamp, AnchorData->Transform, GetAlignmentTransform(), AnchorData->BlendShapes, AnchorData->FaceVerts, AnchorData->FaceIndices);
					for (UARPin* Pin : PinsToUpdate)
					{
						const FTransform Pin_LocalToTrackingTransform_PostUpdate = Pin->GetLocalToTrackingTransform_NoAlignment() * AnchorDeltaTransform;
						Pin->OnTransformUpdated(Pin_LocalToTrackingTransform_PostUpdate);
					}
				}
				break;
			}
		}
	}
}

void FAppleARKitSystem::SessionDidRemoveAnchors_Internal( FGuid AnchorGuid )
{
	// In case we have camera tracking turned off, we still need to update the frame
	if (!GetSessionConfig().ShouldEnableCameraTracking())
	{
		UpdateFrame();
	}

	// Notify pin that it is being orphaned
	{
		UARTrackedGeometry* TrackedGeometryBeingRemoved = TrackedGeometries.FindChecked(AnchorGuid);
		TrackedGeometryBeingRemoved->UpdateTrackingState(EARTrackingState::StoppedTracking);
		
		TArray<UARPin*> ARPinsBeingOrphaned = ARKitUtil::PinsFromGeometry(TrackedGeometryBeingRemoved, Pins);
		for(UARPin* PinBeingOrphaned : ARPinsBeingOrphaned)
		{
			PinBeingOrphaned->OnTrackingStateChanged(EARTrackingState::StoppedTracking);
		}
	}
	
	TrackedGeometries.Remove(AnchorGuid);
}

#endif //ARKIT_SUPPORT

void FAppleARKitSystem::SessionDidUpdateFrame_Internal( TSharedRef< FAppleARKitFrame, ESPMode::ThreadSafe > Frame )
{
	LastReceivedFrame = Frame;
	UpdateFrame();
}





namespace AppleARKitSupport
{
	TSharedPtr<class FAppleARKitSystem, ESPMode::ThreadSafe> CreateAppleARKitSystem()
	{
#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000
		const bool bIsARApp = GetDefault<UGeneralProjectSettings>()->bSupportAR;
		if (bIsARApp)
		{
			auto NewARKitSystem = NewARSystem<FAppleARKitSystem>();
			return NewARKitSystem;
		}
#endif
		return TSharedPtr<class FAppleARKitSystem, ESPMode::ThreadSafe>();
	}
}
