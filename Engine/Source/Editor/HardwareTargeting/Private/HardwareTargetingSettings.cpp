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
		FString AppliedHardwareEnumValue;
		const bool bFoundKey = GConfig->GetString(TEXT("AppliedHardwareTargetingSettings"), TEXT("AppliedTargetedHardwareClass"), AppliedHardwareEnumValue, GEditorIni);
		if (!bFoundKey)
		{
			return true;
		}
		else if (AppliedHardwareEnumValue.IsNumeric())
		{
			// We used to write out an int instead of a string
			const EHardwareClass::Type EnumValue = static_cast<EHardwareClass::Type>(FPlatformString::Atoi(*AppliedHardwareEnumValue));
			if (EnumValue != Settings->TargetedHardwareClass)
			{
				return true;
			}
		}
		else
		{
			UEnum* Enum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EHardwareClass"), true);
			check(Enum);
			if (Enum->GetEnumName(Settings->TargetedHardwareClass) != AppliedHardwareEnumValue)
			{
				return true;
			}
		}
	}

	{
		FString AppliedGraphicsEnumValue;
		const bool bFoundKey = GConfig->GetString(TEXT("AppliedHardwareTargetingSettings"), TEXT("AppliedDefaultGraphicsPerformance"), AppliedGraphicsEnumValue, GEditorIni);
		if (!bFoundKey)
		{
			return true;
		}
		else if (AppliedGraphicsEnumValue.IsNumeric())
		{
			// We used to write out an int instead of a string
			const EGraphicsPreset::Type EnumValue = EGraphicsPreset::Type(FPlatformString::Atoi(*AppliedGraphicsEnumValue));
			if (EnumValue != Settings->DefaultGraphicsPerformance)
			{
				return true;
			}
		}
		else
		{
			UEnum* Enum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EGraphicsPreset"), true);
			check(Enum);
			if (Enum->GetEnumName(Settings->DefaultGraphicsPerformance) != AppliedGraphicsEnumValue)
			{
				return true;
			}
		}
	}

	return false;
}

void UHardwareTargetingSettings::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent )
{
	SettingChangedEvent.Broadcast();
}