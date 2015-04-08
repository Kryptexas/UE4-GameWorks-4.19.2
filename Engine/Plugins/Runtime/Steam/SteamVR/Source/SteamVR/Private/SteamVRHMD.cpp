// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SteamVRPrivatePCH.h"
#include "SteamVRHMD.h"

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "PostProcess/PostProcessHMD.h"
#include "Classes/SteamVRFunctionLibrary.h"


//---------------------------------------------------
// SteamVR Plugin Implementation
//---------------------------------------------------

class FSteamVRPlugin : public ISteamVRPlugin
{
	/** IHeadMountedDisplayModule implementation */
	virtual TSharedPtr< class IHeadMountedDisplay > CreateHeadMountedDisplay() override;

	FString GetModulePriorityKeyName() const
	{
		return FString(TEXT("SteamVR"));
	}
};

IMPLEMENT_MODULE( FSteamVRPlugin, SteamVR )

TSharedPtr< class IHeadMountedDisplay > FSteamVRPlugin::CreateHeadMountedDisplay()
{
#if STEAMVR_SUPPORTED_PLATFORMS
	TSharedPtr< FSteamVRHMD > SteamVRHMD( new FSteamVRHMD() );
	if( SteamVRHMD->IsInitialized() )
	{
		return SteamVRHMD;
	}
#endif//STEAMVR_SUPPORTED_PLATFORMS
	return nullptr;
}


//---------------------------------------------------
// SteamVR IHeadMountedDisplay Implementation
//---------------------------------------------------

#if STEAMVR_SUPPORTED_PLATFORMS


bool FSteamVRHMD::IsHMDEnabled() const
{
	return bHmdEnabled;
}

void FSteamVRHMD::EnableHMD(bool enable)
{
	bHmdEnabled = enable;

	if (!bHmdEnabled)
	{
		EnableStereo(false);
	}
}

EHMDDeviceType::Type FSteamVRHMD::GetHMDDeviceType() const
{
	return EHMDDeviceType::DT_NoPost;
}

bool FSteamVRHMD::GetHMDMonitorInfo(MonitorInfo& MonitorDesc) 
{
	if (IsInitialized())
	{
		int32 X, Y;
		uint32 Width, Height;
		Hmd->GetWindowBounds(&X, &Y, &Width, &Height);

		MonitorDesc.MonitorName = DisplayId;
		MonitorDesc.MonitorId	= 0;
		MonitorDesc.DesktopX	= X;
		MonitorDesc.DesktopY	= Y;
		MonitorDesc.ResolutionX = Width;
		MonitorDesc.ResolutionY = Height;

		return true;
	}
	else
	{
		MonitorDesc.MonitorName = "";
		MonitorDesc.MonitorId = 0;
		MonitorDesc.DesktopX = MonitorDesc.DesktopY = MonitorDesc.ResolutionX = MonitorDesc.ResolutionY = 0;
	}

	return false;
}

void FSteamVRHMD::GetFieldOfView(float& OutHFOVInDegrees, float& OutVFOVInDegrees) const
{
	OutHFOVInDegrees = 0.0f;
	OutVFOVInDegrees = 0.0f;
}

bool FSteamVRHMD::DoesSupportPositionalTracking() const
{
	return true;
}

bool FSteamVRHMD::HasValidTrackingPosition() const
{
	return bHmdPosTracking && bHaveVisionTracking;
}

void FSteamVRHMD::GetPositionalTrackingCameraProperties(FVector& OutOrigin, FRotator& OutOrientation, float& OutHFOV, float& OutVFOV, float& OutCameraDistance, float& OutNearPlane, float& OutFarPlane) const
{
}

void FSteamVRHMD::SetInterpupillaryDistance(float NewInterpupillaryDistance)
{
}

float FSteamVRHMD::GetInterpupillaryDistance() const
{
	return 0.064f;
}

