// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
// Config and console related parts pulled out of OculusRiftHMD.cpp

#include "OculusRiftHMD.h"
#if !UE_BUILD_SHIPPING
#include "SceneCubemapCapturer.h"
static void CubemapCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar);
#endif


#if !PLATFORM_MAC // Mac uses 0.5/OculusRiftHMD_05.cpp

#define BOOLEAN_COMMAND_HANDLER_BODY(ConsoleName, FieldExpr)\
	if (Args.Num()) \
	{\
		if (Args[0].Equals(TEXT("toggle"), ESearchCase::IgnoreCase))\
		{\
			(FieldExpr) = !(FieldExpr);\
		}\
		else\
		{\
			(FieldExpr) = FCString::ToBool(*Args[0]);\
		}\
	}\
	Ar.Logf(ConsoleName TEXT(" = %s"), (FieldExpr) ? TEXT("On") : TEXT("Off"));

/// @cond DOXYGEN_WARNINGS

FOculusRiftConsoleCommands::FOculusRiftConsoleCommands(class FOculusRiftHMD* InHMDPtr)
	: PixelDensityCommand(TEXT("vr.oculus.PixelDensity"),
		*NSLOCTEXT("OculusRift", "CCommandText_PixelDensity",
			"Oculus Rift specific extension.\nPixel density sets the render target texture size as a factor of recommended texture size.\nSince this may be slighly larger than the native resolution, setting PixelDensity to 1.0 is\nusually not the same as setting r.ScreenPercentage to 100").ToString(),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateRaw(InHMDPtr, &FOculusRiftHMD::PixelDensityCommandHandler))
	, PixelDensityMinCommand(TEXT("vr.oculus.PixelDensity.min"),
		*NSLOCTEXT("OculusRift", "CCommandText_PixelDensityMin",
			"Oculus Rift specific extension.\nMinimum pixel density when adaptive pixel density is enabled").ToString(),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateRaw(InHMDPtr, &FOculusRiftHMD::PixelDensityMinCommandHandler))
	, PixelDensityMaxCommand(TEXT("vr.oculus.PixelDensity.max"),
		*NSLOCTEXT("OculusRift", "CCommandText_PixelDensityMax",
			"Oculus Rift specific extension.\nMaximum pixel density when adaptive pixel density is enabled").ToString(),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateRaw(InHMDPtr, &FOculusRiftHMD::PixelDensityMaxCommandHandler))
	, PixelDensityAdaptiveCommand(TEXT("vr.oculus.PixelDensity.adaptive"),
		*NSLOCTEXT("OculusRift", "CCommandText_PixelDensityAdaptive",
			"Oculus Rift specific extension.\nEnable or disable adaptive pixel density.").ToString(),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateRaw(InHMDPtr, &FOculusRiftHMD::PixelDensityAdaptiveCommandHandler))
	, HQBufferCommand(TEXT("vr.oculus.bHQBuffer"),
		*NSLOCTEXT("OculusRift", "CCommandText_HQBuffer",
			"Oculus Rift specific extension.\nEnable or disable using floating point texture format for the eye layer.").ToString(),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateRaw(InHMDPtr, &FOculusRiftHMD::HQBufferCommandHandler))
	, HQDistortionCommand(TEXT("vr.oculus.bHQDistortion"),
		*NSLOCTEXT("OculusRift", "CCommandText_HQDistortion",
			"Oculus Rift specific extension.\nEnable or disable using multiple mipmap levels for the eye layer.").ToString(),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateRaw(InHMDPtr, &FOculusRiftHMD::HQDistortionCommandHandler))
