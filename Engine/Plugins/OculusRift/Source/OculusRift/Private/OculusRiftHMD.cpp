// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IOculusRiftPlugin.h"
#include "HeadMountedDisplay.h"

#if PLATFORM_SUPPORTS_PRAGMA_PACK
#pragma pack (push,8)
#endif

#if OCULUS_RIFT_SUPPORTED_PLATFORMS
	#include "OVR.h"
	#include "OVRVersion.h"

	#ifdef OVR_VISION_ENABLED
	#include "../Src/Vision/Vision_PoseTracker.h"
	#endif // OVR_VISION_ENABLED

#endif //OCULUS_RIFT_SUPPORTED_PLATFORMS

#if PLATFORM_SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif

#include "OculusRiftHMD.h"

DEFINE_LOG_CATEGORY_STATIC(LogHMD, Log, All);

//#define PERF_LOGGING
#ifdef PERF_LOGGING
static uint32 __prev__prerentm = 0, __prerentm = 0;
static uint32 __applyhmdtm = 0, __prev__applyhmdtm = 0;
#endif

#if !UE_BUILD_SHIPPING
//////////////////////////////////////////////////////////////////////////
class OculusLog : public OVR::Log
{
public:
	OculusLog()
	{
		SetLoggingMask(OVR::LogMask_Debug | OVR::LogMask_Regular);
	}

	// This virtual function receives all the messages,
	// developers should override this function in order to do custom logging
	virtual void    LogMessageVarg(LogMessageType messageType, const char* fmt, va_list argList)
	{
		if ((messageType & GetLoggingMask()) == 0)
			return;

		ANSICHAR buf[1024];
		int32 len = FCStringAnsi::GetVarArgs(buf, sizeof(buf), sizeof(buf)/sizeof(ANSICHAR), fmt, argList);
		if (len > 0 && buf[len - 1] == '\n') // truncate the trailing new-line char, since Logf adds its own
			buf[len - 1] = '\0';
		TCHAR* tbuf = ANSI_TO_TCHAR(buf);
		GLog->Logf(TEXT("OCULUS: %s"), tbuf);
	}
};
#endif // #if !UE_BUILD_SHIPPING

//---------------------------------------------------
// Oculus Rift Plugin Implementation
//---------------------------------------------------

class FOculusRiftPlugin : public IOculusRiftPlugin
{
	/** IHeadMountedDisplayModule implementation */
	virtual TSharedPtr< class IHeadMountedDisplay > CreateHeadMountedDisplay() OVERRIDE;
};

IMPLEMENT_MODULE( FOculusRiftPlugin, OculusRift )

TSharedPtr< class IHeadMountedDisplay > FOculusRiftPlugin::CreateHeadMountedDisplay()
{
#if OCULUS_RIFT_SUPPORTED_PLATFORMS
	TSharedPtr< FOculusRiftHMD > OculusRiftHMD( new FOculusRiftHMD() );
	if( OculusRiftHMD->IsInitialized() )
	{
		return OculusRiftHMD;
	}
#endif//OCULUS_RIFT_SUPPORTED_PLATFORMS
	return NULL;
}

/** A handler to support hot-plug USB / HDMI messages from Oculus. */
class FOculusMessageHandler : public FTickableGameObject, public OVR::MessageHandler
{
public:
	// Message queue for device manager
	struct DeviceStatusNotificationDesc
	{
		OVR::DeviceHandle    Handle;
		OVR::MessageType     Action;

		DeviceStatusNotificationDesc() :Action(OVR::Message_None) {}
		DeviceStatusNotificationDesc(OVR::MessageType mt, const OVR::DeviceHandle& dev)
			: Handle(dev), Action(mt) {}
	};
	OVR::Array<DeviceStatusNotificationDesc> DeviceStatusNotificationsQueue;

	FOculusMessageHandler(FOculusRiftHMD* plugin) : pPlugin(plugin) 
	{
		check(pPlugin != NULL);
	}
	~FOculusMessageHandler()
	{
		check(this == pPlugin->pMsgHandler);
		pPlugin->pMsgHandler = NULL;
	}

	// FTickableObject
	virtual void Tick(float DeltaTime);
	virtual bool IsTickable() const;
	virtual bool IsTickableWhenPaused() const { return IsTickable(); }
	virtual TStatId GetStatId() const { return TStatId(); }

	// OVR::MessageHandler
	virtual void OnMessage(const OVR::Message& msg);

	void Clear() { DeviceStatusNotificationsQueue.Clear(); }

public: // data
	FOculusRiftHMD* pPlugin;
};

//////////////////////////////////////////////////////////////////////////
class ConditionalLocker
{
public:
	ConditionalLocker(bool condition, OVR::Lock* plock) :
		pLock((condition) ? plock : NULL)
	{
		OVR_ASSERT(!condition || pLock);
		if (pLock)
			pLock->DoLock();
	}
	~ConditionalLocker()
	{
		if (pLock)
			pLock->Unlock();
	}
private:
	OVR::Lock*	pLock;
};

//---------------------------------------------------
// Oculus Rift IHeadMountedDisplay Implementation
//---------------------------------------------------

#if OCULUS_RIFT_SUPPORTED_PLATFORMS

bool FOculusRiftHMD::IsHMDEnabled() const
{
	return bHMDEnabled;
}
void FOculusRiftHMD::EnableHMD(bool enable)
{
	bHMDEnabled = enable;
	if (!bHMDEnabled)
	{
		EnableStereo(false);
	}
}

EHMDDeviceType::Type FOculusRiftHMD::GetHMDDeviceType() const
{
	return EHMDDeviceType::DT_OculusRift;
}