void FSteamVRHMD::GetCurrentPose(FQuat& CurrentOrientation, FVector& CurrentPosition, uint32 DeviceId, bool bForceRefresh /* = false*/)
{
	if (!Hmd)
	{
		return;
	}

	check(DeviceId < vr::k_unMaxTrackedDeviceCount);

	if (bForceRefresh)
	{
		// With SteamVR, we should only update on the PreRender_ViewFamily, and then the next frame should use the previous frame's results
		check(IsInRenderingThread());

		TrackingFrame.FrameNumber = GFrameNumberRenderThread;

		vr::TrackedDevicePose_t Poses[vr::k_unMaxTrackedDeviceCount];
		VRCompositor->WaitGetPoses(Poses, ARRAYSIZE(Poses));

		for (uint32 i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i)
		{
			TrackingFrame.bDeviceIsValid[i] = Poses[i].bDeviceIsValid;
			TrackingFrame.bPoseIsValid[i] = Poses[i].bPoseIsValid;

			FVector LocalCurrentPosition;
			FQuat LocalCurrentOrientation;
			PoseToOrientationAndPosition(Poses[i].mDeviceToAbsoluteTracking, LocalCurrentOrientation, LocalCurrentPosition);

			TrackingFrame.DeviceOrientation[i] = LocalCurrentOrientation;
			TrackingFrame.DevicePosition[i] = LocalCurrentPosition;

			TrackingFrame.RawPoses[i] = Poses[i].mDeviceToAbsoluteTracking;
		}
	}
	
	// Update CurrentOrientation and CurrentPosition for the desired device, if valid
	if (TrackingFrame.bPoseIsValid[DeviceId])
 	{
		CurrentOrientation = TrackingFrame.DeviceOrientation[DeviceId];
		CurrentPosition = TrackingFrame.DevicePosition[DeviceId];
 	}
}



bool FSteamVRHMD::IsInsideSoftBounds()
{
	if (VRChaperone)
	{
		vr::HmdMatrix34_t VRPose = TrackingFrame.RawPoses[vr::k_unTrackedDeviceIndex_Hmd];
		FMatrix Pose = ToFMatrix(VRPose);
		
		const FVector HMDLocation(Pose.M[3][0], 0.f, Pose.M[3][2]);

		bool bLastWasNegative = false;

		// Since the order of the soft bounds are points on a plane going clockwise, wind around the sides, checking the crossproduct of the affine side to the affine HMD position.  If they're all on the same side, we're in the bounds
		for (uint8 i = 0; i < 4; ++i)
		{
			const FVector PointA = ChaperoneBounds.SoftBounds.Corners[i];
			const FVector PointB = ChaperoneBounds.SoftBounds.Corners[(i + 1) % 4];

			const FVector AffineSegment = PointB - PointA;
			const FVector AffinePoint = HMDLocation - PointA;
			const FVector CrossProduct = FVector::CrossProduct(AffineSegment, AffinePoint);

			const bool bIsNegative = (CrossProduct.Y < 0);

			// If the cross between the point and the side has flipped, that means we're not consistent, and therefore outside the bounds
			if ((i > 0) && (bLastWasNegative != bIsNegative))
			{
				return false;
			}

			bLastWasNegative = bIsNegative;
		}

		return true;
	}

	return false;
}

/** Helper function to convert bounds from SteamVR space to scaled Unreal space*/
TArray<FVector> ConvertBoundsToUnrealSpace(const FBoundingQuad& InBounds, const float WorldToMetersScale)
{
	TArray<FVector> Bounds;

	for (int32 i = 0; i < ARRAYSIZE(InBounds.Corners); ++i)
	{
		const FVector SteamVRCorner = InBounds.Corners[i];
		const FVector UnrealVRCorner(-SteamVRCorner.Z, SteamVRCorner.X, SteamVRCorner.Y);
		Bounds.Add(UnrealVRCorner * WorldToMetersScale);
	}

	return Bounds;
}

TArray<FVector> FSteamVRHMD::GetSoftBounds() const
{
	return ConvertBoundsToUnrealSpace(ChaperoneBounds.SoftBounds, WorldToMetersScale);
}

TArray<FVector> FSteamVRHMD::GetHardBounds() const
{
	TArray<FVector> Bounds;

	for (int32 i = 0; i < ChaperoneBounds.HardBounds.Num(); ++i)
	{
		Bounds.Append(ConvertBoundsToUnrealSpace(ChaperoneBounds.HardBounds[i], WorldToMetersScale));
	}
	
	return Bounds;
}

void FSteamVRHMD::PoseToOrientationAndPosition(const vr::HmdMatrix34_t& InPose, FQuat& OutOrientation, FVector& OutPosition) const
{
	FMatrix Pose = ToFMatrix(InPose);
	FQuat Orientation(Pose);
 
	OutOrientation.X = -Orientation.Z;
	OutOrientation.Y = Orientation.X;
	OutOrientation.Z = Orientation.Y;
 	OutOrientation.W = -Orientation.W;	

	FVector Position = FVector(-Pose.M[3][2], Pose.M[3][0], Pose.M[3][1]) * WorldToMetersScale;
	OutPosition = BaseOrientation.Inverse().RotateVector(Position);

	OutOrientation = BaseOrientation.Inverse() * OutOrientation;
	OutOrientation.Normalize();
}

void FSteamVRHMD::GetCurrentOrientationAndPosition(FQuat& CurrentOrientation, FVector& CurrentPosition)
{
	check(IsInGameThread());
	GetCurrentPose(CurHmdOrientation, CurHmdPosition);
	CurrentOrientation = LastHmdOrientation = CurHmdOrientation;

	CurrentPosition = CurHmdPosition;
}

