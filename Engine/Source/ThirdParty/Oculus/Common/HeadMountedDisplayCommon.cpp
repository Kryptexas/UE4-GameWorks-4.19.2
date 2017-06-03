// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "HeadMountedDisplayCommon.h"
#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "PostProcess/PostProcessHMD.h"
#include "ScreenRendering.h"
#include "AsyncLoadingSplash.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/PlayerController.h"
#include "DrawDebugHelpers.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "PipelineStateCache.h"

#if OCULUS_STRESS_TESTS_ENABLED
#include "OculusStressTests.h"
#endif

FHMDSettings::FHMDSettings() :
	InterpupillaryDistance(0.064f)
	, UserDistanceToScreenModifier(0.f)
	, NearClippingPlane(0)
	, FarClippingPlane(0)
	, BaseOffset(0, 0, 0)
	, BaseOrientation(FQuat::Identity)
	, PositionOffset(FVector::ZeroVector)
{
	Flags.Raw = 0;
	Flags.bHMDEnabled = true;
	Flags.bChromaAbCorrectionEnabled = true;
	Flags.bUpdateOnRT = true;
	Flags.bHQBuffer = false;
	Flags.bPlayerControllerFollowsHmd = true;
	Flags.bPlayerCameraManagerFollowsHmdOrientation = true;
	Flags.bPlayerCameraManagerFollowsHmdPosition = true;
	EyeRenderViewport[0] = EyeRenderViewport[1] = EyeRenderViewport[2] = FIntRect(0, 0, 0, 0);

}

TSharedPtr<FHMDSettings, ESPMode::ThreadSafe> FHMDSettings::Clone() const
{
	TSharedPtr<FHMDSettings, ESPMode::ThreadSafe> NewSettings = MakeShareable(new FHMDSettings(*this));
	return NewSettings;
}

void FHMDSettings::SetEyeRenderViewport(int OneEyeVPw, int OneEyeVPh)
{
	EyeRenderViewport[0].Min = FIntPoint(0, 0);
	EyeRenderViewport[0].Max = FIntPoint(OneEyeVPw, OneEyeVPh);
	EyeRenderViewport[1].Min = FIntPoint(OneEyeVPw, 0);
	EyeRenderViewport[1].Max = FIntPoint(OneEyeVPw*2, OneEyeVPh);
}

FIntPoint FHMDSettings::GetTextureSize() const 
{ 
	return FIntPoint(EyeRenderViewport[1].Min.X + EyeRenderViewport[1].Size().X, 
					 EyeRenderViewport[1].Min.Y + EyeRenderViewport[1].Size().Y); 
}

//////////////////////////////////////////////////////////////////////////

FHMDGameFrame::FHMDGameFrame() :
	FrameNumber(0),
	MonoCullingDistance(0.0f),
	WorldToMetersScaleWhileInFrame(100.f)
{
	LastHmdOrientation = FQuat::Identity;
	LastHmdPosition = FVector::ZeroVector;
	CameraScale3D = FVector(1.0f, 1.0f, 1.0f);
	ViewportSize = FIntPoint(0,0);
	PlayerLocation = FVector::ZeroVector;
	PlayerOrientation = FQuat::Identity;
	Flags.Raw = 0;
}

TSharedPtr<FHMDGameFrame, ESPMode::ThreadSafe> FHMDGameFrame::Clone() const
{
	TSharedPtr<FHMDGameFrame, ESPMode::ThreadSafe> NewFrame = MakeShareable(new FHMDGameFrame(*this));
	return NewFrame;
}


float FHMDGameFrame::GetWorldToMetersScale() const
{
	if( Flags.bOutOfFrame && IsInGameThread() && GWorld != nullptr )
	{
		// We're not currently rendering a frame, so just use whatever world to meters the main world is using.
		// This can happen when we're polling input in the main engine loop, before ticking any worlds.
#if WITH_EDITOR
		// Workaround to allow WorldToMeters scaling to work correctly for controllers while running inside PIE.
		// The main world will most likely not be pointing at the PIE world while polling input, so if we find a world context 
		// of that type, use that world's WorldToMeters instead.
		if (GIsEditor)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.WorldType == EWorldType::PIE)
				{
					return Context.World()->GetWorldSettings()->WorldToMeters;
				}
			}
		}
#endif //WITH_EDITOR
		return GWorld->GetWorldSettings()->WorldToMeters;
	}

	return WorldToMetersScaleWhileInFrame;
}


void FHMDGameFrame::SetWorldToMetersScale( const float NewWorldToMetersScale )
{
	WorldToMetersScaleWhileInFrame = NewWorldToMetersScale;
}


//////////////////////////////////////////////////////////////////////////
FHMDViewExtension::FHMDViewExtension(FHeadMountedDisplay* InDelegate)
	: Delegate(InDelegate)
{
}

FHMDViewExtension::~FHMDViewExtension()
{
}

void FHMDViewExtension::SetupViewFamily(FSceneViewFamily& InViewFamily)
{
	check(IsInGameThread());

	Delegate->SetupViewFamily(InViewFamily);
}
void FHMDViewExtension::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
	check(IsInGameThread());

	Delegate->SetupView(InViewFamily, InView);
}

void FHMDViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
	check(IsInGameThread());

	auto InRenderFrame = Delegate->PassRenderFrameOwnership();

	// saving only a pointer to the frame should be fine, since the game thread doesn't suppose to modify it anymore.
	RenderFrame = InRenderFrame;
}

void FHMDViewExtension::PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily)
{
	check(IsInRenderingThread());
}

void FHMDViewExtension::PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView)
{
	check(IsInRenderingThread());
}

void FHMDViewExtension::PostRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily)
{
	FHMDViewExtension& RenderContext = *this;

	FHeadMountedDisplay* HeadMountedDisplay = static_cast<FHeadMountedDisplay*>(RenderContext.Delegate);

	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);

	// PokeAHole not supported yet for direct-multiview.
	static const auto CVarMobileMultiViewDirect = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("vr.MobileMultiView.Direct"));
	const bool bIsMobileMultiViewDirectEnabled = (CVarMobileMultiViewDirect && CVarMobileMultiViewDirect->GetValueOnAnyThread() != 0);
	if (bIsMobileMultiViewDirectEnabled)
	{
		return;
	}

	SetRenderTarget(RHICmdList, InViewFamily.RenderTarget->GetRenderTargetTexture(), SceneContext.GetSceneDepthTexture());

	FHMDLayerManager *LayerMgr = HeadMountedDisplay->GetLayerManager();
	LayerMgr->PokeAHole(RHICmdList, RenderContext.RenderFrame.Get(), HeadMountedDisplay->GetRendererModule(), InViewFamily);
	check(IsInRenderingThread());
}

////////////////////////////////////////////////////////////////////////////
FHeadMountedDisplay::FHeadMountedDisplay()
	: CommonConsoleCommands(this)
{
	Flags.Raw = 0;
	Flags.bNeedUpdateStereoRenderingParams = true;
	DeltaControlRotation = FRotator::ZeroRotator;  // used from ApplyHmdRotation

#if !UE_BUILD_SHIPPING
	SideOfSingleCubeInMeters = 0;
	SeaOfCubesVolumeSizeInMeters = 0;
	NumberOfCubesInOneSide = 0;
	CenterOffsetInMeters = FVector::ZeroVector; // offset from the center of 'sea of cubes'
#endif

	CurrentFrameNumber.Set(1);
}

FHeadMountedDisplay::~FHeadMountedDisplay()
{
}

bool FHeadMountedDisplay::IsInitialized() const
{
	return (Settings->Flags.InitStatus & FHMDSettings::eInitialized) != 0;
}

TSharedPtr<ISceneViewExtension, ESPMode::ThreadSafe> FHeadMountedDisplay::GetViewExtension()
{
	TSharedPtr<FHMDViewExtension, ESPMode::ThreadSafe> Result(MakeShareable(new FHMDViewExtension(this)));
	return Result;
}