bool FOculusRiftHMD::GetHMDMonitorInfo(MonitorInfo& MonitorDesc) const
{
	if (IsInitialized())
	{
		MonitorDesc.MonitorName = DeviceInfo.DisplayDeviceName;
		MonitorDesc.MonitorId	= DeviceInfo.DisplayId;
		MonitorDesc.DesktopX	= DeviceInfo.DesktopX;
		MonitorDesc.DesktopY	= DeviceInfo.DesktopY;
		MonitorDesc.ResolutionX = DeviceInfo.HResolution;
		MonitorDesc.ResolutionY = DeviceInfo.VResolution;
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

bool FOculusRiftHMD::DoesSupportPositionalTracking() const
{
#ifdef OVR_VISION_ENABLED
	 return bHmdPosTracking;
#else
	return false;
#endif //OVR_VISION_ENABLED
}

bool FOculusRiftHMD::HasValidTrackingPosition() const
{
#ifdef OVR_VISION_ENABLED
	return bHmdPosTracking && pSensorFusion && pSensorFusion->IsVisionPositionTracking();
#else
	return false;
#endif //OVR_VISION_ENABLED
}

#define CAMERA_HFOV			74.0f
#define CAMERA_VFOV			54.0f
#define CAMERA_MIN_DISTANCE 0.25f // meters
#define CAMERA_MAX_DISTANCE 1.80f // meters
#define CAMERA_DISTANCE		1.00f // meters (focal point to origin for position)

void FOculusRiftHMD::GetPositionalTrackingCameraProperties(FVector& OutOrigin, FRotator& OutOrientation, float& OutHFOV, float& OutVFOV, float& OutCameraDistance, float& OutNearPlane, float& OutFarPlane) const
{
	OutHFOV = CAMERA_HFOV;
	OutVFOV = CAMERA_VFOV;
	OutNearPlane  = CAMERA_MIN_DISTANCE * WorldToMetersScale;
	OutFarPlane   = CAMERA_MAX_DISTANCE * WorldToMetersScale;
	OutCameraDistance = CAMERA_DISTANCE * WorldToMetersScale;

	// correct position according to BaseOrientation and BaseOffset
	const FVector off = ToFVector_M2U(BaseOffset);

	const FQuat Orient = BaseOrientation.Inverse() * DeltaControlOrientation;
	OutOrientation = Orient.Rotator();

	// Calculate origin: where player's eyes are located in the world RELATIVELY to current player's location. This is where we'd
	// need to translate pre-rotated (using OutOrientation) frustum (again: plus player's current location).
	const FVector Origin = off - MetersToUU(FVector(CAMERA_DISTANCE, 0, 0)); // the focal point distance
	OutOrigin = FVector::ZeroVector - Orient.RotateVector(Origin);
}

bool FOculusRiftHMD::IsInLowPersistenceMode() const
{
	return bLowPersistenceMode;
}

void FOculusRiftHMD::EnableLowPersistenceMode(bool Enable)
{
#ifdef OVR_VISION_ENABLED
	bLowPersistenceMode = Enable;
	pSensor->SetLowPersistence(bLowPersistenceMode);
#endif
}

float FOculusRiftHMD::GetUserDistanceToScreenModifier() const
{
	return UserDistanceToScreenModifier;
}

void FOculusRiftHMD::SetUserDistanceToScreenModifier(float NewUserDistanceToScreenModifier)
{
	UserDistanceToScreenModifier = NewUserDistanceToScreenModifier;

	UpdateStereoRenderingParams();
}

float FOculusRiftHMD::GetInterpupillaryDistance() const
{
	return InterpupillaryDistance;
}

void FOculusRiftHMD::SetInterpupillaryDistance(float NewInterpupillaryDistance)
{
	InterpupillaryDistance = NewInterpupillaryDistance;

	UpdateStereoRenderingParams();
}

float FOculusRiftHMD::GetFieldOfView() const
{
	return FOV;
}

void FOculusRiftHMD::GetCurrentOrientationAndPosition(FQuat& CurrentOrientation, FVector& CurrentPosition) const
{
#ifdef OVR_VISION_ENABLED
	if (bHmdPosTracking)
	{
		OVR::Pose<double> pose = pSensorFusion->GetPoseRCF(pSensorFusion->GetPredictionDelta());
		
		CurrentOrientation = ToFQuat(pose.Orientation);

		// correct position according to BaseOrientation and BaseOffset
		CurrentPosition = BaseOrientation.Inverse().RotateVector(ToFVector_M2U(pose.Position - BaseOffset));

#if !UE_BUILD_SHIPPING
		bool hadVisionTracking = bHaveVisionTracking;
		bHaveVisionTracking = pSensorFusion->IsVisionPositionTracking();
		if (bHaveVisionTracking && !hadVisionTracking)
			UE_LOG(LogHMD, Warning, TEXT("Vision Tracking Acquired"));
		if (!bHaveVisionTracking && hadVisionTracking)
			UE_LOG(LogHMD, Warning, TEXT("Lost Vision Tracking"));

		static bool wasActive = true;
		if (wasActive && (!pPoseTracker || !pPoseTracker->IsActive()))
			UE_LOG(LogHMD, Warning, TEXT("Vision Tracking Thread Disabled"));
		wasActive = (pPoseTracker && pPoseTracker->IsActive());

		if (bPosTrackingSim)
		{
			// substitute positional info with the simulated one
			Vector3d headModel = GetHeadModel();
			headModel = pose.Orientation.Rotate(headModel);
			pose.Position = SimPosition + headModel;
			CurrentPosition = ToFVector_M2U(pose.Position);
		}
#endif // #if !UE_BUILD_SHIPPING

		CurrentPosition *= PositionScaleFactor;

		//UE_LOG(LogHMD, Warning, TEXT("OVRPos %.4f, %.4f, %.4f"), cr.P.x, cr.P.y, cr.P.z);
		//UE_LOG(LogHMD, Warning, TEXT("UE Pos %.4f, %.4f, %.4f"), CurrentPosition.X, CurrentPosition.Y, CurrentPosition.Z);
	}
	else
#endif // OVR_VISION_ENABLED
	{
		CurrentOrientation = ToFQuat(pSensorFusion->GetPredictedOrientation());
		CurrentPosition = FVector(0.f);
	}

	// apply base orientation correction to CurrentOrientation
	CurrentOrientation = BaseOrientation.Inverse() * CurrentOrientation;
	CurrentOrientation.Normalize();
}

void FOculusRiftHMD::UpdatePlayerViewPoint(const FQuat& CurrentOrientation, const FVector& CurrentPosition, 
										   const FQuat& BaseViewOrientation, const FVector& BaseViewPosition, 
										   FRotator& ViewRotation, FVector& ViewLocation)
{
	const FQuat DeltaOrient = BaseViewOrientation.Inverse() * CurrentOrientation;
	ViewRotation = FRotator(ViewRotation.Quaternion() * DeltaOrient);
}

void FOculusRiftHMD::ApplyHmdRotation(APlayerController* PC, FRotator& ViewRotation)
{
	ConditionalLocker lock(bUpdateOnRT, &UpdateOnRTLock);

#ifdef PERF_LOGGING
	__prev__applyhmdtm = __applyhmdtm;
	__applyhmdtm = OVR::Timer::GetTicksMs();
#endif

	// Temporary turn off this method if bUpdateOnRt is true.
	// 	if (bUpdateOnRT)
    //	return;

	ViewRotation.Normalize();

	GetCurrentOrientationAndPosition(CurHmdOrientation, CurHmdPosition);
	LastHmdOrientation = CurHmdOrientation;

	const FRotator DeltaRot = ViewRotation - PC->GetControlRotation();
	DeltaControlRotation = (DeltaControlRotation + DeltaRot).GetNormalized();

	// Pitch from other sources is never good, because there is an absolute up and down that must be respected to avoid motion sickness.
	// Same with roll.
	DeltaControlRotation.Pitch = 0;
	DeltaControlRotation.Roll = 0;
	DeltaControlOrientation = DeltaControlRotation.Quaternion();

	ViewRotation = FRotator(DeltaControlOrientation * CurHmdOrientation);

	// If updating on RenderThread is on then ProcessLatencyTesterInput() 
	// should be called there.
	if (!bUpdateOnRT)
	{
		// Process Latency tester input only if quad for the previous one 
		// was already rendered.
		if (!LatencyTestFrameNumber)
		{
			ProcessLatencyTesterInput();
			LatencyTestFrameNumber = GFrameNumber;
		}
	}

#if !UE_BUILD_SHIPPING
	if (bDrawTrackingCameraFrustum)
		DrawDebugTrackingCameraFrustum(PC->GetPawnOrSpectator()->GetPawnViewLocation());
#endif
}

void FOculusRiftHMD::UpdatePlayerCameraRotation(APlayerCameraManager* Camera, struct FMinimalViewInfo& POV)
{
	ConditionalLocker lock(bUpdateOnRT, &UpdateOnRTLock);

	GetCurrentOrientationAndPosition(CurHmdOrientation, CurHmdPosition);
	LastHmdOrientation = CurHmdOrientation;

	static const FName NAME_Fixed = FName(TEXT("Fixed"));
	static const FName NAME_ThirdPerson = FName(TEXT("ThirdPerson"));
	static const FName NAME_FreeCam = FName(TEXT("FreeCam"));
	static const FName NAME_FreeCam_Default = FName(TEXT("FreeCam_Default"));
	static const FName NAME_FirstPerson = FName(TEXT("FirstPerson"));

	DeltaControlRotation = POV.Rotation;
	DeltaControlOrientation = DeltaControlRotation.Quaternion();

	// Apply HMD orientation to camera rotation.
	POV.Rotation = FRotator(POV.Rotation.Quaternion() * CurHmdOrientation);

	// If updating on RenderThread is on then ProcessLatencyTesterInput() 
	// should be called there.
	if (!bUpdateOnRT)
	{
		// Process Latency tester input only if quad for the previous one 
		// was already rendered.
		if (!LatencyTestFrameNumber)
		{
			ProcessLatencyTesterInput();
			LatencyTestFrameNumber = GFrameNumber;
		}
	}

#if !UE_BUILD_SHIPPING
	if (bDrawTrackingCameraFrustum)
		DrawDebugTrackingCameraFrustum(POV.Location);
#endif
}

#if !UE_BUILD_SHIPPING
void FOculusRiftHMD::DrawDebugTrackingCameraFrustum(const FVector& ViewLocation)
{
	const FColor c = (HasValidTrackingPosition() ? FColor::Green : FColor::Red);
	FVector origin;
	FRotator rotation;
	float hfovDeg, vfovDeg, nearPlane, farPlane, cameraDist;
	GetPositionalTrackingCameraProperties(origin, rotation, hfovDeg, vfovDeg, cameraDist, nearPlane, farPlane);

	// Level line
	//DrawDebugLine(GWorld, ViewLocation, FVector(ViewLocation.X + 1000, ViewLocation.Y, ViewLocation.Z), FColor::Blue);

	const float hfov = Math<float>::DegreeToRadFactor * hfovDeg * 0.5f;
	const float vfov = Math<float>::DegreeToRadFactor * vfovDeg * 0.5f;
	FVector coneTop(0, 0, 0);
	FVector coneBase1(-farPlane, farPlane * FMath::Tan(hfov), farPlane * FMath::Tan(vfov));
	FVector coneBase2(-farPlane,-farPlane * FMath::Tan(hfov), farPlane * FMath::Tan(vfov));
	FVector coneBase3(-farPlane,-farPlane * FMath::Tan(hfov),-farPlane * FMath::Tan(vfov));
	FVector coneBase4(-farPlane, farPlane * FMath::Tan(hfov),-farPlane * FMath::Tan(vfov));
	FMatrix m(FMatrix::Identity);
	m *= FRotationMatrix(rotation);
	m *= FTranslationMatrix(origin);
	m *= FTranslationMatrix(ViewLocation); // to location of pawn
	coneTop = m.TransformPosition(coneTop);
	coneBase1 = m.TransformPosition(coneBase1);
	coneBase2 = m.TransformPosition(coneBase2);
	coneBase3 = m.TransformPosition(coneBase3);
	coneBase4 = m.TransformPosition(coneBase4);

	// draw a point at the camera pos
	DrawDebugPoint(GWorld, coneTop, 5, c);

	// draw main pyramid, from top to base
	DrawDebugLine(GWorld, coneTop, coneBase1, c);
	DrawDebugLine(GWorld, coneTop, coneBase2, c);
	DrawDebugLine(GWorld, coneTop, coneBase3, c);
	DrawDebugLine(GWorld, coneTop, coneBase4, c);
											  
	// draw base (far plane)				  
	DrawDebugLine(GWorld, coneBase1, coneBase2, c);
	DrawDebugLine(GWorld, coneBase2, coneBase3, c);
	DrawDebugLine(GWorld, coneBase3, coneBase4, c);
	DrawDebugLine(GWorld, coneBase4, coneBase1, c);

	// draw near plane
	FVector coneNear1(-nearPlane, nearPlane * FMath::Tan(hfov), nearPlane * FMath::Tan(vfov));
	FVector coneNear2(-nearPlane,-nearPlane * FMath::Tan(hfov), nearPlane * FMath::Tan(vfov));
	FVector coneNear3(-nearPlane,-nearPlane * FMath::Tan(hfov),-nearPlane * FMath::Tan(vfov));
	FVector coneNear4(-nearPlane, nearPlane * FMath::Tan(hfov),-nearPlane * FMath::Tan(vfov));
	coneNear1 = m.TransformPosition(coneNear1);
	coneNear2 = m.TransformPosition(coneNear2);
	coneNear3 = m.TransformPosition(coneNear3);
	coneNear4 = m.TransformPosition(coneNear4);
	DrawDebugLine(GWorld, coneNear1, coneNear2, c);
	DrawDebugLine(GWorld, coneNear2, coneNear3, c);
	DrawDebugLine(GWorld, coneNear3, coneNear4, c);
	DrawDebugLine(GWorld, coneNear4, coneNear1, c);

	// center line
	FVector centerLine(-cameraDist, 0, 0);
	centerLine = m.TransformPosition(centerLine);
	DrawDebugLine(GWorld, coneTop, centerLine, FColor::Yellow);
	DrawDebugPoint(GWorld, centerLine, 5, FColor::Yellow);
}
#endif // #if !UE_BUILD_SHIPPING

float FOculusRiftHMD::GetLensCenterOffset() const
{
	return LensCenterOffset;
}

float FOculusRiftHMD::GetDistortionScalingFactor() const
{
	return 1.0f / DistortionScale;
}

void FOculusRiftHMD::GetDistortionWarpValues(FVector4& K) const
{
	K = HmdDistortion;
}

bool FOculusRiftHMD::GetChromaAbCorrectionValues(FVector4& K) const
{
	if (bChromaAbCorrectionEnabled)
	{
		K = ChromaAbCorrection;
	}
	else
	{
		K = FVector4(0, 0, 0, 0);
	}
	return bChromaAbCorrectionEnabled;
}

bool FOculusRiftHMD::IsChromaAbCorrectionEnabled() const
{
	return bChromaAbCorrectionEnabled;
}

ISceneViewExtension* FOculusRiftHMD::GetViewExtension()
{
	return this;
}

bool FOculusRiftHMD::Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar )
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
		else if (FParse::Command( &Cmd, TEXT("RESET") ))
		{
			bOverrideStereo = false;
			bOverrideIPD = false;
			InterpupillaryDistance = DeviceInfo.InterpupillaryDistance;
			UpdateStereoRenderingParams();
			return true;
		}
		else if (FParse::Command(&Cmd, TEXT("SHOW")))
		{
			const FVector hm = ToFVector(GetHeadModel());
			Ar.Logf(TEXT("stereo ipd=%.4f headmodel={%.3f, %.3f, %.3f}\n   fov=%.3f dc=%.3f pc=%.3f"), GetInterpupillaryDistance(), 
				hm.X, hm.Y, hm.Z, FMath::RadiansToDegrees(FOV), LensCenterOffset, ProjectionCenterOffset);
		}

		// normal configuration
		float val;
		if (FParse::Value( Cmd, TEXT("E="), val))
		{
			SetInterpupillaryDistance( val );
			bOverrideIPD = true;
		}

		if(FParse::Value( Cmd, TEXT("HEADX="), val))
		{
			FVector hm = ToFVector(GetHeadModel());
			hm.X = val;
			SetHeadModel(ToOVRVector<OVR::Vector3d>(hm));
		}

		if(FParse::Value( Cmd, TEXT("HEADY="), val))
		{
			FVector hm = ToFVector(GetHeadModel());
			hm.Y = val;
			SetHeadModel(ToOVRVector<OVR::Vector3d>(hm));
		}

		if(FParse::Value( Cmd, TEXT("HEADZ="), val))
		{
			FVector hm = ToFVector(GetHeadModel());
			hm.Z = val;
			SetHeadModel(ToOVRVector<OVR::Vector3d>(hm));
		}
			
		// debug configuration
		if (bDevSettingsEnabled)
		{
			if (FParse::Value(Cmd, TEXT("PC="), ProjectionCenterOffset) ||
				FParse::Value(Cmd, TEXT("DC="), LensCenterOffset))
			{
				bOverrideStereo = true;
			}
			if (FParse::Value(Cmd, TEXT("FOV="), FOV))
			{
				FOV = FMath::DegreesToRadians(FOV);
				bOverrideStereo = true;
			}
		}
		return true;
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
		else if (FParse::Command(&Cmd, TEXT("VSYNC")))
		{
			if (FParse::Command( &Cmd, TEXT("RESET") ))
			{
				if (bStereoEnabled)
				{
					bVSync = bSavedVSync;
					ApplySystemOverridesOnStereo();
				}
				bOverrideVSync = false;
				return true;
			}
			else
			{
				if (FParse::Command(&Cmd, TEXT("ON")) || FParse::Command(&Cmd, TEXT("1")))
				{
					bVSync = true;
					bOverrideVSync = true;
					ApplySystemOverridesOnStereo();
					return true;
				}
				else if (FParse::Command(&Cmd, TEXT("OFF")) || FParse::Command(&Cmd, TEXT("0")))
				{
					bVSync = false;
					bOverrideVSync = true;
					ApplySystemOverridesOnStereo();
					return true;
				}
			}
			return false;
		}
		else if (FParse::Command(&Cmd, TEXT("SP")) || 
				 FParse::Command(&Cmd, TEXT("SCREENPERCENTAGE")))
		{
			FString CmdName = FParse::Token(Cmd, 0);
			if (CmdName.IsEmpty())
				return false;

			if (!FCString::Stricmp(*CmdName, TEXT("RESET")))
			{
				bOverrideScreenPercentage = false;
				ApplySystemOverridesOnStereo();
			}
			else
			{
				float sp = FCString::Atof(*CmdName);
				if (sp >= 30 && sp <= 300)
				{
					bOverrideScreenPercentage = true;
					ScreenPercentage = sp;
					ApplySystemOverridesOnStereo();
				}
				else
				{
					Ar.Logf(TEXT("Value is out of range [30..300]"));
				}
			}
			return true;
		}
#ifdef OVR_VISION_ENABLED
		else if (FParse::Command(&Cmd, TEXT("LP"))) // low persistence mode
		{
			FString CmdName = FParse::Token(Cmd, 0);
			if (!pSensor || CmdName.IsEmpty())
				return false;

			if (!FCString::Stricmp(*CmdName, TEXT("ON")))
			{
				bLowPersistenceMode = true;
			}
			else if (!FCString::Stricmp(*CmdName, TEXT("OFF")))
			{
				bLowPersistenceMode = false;
			}
			else if (!FCString::Stricmp(*CmdName, TEXT("TOGGLE")))
			{
				bLowPersistenceMode = !bLowPersistenceMode;
			}
			pSensor->SetLowPersistence(bLowPersistenceMode);
			return true;
		}
#endif // #ifdef OVR_VISION_ENABLED
		else if (FParse::Command(&Cmd, TEXT("UPDATEONRT"))) // update on renderthread
		{
			FString CmdName = FParse::Token(Cmd, 0);
			if (CmdName.IsEmpty())
				return false;

			if (!FCString::Stricmp(*CmdName, TEXT("ON")))
			{
				bUpdateOnRT = true;
			}
			else if (!FCString::Stricmp(*CmdName, TEXT("OFF")))
			{
				bUpdateOnRT = false;
			}
			else if (!FCString::Stricmp(*CmdName, TEXT("TOGGLE")))
			{
				bUpdateOnRT = !bUpdateOnRT;
			}
			return true;
		}
	}
	else if (FParse::Command(&Cmd, TEXT("HMDMAG")))
	{
		if (FParse::Command(&Cmd, TEXT("ON")))
		{
			pSensorFusion->SetYawCorrectionEnabled(true);
			return true;
		}
		else if (FParse::Command(&Cmd, TEXT("OFF")))
		{
			pSensorFusion->SetYawCorrectionEnabled(false);
			return true;
		}
		else if (FParse::Command(&Cmd, TEXT("SHOW")))
		{
			FString CalibState;
			if (pSensorFusion->HasMagCalibration())
			{
				CalibState = TEXT("calibrated");
			}
			Ar.Logf(TEXT("mag %s %s"), pSensorFusion->IsYawCorrectionEnabled() ? 
				TEXT("on") : TEXT("off"), *CalibState);
			return true;
		}
		return false;
	}
	else if (FParse::Command(&Cmd, TEXT("HMDTILT")))
	{
		if (FParse::Command(&Cmd, TEXT("ON")))
		{
			pSensorFusion->SetGravityEnabled(true);
			return true;
		}
		else if (FParse::Command(&Cmd, TEXT("OFF")))
		{
			pSensorFusion->SetGravityEnabled(false);
			return true;
		}
		else if (FParse::Command(&Cmd, TEXT("SHOW")))
		{
			Ar.Logf(TEXT("tilt correction %s"), pSensorFusion->IsGravityEnabled() ? 
				TEXT("on") : TEXT("off"));
			return true;
		}
		return false;
	}
	else if (FParse::Command(&Cmd, TEXT("HMDWARP")))
    {
        if (FParse::Command( &Cmd, TEXT("ON") ))
        {
            bHmdDistortion = true;
            return true;
        }
        else if (FParse::Command( &Cmd, TEXT("OFF") ))
        {
            bHmdDistortion = false;
            return true;
        }
		if (FParse::Command(&Cmd, TEXT("CHA")))
		{
			bChromaAbCorrectionEnabled = true;
		}
		else if (FParse::Command(&Cmd, TEXT("NOCHA")))
		{
			bChromaAbCorrectionEnabled = false;
		}

		if (FParse::Command(&Cmd, TEXT("RESET")))
		{
			HmdDistortion.X = DeviceInfo.DistortionK[0];
			HmdDistortion.Y = DeviceInfo.DistortionK[1];
			HmdDistortion.Z = DeviceInfo.DistortionK[2];
			HmdDistortion.W = DeviceInfo.DistortionK[3];
			ChromaAbCorrection.X = DeviceInfo.ChromaAbCorrection[0];
			ChromaAbCorrection.Y = DeviceInfo.ChromaAbCorrection[1];
			ChromaAbCorrection.Z = DeviceInfo.ChromaAbCorrection[2];
			ChromaAbCorrection.W = DeviceInfo.ChromaAbCorrection[3];
			bOverrideDistortion = false;
		}

		if (bDevSettingsEnabled)
		{
			bOverrideDistortion |= FParse::Value(Cmd, TEXT("1="), HmdDistortion.X);
			bOverrideDistortion |= FParse::Value(Cmd, TEXT("3="), HmdDistortion.Y);
			bOverrideDistortion |= FParse::Value(Cmd, TEXT("5="), HmdDistortion.Z);
			bOverrideDistortion |= FParse::Value(Cmd, TEXT("7="), HmdDistortion.W);
			//FParse::Value(Cmd, TEXT("C="), HmdDistortionOffset.X);
			//FParse::Value(Cmd, TEXT("V="), HmdDistortionOffset.Y);
			FParse::Value(Cmd, TEXT("S="), DistortionScale);
			float Adj;
			if (FParse::Value(Cmd, TEXT("1+="), Adj))      { HmdDistortion.X += Adj; bOverrideDistortion = true; }
			if (FParse::Value(Cmd, TEXT("3+="), Adj))      { HmdDistortion.Y += Adj; bOverrideDistortion = true; }
			if (FParse::Value(Cmd, TEXT("5+="), Adj))      { HmdDistortion.Z += Adj; bOverrideDistortion = true; }
			if (FParse::Value(Cmd, TEXT("7+="), Adj))      { HmdDistortion.W += Adj; bOverrideDistortion = true; }
			//if (FParse::Value(Cmd, TEXT("C+="), Adj))      { HmdDistortionOffset.X += Adj; }
		}

		if (FParse::Command(&Cmd, TEXT("SHOW")))
		{
			Ar.Logf(TEXT("hmdwarp 1=%.3f 3=%.3f 5=%.3f 7=%.3f c=%.3f v=%.3f s=%.3f %s"), HmdDistortion.X, HmdDistortion.Y, 
				HmdDistortion.Z, HmdDistortion.W, LensCenterOffset, 0.f, /*HmdDistortionOffset.X, HmdDistortionOffset.Y,*/
				DistortionScale, (bChromaAbCorrectionEnabled ? TEXT("cha") : TEXT("nocha")));
		}
		// centering changes with distortion on/off
		UpdateStereoRenderingParams();

		return true;
    }
	else if (FParse::Command(&Cmd, TEXT("HMDINFO")))
	{
		if (FParse::Command(&Cmd, TEXT("RESET")) && pHMD)
		{
			pHMD->GetDeviceInfo(&DeviceInfo);
			UpdateStereoRenderingParams();
			return true;
		}
		else if (FParse::Command(&Cmd, TEXT("SHOW")))
		{
			Ar.Logf(TEXT("hmdinfo ss=%.3fx%.3f esd=%.3f lsd=%.3f ipd=%.4f res=%dx%d"), DeviceInfo.HScreenSize, DeviceInfo.VScreenSize,
				DeviceInfo.EyeToScreenDistance, DeviceInfo.LensSeparationDistance, DeviceInfo.InterpupillaryDistance,
				DeviceInfo.HResolution, DeviceInfo.VResolution);
			return true;
		}
		else if (FParse::Command(&Cmd, TEXT("SET")))
		{
			if (bDevSettingsEnabled)
			{
				FParse::Value(Cmd, TEXT("ESD="), DeviceInfo.EyeToScreenDistance);
				FParse::Value(Cmd, TEXT("LSD="), DeviceInfo.LensSeparationDistance);
				FParse::Value(Cmd, TEXT("HSS="), DeviceInfo.HScreenSize);
				FParse::Value(Cmd, TEXT("VSS="), DeviceInfo.VScreenSize);
				int32 Res;
				if (FParse::Value(Cmd, TEXT("HRES="), Res))
				{
					DeviceInfo.HResolution = Res;
				}
				if (FParse::Value(Cmd, TEXT("VRES="), Res))
				{
					DeviceInfo.VResolution = Res;
				}
				UpdateStereoRenderingParams();
				return true;
			}
			else
				Ar.Logf(TEXT("Oculus dev settings mode is not enabled."));
		}
		return false;
	}
	else if (FParse::Command(&Cmd, TEXT("HMDPOS")))
	{
		if (FParse::Command(&Cmd, TEXT("RESET")))
		{
			FString YawStr = FParse::Token(Cmd, 0);
			float yaw = 0.f;
			if (!YawStr.IsEmpty())
			{
				yaw = FCString::Atof(*YawStr);
			}
			ResetOrientationAndPosition(yaw);
			return true;
		}
#ifdef OVR_VISION_ENABLED
		else if (FParse::Command(&Cmd, TEXT("ON")))
		{
			pSensorFusion->SetVisionPositionEnabled(true);
			return true;
		}
		else if (FParse::Command(&Cmd, TEXT("OFF")))
		{
			pSensorFusion->SetVisionPositionEnabled(false);
			return true;
		}
		else if (FParse::Command(&Cmd, TEXT("TOGGLE")))
		{
			pSensorFusion->SetVisionPositionEnabled(!pSensorFusion->IsVisionPositionEnabled());
			return true;
		}
		else if (FParse::Command(&Cmd, TEXT("ENABLE")))
		{
			bHmdPosTracking = true;
			return true;
		}
		else if (FParse::Command(&Cmd, TEXT("DISABLE")))
		{
			bHmdPosTracking = false;
			return true;
		}
		else if (FParse::Command(&Cmd, TEXT("SET")))
		{
			FParse::Value(Cmd, TEXT("SCALE="), PositionScaleFactor);
			return true;
		}
#if !UE_BUILD_SHIPPING
		else if (FParse::Command(&Cmd, TEXT("SHOWCAMERA")))
		{
			if (FParse::Command(&Cmd, TEXT("OFF")))
			{
				bDrawTrackingCameraFrustum = false;
				return true;
			}
			else if (FParse::Command(&Cmd, TEXT("TOGGLE")))
			{
				bDrawTrackingCameraFrustum = !bDrawTrackingCameraFrustum;
				return true;
			}
			bDrawTrackingCameraFrustum = true;
			return true;
		}
#endif // #if !UE_BUILD_SHIPPING
		else if (FParse::Command(&Cmd, TEXT("SHOW")))
		{
			FString camStatus;
			if (pPoseTracker && pPoseTracker->IsActive())
				camStatus = TEXT("running");
			else
				camStatus = TEXT("not running");
			FString hmdPosStatus;
			if (bHmdPosTracking)
			{
				if (pSensorFusion->IsVisionPositionEnabled())
					hmdPosStatus = "on";
				else
					hmdPosStatus = "enabled, but off";
			}
			else
				hmdPosStatus = "disabled";
			Ar.Logf(TEXT("hmdpos is %s, scale=%.4f, camera='%s', vis='%s'"), *hmdPosStatus, 
				PositionScaleFactor, *camStatus, 
				(pSensorFusion->IsVisionPositionTracking() ? TEXT("active") : TEXT("lost")));
			return true;
		}
#endif // #ifdef OVR_VISION_ENABLED
	}
	else if (FParse::Command(&Cmd, TEXT("OCULUSDEV")))
	{
		if (FParse::Command(&Cmd, TEXT("ON")))
		{
			bDevSettingsEnabled = true;
		}
		else if (FParse::Command(&Cmd, TEXT("OFF")))
		{
			bDevSettingsEnabled = false;
		}
		UpdateStereoRenderingParams();
		return true;
	}
	if (FParse::Command(&Cmd, TEXT("MOTION")))
	{
		FString CmdName = FParse::Token(Cmd, 0);
		if (CmdName.IsEmpty())
			return false;

		if (!FCString::Stricmp(*CmdName, TEXT("OFF")))
		{
			pSensorFusion->EnableMotionTracking(false);
			bHeadTrackingEnforced = false;
			return true;
		}
		else if (!FCString::Stricmp(*CmdName, TEXT("ON")))
		{
			pSensorFusion->EnableMotionTracking(true);
			bHeadTrackingEnforced = false;
			return true;
		}
		else if (!FCString::Stricmp(*CmdName, TEXT("ENFORCE")))
		{
			pSensorFusion->EnableMotionTracking(true);
			bHeadTrackingEnforced = true;
			return true;
		}
		else if (!FCString::Stricmp(*CmdName, TEXT("RESET")))
		{
			pSensorFusion->Reset();
			bHeadTrackingEnforced = false;
			CurHmdOrientation = FQuat::Identity;
			ResetControlRotation();
			return true;
		}
		else if (!FCString::Stricmp(*CmdName, TEXT("SHOW")))
		{
			if (pSensorFusion->IsPredictionEnabled())
			{
				Ar.Logf(TEXT("motion %s prediction=%.3f gain=%.3f"), (pSensorFusion->IsMotionTrackingEnabled()) ? TEXT("on") : TEXT("off"),
					pSensorFusion->GetPredictionDelta(), pSensorFusion->GetAccelGain());
			}
			else
			{
				Ar.Logf(TEXT("motion %s prediction OFF gain=%.3f"), (pSensorFusion->IsMotionTrackingEnabled()) ? TEXT("on") : TEXT("off"),
					pSensorFusion->GetAccelGain());
			}
	
			return true;
		}

		FString Value = FParse::Token(Cmd, 0);
		if (Value.IsEmpty())
		{
			return false;
		}
		if (!FCString::Stricmp(*CmdName, TEXT("GAIN")))
		{
			AccelGain = FCString::Atof(*Value);
			pSensorFusion->SetAccelGain(AccelGain);
			return true;
		}
		else if (!FCString::Stricmp(*CmdName, TEXT("PRED")))
		{
			if (!FCString::Stricmp(*Value, TEXT("OFF")))
				pSensorFusion->SetPredictionEnabled(false);
			else if (!FCString::Stricmp(*Value, TEXT("ON")))
				pSensorFusion->SetPredictionEnabled(true);
			else
			{
				MotionPrediction = FCString::Atof(*Value);
				pSensorFusion->SetPrediction(MotionPrediction, (MotionPrediction > 0));
			}
			return true;
		}

		return false;
	}
	else if (FParse::Command(&Cmd, TEXT("SETFINISHFRAME")))
	{
		static IConsoleVariable* CFinishFrameVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FinishCurrentFrame"));

		if (FParse::Command(&Cmd, TEXT("ON")))
		{
			bAllowFinishCurrentFrame = true;
			if (bStereoEnabled)
			{
				CFinishFrameVar->Set(bAllowFinishCurrentFrame);
			}
			return true;
		}
		else if (FParse::Command(&Cmd, TEXT("OFF")))
		{
			bAllowFinishCurrentFrame = false;
			if (bStereoEnabled)
			{
				CFinishFrameVar->Set(bAllowFinishCurrentFrame);
			}
			return true;
		}
		return false;
	}
	else if (FParse::Command(&Cmd, TEXT("UNCAPFPS")))
	{
		GEngine->bSmoothFrameRate = false;
		return true;
	}
	else if (FParse::Command(&Cmd, TEXT("OVRVERSION")))
	{
		static const char* Results = OVR_VERSION_STRING;
		Ar.Logf(TEXT("LibOVR: %s, Plugin built on %s, %s"), UTF8_TO_TCHAR(Results), 
			UTF8_TO_TCHAR(__DATE__), UTF8_TO_TCHAR(__TIME__));
		return true;
	}
