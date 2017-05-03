// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "OculusRiftHMD.h"
#include "OculusRiftMeshAssets.h"
#include "Misc/EngineVersion.h"
#include "Misc/CoreDelegates.h"
#include "Engine/Engine.h"
#include "EngineGlobals.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Engine/GameEngine.h"

#if !PLATFORM_MAC // Mac uses 0.5/OculusRiftHMD_05.cpp

#include "EngineAnalytics.h"
#include "IAnalyticsProvider.h"
#include "AnalyticsEventAttribute.h"
#include "SceneViewport.h"
#include "PostProcess/SceneRenderTargets.h"
#include "HardwareInfo.h"
#include "Framework/Application/SlateApplication.h"

#if WITH_EDITOR
#include "Editor/UnrealEd/Classes/Editor/EditorEngine.h"
#endif

//---------------------------------------------------
// Oculus Rift Plugin Implementation
//---------------------------------------------------

#if OCULUS_RIFT_SUPPORTED_PLATFORMS
#if !UE_BUILD_SHIPPING
static void OVR_CDECL OvrLogCallback(uintptr_t userData, int level, const char* message)
{
	FString tbuf = ANSI_TO_TCHAR(message);
	const TCHAR* levelStr = TEXT("");
	switch (level)
	{
	case ovrLogLevel_Debug: levelStr = TEXT(" Debug:"); break;
	case ovrLogLevel_Info:  levelStr = TEXT(" Info:"); break;
	case ovrLogLevel_Error: levelStr = TEXT(" Error:"); break;
	}

	GLog->Logf(TEXT("OCULUS:%s %s"), levelStr, *tbuf);
}
#endif // !UE_BUILD_SHIPPING

static FString GetVersionString()
{
	const char* Results = ovr_GetVersionString();
	FString s = FString::Printf(TEXT("%s, LibOVR: %s, SDK: %s, built %s, %s"), *FEngineVersion::Current().ToString(), UTF8_TO_TCHAR(Results),
		UTF8_TO_TCHAR(OVR_VERSION_STRING), UTF8_TO_TCHAR(__DATE__), UTF8_TO_TCHAR(__TIME__));
	return s;
}

#endif // OCULUS_RIFT_SUPPORTED_PLATFORMS

FOculusRiftPlugin::FOculusRiftPlugin()
{
	bInitialized = false;
	bInitializeCalled = false;
}

bool FOculusRiftPlugin::Initialize()
{
	if(!bInitializeCalled)
	{
		bInitialized = false;
		bInitializeCalled = true;

#if OCULUS_RIFT_SUPPORTED_PLATFORMS
		// Only init module when running Game or Editor, and Oculus service is running
		if (!IsRunningDedicatedServer() && ovr_Detect(0).IsOculusServiceRunning)
		{
			ovrInitParams initParams;
			FMemory::Memset(initParams, 0);
			initParams.Flags = ovrInit_RequestVersion;
			initParams.RequestedMinorVersion = OVR_MINOR_VERSION;
#if !UE_BUILD_SHIPPING
			initParams.LogCallback = OvrLogCallback;
#endif
			ovrResult result = ovr_Initialize(&initParams);

			if (OVR_SUCCESS(result))
			{
				FString identityStr;
				static const TCHAR* AppNameKey = TEXT("ApplicationName:");
				static const TCHAR* AppVersionKey = TEXT("ApplicationVersion:");
				static const TCHAR* EngineNameKey = TEXT("EngineName:");
				static const TCHAR* EngineVersionKey = TEXT("EngineVersion:");
				static const TCHAR* EnginePluginNameKey = TEXT("EnginePluginName:");
				static const TCHAR* EnginePluginVersionKey = TEXT("EnginePluginVersion:");
				static const TCHAR* EngineEditorKey = TEXT("EngineEditor:");

				identityStr += FString(AppNameKey) + FString(FPlatformProcess::ExecutableName()) + TEXT("\n");
				identityStr += FString(EngineNameKey) + TEXT("UnrealEngine") + TEXT("\n");
				identityStr += FString(EngineVersionKey) + FEngineVersion::Current().ToString() + TEXT("\n");
				identityStr += FString(EnginePluginNameKey) + TEXT("OculusRiftHMD") + TEXT("\n");
				identityStr += FString(EnginePluginVersionKey) + GetVersionString() + TEXT("\n");
				identityStr += FString(EngineEditorKey) + ((GIsEditor) ? TEXT("true") : TEXT("false")) + TEXT("\n");

				ovr_IdentifyClient(TCHAR_TO_ANSI(*identityStr));
				bInitialized = true;
			}
			else if (ovrError_LibLoad == result)
			{
				// Can't load library!
				UE_LOG(LogHMD, Log, TEXT("Can't find Oculus library %s: is proper Runtime installed? Version: %s"), 
					TEXT(OVR_FILE_DESCRIPTION_STRING), TEXT(OVR_VERSION_STRING));
			}
		}
#endif // OCULUS_RIFT_SUPPORTED_PLATFORMS
	}

	return bInitialized;
}

#if OCULUS_RIFT_SUPPORTED_PLATFORMS
ovrResult FOculusRiftPlugin::CreateSession(ovrSession* session, ovrGraphicsLuid* luid)
{
	ovrResult result;

	// Try creating session
	result = ovr_Create(session, luid);
	if (!OVR_SUCCESS(result))
	{
		if (result == ovrError_ServiceConnection || result == ovrError_ServiceError || result == ovrError_NotInitialized)
		{
			// Shutdown connection to service
			FlushRenderingCommands();
			ShutdownModule();
			bInitializeCalled = false;

			// Reinitialize connection to service
			if(Initialize())
			{
				// Retry creating session
				result = ovr_Create(session, luid);
			}
		}
	}

	// Remember which devices are connected to the HMD
	if (OVR_SUCCESS(result))
	{
#ifdef OVR_D3D
		SetGraphicsAdapter(*luid);
#endif

		WCHAR AudioInputDevice[OVR_AUDIO_MAX_DEVICE_STR_SIZE];
		if (OVR_SUCCESS(ovr_GetAudioDeviceInGuidStr(AudioInputDevice)))
		{
			GConfig->SetString(TEXT("Oculus.Settings"), TEXT("AudioInputDevice"), AudioInputDevice, GEngineIni);
		}

		WCHAR AudioOutputDevice[OVR_AUDIO_MAX_DEVICE_STR_SIZE];
		if (OVR_SUCCESS(ovr_GetAudioDeviceOutGuidStr(AudioOutputDevice)))
		{
			GConfig->SetString(TEXT("Oculus.Settings"), TEXT("AudioOutputDevice"), AudioOutputDevice, GEngineIni);
		}
	}

	return result;
}

void FOculusRiftPlugin::DestroySession(ovrSession session)
{
	ovr_Destroy(session);
}
#endif //OCULUS_RIFT_SUPPORTED_PLATFORMS

void FOculusRiftPlugin::ShutdownModule()
{
#if OCULUS_RIFT_SUPPORTED_PLATFORMS
	if (bInitialized)
	{
		ovr_Shutdown();
		UE_LOG(LogHMD, Log, TEXT("Oculus shutdown."));

		bInitialized = false;
		bInitializeCalled = false;
	}
#endif // OCULUS_RIFT_SUPPORTED_PLATFORMS
}

FString FOculusRiftPlugin::GetModuleKeyName() const
{
	return FString(TEXT("OculusRift"));
}

bool FOculusRiftPlugin::PreInit()
{
#if OCULUS_RIFT_SUPPORTED_PLATFORMS
	if (Initialize())
	{
#ifdef OVR_D3D
		DisableSLI();
#endif

		// Create (and destroy) a session to record which devices are connected to the HMD
		ovrSession session;
		ovrGraphicsLuid luid;
		if (OVR_SUCCESS(CreateSession(&session, &luid)))
		{
			DestroySession(session);
		}

		return true;
	}
#endif // OCULUS_RIFT_SUPPORTED_PLATFORMS
	return false;
}

bool FOculusRiftPlugin::IsHMDConnected()
{
#if OCULUS_RIFT_SUPPORTED_PLATFORMS
	if(!IsRunningDedicatedServer() && ovr_Detect(0).IsOculusHMDConnected)
	{
		return true;
	}
#endif // OCULUS_RIFT_SUPPORTED_PLATFORMS
	return false;
}

int FOculusRiftPlugin::GetGraphicsAdapter()
{
	int GraphicsAdapter = -1;
	GConfig->GetInt(TEXT("Oculus.Settings"), TEXT("GraphicsAdapter"), GraphicsAdapter, GEngineIni);
	return GraphicsAdapter;
}

FString FOculusRiftPlugin::GetAudioInputDevice()
{
	FString AudioInputDevice;
	GConfig->GetString(TEXT("Oculus.Settings"), TEXT("AudioInputDevice"), AudioInputDevice, GEngineIni);
	return AudioInputDevice;
}

FString FOculusRiftPlugin::GetAudioOutputDevice()
{
	FString AudioOutputDevice;
	GConfig->GetString(TEXT("Oculus.Settings"), TEXT("AudioOutputDevice"), AudioOutputDevice, GEngineIni);
	return AudioOutputDevice;
}

TSharedPtr< class IHeadMountedDisplay, ESPMode::ThreadSafe > FOculusRiftPlugin::CreateHeadMountedDisplay()
{
#if OCULUS_RIFT_SUPPORTED_PLATFORMS
	if (Initialize())
	{
		TSharedPtr< FOculusRiftHMD, ESPMode::ThreadSafe > OculusRiftHMD(new FOculusRiftHMD());
		if (OculusRiftHMD->IsInitialized())
		{
			HeadMountedDisplay = OculusRiftHMD;
			return OculusRiftHMD;
		}
	}
#endif//OCULUS_RIFT_SUPPORTED_PLATFORMS
	HeadMountedDisplay = nullptr;
	return NULL;
}

#if OCULUS_RIFT_SUPPORTED_PLATFORMS
bool FOculusRiftPlugin::PoseToOrientationAndPosition(const ovrPosef& Pose, FQuat& OutOrientation, FVector& OutPosition) const
{
	check(IsInGameThread());
	bool bRetVal = false;

	IHeadMountedDisplay* HMD = HeadMountedDisplay.Pin().Get();
	if (HMD && HMD->GetHMDDeviceType() == EHMDDeviceType::DT_OculusRift)
	{
		FOculusRiftHMD* OculusHMD = static_cast<FOculusRiftHMD*>(HMD);

		auto Frame = OculusHMD->GetFrame();
		if (Frame != nullptr)
		{
			Frame->PoseToOrientationAndPosition(Pose, /* Out */ OutOrientation, /* Out */ OutPosition);
			bRetVal = true;
		}
	}

	return bRetVal;
}

class FOvrSessionShared* FOculusRiftPlugin::GetSession()
{
	check(IsInGameThread());

