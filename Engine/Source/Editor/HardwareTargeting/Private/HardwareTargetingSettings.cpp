// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "HardwareTargetingPrivatePCH.h"
#include "HardwareTargetingModule.h"
#include "HardwareTargetingSettings.h"
#include "Settings.h"
#include "Internationalization.h"
#include "SDecoratedEnumCombo.h"

UHardwareTargetingSettings::UHardwareTargetingSettings(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, TargetedHardwareClass(EHardwareClass::Desktop)
	, DefaultGraphicsPerformance(EGraphicsPreset::Maximum)
{
}

bool UHardwareTargetingSettings::HasPendingChanges() const
{
	UHardwareTargetingSettings* Settings = GetMutableDefault<UHardwareTargetingSettings>();

	{
		int32 AppliedHardwareEnumValue = 0;
		bool bFoundKey = GConfig->GetInt(TEXT("AppliedHardwareTargetingSettings"), TEXT("AppliedTargetedHardwareClass"), AppliedHardwareEnumValue, GEditorIni);
		if (!bFoundKey || (EHardwareClass::Type(AppliedHardwareEnumValue) != Settings->TargetedHardwareClass))
		{
			return true;
		}
	}

	{
		int32 AppliedGraphicsEnumValue = 0;
		bool bFoundKey = GConfig->GetInt(TEXT("AppliedHardwareTargetingSettings"), TEXT("AppliedDefaultGraphicsPerformance"), AppliedGraphicsEnumValue, GEditorIni);
		if (!bFoundKey || (EGraphicsPreset::Type(AppliedGraphicsEnumValue) != Settings->DefaultGraphicsPerformance))
		{
			return true;
		}
	}

	return false;
}

void UHardwareTargetingSettings::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent )
{
	SettingChangedEvent.Broadcast();
}