void FSteamVRHMD::GetTrackedDeviceIds(TArray<int32>& TrackedIds)
{
	TrackedIds.Empty();

	for (uint32 i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i)
	{
		// Add only devices with a currently valid tracked pose, and exclude the HMD
		if ((i != vr::k_unTrackedDeviceIndex_Hmd) && TrackingFrame.bPoseIsValid[i])
	{
			TrackedIds.Add(i);
		}
	}
}

bool FSteamVRHMD::GetTrackedObjectOrientationAndPosition(uint32 DeviceId, FQuat& CurrentOrientation, FVector& CurrentPosition)
{
	check(IsInGameThread());

	bool bHasValidPose = false;

	if (DeviceId < vr::k_unMaxTrackedDeviceCount)
	{
		CurrentOrientation = TrackingFrame.DeviceOrientation[DeviceId];
		CurrentPosition = TrackingFrame.DevicePosition[DeviceId];

		bHasValidPose = TrackingFrame.bPoseIsValid[DeviceId] && TrackingFrame.bDeviceIsValid[DeviceId];
	}

	return bHasValidPose;
}

bool FSteamVRHMD::GetTrackedDeviceIdFromControllerIndex(int32 ControllerIndex, int32& OutDeviceId)
{
	check(IsInGameThread());

	OutDeviceId = -1;

	if ((ControllerIndex < 0) || (ControllerIndex >= MAX_STEAM_CONTROLLERS))
	{
		return false;
	}

	OutDeviceId = ControllerIndexToTrackedDeviceIndex[ControllerIndex];
	
	return (OutDeviceId != -1);
}

ISceneViewExtension* FSteamVRHMD::GetViewExtension()
{
	return this;
}

void FSteamVRHMD::ApplyHmdRotation(APlayerController* PC, FRotator& ViewRotation)
{
	ViewRotation.Normalize();

	GetCurrentPose(CurHmdOrientation, CurHmdPosition);
	LastHmdOrientation = CurHmdOrientation;

	const FRotator DeltaRot = ViewRotation - PC->GetControlRotation();
	DeltaControlRotation = (DeltaControlRotation + DeltaRot).GetNormalized();

	// Pitch from other sources is never good, because there is an absolute up and down that must be respected to avoid motion sickness.
	// Same with roll.
	DeltaControlRotation.Pitch = 0;
	DeltaControlRotation.Roll = 0;
	DeltaControlOrientation = DeltaControlRotation.Quaternion();

	ViewRotation = FRotator(DeltaControlOrientation * CurHmdOrientation);
}

void FSteamVRHMD::UpdatePlayerCameraRotation(APlayerCameraManager* Camera, struct FMinimalViewInfo& POV)
{
	GetCurrentPose(CurHmdOrientation, CurHmdPosition);
	LastHmdOrientation = CurHmdOrientation;

	DeltaControlRotation = POV.Rotation;
	DeltaControlOrientation = DeltaControlRotation.Quaternion();

	// Apply HMD orientation to camera rotation.
	POV.Rotation = FRotator(POV.Rotation.Quaternion() * CurHmdOrientation);
}

bool FSteamVRHMD::IsChromaAbCorrectionEnabled() const
{
	return true;
}

bool FSteamVRHMD::Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar )
{
	if (FParse::Command( &Cmd, TEXT("STEREO") ))
	{
		if (FParse::Command(&Cmd, TEXT("ON")))
		{
			if (!IsHMDEnabled())
			{
				Ar.Logf(TEXT("HMD is disabled. Use 'hmd enable' to re-enable it."));
			}
			EnableStereo(true);
			return true;
		}
		else if (FParse::Command(&Cmd, TEXT("OFF")))
		{
			EnableStereo(false);
			return true;
		}

		float val;
		if (FParse::Value(Cmd, TEXT("E="), val))
		{
			IPD = val;
		}
	}
	else if (FParse::Command(&Cmd, TEXT("HMD")))
	{
		if (FParse::Command(&Cmd, TEXT("ENABLE")))
		{
			EnableHMD(true);
			return true;
		}
		else if (FParse::Command(&Cmd, TEXT("DISABLE")))
		{
			EnableHMD(false);
			return true;
		}

		int32 val;
		if (FParse::Value(Cmd, TEXT("MIRROR"), val))
		{
			if ((val >= 0) && (val <= 2))
			{
				WindowMirrorMode = val;
			}
			else
		{
				Ar.Logf(TEXT("HMD MIRROR accepts values from 0 though 2"));
		}

			return true;
		}
	}
	else if (FParse::Command(&Cmd, TEXT("UNCAPFPS")))
	{
		GEngine->bSmoothFrameRate = false;
		return true;
	}

	return false;
}