TSharedPtr<FHMDGameFrame, ESPMode::ThreadSafe> FHeadMountedDisplay::PassRenderFrameOwnership()
{
	TSharedPtr<FHMDGameFrame, ESPMode::ThreadSafe> RenFr = RenderFrame;
	RenderFrame = nullptr;
	return RenFr;
}

TSharedPtr<FHMDGameFrame, ESPMode::ThreadSafe> FHeadMountedDisplay::CreateNewGameFrame() const
{
	TSharedPtr<FHMDGameFrame, ESPMode::ThreadSafe> Result(MakeShareable(new FHMDGameFrame()));
	return Result;
}

TSharedPtr<FHMDSettings, ESPMode::ThreadSafe> FHeadMountedDisplay::CreateNewSettings() const
{
	TSharedPtr<FHMDSettings, ESPMode::ThreadSafe> Result(MakeShareable(new FHMDSettings()));
	return Result;
}

FHMDGameFrame* FHeadMountedDisplay::GetCurrentFrame() const
{
	//UE_LOG(LogHMD, Log, TEXT("Getting current frame, frame number %d"), int(CurrentFrameNumber.GetValue()));
	if (!Frame.IsValid())
	{
		//UE_LOG(LogHMD, Log, TEXT("		----- frame is null!!!"));
		return nullptr;
	}

	check(!IsInActualRenderingThread());
	// A special case, when the game frame is over but rendering hasn't started yet
	if (Frame->Flags.bOutOfFrame && RenderFrame.IsValid() && RenderFrame->Flags.bOutOfFrame)
	{
		//UE_LOG(LogHMD, Log, TEXT("		+++++ render frame, %d"), RenderFrame->FrameNumber);
		return RenderFrame.Get();
	}
	// Technically speaking, we should return the frame only if frame counters are equal.
	// However, there are some calls UE makes from outside of the frame (for example, when 
	// switching to/from fullscreen), thus, returning the previous frame in this case.
	if (Frame->FrameNumber == CurrentFrameNumber.GetValue() || Frame->Flags.bOutOfFrame)
	{
		//UE_LOG(LogHMD, Log, TEXT("		+++++ game frame, %d"), Frame->FrameNumber);
		return Frame.Get();
	}
	//UE_LOG(LogHMD, Log, TEXT("		----- no frame found!"));
	return nullptr;
}

bool FHeadMountedDisplay::OnStartGameFrame(FWorldContext& WorldContext)
{
	check(IsInGameThread());

#if OCULUS_STRESS_TESTS_ENABLED
	FOculusStressTester::TickCPU_GameThread(this);
#endif

	if( !WorldContext.World() || ( !( GEnableVREditorHacks && WorldContext.WorldType == EWorldType::Editor ) && !WorldContext.World()->IsGameWorld() ) )	// @todo vreditor: (Also see OnEndGameFrame()) Kind of a hack here so we can use VR in editor viewports.  We need to consider when running GameWorld viewports inside the editor with VR.
	{
		// ignore all non-game worlds
		return false;
	}

	Frame.Reset();
	Flags.bFrameStarted = true;

	bool bStereoEnabled = Settings->Flags.bStereoEnabled;
	bool bStereoDesired = bStereoEnabled;

	if(Flags.bNeedEnableStereo)
	{
		bStereoDesired = true;
	}

	if(bStereoDesired && (Flags.bNeedDisableStereo || !Settings->Flags.bHMDEnabled))
	{
		bStereoDesired = false;
	}

	bool bStereoDesiredAndIsConnected = bStereoDesired;

	if(bStereoDesired && !(bStereoEnabled ? IsHMDActive() : IsHMDConnected()))
	{
		bStereoDesiredAndIsConnected = false;
	}

	Flags.bNeedEnableStereo = false;
	Flags.bNeedDisableStereo = false;

	if(bStereoEnabled != bStereoDesiredAndIsConnected)
	{
		bStereoEnabled = DoEnableStereo(bStereoDesiredAndIsConnected);
	}

	// Keep trying to enable stereo until we succeed
	Flags.bNeedEnableStereo = bStereoDesired && !bStereoEnabled;

	if (!Settings->IsStereoEnabled() && !Settings->Flags.bHeadTrackingEnforced)
	{
		return false;
	}

	if (Flags.bNeedUpdateDistortionCaps)
	{
		UpdateDistortionCaps();
	}

	if (Flags.bApplySystemOverridesOnStereo)
	{
		ApplySystemOverridesOnStereo();
		Flags.bApplySystemOverridesOnStereo = false;
	}

	if (Flags.bNeedUpdateStereoRenderingParams)
	{
		UpdateStereoRenderingParams();
	}

	auto CurrentFrame = CreateNewGameFrame();
	Frame = CurrentFrame;

	CurrentFrame->FrameNumber = (uint32)CurrentFrameNumber.Increment();
	CurrentFrame->Flags.bOutOfFrame = false;

	CurrentFrame->SetWorldToMetersScale( WorldContext.World()->GetWorldSettings()->WorldToMeters );

	return true;
}

bool FHeadMountedDisplay::OnEndGameFrame(FWorldContext& WorldContext)
{
	check(IsInGameThread());

	Flags.bFrameStarted = false;

	FHMDGameFrame* const CurrentGameFrame = Frame.Get();

	if( !WorldContext.World() || ( !( GEnableVREditorHacks && WorldContext.WorldType == EWorldType::Editor ) && !WorldContext.World()->IsGameWorld() ) || !CurrentGameFrame )
	{
		// ignore all non-game worlds
		return false;
	}

	CurrentGameFrame->Flags.bOutOfFrame = true;

	if (!CurrentGameFrame->Settings->IsStereoEnabled() && !CurrentGameFrame->Settings->Flags.bHeadTrackingEnforced)
	{
		return false;
	}
	// make sure there was only one game worlds.
	//	FGameFrame* CurrentRenderFrame = GetRenderFrame();
	//@todo hmd	check(CurrentRenderFrame->FrameNumber != CurrentGameFrame->FrameNumber);

	// make sure end frame matches the start
	check(CurrentFrameNumber.GetValue() == CurrentGameFrame->FrameNumber);

	if (CurrentFrameNumber.GetValue() == CurrentGameFrame->FrameNumber)
	{
		RenderFrame = Frame;
	}
	else
	{
		RenderFrame.Reset();
	}
	return true;
}

void FHeadMountedDisplay::CreateAndInitNewGameFrame(const AWorldSettings* WorldSettings)
{
	TSharedPtr<FHMDGameFrame, ESPMode::ThreadSafe> CurrentFrame = CreateNewGameFrame();
	CurrentFrame->Settings = GetSettings()->Clone();
	Frame = CurrentFrame;

	CurrentFrame->FrameNumber = CurrentFrameNumber.GetValue();
	CurrentFrame->Flags.bOutOfFrame = false;

	if (WorldSettings)
	{
		CurrentFrame->SetWorldToMetersScale(WorldSettings->WorldToMeters);
	}
	else
	{
		CurrentFrame->SetWorldToMetersScale(100.f);
	}
}

bool FHeadMountedDisplay::IsHMDEnabled() const
{
	return (Settings->Flags.bHMDEnabled);
}

void FHeadMountedDisplay::EnableHMD(bool enable)
{
	Settings->Flags.bHMDEnabled = enable;
	if (!Settings->Flags.bHMDEnabled)
	{
		EnableStereo(false);
	}
}

bool FHeadMountedDisplay::DoesSupportPositionalTracking() const
{
	return false;
}

bool FHeadMountedDisplay::HasValidTrackingPosition()
{
	return false;
}

void FHeadMountedDisplay::RebaseObjectOrientationAndPosition(FVector& OutPosition, FQuat& OutOrientation) const
{
}

float FHeadMountedDisplay::GetInterpupillaryDistance() const
{
	return Settings->InterpupillaryDistance;
}

void FHeadMountedDisplay::SetInterpupillaryDistance(float NewInterpupillaryDistance)
{
	Settings->InterpupillaryDistance = NewInterpupillaryDistance;
	UpdateStereoRenderingParams();
}

