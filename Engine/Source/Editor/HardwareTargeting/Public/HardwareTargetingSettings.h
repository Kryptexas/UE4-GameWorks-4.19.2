// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "HardwareTargetingSettings.generated.h"

/** Enum specifying a class of hardware */
UENUM()
namespace EHardwareClass
{
	enum Type
	{
		Desktop, Mobile
	};
}

/** Enum specifying a graphics preset preference */
UENUM()
namespace EGraphicsPreset
{
	enum Type
	{
		Maximum, Scalable
	};
}

/** Hardware targeting settings, stored in default config, per-project */
UCLASS(config=Editor, defaultconfig)
class HARDWARETARGETING_API UHardwareTargetingSettings
	: public UObject
{
	GENERATED_UCLASS_BODY()

	/** Enum specifying which class of hardware this game is targeting */
	UPROPERTY(config, EditAnywhere, category=None)
	TEnumAsByte<EHardwareClass::Type> TargetedHardwareClass;

	/** Enum specifying a graphics preset to use for this game */
	UPROPERTY(config, EditAnywhere, category=None)
	TEnumAsByte<EGraphicsPreset::Type> DefaultGraphicsPerformance;

	/** Check if these settings have any pending changes that require action */
	bool HasPendingChanges() const;

#if WITH_EDITOR
public:
	/** Returns an event delegate that is executed when a setting has changed. */
	DECLARE_EVENT(UHardwareTargetingSettings, FSettingChangedEvent);
	FSettingChangedEvent& OnSettingChanged( ) { return SettingChangedEvent; }

protected:
	/** Called when a property on this object is changed */
	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent ) override;

private:
	/** Holds an event delegate that is executed when a setting has changed. */
	FSettingChangedEvent SettingChangedEvent;
#endif
};