void FSteamVRHMD::OnScreenModeChange(EWindowMode::Type WindowMode)
{
	EnableStereo(WindowMode != EWindowMode::Windowed);
}

bool FSteamVRHMD::IsPositionalTrackingEnabled() const
{
	return bHmdPosTracking;
}

bool FSteamVRHMD::EnablePositionalTracking(bool enable)
{
	bHmdPosTracking = enable;
	return IsPositionalTrackingEnabled();
}

bool FSteamVRHMD::IsHeadTrackingAllowed() const
{
	return GEngine->IsStereoscopic3D();
}

bool FSteamVRHMD::IsInLowPersistenceMode() const
{
	return true;
}

void FSteamVRHMD::EnableLowPersistenceMode(bool Enable)
{
}


void FSteamVRHMD::ResetOrientationAndPosition(float yaw)
{
	ResetOrientation(yaw);
	ResetPosition();
}

void FSteamVRHMD::ResetOrientation(float Yaw)
{
	FRotator ViewRotation;
	ViewRotation = FRotator(TrackingFrame.DeviceOrientation[vr::k_unTrackedDeviceIndex_Hmd]);
	ViewRotation.Pitch = 0;
	ViewRotation.Roll = 0;

	if (Yaw != 0.f)
	{
		// apply optional yaw offset
		ViewRotation.Yaw -= Yaw;
		ViewRotation.Normalize();
	}

	BaseOrientation = ViewRotation.Quaternion();
}
void FSteamVRHMD::ResetPosition()
{
	//@todo steamvr: ResetPosition()
}

void FSteamVRHMD::SetClippingPlanes(float NCP, float FCP)
{
}

void FSteamVRHMD::SetBaseRotation(const FRotator& BaseRot)
{
}
FRotator FSteamVRHMD::GetBaseRotation() const
{
	return FRotator::ZeroRotator;
}

void FSteamVRHMD::SetBaseOrientation(const FQuat& BaseOrient)
{
	BaseOrientation = BaseOrient;
}

FQuat FSteamVRHMD::GetBaseOrientation() const
{
	return BaseOrientation;
}

void FSteamVRHMD::SetPositionOffset(const FVector& PosOff)
{
}

FVector FSteamVRHMD::GetPositionOffset() const
{
	return FVector::ZeroVector;
}

bool FSteamVRHMD::IsStereoEnabled() const
{
	return bStereoEnabled && bHmdEnabled;
}

bool FSteamVRHMD::EnableStereo(bool stereo)
{
	bStereoEnabled = (IsHMDEnabled()) ? stereo : false;

	FSystemResolution::RequestResolutionChange(1280, 720, (stereo) ? EWindowMode::WindowedMirror : EWindowMode::Windowed);

	return bStereoEnabled;
}

