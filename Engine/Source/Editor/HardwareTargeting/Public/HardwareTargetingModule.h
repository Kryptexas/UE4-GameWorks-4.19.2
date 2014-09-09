// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"
#include "HardwareTargetingSettings.h"

DECLARE_DELEGATE_OneParam(FOnHardwareClassChanged, EHardwareClass::Type)
DECLARE_DELEGATE_OneParam(FOnGraphicsPresetChanged, EGraphicsPreset::Type)

class IHardwareTargetingModule : public IModuleInterface
{
public:

	/** Singleton access to this module */
	HARDWARETARGETING_API static IHardwareTargetingModule& Get();

	/** Apply the current hardware targeting settings if they have changed */
	virtual void ApplyHardwareTargetingSettings() = 0;

	/** Make a new combo box for choosing a hardware class target */
	virtual TSharedRef<SWidget> MakeHardwareClassTargetCombo(FOnHardwareClassChanged OnChanged, TAttribute<EHardwareClass::Type> SelectedEnum) = 0;

	/** Make a new combo box for choosing a graphics preference */
	virtual TSharedRef<SWidget> MakeGraphicsPresetTargetCombo(FOnGraphicsPresetChanged OnChanged, TAttribute<EGraphicsPreset::Type> SelectedEnum) = 0;
};