bool FHeadMountedDisplay::IsChromaAbCorrectionEnabled() const
{
	const auto frame = GetCurrentFrame();
	return (frame && frame->Settings->Flags.bChromaAbCorrectionEnabled);
}

bool FHeadMountedDisplay::IsPositionalTrackingEnabled() const
{
	return false;
}

bool FHeadMountedDisplay::EnableStereo(bool bStereo)
{
	return DoEnableStereo(bStereo);
}

bool FHeadMountedDisplay::IsStereoEnabled() const
{
	if (IsInGameThread())
	{
		const auto frame = GetCurrentFrame();
		if (frame && frame->Settings.IsValid())
		{
			return (frame->Settings->IsStereoEnabled());
		}
		else
		{
			// If IsStereoEnabled is called when a game frame hasn't started, then always return false.
			// In the case when you need to check if stereo is GOING TO be enabled in next frame,
			// use explicit call to IsStereoEnabledOnNextFrame()
			return false;
		}
	}
	else
	{
		return false;
	}
}

bool FHeadMountedDisplay::IsStereoEnabledOnNextFrame() const
{
	return Settings->IsStereoEnabled();
}

void FHeadMountedDisplay::AdjustViewRect(EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const
{
	SizeX = SizeX / 2;
	if (StereoPass == eSSP_RIGHT_EYE)
	{
		X += SizeX;
	}
}

void FHeadMountedDisplay::SetClippingPlanes(float NCP, float FCP)
{
	Settings->NearClippingPlane = NCP;
	Settings->FarClippingPlane = FCP;
	Settings->Flags.bClippingPlanesOverride = false; // prevents from saving in .ini file
}

void FHeadMountedDisplay::SetBaseRotation(const FRotator& BaseRot)
{
	SetBaseOrientation(BaseRot.Quaternion());
}

FRotator FHeadMountedDisplay::GetBaseRotation() const
{
	return GetBaseOrientation().Rotator();
}

void FHeadMountedDisplay::SetBaseOrientation(const FQuat& BaseOrient)
{
	Settings->BaseOrientation = BaseOrient;
}

FQuat FHeadMountedDisplay::GetBaseOrientation() const
{
	return Settings->BaseOrientation;
}

void FHeadMountedDisplay::SetBaseOffsetInMeters(const FVector& BaseOffset)
{
	Settings->BaseOffset = BaseOffset;
}

FVector FHeadMountedDisplay::GetBaseOffsetInMeters() const
{
	return Settings->BaseOffset;
}

bool FHeadMountedDisplay::IsHeadTrackingAllowed() const
{
	const auto frame = GetCurrentFrame();
	return (frame && frame->Settings.IsValid() && (frame->Settings->Flags.bHeadTrackingEnforced || frame->Settings->IsStereoEnabled()));
}

void FHeadMountedDisplay::ResetStereoRenderingParams()
{
	Flags.bNeedUpdateStereoRenderingParams = true;
	Settings->Flags.bOverrideIPD = false;
	Settings->NearClippingPlane = Settings->FarClippingPlane = 0.f;
	Settings->Flags.bClippingPlanesOverride = true; // forces zeros to be written to ini file to use default values next run
}

void FHeadMountedDisplay::ResetControlRotation() const
{
	// Switching back to non-stereo mode: reset player rotation and aim.
	// Should we go through all playercontrollers here?
	APlayerController* pc = GEngine->GetFirstLocalPlayerController(GWorld);
	if (pc)
	{
		// Reset Aim? @todo
		FRotator r = pc->GetControlRotation();
		r.Normalize();
		// Reset roll and pitch of the player
		r.Roll = 0;
		r.Pitch = 0;
		pc->SetControlRotation(r);
	}
}

void FHeadMountedDisplay::GetCurrentHMDPose(FQuat& CurrentOrientation, FVector& CurrentPosition,
	bool bUseOrienationForPlayerCamera, bool bUsePositionForPlayerCamera, const FVector& PositionScale)
{
	// only supposed to be used from the game thread
	check(!IsInActualRenderingThread());
	auto frame = GetCurrentFrame();
	if (!frame)
	{
		CurrentOrientation = FQuat::Identity;
		CurrentPosition = FVector::ZeroVector;
		return;
	}
	if (PositionScale != FVector::ZeroVector)
	{
		frame->CameraScale3D = PositionScale;
	}
	GetCurrentPose(CurrentOrientation, CurrentPosition, bUseOrienationForPlayerCamera, bUsePositionForPlayerCamera);
	if (bUseOrienationForPlayerCamera)
	{
		frame->LastHmdOrientation = CurrentOrientation;
		frame->Flags.bOrientationChanged = bUseOrienationForPlayerCamera;
	}
	if (bUsePositionForPlayerCamera)
	{
		frame->LastHmdPosition = CurrentPosition;
		frame->Flags.bPositionChanged = bUsePositionForPlayerCamera;
	}
}

void FHeadMountedDisplay::GetCurrentOrientationAndPosition(FQuat& CurrentOrientation, FVector& CurrentPosition)
{
	GetCurrentHMDPose(CurrentOrientation, CurrentPosition, false, false, FVector::ZeroVector);
}

FVector FHeadMountedDisplay::GetNeckPosition(const FQuat& CurrentOrientation, const FVector& CurrentPosition, const FVector& PositionScale)
{
	const auto frame = GetCurrentFrame();
	if (!frame)
	{
		return FVector::ZeroVector;
	}
	FVector UnrotatedPos = CurrentOrientation.Inverse().RotateVector(CurrentPosition);
	UnrotatedPos.X -= frame->Settings->NeckToEyeInMeters.X * frame->GetWorldToMetersScale();
	UnrotatedPos.Z -= frame->Settings->NeckToEyeInMeters.Y * frame->GetWorldToMetersScale();
	return UnrotatedPos;
}

void FHeadMountedDisplay::ApplyHmdRotation(APlayerController* PC, FRotator& ViewRotation)
{
	auto frame = GetCurrentFrame();
	if (!frame)
	{
		return;
	}
	if( !frame->Settings->Flags.bPlayerControllerFollowsHmd )
	{
		return;
	}
#if !UE_BUILD_SHIPPING
	if (frame->Settings->Flags.bDoNotUpdateOnGT)
		return;
#endif

	ViewRotation.Normalize();

	FQuat CurHmdOrientation;
	FVector CurHmdPosition;
	GetCurrentPose(CurHmdOrientation, CurHmdPosition, true, true);
	frame->LastHmdOrientation = CurHmdOrientation;

	const FRotator DeltaRot = ViewRotation - PC->GetControlRotation();
	DeltaControlRotation = (DeltaControlRotation + DeltaRot).GetNormalized();

	// Pitch from other sources is never good, because there is an absolute up and down that must be respected to avoid motion sickness.
	// Same with roll.
	DeltaControlRotation.Pitch = 0;
	DeltaControlRotation.Roll = 0;
	const FQuat DeltaControlOrientation = DeltaControlRotation.Quaternion();

	ViewRotation = FRotator(DeltaControlOrientation * CurHmdOrientation);

	frame->Flags.bPlayerControllerFollowsHmd = true;
	frame->Flags.bOrientationChanged = true;
	frame->Flags.bPositionChanged = true;
}

bool FHeadMountedDisplay::UpdatePlayerCamera(FQuat& CurrentOrientation, FVector& CurrentPosition)
{
	auto frame = GetCurrentFrame();
	if (!frame)
	{
		return false;
	}

#if !UE_BUILD_SHIPPING
	if (frame->Settings->Flags.bDoNotUpdateOnGT)
	{
		frame->Flags.bOrientationChanged = frame->Settings->Flags.bPlayerCameraManagerFollowsHmdOrientation;// POV.bFollowHmdOrientation;
		frame->Flags.bPositionChanged = frame->Settings->Flags.bPlayerCameraManagerFollowsHmdPosition;//POV.bFollowHmdPosition;
		return false;
	}
#endif
	GetCurrentPose(CurrentOrientation, CurrentPosition, frame->Settings->Flags.bPlayerCameraManagerFollowsHmdOrientation, frame->Settings->Flags.bPlayerCameraManagerFollowsHmdPosition);

	if (frame->Settings->Flags.bPlayerCameraManagerFollowsHmdOrientation) //POV.bFollowHmdOrientation
	{
		frame->LastHmdOrientation = CurrentOrientation;
		frame->Flags.bOrientationChanged = true;
	}

	if (frame->Settings->Flags.bPlayerCameraManagerFollowsHmdPosition) // POV.bFollowHmdPosition
	{
		frame->LastHmdPosition = CurrentPosition;
		frame->Flags.bPositionChanged = true;
	}

	return true;
}

static FHMDLayerManager::LayerOriginType ConvertLayerPositionType(IStereoLayers::ELayerType InLayerPositionType)
{
	switch (InLayerPositionType)
	{
	case IStereoLayers::WorldLocked:
		return FHMDLayerManager::Layer_WorldLocked;
	case IStereoLayers::TrackerLocked:
		return FHMDLayerManager::Layer_TorsoLocked;
	case IStereoLayers::FaceLocked:
		return FHMDLayerManager::Layer_HeadLocked;
	default:
		return FHMDLayerManager::Layer_UnknownOrigin;
	};
}

static FHMDLayerDesc::ELayerTypeMask ConvertLayerType(IStereoLayers::ELayerShape InLayerType)
{
	switch (InLayerType)
	{
	case IStereoLayers::QuadLayer:
		return FHMDLayerDesc::Quad;
	case IStereoLayers::CylinderLayer:
		return FHMDLayerDesc::Cylinder;
	case IStereoLayers::CubemapLayer:
		return FHMDLayerDesc::Cubemap;
	default:
		return FHMDLayerDesc::Quad;
	};
}

uint32 FHeadMountedDisplay::CreateLayer(const IStereoLayers::FLayerDesc& InLayerDesc)
{
	FHMDLayerManager* pLayerMgr = GetLayerManager();
	if (pLayerMgr)
	{
		if (InLayerDesc.Texture)
		{
			FScopeLock ScopeLock(&pLayerMgr->LayersLock);
			uint32 id = 0;
			pLayerMgr->AddLayer(ConvertLayerType(InLayerDesc.ShapeType), InLayerDesc.Priority, ConvertLayerPositionType(InLayerDesc.PositionType), id);
			if (id != 0)
			{
				SetLayerDesc(id, InLayerDesc);
			}
			return id;
		}
		else
		{
			// non quad/texture layers are not supported yet
			check(0);
		}
	}
	return 0;
}

void FHeadMountedDisplay::DestroyLayer(uint32 LayerId)
{
	if (LayerId > 0)
	{
		FHMDLayerManager* pLayerMgr = GetLayerManager();
		if (pLayerMgr)
		{
			pLayerMgr->RemoveLayer(LayerId);
		}
	}
}

void FHeadMountedDisplay::SetLayerDesc(uint32 LayerId, const IStereoLayers::FLayerDesc& InLayerDesc)
{
	if (LayerId == 0)
	{
		return;
	}

	FHMDLayerManager* pLayerMgr = GetLayerManager();
	if (!pLayerMgr)
	{
		return;
	}

	const FHMDLayerDesc* pLayer = pLayerMgr->GetLayerDesc(LayerId);
	if (pLayer && (pLayer->GetType() == FHMDLayerDesc::Quad || pLayer->GetType() == FHMDLayerDesc::Cylinder || pLayer->GetType() == FHMDLayerDesc::Cubemap))
	{
		FVector2D NewQuadSize = InLayerDesc.QuadSize;
		float NewCylinderHeight = InLayerDesc.CylinderHeight;
		if (InLayerDesc.Flags & IStereoLayers::LAYER_FLAG_QUAD_PRESERVE_TEX_RATIO && InLayerDesc.Texture.IsValid())
		{
			FRHITexture2D* Texture2D = InLayerDesc.Texture->GetTexture2D();
			if (Texture2D)
			{
				const float TexRatio = (Texture2D->GetSizeY() == 0) ? 1280.0f/720.0f : (float)Texture2D->GetSizeX() / (float)Texture2D->GetSizeY();
				NewCylinderHeight = (TexRatio) ? InLayerDesc.CylinderSize.Y / TexRatio : NewCylinderHeight;
				NewQuadSize.Y = (TexRatio) ? NewQuadSize.X / TexRatio : NewQuadSize.Y;
			}
		}

		FHMDLayerDesc Layer = *pLayer;
		Layer.SetFlags(InLayerDesc.Flags);
		Layer.SetLockedToHead(InLayerDesc.PositionType == IStereoLayers::ELayerType::FaceLocked);
		Layer.SetLockedToTorso(InLayerDesc.PositionType == IStereoLayers::ELayerType::TrackerLocked);
		Layer.SetTexture(InLayerDesc.Texture);
		Layer.SetLeftTexture(InLayerDesc.LeftTexture);
		Layer.SetQuadSize(NewQuadSize);
		Layer.SetCylinderSize(InLayerDesc.CylinderSize);
		Layer.SetCylinderHeight(NewCylinderHeight);
		Layer.SetTransform(InLayerDesc.Transform);
		Layer.ResetChangedFlags();
		Layer.SetTextureViewport(InLayerDesc.UVRect);
		pLayerMgr->UpdateLayer(Layer);
	}
}

bool FHeadMountedDisplay::GetLayerDesc(uint32 LayerId, IStereoLayers::FLayerDesc& OutLayerDesc)
{
	if (LayerId == 0)
	{
		return false;
	}

	const FHMDLayerManager* pLayerMgr = GetLayerManager();
	if (!pLayerMgr)
	{
		return false;
	}

	const FHMDLayerDesc* pLayer = pLayerMgr->GetLayerDesc(LayerId);
	if (!pLayer)
	{
		return false;
	}

	FHMDLayerDesc Layer = *pLayer;
	OutLayerDesc.Priority = Layer.GetPriority();
	OutLayerDesc.QuadSize = Layer.GetQuadSize();
	OutLayerDesc.Texture = Layer.GetTexture();
	OutLayerDesc.LeftTexture = Layer.GetLeftTexture();
	OutLayerDesc.Transform = Layer.GetTransform();
	OutLayerDesc.UVRect = Layer.GetTextureViewport();
	OutLayerDesc.CylinderSize = Layer.GetCylinderSize();
	OutLayerDesc.CylinderHeight = Layer.GetCylinderHeight();

	if (Layer.IsHeadLocked())
	{
		OutLayerDesc.PositionType = IStereoLayers::FaceLocked;
	}
	else if (Layer.IsTorsoLocked())
	{
		OutLayerDesc.PositionType = IStereoLayers::TrackerLocked;
	}
	else
	{
		OutLayerDesc.PositionType = IStereoLayers::WorldLocked;
	}

	switch (pLayer->GetType())
	{
	case FHMDLayerDesc::Quad:
		OutLayerDesc.ShapeType = IStereoLayers::QuadLayer;
		break;

	case FHMDLayerDesc::Cylinder:
		OutLayerDesc.ShapeType = IStereoLayers::CylinderLayer;
		break;
	case FHMDLayerDesc::Cubemap:
		OutLayerDesc.ShapeType = IStereoLayers::CubemapLayer;
	default:
		return false;
		break;
	}

	return true;
}

void FHeadMountedDisplay::MarkTextureForUpdate(uint32 LayerId)
{
	if (LayerId == 0)
	{
		return;
	}

	const FHMDLayerManager* pLayerMgr = GetLayerManager();
	if (!pLayerMgr)
	{
		return;
	}

	const FHMDLayerDesc* pLayer = pLayerMgr->GetLayerDesc(LayerId);
	if (!pLayer)
	{
		return;
	}

	pLayer->MarkTextureChanged();
}

void FHeadMountedDisplay::UpdateSplashScreen()
{
	FAsyncLoadingSplash* Splash = GetAsyncLoadingSplash();
	if (!Splash)
	{
		return;
	}

	FTexture2DRHIRef Texture2D = (bSplashShowMovie && SplashMovie.IsValid()) ? SplashMovie : SplashTexture;
	FTextureRHIRef Texture;
	float InvAspectRatio = 1.0;
	if (Texture2D.IsValid())
	{
		Texture = (FRHITexture*)Texture2D.GetReference();
		const FIntPoint TextureSize = Texture2D->GetSizeXY();
		if (TextureSize.X > 0)
		{
			InvAspectRatio = float(TextureSize.Y) / float(TextureSize.X);
		}
	}

	// Disable features incompatible with the generalized VR splash screen
	Splash->SetAutoShow(false);
	Splash->SetLoadingIconMode(false);
	
	if (bSplashIsShown && Texture.IsValid())
	{
		if (SplashLayerHandle)
		{
			FAsyncLoadingSplash::FSplashDesc CurrentDesc;
			Splash->GetSplash(0, CurrentDesc);
			CurrentDesc.LoadedTexture = Texture;
			CurrentDesc.TextureOffset = SplashOffset;
			CurrentDesc.TextureScale = SplashScale;
		}
		else
		{
			Splash->ClearSplashes();

			FAsyncLoadingSplash::FSplashDesc NewDesc;
			NewDesc.LoadedTexture = Texture;
			// Set texture size to 8m wide, keeping the aspect ratio.
			NewDesc.QuadSizeInMeters = FVector2D(8.0f, 8.0f * InvAspectRatio);

			FTransform Translation(FVector(5.0f, 0.0f, 0.0f));

			FHMDGameFrame* CurrentFrame = GetCurrentFrame();
			FRotator Rotation(CurrentFrame->LastHmdOrientation);
			Rotation.Pitch = 0.0f;
			Rotation.Roll = 0.0f;

			NewDesc.TransformInMeters = Translation * FTransform(Rotation.Quaternion());

			NewDesc.TextureOffset = SplashOffset;
			NewDesc.TextureScale = SplashScale;
			NewDesc.bNoAlphaChannel = true;
			Splash->AddSplash(NewDesc);

			Splash->Show(FAsyncLoadingSplash::EShowType::ShowManually);

			SplashLayerHandle = 1;
		}
	}
	else
	{
		if (SplashLayerHandle)
		{
			Splash->Hide(FAsyncLoadingSplash::EShowType::ShowManually);
			Splash->ClearSplashes();
			SplashLayerHandle = 0;
		}
	}
}

void FHeadMountedDisplay::QuantizeBufferSize(int32& InOutBufferSizeX, int32& InOutBufferSizeY, uint32 DividableBy)
{
	const uint32 Mask = ~(DividableBy - 1);
	InOutBufferSizeX = (InOutBufferSizeX + DividableBy - 1) & Mask;
	InOutBufferSizeY = (InOutBufferSizeY + DividableBy - 1) & Mask;
}

/************************************************************************/
/* Layers                                                               */
/************************************************************************/
FHMDLayerDesc::FHMDLayerDesc(class FHMDLayerManager& InLayerMgr, ELayerTypeMask InType, uint32 InPriority, uint32 InID) :
	LayerManager(InLayerMgr)
	, Id(InID | InType)
	, Texture(nullptr)
	, LeftTexture(nullptr)
	, Flags(0)
	, TextureUV(ForceInit)
	, QuadSize(FVector2D::ZeroVector)
	, CylinderHeight(0)
	, CylinderSize(FVector2D::ZeroVector)
	, Priority(InPriority & IdMask)
	, bHighQuality(true)
	, bHeadLocked(false)
	, bTorsoLocked(false)
	, bTextureHasChanged(true)
	, bTransformHasChanged(true)
	, bNewLayer(true)
	, bAlreadyAdded(false)
{
	TextureUV.Min = FVector2D(0, 0);
	TextureUV.Max = FVector2D(1, 1);
}

void FHMDLayerDesc::SetTransform(const FTransform& InTrn)
{
	Transform = InTrn;
	bTransformHasChanged = true;
	LayerManager.SetDirty();
}

void FHMDLayerDesc::SetQuadSize(const FVector2D& InSize)
{
	if (QuadSize == InSize)
	{
		return;
	}

	QuadSize = InSize;
	bTransformHasChanged = true;
	LayerManager.SetDirty();
}

void FHMDLayerDesc::SetCylinderSize(const FVector2D& InSize)
{
	if (CylinderSize == InSize)
	{
		return;
	}

	CylinderSize = InSize;
	bTransformHasChanged = true;
	LayerManager.SetDirty();
}

void FHMDLayerDesc::SetCylinderHeight(const float InHeight)
{
	if (CylinderHeight == InHeight)
	{
		return;
	}

	CylinderHeight = InHeight;
	bTransformHasChanged = true;
	LayerManager.SetDirty();
}

void FHMDLayerDesc::SetTexture(FTextureRHIRef InTexture)
{
	if (Texture == InTexture)
	{
		return;
	}

	Texture = InTexture;
	bTextureHasChanged = true;
	LayerManager.SetDirty();
}

void FHMDLayerDesc::SetLeftTexture(FTextureRHIRef InTexture)
{
	if (LeftTexture == InTexture)
	{
		return;
	}

	LeftTexture = InTexture;
	bTextureHasChanged = true;
	LayerManager.SetDirty();
}

void FHMDLayerDesc::SetTextureSet(const FTextureSetProxyPtr& InTextureSet)
{
	TextureSet = InTextureSet;
	bTextureHasChanged = true;
	LayerManager.SetDirty();
}

void FHMDLayerDesc::SetTextureViewport(const FBox2D& InUVRect)
{
	if (!InUVRect.bIsValid)
	{
		return;
	}

	if (TextureUV == InUVRect && TextureUV.bIsValid)
	{
		return;
	}

	TextureUV = InUVRect;
	bTransformHasChanged = true;
	LayerManager.SetDirty();
}

void FHMDLayerDesc::SetPriority(uint32 InPriority)
{
	if (Priority == InPriority)
	{
		return;
	}

	Priority = InPriority;
	LayerManager.SetDirty();
}

FHMDLayerDesc& FHMDLayerDesc::operator=(const FHMDLayerDesc& InSrc)
{
	check(&LayerManager == &InSrc.LayerManager);
	
	if (Id != InSrc.Id || Priority != InSrc.Priority || bHighQuality != InSrc.bHighQuality || bHeadLocked != InSrc.bHeadLocked || bTorsoLocked != InSrc.bTorsoLocked)
	{
		bNewLayer = true;
		Id = InSrc.Id;
		Priority = InSrc.Priority;
		bHighQuality = InSrc.bHighQuality;
		bHeadLocked = InSrc.bHeadLocked;
		bTorsoLocked = InSrc.bTorsoLocked;
	}
	if (Texture != InSrc.Texture)
	{
		bTextureHasChanged = true;
		Texture = InSrc.Texture;
		LeftTexture = InSrc.LeftTexture;
	}
	if (TextureSet.Get() != InSrc.TextureSet.Get())
	{
		bTextureHasChanged = true;
		TextureSet = InSrc.TextureSet;
	}
	if (!(TextureUV == InSrc.TextureUV) || QuadSize != InSrc.QuadSize || !Transform.Equals(InSrc.Transform) || CylinderSize != InSrc.CylinderSize || CylinderHeight != InSrc.CylinderHeight)
	{
		bTransformHasChanged = true;
		TextureUV = InSrc.TextureUV;
		QuadSize = InSrc.QuadSize;
		CylinderSize = InSrc.CylinderSize;
		CylinderHeight = InSrc.CylinderHeight;
		Transform = InSrc.Transform;
	}
	
	Flags = InSrc.Flags;
	LayerManager.SetDirty();
	return *this;
}

void FHMDLayerDesc::ResetChangedFlags()
{
	bTextureHasChanged = bTransformHasChanged = bNewLayer = bAlreadyAdded = false;
}

//////////////////////////////////////////////////////////////////////////
FHMDRenderLayer::FHMDRenderLayer(FHMDLayerDesc& InLayerInfo) :
	LayerInfo(InLayerInfo)
	, bOwnsTextureSet(true)
{
	if (InLayerInfo.HasTextureSet())
	{
		TransferTextureSet(InLayerInfo);
	}
}

TSharedPtr<FHMDRenderLayer> FHMDRenderLayer::Clone() const
{
	TSharedPtr<FHMDRenderLayer> NewLayer = MakeShareable(new FHMDRenderLayer(*this));
	return NewLayer;
}

void FHMDRenderLayer::ReleaseResources()
{
	if (bOwnsTextureSet && TextureSet.IsValid())
	{
		TextureSet->ReleaseResources();
	}
}

//////////////////////////////////////////////////////////////////////////
FHMDLayerManager::FHMDLayerManager() : 
	CurrentId(0)
	, bLayersChanged(false)
{
}

FHMDLayerManager::~FHMDLayerManager()
{
}

void FHMDLayerManager::Startup()
{
}

void FHMDLayerManager::Shutdown()
{
	{
		FScopeLock ScopeLock(&LayersLock);
		RemoveAllLayers();

		LayersToRender.SetNum(0);

		CurrentId = 0;
		bLayersChanged = false;
	}
}

uint32 FHMDLayerManager::GetTotalNumberOfLayers() const
{
	return EyeLayers.Num() + QuadLayers.Num() + DebugLayers.Num();
}

TSharedPtr<FHMDLayerDesc> 
FHMDLayerManager::AddLayer(FHMDLayerDesc::ELayerTypeMask InType, uint32 InPriority, LayerOriginType InLayerOriginType, uint32& OutLayerId)
{
	if (GetTotalNumberOfLayers() >= GetTotalNumberOfLayersSupported() || !ShouldSupportLayerType(InType))
	{
		return nullptr;
	}
	TSharedPtr<FHMDLayerDesc> NewLayerDesc = MakeShareable(new FHMDLayerDesc(*this, InType, InPriority, ++CurrentId));

	switch (InLayerOriginType)
	{
	case Layer_HeadLocked:
		NewLayerDesc->bHeadLocked = true;
		break;

	case Layer_TorsoLocked:
		NewLayerDesc->bTorsoLocked = true;
		break;

	default: // default is WorldLocked
		break;
	}

	OutLayerId = NewLayerDesc->GetId();

	FScopeLock ScopeLock(&LayersLock);
	TArray<TSharedPtr<FHMDLayerDesc> >& Layers = GetLayersArrayById(OutLayerId);
	Layers.Add(NewLayerDesc);

	bLayersChanged = true;
	return NewLayerDesc;
}

void FHMDLayerManager::RemoveLayer(uint32 LayerId)
{
	if (LayerId != 0)
	{
		FScopeLock ScopeLock(&LayersLock);
		TArray<TSharedPtr<FHMDLayerDesc> >& Layers = GetLayersArrayById(LayerId);
		uint32 idx = FindLayerIndex(Layers, LayerId);
		if (idx != ~0u)
		{
			Layers.RemoveAt(idx);
		}
		bLayersChanged = true;
	}
}

void FHMDLayerManager::RemoveAllLayers()
{
	FScopeLock ScopeLock(&LayersLock);
	EyeLayers.SetNum(0);
	QuadLayers.SetNum(0);
	DebugLayers.SetNum(0);
	bLayersChanged = true;
}

const FHMDLayerDesc* FHMDLayerManager::GetLayerDesc(uint32 LayerId) const
{
	FScopeLock ScopeLock(&LayersLock);
	return FindLayer_NoLock(LayerId).Get();
}

const TArray<TSharedPtr<FHMDLayerDesc> >& FHMDLayerManager::GetLayersArrayById(uint32 LayerId) const
{
	switch (LayerId & FHMDLayerDesc::TypeMask)
	{
	case FHMDLayerDesc::Eye:
		return EyeLayers;
		break;
	case FHMDLayerDesc::Quad:
	case FHMDLayerDesc::Cylinder:
	case FHMDLayerDesc::Cubemap:
		return QuadLayers;
		break;
	case FHMDLayerDesc::Debug:
		return DebugLayers;
		break;
	default:
		checkf(0, TEXT("Invalid layer type %d (id = 0x%X)"), int(LayerId & FHMDLayerDesc::TypeMask), LayerId);
	}
	return EyeLayers;
}

TArray<TSharedPtr<FHMDLayerDesc> >& FHMDLayerManager::GetLayersArrayById(uint32 LayerId) 
{
	switch (LayerId & FHMDLayerDesc::TypeMask)
	{
	case FHMDLayerDesc::Eye:
		return EyeLayers;
		break;
	case FHMDLayerDesc::Quad:
	case FHMDLayerDesc::Cylinder:
	case FHMDLayerDesc::Cubemap:
		return QuadLayers;
		break;
	case FHMDLayerDesc::Debug:
		return DebugLayers;
		break;
	default:
		checkf(0, TEXT("Invalid layer type %d (id = 0x%X)"), int(LayerId & FHMDLayerDesc::TypeMask), LayerId);
	}
	return EyeLayers;
}

// returns ~0u if not found
uint32 FHMDLayerManager::FindLayerIndex(const TArray<TSharedPtr<FHMDLayerDesc> >& Layers, uint32 LayerId)
{
	for (uint32 i = 0, n = Layers.Num(); i < n; ++i)
	{
		if (Layers[i].IsValid() && Layers[i]->GetId() == LayerId)
		{
			return i;
		}
	}
	return ~0u;
}

TSharedPtr<FHMDLayerDesc> FHMDLayerManager::FindLayer_NoLock(uint32 LayerId) const
{
	if (LayerId != 0)
	{
		const TArray<TSharedPtr<FHMDLayerDesc> >& Layers = GetLayersArrayById(LayerId);
		uint32 idx = FindLayerIndex(Layers, LayerId);
		if (idx != ~0u)
		{
			return Layers[idx];
		}
	}
	return nullptr;
}

void FHMDLayerManager::UpdateLayer(const FHMDLayerDesc& InLayerDesc)
{
	FScopeLock ScopeLock(&LayersLock);
	if (InLayerDesc.Id != 0)
	{
		TArray<TSharedPtr<FHMDLayerDesc> >& Layers = GetLayersArrayById(InLayerDesc.GetId());
		uint32 idx = FindLayerIndex(Layers, InLayerDesc.GetId());
		if (idx != ~0u)
		{
			*Layers[idx].Get() = InLayerDesc;
			SetDirty();
		}
	}
}

TSharedPtr<FHMDRenderLayer> FHMDLayerManager::CreateRenderLayer_RenderThread(FHMDLayerDesc& InDesc)
{
	TSharedPtr<FHMDRenderLayer> NewLayer = MakeShareable(new FHMDRenderLayer(InDesc));
	return NewLayer;
}

FHMDRenderLayer* FHMDLayerManager::GetRenderLayer_RenderThread_NoLock(uint32 LayerId)
{
	for (uint32 i = 0, n = LayersToRender.Num(); i < n; ++i)
	{
		auto RenderLayer = LayersToRender[i];
		if (RenderLayer->GetLayerDesc().GetId() == LayerId)
		{
			return RenderLayer.Get();
		}
	}
	return nullptr;
}

void FHMDLayerManager::ReleaseTextureSetsInArray_RenderThread_NoLock(TArray<TSharedPtr<FHMDLayerDesc> >& Layers)
{
	for (int32 i = 0; i < Layers.Num(); ++i)
	{
		if (Layers[i].IsValid())
		{
			FTextureSetProxyPtr TexSet = Layers[i]->GetTextureSet();
			if (TexSet.IsValid())
			{
				TexSet->ReleaseResources();
			}
			Layers[i]->SetTextureSet(nullptr);
		}
	}
}

void FHMDLayerManager::ReleaseTextureSets_RenderThread_NoLock()
{
	ReleaseTextureSetsInArray_RenderThread_NoLock(EyeLayers);
	ReleaseTextureSetsInArray_RenderThread_NoLock(QuadLayers);
	ReleaseTextureSetsInArray_RenderThread_NoLock(DebugLayers);
}

void FHMDLayerManager::PreSubmitUpdate_RenderThread(FRHICommandListImmediate& RHICmdList, const FHMDGameFrame* CurrentFrame, bool ShowFlagsRendering)
{
	check(IsInRenderingThread());

	if (bLayersChanged)
	{
		// If layers were changed then make a new snapshot of layers for rendering.
		// Then sort the array by priority.
		FScopeLock ScopeLock(&LayersLock);

		// go through render layers and check whether the layer is modified or removed
		uint32 NumOfAlreadyAdded = 0;
		for (uint32 i = 0, n = LayersToRender.Num(); i < n; ++i)
		{
			auto RenderLayer = LayersToRender[i];
			TSharedPtr<FHMDLayerDesc> LayerDesc = FindLayer_NoLock(RenderLayer->GetLayerDesc().GetId());
			if (LayerDesc.IsValid() && !LayerDesc->IsNewLayer())
			{
				// the layer has changed. modify the render counterpart accordingly.
				if (LayerDesc->IsTextureChanged())
				{
					RenderLayer->ReleaseResources();
					if (LayerDesc->HasTextureSet())
					{
						check(!LayerDesc->HasTexture());
						RenderLayer->TransferTextureSet(*LayerDesc.Get());
					}
					else if (LayerDesc->HasTexture())
					{
						check(!LayerDesc->HasTextureSet());
					}
				}
				if (LayerDesc->IsTransformChanged())
				{
					RenderLayer->SetLayerDesc(*LayerDesc.Get());
				}
				LayerDesc->bAlreadyAdded = true;
				++NumOfAlreadyAdded;
			}
			else
			{
				// layer desc is not found, deleted, or just added. Release resources, kill the renderlayer.
				RenderLayer->ReleaseResources();
				LayersToRender[i] = nullptr;
			}
		}

		LayersToRender.SetNum(EyeLayers.Num() + QuadLayers.Num() + DebugLayers.Num() + LayersToRender.Num() - NumOfAlreadyAdded);

		for (uint32 i = 0, n = EyeLayers.Num(); i < n; ++i)
		{
			TSharedPtr<FHMDLayerDesc> pLayer = EyeLayers[i];
			check(pLayer.IsValid());
			if (!pLayer->bAlreadyAdded) // add only new layers, not already added to the LayersToRender array
			{
				LayersToRender.Add(CreateRenderLayer_RenderThread(*pLayer.Get()));
			}
			pLayer->ResetChangedFlags();
		}
		for (uint32 i = 0, n = QuadLayers.Num(); i < n; ++i)
		{
			TSharedPtr<FHMDLayerDesc> pLayer = QuadLayers[i];
			check(pLayer.IsValid());
			if (!pLayer->bAlreadyAdded) // add only new layers, not already added to the LayersToRender array
			{
				LayersToRender.Add(CreateRenderLayer_RenderThread(*pLayer.Get()));
			}
		}
		for (uint32 i = 0, n = DebugLayers.Num(); i < n; ++i)
		{
			TSharedPtr<FHMDLayerDesc> pLayer = DebugLayers[i];
			check(pLayer.IsValid());
			if (!pLayer->bAlreadyAdded) // add only new layers, not already added to the LayersToRender array
			{
				LayersToRender.Add(CreateRenderLayer_RenderThread(*pLayer.Get()));
			}
		}

		static const auto CVarMixLayerPriorities = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("vr.StereoLayers.bMixLayerPriorities"));
		const bool bRenderHeadLockedFront = (CVarMixLayerPriorities->GetValueOnRenderThread() == 0) ;

		auto Comparator = [=](const TSharedPtr<FHMDRenderLayer>& l1, const TSharedPtr<FHMDRenderLayer>& l2)
		{
			if (!l1.IsValid())
			{
				return false;
			}
			if (!l2.IsValid())
			{
				return true;
			}
			auto LayerDesc1 = l1->GetLayerDesc();
			auto LayerDesc2 = l2->GetLayerDesc();
			if (LayerDesc1.IsPokeAHole() == LayerDesc2.IsPokeAHole())
			{
				if (LayerDesc1.GetType() == LayerDesc2.GetType())
				{
					if (bRenderHeadLockedFront || LayerDesc1.IsHeadLocked() == LayerDesc2.IsHeadLocked())
					{
						return LayerDesc1.GetPriority() < LayerDesc2.GetPriority();
					}
					else
					{
						return LayerDesc2.IsHeadLocked();
					}
				}
				else
				{
					return bool(LayerDesc1.IsPokeAHole() ^ (LayerDesc1.GetType() < LayerDesc2.GetType()));
				}
			}
			else
			{
				return LayerDesc1.IsPokeAHole();
			}
		};
		LayersToRender.Sort(Comparator);
		// all empty (nullptr) entries should be at the end of the array. 
		// The total number of render layers should be equal to sum of all layers.
		LayersToRender.SetNum(EyeLayers.Num() + QuadLayers.Num() + DebugLayers.Num());
		bLayersChanged = false;
	}
}