	IHeadMountedDisplay* HMD = HeadMountedDisplay.Pin().Get();
	if (HMD && HMD->GetHMDDeviceType() == EHMDDeviceType::DT_OculusRift)
	{
		FOculusRiftHMD* OculusHMD = static_cast<FOculusRiftHMD*>(HMD);
		return OculusHMD->Session.Get();
	}

	return nullptr;
}

bool FOculusRiftPlugin::GetCurrentTrackingState(ovrTrackingState* TrackingState)
{
	check(IsInGameThread());
	bool bRetVal = false;

	IHeadMountedDisplay* HMD = HeadMountedDisplay.Pin().Get();
	if (TrackingState && HMD && HMD->GetHMDDeviceType() == EHMDDeviceType::DT_OculusRift)
	{
		FOculusRiftHMD* OculusHMD = static_cast<FOculusRiftHMD*>(HMD);
		auto Frame = OculusHMD->GetFrame();
		if (Frame != nullptr)
		{
			*TrackingState = Frame->InitialTrackingState;
			bRetVal = true;
		}
	}

	return bRetVal;
}
#endif //OCULUS_RIFT_SUPPORTED_PLATFORMS

IMPLEMENT_MODULE( FOculusRiftPlugin, OculusRift )


//---------------------------------------------------
// Oculus Rift IHeadMountedDisplay Implementation
//---------------------------------------------------

#if OCULUS_RIFT_SUPPORTED_PLATFORMS

//////////////////////////////////////////////////////////////////////////
FSettings::FSettings()
	: PixelDensity(1.0f)
	, PixelDensityMin(ClampPixelDensityMin)
	, PixelDensityMax(ClampPixelDensityMax)
	, bPixelDensityAdaptive(false)
	, IdealScreenPercentage(100.f)
	, CurrentScreenPercentage(100.f)
{
	FMemory::Memset(EyeRenderDesc, 0);
	FMemory::Memset(EyeProjectionMatrices, 0);
	FMemory::Memset(EyeFov, 0);

	SupportedTrackingCaps = SupportedHmdCaps = 0;
	TrackingCaps = HmdCaps = 0;


	RenderTargetSize = FIntPoint(0, 0);
}

TSharedPtr<FHMDSettings, ESPMode::ThreadSafe> FSettings::Clone() const
{
	TSharedPtr<FSettings, ESPMode::ThreadSafe> NewSettings = MakeShareable(new FSettings(*this));
	return NewSettings;
}

//////////////////////////////////////////////////////////////////////////
FGameFrame::FGameFrame()
{
	FMemory::Memset(InitialTrackingState, 0);
	FMemory::Memset(CurEyeRenderPose, 0);
	FMemory::Memset(CurHeadPose, 0);
	FMemory::Memset(EyeRenderPose, 0);
	FMemory::Memset(HeadPose, 0);
	FMemory::Memset(SessionStatus, 0);
}

TSharedPtr<FHMDGameFrame, ESPMode::ThreadSafe> FGameFrame::Clone() const
{
	TSharedPtr<FGameFrame, ESPMode::ThreadSafe> NewFrame = MakeShareable(new FGameFrame(*this));
	return NewFrame;
}

//////////////////////////////////////////////////////////////////////////
TSharedPtr<FHMDGameFrame, ESPMode::ThreadSafe> FOculusRiftHMD::CreateNewGameFrame() const
{
	TSharedPtr<FGameFrame, ESPMode::ThreadSafe> Result(MakeShareable(new FGameFrame()));
	return Result;
}

TSharedPtr<FHMDSettings, ESPMode::ThreadSafe> FOculusRiftHMD::CreateNewSettings() const
{
	TSharedPtr<FSettings, ESPMode::ThreadSafe> Result(MakeShareable(new FSettings()));
	return Result;
}

bool FOculusRiftHMD::OnStartGameFrame( FWorldContext& InWorldContext )
{
	if (GIsRequestingExit)
	{
		return false;
	}

	// check if HMD is marked as invalid and needs to be killed.
	if (pCustomPresent && pCustomPresent->NeedsToKillHmd())
	{
		DoEnableStereo(false);
		ReleaseDevice();
		Flags.bNeedEnableStereo = true;
	}

	check(Settings.IsValid());
	if (!Settings->IsStereoEnabled())
	{
		FApp::SetUseVRFocus(false);
		FApp::SetHasVRFocus(false);
	}

	if (pCustomPresent)
	{
		ovrResult SubmitFrameResult = pCustomPresent->GetLastSubmitFrameResult();
		if (SubmitFrameResult != LastSubmitFrameResult)
		{
			if ((SubmitFrameResult == ovrError_DisplayLost || SubmitFrameResult == ovrError_NoHmd) && !OCFlags.DisplayLostDetected)
			{
				FCoreDelegates::VRHeadsetLost.Broadcast();
				OCFlags.DisplayLostDetected = true;
			}
			else if (OVR_SUCCESS(SubmitFrameResult))
			{
				if (OCFlags.DisplayLostDetected)
				{
					FCoreDelegates::VRHeadsetReconnected.Broadcast();
				}
				OCFlags.DisplayLostDetected = false;
			}
			LastSubmitFrameResult = SubmitFrameResult;
		}
	}

	if (!FHeadMountedDisplay::OnStartGameFrame( InWorldContext ))
	{
		return false;
	}

	FGameFrame* CurrentFrame = GetFrame();
	FSettings*  MasterSettings = GetSettings();

	// need to make a copy of settings here, since settings could change.
	CurrentFrame->Settings = MasterSettings->Clone();
	FSettings* CurrentSettings = CurrentFrame->GetSettings();

	bool retval = true;

	do { // to perform 'break' from inside
	FOvrSessionShared::AutoSession OvrSession(Session);
	if (OvrSession)
	{
		if (OCFlags.NeedSetTrackingOrigin)
		{
			ovr_SetTrackingOriginType(OvrSession, OvrOrigin);
			OCFlags.NeedSetTrackingOrigin = false;
		}

		ovr_GetSessionStatus(OvrSession, &CurrentFrame->SessionStatus);

		FApp::SetUseVRFocus(true);
		FApp::SetHasVRFocus(CurrentFrame->SessionStatus.IsVisible != ovrFalse);

		// Do not pause if Editor is running (otherwise it will become very lagy)
		if (!GIsEditor)
		{
			if (!CurrentFrame->SessionStatus.IsVisible)
			{
				// not visible, 
				if (!Settings->Flags.bPauseRendering)
				{
					UE_LOG(LogHMD, Log, TEXT("The app went out of VR focus, seizing rendering..."));
				}
			}
			else if (Settings->Flags.bPauseRendering)
			{
				UE_LOG(LogHMD, Log, TEXT("The app got VR focus, restoring rendering..."));
			}
			if (OCFlags.NeedSetFocusToGameViewport)
			{
				if (CurrentFrame->SessionStatus.IsVisible)
				{
					UE_LOG(LogHMD, Log, TEXT("Setting user focus to game viewport since session status is visible..."));
					FSlateApplication::Get().SetAllUserFocusToGameViewport();
					OCFlags.NeedSetFocusToGameViewport = false;
				}
			}

			bool bPrevPause = Settings->Flags.bPauseRendering;
			Settings->Flags.bPauseRendering = CurrentFrame->Settings->Flags.bPauseRendering = !CurrentFrame->SessionStatus.IsVisible;

			if (bPrevPause != Settings->Flags.bPauseRendering)
			{
				APlayerController* const PC = GEngine->GetFirstLocalPlayerController(InWorldContext.World());
				if (Settings->Flags.bPauseRendering)
				{
					// focus is lost
					GEngine->SetMaxFPS(10);

					if (!FCoreDelegates::ApplicationWillEnterBackgroundDelegate.IsBound())
					{
						OCFlags.AppIsPaused = false;
						// default action: set pause if not already paused
						if (PC && !PC->IsPaused())
						{
							PC->SetPause(true);
							OCFlags.AppIsPaused = true;
						}
					}
					else
					{
						FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Broadcast();
					}
				}
				else
				{
					// focus is gained
					GEngine->SetMaxFPS(0);

					if (!FCoreDelegates::ApplicationHasEnteredForegroundDelegate.IsBound())
					{
						// default action: unpause if was paused by the plugin
						if (PC && OCFlags.AppIsPaused)
						{
							PC->SetPause(false);
						}
						OCFlags.AppIsPaused = false;
					}
					else
					{
						FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Broadcast();
					}
				}
			}
		}

		if (CurrentFrame->SessionStatus.ShouldQuit || OCFlags.EnforceExit)
		{
			FPlatformMisc::LowLevelOutputDebugString(TEXT("OculusRift plugin requested exit (ShouldQuit == 1)\n"));
#if WITH_EDITOR
			if (GIsEditor)
			{
				FSceneViewport* SceneVP = FindSceneViewport();
				if (SceneVP && SceneVP->IsStereoRenderingAllowed())
				{
					TSharedPtr<SWindow> Window = SceneVP->FindWindow();
					Window->RequestDestroyWindow();
				}
			}
			else
#endif//WITH_EDITOR
			{
				// ApplicationWillTerminateDelegate will fire from inside of the RequestExit
				FPlatformMisc::RequestExit(false);
			}
			OCFlags.EnforceExit = false;
			retval = false;
			break; // goto end
		}

		if (CurrentFrame->SessionStatus.ShouldRecenter)
		{
			FPlatformMisc::LowLevelOutputDebugString(TEXT("OculusRift plugin was requested to recenter\n"));
			if (FCoreDelegates::VRHeadsetRecenter.IsBound())
			{
				FCoreDelegates::VRHeadsetRecenter.Broadcast();

				// we must call ovr_ClearShouldRecenterFlag, otherwise ShouldRecenter flag won't reset
				ovr_ClearShouldRecenterFlag(OvrSession);
			}
			else
			{
				ResetOrientationAndPosition();
			}
		}

		CurrentSettings->EyeRenderDesc[0] = ovr_GetRenderDesc(OvrSession, ovrEye_Left, CurrentSettings->EyeFov[0]);
		CurrentSettings->EyeRenderDesc[1] = ovr_GetRenderDesc(OvrSession, ovrEye_Right, CurrentSettings->EyeFov[1]);
#if !UE_BUILD_SHIPPING
		const OVR::Vector3f newLeft(CurrentSettings->EyeRenderDesc[0].HmdToEyeOffset);
		const OVR::Vector3f newRight(CurrentSettings->EyeRenderDesc[1].HmdToEyeOffset);
		const OVR::Vector3f prevLeft(MasterSettings->EyeRenderDesc[0].HmdToEyeOffset);
		const OVR::Vector3f prevRight(MasterSettings->EyeRenderDesc[1].HmdToEyeOffset);
		// for debugging purposes only: overriding IPD
		if( CurrentSettings->Flags.bOverrideIPD )
		{
			check( CurrentSettings->InterpupillaryDistance >= 0 );
			CurrentSettings->EyeRenderDesc[ 0 ].HmdToEyeOffset.x = -CurrentSettings->InterpupillaryDistance * 0.5f;
			CurrentSettings->EyeRenderDesc[ 1 ].HmdToEyeOffset.x = CurrentSettings->InterpupillaryDistance * 0.5f;
		}
		else if (newLeft != prevLeft || newRight != prevRight)
		{
			const float newIAD = newRight.Distance(newLeft);
			UE_LOG(LogHMD, Log, TEXT("IAD has changed, new IAD is %.4f meters"), newIAD);
		}
#endif // #if !UE_BUILD_SHIPPING
		// Save EyeRenderDesc in main settings.
		MasterSettings->EyeRenderDesc[0] = CurrentSettings->EyeRenderDesc[0];
		MasterSettings->EyeRenderDesc[1] = CurrentSettings->EyeRenderDesc[1];

		// Save eye and head poses
		CurrentFrame->InitialTrackingState = CurrentFrame->GetTrackingState(OvrSession);
		CurrentFrame->GetHeadAndEyePoses(CurrentFrame->InitialTrackingState, CurrentFrame->CurHeadPose, CurrentFrame->CurEyeRenderPose);

		CurrentFrame->Flags.bHaveVisionTracking = (CurrentFrame->InitialTrackingState.StatusFlags & ovrStatus_PositionTracked) != 0;
		if (CurrentFrame->Flags.bHaveVisionTracking && !Flags.bHadVisionTracking)
		{
			UE_LOG(LogHMD, Log, TEXT("Vision Tracking Acquired"));
		}
		if (!CurrentFrame->Flags.bHaveVisionTracking && Flags.bHadVisionTracking)
		{
			UE_LOG(LogHMD, Log, TEXT("Lost Vision Tracking"));
		}
		Flags.bHadVisionTracking = CurrentFrame->Flags.bHaveVisionTracking;
#if !UE_BUILD_SHIPPING
		{ // used for debugging, do not remove
			FQuat CurHmdOrientation;
			FVector CurHmdPosition;
			GetCurrentPose(CurHmdOrientation, CurHmdPosition, false, false);
			//UE_LOG(LogHMD, Log, TEXT("BFPOSE: Pos %.3f %.3f %.3f, fr: %d"), CurHmdPosition.X, CurHmdPosition.Y, CurHmdPosition.Y,(int)CurrentFrame->FrameNumber);
			//UE_LOG(LogHMD, Log, TEXT("BFPOSE: Yaw %.3f Pitch %.3f Roll %.3f, fr: %d"), CurHmdOrientation.Rotator().Yaw, CurHmdOrientation.Rotator().Pitch, CurHmdOrientation.Rotator().Roll,(int)CurrentFrame->FrameNumber);
		}
#endif
	}
	} while (0);
	if (GIsRequestingExit)
	{
		// need to shutdown HMD here, otherwise the whole shutdown process may take forever.
		PreShutdown();
		GEngine->ShutdownHMD();
		// note, 'this' may become invalid after ShutdownHMD
	}
	return retval;
}