#if !UE_BUILD_SHIPPING
	, OvrPropertyCommand(TEXT("vr.oculus.Debug.Property"),
		*NSLOCTEXT("OculusRift", "CCommandText_Property",
			"Oculus Rift specific extension.\nRead or write an OVR property.").ToString(),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateRaw(InHMDPtr, &FOculusRiftHMD::OvrPropertyCommandHandler))
	, StatsCommand(TEXT("vr.oculus.Debug.bShowStats"),
		*NSLOCTEXT("OculusRift", "CCommandText_Stats",
			"Oculus Rift specific extension.\nEnable or disable rendering of stats.").ToString(),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateRaw(InHMDPtr, &FOculusRiftHMD::StatsCommandHandler))
	, GridCommand(TEXT("vr.oculus.Debug.bDrawGrid"),
		*NSLOCTEXT("OculusRift", "CCommandText_Grid",
			"Oculus Rift specific extension.\nEnable or disable rendering of debug grid.").ToString(),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateRaw(InHMDPtr, &FOculusRiftHMD::GridCommandHandler))
	, CubemapCommand(TEXT("vr.oculus.Debug.CaptureCubemap"),
		*NSLOCTEXT("OculusRift", "CCommandText_Cubemap",
			"Oculus Rift specific extension.\nCaptures a cubemap for Oculus Home.\nOptional arguments (default is zero for all numeric arguments):\n  xoff=<float> -- X axis offset from the origin\n  yoff=<float> -- Y axis offset\n  zoff=<float> -- Z axis offset\n  yaw=<float>  -- the direction to look into (roll and pitch is fixed to zero)\n  gearvr       -- Generate a GearVR format cubemap\n    (height of the captured cubemap will be 1024 instead of 2048 pixels)\n").ToString(),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(CubemapCommandHandler))
#endif // !UE_BUILD_SHIPPING
{
}

/// @endcond

// Translate between legacy mirror mode values to the new ones
static EMirrorWindowMode TranslateMirrorMode(int32 ObsoleteMode)
{
	switch (ObsoleteMode)
	{
	case 0:
		return EMirrorWindowMode::Distorted;
	case 1:
		return EMirrorWindowMode::Undistorted;
	case 2:
		return EMirrorWindowMode::SingleEye;
	case 3:
		return EMirrorWindowMode::SingleEyeLetterboxed;
	case 4:
		return EMirrorWindowMode::SingleEyeCroppedToFill;
	default:
		return EMirrorWindowMode::Disabled;
	}
}

/// @cond DOXYGEN_WARNINGS

/**
Clutch to support setting the r.ScreenPercentage and make the equivalent change to PixelDensity

As we don't want to default to 100%, we ignore the value if the flags indicate the value is set by the constructor or scalability settings.
*/
void FOculusRiftHMD::CVarSinkHandler()
{
	if (GEngine && GEngine->HMDDevice.IsValid())
	{
		auto HMDType = GEngine->HMDDevice->GetHMDDeviceType();
		if (HMDType == EHMDDeviceType::DT_OculusRift)
		{
			FOculusRiftHMD* HMD = static_cast<FOculusRiftHMD*>(GEngine->HMDDevice.Get());
			if (HMD->GetSettings()->UpdatePixelDensityFromScreenPercentage())
			{
				HMD->Flags.bNeedUpdateStereoRenderingParams = true;
			}
		}
	}
}

FAutoConsoleVariableSink FOculusRiftHMD::CVarSink(FConsoleCommandDelegate::CreateStatic(&FOculusRiftHMD::CVarSinkHandler));

void FOculusRiftHMD::PixelDensityCommandHandler(const TArray<FString>& Args, UWorld*, FOutputDevice& Ar)
{
	if (Args.Num())
	{
		SetPixelDensity(FCString::Atof(*Args[0]));
		Flags.bNeedUpdateStereoRenderingParams = true;
	}
	Ar.Logf(TEXT("vr.oculus.PixelDensity = \"%1.2f\""), GetSettings()->PixelDensity);
}

void FOculusRiftHMD::PixelDensityMinCommandHandler(const TArray<FString>& Args, UWorld*, FOutputDevice& Ar)
{
	FSettings* CurrentSettings = GetSettings();
	if (Args.Num())
	{
		CurrentSettings->PixelDensityMin = FMath::Clamp(FCString::Atof(*Args[0]), ClampPixelDensityMin, ClampPixelDensityMax);
		CurrentSettings->PixelDensityMax = FMath::Max(CurrentSettings->PixelDensityMin, CurrentSettings->PixelDensityMax);
		float NewPixelDensity = FMath::Clamp(CurrentSettings->PixelDensity, CurrentSettings->PixelDensityMin, CurrentSettings->PixelDensityMax);
		if (!FMath::IsNearlyEqual(NewPixelDensity, CurrentSettings->PixelDensity))
		{
			CurrentSettings->PixelDensity = NewPixelDensity;
			CurrentSettings->UpdateScreenPercentageFromPixelDensity();
		}
		Flags.bNeedUpdateStereoRenderingParams = true;
	}
	Ar.Logf(TEXT("vr.oculus.PixelDensity.min = \"%1.2f\""), CurrentSettings->PixelDensityMin);
}