void FSteamVRHMD::AdjustViewRect(EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const
{
	//@todo steamvr: get the actual rects from steamvr
	SizeX = SizeX / 2;
	if( StereoPass == eSSP_RIGHT_EYE )
	{
		X += SizeX;
	}
}

void FSteamVRHMD::CalculateStereoViewOffset(const enum EStereoscopicPass StereoPassType, const FRotator& ViewRotation, const float WorldToMeters, FVector& ViewLocation)
{
	if( StereoPassType != eSSP_FULL)
	{
		vr::Hmd_Eye HmdEye = (StereoPassType == eSSP_LEFT_EYE) ? vr::Eye_Left : vr::Eye_Right;
		vr::HmdMatrix34_t HeadFromEye = Hmd->GetEyeToHeadTransform(HmdEye);

		// grab the eye position, currently ignoring the rotation supplied by GetHeadFromEyePose()
		FVector TotalOffset = FVector(-HeadFromEye.m[2][3], HeadFromEye.m[0][3], HeadFromEye.m[1][3]) * WorldToMeters;

		ViewLocation += ViewRotation.Quaternion().RotateVector(TotalOffset);

 		const FVector vHMDPosition = DeltaControlOrientation.RotateVector(TrackingFrame.DevicePosition[vr::k_unTrackedDeviceIndex_Hmd]);
		ViewLocation += vHMDPosition;
	}
}

FMatrix FSteamVRHMD::GetStereoProjectionMatrix(const enum EStereoscopicPass StereoPassType, const float FOV) const
{
	check(IsStereoEnabled());

	vr::Hmd_Eye HmdEye = (StereoPassType == eSSP_LEFT_EYE) ? vr::Eye_Left : vr::Eye_Right;
	float Left, Right, Top, Bottom;

	Hmd->GetProjectionRaw(HmdEye, &Right, &Left, &Top, &Bottom);
	Bottom *= -1.0f;
	Top *= -1.0f;
	Right *= -1.0f;
	Left *= -1.0f;

	float ZNear = GNearClippingPlane;

	float SumRL = (Right + Left);
	float SumTB = (Top + Bottom);
	float InvRL = (1.0f / (Right - Left));
	float InvTB = (1.0f / (Top - Bottom));

#if 1
	FMatrix Mat = FMatrix(
		FPlane((2.0f * InvRL), 0.0f, 0.0f, 0.0f),
		FPlane(0.0f, (2.0f * InvTB), 0.0f, 0.0f),
		FPlane((SumRL * InvRL), (SumTB * InvTB), 0.0f, 1.0f),
		FPlane(0.0f, 0.0f, ZNear, 0.0f)
		);
#else
	vr::HmdMatrix44_t SteamMat = Hmd->GetProjectionMatrix(HmdEye, ZNear, 10000.0f, vr::API_DirectX);
	FMatrix Mat = ToFMatrix(SteamMat);

	Mat.M[3][3] = 0.0f;
	Mat.M[2][3] = 1.0f;
	Mat.M[2][2] = 0.0f;
	Mat.M[3][2] = ZNear;
#endif

	return Mat;

}

void FSteamVRHMD::InitCanvasFromView(FSceneView* InView, UCanvas* Canvas)
{
}

void FSteamVRHMD::PushViewportCanvas(EStereoscopicPass StereoPass, FCanvas *InCanvas, UCanvas *InCanvasObject, FViewport *InViewport) const 
{
	FMatrix m;
	m.SetIdentity();
	InCanvas->PushAbsoluteTransform(m);
}

void FSteamVRHMD::PushViewCanvas(EStereoscopicPass StereoPass, FCanvas *InCanvas, UCanvas *InCanvasObject, FSceneView *InView) const 
{
	FMatrix m;
	m.SetIdentity();
	InCanvas->PushAbsoluteTransform(m);
}

void FSteamVRHMD::GetEyeRenderParams_RenderThread(EStereoscopicPass StereoPass, FVector2D& EyeToSrcUVScaleValue, FVector2D& EyeToSrcUVOffsetValue) const
{
	if (StereoPass == eSSP_LEFT_EYE)
	{
		EyeToSrcUVOffsetValue.X = 0.0f;
		EyeToSrcUVOffsetValue.Y = 0.0f;

		EyeToSrcUVScaleValue.X = 0.5f;
		EyeToSrcUVScaleValue.Y = 1.0f;
	}
	else
	{
		EyeToSrcUVOffsetValue.X = 0.5f;
		EyeToSrcUVOffsetValue.Y = 0.0f;

		EyeToSrcUVScaleValue.X = 0.5f;
		EyeToSrcUVScaleValue.Y = 1.0f;
	}
}


void FSteamVRHMD::ModifyShowFlags(FEngineShowFlags& ShowFlags)
{
	ShowFlags.MotionBlur = 0;
	ShowFlags.HMDDistortion = false;
	ShowFlags.StereoRendering = IsStereoEnabled();
}

void FSteamVRHMD::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
	InView.BaseHmdOrientation = LastHmdOrientation;
	InView.BaseHmdLocation = LastHmdPosition;
	WorldToMetersScale = InView.WorldToMetersScale;
	InViewFamily.bUseSeparateRenderTarget = true;
}

void FSteamVRHMD::PreRenderView_RenderThread(FSceneView& View)
{
	check(IsInRenderingThread());

	// The last view location used to set the view will be in BaseHmdOrientation.  We need to calculate the delta from that, so that
	// cameras that rely on game objects (e.g. other components) for their positions don't need to be updated on the render thread.
	const FQuat DeltaOrient = View.BaseHmdOrientation.Inverse() * TrackingFrame.DeviceOrientation[vr::k_unTrackedDeviceIndex_Hmd];
	View.ViewRotation = FRotator(View.ViewRotation.Quaternion() * DeltaOrient);
 	View.UpdateViewMatrix();
}

void FSteamVRHMD::PreRenderViewFamily_RenderThread(FSceneViewFamily& ViewFamily)
{
	check(IsInRenderingThread());
	GetActiveRHIBridgeImpl()->BeginRendering();

	FVector CurrentPosition;
	FQuat CurrentOrientation;
	GetCurrentPose(CurrentOrientation, CurrentPosition, vr::k_unTrackedDeviceIndex_Hmd, true);
}

void FSteamVRHMD::UpdateViewport(bool bUseSeparateRenderTarget, const FViewport& InViewport, SViewport* ViewportWidget)
{
	check(IsInGameThread());

	FRHIViewport* const ViewportRHI = InViewport.GetViewportRHI().GetReference();

	if (!IsStereoEnabled())
	{
		if (!bUseSeparateRenderTarget)
		{
			ViewportRHI->SetCustomPresent(nullptr);
		}
		return;
	}

	GetActiveRHIBridgeImpl()->UpdateViewport(InViewport, ViewportRHI);
}