bool FOculusRiftHMD::IsHMDConnected()
{
	return Settings->Flags.bHMDEnabled && ovr_Detect(0).IsOculusHMDConnected;
}

EHMDWornState::Type FOculusRiftHMD::GetHMDWornState()
{
	FOvrSessionShared::AutoSession OvrSession(Session);
	// If there is no VR Session, initialize the Oculus HMD
	if (!OvrSession)
	{
		InitDevice();
	}
	if (OvrSession && !pCustomPresent->NeedsToKillHmd())
	{
		ovrSessionStatus sessionStatus;
		ovr_GetSessionStatus(OvrSession, &sessionStatus);
		
		// Check if the HMD is worn 
		if (sessionStatus.HmdMounted )
		{
			return EHMDWornState::Worn; // already created and worn

		}
	}
	return EHMDWornState::NotWorn;
}

FGameFrame* FOculusRiftHMD::GetFrame()
{
	return static_cast<FGameFrame*>(GetCurrentFrame());
}

const FGameFrame* FOculusRiftHMD::GetFrame() const
{
	return static_cast<FGameFrame*>(GetCurrentFrame());
}

EHMDDeviceType::Type FOculusRiftHMD::GetHMDDeviceType() const
{
	return EHMDDeviceType::DT_OculusRift;
}

bool FOculusRiftHMD::GetHMDMonitorInfo(MonitorInfo& MonitorDesc)
{
	return false;
}

bool FOculusRiftHMD::DoesSupportPositionalTracking() const
{
	const FGameFrame* frame = GetFrame();
	if (frame)
	{
		const FSettings* OculusSettings = frame->GetSettings();
		return (OculusSettings->SupportedTrackingCaps & ovrTrackingCap_Position) != 0;
	}
	return false;
}

bool FOculusRiftHMD::HasValidTrackingPosition()
{
	const auto frame = GetFrame();
	return (frame && frame->Flags.bHaveVisionTracking);
}

uint32 FOculusRiftHMD::GetNumOfTrackingSensors() const
{
	if (!Session->IsActive())
	{
		return 0;
	}
	FOvrSessionShared::AutoSession OvrSession(Session);
	return ovr_GetTrackerCount(OvrSession);
}

#define SENSOR_FOCAL_DISTANCE			1.00f // meters (focal point to origin for position)

bool FOculusRiftHMD::GetTrackingSensorProperties(uint8 InSensorIndex, FVector& OutOrigin, FQuat& OutOrientation, float& OutLeftFOV, float& OutRightFOV, float& OutTopFOV, float& OutBottomFOV, float& OutCameraDistance, float& OutNearPlane, float& OutFarPlane) const
{
	OutOrigin = FVector::ZeroVector;
	OutOrientation = FQuat::Identity;
	OutLeftFOV = OutRightFOV = OutTopFOV = OutBottomFOV = OutCameraDistance = OutNearPlane = OutFarPlane = 0;

	const auto frame = GetFrame();
	if (!Session->IsActive() || !frame)
	{
		return false;
	}

	FOvrSessionShared::AutoSession OvrSession(Session);
	unsigned int NumOfSensors = ovr_GetTrackerCount(OvrSession);
	if (InSensorIndex >= NumOfSensors)
	{
		return false;
	}
	const ovrTrackerDesc TrackerDesc = ovr_GetTrackerDesc(OvrSession, InSensorIndex);
	const ovrTrackerPose TrackerPose = ovr_GetTrackerPose(OvrSession, InSensorIndex);

	const float WorldToMetersScale = frame->GetWorldToMetersScale();
	check(WorldToMetersScale >= 0);

	OutCameraDistance = SENSOR_FOCAL_DISTANCE * WorldToMetersScale;
	OutLeftFOV = OutRightFOV = FMath::RadiansToDegrees(TrackerDesc.FrustumHFovInRadians) * 0.5f;
	OutTopFOV = OutBottomFOV = FMath::RadiansToDegrees(TrackerDesc.FrustumVFovInRadians) * 0.5f;
	OutNearPlane = TrackerDesc.FrustumNearZInMeters * WorldToMetersScale;
	OutFarPlane = TrackerDesc.FrustumFarZInMeters * WorldToMetersScale;

	// Check if the sensor pose is available
	if (TrackerPose.TrackerFlags & (ovrTracker_Connected | ovrTracker_PoseTracked))
	{
		FQuat Orient;
		FVector Pos;
		frame->PoseToOrientationAndPosition(TrackerPose.Pose, Orient, Pos);

		OutOrientation = Orient * FQuat(0, 1, 0, 0);
		OutOrigin = Pos + frame->Settings->PositionOffset;
	}
	return true;
}

void FOculusRiftHMD::RebaseObjectOrientationAndPosition(FVector& OutPosition, FQuat& OutOrientation) const
{
}

// Returns tracking state for current frame
ovrTrackingState FGameFrame::GetTrackingState(ovrSession InOvrSession) const
{
	const FSettings* CurrentSettings = GetSettings();
	check(CurrentSettings);
	const double DisplayTime = ovr_GetPredictedDisplayTime(InOvrSession, FrameNumber);
	const bool LatencyMarker = (IsInRenderingThread() || !CurrentSettings->Flags.bUpdateOnRT);
	return ovr_GetTrackingState(InOvrSession, DisplayTime, LatencyMarker);
}

// Returns HeadPose and EyePoses calculated from TrackingState
void FGameFrame::GetHeadAndEyePoses(const ovrTrackingState& TrackingState, ovrPosef& outHeadPose, ovrPosef outEyePoses[2]) const
{
	const FSettings* CurrentSettings = GetSettings();
	const ovrVector3f hmdToEyeViewOffset[2] = { CurrentSettings->EyeRenderDesc[0].HmdToEyeOffset, CurrentSettings->EyeRenderDesc[1].HmdToEyeOffset };

	outHeadPose = TrackingState.HeadPose.ThePose;
	ovr_CalcEyePoses(TrackingState.HeadPose.ThePose, hmdToEyeViewOffset, outEyePoses);
}

void FGameFrame::PoseToOrientationAndPosition(const ovrPosef& InPose, FQuat& OutOrientation, FVector& OutPosition) const
{
	OutOrientation = ToFQuat(InPose.Orientation);

	const float FinalWorldToMetersScale = GetWorldToMetersScale();
	check( FinalWorldToMetersScale >= 0);

	// correct position according to BaseOrientation and BaseOffset. 
	const FVector Pos = (ToFVector_M2U(OVR::Vector3f(InPose.Position), FinalWorldToMetersScale) - (Settings->BaseOffset * FinalWorldToMetersScale)) * CameraScale3D;
	OutPosition = Settings->BaseOrientation.Inverse().RotateVector(Pos);

	// apply base orientation correction to OutOrientation
	OutOrientation = Settings->BaseOrientation.Inverse() * OutOrientation;
	OutOrientation.Normalize();
}

void FOculusRiftHMD::GetCurrentPose(FQuat& CurrentHmdOrientation, FVector& CurrentHmdPosition, bool bUseOrienationForPlayerCamera, bool bUsePositionForPlayerCamera)
{
	check(!IsInActualRenderingThread());

	auto frame = GetFrame();
	check(frame);

	if (IsInGameThread() && (bUseOrienationForPlayerCamera || bUsePositionForPlayerCamera))
	{
		// if this pose is going to be used for camera update then save it.
		// This matters only if bUpdateOnRT is OFF.
		frame->EyeRenderPose[0] = frame->CurEyeRenderPose[0];
		frame->EyeRenderPose[1] = frame->CurEyeRenderPose[1];
		frame->HeadPose = frame->CurHeadPose;
	}

	frame->PoseToOrientationAndPosition(frame->CurHeadPose, CurrentHmdOrientation, CurrentHmdPosition);
	//UE_LOG(LogHMD, Log, TEXT("CRHDPS: Pos %.3f %.3f %.3f"), frame->CurTrackingState.HeadPose.ThePose.Position.x, frame->CurTrackingState.HeadPose.ThePose.Position.y, frame->CurTrackingState.HeadPose.ThePose.Position.z);
	//UE_LOG(LogHMD, Log, TEXT("CRPOSE: Pos %.3f %.3f %.3f"), CurrentHmdPosition.X, CurrentHmdPosition.Y, CurrentHmdPosition.Z);
	//UE_LOG(LogHMD, Log, TEXT("CRPOSE: Yaw %.3f Pitch %.3f Roll %.3f"), CurrentHmdOrientation.Rotator().Yaw, CurrentHmdOrientation.Rotator().Pitch, CurrentHmdOrientation.Rotator().Roll);
}