#if !UE_BUILD_SHIPPING
	else if (FParse::Command(&Cmd, TEXT("OVRLATENCY")))
	{
		if (pLatencyTest && pLatencyTester)
		{
			const char* Results = pLatencyTest->GetResultsString();
			if (Results)
			{
				Ar.Logf(UTF8_TO_TCHAR(Results));
			}
		}
		else
		{
			Ar.Logf(TEXT("No latency tester connected"));
		}
		return true;
	}
#endif
	return false;
}

void FOculusRiftHMD::OnScreenModeChange(bool bFullScreenNow)
{
	EnableStereo(bFullScreenNow);
	UpdateStereoRenderingParams();
}

bool FOculusRiftHMD::IsPositionalTrackingEnabled() const
{
#ifdef OVR_VISION_ENABLED
	return bHmdPosTracking && ((pPoseTracker && pPoseTracker->IsActive())
#if !UE_BUILD_SHIPPING
		|| bPosTrackingSim
#endif
		);
#else
	return false;
#endif // #ifdef OVR_VISION_ENABLED
}

bool FOculusRiftHMD::EnablePositionalTracking(bool enable)
{
#ifdef OVR_VISION_ENABLED
	bHmdPosTracking = enable;
	return IsPositionalTrackingEnabled();
#else
	OVR_UNUSED(enable);
	return false;
#endif
}