FSteamVRHMD::BridgeBaseImpl* FSteamVRHMD::GetActiveRHIBridgeImpl()
{
#if PLATFORM_WINDOWS
	if (pD3D11Bridge)
	{
		return pD3D11Bridge;
	}
#endif

	return nullptr;
}

void FSteamVRHMD::CalculateRenderTargetSize(uint32& InOutSizeX, uint32& InOutSizeY) const
{
	check(IsInGameThread());

//	if (Flags.bScreenPercentageEnabled)
	{
		static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.ScreenPercentage"));
		float value = CVar->GetValueOnGameThread();
		if (value > 0.0f)
		{
			InOutSizeX = FMath::CeilToInt(InOutSizeX * value / 100.f);
			InOutSizeY = FMath::CeilToInt(InOutSizeY * value / 100.f);
		}
	}
}

bool FSteamVRHMD::NeedReAllocateViewportRenderTarget(const FViewport& Viewport) const
{
	check(IsInGameThread());

	if (IsStereoEnabled())
	{
		const uint32 InSizeX = Viewport.GetSizeXY().X;
		const uint32 InSizeY = Viewport.GetSizeXY().Y;
		FIntPoint RenderTargetSize;
		RenderTargetSize.X = Viewport.GetRenderTargetTexture()->GetSizeX();
		RenderTargetSize.Y = Viewport.GetRenderTargetTexture()->GetSizeY();

		uint32 NewSizeX = InSizeX, NewSizeY = InSizeY;
		CalculateRenderTargetSize(NewSizeX, NewSizeY);
		if (NewSizeX != RenderTargetSize.X || NewSizeY != RenderTargetSize.Y)
		{
			return true;
		}
	}
	return false;
}

FSteamVRHMD::FSteamVRHMD() :
	Hmd(nullptr),
	bHmdEnabled(true),
	bStereoEnabled(false),
	bHmdPosTracking(true),
	bHaveVisionTracking(false),
	IPD(0.064f),
	WindowMirrorMode(0),
	CurHmdOrientation(FQuat::Identity),
	LastHmdOrientation(FQuat::Identity),
	BaseOrientation(FQuat::Identity),
	DeltaControlRotation(FRotator::ZeroRotator),
	DeltaControlOrientation(FQuat::Identity),
	CurHmdPosition(FVector::ZeroVector),
	WorldToMetersScale(100.0f),
	RendererModule(nullptr),
	SteamDLLHandle(nullptr),
	IdealScreenPercentage(100.0f)
{
	Startup();
}

FSteamVRHMD::~FSteamVRHMD()
{
	Shutdown();
}

bool FSteamVRHMD::IsInitialized() const
{
	return (Hmd != nullptr);
}