TSharedPtr<ISceneViewExtension, ESPMode::ThreadSafe> FOculusRiftHMD::GetViewExtension()
{
 	TSharedPtr<FViewExtension, ESPMode::ThreadSafe> Result(MakeShareable(new FViewExtension(this)));
 	return Result;
}

void FOculusRiftHMD::ResetStereoRenderingParams()
{
	FHeadMountedDisplay::ResetStereoRenderingParams();
	Settings->InterpupillaryDistance = -1.f;
	Settings->Flags.bOverrideIPD = false;
}


void FSettings::UpdateScreenPercentageFromPixelDensity()
{
	if (IdealScreenPercentage > 0.f)
	{
		static const auto ScreenPercentageCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"));
		// Set r.ScreenPercentage to reflect the change to PixelDensity. Set CurrentScreenPercentage as well so the CVar sink handler doesn't kick in as well.
		CurrentScreenPercentage = PixelDensity * IdealScreenPercentage;
		ScreenPercentageCVar->Set(CurrentScreenPercentage, EConsoleVariableFlags(ScreenPercentageCVar->GetFlags() & ECVF_SetByMask)); // Use same priority as the existing priority
	}
}

bool FSettings::UpdatePixelDensityFromScreenPercentage()
{
	static const auto ScreenPercentageCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"));
	check(ScreenPercentageCVar);

	int SetBy = (ScreenPercentageCVar->GetFlags() & ECVF_SetByMask);
	float ScreenPercentage = ScreenPercentageCVar->GetFloat();

	//<= 0 means disable override. As does 100.f if set the flags indicate its set via scalability settings, as we want to use the Oculus default in that case.
	if (ScreenPercentage <= 0.f || SetBy == ECVF_SetByConstructor || (SetBy == ECVF_SetByScalability && CurrentScreenPercentage == 100.f) || FMath::IsNearlyEqual(ScreenPercentage, CurrentScreenPercentage))
	{
		CurrentScreenPercentage = ScreenPercentage;
		return false;
	}

	if (IdealScreenPercentage > 0.f)
	{
		PixelDensity = FMath::Clamp(ScreenPercentage / IdealScreenPercentage, ClampPixelDensityMin, ClampPixelDensityMax);
		PixelDensityMin = FMath::Min(PixelDensity, PixelDensityMin);
		PixelDensityMax = FMath::Max(PixelDensity, PixelDensityMax);
	}

	CurrentScreenPercentage = ScreenPercentage;
	return true;
}

void FOculusRiftHMD::SetPixelDensity(float NewPD)
{
	auto CurrentSettings = GetSettings();
	CurrentSettings->PixelDensity = FMath::Clamp(NewPD, ClampPixelDensityMin, ClampPixelDensityMax);
	CurrentSettings->PixelDensityMin = FMath::Min(CurrentSettings->PixelDensity, CurrentSettings->PixelDensityMin);
	CurrentSettings->PixelDensityMax = FMath::Max(CurrentSettings->PixelDensity, CurrentSettings->PixelDensityMax);
	CurrentSettings->UpdateScreenPercentageFromPixelDensity();
	Flags.bNeedUpdateStereoRenderingParams = true;
}

FString FOculusRiftHMD::GetVersionString() const
{
	return ::GetVersionString();
}


void FOculusRiftHMD::UseImplicitHmdPosition( bool bInImplicitHmdPosition ) 
{ 
	if( GEnableVREditorHacks )
	{
		const auto frame = GetFrame();
		if( frame )
		{
			frame->Flags.bPlayerControllerFollowsHmd = bInImplicitHmdPosition;
		}
	}
}


bool FOculusRiftHMD::PopulateAnalyticsAttributes(TArray<FAnalyticsEventAttribute>& EventAttributes)
{
	if (!FHeadMountedDisplayBase::PopulateAnalyticsAttributes(EventAttributes))
	{
		return false;
	}

	FOvrSessionShared::AutoSession OvrSession(Session);
	if (OvrSession)
	{
		EventAttributes.Add(FAnalyticsEventAttribute(TEXT("DeviceManufacturer"),ANSI_TO_TCHAR(HmdDesc.Manufacturer)));
		EventAttributes.Add(FAnalyticsEventAttribute(TEXT("DeviceProductName"), ANSI_TO_TCHAR(HmdDesc.ProductName)));
	}
	EventAttributes.Add(FAnalyticsEventAttribute(TEXT("OverrideInterpupillaryDistance"), (bool) Settings->Flags.bOverrideIPD));

	EventAttributes.Add(FAnalyticsEventAttribute(TEXT("HQBuffer"), (bool) Settings->Flags.bHQBuffer));
	EventAttributes.Add(FAnalyticsEventAttribute(TEXT("HQDistortion"), (bool) Settings->Flags.bHQDistortion));
	EventAttributes.Add(FAnalyticsEventAttribute(TEXT("UpdateOnRT"), (bool) Settings->Flags.bUpdateOnRT));

	return true;
}

class FSceneViewport* FOculusRiftHMD::FindSceneViewport()
{
	if (!GIsEditor)
	{
		UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
		return GameEngine->SceneViewport.Get();
	}
#if WITH_EDITOR
	else
	{
		UEditorEngine* EditorEngine = CastChecked<UEditorEngine>(GEngine);
		FSceneViewport* PIEViewport = (FSceneViewport*)EditorEngine->GetPIEViewport();
		if( PIEViewport != nullptr && PIEViewport->IsStereoRenderingAllowed() )
		{
			// PIE is setup for stereo rendering
			return PIEViewport;
		}
		else
		{
			// Check to see if the active editor viewport is drawing in stereo mode
			// @todo vreditor: Should work with even non-active viewport!
			FSceneViewport* EditorViewport = (FSceneViewport*)EditorEngine->GetActiveViewport();
			if( EditorViewport != nullptr && EditorViewport->IsStereoRenderingAllowed() )
			{
				return EditorViewport;
			}
		}
	}
#endif
	return nullptr;
}

//---------------------------------------------------
// Oculus Rift IStereoRendering Implementation
//---------------------------------------------------
bool FOculusRiftHMD::DoEnableStereo(bool bStereo)
{
	check(IsInGameThread());

	FSceneViewport* SceneVP = FindSceneViewport();

	if (!Settings->Flags.bHMDEnabled || (SceneVP && !SceneVP->IsStereoRenderingAllowed()))
	{
		bStereo = false;
	}

	if (Settings->Flags.bStereoEnabled && bStereo || !Settings->Flags.bStereoEnabled && !bStereo)
	{
		// already in the desired mode
		return Settings->Flags.bStereoEnabled;
	}

	TSharedPtr<SWindow> Window;

	if (SceneVP)
	{
		Window = SceneVP->FindWindow();
	}

	if (!Window.IsValid() || !SceneVP || !SceneVP->GetViewportWidget().IsValid())
	{
		// try again next frame
		if(bStereo)
		{
			Flags.bNeedEnableStereo = true;

			// a special case when stereo is enabled while window is not available yet:
			// most likely this is happening from BeginPlay. In this case, if frame exists (created in OnBeginPlay)
			// then we need init device and populate the initial tracking for head/hand poses.
			auto CurrentFrame = GetGameFrame();
			if (CurrentFrame)
			{
				InitDevice();
				FOvrSessionShared::AutoSession OvrSession(Session);
				CurrentFrame->InitialTrackingState = CurrentFrame->GetTrackingState(OvrSession);
				CurrentFrame->GetHeadAndEyePoses(CurrentFrame->InitialTrackingState, CurrentFrame->CurHeadPose, CurrentFrame->CurEyeRenderPose);
			}
		}
		else
		{
			Flags.bNeedDisableStereo = true;
		}

		return Settings->Flags.bStereoEnabled;
	}

	if (OnOculusStateChange(bStereo))
	{
		Settings->Flags.bStereoEnabled = bStereo;

		// Uncap fps to enable FPS higher than 62
		GEngine->bForceDisableFrameRateSmoothing = bStereo;

		// Set MirrorWindow state on the Window
		Window->SetMirrorWindow(bStereo);

		if (bStereo)
		{
			// Set viewport size to Rift resolution
			SceneVP->SetViewportSize(HmdDesc.Resolution.w, HmdDesc.Resolution.h);

			if (Settings->Flags.bPauseRendering)
			{
				GEngine->SetMaxFPS(10);
			}
		}
		else
		{
			if (Settings->Flags.bPauseRendering)
			{
				GEngine->SetMaxFPS(0);
			}

			// Restore viewport size to window size
			FVector2D size = Window->GetSizeInScreen();
			SceneVP->SetViewportSize(size.X, size.Y);
			Window->SetViewportSizeDrivenByWindow(true);
		}
	}

	return Settings->Flags.bStereoEnabled;
}

bool FOculusRiftHMD::OnOculusStateChange(bool bIsEnabledNow)
{
	if (!bIsEnabledNow)
	{
		// Switching from stereo
		ReleaseDevice();

		ResetControlRotation();
		return true;
	}
	else
	{
		// Switching to stereo
		InitDevice();

		if (Session->IsActive())
		{
			Flags.bApplySystemOverridesOnStereo = true;
			UpdateStereoRenderingParams();
			return true;
		}
		DeltaControlRotation = FRotator::ZeroRotator;
	}
	return false;
}

float FOculusRiftHMD::GetVsyncToNextVsync() const
{
	return GetSettings()->VsyncToNextVsync;
}

FPerformanceStats FOculusRiftHMD::GetPerformanceStats() const
{
	return PerformanceStats;
}

FVector FOculusRiftHMD::ScaleAndMovePointWithPlayer(ovrVector3f& OculusRiftPoint)
{
	/* Initalization */
	FGameFrame* frame = GetFrame();
	const float WorldToMetersScale = frame->GetWorldToMetersScale();

	FVector LastPlayerPos;
	FQuat LastPlayerRot;
	this->GetLastPlayerLocationAndRotation(LastPlayerPos, LastPlayerRot);

	FMatrix transmat;
	transmat.SetIdentity();
	transmat = transmat.ConcatTranslation(LastPlayerPos);


	FVector ConvertedPoint = ToFVector_M2U(OculusRiftPoint, WorldToMetersScale);

	FRotator RotateWithPlayer = LastPlayerRot.Rotator();
	FVector TransformWithPlayer = RotateWithPlayer.RotateVector(ConvertedPoint);
	TransformWithPlayer = FVector(transmat.TransformPosition(TransformWithPlayer));

	return TransformWithPlayer;
}