void FOculusRiftHMD::PixelDensityMaxCommandHandler(const TArray<FString>& Args, UWorld*, FOutputDevice& Ar)
{
	FSettings* CurrentSettings = GetSettings();
	if (Args.Num())
	{
		GetSettings()->PixelDensityMax = FMath::Clamp(FCString::Atof(*Args[0]), ClampPixelDensityMin, ClampPixelDensityMax);
		CurrentSettings->PixelDensityMin = FMath::Min(CurrentSettings->PixelDensityMin, CurrentSettings->PixelDensityMax);
		float NewPixelDensity = FMath::Clamp(CurrentSettings->PixelDensity, CurrentSettings->PixelDensityMin, CurrentSettings->PixelDensityMax);
		if (!FMath::IsNearlyEqual(NewPixelDensity, CurrentSettings->PixelDensity))
		{
			CurrentSettings->PixelDensity = NewPixelDensity;
			CurrentSettings->UpdateScreenPercentageFromPixelDensity();
		}
		Flags.bNeedUpdateStereoRenderingParams = true;
	}
	Ar.Logf(TEXT("vr.oculus.PixelDensity.max = \"%1.2f\""), CurrentSettings->PixelDensityMax);
}

void FOculusRiftHMD::PixelDensityAdaptiveCommandHandler(const TArray<FString>& Args, UWorld*, FOutputDevice& Ar)
{
	FSettings* CurrentSettings = GetSettings();
	BOOLEAN_COMMAND_HANDLER_BODY(TEXT("vr.oculus.PixelDensity.adaptive"), CurrentSettings->bPixelDensityAdaptive)
	Flags.bNeedUpdateStereoRenderingParams = true;
}

void FOculusRiftHMD::HQBufferCommandHandler(const TArray<FString>& Args, UWorld*, FOutputDevice& Ar)
{
	FSettings* CurrentSettings = GetSettings();
	bool OldValue = CurrentSettings->Flags.bHQBuffer;
	BOOLEAN_COMMAND_HANDLER_BODY(TEXT("vr.oculus.bHQBuffer"), CurrentSettings->Flags.bHQBuffer)
	if (OldValue != CurrentSettings->Flags.bHQBuffer && pCustomPresent != nullptr)
	{
		pCustomPresent->MarkTexturesInvalid();
	}
}

void FOculusRiftHMD::HQDistortionCommandHandler(const TArray<FString>& Args, UWorld*, FOutputDevice& Ar)
{
	FSettings* CurrentSettings = GetSettings();
	bool OldValue = CurrentSettings->Flags.bHQDistortion;
	BOOLEAN_COMMAND_HANDLER_BODY(TEXT("vr.oculus.bHQDistortion"), CurrentSettings->Flags.bHQDistortion)
		if (OldValue != CurrentSettings->Flags.bHQDistortion && pCustomPresent != nullptr)
		{
			pCustomPresent->MarkTexturesInvalid();
		}
}