void FSteamVRHMD::Startup()
{
	// load the steam library
	if (!LoadSteamModule())
	{
		return;
	}

	// grab a pointer to the renderer module for displaying our mirror window
	static const FName RendererModuleName("Renderer");
	RendererModule = FModuleManager::GetModulePtr<IRendererModule>(RendererModuleName);
	
	bool ret = SteamAPI_Init();

	// look for a headset
	vr::HmdError HmdErr = vr::HmdError_None;
	Hmd = vr::VR_Init(&HmdErr);
	
	// make sure that the version of the HMD we're compiled against is correct
	Hmd = (vr::IVRSystem*)vr::VR_GetGenericInterface(vr::IVRSystem_Version, &HmdErr);	//@todo steamvr: verify init error handling

	// initialize SteamController().  Note: this is redundant with the SteamController support.  Verify that the API can handle multiple initializations/teardowns
	if (SteamController() != nullptr)
	{
		FString PluginsDir = FPaths::EnginePluginsDir();
		FString ContentDir = FPaths::Combine(*PluginsDir, TEXT("Runtime"), TEXT("Steam"), TEXT("SteamController"), TEXT("Content"));
		FString VdfPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(*ContentDir, TEXT("Controller.vdf")));

		bool bInited = SteamController()->Init(TCHAR_TO_ANSI(*VdfPath));
		UE_LOG(LogHMD, Log, TEXT("SteamController %s initialized with vdf file '%s'."), bInited ? TEXT("could not be") : TEXT("has been"), *VdfPath);
	}

	// attach to the compositor
	if ((Hmd != nullptr) && (HmdErr == vr::HmdError_None))
	{
		VRCompositor = (vr::IVRCompositor*)vr::VR_GetGenericInterface(vr::IVRCompositor_Version, &HmdErr);

		if ((VRCompositor != nullptr) && (HmdErr == vr::HmdError_None))
		{
			// determine our compositor type
			vr::Compositor_DeviceType CompositorDeviceType = vr::Compositor_DeviceType_None;
			if (IsPCPlatform(GMaxRHIShaderPlatform) && !IsOpenGLPlatform(GMaxRHIShaderPlatform))
			{
				CompositorDeviceType = vr::Compositor_DeviceType_D3D11;
			}
			else if (IsOpenGLPlatform(GMaxRHIShaderPlatform))
			{
				check(0);	//@todo steamvr: use old path for mac and linux until support is added
				CompositorDeviceType = vr::Compositor_DeviceType_OpenGL;
			}
		}
	}

	if ((Hmd != nullptr) && (HmdErr == vr::HmdError_None))
	{
		// grab info about the attached display
		char Buf[128];
		FString DriverId;
		vr::TrackedPropertyError Error;

		Hmd->GetStringTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String, Buf, sizeof(Buf), &Error);
		if (Error == vr::TrackedProp_Success)
		{
			DriverId = FString(UTF8_TO_TCHAR(Buf));
		}

		Hmd->GetStringTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String, Buf, sizeof(Buf), &Error);
		if (Error == vr::TrackedProp_Success)
		{
			DisplayId = FString(UTF8_TO_TCHAR(Buf));
		}

		// determine our ideal screen percentage
		uint32 RecommendedWidth, RecommendedHeight;
		Hmd->GetRecommendedRenderTargetSize(&RecommendedWidth, &RecommendedHeight);
		RecommendedWidth *= 2;

		int32 ScreenX, ScreenY;
		uint32 ScreenWidth, ScreenHeight;
		Hmd->GetWindowBounds(&ScreenX, &ScreenY, &ScreenWidth, &ScreenHeight);

		float WidthPercentage = ((float)RecommendedWidth / (float)ScreenWidth) * 100.0f;
		float HeightPercentage = ((float)RecommendedHeight / (float)ScreenHeight) * 100.0f;

		float IdealScreenPercentage = FMath::Max(WidthPercentage, HeightPercentage);

		//@todo steamvr: move out of here
		static IConsoleVariable* CScrPercVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"));

		if (FMath::RoundToInt(CScrPercVar->GetFloat()) != FMath::RoundToInt(IdealScreenPercentage))
		{
			CScrPercVar->Set(IdealScreenPercentage);
		}

		// disable vsync
		static IConsoleVariable* CVSyncVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync"));
		CVSyncVar->Set(false);

		// enforce finishcurrentframe
		static IConsoleVariable* CFCFVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.finishcurrentframe"));
		CFCFVar->Set(true);

		// Uncap fps to enable FPS higher than 62
		GEngine->bSmoothFrameRate = false;

		// Grab the chaperone
		vr::HmdError ChaperoneErr = vr::HmdError_None;
		VRChaperone = (vr::IVRChaperone*)vr::VR_GetGenericInterface(vr::IVRChaperone_Version, &ChaperoneErr);
		if ((VRChaperone != nullptr) && (ChaperoneErr == vr::HmdError_None))
		{
			ChaperoneBounds = FChaperoneBounds(VRChaperone);
		}
		else
		{
			UE_LOG(LogHMD, Warning, TEXT("Failed to initialize Chaperone.  Error: %d"), (int32)ChaperoneErr);
		}

		// Initialize the controller to tracked device mappings
		InitializeControllerMappings();

#if PLATFORM_WINDOWS
		if (IsPCPlatform(GMaxRHIShaderPlatform) && !IsOpenGLPlatform(GMaxRHIShaderPlatform))
		{
			pD3D11Bridge = new D3D11Bridge(this);
		}
#endif

		LoadFromIni();

		UE_LOG(LogHMD, Log, TEXT("SteamVR initialized.  Driver: %s  Display: %s"), *DriverId, *DisplayId);
	}
	else
	{
		UE_LOG(LogHMD, Warning, TEXT("SteamVR failed to initialize.  Error: %d"), (int32)HmdErr);

		if (Hmd != nullptr)
		{
			Hmd = nullptr;
		}
	}
}

void FSteamVRHMD::LoadFromIni()
{
	const TCHAR* SteamVRSettings = TEXT("SteamVR.Settings");
	int32 i;

	if (GConfig->GetInt(SteamVRSettings, TEXT("WindowMirrorMode"), i, GEngineIni))
	{
		WindowMirrorMode = i;
	}
}

void FSteamVRHMD::SaveToIni()
{
	const TCHAR* SteamVRSettings = TEXT("SteamVR.Settings");
	GConfig->SetInt(SteamVRSettings, TEXT("WindowMirrorMode"), WindowMirrorMode, GEngineIni);
}

