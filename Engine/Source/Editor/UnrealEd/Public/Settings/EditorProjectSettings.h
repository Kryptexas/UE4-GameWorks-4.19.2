// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UnitConversion.h"

#include "EditorProjectSettings.generated.h"

/** UENUM to define the specific set of allowable unit types */
UENUM()
enum class EUnitDisplay : uint8
{
	None, Metric, Imperial
};

/** Editor project appearance settings. Stored in default config, per-project */
UCLASS(config=Editor, defaultconfig)
class UNREALED_API UEditorProjectAppearanceSettings : public UObject
{
public:
	GENERATED_BODY()
	UEditorProjectAppearanceSettings(const FObjectInitializer&);

	/** UENUM to define the specific set of allowable default units */
	UENUM()
	enum class EDefaultLocationUnit : uint8
	{
		Micrometers, Millimeters, Centimeters, Meters, Kilometers,
		Inches, Feet, Yards, Miles
	};

protected:
	/** Called when a property on this object is changed */
	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent ) override;
	virtual void PostInitProperties() override;

private:
	
	UPROPERTY(EditAnywhere, config, Category=Units, meta=(Tooltip="Specifies how to present applicable property units on editor interfaces."))
	EUnitDisplay UnitDisplay;

	UPROPERTY(EditAnywhere, config, Category=Units, meta=(Tooltip="The default units to use for location and distance measurements on editor user interfaces when not explicitly specified by the user."))
	EDefaultLocationUnit DefaultInputUnits;
};