OVR::Vector3d FOculusRiftHMD::GetHeadModel() const
{
#ifdef OVR_VISION_ENABLED
	return pSensorFusion->GetHeadModel();
#else
	return HeadModel_Meters;
#endif
}

void    FOculusRiftHMD::SetHeadModel(const OVR::Vector3d& hm)
{
#ifdef OVR_VISION_ENABLED
	pSensorFusion->SetHeadModel(hm);
#else
	HeadModel_Meters = hm;
#endif
}

//---------------------------------------------------
// Oculus Rift IStereoRendering Implementation
//---------------------------------------------------
bool FOculusRiftHMD::IsStereoEnabled() const
{
	return bStereoEnabled && bHMDEnabled;
}

bool FOculusRiftHMD::EnableStereo(bool stereo)
{
	bStereoEnabled = (IsHMDEnabled()) ? stereo : false;
	OnOculusStateChange(bStereoEnabled);
	return bStereoEnabled;
}

void FOculusRiftHMD::ResetControlRotation() const
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

void FOculusRiftHMD::OnOculusStateChange(bool bIsEnabledNow)
{
	bHmdDistortion = bIsEnabledNow;
	if (!bIsEnabledNow)
	{
		ResetControlRotation();
		RestoreSystemValues();
	}
	else
	{
		SaveSystemValues();
		ApplySystemOverridesOnStereo(bIsEnabledNow);
	}
	// need to distribute the event to user's code somehow... (!AB) @todo
}