#if !UE_BUILD_SHIPPING
void FOculusRiftHMD::OvrPropertyCommandHandler(const TArray<FString>& Args, UWorld*, FOutputDevice& Ar)
{
	static const FString TypeNames[4] = { TEXT("bool") , TEXT("string"), TEXT("float"), TEXT("int") };
	enum EPropType
	{
		PTInvalid = -1,
		PTBool,
		PTString,
		PTFloat,
		PTInt
	};
	EPropType PropType = PTInvalid;
	FString PropName;
	
	if (Args.Num() >= 2)
	{
		for (int I = 0; I < ARRAY_COUNT(TypeNames); I++)
		{
			if (Args[0].Equals(TypeNames[I], ESearchCase::IgnoreCase))
			{
				PropType = static_cast<EPropType>(I);
				break;
			}
		}
		PropName = Args[1];
	}

	if (PropType == PTInvalid || PropName.IsEmpty() )
	{
		Ar.Log(TEXT("Usage: vr.oculus.Debug.Property bool|string|float|int <name> [value]"));
		return;
	}
	
	FOvrSessionShared::AutoSession OvrSession(Session);
	ANSICHAR* PropNameAnsi = TCHAR_TO_ANSI(*PropName);
	if (Args.Num() >= 3) // Set
	{
		ovrBool Success = ovrFalse;
		switch (PropType)
		{
		case PTBool:
			Success = ovr_SetBool(OvrSession, PropNameAnsi, FCString::ToBool(*Args[2]) ? ovrTrue : ovrFalse);
			break;
		case PTString:
			Success = ovr_SetString(OvrSession, PropNameAnsi, TCHAR_TO_UTF8(*Args[2]));
			break;
		case PTFloat:
			Success = ovr_SetFloat(OvrSession, PropNameAnsi, FCString::Atof(*Args[2]));
			break;
		case PTInt:
			Success = ovr_SetInt(OvrSession, PropNameAnsi, FCString::Atoi(*Args[2]));
			break;
		default:
			check(false);
			break;
		}
		Ar.Logf(TEXT("vr.oculus.Debug.Property %s %s: %s"), *Args[0], *PropName, Success ? TEXT("Value stored successfully") : TEXT("Could not store value."));
	}
	else // Get
	{
		FString StringValue;
		switch (PropType)
		{
		case PTBool:
			StringValue = ovr_GetBool(OvrSession, PropNameAnsi, ovrFalse) ? TEXT("True") : TEXT("False");
			break;
		case PTString:
			StringValue = FString::Printf(TEXT("\"%s\""), UTF8_TO_TCHAR(ovr_GetString(OvrSession, PropNameAnsi, "")));
			break;
		case PTFloat:
			StringValue = FString::Printf(TEXT("%f"), ovr_GetFloat(OvrSession, PropNameAnsi, 0));
			break;
		case PTInt:
			StringValue = FString::FromInt(ovr_GetInt(OvrSession, PropNameAnsi, 0));
			break;
		default:
			check(false);
			break;
		}
		Ar.Logf(TEXT("vr.oculus.Debug.Property %s %s = %s"), *Args[0], *PropName, *StringValue);
	}
}

void FOculusRiftHMD::StatsCommandHandler(const TArray<FString>& Args, UWorld*, FOutputDevice& Ar)
{
	BOOLEAN_COMMAND_HANDLER_BODY(TEXT("vr.oculus.Debug.bShowStats"), Settings->Flags.bShowStats)
}

void FOculusRiftHMD::GridCommandHandler(const TArray<FString>& Args, UWorld*, FOutputDevice& Ar)
{
	BOOLEAN_COMMAND_HANDLER_BODY(TEXT("vr.oculus.Debug.bDrawGrid"), Settings->Flags.bDrawGrid)
}

/// @endcond

const uint32 CaptureHeight = 2048;
static void CubemapCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	bool bCreateGearVRCubemap = false;
	FVector CaptureOffset(FVector::ZeroVector);
	float Yaw = 0.f;
	for (const FString& Arg : Args)
	{
		FParse::Value(*Arg, TEXT("XOFF="), CaptureOffset.X);
		FParse::Value(*Arg, TEXT("YOFF="), CaptureOffset.Y);
		FParse::Value(*Arg, TEXT("ZOFF="), CaptureOffset.Z);
		FParse::Value(*Arg, TEXT("YAW="), Yaw);
		if (Arg.Equals(TEXT("GEARVR"), ESearchCase::IgnoreCase))
		{
			bCreateGearVRCubemap = true;
		}
	}

	USceneCubemapCapturer*	CubemapCapturer = NewObject<USceneCubemapCapturer>();
	CubemapCapturer->AddToRoot(); // TODO: Don't add the object to the GC root
	CubemapCapturer->SetOffset(CaptureOffset);
	if (Yaw != 0.f)
	{
		FRotator Rotation(FRotator::ZeroRotator);
		Rotation.Yaw = Yaw;
		const FQuat Orient(Rotation);
		CubemapCapturer->SetInitialOrientation(Orient);
	}
	CubemapCapturer->StartCapture(World, bCreateGearVRCubemap ? CaptureHeight / 2 : CaptureHeight);
}

