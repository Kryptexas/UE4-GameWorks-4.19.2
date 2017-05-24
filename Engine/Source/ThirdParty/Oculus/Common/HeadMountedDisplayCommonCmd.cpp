// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "HeadMountedDisplayCommon.h"
#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "PostProcess/PostProcessHMD.h"
#include "ScreenRendering.h"

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

FHMDCommonConsoleCommands::FHMDCommonConsoleCommands(class FHeadMountedDisplay* InHMDPtr)
	: UpdateOnRenderThreadCommand(TEXT("vr.oculus.bUpdateOnRenderThread"),
		*NSLOCTEXT("OculusRift", "CCommandText_UpdateRT",
			"Oculus Rift specific extension.\nEnables or disables updating on the render thread.").ToString(),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateRaw(InHMDPtr, &FHeadMountedDisplay::UpdateOnRenderThreadCommandHandler))
#if !UE_BUILD_SHIPPING
	, UpdateOnGameThreadCommand(TEXT("vr.oculus.bUpdateOnGameThread"),
		*NSLOCTEXT("OculusRift", "CCommandText_UpdateGT",
			"Oculus Rift specific extension.\nEnables or disables updating on the game thread.").ToString(),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateRaw(InHMDPtr, &FHeadMountedDisplay::UpdateOnGameThreadCommandHandler))
	, PositionOffsetCommand(TEXT("vr.oculus.Debug.PositionOffset"),
		*NSLOCTEXT("OculusRift", "CCommandText_PosOff",
			"Oculus Rift specific extension.\nAdd an offset tp the current position.").ToString(),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateRaw(InHMDPtr, &FHeadMountedDisplay::PositionOffsetCommandHandler))
	, EnforceHeadTrackingCommand(TEXT("vr.oculus.Debug.EnforceHeadTracking"),
		*NSLOCTEXT("OculusRift", "CCommandText_EnforceTracking",
			"Oculus Rift specific extension.\nSet to on to enforce head tracking even when not in stereo mode.").ToString(),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateRaw(InHMDPtr, &FHeadMountedDisplay::EnforceHeadTrackingCommandHandler))
#endif
	, ShowSettingsCommand(TEXT("vr.oculus.Debug.Show"),
		*NSLOCTEXT("OculusRift", "CCommandText_Show",
			"Oculus Rift specific extension.\nShows the current value of various stereo rendering params.").ToString(),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateRaw(InHMDPtr, &FHeadMountedDisplay::ShowSettingsCommandHandler))
	, ResetSettingsCommand(TEXT("vr.oculus.Debug.Reset"),
		*NSLOCTEXT("OculusRift", "CCommandText_Reset",
			"Oculus Rift specific extension.\nResets various stereo rendering params back to the original setting.").ToString(),
		FConsoleCommandDelegate::CreateRaw(InHMDPtr, &FHeadMountedDisplay::ResetStereoRenderingParams))
	, IPDCommand(TEXT("vr.oculus.Debug.IPD"),
		*NSLOCTEXT("OculusRift", "CCommandText_IPD",
			"Oculus Rift specific extension.\nShows or changes the current interpupillary distance in meters.").ToString(),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateRaw(InHMDPtr, &FHeadMountedDisplay::IPDCommandHandler))
	, FCPCommand(TEXT("vr.oculus.Debug.FCP"),
		*NSLOCTEXT("OculusRift", "CCommandText_FCP",
			"Oculus Rift specific extension.\nShows or overrides the current far clipping plane.").ToString(),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateRaw(InHMDPtr, &FHeadMountedDisplay::FCPCommandHandler))
	, NCPCommand(TEXT("vr.oculus.Debug.NCP"),
		*NSLOCTEXT("OculusRift", "CCommandText_NCP",
			"Oculus Rift specific extension.\nShows or overrides the current near clipping plane.").ToString(),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateRaw(InHMDPtr, &FHeadMountedDisplay::NCPCommandHandler))
{}

void FHeadMountedDisplay::UpdateOnRenderThreadCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	BOOLEAN_COMMAND_HANDLER_BODY(TEXT("vr.oculus.bUpdateOnRenderThread"), Settings->Flags.bUpdateOnRT);
}

#if !UE_BUILD_SHIPPING
void FHeadMountedDisplay::UpdateOnGameThreadCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	bool bUpdateOnGt = !Settings->Flags.bDoNotUpdateOnGT;
	BOOLEAN_COMMAND_HANDLER_BODY(TEXT("vr.oculus.bUpdateOnGameThread"), bUpdateOnGt);
	Settings->Flags.bDoNotUpdateOnGT = !bUpdateOnGt;
}