static void DrawPokeAHoleQuadMesh(FRHICommandList& RHICmdList, const FMatrix& PosTransform, float X, float Y, float Z, float SizeX, float SizeY, float SizeZ, bool InvertCoords)
{
	float ClipSpaceQuadZ = 0.0f;

	FFilterVertex Vertices[4];
	Vertices[0].Position = PosTransform.TransformFVector4(FVector4(X, Y, Z, 1));
	Vertices[1].Position = PosTransform.TransformFVector4(FVector4(X + SizeX, Y, Z + SizeZ, 1));
	Vertices[2].Position = PosTransform.TransformFVector4(FVector4(X, Y + SizeY, Z, 1));
	Vertices[3].Position = PosTransform.TransformFVector4(FVector4(X + SizeX, Y + SizeY, Z + SizeZ, 1));

	if (InvertCoords)
	{
		Vertices[0].UV = FVector2D(1, 0);
		Vertices[1].UV = FVector2D(1, 1);
		Vertices[2].UV = FVector2D(0, 0);
		Vertices[3].UV = FVector2D(0, 1);
	}
	else
	{
		Vertices[0].UV = FVector2D(0, 1);
		Vertices[1].UV = FVector2D(0, 0);
		Vertices[2].UV = FVector2D(1, 1);
		Vertices[3].UV = FVector2D(1, 0);
	}

	static const uint16 Indices[] = { 0, 1, 3, 0, 3, 2 };

	DrawIndexedPrimitiveUP(RHICmdList, PT_TriangleList, 0, 4, 2, Indices, sizeof(Indices[0]), Vertices, sizeof(Vertices[0]));
}