float FOculusRiftHMD::ConvertFloat_M2U(float OculusFloat)
{
	FGameFrame* frame = GetFrame();
	const float WorldToMetersScale = frame->GetWorldToMetersScale();
	OculusFloat *= WorldToMetersScale;
	return OculusFloat;
}

FVector FOculusRiftHMD::ConvertVector_M2U(ovrVector3f OculusRiftPoint)
{
	/* Initalization */
	FGameFrame* frame = GetFrame();
	const float WorldToMetersScale = frame->GetWorldToMetersScale();

	return ToFVector_M2U(OculusRiftPoint, WorldToMetersScale);
}

void FOculusRiftHMD::GetLastPlayerLocationAndRotation(FVector& PlayerLocation, FQuat& PlayerRotation)
{
	PlayerLocation = this->LastPlayerLocation;
	PlayerRotation = this->LastPlayerRotation;
}

void FOculusRiftHMD::CalculateStereoViewOffset(const EStereoscopicPass StereoPassType, const FRotator& ViewRotation, const float WorldToMeters, FVector& ViewLocation)
{
	check(WorldToMeters != 0.f);

	const int idx = (StereoPassType == eSSP_LEFT_EYE) ? 0 : 1;

	if (IsInGameThread())
	{
		const auto frame = GetFrame();
		if (!frame)
		{
			return;
		}

		// This method is called from GetProjectionData on a game thread.
		// The modified ViewLocation is used ONLY for ViewMatrix composition, it is not
		// stored modified in the ViewInfo. ViewInfo.ViewLocation remains unmodified.

		if (StereoPassType != eSSP_FULL || frame->Settings->Flags.bHeadTrackingEnforced)
		{
			if (!frame->Flags.bOrientationChanged)
			{
				UE_LOG(LogHMD, Verbose, TEXT("Orientation wasn't applied to a camera in frame %d"), int(CurrentFrameNumber.GetValue()));
			}

			FVector CurEyePosition;
			FQuat CurEyeOrient;
			frame->PoseToOrientationAndPosition(frame->EyeRenderPose[idx], CurEyeOrient, CurEyePosition);

			FVector HeadPosition = FVector::ZeroVector;
			// If we use PlayerController->bFollowHmd then we must apply full EyePosition (HeadPosition == 0).
			// Otherwise, we will apply only a difference between EyePosition and HeadPosition, since
			// HeadPosition is supposedly already applied.
			if (!frame->Flags.bPlayerControllerFollowsHmd)
			{
				FQuat HeadOrient; 
				frame->PoseToOrientationAndPosition(frame->HeadPose, HeadOrient, HeadPosition);
			}

			// apply stereo disparity to ViewLocation. Note, ViewLocation already contains HeadPose.Position, thus
			// we just need to apply delta between EyeRenderPose.Position and the HeadPose.Position. 
			// EyeRenderPose and HeadPose are captured by the same call to GetEyePoses.
			const FVector HmdToEyeOffset = CurEyePosition - HeadPosition;

			frame->PlayerLocation = ViewLocation - HeadPosition;
			this->LastPlayerLocation = frame->PlayerLocation;

			// Calculate the difference between the final ViewRotation and EyeOrientation:
			// we need to rotate the HmdToEyeOffset by this differential quaternion.
			// When bPlayerControllerFollowsHmd == true, the DeltaControlOrientation already contains
			// the proper value (see ApplyHmdRotation)
			//FRotator r = ViewRotation - CurEyeOrient.Rotator();
			const FQuat ViewOrient = ViewRotation.Quaternion();
			const FQuat DeltaControlOrientation =  ViewOrient * CurEyeOrient.Inverse();
			this->LastPlayerRotation = DeltaControlOrientation;
			//UE_LOG(LogHMD, Log, TEXT("EYEROT: Yaw %.3f Pitch %.3f Roll %.3f"), CurEyeOrient.Rotator().Yaw, CurEyeOrient.Rotator().Pitch, CurEyeOrient.Rotator().Roll);
			//UE_LOG(LogHMD, Log, TEXT("VIEROT: Yaw %.3f Pitch %.3f Roll %.3f"), ViewRotation.Yaw, ViewRotation.Pitch, ViewRotation.Roll);
			//UE_LOG(LogHMD, Log, TEXT("DLTROT: Yaw %.3f Pitch %.3f Roll %.3f"), DeltaControlOrientation.Rotator().Yaw, DeltaControlOrientation.Rotator().Pitch, DeltaControlOrientation.Rotator().Roll);

			// The HMDPosition already has HMD orientation applied.
			// Apply rotational difference between HMD orientation and ViewRotation
			// to HMDPosition vector. 
			const FVector vEyePosition = DeltaControlOrientation.RotateVector(HmdToEyeOffset) + frame->Settings->PositionOffset;
			ViewLocation += vEyePosition;
			
			//UE_LOG(LogHMD, Log, TEXT("DLTPOS: %.3f %.3f %.3f"), vEyePosition.X, vEyePosition.Y, vEyePosition.Z);
		}
	}
}

void FOculusRiftHMD::ResetOrientationAndPosition(float yaw)
{
	Settings->Flags.bHeadTrackingEnforced = false;
	Settings->BaseOffset = FVector::ZeroVector;
	if (yaw != 0.0f)
	{
		Settings->BaseOrientation = FRotator(0, -yaw, 0).Quaternion();
	}
	else
	{
		Settings->BaseOrientation = FQuat::Identity;
	}
	FOvrSessionShared::AutoSession OvrSession(Session);
	if (OvrSession)
	{
		ovr_RecenterTrackingOrigin(OvrSession);
	}
}

void FOculusRiftHMD::ResetOrientation(float yaw)
{
	// Reset only orientation; keep the same position
	Settings->Flags.bHeadTrackingEnforced = false;
	if (yaw != 0.0f)
	{
		Settings->BaseOrientation = FRotator(0, -yaw, 0).Quaternion();
	}
	else
	{
		Settings->BaseOrientation = FQuat::Identity;
	}
	Settings->BaseOffset = FVector::ZeroVector;
	FOvrSessionShared::AutoSession OvrSession(Session);
	if (OvrSession)
	{
		ovr_RecenterTrackingOrigin(OvrSession);
		const ovrTrackingState post = ovr_GetTrackingState(OvrSession, ovr_GetTimeInSeconds(), false);

		UE_LOG(LogHMD, Log, TEXT("ORIGINPOS: %.3f %.3f %.3f"), ToFVector(post.CalibratedOrigin.Position).X, ToFVector(post.CalibratedOrigin.Position).Y, ToFVector(post.CalibratedOrigin.Position).Z);

		// calc base offset to compensate the offset after the ovr_RecenterTrackingOrigin call
		Settings->BaseOffset		= ToFVector(post.CalibratedOrigin.Position);
		
		//@DBG: if the following line is uncommented then orientation and position in UE coords should stay the same.
		//Settings->BaseOrientation	= ToFQuat(post.CalibratedOrigin.Orientation);
	}
}

void FOculusRiftHMD::ResetPosition()
{
	// Reset only position; keep the same orientation
	Settings->Flags.bHeadTrackingEnforced = false;
	Settings->BaseOffset = FVector::ZeroVector;
	FOvrSessionShared::AutoSession OvrSession(Session);
	if (OvrSession)
	{
		ovr_RecenterTrackingOrigin(OvrSession);
		const ovrTrackingState post = ovr_GetTrackingState(OvrSession, ovr_GetTimeInSeconds(), false);

		// calc base orientation to compensate the offset after the ovr_RecenterTrackingOrigin call
		Settings->BaseOrientation = ToFQuat(post.CalibratedOrigin.Orientation);
	}
}

FMatrix FOculusRiftHMD::GetStereoProjectionMatrix(EStereoscopicPass StereoPassType, const float FOV) const
{
	auto frame = GetFrame();
	check(frame);
	check(IsStereoEnabled());

	const FSettings* FrameSettings = frame->GetSettings();

	const int idx = (StereoPassType == eSSP_LEFT_EYE) ? 0 : 1;

	FMatrix proj = ToFMatrix(FrameSettings->EyeProjectionMatrices[idx]);

	// correct far and near planes for reversed-Z projection matrix
	const float WorldScale = frame->GetWorldToMetersScale() * (1.0 / 100.0f); // physical scale is 100 UUs/meter
	const float InNearZ = (FrameSettings->NearClippingPlane) ? FrameSettings->NearClippingPlane : (GNearClippingPlane * WorldScale);
	const float InFarZ = (FrameSettings->FarClippingPlane) ? FrameSettings->FarClippingPlane : (GNearClippingPlane * WorldScale);

	proj.M[3][3] = 0.0f;
	proj.M[2][3] = 1.0f;

	proj.M[2][2] = (InNearZ == InFarZ) ? 0.0f    : InNearZ / (InNearZ - InFarZ);
	proj.M[3][2] = (InNearZ == InFarZ) ? InNearZ : -InFarZ * InNearZ / (InNearZ - InFarZ);

	return proj;
}

void FOculusRiftHMD::GetOrthoProjection(int32 RTWidth, int32 RTHeight, float OrthoDistance, FMatrix OrthoProjection[2]) const
{
	const auto frame = GetFrame();
	const FSettings* FrameSettings = frame->GetSettings();
	// We deliberately ignore the world to meters setting and always use 100 here, as canvas distance is hard coded based on an 100 uus per meter assumption.
	float orthoDistance = OrthoDistance / 100.f; 

	for(int eyeIndex = 0; eyeIndex < 2; eyeIndex++)
	{
		const FIntRect& eyeRenderViewport = FrameSettings->EyeRenderViewport[eyeIndex];
		const ovrEyeRenderDesc& eyeRenderDesc = FrameSettings->EyeRenderDesc[eyeIndex];
		const ovrMatrix4f& perspectiveProjection = FrameSettings->PerspectiveProjection[eyeIndex];

		ovrVector2f orthoScale = OVR::Vector2f(1.0f) / OVR::Vector2f(eyeRenderDesc.PixelsPerTanAngleAtCenter);
		ovrMatrix4f orthoSubProjection = ovrMatrix4f_OrthoSubProjection(perspectiveProjection, orthoScale, orthoDistance, eyeRenderDesc.HmdToEyeOffset.x);

		OrthoProjection[eyeIndex] = FScaleMatrix(FVector(
			2.0f / (float) RTWidth,
			1.0f / (float) RTHeight,
			1.0f));

		OrthoProjection[eyeIndex] *= FTranslationMatrix(FVector(
			orthoSubProjection.M[0][3] * 0.5f,
			0.0f,
			0.0f));

		OrthoProjection[eyeIndex] *= FScaleMatrix(FVector(
			(float) eyeRenderViewport.Width(),
			(float) eyeRenderViewport.Height(),
			1.0f));

		OrthoProjection[eyeIndex] *= FTranslationMatrix(FVector(
			(float) eyeRenderViewport.Min.X,
			(float) eyeRenderViewport.Min.Y,
			0.0f));

		OrthoProjection[eyeIndex] *= FScaleMatrix(FVector(
			(float) RTWidth / (float) FrameSettings->RenderTargetSize.X,
			(float) RTHeight / (float) FrameSettings->RenderTargetSize.Y,
			1.0f));
	}
}