void FHeadMountedDisplay::PositionOffsetCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	if (Args.Num() == 1 && Args[0].Equals(TEXT("reset"), ESearchCase::IgnoreCase))
	{
		Settings->PositionOffset = FVector::ZeroVector;
	}
	else if (Args.Num() >= 3)
	{
		Settings->PositionOffset.X = FCString::Atof(*Args[0]);
		Settings->PositionOffset.Y = FCString::Atof(*Args[1]);
		Settings->PositionOffset.Z = FCString::Atof(*Args[2]);
	}
	Ar.Logf(TEXT("vr.oculus.Debug.PositionOffset = %.2f %.2f %.2f"), Settings->PositionOffset.X, Settings->PositionOffset.Y, Settings->PositionOffset.Z);
}

void FHeadMountedDisplay::EnforceHeadTrackingCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	if (Args.Num() > 0)
	{
		Settings->Flags.bHeadTrackingEnforced = Args[0].Equals(TEXT("toggle"), ESearchCase::IgnoreCase) ? !Settings->Flags.bHeadTrackingEnforced : FCString::ToBool(*Args[0]);
		if (!Settings->Flags.bHeadTrackingEnforced)
		{
			ResetControlRotation();
		}
	}
	Ar.Logf(TEXT("Enforced head tracking is %s"), Settings->Flags.bHeadTrackingEnforced ? TEXT("on") : TEXT("off"));
}
#endif

void FHeadMountedDisplay::ShowSettingsCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	Ar.Logf(TEXT("stereo ipd=%.4f\n nearPlane=%.4f farPlane=%.4f"), GetInterpupillaryDistance(),
		(Settings->NearClippingPlane) ? Settings->NearClippingPlane : GNearClippingPlane, Settings->FarClippingPlane);
}

void FHeadMountedDisplay::IPDCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	if (Args.Num() > 0)
	{
		float Value = FCString::Atof(*Args[0]);
		if (Value > 0)
		{
			SetInterpupillaryDistance(Value);
			Settings->Flags.bOverrideIPD = true;
		}
		else
		{
			Settings->Flags.bOverrideIPD = false;
		}
		Flags.bNeedUpdateStereoRenderingParams = true;
	}
	Ar.Logf(TEXT("vr.oculus.Debug.IPD = %f"), GetInterpupillaryDistance());
}

void FHeadMountedDisplay::FCPCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	if (Args.Num() > 0)
	{
		Settings->FarClippingPlane = FCString::Atof(*Args[0]);
		Settings->Flags.bClippingPlanesOverride = true;
	}
	Ar.Logf(TEXT("vr.oculus.Debug.FCP = %f"), Settings->FarClippingPlane);
}

void FHeadMountedDisplay::NCPCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	if (Args.Num() > 0)
	{
		Settings->NearClippingPlane = FCString::Atof(*Args[0]);
		Settings->Flags.bClippingPlanesOverride = true;
	}
	Ar.Logf(TEXT("vr.oculus.Debug.NCP = %f"), (Settings->NearClippingPlane) ? Settings->NearClippingPlane : GNearClippingPlane);
}

bool FHMDCommonConsoleCommands::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	const TCHAR* OrigCmd = Cmd;
	FString AliasedCommand;

	if (FParse::Command(&Cmd, TEXT("STEREO")))
	{
		
		if (FParse::Command(&Cmd, TEXT("RESET")))
		{
			AliasedCommand = TEXT("vr.oculus.Debug.Reset");
		}
		else if (FParse::Command(&Cmd, TEXT("SHOW")))
		{
			AliasedCommand = TEXT("vr.oculus.Debug.Show");
		}
		else
		{
			bool bValidCommand = false;
			float Value;
			if (FParse::Value(Cmd, TEXT("E="), Value))
			{
				AliasedCommand = FString::Printf(TEXT("vr.oculus.Debug.IPD %f"), Value);
				Ar.Logf(ELogVerbosity::Warning, TEXT("STEREO E=%f is deprecated. Use %s instead"), Value, *AliasedCommand);
				IConsoleManager::Get().ProcessUserConsoleInput(*AliasedCommand, Ar, InWorld);
				bValidCommand = true;
			}
			if (FParse::Value(Cmd, TEXT("FCP="), Value)) // far clipping plane override
			{
				AliasedCommand = FString::Printf(TEXT("vr.oculus.Debug.FCP %f"), Value);
				Ar.Logf(ELogVerbosity::Warning, TEXT("STEREO FCP=%f is deprecated. Use %s instead"), Value, *AliasedCommand);
				IConsoleManager::Get().ProcessUserConsoleInput(*AliasedCommand, Ar, InWorld);
				bValidCommand = true;
			}
			if (FParse::Value(Cmd, TEXT("NCP="), Value)) // near clipping plane override
			{
				AliasedCommand = FString::Printf(TEXT("vr.oculus.Debug.NCP %f"), Value);
				Ar.Logf(ELogVerbosity::Warning, TEXT("STEREO NCP=%f is deprecated. Use %s instead"), Value, *AliasedCommand);
				IConsoleManager::Get().ProcessUserConsoleInput(*AliasedCommand, Ar, InWorld);
				bValidCommand = true;
			}
			if (FParse::Value(Cmd, TEXT("CS="), Value))
			{
				AliasedCommand = FString::Printf(TEXT("vr.WorldToMetersScale %f"), Value*100.0);
				Ar.Logf(ELogVerbosity::Warning, TEXT("STEREO CS=%f is deprecated. Use %s instead"), Value, *AliasedCommand);
				IConsoleManager::Get().ProcessUserConsoleInput(*AliasedCommand, Ar, InWorld);
				bValidCommand = true;
			}

			return bValidCommand;
		}
	}
	else if (FParse::Command(&Cmd, TEXT("HMD")))
	{
		if (FParse::Command(&Cmd, TEXT("UPDATEONRT")))
		{
			FString CmdName = FParse::Token(Cmd, 0);
			AliasedCommand = FString::Printf (TEXT("vr.oculus.bUpdateOnRenderThread %s"), *(CmdName.IsEmpty()?TEXT("toggle"):CmdName.ToLower()));
		}
#if !UE_BUILD_SHIPPING
		else if (FParse::Command(&Cmd, TEXT("UPDATEONGT"))) // update on game thread
		{
			FString CmdName = FParse::Token(Cmd, 0);
			AliasedCommand = FString::Printf(TEXT("vr.oculus.bUpdateOnGameThread %s"), *(CmdName.IsEmpty() ? TEXT("toggle") : CmdName.ToLower()));

		}
#endif //UE_BUILD_SHIPPING
	}