static void DrawPokeAHoleCylinderMesh(FRHICommandList& RHICmdList, FVector Base, FVector X, FVector Y, const FMatrix& PosTransform, float ArcAngle, float CylinderHeight, float CylinderRadius, bool InvertCoords)
{
	float ClipSpaceQuadZ = 0.0f;
	const int Sides = 40;

	FFilterVertex Vertices[2 * (Sides+1)];
	static uint16 Indices[6 * Sides]; //	 = { 0, 1, 3, 0, 3, 2 };

	float currentAngle = -ArcAngle / 2;
	float angleStep = ArcAngle / Sides;

	FVector LastVertex = Base + CylinderRadius * (FMath::Cos(currentAngle) * X + FMath::Sin(currentAngle) * Y);
	FVector HalfHeight = FVector(0, 0, CylinderHeight / 2);

	Vertices[0].Position = PosTransform.TransformFVector4(LastVertex - HalfHeight);
	Vertices[1].Position = PosTransform.TransformFVector4(LastVertex + HalfHeight);
	Vertices[0].UV = FVector2D(1, 0);
	Vertices[1].UV = FVector2D(1, 1);

	currentAngle += angleStep;

	for (int side = 0; side < Sides; side++)
	{
		FVector ThisVertex = Base + CylinderRadius * (FMath::Cos(currentAngle) * X + FMath::Sin(currentAngle) * Y);
		currentAngle += angleStep;

		Vertices[2*(side+1)].Position = PosTransform.TransformFVector4(ThisVertex - HalfHeight);
		Vertices[2*(side+1)+1].Position = PosTransform.TransformFVector4(ThisVertex + HalfHeight);
		Vertices[2 * (side + 1)].UV = FVector2D(1 - (side + 1) / (float)Sides, 0);
		Vertices[2 * (side + 1) + 1].UV = FVector2D(1 - (side + 1) / (float)Sides, 1);

		Indices[6 * side + 0] = 2 * side;
		Indices[6 * side + 1] = 2 * side + 1;
		Indices[6 * side + 2] = 2 * (side + 1) + 1;
		Indices[6 * side + 3] = 2 * side;
		Indices[6 * side + 4] = 2 * (side + 1) + 1;
		Indices[6 * side + 5] = 2 * (side + 1);

		LastVertex = ThisVertex;
	}
	
	DrawIndexedPrimitiveUP(RHICmdList, PT_TriangleList, 0, 2*(Sides+1), 2*Sides, Indices, sizeof(Indices[0]), Vertices, sizeof(Vertices[0]));
}