void FOculusRiftHMD::InitCanvasFromView(FSceneView* InView, UCanvas* Canvas)
{
	// This is used for placing small HUDs (with names)
	// over other players (for example, in Capture Flag).
	// HmdOrientation should be initialized by GetCurrentOrientation (or
	// user's own value).
}

//---------------------------------------------------
// Oculus Rift ISceneViewExtension Implementation
//---------------------------------------------------
void FOculusRiftHMD::SetupViewFamily(FSceneViewFamily& InViewFamily)
{
	auto frame = GetFrame();
	check(frame);

	extern RENDERER_API TAutoConsoleVariable<int32> CVarAllowMotionBlurInVR;

	InViewFamily.EngineShowFlags.MotionBlur = CVarAllowMotionBlurInVR->GetInt() != 0;
	InViewFamily.EngineShowFlags.HMDDistortion = false;
	InViewFamily.EngineShowFlags.StereoRendering = IsStereoEnabled();
	if (frame->Settings->Flags.bPauseRendering)
	{
		InViewFamily.EngineShowFlags.Rendering = 0;
	}
}

void FOculusRiftHMD::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
	auto frame = GetFrame();
	check(frame);

	InView.BaseHmdOrientation = frame->LastHmdOrientation;
	InView.BaseHmdLocation = frame->LastHmdPosition;

	InViewFamily.bUseSeparateRenderTarget = ShouldUseSeparateRenderTarget();

	const int eyeIdx = (InView.StereoPass == eSSP_LEFT_EYE) ? 0 : 1;

	const FSettings* const CurrentSettings = frame->GetSettings();

	InView.ViewRect = CurrentSettings->EyeRenderViewport[eyeIdx];

	if (CurrentSettings->bPixelDensityAdaptive)
	{
		InView.ResolutionOverrideRect = CurrentSettings->EyeMaxRenderViewport[eyeIdx];
	}

	frame->CachedViewRotation[eyeIdx] = InView.ViewRotation;
}

bool FOculusRiftHMD::IsHeadTrackingAllowed() const
{
#if WITH_EDITOR
	if (GIsEditor)
	{
		// @todo vreditor: We need to do a pass over VREditor code and make sure we are handling the VR modes correctly.  HeadTracking can be enabled without Stereo3D, for example
		// @todo vreditor: Added GEnableVREditorHacks check below to allow head movement in non-PIE editor; needs revisit
		UEditorEngine* EdEngine = Cast<UEditorEngine>(GEngine);
		return Session->IsActive() && (!EdEngine || ( GEnableVREditorHacks || EdEngine->bUseVRPreviewForPlayWorld ) || GetDefault<ULevelEditorPlaySettings>()->ViewportGetsHMDControl) &&	(Settings->Flags.bHeadTrackingEnforced || GEngine->IsStereoscopic3D());
	}
#endif//WITH_EDITOR
	return Session->IsActive() && FHeadMountedDisplay::IsHeadTrackingAllowed();
}

//---------------------------------------------------
// Oculus Rift Specific
//---------------------------------------------------

FOculusRiftHMD::FOculusRiftHMD()
	:
	Session(nullptr)
	, OvrOrigin(ovrTrackingOrigin_EyeLevel)
	, LastSubmitFrameResult(ovrSuccess)
	, ConsoleCommands(this)
{
	OCFlags.Raw = 0;
	DeltaControlRotation = FRotator::ZeroRotator;
	HmdDesc.Type = ovrHmd_None;
	Session = MakeShareable(new FOvrSessionShared());
	LastPlayerLocation = FVector(0.0f);
	LastPlayerRotation = FQuat(0.0f, 0.0f, 0.0f, 0.0f);

	Settings = MakeShareable(new FSettings);
	RendererModule = nullptr;
	Startup();
}

FOculusRiftHMD::~FOculusRiftHMD()
{
	Shutdown();
}


void FOculusRiftHMD::Startup()
{
#if PLATFORM_MAC
	if (GIsEditor)
	{
		// no editor support for Mac yet
		return;
	}
#endif 
	LastSubmitFrameResult = ovrSuccess;
	HmdDesc.Type = ovrHmd_None;

	Settings->Flags.InitStatus |= FSettings::eStartupExecuted;

	if (GIsEditor)
	{
		Settings->Flags.bHeadTrackingEnforced = true;
	}

	check(!pCustomPresent.GetReference())

	FString RHIString;
	{
		FString HardwareDetails = FHardwareInfo::GetHardwareDetailsString();
		FString RHILookup = NAME_RHI.ToString() + TEXT( "=" );

		if (!FParse::Value(*HardwareDetails, *RHILookup, RHIString))
		{
			return;
		}
	}

#ifdef OVR_D3D
	if (RHIString == TEXT("D3D11"))
	{
		pCustomPresent = new D3D11Bridge(Session, this);
	}
	else if (RHIString == TEXT("D3D12"))
	{
		pCustomPresent = new D3D12Bridge(Session, this);
	}
	else
#endif
#ifdef OVR_GL
	if (RHIString == TEXT("OpenGL"))
	{
		pCustomPresent = new OGLBridge(Session, this);
	}
	else
#endif
	{
		UE_LOG(LogHMD, Warning, TEXT("%s is not currently supported by OculusRiftHMD plugin"), *RHIString);
		return;
	}

	Settings->Flags.InitStatus |= FSettings::eInitialized;

	UE_LOG(LogHMD, Log, TEXT("Oculus plugin initialized. Version: %s"), *GetVersionString());

	// grab a pointer to the renderer module for displaying our mirror window
	static const FName RendererModuleName("Renderer");
	RendererModule = FModuleManager::GetModulePtr<IRendererModule>(RendererModuleName);

	Splash = MakeShareable(new FOculusRiftSplash(this));
	Splash->Startup();
}

void FOculusRiftHMD::Shutdown()
{
	if (!Settings.IsValid() || !(Settings->Flags.InitStatus & FSettings::eInitialized))
	{
		return;
	}

	if (Splash.IsValid())
	{
		Splash->Shutdown();
		Splash = nullptr;
	}

	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(ShutdownRen,
	FOculusRiftHMD*, Plugin, this,
	{
		Plugin->ShutdownRendering();
	});
	FlushRenderingCommands();

	ReleaseDevice();
	
	Settings = nullptr;
	Frame = nullptr;
	RenderFrame = nullptr;
}

void FOculusRiftHMD::PreShutdown()
{
	if (Splash.IsValid())
	{
		Splash->PreShutdown();
	}
}

bool FOculusRiftHMD::InitDevice()
{
	check(pCustomPresent);

	FOvrSessionShared::AutoSession OvrSession(Session);
	if (OvrSession && !pCustomPresent->NeedsToKillHmd())
	{
		ovrSessionStatus sessionStatus;
		ovr_GetSessionStatus(OvrSession, &sessionStatus);

		if (sessionStatus.HmdPresent)
		{
			return true; // already created and present
		}
	}

	ReleaseDevice();
	FSettings* CurrentSettings = GetSettings();
	HmdDesc.Type = ovrHmd_None;

	if(!IsHMDConnected())
	{
		// don't bother with ovr_Create if HMD is not connected
		return false; 
	}

 	ovrGraphicsLuid luid;
	ovrResult result = Session->Create(luid);
	if (OVR_SUCCESS(result) && Session->IsActive())
	{
		OCFlags.NeedSetFocusToGameViewport = true;

		if(pCustomPresent->IsUsingGraphicsAdapter(luid))
		{
			HmdDesc = ovr_GetHmdDesc(OvrSession);

			CurrentSettings->SupportedHmdCaps = HmdDesc.AvailableHmdCaps;
			CurrentSettings->SupportedTrackingCaps = HmdDesc.AvailableTrackingCaps;
			CurrentSettings->TrackingCaps = HmdDesc.DefaultTrackingCaps;
			CurrentSettings->HmdCaps = HmdDesc.DefaultHmdCaps;

			LoadFromIni();

			UpdateDistortionCaps();
			UpdateHmdRenderInfo();
			UpdateStereoRenderingParams();
			UpdateHmdCaps();

			if (!HiddenAreaMeshes[0].IsValid() || !HiddenAreaMeshes[1].IsValid())
			{
				SetupOcclusionMeshes();
			}

			// Do not set VR focus in Editor by just creating a device; Editor may have it created w/o requiring focus.
			// Instead, set VR focus in OnBeginPlay (VR Preview will run there first).
			if (!GIsEditor)
			{
				FApp::SetUseVRFocus(true);
				FApp::SetHasVRFocus(true);
			}
		}
		else
		{
			// UNDONE Message that you need to restart application to use correct adapter
			Session->Destroy();
		}
	}
	else
	{
		UE_LOG(LogHMD, Log, TEXT("HMD can't be initialized, err = %d"), int(result));
	}

	return Session->IsActive();
}

void FOculusRiftHMD::ReleaseDevice()
{
	if (Session->IsActive())
	{
		SaveToIni();


		// Wait for all resources to be released

		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(ResetRen,
		FOculusRiftHMD*, Plugin, this,
		{
			if (Plugin->pCustomPresent)
			{
				Plugin->pCustomPresent->Reset();
			}
		});

		// Wait for all resources to be released
		FlushRenderingCommands();

		// The Editor may release VR focus in OnEndPlay
		if (!GIsEditor)
		{
			FApp::SetUseVRFocus(false);
			FApp::SetHasVRFocus(false);
		}

		Session->Destroy();
		FMemory::Memzero(HmdDesc);
	}
	if (pCustomPresent)
	{
		pCustomPresent->ResetNeedsToKillHmd();
	}
}