/// @cond DOXYGEN_WARNINGS

void FOculusRiftHMD::EnforceHeadTrackingCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	bool bOldValue = Settings->Flags.bHeadTrackingEnforced;
	FHeadMountedDisplay::EnforceHeadTrackingCommandHandler(Args, World, Ar);
	if (!bOldValue && Settings->Flags.bHeadTrackingEnforced)
	{
		InitDevice();
	}
}

#endif // !UE_BUILD_SHIPPING

bool FOculusRiftConsoleCommands::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	const TCHAR* OrigCmd = Cmd;
	FString AliasedCommand;

	if (FParse::Command(&Cmd, TEXT("HMD")))
	{
		if (FParse::Command(&Cmd, TEXT("HQBUF")))
		{
			FString CmdName = FParse::Token(Cmd, 0).ToLower();
			if (CmdName.IsEmpty())
			{
				CmdName = TEXT("toggle");
			}
			AliasedCommand = FString::Printf(TEXT("vr.oculus.bHQBuffer %s"), *CmdName);
		}
		else if (FParse::Command(&Cmd, TEXT("HQDISTORTION")))
		{
			FString CmdName = FParse::Token(Cmd, 0).ToLower();
			if (CmdName.IsEmpty())
			{
				CmdName = TEXT("toggle");
			}
			AliasedCommand = FString::Printf(TEXT("vr.oculus.bHQDistortion %s"), *CmdName);
		}
		else if (FParse::Command(&Cmd, TEXT("MIRROR"))) // to mirror or not to mirror?...
		{
			// HMD MIRROR is obsolete. Translate to a equivalent vr.MirrorMode command.
			// For toggling on and off, use negative values to store the previously used mirror mode.
			static const auto CVarMirrorMode = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("vr.MirrorMode"));
			int32  CurrentMirrorWindowMode = CVarMirrorMode->GetValueOnGameThread();
			AliasedCommand = TEXT("vr.MirrorMode");
			FString CmdName = FParse::Token(Cmd, 0);
			if (!CmdName.IsEmpty())
			{
				if (!FCString::Stricmp(*CmdName, TEXT("ON")))
				{
					AliasedCommand = FString::Printf(TEXT("vr.MirrorMode %d"), CurrentMirrorWindowMode < 0 ? -CurrentMirrorWindowMode : CurrentMirrorWindowMode == 0 ? 1 : CurrentMirrorWindowMode);
				}
				else if (!FCString::Stricmp(*CmdName, TEXT("OFF")))
				{
					AliasedCommand = FString::Printf(TEXT("vr.MirrorMode %d"), CurrentMirrorWindowMode > 0 ? -CurrentMirrorWindowMode : CurrentMirrorWindowMode);
				}
				else if (!FCString::Stricmp(*CmdName, TEXT("MODE")))
				{
					// vr.MirrorMode uses a different numbering. Translate from the old values to new.
					FString ModeName = FParse::Token(Cmd, 0);
					EMirrorWindowMode NewMode = TranslateMirrorMode(FCString::Atoi(*ModeName));
					AliasedCommand = FString::Printf(TEXT("vr.MirrorMode %d"), CurrentMirrorWindowMode > 0 ? (int32)NewMode : -(int32)NewMode);
				}
				else if (!FCString::Stricmp(*CmdName, TEXT("RESET")))
				{
					AliasedCommand = TEXT("vr.MirrorMode 1");
				}
				else
				{
					return false;
				}
			}
			else
			{
				AliasedCommand = FString::Printf(TEXT("vr.MirrorMode %d"), CurrentMirrorWindowMode == 0 ? 1 : -CurrentMirrorWindowMode);
			}
		}
#if !UE_BUILD_SHIPPING
		else if (FParse::Command(&Cmd, TEXT("STATS"))) // status / statistics
		{
			AliasedCommand = TEXT("vr.Oculus.Debug.bShowStats toggle");
		}
		else if (FParse::Command(&Cmd, TEXT("GRID"))) // grid
		{
			AliasedCommand = TEXT("vr.Oculus.Debug.bDrawGrid toggle");
		}
		else if (FParse::Command(&Cmd, TEXT("CUBEMAP")))
		{
			AliasedCommand = FString::Printf(TEXT("vr.Oculus.Debug.CaptureCubemap%s"), Cmd);
		}