// Initialize the controller to tracked device mappings
void FSteamVRHMD::InitializeControllerMappings()
{
	// invalidate all of the mappings
	for (uint32 i = 0; i < MAX_STEAM_CONTROLLERS; ++i)
	{
		ControllerIndexToTrackedDeviceIndex[i] = -1;
	}

	if (SteamController() == nullptr)
	{
		return;
	}

	// grab a temporary array of the tracked device ids for comparing later so we're not constantly looping over the tracked devices
	uint32 TrackedDeviceIds[vr::k_unMaxTrackedDeviceCount];
	
	char Buf[128];
	uint32 BufLen = sizeof(Buf);
	vr::TrackedPropertyError Error;

	for (uint32 DeviceIndex = 0; DeviceIndex < vr::k_unMaxTrackedDeviceCount; ++DeviceIndex)
	{
		Hmd->GetStringTrackedDeviceProperty(DeviceIndex, vr::Prop_AttachedDeviceId_String, Buf, sizeof(Buf), &Error);
		if (Error == vr::TrackedProp_Success)
		{
			TrackedDeviceIds[DeviceIndex] = (uint32)FCStringAnsi::Atoi64(Buf);
		}
		else
		{
			TrackedDeviceIds[DeviceIndex] = InvalidTrackedDeviceId;
		}
	}

	SteamControllerState_t ControllerState;

	// iterate over the controllers
	for (uint32 ControllerIndex = 0; ControllerIndex < MAX_STEAM_CONTROLLERS; ++ControllerIndex)
	{
		if (!SteamController()->GetControllerState(ControllerIndex, &ControllerState))
		{
			continue;
		}

		if (ControllerState.unPacketNum == 0)
		{
			continue;
		}

		uint32 ControllerId = ((uint32)ControllerState.sLeftPadX << 16) | ((uint32)ControllerState.sLeftPadY & 0xFFFF);

		// loop over our cached tracked device ids to see which tracked device index we are mapped to
		for (uint32 DeviceIndex = 0; DeviceIndex < vr::k_unMaxTrackedDeviceCount; ++DeviceIndex)
		{
			if (TrackedDeviceIds[DeviceIndex] == ControllerId)
			{
				// found a match, add it to our controller index to tracked device index 
				ControllerIndexToTrackedDeviceIndex[ControllerIndex] = DeviceIndex;
				break;
			}
		}
	}
}

void FSteamVRHMD::Shutdown()
{
	// save any runtime configuration changes to the .ini
	SaveToIni();

	// shut down our headset
	vr::VR_Shutdown();
	Hmd = nullptr;

	// shut down the controller interface
	if (SteamController() != nullptr)
	{
		SteamController()->Shutdown();
	}

	// unload steam library
	UnloadSteamModule();
}

bool FSteamVRHMD::LoadSteamModule()
{
#if PLATFORM_WINDOWS
	#if PLATFORM_64BITS
		FString RootSteamPath;
		TCHAR VROverridePath[MAX_PATH];
		FPlatformMisc::GetEnvironmentVariable(TEXT("VR_OVERRIDE"), VROverridePath, MAX_PATH);
	
		if (FCString::Strlen(VROverridePath) > 0)
		{
			RootSteamPath = FString::Printf(TEXT("%s\\bin\\win64\\"), VROverridePath);
		}
		else
		{
			RootSteamPath = FPaths::EngineDir() / FString::Printf(TEXT("Binaries/ThirdParty/Steamworks/%s/Win64/"), STEAM_SDK_VER);
		}
		
		FPlatformProcess::PushDllDirectory(*RootSteamPath);
		SteamDLLHandle = FPlatformProcess::GetDllHandle(*(RootSteamPath + "steam_api64.dll"));
		FPlatformProcess::PopDllDirectory(*RootSteamPath);
	#else
		FString RootSteamPath = FPaths::EngineDir() / FString::Printf(TEXT("Binaries/ThirdParty/Steamworks/%s/Win32/"), STEAM_SDK_VER); 
		FPlatformProcess::PushDllDirectory(*RootSteamPath);
		SteamDLLHandle = FPlatformProcess::GetDllHandle(*(RootSteamPath + "steam_api.dll"));
		FPlatformProcess::PopDllDirectory(*RootSteamPath);
	#endif
#elif PLATFORM_MAC
	SteamDLLHandle = FPlatformProcess::GetDllHandle(TEXT("libsteam_api.dylib"));
#endif	//PLATFORM_WINDOWS

	if (!SteamDLLHandle)
	{
		UE_LOG(LogHMD, Warning, TEXT("Failed to load Steam library."));
		return false;
	}

	return true;
}

void FSteamVRHMD::UnloadSteamModule()
{
	if (SteamDLLHandle != nullptr)
	{
		FPlatformProcess::FreeDllHandle(SteamDLLHandle);
		SteamDLLHandle = nullptr;
	}
}

#endif //STEAMVR_SUPPORTED_PLATFORMS