static void DrawPokeAHoleMesh(FRHICommandList& RHICmdList,const FHMDLayerDesc& LayerDesc, const FMatrix& matrix, float scale, bool invertCoords)
{
	if (LayerDesc.GetType() == FHMDLayerDesc::Quad)
	{
		FVector2D quadsize = LayerDesc.GetQuadSize();

		DrawPokeAHoleQuadMesh(RHICmdList, matrix, 0, -quadsize.X*scale / 2, -quadsize.Y*scale / 2, 0, quadsize.X*scale, quadsize.Y*scale, invertCoords);

	}
	else if (LayerDesc.GetType() == FHMDLayerDesc::Cylinder)
	{
		FVector XAxis = FVector(-1, 0, 0);
		FVector YAxis = FVector(0, 1, 0);
		FVector Base = FVector::ZeroVector;
		
		float CylinderRadius = LayerDesc.GetCylinderSize().X;
		float ArcAngle = LayerDesc.GetCylinderSize().Y / CylinderRadius;
		float CylinderHeight = LayerDesc.GetCylinderHeight();

		DrawPokeAHoleCylinderMesh(RHICmdList, Base, XAxis, YAxis, matrix, ArcAngle*scale, CylinderHeight*scale, CylinderRadius, invertCoords);
	}
}