#if !UE_BUILD_SHIPPING
	else if (FParse::Command(&Cmd, TEXT("HMDDBG")))
	{
		if (FParse::Command(&Cmd, TEXT("SHOWCAMERA")))
		{
			AliasedCommand = TEXT("vr.Debug.VisualizeTrackingSensors");
			FString CmdName = FParse::Token(Cmd, 0);
			if(!CmdName.IsEmpty())
			{ 
				AliasedCommand += FString::Printf(TEXT(" %s"), FCString::ToBool(*CmdName)?TEXT("True"):TEXT("False"));
			}
		}
		else if (FParse::Command(&Cmd, TEXT("POSOFF")))
		{
			AliasedCommand = TEXT("vr.oculus.Debug.PositionOffset");
			FString StrX = FParse::Token(Cmd, 0);
			FString StrY = FParse::Token(Cmd, 0);
			FString StrZ = FParse::Token(Cmd, 0);
			if (!StrZ.IsEmpty())
			{
				AliasedCommand += FString::Printf(TEXT(" %s %s %s"), *StrX, *StrY, *StrZ);
			}
			else if (!StrX.IsEmpty())
			{
				return false;
			}
		}
	}
#endif //!UE_BUILD_SHIPPING
	else if (FParse::Command(&Cmd, TEXT("HMDPOS")))
	{
		if (FParse::Command(&Cmd, TEXT("RESET")))
		{
			AliasedCommand = TEXT("vr.HeadTracking.Reset");
			FString YawStr = FParse::Token(Cmd, 0);
			if (!YawStr.IsEmpty())
			{
				AliasedCommand += TEXT(" ") + YawStr;
			}
		}
		else if (FParse::Command(&Cmd, TEXT("RESETPOS")))
		{
			AliasedCommand = TEXT("vr.HeadTracking.ResetPosition");
		}
		else if (FParse::Command(&Cmd, TEXT("RESETROT")))
		{
			AliasedCommand = TEXT("vr.HeadTracking.ResetOrientation");
			FString YawStr = FParse::Token(Cmd, 0);
			if (!YawStr.IsEmpty())
			{
				AliasedCommand += TEXT(" ") + YawStr;
			}
		}
		else if (FParse::Command(&Cmd, TEXT("ENFORCE")))
		{
			FString CmdName = FParse::Token(Cmd, 0);
			if (CmdName.IsEmpty())
				return false;
			AliasedCommand = TEXT("vr.oculus.Debug.EnforceHeadTracking ") + CmdName;
		}
		else if (FParse::Command(&Cmd, TEXT("SHOW")))
		{
			AliasedCommand = TEXT("vr.HeadTracking.Status");
		}
	}
	
	if (!AliasedCommand.IsEmpty())
	{
		Ar.Logf(ELogVerbosity::Warning, TEXT("%s is deprecated. Use %s instead"), OrigCmd, *AliasedCommand);
		return IConsoleManager::Get().ProcessUserConsoleInput(*AliasedCommand, Ar, InWorld);
	}
	return false;
}
