// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OculusRiftPrivate.h"
#include "OculusRiftHMD.h"

#define DEFAULT_PREDICTION_IN_SECONDS 0.035



//---------------------------------------------------
// Oculus Rift Plugin Implementation
//---------------------------------------------------

class FOculusRiftPlugin : public IOculusRiftPlugin
{
	/** IHeadMountedDisplayModule implementation */
	virtual TSharedPtr< class IHeadMountedDisplay > CreateHeadMountedDisplay() override;
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
		MonitorDesc.MonitorName = HmdDesc.ProductName;//?
		MonitorDesc.MonitorId	= 0; //?
		MonitorDesc.DesktopX	= HmdDesc.WindowsPos.x;
		MonitorDesc.DesktopY	= HmdDesc.WindowsPos.y;
		MonitorDesc.ResolutionX = HmdDesc.Resolution.w;
		MonitorDesc.ResolutionY = HmdDesc.Resolution.h;
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
	return bHmdPosTracking && bHaveVisionTracking;
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
	bLowPersistenceMode = Enable;
	UpdateSensorHmdCaps();
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

void FOculusRiftHMD::GetFieldOfView(float& OutHFOVInDegrees, float& OutVFOVInDegrees) const
{
	OutHFOVInDegrees = FMath::RadiansToDegrees(HFOVInRadians);
	OutVFOVInDegrees = FMath::RadiansToDegrees(VFOVInRadians);
}

void FOculusRiftHMD::PoseToOrientationAndPosition(const ovrPosef& InPose, FQuat& OutOrientation, FVector& OutPosition) const
{
	OutOrientation = ToFQuat(InPose.Orientation);
#ifdef OVR_VISION_ENABLED
	// correct position according to BaseOrientation and BaseOffset
	OutPosition = BaseOrientation.Inverse().RotateVector(ToFVector_M2U(Vector3f(InPose.Position) - BaseOffset));
#else // OVR_VISION_ENABLED
	OutPosition = FVector(0.f);
#endif // OVR_VISION_ENABLED

	// apply base orientation correction to OutOrientation
	OutOrientation = BaseOrientation.Inverse() * OutOrientation;
	OutOrientation.Normalize();
}

void FOculusRiftHMD::GetCurrentOrientationAndPosition(FQuat& CurrentOrientation, FVector& CurrentPosition) const
{
	const ovrSensorState ss = ovrHmd_GetSensorState(Hmd, ovr_GetTimeInSeconds() + MotionPredictionInSeconds);
	const ovrPosef& pose = ss.Predicted.Pose;
	PoseToOrientationAndPosition(pose, CurrentOrientation, CurrentPosition);
#ifdef OVR_VISION_ENABLED
	if (bHmdPosTracking)
	{
#if !UE_BUILD_SHIPPING
		bool hadVisionTracking = bHaveVisionTracking;
		bHaveVisionTracking = (ss.StatusFlags & ovrStatus_PositionTracked) != 0;
		if (bHaveVisionTracking && !hadVisionTracking)
			UE_LOG(LogHMD, Warning, TEXT("Vision Tracking Acquired"));
		if (!bHaveVisionTracking && hadVisionTracking)
			UE_LOG(LogHMD, Warning, TEXT("Lost Vision Tracking"));
#endif // #if !UE_BUILD_SHIPPING
	}
#endif // OVR_VISION_ENABLED
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
#if !UE_BUILD_SHIPPING
	if (bDoNotUpdateOnGT)
		return;
#endif
	ConditionalLocker lock(bUpdateOnRT, &UpdateOnRTLock);

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
	if (bDrawTrackingCameraFrustum && PC->GetPawnOrSpectator())
	{
		DrawDebugTrackingCameraFrustum(PC->GetWorld(), PC->GetPawnOrSpectator()->GetPawnViewLocation());
	}
#endif
}

void FOculusRiftHMD::UpdatePlayerCameraRotation(APlayerCameraManager* Camera, struct FMinimalViewInfo& POV)
{
#if !UE_BUILD_SHIPPING
	if (bDoNotUpdateOnGT)
		return;
#endif
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
	{
		DrawDebugTrackingCameraFrustum(Camera->GetWorld(), POV.Location);
	}
#endif
}

#if !UE_BUILD_SHIPPING
void FOculusRiftHMD::DrawDebugTrackingCameraFrustum(UWorld* World, const FVector& ViewLocation)
{
	const FColor c = (HasValidTrackingPosition() ? FColor::Green : FColor::Red);
	FVector origin;
	FRotator rotation;
	float hfovDeg, vfovDeg, nearPlane, farPlane, cameraDist;
	GetPositionalTrackingCameraProperties(origin, rotation, hfovDeg, vfovDeg, cameraDist, nearPlane, farPlane);

	// Level line
	//DrawDebugLine(World, ViewLocation, FVector(ViewLocation.X + 1000, ViewLocation.Y, ViewLocation.Z), FColor::Blue);

	const float hfov = FMath::DegreesToRadians(hfovDeg * 0.5f);
	const float vfov = FMath::DegreesToRadians(vfovDeg * 0.5f);
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
	DrawDebugPoint(World, coneTop, 5, c);

	// draw main pyramid, from top to base
	DrawDebugLine(World, coneTop, coneBase1, c);
	DrawDebugLine(World, coneTop, coneBase2, c);
	DrawDebugLine(World, coneTop, coneBase3, c);
	DrawDebugLine(World, coneTop, coneBase4, c);
											  
	// draw base (far plane)				  
	DrawDebugLine(World, coneBase1, coneBase2, c);
	DrawDebugLine(World, coneBase2, coneBase3, c);
	DrawDebugLine(World, coneBase3, coneBase4, c);
	DrawDebugLine(World, coneBase4, coneBase1, c);

	// draw near plane
	FVector coneNear1(-nearPlane, nearPlane * FMath::Tan(hfov), nearPlane * FMath::Tan(vfov));
	FVector coneNear2(-nearPlane,-nearPlane * FMath::Tan(hfov), nearPlane * FMath::Tan(vfov));
	FVector coneNear3(-nearPlane,-nearPlane * FMath::Tan(hfov),-nearPlane * FMath::Tan(vfov));
	FVector coneNear4(-nearPlane, nearPlane * FMath::Tan(hfov),-nearPlane * FMath::Tan(vfov));
	coneNear1 = m.TransformPosition(coneNear1);
	coneNear2 = m.TransformPosition(coneNear2);
	coneNear3 = m.TransformPosition(coneNear3);
	coneNear4 = m.TransformPosition(coneNear4);
	DrawDebugLine(World, coneNear1, coneNear2, c);
	DrawDebugLine(World, coneNear2, coneNear3, c);
	DrawDebugLine(World, coneNear3, coneNear4, c);
	DrawDebugLine(World, coneNear4, coneNear1, c);

	// center line
	FVector centerLine(-cameraDist, 0, 0);
	centerLine = m.TransformPosition(centerLine);
	DrawDebugLine(World, coneTop, centerLine, FColor::Yellow);
	DrawDebugPoint(World, centerLine, 5, FColor::Yellow);
}
#endif // #if !UE_BUILD_SHIPPING

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
			InterpupillaryDistance = ovrHmd_GetFloat(Hmd, OVR_KEY_IPD, OVR_DEFAULT_IPD);
			UpdateStereoRenderingParams();
			return true;
		}
		else if (FParse::Command(&Cmd, TEXT("SHOW")))
		{
			const FVector hm = ToFVector(GetHeadModel());
			Ar.Logf(TEXT("stereo ipd=%.4f headmodel={%.3f, %.3f, %.3f}\n   hfov=%.3f vfov=%.3f"), GetInterpupillaryDistance(),
				hm.X, hm.Y, hm.Z, FMath::RadiansToDegrees(HFOVInRadians), FMath::RadiansToDegrees(VFOVInRadians));
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
			SetHeadModel(ToOVRVector<OVR::Vector3f>(hm));
		}

		if(FParse::Value( Cmd, TEXT("HEADY="), val))
		{
			FVector hm = ToFVector(GetHeadModel());
			hm.Y = val;
			SetHeadModel(ToOVRVector<OVR::Vector3f>(hm));
		}

		if(FParse::Value( Cmd, TEXT("HEADZ="), val))
		{
			FVector hm = ToFVector(GetHeadModel());
			hm.Z = val;
			SetHeadModel(ToOVRVector<OVR::Vector3f>(hm));
		}
			
		// debug configuration
		if (bDevSettingsEnabled)
		{
			float fov;
			if (FParse::Value(Cmd, TEXT("HFOV="), fov))
			{
				HFOVInRadians = FMath::DegreesToRadians(fov);
				bOverrideStereo = true;
			}
			else if (FParse::Value(Cmd, TEXT("VFOV="), fov))
			{
				VFOVInRadians = FMath::DegreesToRadians(fov);
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
				else if (FParse::Command(&Cmd, TEXT("TOGGLE")) || FParse::Command(&Cmd, TEXT("")))
				{
					bVSync = !bVSync;
					bOverrideVSync = true;
					ApplySystemOverridesOnStereo();
					Ar.Logf(TEXT("VSync is currently %s"), (bVSync) ? TEXT("ON") : TEXT("OFF"));
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
		else if (FParse::Command(&Cmd, TEXT("LP"))) // low persistence mode
		{
			FString CmdName = FParse::Token(Cmd, 0);
			if (!CmdName.IsEmpty())
			{
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
				else
				{
					return false;
				}
			}
			else
			{
				bLowPersistenceMode = !bLowPersistenceMode;
			}
			UpdateSensorHmdCaps();
			Ar.Logf(TEXT("Low Persistence is currently %s"), (bLowPersistenceMode) ? TEXT("ON") : TEXT("OFF"));
			return true;
		}
		else if (FParse::Command(&Cmd, TEXT("UPDATEONRT"))) // update on renderthread
		{
			FString CmdName = FParse::Token(Cmd, 0);
			if (!CmdName.IsEmpty())
			{
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
				else
				{
					return false;
				}
			}
			else
			{
				bUpdateOnRT = !bUpdateOnRT;
			}
			Ar.Logf(TEXT("Update on render thread is currently %s"), (bUpdateOnRT) ? TEXT("ON") : TEXT("OFF"));
			return true;
		}
		else if (FParse::Command(&Cmd, TEXT("RCF"))) // update on renderthread
		{
			FString CmdName = FParse::Token(Cmd, 0);
			if (!CmdName.IsEmpty())
			{
				if (!FCString::Stricmp(*CmdName, TEXT("SHOW")))
				{
					Ar.Logf(TEXT("rcf correction={%.3f, %.3f, %.3f}"), RCFCorrection.X, RCFCorrection.Y, RCFCorrection.Z);
					return true;
				}
				else if (!FCString::Stricmp(*CmdName, TEXT("RESET")))
				{
					RCFCorrection = FVector::ZeroVector;
					return true;
				}
				else if (!FCString::Stricmp(*CmdName, TEXT("SET")))
				{
					float val;
					if (FParse::Value(Cmd, TEXT("X="), val))
					{
						RCFCorrection.X = val;
					}

					if (FParse::Value(Cmd, TEXT("Y="), val))
					{
						RCFCorrection.Y = val;
					}

					if (FParse::Value(Cmd, TEXT("Z="), val))
					{
						RCFCorrection.Z = val;
					}
					return true;
				}
			}
		}
#ifdef OVR_DIRECT_RENDERING
		else if (FParse::Command(&Cmd, TEXT("TIMEWARP"))) 
		{
			FString CmdName = FParse::Token(Cmd, 0);
			if (!CmdName.IsEmpty())
			{
				if (!FCString::Stricmp(*CmdName, TEXT("ON")))
				{
					bTimeWarp = true;
				}
				else if (!FCString::Stricmp(*CmdName, TEXT("OFF")))
				{
					bTimeWarp = false;
				}
				else if (!FCString::Stricmp(*CmdName, TEXT("TOGGLE")))
				{
					bTimeWarp = !bTimeWarp;
				}
				else
				{
					return false;
				}
			}
			else
			{
				bTimeWarp = !bTimeWarp;
			}
			Ar.Logf(TEXT("TimeWarp is currently %s"), (bTimeWarp) ? TEXT("ON") : TEXT("OFF"));
			#ifdef OVR_DIRECT_RENDERING 
			mD3D11Bridge.bNeedReinitRendererAPI = true;
			#endif
			return true;
		}
#endif // #ifdef OVR_DIRECT_RENDERING
#if !UE_BUILD_SHIPPING
		else if (FParse::Command(&Cmd, TEXT("UPDATEONGT"))) // update on game thread
		{
			FString CmdName = FParse::Token(Cmd, 0);
			if (!CmdName.IsEmpty())
			{
				if (!FCString::Stricmp(*CmdName, TEXT("ON")))
				{
					bDoNotUpdateOnGT = false;
				}
				else if (!FCString::Stricmp(*CmdName, TEXT("OFF")))
				{
					bDoNotUpdateOnGT = true;
				}
				else if (!FCString::Stricmp(*CmdName, TEXT("TOGGLE")))
				{
					bDoNotUpdateOnGT = !bDoNotUpdateOnGT;
				}
				else
				{
					return false;
				}
			}
			else
			{
				bDoNotUpdateOnGT = !bDoNotUpdateOnGT;
			}
			Ar.Logf(TEXT("Update on game thread is currently %s"), (!bDoNotUpdateOnGT) ? TEXT("ON") : TEXT("OFF"));
			return true;
		}
		else if (FParse::Command(&Cmd, TEXT("STATS"))) // status / statistics
		{
			bShowStats = !bShowStats;
		}
#endif //UE_BUILD_SHIPPING
	}
	else if (FParse::Command(&Cmd, TEXT("HMDMAG")))
	{
		if (FParse::Command(&Cmd, TEXT("ON")))
		{
			bYawDriftCorrectionEnabled = true;
			UpdateSensorHmdCaps();
			return true;
		}
		else if (FParse::Command(&Cmd, TEXT("OFF")))
		{
			bYawDriftCorrectionEnabled = false;
			UpdateSensorHmdCaps();
			return true;
		}
		else if (FParse::Command(&Cmd, TEXT("SHOW")))
		{
			Ar.Logf(TEXT("mag %s"), bYawDriftCorrectionEnabled ? 
				TEXT("on") : TEXT("off"));
			return true;
		}
		return false;
	}
	else if (FParse::Command(&Cmd, TEXT("HMDTILT")))
	{
		if (FParse::Command(&Cmd, TEXT("ON")))
		{
			bTiltCorrectionEnabled = true;
			return true;
		}
		else if (FParse::Command(&Cmd, TEXT("OFF")))
		{
			bTiltCorrectionEnabled = false;
			return true;
		}
		else if (FParse::Command(&Cmd, TEXT("SHOW")))
		{
 			Ar.Logf(TEXT("tilt correction %s"), bTiltCorrectionEnabled ? 
 				TEXT("on") : TEXT("off"));
 			return true;
		}
		return false;
	}
#ifndef OVR_DIRECT_RENDERING
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

		if (FParse::Command(&Cmd, TEXT("SHOW")))
		{
			Ar.Logf(TEXT("hmdwarp %s sc=%f %s"), (bHmdDistortion ? TEXT("on") : TEXT("off"))
				, IdealScreenPercentage / 100.f
				, (bChromaAbCorrectionEnabled ? TEXT("cha") : TEXT("nocha")));
		}
		return true;
    }
#endif //OVR_DIRECT_RENDERING
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
		else if (FParse::Command(&Cmd, TEXT("ON")) || FParse::Command(&Cmd, TEXT("ENABLE")))
		{
			bHmdPosTracking = true;
			UpdateSensorHmdCaps();
			return true;
		}
		else if (FParse::Command(&Cmd, TEXT("OFF")) || FParse::Command(&Cmd, TEXT("DISABLE")))
		{
			bHmdPosTracking = false;
			UpdateSensorHmdCaps();
			return true;
		}
		else if (FParse::Command(&Cmd, TEXT("TOGGLE")))
		{
			bHmdPosTracking = !bHmdPosTracking;
			UpdateSensorHmdCaps();
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
			if (FParse::Command(&Cmd, TEXT("ON")))
			{
				bDrawTrackingCameraFrustum = true;
				return true;
			}
			else 
			{
				bDrawTrackingCameraFrustum = !bDrawTrackingCameraFrustum;
				return true;
			}
		}
#endif // #if !UE_BUILD_SHIPPING
		else if (FParse::Command(&Cmd, TEXT("SHOW")))
		{
			Ar.Logf(TEXT("hmdpos is %s, vision='%s'"), 
				(bHmdPosTracking ? TEXT("enabled") : TEXT("disabled")),
				(bHaveVisionTracking ? TEXT("active") : TEXT("lost")));
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

		if (!FCString::Stricmp(*CmdName, TEXT("ON")))
		{
			bHeadTrackingEnforced = false;
			return true;
		}
		else if (!FCString::Stricmp(*CmdName, TEXT("ENFORCE")))
		{
			bHeadTrackingEnforced = !bHeadTrackingEnforced;
			if (!bHeadTrackingEnforced)
			{
				CurHmdOrientation = FQuat::Identity;
				ResetControlRotation();
			}
			return true;
		}
		else if (!FCString::Stricmp(*CmdName, TEXT("RESET")))
		{
			bHeadTrackingEnforced = false;
			CurHmdOrientation = FQuat::Identity;
			ResetControlRotation();
			return true;
		}
		else if (!FCString::Stricmp(*CmdName, TEXT("SHOW")))
		{
			if (MotionPredictionInSeconds > 0)
			{
				Ar.Logf(TEXT("motion prediction=%.3f"),	MotionPredictionInSeconds);
			}
			else
			{
				Ar.Logf(TEXT("motion prediction OFF"));
			}
	
			return true;
		}

		FString Value = FParse::Token(Cmd, 0);
		if (Value.IsEmpty())
		{
			return false;
		}
		if (!FCString::Stricmp(*CmdName, TEXT("PRED")))
		{
			if (!FCString::Stricmp(*Value, TEXT("OFF")))
			{
				MotionPredictionInSeconds = 0.0;
			}
			else if (!FCString::Stricmp(*Value, TEXT("OFF")))
			{
				MotionPredictionInSeconds = DEFAULT_PREDICTION_IN_SECONDS;
			}
			else
			{
				MotionPredictionInSeconds = FCString::Atod(*Value);
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

	return false;
}

void FOculusRiftHMD::OnScreenModeChange(EWindowMode::Type WindowMode)
{
	EnableStereo(WindowMode != EWindowMode::Windowed);
	UpdateStereoRenderingParams();
}

bool FOculusRiftHMD::IsPositionalTrackingEnabled() const
{
#ifdef OVR_VISION_ENABLED
	return bHmdPosTracking;
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

OVR::Vector3f FOculusRiftHMD::GetHeadModel() const
{
#ifdef OVR_VISION_ENABLED
	//check(0);
	return Vector3f();
#else
	return HeadModel_Meters;
#endif
}

void    FOculusRiftHMD::SetHeadModel(const OVR::Vector3f& hm)
{
#ifdef OVR_VISION_ENABLED
	//check(0);
	//pSensorFusion->SetHeadModel(hm);
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

#ifdef OVR_DIRECT_RENDERING
		GDynamicRHI->InitHMDDevice(nullptr);
#endif

	}
	else
	{
		SaveSystemValues();
		ApplySystemOverridesOnStereo(bIsEnabledNow);

		UpdateStereoRenderingParams();
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
		else
		{
			static IConsoleVariable* CVSyncVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync"));
			bVSync = CVSyncVar->GetInt() != 0;
			#ifdef OVR_DIRECT_RENDERING 
			mD3D11Bridge.bNeedReinitRendererAPI = true;
			#endif
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
	float DesiredSceeenPercentage;
	if (bOverrideScreenPercentage)
	{
		DesiredSceeenPercentage = ScreenPercentage;
	}
	else
	{
		DesiredSceeenPercentage = IdealScreenPercentage;
	}
	if (FMath::RoundToInt(CScrPercVar->GetFloat()) != FMath::RoundToInt(DesiredSceeenPercentage))
	{
		CScrPercVar->Set(DesiredSceeenPercentage);
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

void FOculusRiftHMD::CalculateStereoViewOffset(const EStereoscopicPass StereoPassType, const FRotator& ViewRotation, const float WorldToMeters, FVector& ViewLocation)
{
	ConditionalLocker lock(bUpdateOnRT, &UpdateOnRTLock);
	if (bNeedUpdateStereoRenderingParams)
		UpdateStereoRenderingParams();

	if( StereoPassType != eSSP_FULL )
	{
		check(WorldToMeters != 0.f)

		const int idx = (StereoPassType == eSSP_LEFT_EYE) ? 0 : 1;
        const float PassEyeOffset = -EyeRenderDesc[idx].ViewAdjust.x * WorldToMeters;

		FVector TotalOffset = FVector(0,PassEyeOffset,0);
		FVector RCFDelta = RCFCorrection * WorldToMeters; //@TEMP
		TotalOffset += RCFDelta;

		ViewLocation += ViewRotation.Quaternion().RotateVector(TotalOffset);

		// The HMDPosition already has HMD orientation applied.
		// Apply rotational difference between HMD orientation and ViewRotation
		// to HMDPosition vector. 

		const FVector vHMDPosition = DeltaControlOrientation.RotateVector(CurHmdPosition);
		ViewLocation += vHMDPosition;
	}
	else if (bHeadTrackingEnforced)
	{
		const FVector vHMDPosition = DeltaControlOrientation.RotateVector(CurHmdPosition);
		ViewLocation += vHMDPosition;
	}
}

void FOculusRiftHMD::ResetOrientationAndPosition(float yaw)
{
	const ovrSensorState ss = ovrHmd_GetSensorState(Hmd, ovr_GetTimeInSeconds());
	const ovrPosef& pose = ss.Recorded.Pose;
	const OVR::Quatf orientation = OVR::Quatf(pose.Orientation);

	// Reset position
#ifdef OVR_VISION_ENABLED
	BaseOffset = pose.Position;
#else
	BaseOffset = OVR::Vector3f(0, 0, 0);
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
	// Stereo params must be recalculated already, see CalculateStereoViewOffset.
	check(!bNeedUpdateStereoRenderingParams);
	check(IsStereoEnabled());

	const int idx = (StereoPassType == eSSP_LEFT_EYE) ? 0 : 1;

	FMatrix proj		 = ToFMatrix(EyeProjectionMatrices[idx]);

	// correct far and near planes
	proj.M[2][2] = proj.M[3][3] = 0.0f;
	proj.M[2][3] = 1.0f;
	proj.M[3][2] = GNearClippingPlane;

	return proj;
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
		int32 SideSizeX = FMath::TruncToInt(InViewport->GetSizeXY().X * 0.5);

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

		float ScaleFactor = 1.0f;
		FScaleMatrix m(ScaleFactor);

		InCanvas->PushAbsoluteTransform(FTranslationMatrix(
			FVector(((StereoPass == eSSP_RIGHT_EYE) ? SideSizeX : 0) + Disparity, 0, 0))*m);
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
#ifndef OVR_DIRECT_RENDERING
	ShowFlags.HMDDistortion = bHmdDistortion;
#else
	ShowFlags.HMDDistortion = false;
#endif
	ShowFlags.ScreenPercentage = true;
	ShowFlags.StereoRendering = IsStereoEnabled();
}

void FOculusRiftHMD::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
	InView.BaseHmdOrientation = LastHmdOrientation;
	InView.BaseHmdLocation = FVector(0.f);
	WorldToMetersScale = InView.WorldToMetersScale;

#ifndef OVR_DIRECT_RENDERING
	InViewFamily.bUseSeparateRenderTarget = false;

	// check and save texture size. 
	if (InView.StereoPass == eSSP_LEFT_EYE)
	{
		if (EyeViewportSize != InView.ViewRect.Size())
		{
			EyeViewportSize = InView.ViewRect.Size();
			bNeedUpdateStereoRenderingParams = true;
		}
	}
#else
	InViewFamily.bUseSeparateRenderTarget = ShouldUseSeparateRenderTarget();
#endif
}

void FOculusRiftHMD::ProcessLatencyTesterInput() const
{
}

bool FOculusRiftHMD::IsHeadTrackingAllowed() const
{
	return bHeadTrackingEnforced || GEngine->IsStereoscopic3D();
}

//---------------------------------------------------
// Oculus Rift Specific
//---------------------------------------------------

FOculusRiftHMD::FOculusRiftHMD()
	: 
#ifdef OVR_DIRECT_RENDERING
#if defined(OVR_D3D_VERSION) && (OVR_D3D_VERSION == 11)
	mD3D11Bridge(getThis()),
#endif
#endif
	 bWasInitialized(false)
	, bStereoEnabled(false)
	, bHMDEnabled(true)
	, bNeedUpdateStereoRenderingParams(true)
    , bOverrideStereo(false)
	, bOverrideIPD(false)
	, bOverrideDistortion(false)
	, bDevSettingsEnabled(false)
	, bOverrideFOV(false)
	, bOverrideVSync(true)
	, bVSync(true)
	, bSavedVSync(false)
	, SavedScrPerc(100.f)
	, bOverrideScreenPercentage(false)
	, ScreenPercentage(100.f)
	, bAllowFinishCurrentFrame(false)
	, InterpupillaryDistance(OVR_DEFAULT_IPD)
	, WorldToMetersScale(100.f)
	, UserDistanceToScreenModifier(0.f)
	, VFOVInRadians(FMath::DegreesToRadians(90.f))
	, HFOVInRadians(FMath::DegreesToRadians(90.f))
	, MotionPredictionInSeconds(DEFAULT_PREDICTION_IN_SECONDS)
	, AccelGain(0.f)
	, bUseHeadModel(true)
	, bHmdDistortion(true)
	, bChromaAbCorrectionEnabled(true)
	, bYawDriftCorrectionEnabled(true)
	, bTiltCorrectionEnabled(true)
	, bOverride2D(false)
	, HudOffset(0.f)
	, CanvasCenterOffset(0.f)
	, bLowPersistenceMode(false)
	, bUpdateOnRT(true)
	, bHeadTrackingEnforced(false)
#if !UE_BUILD_SHIPPING
	, bDoNotUpdateOnGT(false)
	, bDrawTrackingCameraFrustum(false)
	, bShowStats(false)
#endif
	, bTimeWarp(true)
	, LastHmdOrientation(FQuat::Identity)
	, CurHmdOrientation(FQuat::Identity)
	, DeltaControlRotation(FRotator::ZeroRotator)
	, DeltaControlOrientation(FQuat::Identity)
	, CurHmdPosition(FVector::ZeroVector)
	, BaseOffset(0, 0, 0)
	, BaseOrientation(FQuat::Identity)
	, SensorHmdCaps(0)
	, DistortionCaps(0)
	, HmdCaps(0)
	, EyeViewportSize(0, 0)
#ifdef OVR_DIRECT_RENDERING
	, RenderTargetSize(0, 0)
#endif
	, RenderParams_RenderThread(getThis())
	, bHmdPosTracking(false)
	, bHaveVisionTracking(false)
	, RCFCorrection(FVector::ZeroVector)
#ifndef OVR_VISION_ENABLED
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
	// Initializes LibOVR. This LogMask_All enables maximum logging.
	// Custom allocator can also be specified here.
	ovr_Initialize();

	Hmd = ovrHmd_Create(0);
	if (Hmd)
	{
		bWasInitialized = true;

		DistortionCaps = ovrDistortionCap_Chromatic | ovrDistortionCap_Vignette | ovrDistortionCap_TimeWarp;
		HmdCaps = (bVSync ? 0 : ovrHmdCap_NoVSync);

		UpdateHmdRenderInfo();
		UpdateStereoRenderingParams();
		UE_LOG(LogHMD, Log, TEXT("Oculus initialized."));

		// Uncap fps to enable FPS higher than 62
		GEngine->bSmoothFrameRate = false;
	}
	else
	{
		UE_LOG(LogHMD, Warning, TEXT("No Oculus HMD detected!"));
	}

	LoadFromIni();
	SaveSystemValues();

	UpdateSensorHmdCaps();
}

void FOculusRiftHMD::Shutdown()
{
	SaveToIni();

	ovrHmd_Destroy(Hmd);
	
#ifndef OVR_DIRECT_RENDERING
	for (unsigned i = 0; i < sizeof(pDistortionMesh) / sizeof(pDistortionMesh[0]); ++i)
	{
		pDistortionMesh[i] = NULL;
	}
#endif
	{
		OVR::Lock::Locker lock(&StereoParamsLock);
		RenderParams_RenderThread.Clear();
	}
	ovr_Shutdown();
	UE_LOG(LogHMD, Log, TEXT("Oculus shutdown."));
}

void FOculusRiftHMD::UpdateSensorHmdCaps()
{
	if (Hmd)
	{
		SensorHmdCaps = ovrSensorCap_Orientation | ovrHmdCap_LatencyTest;
		if (bYawDriftCorrectionEnabled)
		{
			SensorHmdCaps |= ovrSensorCap_YawCorrection;
		}
		else
		{
			SensorHmdCaps &= ~ovrSensorCap_YawCorrection;
		}
		if (bHmdPosTracking)
		{
			SensorHmdCaps |= ovrSensorCap_Position;
		}
		else
		{
			SensorHmdCaps &= ~ovrSensorCap_Position;
		}

		ovrHmd_StartSensor(Hmd, SensorHmdCaps, 0);

		if (bLowPersistenceMode)
		{
			HmdCaps |= ovrHmdCap_LowPersistence;
		}
		else
		{
			HmdCaps &= ~ovrHmdCap_LowPersistence;
		}
		ovrHmd_SetEnabledCaps(Hmd, HmdCaps);
	}
}

static ovrFovPort SymmetricalFOV(const ovrFovPort& fov)
{
	ovrFovPort newFov;
	const float VHalfTan = FMath::Max(fov.DownTan, fov.UpTan);
	const float HHalfTan = FMath::Max(fov.LeftTan, fov.RightTan);
	newFov.DownTan = newFov.UpTan		= VHalfTan; //FMath::Max(VHalfTan, HHalfTan);
	newFov.LeftTan = newFov.RightTan	= HHalfTan; //FMath::Max(VHalfTan, HHalfTan);
	return newFov;
}

FORCEINLINE static float GetVerticalFovRadians(const ovrFovPort& fov)
{
	return FMath::Atan(fov.UpTan) + FMath::Atan(fov.DownTan);
}

FORCEINLINE static float GetHorizontalFovRadians(const ovrFovPort& fov)
{
	return FMath::Atan(fov.LeftTan) + FMath::Atan(fov.RightTan);
}

void FOculusRiftHMD::UpdateHmdRenderInfo()
{
	check(Hmd);

	// Assuming we've successfully grabbed the device, read the configuration data from it, which we'll use for projection
	ovrHmd_GetDesc(Hmd, &HmdDesc);

	UE_LOG(LogHMD, Warning, TEXT("HMD %s, Monitor %s, res = %d x %d, windowPos = {%d, %d}"), ANSI_TO_TCHAR(HmdDesc.ProductName), 
		ANSI_TO_TCHAR(HmdDesc.DisplayDeviceName), HmdDesc.Resolution.w, HmdDesc.Resolution.h, HmdDesc.WindowsPos.x, HmdDesc.WindowsPos.y); 

	// Calc FOV
	if (!bOverrideFOV)
	{
		// Calc FOV, symmetrical, for each eye. 
		EyeFov[0] = SymmetricalFOV(HmdDesc.DefaultEyeFov[0]);
		EyeFov[1] = SymmetricalFOV(HmdDesc.DefaultEyeFov[1]);

		// Calc FOV in radians
		VFOVInRadians = FMath::Max(GetVerticalFovRadians(EyeFov[0]), GetVerticalFovRadians(EyeFov[1]));
		HFOVInRadians = FMath::Max(GetHorizontalFovRadians(EyeFov[0]), GetHorizontalFovRadians(EyeFov[1]));
	}

	const Sizei recommenedTex0Size = ovrHmd_GetFovTextureSize(Hmd, ovrEye_Left, EyeFov[0], 1.0f);
	const Sizei recommenedTex1Size = ovrHmd_GetFovTextureSize(Hmd, ovrEye_Right, EyeFov[1], 1.0f);

	Sizei idealRenderTargetSize;
	idealRenderTargetSize.w = recommenedTex0Size.w + recommenedTex1Size.w;
	idealRenderTargetSize.h = FMath::Max(recommenedTex0Size.h, recommenedTex1Size.h);

	IdealScreenPercentage = FMath::Max(float(idealRenderTargetSize.w) / float(HmdDesc.Resolution.w) * 100.f,
									   float(idealRenderTargetSize.h) / float(HmdDesc.Resolution.h) * 100.f);

	// Override eye distance by the value from HMDInfo (stored in Profile).
	if (!bOverrideIPD)
	{
		InterpupillaryDistance = ovrHmd_GetFloat(Hmd, OVR_KEY_IPD, OVR_DEFAULT_IPD);
	}

	// Default texture size (per eye) is equal to half of W x H resolution. Will be overridden in SetupView.
	EyeViewportSize = FIntPoint(HmdDesc.Resolution.w / 2, HmdDesc.Resolution.h);

	bNeedUpdateStereoRenderingParams = true;
}

void FOculusRiftHMD::UpdateStereoRenderingParams()
{
	// If we've manually overridden stereo rendering params for debugging, don't mess with them
	if (bOverrideStereo || !IsStereoEnabled())
	{
		return;
	}
	if (IsInitialized())
	{
		Lock::Locker lock(&StereoParamsLock);

		TextureSize = Sizei(EyeViewportSize.X * 2, EyeViewportSize.Y);

		EyeRenderViewport[0].Pos = Vector2i(0, 0);
		EyeRenderViewport[0].Size = Sizei(EyeViewportSize.X, EyeViewportSize.Y);
		EyeRenderViewport[1].Pos = Vector2i(EyeViewportSize.X, 0);
		EyeRenderViewport[1].Size = EyeRenderViewport[0].Size;

		//!AB: note, for Direct Rendering EyeRenderDesc is calculated twice, once
		// here and another time in BeginRendering_RenderThread. I need to have EyeRenderDesc
		// on a game thread for ViewAdjust (for ProjectionMatrix calculation). @@TODO: revise.
		EyeRenderDesc[0] = ovrHmd_GetRenderDesc(Hmd, ovrEye_Left, EyeFov[0]);
		EyeRenderDesc[1] = ovrHmd_GetRenderDesc(Hmd, ovrEye_Right, EyeFov[1]);

		const bool bRightHanded = false;
		EyeProjectionMatrices[0] = ovrMatrix4f_Projection(EyeFov[0], 0.01f, 10000.0f, bRightHanded);
		EyeProjectionMatrices[1] = ovrMatrix4f_Projection(EyeFov[1], 0.01f, 10000.0f, bRightHanded);

		// 2D elements offset
		if (!bOverride2D)
		{
			float ScreenSizeInMeters[2]; // 0 - width, 1 - height
			float LensSeparationInMeters;
			LensSeparationInMeters = ovrHmd_GetFloat(Hmd, "LensSeparation", 0);
			ovrHmd_GetFloatArray(Hmd, "ScreenSize", ScreenSizeInMeters, 2);

			// Recenter projection (meters)
			const float LeftProjCenterM = ScreenSizeInMeters[0] * 0.25f;
			const float LensRecenterM = LeftProjCenterM - LensSeparationInMeters * 0.5f;

			// Recenter projection (normalized)
			const float LensRecenter = 4.0f * LensRecenterM / ScreenSizeInMeters[0];

			HudOffset = 0.25f * InterpupillaryDistance * (HmdDesc.Resolution.w / ScreenSizeInMeters[0]) / 15.0f;
			CanvasCenterOffset = (0.25f * LensRecenter) * HmdDesc.Resolution.w;
		}

		PrecalculatePostProcess_NoLock();
		#ifdef OVR_DIRECT_RENDERING 
		mD3D11Bridge.bNeedReinitRendererAPI = true;
		#endif
	}
	else
	{
		CanvasCenterOffset = 0.f;
	}
	bNeedUpdateStereoRenderingParams = false;
}

void FOculusRiftHMD::LoadFromIni()
{
	//if (pSensorFusion)
	{
		const TCHAR* OculusSettings = TEXT("Oculus.Settings");
		bool v;
		float f;
		if (GConfig->GetBool(OculusSettings, TEXT("bChromaAbCorrectionEnabled"), v, GEngineIni))
		{
			bChromaAbCorrectionEnabled = v;
		}
		if (GConfig->GetBool(OculusSettings, TEXT("bYawDriftCorrectionEnabled"), v, GEngineIni))
		{
			bYawDriftCorrectionEnabled = v;
		}
		if (GConfig->GetBool(OculusSettings, TEXT("bDevSettingsEnabled"), v, GEngineIni))
		{
			bDevSettingsEnabled = v;
		}
		if (GConfig->GetBool(OculusSettings, TEXT("bTiltCorrectionEnabled"), v, GEngineIni))
		{
			bTiltCorrectionEnabled = v;
		}
		if (GConfig->GetFloat(OculusSettings, TEXT("MotionPrediction"), f, GEngineIni))
		{
			MotionPredictionInSeconds = f;
		}
		FVector vec;
		if (GConfig->GetVector(OculusSettings, TEXT("RCFCorrection"), vec, GEngineIni))
		{
			RCFCorrection = vec;
		}
		if (GConfig->GetVector(OculusSettings, TEXT("HeadModel_v2"), vec, GEngineIni))
		{
//			SetHeadModel(ToOVRVector<OVR::Vector3f>(vec));
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
				if (GConfig->GetFloat(OculusSettings, TEXT("HFOV"), f, GEngineIni))
				{
					HFOVInRadians = f;
				}
				if (GConfig->GetFloat(OculusSettings, TEXT("VFOV"), f, GEngineIni))
				{
					VFOVInRadians = f;
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
		if (GConfig->GetBool(OculusSettings, TEXT("bHmdPosTracking"), v, GEngineIni))
		{
			bHmdPosTracking = v;
		}
		if (GConfig->GetBool(OculusSettings, TEXT("bLowPersistenceMode"), v, GEngineIni))
		{
			bLowPersistenceMode = v;
		}
		if (GConfig->GetBool(OculusSettings, TEXT("bUpdateOnRT"), v, GEngineIni))
		{
			bUpdateOnRT = v;
		}
	}
}

void FOculusRiftHMD::SaveToIni()
{
	//if (pSensorFusion)
	{
		const TCHAR* OculusSettings = TEXT("Oculus.Settings");
		GConfig->SetBool(OculusSettings, TEXT("bChromaAbCorrectionEnabled"), bChromaAbCorrectionEnabled, GEngineIni);
		GConfig->SetBool(OculusSettings, TEXT("bYawDriftCorrectionEnabled"), bYawDriftCorrectionEnabled, GEngineIni);
		GConfig->SetBool(OculusSettings, TEXT("bDevSettingsEnabled"), bDevSettingsEnabled, GEngineIni);
		GConfig->SetBool(OculusSettings, TEXT("bTiltCorrectionEnabled"), bTiltCorrectionEnabled, GEngineIni);
		GConfig->SetFloat(OculusSettings, TEXT("MotionPrediction"), float(MotionPredictionInSeconds), GEngineIni);

		//const OVR::Vector3f hm = GetHeadModel();
		//GConfig->SetVector(OculusSettings, TEXT("HeadModel_v2"), ToFVector(hm), GEngineIni);

		GConfig->SetVector(OculusSettings, TEXT("RCFCorrection"), RCFCorrection, GEngineIni);

		GConfig->SetBool(OculusSettings, TEXT("bOverrideIPD"), bOverrideIPD, GEngineIni);
		if (bOverrideIPD)
		{
			GConfig->SetFloat(OculusSettings, TEXT("IPD"), GetInterpupillaryDistance(), GEngineIni);
		}
		GConfig->SetBool(OculusSettings, TEXT("bOverrideStereo"), bOverrideStereo, GEngineIni);
		if (bOverrideStereo)
		{
			GConfig->SetFloat(OculusSettings, TEXT("HFOV"), HFOVInRadians, GEngineIni);
			GConfig->SetFloat(OculusSettings, TEXT("VFOV"), VFOVInRadians, GEngineIni);
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
		GConfig->SetBool(OculusSettings, TEXT("bHmdPosTracking"), bHmdPosTracking, GEngineIni);
		GConfig->SetBool(OculusSettings, TEXT("bLowPersistenceMode"), bLowPersistenceMode, GEngineIni);
#endif

		GConfig->SetBool(OculusSettings, TEXT("bUpdateOnRT"), bUpdateOnRT, GEngineIni);
	}
}


#if 0

#if !UE_BUILD_SHIPPING
bool FOculusRiftHMD::HandleInputKey(UPlayerInput* pPlayerInput, 
	EKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad)
{
#if 0 //def OVR_VISION_ENABLED
	//UE_LOG(LogHMD, Log, TEXT("Key %d, %f, gamepad %d"), (int)Key, AmountDepressed, (int)bGamepad);
	if (bGamepad && EventType == IE_Pressed)
	{
		switch (Key)
		{
		case EKeys::Gamepad_RightThumbstick:
			// toggles mode for the right stick
			bPosTrackingSim = !bPosTrackingSim;
			SimPosition.x = SimPosition.y = SimPosition.z = 0;
			break;
		case EKeys::Gamepad_RightStick_Up: 
		case EKeys::Gamepad_RightStick_Down:
		case EKeys::Gamepad_RightStick_Left:
		case EKeys::Gamepad_RightStick_Right:
			if (bPosTrackingSim)
			{
				return true;
			}
			break;
		default:; 
		}
	}
#endif
	return false;
}

bool FOculusRiftHMD::HandleInputAxis(UPlayerInput*, EKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad)
{
#if 0 //def OVR_VISION_ENABLED
	// by default, Delta is changing in [-1..1] interval.
	const float InputAxisScale = 0.5f;  // make it [-0.5 .. +0.5] to simulate pos tracker input

//	UE_LOG(LogHMD, Log, TEXT("Key %d, delta %f, time %f, samples %d "), 
//		(int)Key, Delta, DeltaTime, NumSamples);
	if (bGamepad && bPosTrackingSim)
	{
		switch (Key)
		{
		case EKeys::Gamepad_LeftX:
			return true;
		case EKeys::Gamepad_LeftY:
			// up - down, Oculus right-handed XYZ coord system.
			SimPosition.y = float(Delta * InputAxisScale);
			return true;
		case EKeys::Gamepad_RightX:
			// left - right, Oculus right-handed XYZ coord system.
			SimPosition.x = float(Delta * InputAxisScale);
			return true;
		case EKeys::Gamepad_RightY:
			// forward - backward, Oculus right-handed XYZ coord system.
			SimPosition.z = float(Delta * InputAxisScale);
			return true;
		default:;
		}
	}
#endif
	return false;
}
#endif //!UE_BUILD_SHIPPING
#endif

#endif //OCULUS_RIFT_SUPPORTED_PLATFORMS