#endif //UE_BUILD_SHIPPING
		else
		{
			FString CmdName = FParse::Token(Cmd, 0);
			// PD PDMIN PDMAX PDADAPTIVE
			if (CmdName.StartsWith(TEXT("PD")))
			{
				AliasedCommand = TEXT("vr.oculus.PixelDensity");
				if (CmdName.Len() > 2)
				{
					AliasedCommand += TEXT(".") + CmdName.RightChop(2).ToLower();
				}
				FString ValueStr = FParse::Token(Cmd, 0);
				if (!ValueStr.IsEmpty())
				{
					AliasedCommand += TEXT(" ") + ValueStr;
				}
			}
#if !UE_BUILD_SHIPPING
			else if (CmdName.StartsWith(TEXT("SET")) || CmdName.StartsWith(TEXT("GET")))
			{
				AliasedCommand = FString::Printf(TEXT("vr.oculus.Debug.Property %s %s"), *CmdName.RightChop(3).ToLower(), Cmd);
			}
#endif // #if !UE_BUILD_SHIPPING
			else
			{
				return false;
			}
		}
	}
	else if (FParse::Command(&Cmd, TEXT("OVRVERSION")))
	{
		AliasedCommand = TEXT("vr.HMDVersion");
	}

	if (!AliasedCommand.IsEmpty())
	{
		Ar.Logf(ELogVerbosity::Warning, TEXT("%s is deprecated. Use %s instead"), OrigCmd, *AliasedCommand);
		return IConsoleManager::Get().ProcessUserConsoleInput(*AliasedCommand, Ar, InWorld);
	}

	return false;
}

void FOculusRiftHMD::LoadFromIni()
{
	const TCHAR* OculusSettings = TEXT("Oculus.Settings");
	bool v;
	float f;
	int i;
	FVector vec;
	if (GConfig->GetBool(OculusSettings, TEXT("bChromaAbCorrectionEnabled"), v, GEngineIni))
	{
		Settings->Flags.bChromaAbCorrectionEnabled = v;
	}
#if !UE_BUILD_SHIPPING
	if (GConfig->GetBool(OculusSettings, TEXT("bOverrideIPD"), v, GEngineIni))
	{
		Settings->Flags.bOverrideIPD = v;
		if (Settings->Flags.bOverrideIPD)
		{
			if (GConfig->GetFloat(OculusSettings, TEXT("IPD"), f, GEngineIni))
			{
				check(!FMath::IsNaN(f));
				SetInterpupillaryDistance(FMath::Clamp(f, 0.0f, 1.0f));
			}
		}
	}
#endif // #if !UE_BUILD_SHIPPING
	if (GConfig->GetFloat(OculusSettings, TEXT("PixelDensityMax"), f, GEngineIni))
	{
		check(!FMath::IsNaN(f));
		GetSettings()->PixelDensityMax = FMath::Clamp(f, ClampPixelDensityMin, ClampPixelDensityMax);
	}
	if (GConfig->GetFloat(OculusSettings, TEXT("PixelDensityMin"), f, GEngineIni))
	{
		check(!FMath::IsNaN(f));
		GetSettings()->PixelDensityMin = FMath::Clamp(f, ClampPixelDensityMin, ClampPixelDensityMax);
	}
	if (GConfig->GetFloat(OculusSettings, TEXT("PixelDensity"), f, GEngineIni))
	{
		check(!FMath::IsNaN(f));
		GetSettings()->PixelDensity = FMath::Clamp(f, GetSettings()->PixelDensityMin, GetSettings()->PixelDensityMax);
	}
#if 0
	// TODO: getting AdaptiveGpuPerformanceScale too early may cause an illegal address exception. Ignore the saved INI value until that is fixed.
	if (GConfig->GetBool(OculusSettings, TEXT("bPixelDensityAdaptive"), v, GEngineIni))
	{
		GetSettings()->bPixelDensityAdaptive = v;
	}
#endif
	if (GConfig->GetBool(OculusSettings, TEXT("bHQBuffer"), v, GEngineIni))
	{
		Settings->Flags.bHQBuffer = v;
	}
	if (GConfig->GetBool(OculusSettings, TEXT("bHQDistortion"), v, GEngineIni))
	{
		Settings->Flags.bHQDistortion = v;
	}
	if (GConfig->GetBool(OculusSettings, TEXT("bUpdateOnRT"), v, GEngineIni))
	{
		Settings->Flags.bUpdateOnRT = v;
	}
	if (GConfig->GetFloat(OculusSettings, TEXT("FarClippingPlane"), f, GEngineIni))
	{
		check(!FMath::IsNaN(f));
		if (f < 0)
		{
			f = 0;
		}
		Settings->FarClippingPlane = f;
	}
	if (GConfig->GetFloat(OculusSettings, TEXT("NearClippingPlane"), f, GEngineIni))
	{
		check(!FMath::IsNaN(f));
		if (f < 0)
		{
			f = 0;
		}
		Settings->NearClippingPlane = f;
	}
	if (GConfig->GetInt(OculusSettings, TEXT("MirrorWindowMode"), i, GEngineIni))
	{
		// Translate the obsolete setting and store it in the vr.MirrorMode console var.
		// The old version used negative values to persist the mode when disabled.
		// Since vr.MirrorMode treats any value < 1 as disabled, we can keep using that convention.
		// (and it will even work for all modes including Distorted, as we no longer use 0 for that)
		static const auto CVarMirrorMode = IConsoleManager::Get().FindConsoleVariable(TEXT("vr.MirrorMode"));
		CVarMirrorMode->Set((i < 0 ? -1 : 1) * (int32)TranslateMirrorMode(FMath::Abs(i)), ECVF_SetBySystemSettingsIni);
	}
}