void FHMDLayerManager::PokeAHole(FRHICommandListImmediate& RHICmdList, const FHMDGameFrame* CurrentFrame, IRendererModule* RendererModule,  FSceneViewFamily& InViewFamily)
{
	const uint32 NumLayers = LayersToRender.Num();

	if (NumLayers > 0 && CurrentFrame)
	{
		const auto FeatureLevel = GMaxRHIFeatureLevel;

		const FViewInfo* LeftView = (FViewInfo*)(InViewFamily.Views[0]);
		const FViewInfo* RightView = (FViewInfo*)(InViewFamily.Views[1]);

		TShaderMapRef<FOculusVertexShader> ScreenVertexShader(LeftView->ShaderMap);
		TShaderMapRef<FOculusAlphaInverseShader> PixelShader(LeftView->ShaderMap);
		TShaderMapRef<FOculusWhiteShader> WhitePixelShader(LeftView->ShaderMap);

		for (uint32 i = 0; i < NumLayers; ++i)
		{
			auto RenderLayer = static_cast<FHMDRenderLayer*>(LayersToRender[i].Get());
			if (!RenderLayer || !RenderLayer->IsFullySetup())
			{
				continue;
			}
			const FHMDLayerDesc& LayerDesc = RenderLayer->GetLayerDesc();
			FTextureRHIRef Texture = LayerDesc.GetTexture();

			FMatrix leftMat, rightMat;
			bool invertCoords;
			GetPokeAHoleMatrices(LeftView, RightView, LayerDesc, CurrentFrame, leftMat, rightMat, invertCoords);

			FGraphicsPipelineStateInitializer GraphicsPSOInit;
			RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
			GraphicsPSOInit.PrimitiveType = PT_TriangleList;

			if (LayerDesc.IsPokeAHole())
			{
				if (LayerDesc.GetType() == FHMDLayerDesc::Cubemap)
				{
					RHICmdList.SetViewport(LeftView->ViewRect.Min.X, LeftView->ViewRect.Min.Y, 0.000, RightView->ViewRect.Max.X, RightView->ViewRect.Max.Y, 0.000);
					RHICmdList.SetScissorRect(false, 0, 0, 0, 0);

					GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
					GraphicsPSOInit.BlendState = TStaticBlendState<CW_ALPHA, BO_Add, BF_One, BF_Zero, BO_Add, BF_One, BF_Zero>::GetRHI();
					GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<
						false, CF_Always
					>::GetRHI();

					GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = RendererModule->GetFilterVertexDeclaration().VertexDeclarationRHI;
					GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*ScreenVertexShader);
					GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*WhitePixelShader);
					SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

					DrawPokeAHoleQuadMesh(RHICmdList, FMatrix::Identity, -1, -1, 0, 2, 2, 0, false);

					GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<
						false, CF_Always
					>::GetRHI();

					TShaderMapRef<FOculusBlackShader> BlackPixelShader(LeftView->ShaderMap);

					GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = RendererModule->GetFilterVertexDeclaration().VertexDeclarationRHI;
					GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*ScreenVertexShader);
					GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*BlackPixelShader);
					SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

					DrawPokeAHoleQuadMesh(RHICmdList, FMatrix::Identity, -1, -1, 0, 2, 2, 0, false);
				}
				else
				{
					RHICmdList.SetViewport(LeftView->ViewRect.Min.X, LeftView->ViewRect.Min.Y, 0, LeftView->ViewRect.Max.X, LeftView->ViewRect.Max.Y, 1);
					RHICmdList.SetScissorRect(false, 0, 0, 0, 0);

					GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
					GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<
						false, CF_Always
					>::GetRHI();
					GraphicsPSOInit.BlendState = TStaticBlendState<CW_ALPHA>::GetRHI();

					//draw quad outlines
					GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = RendererModule->GetFilterVertexDeclaration().VertexDeclarationRHI;
					GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*ScreenVertexShader);
					GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*WhitePixelShader);
					SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

					DrawPokeAHoleMesh(RHICmdList, LayerDesc, leftMat, 1.00, invertCoords);

					RHICmdList.SetViewport(RightView->ViewRect.Min.X, RightView->ViewRect.Min.Y, 0, RightView->ViewRect.Max.X, RightView->ViewRect.Max.Y, 1);

					DrawPokeAHoleMesh(RHICmdList, LayerDesc, rightMat, 1.00, invertCoords);

					GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<
						false, CF_DepthNearOrEqual
					>::GetRHI();
					GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_InverseSourceAlpha, BF_SourceAlpha, BO_Add, BF_One, BF_Zero>::GetRHI();

					//draw inverse alpha
					GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = RendererModule->GetFilterVertexDeclaration().VertexDeclarationRHI;
					GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*ScreenVertexShader);
					GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
					SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

					PixelShader->SetParameters(RHICmdList, TStaticSamplerState<SF_Bilinear>::GetRHI(), Texture);

					RHICmdList.SetViewport(LeftView->ViewRect.Min.X, LeftView->ViewRect.Min.Y, 0, LeftView->ViewRect.Max.X, LeftView->ViewRect.Max.Y, 1);

					DrawPokeAHoleMesh(RHICmdList, LayerDesc, leftMat, 0.99, invertCoords);

					RHICmdList.SetViewport(RightView->ViewRect.Min.X, RightView->ViewRect.Min.Y, 0, RightView->ViewRect.Max.X, RightView->ViewRect.Max.Y, 1);

					DrawPokeAHoleMesh(RHICmdList, LayerDesc, rightMat, 0.99, invertCoords);
				}
			
			}
			
		}
	}
}