void FOculusRiftHMD::SetupOcclusionMeshes()
{
	if (HmdDesc.Type == ovrHmdType::ovrHmd_DK2)
	{
		HiddenAreaMeshes[0].BuildMesh(DK2_LeftEyeHiddenAreaPositions, HiddenAreaVertexCount, FHMDViewMesh::MT_HiddenArea);
		HiddenAreaMeshes[1].BuildMesh(DK2_RightEyeHiddenAreaPositions, HiddenAreaVertexCount, FHMDViewMesh::MT_HiddenArea);
		VisibleAreaMeshes[0].BuildMesh(DK2_LeftEyeVisibleAreaPositions, VisibleAreaVertexCount, FHMDViewMesh::MT_VisibleArea);
		VisibleAreaMeshes[1].BuildMesh(DK2_RightEyeVisibleAreaPositions, VisibleAreaVertexCount, FHMDViewMesh::MT_VisibleArea);
	}
	else if (HmdDesc.Type == ovrHmdType::ovrHmd_CB)
	{
		HiddenAreaMeshes[0].BuildMesh(CB_LeftEyeHiddenAreaPositions, HiddenAreaVertexCount, FHMDViewMesh::MT_HiddenArea);
		HiddenAreaMeshes[1].BuildMesh(CB_RightEyeHiddenAreaPositions, HiddenAreaVertexCount, FHMDViewMesh::MT_HiddenArea);
		VisibleAreaMeshes[0].BuildMesh(CB_LeftEyeVisibleAreaPositions, VisibleAreaVertexCount, FHMDViewMesh::MT_VisibleArea);
		VisibleAreaMeshes[1].BuildMesh(CB_RightEyeVisibleAreaPositions, VisibleAreaVertexCount, FHMDViewMesh::MT_VisibleArea);
	}
	else if (HmdDesc.Type > ovrHmdType::ovrHmd_CB)
	{
		HiddenAreaMeshes[0].BuildMesh(EVT_LeftEyeHiddenAreaPositions, HiddenAreaVertexCount, FHMDViewMesh::MT_HiddenArea);
		HiddenAreaMeshes[1].BuildMesh(EVT_RightEyeHiddenAreaPositions, HiddenAreaVertexCount, FHMDViewMesh::MT_HiddenArea);
		VisibleAreaMeshes[0].BuildMesh(EVT_LeftEyeVisibleAreaPositions, VisibleAreaVertexCount, FHMDViewMesh::MT_VisibleArea);
		VisibleAreaMeshes[1].BuildMesh(EVT_RightEyeVisibleAreaPositions, VisibleAreaVertexCount, FHMDViewMesh::MT_VisibleArea);
	}
}

FORCEINLINE static float CalculateVerticalFovInDegrees(const ovrFovPort& fovLeftEye, const ovrFovPort& fovRightEye)
{
	return FMath::RadiansToDegrees(FMath::Atan(FMath::Max(fovLeftEye.UpTan, fovRightEye.UpTan)) + FMath::Atan(FMath::Max(fovLeftEye.DownTan, fovRightEye.DownTan)));
}

FORCEINLINE static float CalculateHorizontalFovInDegrees(const ovrFovPort& fovLeftEye, const ovrFovPort& fovRightEye)
{
	return FMath::RadiansToDegrees(FMath::Atan(fovLeftEye.LeftTan) + FMath::Atan(fovRightEye.RightTan));
}

void FOculusRiftHMD::GetFieldOfView(float& InOutHFOVInDegrees, float& InOutVFOVInDegrees) const
{
	const FSettings* CurrentSettings = GetSettings();
	InOutHFOVInDegrees = CalculateHorizontalFovInDegrees(CurrentSettings->EyeFov[0], CurrentSettings->EyeFov[1]);
	InOutVFOVInDegrees = CalculateVerticalFovInDegrees(CurrentSettings->EyeFov[0], CurrentSettings->EyeFov[1]);
}

void FOculusRiftHMD::UpdateHmdRenderInfo()
{
	static const auto ScreenPercentageCVar = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.ScreenPercentage"));
	FOvrSessionShared::AutoSession OvrSession(Session);
	check(OvrSession);

	UE_LOG(LogHMD, Log, TEXT("HMD %s, res = %d x %d"), ANSI_TO_TCHAR(HmdDesc.ProductName), 
		HmdDesc.Resolution.w, HmdDesc.Resolution.h); 

	FSettings* CurrentSettings = GetSettings();
	// Save FOV
	CurrentSettings->EyeFov[0] = HmdDesc.DefaultEyeFov[0];
	CurrentSettings->EyeFov[1] = HmdDesc.DefaultEyeFov[1];

	const ovrSizei recommenedTex0Size = ovr_GetFovTextureSize(OvrSession, ovrEye_Left, CurrentSettings->EyeFov[0], 1.0f);
	const ovrSizei recommenedTex1Size = ovr_GetFovTextureSize(OvrSession, ovrEye_Right, CurrentSettings->EyeFov[1], 1.0f);

	ovrSizei idealRenderTargetSize;
	idealRenderTargetSize.w = recommenedTex0Size.w + recommenedTex1Size.w;
	idealRenderTargetSize.h = FMath::Max(recommenedTex0Size.h, recommenedTex1Size.h);

	CurrentSettings->CurrentScreenPercentage = ScreenPercentageCVar->GetValueOnGameThread();
	CurrentSettings->IdealScreenPercentage = FMath::Max(float(idealRenderTargetSize.w) / float(HmdDesc.Resolution.w) * 100.f,
														float(idealRenderTargetSize.h) / float(HmdDesc.Resolution.h) * 100.f);

	// Override eye distance by the value from HMDInfo (stored in Profile).
	if (!CurrentSettings->Flags.bOverrideIPD)
	{
		CurrentSettings->InterpupillaryDistance = -1.f;
	}

	// cache eye to neck distance.
	float neck2eye[2] = { OVR_DEFAULT_NECK_TO_EYE_HORIZONTAL, OVR_DEFAULT_NECK_TO_EYE_VERTICAL };
	ovr_GetFloatArray(OvrSession, OVR_KEY_NECK_TO_EYE_DISTANCE, neck2eye, 2);
	CurrentSettings->NeckToEyeInMeters = FVector2D(neck2eye[0], neck2eye[1]);

	// cache VsyncToNextVsync value
	CurrentSettings->VsyncToNextVsync = ovr_GetFloat(OvrSession, "VsyncToNextVsync", 0.0f);

	// Default texture size (per eye) is equal to half of W x H resolution. Will be overridden in SetupView.
	//CurrentSettings->SetViewportSize(HmdDesc.Resolution.w / 2, HmdDesc.Resolution.h);

	Flags.bNeedUpdateStereoRenderingParams = true;
}

void FOculusRiftHMD::UpdateStereoRenderingParams()
{
	check(IsInGameThread());

	FSettings* CurrentSettings = GetSettings();

	if (!CurrentSettings->IsStereoEnabled() && !CurrentSettings->Flags.bHeadTrackingEnforced)
	{
		return;
	}
	FOvrSessionShared::AutoSession OvrSession(Session);
	if (IsInitialized() && Session->IsActive())
	{
		CurrentSettings->EyeRenderDesc[0] = ovr_GetRenderDesc(OvrSession, ovrEye_Left, CurrentSettings->EyeFov[0]);
		CurrentSettings->EyeRenderDesc[1] = ovr_GetRenderDesc(OvrSession, ovrEye_Right, CurrentSettings->EyeFov[1]);
#if !UE_BUILD_SHIPPING
		if (CurrentSettings->Flags.bOverrideIPD)
		{
			check(CurrentSettings->InterpupillaryDistance >= 0);
			CurrentSettings->EyeRenderDesc[0].HmdToEyeOffset.x = -CurrentSettings->InterpupillaryDistance * 0.5f;
			CurrentSettings->EyeRenderDesc[1].HmdToEyeOffset.x = CurrentSettings->InterpupillaryDistance * 0.5f;
		}
#endif // #if !UE_BUILD_SHIPPING

		const unsigned int ProjModifiers = ovrProjection_None; //@TODO revise to use ovrProjection_FarClipAtInfinity and/or ovrProjection_FarLessThanNear
		// Far and Near clipping planes will be modified in GetStereoProjectionMatrix()
		CurrentSettings->EyeProjectionMatrices[0] = ovrMatrix4f_Projection(CurrentSettings->EyeFov[0], 0.01f, 10000.0f, ProjModifiers | ovrProjection_LeftHanded);
		CurrentSettings->EyeProjectionMatrices[1] = ovrMatrix4f_Projection(CurrentSettings->EyeFov[1], 0.01f, 10000.0f, ProjModifiers | ovrProjection_LeftHanded);

		CurrentSettings->PerspectiveProjection[0] = ovrMatrix4f_Projection(CurrentSettings->EyeFov[0], 0.01f, 10000.f, ProjModifiers & ~ovrProjection_LeftHanded);
		CurrentSettings->PerspectiveProjection[1] = ovrMatrix4f_Projection(CurrentSettings->EyeFov[1], 0.01f, 10000.f, ProjModifiers & ~ovrProjection_LeftHanded);

		// Update PixelDensity
		float PixelDensity = CurrentSettings->PixelDensity;

		if (CurrentSettings->bPixelDensityAdaptive && pCustomPresent && pCustomPresent->IsReadyToSubmitFrame())
		{
			PixelDensity *= ovr_GetFloat(OvrSession, "AdaptiveGpuPerformanceScale", 1.0f);
		}

		PixelDensity = FMath::Clamp(PixelDensity, CurrentSettings->PixelDensityMin, CurrentSettings->PixelDensityMax);

		// Calculate maximum and desired viewport sizes
		// Viewports need to be at least 1x1 to avoid division-by-zero
		ovrSizei vpSize[ovrEye_Count];
		ovrSizei vpSizeMax[ovrEye_Count];

		for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
		{
			vpSize[eyeIndex] = ovr_GetFovTextureSize(OvrSession, (ovrEyeType)eyeIndex, CurrentSettings->EyeFov[eyeIndex], PixelDensity);
			vpSize[eyeIndex].w = FMath::Max(vpSize[eyeIndex].w, 1);
			vpSize[eyeIndex].h = FMath::Max(vpSize[eyeIndex].h, 1);

			if (CurrentSettings->bPixelDensityAdaptive)
			{
				vpSizeMax[eyeIndex] = ovr_GetFovTextureSize(OvrSession, (ovrEyeType)eyeIndex, CurrentSettings->EyeFov[eyeIndex], CurrentSettings->PixelDensityMax);
				vpSizeMax[eyeIndex].w = FMath::Max(vpSizeMax[eyeIndex].w, 1);
				vpSizeMax[eyeIndex].h = FMath::Max(vpSizeMax[eyeIndex].h, 1);
			}
			else
			{
				vpSizeMax[eyeIndex] = vpSize[eyeIndex];
			}
		}

		// Calculate render target size
		ovrSizei rtSize;
		const int32 rtPadding = FMath::CeilToInt(CurrentSettings->GetTexturePaddingPerEye() * 2.0f);
		{
			// Render target needs to be large enough to handle max pixel density
			rtSize.w = vpSizeMax[ovrEye_Left].w + rtPadding + vpSizeMax[ovrEye_Right].w;
			rtSize.h = FMath::Max(vpSizeMax[ovrEye_Left].h, vpSizeMax[ovrEye_Right].h);

			// Quantize render target size to multiple of 16
			FHeadMountedDisplay::QuantizeBufferSize(rtSize.w, rtSize.h, 4);
		}

		const int32 FamilyWidth = vpSize[ovrEye_Left].w + vpSize[ovrEye_Right].w + rtPadding;
		const int32 FamilyWidthMax = vpSizeMax[ovrEye_Left].w + vpSizeMax[ovrEye_Right].w + rtPadding;

		if (!FMath::IsNearlyEqual(CurrentSettings->PixelDensity, PixelDensity))
		{
			CurrentSettings->PixelDensity = PixelDensity;
			CurrentSettings->UpdateScreenPercentageFromPixelDensity();
		}
		CurrentSettings->RenderTargetSize.X = rtSize.w;
		CurrentSettings->RenderTargetSize.Y = rtSize.h;
		CurrentSettings->EyeRenderViewport[0] = FIntRect(0, 0, vpSize[ovrEye_Left].w, vpSize[ovrEye_Left].h);
		CurrentSettings->EyeRenderViewport[1] = FIntRect(FamilyWidth - vpSize[ovrEye_Right].w, 0, FamilyWidth, vpSize[ovrEye_Right].h);

		CurrentSettings->EyeMaxRenderViewport[0] = FIntRect(0, 0, vpSizeMax[ovrEye_Left].w, vpSizeMax[ovrEye_Left].h);
		CurrentSettings->EyeMaxRenderViewport[1] = FIntRect(FamilyWidthMax - vpSizeMax[ovrEye_Right].w, 0, FamilyWidthMax, vpSizeMax[ovrEye_Right].h);

		// If PixelDensity is adaptive, we need update every frame
		Flags.bNeedUpdateStereoRenderingParams = CurrentSettings->bPixelDensityAdaptive;
	}
}