void FOculusRiftHMD::SaveToIni()
{
#if !UE_BUILD_SHIPPING
	const TCHAR* OculusSettings = TEXT("Oculus.Settings");
	GConfig->SetBool(OculusSettings, TEXT("bChromaAbCorrectionEnabled"), Settings->Flags.bChromaAbCorrectionEnabled, GEngineIni);

	GConfig->SetBool(OculusSettings, TEXT("bOverrideIPD"), Settings->Flags.bOverrideIPD, GEngineIni);
	if (Settings->Flags.bOverrideIPD)
	{
		GConfig->SetFloat(OculusSettings, TEXT("IPD"), GetInterpupillaryDistance(), GEngineIni);
	}

	// Don't save current (dynamically determined) pixel density if adaptive pixel density is currently enabled
	if (!GetSettings()->bPixelDensityAdaptive)
	{
		GConfig->SetFloat(OculusSettings, TEXT("PixelDensity"), GetSettings()->PixelDensity, GEngineIni);
	}
	GConfig->SetFloat(OculusSettings, TEXT("PixelDensityMin"), GetSettings()->PixelDensityMin, GEngineIni);
	GConfig->SetFloat(OculusSettings, TEXT("PixelDensityMax"), GetSettings()->PixelDensityMax, GEngineIni);
	GConfig->SetBool(OculusSettings, TEXT("bPixelDensityAdaptive"), GetSettings()->bPixelDensityAdaptive, GEngineIni);

	GConfig->SetBool(OculusSettings, TEXT("bHQBuffer"), Settings->Flags.bHQBuffer, GEngineIni);
	GConfig->SetBool(OculusSettings, TEXT("bHQDistortion"), Settings->Flags.bHQDistortion, GEngineIni);

	GConfig->SetBool(OculusSettings, TEXT("bUpdateOnRT"), Settings->Flags.bUpdateOnRT, GEngineIni);

	if (Settings->Flags.bClippingPlanesOverride)
	{
		GConfig->SetFloat(OculusSettings, TEXT("FarClippingPlane"), Settings->FarClippingPlane, GEngineIni);
		GConfig->SetFloat(OculusSettings, TEXT("NearClippingPlane"), Settings->NearClippingPlane, GEngineIni);
	}
#endif // #if !UE_BUILD_SHIPPING
}

/// @endcond

#endif