void FOculusRiftHMD::ApplySystemOverridesOnStereo(bool bForce)
{
	if (bStereoEnabled || bForce)
	{
		// Set the current VSync state
		if (bOverrideVSync)
		{
			static IConsoleVariable* CVSyncVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync"));
			CVSyncVar->Set(bVSync);
		}

		static IConsoleVariable* CFinishFrameVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FinishCurrentFrame"));
		CFinishFrameVar->Set(bAllowFinishCurrentFrame);
	}
}

void FOculusRiftHMD::SaveSystemValues()
{
	static IConsoleVariable* CVSyncVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync"));
	bSavedVSync = CVSyncVar->GetInt() != 0;

	static IConsoleVariable* CScrPercVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"));
	SavedScrPerc = CScrPercVar->GetFloat();
}

void FOculusRiftHMD::RestoreSystemValues()
{
	static IConsoleVariable* CVSyncVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync"));
	CVSyncVar->Set(bSavedVSync);

	static IConsoleVariable* CScrPercVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"));
	CScrPercVar->Set(SavedScrPerc);

	static IConsoleVariable* CFinishFrameVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FinishCurrentFrame"));
	CFinishFrameVar->Set(false);
}

void FOculusRiftHMD::UpdateScreenSettings(const FViewport*)
{
	// Set the current ScreenPercentage state
	static IConsoleVariable* CScrPercVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"));
	if (bOverrideScreenPercentage)
	{
		CScrPercVar->Set(ScreenPercentage);
	}
	else
	{
		CScrPercVar->Set(DistortionScale*100.f);
	}
}