bool FOculusRiftHMD::HandleInputKey(UPlayerInput* pPlayerInput,
	const FKey& Key, EInputEvent EventType, float AmountDepressed, bool bGamepad)
{
	return false;
}

void FOculusRiftHMD::MakeSureValidFrameExists(AWorldSettings* InWorldSettings)
{
	CreateAndInitNewGameFrame(InWorldSettings);
	check(Frame.IsValid());
	Frame->Flags.bOutOfFrame = true;
	Frame->Settings->Flags.bHeadTrackingEnforced = true; // to make sure HMD data is available

	// if we need to enable stereo then populate the frame with initial tracking data.
	// once this is done GetOrientationAndPosition will be able to return actual HMD / MC data (when requested
	// from BeginPlay event, for example).
	if (Flags.bNeedEnableStereo)
	{
		auto CurrentFrame = GetGameFrame();
		InitDevice();
		FOvrSessionShared::AutoSession OvrSession(Session);
		CurrentFrame->InitialTrackingState = CurrentFrame->GetTrackingState(OvrSession);
		CurrentFrame->GetHeadAndEyePoses(CurrentFrame->InitialTrackingState, CurrentFrame->CurHeadPose, CurrentFrame->CurEyeRenderPose);
	}
}

void FOculusRiftHMD::OnBeginPlay(FWorldContext& InWorldContext)
{
	CachedViewportWidget.Reset();
	CachedWindow.Reset();

	if( !GEnableVREditorHacks )
	{
#if WITH_EDITOR
		// @TODO: add more values here.
		// This call make sense when 'Play' is used from the Editor;
		if( GIsEditor )
		{
			Settings->PositionOffset = FVector::ZeroVector;
			Settings->BaseOrientation = FQuat::Identity;
			Settings->BaseOffset = FVector::ZeroVector;
			//Settings->WorldToMetersScale = InWorldContext.World()->GetWorldSettings()->WorldToMeters;
			//Settings->Flags.bWorldToMetersOverride = false;
			InitDevice();

			FApp::SetUseVRFocus( true );
			FApp::SetHasVRFocus( true );
			OnStartGameFrame( InWorldContext );
		}
		else
#endif
		{
			MakeSureValidFrameExists( InWorldContext.World()->GetWorldSettings() );
		}
	}
}

void FOculusRiftHMD::OnEndPlay(FWorldContext& InWorldContext)
{
	if (GIsEditor && !GEnableVREditorHacks)
	{
		// @todo vreditor: If we add support for starting PIE while in VR Editor, we don't want to kill stereo mode when exiting PIE
		EnableStereo(false);
		ReleaseDevice();

		FApp::SetUseVRFocus(false);
		FApp::SetHasVRFocus(false);

		if (Splash.IsValid())
		{
			Splash->ClearSplashes();
		}
	}
}

void FOculusRiftHMD::GetRawSensorData(SensorData& OutData)
{
	FMemory::Memset(OutData, 0);
	InitDevice();
	FOvrSessionShared::AutoSession OvrSession(Session);
	if (Session->IsActive())
	{
		const ovrTrackingState ss = ovr_GetTrackingState(OvrSession, ovr_GetTimeInSeconds(), false);
		OutData.AngularAcceleration = ToFVector(ss.HeadPose.AngularAcceleration);
		OutData.LinearAcceleration	= ToFVector(ss.HeadPose.LinearAcceleration);
		OutData.AngularVelocity = ToFVector(ss.HeadPose.AngularVelocity);
		OutData.LinearVelocity = ToFVector(ss.HeadPose.LinearVelocity);
		OutData.TimeInSeconds  = ss.HeadPose.TimeInSeconds;
	}
}

bool FOculusRiftHMD::GetUserProfile(UserProfile& OutProfile) 
{
	InitDevice();
	FOvrSessionShared::AutoSession OvrSession(Session);
	if (Session->IsActive())
	{
		OutProfile.Name = ovr_GetString(OvrSession, OVR_KEY_USER, "");
		OutProfile.Gender = ovr_GetString(OvrSession, OVR_KEY_GENDER, OVR_DEFAULT_GENDER);
		OutProfile.PlayerHeight = ovr_GetFloat(OvrSession, OVR_KEY_PLAYER_HEIGHT, OVR_DEFAULT_PLAYER_HEIGHT);
		OutProfile.EyeHeight = ovr_GetFloat(OvrSession, OVR_KEY_EYE_HEIGHT, OVR_DEFAULT_EYE_HEIGHT);

		const FSettings* CurrentSettings = GetSettings();
		check(CurrentSettings)
		if (CurrentSettings->Flags.bOverrideIPD)
		{
			OutProfile.IPD = CurrentSettings->InterpupillaryDistance;
		}
		else
		{
			OutProfile.IPD = FMath::Abs(CurrentSettings->EyeRenderDesc[0].HmdToEyeOffset.x) + FMath::Abs(CurrentSettings->EyeRenderDesc[1].HmdToEyeOffset.x);
		}

		float neck2eye[2] = {OVR_DEFAULT_NECK_TO_EYE_HORIZONTAL, OVR_DEFAULT_NECK_TO_EYE_VERTICAL};
		ovr_GetFloatArray(OvrSession, OVR_KEY_NECK_TO_EYE_DISTANCE, neck2eye, 2);
		OutProfile.NeckToEyeDistance = FVector2D(neck2eye[0], neck2eye[1]);
		OutProfile.ExtraFields.Reset();
#if 0
		int EyeRelief = ovr_GetInt(OvrSession, OVR_KEY_EYE_RELIEF_DIAL, OVR_DEFAULT_EYE_RELIEF_DIAL);
		OutProfile.ExtraFields.Add(FString(OVR_KEY_EYE_RELIEF_DIAL)) = FString::FromInt(EyeRelief);
		
		float eye2nose[2] = { 0.f, 0.f };
		ovr_GetFloatArray(OvrSession, OVR_KEY_EYE_TO_NOSE_DISTANCE, eye2nose, 2);
		OutProfile.ExtraFields.Add(FString(OVR_KEY_EYE_RELIEF_DIAL"Horizontal")) = FString::SanitizeFloat(eye2nose[0]);
		OutProfile.ExtraFields.Add(FString(OVR_KEY_EYE_RELIEF_DIAL"Vertical")) = FString::SanitizeFloat(eye2nose[1]);

		float maxEye2Plate[2] = { 0.f, 0.f };
		ovr_GetFloatArray(OvrSession, OVR_KEY_MAX_EYE_TO_PLATE_DISTANCE, maxEye2Plate, 2);
		OutProfile.ExtraFields.Add(FString(OVR_KEY_MAX_EYE_TO_PLATE_DISTANCE"Horizontal")) = FString::SanitizeFloat(maxEye2Plate[0]);
		OutProfile.ExtraFields.Add(FString(OVR_KEY_MAX_EYE_TO_PLATE_DISTANCE"Vertical")) = FString::SanitizeFloat(maxEye2Plate[1]);

		OutProfile.ExtraFields.Add(FString(ovr_GetString(OvrSession, OVR_KEY_EYE_CUP, "")));
#endif
		return true;
	}
	return false; 
}

void FOculusRiftHMD::ApplySystemOverridesOnStereo(bool force)
{
	check(IsInGameThread());
	// ALWAYS SET r.FinishCurrentFrame to 0! Otherwise the perf might be poor.
	// @TODO: revise the FD3D11DynamicRHI::RHIEndDrawingViewport code (and other renderers)
	// to ignore this var completely.
 	static const auto CFinishFrameVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FinishCurrentFrame"));
 	CFinishFrameVar->Set(0);
}

void FOculusRiftHMD::SetTrackingOrigin(EHMDTrackingOrigin::Type InOrigin)
{
	switch (InOrigin)
	{
	case EHMDTrackingOrigin::Eye:
		OvrOrigin = ovrTrackingOrigin_EyeLevel;
		break;
	case EHMDTrackingOrigin::Floor:
		OvrOrigin = ovrTrackingOrigin_FloorLevel;
		break;
	default:
		UE_LOG(LogHMD, Error, TEXT("Unknown tracking origin type %d, defaulting to 'eye level'"), int(InOrigin));
		OvrOrigin = ovrTrackingOrigin_EyeLevel;
	}
	FOvrSessionShared::AutoSession OvrSession(Session);
	if (Session->IsActive())
	{
		ovr_SetTrackingOriginType(OvrSession, OvrOrigin);
		OCFlags.NeedSetTrackingOrigin = false;
	}
	else
	{
		OCFlags.NeedSetTrackingOrigin = true;
	}
}

EHMDTrackingOrigin::Type FOculusRiftHMD::GetTrackingOrigin()
{
	FOvrSessionShared::AutoSession OvrSession(Session);
	if (Session->IsActive())
	{
		OvrOrigin = ovr_GetTrackingOriginType(OvrSession);
	}
	EHMDTrackingOrigin::Type rv = EHMDTrackingOrigin::Eye;
	switch (OvrOrigin)
	{
	case ovrTrackingOrigin_EyeLevel:
		rv = EHMDTrackingOrigin::Eye;
		break;
	case ovrTrackingOrigin_FloorLevel:
		rv = EHMDTrackingOrigin::Floor;
		break;
	default:
		UE_LOG(LogHMD, Error, TEXT("Unsupported ovr tracking origin type %d"), int(OvrOrigin));
		break;
	}
	return rv;
}

//////////////////////////////////////////////////////////////////////////
FViewExtension::FViewExtension(FHeadMountedDisplay* InDelegate) 
	: FHMDViewExtension(InDelegate)
	, ShowFlags(ESFIM_All0)
	, bFrameBegun(false)
{
	auto OculusHMD = static_cast<FOculusRiftHMD*>(InDelegate);
	Session = OculusHMD->Session;

	pPresentBridge = OculusHMD->pCustomPresent;
}

#endif //OCULUS_RIFT_SUPPORTED_PLATFORMS
#endif //#if !PLATFORM_MAC