void FOculusRiftHMD::AdjustViewRect(EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const
{
    SizeX = SizeX / 2;
    if( StereoPass == eSSP_RIGHT_EYE )
    {
        X += SizeX;
    }
}

void FOculusRiftHMD::CalculateStereoViewOffset(const EStereoscopicPass StereoPassType, const FRotator& ViewRotation, const float WorldToMeters, FVector& ViewLocation) const
{
	ConditionalLocker lock(bUpdateOnRT, &UpdateOnRTLock);

	if( StereoPassType != eSSP_FULL)
	{
		check(WorldToMeters != 0.f)

        const float EyeOffset = (InterpupillaryDistance * 0.5f) * WorldToMeters;
		const float PassEyeOffset = (StereoPassType == eSSP_LEFT_EYE) ? -EyeOffset : EyeOffset;
		FVector TotalOffset = FVector(0,PassEyeOffset,0);
		if (!bHmdPosTracking)
		{
			if (bUseHeadModel)
			{
				const FVector hm = ToFVector_M2U(GetHeadModel());
				TotalOffset += hm;
				ViewLocation += ViewRotation.Quaternion().RotateVector(TotalOffset);
			}
		}
		else
		{
			ViewLocation += ViewRotation.Quaternion().RotateVector(TotalOffset);

			// The HMDPosition already has HMD orientation applied.
			// Apply rotational difference between HMD orientation and ViewRotation
			// to HMDPosition vector. 

			const FVector vHMDPosition = DeltaControlOrientation.RotateVector(CurHmdPosition);
			ViewLocation += vHMDPosition;
		}
	}
}

void FOculusRiftHMD::ResetOrientationAndPosition(float yaw)
{
#ifdef OVR_VISION_ENABLED
	// Reset position
	const OVR::Pose<double> pose = pSensorFusion->GetPoseRCF(0);
	BaseOffset = pose.Position;
	// Reset Yaw
	const OVR::Quatd orientation = pose.Orientation;
#else
	BaseOffset = OVR::Vector3d(0, 0, 0);
	// Reset Yaw
	const OVR::Quatd orientation = pSensorFusion->GetPredictedOrientationDouble(0);
#endif // #ifdef OVR_VISION_ENABLED

	FRotator ViewRotation;
	ViewRotation = FRotator(ToFQuat(orientation));
	ViewRotation.Pitch = 0;
	ViewRotation.Roll = 0;

	if (yaw != 0.f)
	{
		// apply optional yaw offset
		ViewRotation.Yaw -= yaw;
		ViewRotation.Normalize();
	}

	BaseOrientation = ViewRotation.Quaternion();
}

FMatrix FOculusRiftHMD::GetStereoProjectionMatrix(enum EStereoscopicPass StereoPassType, const float FOV) const
{
 	const float PassProjectionOffset = (StereoPassType == eSSP_LEFT_EYE) ? ProjectionCenterOffset : -ProjectionCenterOffset;
 
    const float TanHalfFov = FMath::Tan(FOV * 0.5f);
    const float Aspect     = 0.5f * float(DeviceInfo.HResolution) / float(DeviceInfo.VResolution);
    const float XS         = 1.0f / (Aspect * TanHalfFov);
    const float YS         = 1.0f / TanHalfFov;   
 
 	const float InNearZ = GNearClippingPlane;

  	return FMatrix(
  		FPlane(XS,      0.0f,       0.0f,   0.0f),
  		FPlane(0.0f,	YS,	        0.0f,	0.0f),
  		FPlane(0.0f,	0.0f,		0.0f,	1.0f),
  		FPlane(0.0f,	0.0f,		InNearZ, 0.0f))
  
  		* FTranslationMatrix(FVector(PassProjectionOffset,0,0));
}

void FOculusRiftHMD::InitCanvasFromView(FSceneView* InView, UCanvas* Canvas)
{
	// This is used for placing small HUDs (with names)
	// over other players (for example, in Capture Flag).
	// HmdOrientation should be initialized by GetCurrentOrientation (or
	// user's own value).
	FSceneView HmdView(*InView);

	UpdatePlayerViewPoint(Canvas->HmdOrientation, FVector(0.f), HmdView.BaseHmdOrientation, HmdView.BaseHmdLocation, HmdView.ViewRotation, HmdView.ViewLocation);

	HmdView.UpdateViewMatrix();
	Canvas->ViewProjectionMatrix = HmdView.ViewProjectionMatrix;
}

void FOculusRiftHMD::PushViewportCanvas(EStereoscopicPass StereoPass, FCanvas *InCanvas, UCanvas *InCanvasObject, FViewport *InViewport) const
{
	if (StereoPass != eSSP_FULL)
	{
		int32 SideSizeX = FMath::Trunc(InViewport->GetSizeXY().X * 0.5);

		// !AB: temporarily assuming all canvases are at Z = 1.0f and calculating
		// stereo disparity right here. Stereo disparity should be calculated for each
		// element separately, considering its actual Z-depth.
		const float Z = 1.0f;
		float Disparity = Z * HudOffset + Z * CanvasCenterOffset;
		if (StereoPass == eSSP_RIGHT_EYE)
			Disparity = -Disparity;

		if (InCanvasObject)
		{
			//InCanvasObject->Init();
			InCanvasObject->SizeX = SideSizeX;
			InCanvasObject->SizeY = InViewport->GetSizeXY().Y;
			InCanvasObject->SetView(NULL);
			InCanvasObject->Update();
		}

		InCanvas->PushAbsoluteTransform(FTranslationMatrix(
			FVector(((StereoPass == eSSP_RIGHT_EYE) ? SideSizeX : 0) + Disparity, 0, 0)));
	}
	else
	{
		FMatrix m;
		m.SetIdentity();
		InCanvas->PushAbsoluteTransform(m);
	}
}

void FOculusRiftHMD::PushViewCanvas(EStereoscopicPass StereoPass, FCanvas *InCanvas, UCanvas *InCanvasObject, FSceneView *InView) const
{
	if (StereoPass != eSSP_FULL)
	{
		if (InCanvasObject)
		{
			//InCanvasObject->Init();
			InCanvasObject->SizeX = InView->ViewRect.Width();
			InCanvasObject->SizeY = InView->ViewRect.Height();
			InCanvasObject->SetView(InView);
			InCanvasObject->Update();
		}

		InCanvas->PushAbsoluteTransform(FTranslationMatrix(FVector(InView->ViewRect.Min.X, InView->ViewRect.Min.Y, 0)));
	}
	else
	{
		FMatrix m;
		m.SetIdentity();
		InCanvas->PushAbsoluteTransform(m);
	}
}


//---------------------------------------------------
// Oculus Rift ISceneViewExtension Implementation
//---------------------------------------------------

void FOculusRiftHMD::ModifyShowFlags(FEngineShowFlags& ShowFlags)
{
	ShowFlags.MotionBlur = 0;
    ShowFlags.HMDDistortion = bHmdDistortion;
	ShowFlags.ScreenPercentage = true;
}

void FOculusRiftHMD::SetupView(FSceneView& InView)
{
	InView.BaseHmdOrientation = LastHmdOrientation;
	InView.BaseHmdLocation = FVector(0.f);
	WorldToMetersScale = InView.WorldToMetersScale;
}

void FOculusRiftHMD::ProcessLatencyTesterInput() const
{
#if !UE_BUILD_SHIPPING
	if (pLatencyTester && pLatencyTest && pLatencyTest->IsMeasuringNow())
	{
		OVR::Lock::Locker lock(&LatencyTestLock);
		pLatencyTest->ProcessInputs();
	}
#endif
}

void FOculusRiftHMD::PreRenderView_RenderThread(FSceneView& View)
{
#ifdef PERF_LOGGING
	if (View.StereoPass == eSSP_LEFT_EYE || View.StereoPass == eSSP_LEFT_EYE)
	{
		__prev__prerentm = __prerentm;
		__prerentm = OVR::Timer::GetTicksMs();
		UE_LOG(LogHMD, Warning, TEXT("GameThr %d, RenderThr %d, GameThr->RenThr %d"), 
			__applyhmdtm - __prev__applyhmdtm, __prerentm - __prev__prerentm,
			__prerentm - __applyhmdtm);
	}
#endif

	if (bUpdateOnRT)
	{
		OVR::Lock::Locker lock(&UpdateOnRTLock);
		if (View.StereoPass == eSSP_LEFT_EYE || View.StereoPass == eSSP_LEFT_EYE)
		{
			// Right eye will use the same orientation/position readings as left eye.
			GetCurrentOrientationAndPosition(CurHmdOrientation, CurHmdPosition);
	
			ProcessLatencyTesterInput();
			LatencyTestFrameNumber = GFrameNumberRenderThread;
		}

		UpdatePlayerViewPoint(CurHmdOrientation, CurHmdPosition,
			View.BaseHmdOrientation, View.BaseHmdLocation, 
			View.ViewRotation, View.ViewLocation);
		View.UpdateViewMatrix();
	}
}

bool FOculusRiftHMD::GetLatencyTesterColor_RenderThread(FColor& color, const FSceneView& view)
{
#if !UE_BUILD_SHIPPING
	if (pLatencyTest && pLatencyTester && pLatencyTest->IsMeasuringNow())
	{
		if (view.StereoPass == eSSP_LEFT_EYE || view.StereoPass == eSSP_FULL)
		{
			uint32 fr = GFrameNumberRenderThread;
			//UE_LOG(LogHMD, Warning, TEXT("fr %d lfr %d"), fr, LatencyTestFrameNumber);
			if (fr >= LatencyTestFrameNumber)
			{
				// Right eye will use the same color
				OVR::Lock::Locker lock(&LatencyTestLock);
				pLatencyTest->DisplayScreenColor(LatencyTestColor);
				LatencyTestFrameNumber = 0;
			}
		}
		color.R = LatencyTestColor.R;
		color.G = LatencyTestColor.G;
		color.B = LatencyTestColor.B;
		color.A = LatencyTestColor.A;
		return true;
	}
#endif
	return false;
}

bool FOculusRiftHMD::IsHeadTrackingAllowed() const
{
	return bHeadTrackingEnforced || GEngine->IsStereoscopic3D();
}

//---------------------------------------------------
// Oculus Rift Specific
//---------------------------------------------------

FOculusRiftHMD::FOculusRiftHMD()
	: bWasInitialized(false)
	, bStereoEnabled(false)
	, bHMDEnabled(true)
    , bOverrideStereo(false)
	, bOverrideIPD(false)
	, bOverrideDistortion(false)
	, bDevSettingsEnabled(false)
	, bOverrideVSync(true)
	, bVSync(true)
	, bSavedVSync(false)
	, SavedScrPerc(100.f)
	, bOverrideScreenPercentage(false)
	, ScreenPercentage(100.f)
	, bAllowFinishCurrentFrame(false)
	, InterpupillaryDistance(0.064f)
	, WorldToMetersScale(100.f)
	, UserDistanceToScreenModifier(0.f)
	, FOV(FMath::DegreesToRadians(90.f))
	, MotionPrediction(0.04f)
	, AccelGain(0.f)
	, bUseHeadModel(true)
	, bHmdDistortion(true)
	, HmdDistortion(0, 0, 0)
	, DistortionScale(1.f)
	, bChromaAbCorrectionEnabled(true)
	, ChromaAbCorrection(0, 0, 0)
	, ProjectionCenterOffset(0.f)
	, LensCenterOffset(0.f)
	, bOverride2D(false)
	, HudOffset(0.f)
	, CanvasCenterOffset(0.f)
	, bLowPersistenceMode(false)
	, bUpdateOnRT(true)
	, bHeadTrackingEnforced(false)
#if !UE_BUILD_SHIPPING
	, bDrawTrackingCameraFrustum(false)
#endif
	, LastHmdOrientation(FQuat::Identity)
	, CurHmdOrientation(FQuat::Identity)
	, DeltaControlRotation(FRotator::ZeroRotator)
	, DeltaControlOrientation(FQuat::Identity)
	, CurHmdPosition(FVector::ZeroVector)
	, BaseOffset(0, 0, 0)
	, BaseOrientation(FQuat::Identity)
	, pLatencyTest(NULL)
	, pSensorFusion(NULL)
	, pMsgHandler(NULL)
	, bHmdPosTracking(false)
#ifdef OVR_VISION_ENABLED
	, pPoseTracker(NULL)
	, PositionScaleFactor(1.f)
#if !UE_BUILD_SHIPPING
	, SimPosition(0.0, 0.0, 0.0)
	, bPosTrackingSim(false)
	, bHaveVisionTracking(false)
#endif // !UE_BUILD_SHIPPING
#else	
	, HeadModel_Meters(0, 0.17, -0.12)
#endif // OCULUS_USE_VISION
{
	LatencyTestFrameNumber = 0;
#ifdef OVR_VISION_ENABLED
	bHmdPosTracking = true;
#endif
	Startup();
}

FOculusRiftHMD::~FOculusRiftHMD()
{
	Shutdown();
}

bool FOculusRiftHMD::IsInitialized() const
{
	return bWasInitialized;
}

void FOculusRiftHMD::Startup()
{
	// --- Get the Oculus hardware devices ---
	System::Init();

#if !UE_BUILD_SHIPPING
	static OculusLog OcLog;
	OVR::Log::SetGlobalLog(&OcLog);
#endif

	pSensorFusion = new SensorFusion();
	if (!pSensorFusion)
	{
		UE_LOG(LogHMD, Warning, TEXT("Error creating Oculus sensor fusion."));
		return;
	}
	AccelGain = pSensorFusion->GetAccelGain();

	pDevManager = *DeviceManager::Create();
	if( !pDevManager )
	{
		UE_LOG(LogHMD, Warning, TEXT("Error creating Oculus device manager."));
		return;
	}

	// We'll handle it's messages in this case.
	pMsgHandler = new FOculusMessageHandler(this);
	pDevManager->SetMessageHandler(pMsgHandler);

	pHMD = *pDevManager->EnumerateDevices<HMDDevice>().CreateDevice();

	if( pHMD )
	{
		pSensor = *pHMD->GetSensor();

		// Assuming we've successfully grabbed the device, read the configuration data from it, which we'll use for projection
		pHMD->GetDeviceInfo(&DeviceInfo);
	}

#if !UE_BUILD_SHIPPING
	pLatencyTest = new Util::LatencyTest();
	// --- (Optional) Oculus Latency Tester device ---
	if (pLatencyTest)
	{
		pLatencyTester = *pDevManager->EnumerateDevices<LatencyTestDevice>().CreateDevice();
		if (pLatencyTester)
		{
			pLatencyTest->SetDevice(pLatencyTester);
		}
	}
#endif
	
	// --- Output device detection failure ---
	if( !pHMD )
	{
		UE_LOG(LogHMD, Warning, TEXT("No Oculus HMD detected!"));
	}

	if( !pSensor )
	{
		UE_LOG(LogHMD, Warning, TEXT("No Oculus Sensor detected!"));
	}

#if !UE_BUILD_SHIPPING
	if( !pLatencyTester )
	{
		UE_LOG(LogHMD, Log, TEXT("Oculus Latency Tester not found."));
	}
#endif

	if( pHMD != NULL )
	{
		if( pSensor )
		{
			pSensorFusion->AttachToSensor(pSensor);
			pSensorFusion->SetPrediction(MotionPrediction, (MotionPrediction > 0));
		}

        HmdDistortion.X = DeviceInfo.DistortionK[0];
        HmdDistortion.Y = DeviceInfo.DistortionK[1];
        HmdDistortion.Z = DeviceInfo.DistortionK[2];
        HmdDistortion.W = DeviceInfo.DistortionK[3];
		ChromaAbCorrection.X = DeviceInfo.ChromaAbCorrection[0];
		ChromaAbCorrection.Y = DeviceInfo.ChromaAbCorrection[1];
		ChromaAbCorrection.Z = DeviceInfo.ChromaAbCorrection[2];
		ChromaAbCorrection.W = DeviceInfo.ChromaAbCorrection[3];

		// Override eye distance by the value from HMDInfo (stored in Profile).
		InterpupillaryDistance = DeviceInfo.InterpupillaryDistance;

		bWasInitialized = true;
		UpdateStereoRenderingParams();
	}
	else
	{
		// Allow sensor by itself to be used for testing
		pSensor = *pDevManager->EnumerateDevices<SensorDevice>().CreateDevice();

		// Allow use with no headset detected
		memset(&DeviceInfo, 0, sizeof(DeviceInfo));
		DeviceInfo.HScreenSize = 0.14976f;
		DeviceInfo.VScreenSize = 0.0936f;
		DeviceInfo.HResolution = 1280;
		DeviceInfo.VResolution = 800;
		DeviceInfo.InterpupillaryDistance = 0.064f;
		HmdDistortion.X = 1;
		HmdDistortion.Y = 0.18f;
		HmdDistortion.Z = 0.115f;
		HmdDistortion.W = 0;
		//HmdDistortionOffset.X = 0;
		//HmdDistortionOffset.Y = 0;
		ChromaAbCorrection.X = 1;
		ChromaAbCorrection.Y = 0;
		ChromaAbCorrection.Z = 1;
		ChromaAbCorrection.W = 0;
	}

#ifdef OVR_VISION_ENABLED
	//pPoseTracker = NULL;
	pPoseTracker = new OVR::Vision::PoseTracker(*pSensorFusion);
	pCamera = *pDevManager->EnumerateDevices<CameraDevice>().CreateDevice();
	if (pCamera)
	{
		pCamera->SetMessageHandler(pPoseTracker);
		pPoseTracker->AssociateCamera(pCamera);
	}
	if (pSensor)
		pPoseTracker->AssociateHMD(pSensor);
#endif // OCULUS_USE_VISION

	LoadFromIni();
	SaveSystemValues();
}

void FOculusRiftHMD::Shutdown()
{
	SaveToIni();

#ifdef OVR_VISION_ENABLED
	delete pPoseTracker; pPoseTracker = NULL;
	pCamera.Clear();
#endif // OCULUS_USE_VISION

	pSensor.Clear();
	pHMD.Clear();

	delete pSensorFusion; pSensorFusion = NULL;

#if !UE_BUILD_SHIPPING
	// cleanup latency tester under lock, since it could be accessed from
	// another thread.
	{
		OVR::Lock::Locker lock(&LatencyTestLock);
		pLatencyTester.Clear();
		delete pLatencyTest; pLatencyTest = NULL;
	}
#endif

	bWasInitialized = false;

	pDevManager.Clear();

	if (pMsgHandler)
	{
		pMsgHandler->RemoveHandlerFromDevices();
		delete pMsgHandler; pMsgHandler = NULL;
	}
	System::Destroy();
	UE_LOG(LogHMD, Log, TEXT("Oculus shutdown."));
}

void FOculusRiftHMD::LoadFromIni()
{
	if (pSensorFusion)
	{
		const TCHAR* OculusSettings = TEXT("Oculus.Settings");
		bool v;
		float f;
		if (GConfig->GetBool(OculusSettings, TEXT("bChromaAbCorrectionEnabled"), v, GEngineIni))
		{
			bChromaAbCorrectionEnabled = v;
		}
		if (GConfig->GetBool(OculusSettings, TEXT("bMagEnabled"), v, GEngineIni))
		{
			pSensorFusion->SetYawCorrectionEnabled(v);
		}
		if (GConfig->GetBool(OculusSettings, TEXT("bDevSettingsEnabled"), v, GEngineIni))
		{
			bDevSettingsEnabled = v;
		}
		if (GConfig->GetBool(OculusSettings, TEXT("bMotionPredictionEnabled"), v, GEngineIni))
		{
			pSensorFusion->SetPredictionEnabled(v);
		}
		if (GConfig->GetBool(OculusSettings, TEXT("bTiltCorrectionEnabled"), v, GEngineIni))
		{
			pSensorFusion->SetGravityEnabled(v);
		}
		if (GConfig->GetFloat(OculusSettings, TEXT("AccelGain"), f, GEngineIni))
		{
			pSensorFusion->SetAccelGain(f);
		}
		if (GConfig->GetFloat(OculusSettings, TEXT("MotionPrediction"), f, GEngineIni))
		{
			pSensorFusion->SetPrediction(f, pSensorFusion->IsPredictionEnabled());
		}
		FVector vec;
		if (GConfig->GetVector(OculusSettings, TEXT("HeadModel_v2"), vec, GEngineIni))
		{
			SetHeadModel(ToOVRVector<OVR::Vector3d>(vec));
		}
		if (GConfig->GetBool(OculusSettings, TEXT("bOverrideIPD"), v, GEngineIni))
		{
			bOverrideIPD = v;
			if (bOverrideIPD)
			{
				if (GConfig->GetFloat(OculusSettings, TEXT("IPD"), f, GEngineIni))
				{
					SetInterpupillaryDistance(f);
				}
			}
		}
		if (GConfig->GetBool(OculusSettings, TEXT("bOverrideStereo"), v, GEngineIni))
		{
			bOverrideStereo = v;
			if (bOverrideStereo)
			{
				if (GConfig->GetFloat(OculusSettings, TEXT("ProjectionCenterOffset"), f, GEngineIni))
				{
					ProjectionCenterOffset = f;
				}
				if (GConfig->GetFloat(OculusSettings, TEXT("LensCenterOffset"), f, GEngineIni))
				{
					LensCenterOffset = f;
				}
				if (GConfig->GetFloat(OculusSettings, TEXT("FOV"), f, GEngineIni))
				{
					FOV = f;
				}
			}
		}
		if (GConfig->GetBool(OculusSettings, TEXT("bOverrideVSync"), v, GEngineIni))
		{
			bOverrideVSync = v;
			if (GConfig->GetBool(OculusSettings, TEXT("bVSync"), v, GEngineIni))
			{
				bVSync = v;
			}
		}
		if (GConfig->GetBool(OculusSettings, TEXT("bOverrideScreenPercentage"), v, GEngineIni))
		{
			bOverrideScreenPercentage = v;
			if (GConfig->GetFloat(OculusSettings, TEXT("ScreenPercentage"), f, GEngineIni))
			{
				ScreenPercentage = f;
			}
		}
		if (GConfig->GetBool(OculusSettings, TEXT("bAllowFinishCurrentFrame"), v, GEngineIni))
		{
			bAllowFinishCurrentFrame = v;
		}
#ifdef OVR_VISION_ENABLED
		if (GConfig->GetFloat(OculusSettings, TEXT("PositionScaleFactor"), f, GEngineIni))
		{
			PositionScaleFactor = f;
		}
		if (GConfig->GetBool(OculusSettings, TEXT("bHmdPosTracking"), v, GEngineIni))
		{
			bHmdPosTracking = v;
		}
		if (GConfig->GetBool(OculusSettings, TEXT("bLowPersistenceMode"), v, GEngineIni))
		{
			bLowPersistenceMode = v;
			if (pSensor)
				pSensor->SetLowPersistence(bLowPersistenceMode);
		}
#endif		
		if (GConfig->GetBool(OculusSettings, TEXT("bUpdateOnRT"), v, GEngineIni))
		{
			bUpdateOnRT = v;
		}
	}
}

void FOculusRiftHMD::SaveToIni()
{
	if (pSensorFusion)
	{
		const TCHAR* OculusSettings = TEXT("Oculus.Settings");
		GConfig->SetBool(OculusSettings, TEXT("bChromaAbCorrectionEnabled"), bChromaAbCorrectionEnabled, GEngineIni);
		GConfig->SetBool(OculusSettings, TEXT("bMagEnabled"), pSensorFusion->IsYawCorrectionEnabled(), GEngineIni);
		GConfig->SetBool(OculusSettings, TEXT("bDevSettingsEnabled"), bDevSettingsEnabled, GEngineIni);
		GConfig->SetBool(OculusSettings, TEXT("bMotionPredictionEnabled"), pSensorFusion->IsPredictionEnabled(), GEngineIni);
		GConfig->SetBool(OculusSettings, TEXT("bTiltCorrectionEnabled"), pSensorFusion->IsGravityEnabled(), GEngineIni);
		GConfig->SetFloat(OculusSettings, TEXT("AccelGain"), pSensorFusion->GetAccelGain(), GEngineIni);
		GConfig->SetFloat(OculusSettings, TEXT("MotionPrediction"), pSensorFusion->GetPredictionDelta(), GEngineIni);

		const OVR::Vector3d hm = GetHeadModel();
		GConfig->SetVector(OculusSettings, TEXT("HeadModel_v2"), ToFVector(hm), GEngineIni);

		GConfig->SetBool(OculusSettings, TEXT("bOverrideIPD"), bOverrideIPD, GEngineIni);
		if (bOverrideIPD)
		{
			GConfig->SetFloat(OculusSettings, TEXT("IPD"), GetInterpupillaryDistance(), GEngineIni);
		}
		GConfig->SetBool(OculusSettings, TEXT("bOverrideStereo"), bOverrideStereo, GEngineIni);
		if (bOverrideStereo)
		{
			GConfig->SetFloat(OculusSettings, TEXT("ProjectionCenterOffset"), ProjectionCenterOffset, GEngineIni);
			GConfig->SetFloat(OculusSettings, TEXT("LensCenterOffset"), LensCenterOffset, GEngineIni);
			GConfig->SetFloat(OculusSettings, TEXT("FOV"), FOV, GEngineIni);
		}

		GConfig->SetBool(OculusSettings, TEXT("bOverrideVSync"), bOverrideVSync, GEngineIni);
		if (bOverrideVSync)
		{
			GConfig->SetBool(OculusSettings, TEXT("VSync"), bVSync, GEngineIni);
		}

		GConfig->SetBool(OculusSettings, TEXT("bOverrideScreenPercentage"), bOverrideScreenPercentage, GEngineIni);
		if (bOverrideScreenPercentage)
		{
			// Save the current ScreenPercentage state
			GConfig->SetFloat(OculusSettings, TEXT("ScreenPercentage"), ScreenPercentage, GEngineIni);
		}
		GConfig->SetBool(OculusSettings, TEXT("bAllowFinishCurrentFrame"), bAllowFinishCurrentFrame, GEngineIni);

#ifdef OVR_VISION_ENABLED
		GConfig->SetFloat(OculusSettings, TEXT("PositionScaleFactor"), PositionScaleFactor, GEngineIni);
		GConfig->SetBool(OculusSettings, TEXT("bHmdPosTracking"), bHmdPosTracking, GEngineIni);
		GConfig->SetBool(OculusSettings, TEXT("bLowPersistenceMode"), bLowPersistenceMode, GEngineIni);
#endif

		GConfig->SetBool(OculusSettings, TEXT("bUpdateOnRT"), bUpdateOnRT, GEngineIni);
	}
}

void FOculusRiftHMD::UpdateStereoRenderingParams()
{
	// If we've manually overridden stereo rendering params for debugging, don't mess with them
    if( bOverrideStereo )
    {
        return;
    }

	if( IsInitialized() )
	{
        // Recenter projection (meters)
        const float LeftProjCenterM = DeviceInfo.HScreenSize * 0.25f;        
        const float LensRecenterM   = LeftProjCenterM - DeviceInfo.LensSeparationDistance * 0.5f;

        // Recenter projection (normalized)
        const float LensRecenter    = 4.0f * LensRecenterM / DeviceInfo.HScreenSize;

        // Distortion
		ProjectionCenterOffset = LensRecenter;
        LensCenterOffset = LensRecenter;
		if (!bOverrideDistortion || !bDevSettingsEnabled)
		{
			// Configure proper Distortion Fit.
			// For 7" screen, fit to touch left side of the view, leaving a bit of
			// invisible screen on the top (saves on rendering cost).
			// For smaller screens (5.5"), fit to the top.
			float DistortionFitX = -1.0f;
			float DistortionFitY = 0.0f;
			if (DeviceInfo.HScreenSize > 0.0f)
			{
				if (DeviceInfo.HScreenSize <= 0.140f)  // < 7"
				{
					DistortionFitX = 0.0f;
					DistortionFitY = 1.0f;
				}
			}

			float stereoAspect = 0.5f * float(DeviceInfo.HScreenSize) / float(DeviceInfo.VScreenSize);
			float dx = DistortionFitX - LensCenterOffset;
			float dy = DistortionFitY / stereoAspect;
			float fitRadius = FMath::Sqrt(dx * dx + dy * dy);
			DistortionScale = CalcDistortionScale(fitRadius);
		}

        // FOV
        const float DistortedEyeScreenDistM = DeviceInfo.VScreenSize * DistortionScale;
        FOV = (float)(2 * FMath::Atan(0.5f * DistortedEyeScreenDistM / (DeviceInfo.EyeToScreenDistance + UserDistanceToScreenModifier)));

		// 2D elements offset
		if (!bOverride2D)
		{
			HudOffset = 0.25f * InterpupillaryDistance * (DeviceInfo.HResolution / DeviceInfo.HScreenSize) / 15.0f;
			CanvasCenterOffset = (0.25f * LensCenterOffset) * DeviceInfo.HResolution;
		}
	}
	else
	{
		ProjectionCenterOffset = 0.f;
        LensCenterOffset = 0.f;
		CanvasCenterOffset = 0.f;
	}
}

float FOculusRiftHMD::CalcDistortionScale(float InScale)
{
	const float Scale2  = InScale * InScale;
	const float Scale = InScale * (HmdDistortion.X + HmdDistortion.Y * Scale2 + HmdDistortion.Z * Scale2*Scale2 + HmdDistortion.W * Scale2*Scale2*Scale2);
	return Scale / InScale;
}

#if !UE_BUILD_SHIPPING
bool FOculusRiftHMD::HandleInputKey(UPlayerInput* pPlayerInput, FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad)
{
#if 0 //def OVR_VISION_ENABLED
	//UE_LOG(LogHMD, Log, TEXT("Key %d, %f, gamepad %d"), (int)Key, AmountDepressed, (int)bGamepad);
	if (bGamepad && EventType == IE_Pressed)
	{
		if (Key == EKeys::Gamepad_RightThumbstick)
		{
			// toggles mode for the right stick
			bPosTrackingSim = !bPosTrackingSim;
			SimPosition.x = SimPosition.y = SimPosition.z = 0;
		}
		else if (   Key == EKeys::Gamepad_RightStick_Up
			|| Key == EKeys::Gamepad_RightStick_Down
			|| Key == EKeys::Gamepad_RightStick_Left
			|| Key == EKeys::Gamepad_RightStick_Right )
		{
			if (bPosTrackingSim)
			{
				return true;
			}
		}
	}
#endif
	return false;
}

bool FOculusRiftHMD::HandleInputAxis(UPlayerInput*, FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad)
{
#if 0 //def OVR_VISION_ENABLED
	// by default, Delta is changing in [-1..1] interval.
	const float InputAxisScale = 0.5f;  // make it [-0.5 .. +0.5] to simulate pos tracker input

	//	UE_LOG(LogHMD, Log, TEXT("Key %d, delta %f, time %f, samples %d "), 
	//		(int)Key, Delta, DeltaTime, NumSamples);
	if (bGamepad && bPosTrackingSim)
	{
		if (Key == EKeys::Gamepad_LeftX)
		{
			return true;
		}
		else if (Key == EKeys::Gamepad_LeftY)
		{
			// up - down, Oculus right-handed XYZ coord system.
			SimPosition.y = float(Delta * InputAxisScale);
			return true;
		}
		else if (Key == EKeys::Gamepad_RightX)
		{
			// left - right, Oculus right-handed XYZ coord system.
			SimPosition.x = float(Delta * InputAxisScale);
			return true;
		}
		else if (Key == EKeys::Gamepad_RightY)
		{
			// forward - backward, Oculus right-handed XYZ coord system.
			SimPosition.z = float(Delta * InputAxisScale);
			return true;
		}
	}
#endif
	return false;
}
#endif //!UE_BUILD_SHIPPING

/************************************************************************/
/* FOculusMessageHandler                                                */
/************************************************************************/
void FOculusMessageHandler::OnMessage(const OVR::Message& msg)
{
    if (msg.Type == Message_DeviceAdded || msg.Type == Message_DeviceRemoved)
    {
        if (msg.pDevice == pPlugin->pDevManager)
        {
            const MessageDeviceStatus& statusMsg =
                 static_cast<const MessageDeviceStatus&>(msg);

            { // limit the scope of the lock
                OVR::Lock::Locker lock(pPlugin->pDevManager->GetHandlerLock());
                DeviceStatusNotificationsQueue.PushBack(
                    DeviceStatusNotificationDesc(statusMsg.Type, statusMsg.Handle));
            }

            switch (statusMsg.Type)
            {
            case OVR::Message_DeviceAdded:
                UE_LOG(LogHMD, Warning, TEXT("DeviceManager reported device added."));
                break;

            case OVR::Message_DeviceRemoved:
                UE_LOG(LogHMD, Warning, TEXT("DeviceManager reported device removed."));
                break;

            default: ;
            }
        }
    }
}

void  FOculusMessageHandler::Tick(float)
{
    // Check if any new devices were connected.
    bool queueIsEmpty = false;
    while (!queueIsEmpty)
    {
        DeviceStatusNotificationDesc desc;

        {
            OVR::Lock::Locker lock(pPlugin->pDevManager->GetHandlerLock());
            if (DeviceStatusNotificationsQueue.GetSize() == 0)
                break;
            desc = DeviceStatusNotificationsQueue.Front();

            // We can't call Clear under the lock since this may introduce a dead lock:
            // this thread is locked by HandlerLock and the Clear might cause 
            // call of Device->Release, which will use Manager->DeviceLock. The bkg
            // thread is most likely locked by opposite way: 
            // Manager->DeviceLock ==> HandlerLock, therefore - a dead lock.
            // So, just grab the first element, save a copy of it and remove
            // the element (Device->Release won't be called since we made a copy).

            DeviceStatusNotificationsQueue.RemoveAt(0);
            queueIsEmpty = (DeviceStatusNotificationsQueue.GetSize() == 0);
        }

        bool wasAlreadyCreated = desc.Handle.IsCreated();

        if (desc.Action == OVR::Message_DeviceAdded)
        {
            switch(desc.Handle.GetType())
            {
            case Device_Sensor:
                if (desc.Handle.IsAvailable() && !desc.Handle.IsCreated())
                {
                    if (!pPlugin->pSensor)
                    {
                        pPlugin->pSensor = *desc.Handle.CreateDeviceTyped<OVR::SensorDevice>();
                        pPlugin->pSensorFusion->AttachToSensor(pPlugin->pSensor);

                        UE_LOG(LogHMD, Warning, TEXT("Sensor reported device added."));
                    }
                    else if (!wasAlreadyCreated)
                    {
                        UE_LOG(LogHMD, Warning, TEXT("A new SENSOR has been detected, but it is not currently used."));
                    }
                }
                break;
            case Device_LatencyTester:
#if !UE_BUILD_SHIPPING
                if (desc.Handle.IsAvailable() && !desc.Handle.IsCreated())
                {
                    if (!pPlugin->pLatencyTester)
                    {
                        pPlugin->pLatencyTester = *desc.Handle.CreateDeviceTyped<OVR::LatencyTestDevice>();
                        pPlugin->pLatencyTest->SetDevice(pPlugin->pLatencyTester);
                        if (!wasAlreadyCreated)
                        {
                            UE_LOG(LogHMD, Warning, TEXT("Latency Tester reported device added."));
                        }
                    }
                }
#endif // #if !UE_BUILD_SHIPPING
                break;
            case Device_HMD:
                {
                    OVR::HMDInfo info;
                    desc.Handle.GetDeviceInfo(&info);
                    // if strlen(info.DisplayDeviceName) == 0 then
                    // this HMD is 'fake' (created using sensor).
                    if (strlen(info.DisplayDeviceName) > 0 && (!pPlugin->pHMD || !info.IsSameDisplay(pPlugin->DeviceInfo)))
                    {
                        /*if (!pHMD || !desc.Handle.IsDevice(pHMD))
                            pHMD = *desc.Handle.CreateDeviceTyped<HMDDevice>();
                        // update stereo config with new HMDInfo
                        if (pHMD && pHMD->GetDeviceInfo(&TheHMDInfo))
                        {
                            //RenderParams.MonitorName = hmd.DisplayDeviceName;
                            SConfig.SetHMDInfo(TheHMDInfo);
                        }*/
                        UE_LOG(LogHMD, Warning, TEXT("HMD device added."));
                    }
                    break;
                }
            default:;
            }
        }
        else if (desc.Action == OVR::Message_DeviceRemoved)
        {
            if (desc.Handle.IsDevice(pPlugin->pSensor))
            {
                UE_LOG(LogHMD, Warning, TEXT("Sensor reported device removed."));
                pPlugin->pSensorFusion->AttachToSensor(NULL);
                pPlugin->pSensor.Clear();
                //GOculus->eventOnDeviceStatusChange(OCDev_TrackerSensor, OCDevSt_Detached);
            }
#if !UE_BUILD_SHIPPING
            else if (desc.Handle.IsDevice(pPlugin->pLatencyTester))
            {
                UE_LOG(LogHMD, Warning, TEXT("Latency Tester reported device removed."));
                pPlugin->pLatencyTest->SetDevice(NULL);
                pPlugin->pLatencyTester.Clear();
                //GOculus->eventOnDeviceStatusChange(OCDev_LatencyTester, OCDevSt_Detached);
            }
#endif // #if !UE_BUILD_SHIPPING
            else if (desc.Handle.IsDevice(pPlugin->pHMD))
            {
                if (pPlugin->pHMD && !pPlugin->pHMD->IsDisconnected())
                {
                    /*
                    // Disconnect HMD. pSensor is used to restore 'fake' HMD device
                    // (can be NULL).
                    pHMD = pHMD->Disconnect(pSensor);

                    // This will initialize TheHMDInfo with information about configured IPD,
                    // screen size and other variables needed for correct projection.
                    // We pass HMD DisplayDeviceName into the renderer to select the
                    // correct monitor in full-screen mode.
                    if (pHMD && pHMD->GetDeviceInfo(&TheHMDInfo))
                    {
                        //RenderParams.MonitorName = hmd.DisplayDeviceName;
                        SConfig.SetHMDInfo(TheHMDInfo);
                    }*/
                    UE_LOG(LogHMD, Warning, TEXT("HMD device removed."));
                }
            }
        }
    }
}

bool FOculusMessageHandler::IsTickable() const
{
    OVR::Lock::Locker lock(pPlugin->pDevManager->GetHandlerLock());
    return (DeviceStatusNotificationsQueue.GetSize() != 0);
}
#endif //OCULUS_RIFT_SUPPORTED_PLATFORMS
