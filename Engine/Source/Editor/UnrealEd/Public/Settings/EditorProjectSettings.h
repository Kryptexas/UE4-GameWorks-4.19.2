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

/**
* 2D layer settings
*/
USTRUCT()
struct UNREALED_API FMode2DLayer
{
	GENERATED_USTRUCT_BODY()

	FMode2DLayer()
	: Name(TEXT("Default"))
	, Depth(0)
	{ }

	/** Whether snapping to surfaces in the world is enabled */
	UPROPERTY(EditAnywhere, config, Category = Layer)
	FString Name;

	/** The amount of depth to apply when snapping to surfaces */
	UPROPERTY(EditAnywhere, config, Category = Layer)
	float Depth;
};

UENUM()
enum class ELevelEditor2DAxis : uint8
{
	X,
	Y,
	Z
};

/**
* Implements the Project specific 2D viewport settings.
*/
UCLASS(config=Editor)
class UNREALED_API ULevelEditor2DSettings : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	/** If enabled will allow 2D mode */
	UPROPERTY(EditAnywhere, config, Category = General, meta = (DisplayName = "Enable 2D Mode"))
	bool bMode2DEnabled;

	/** When enabled and active snap layer is valid, items dropped in the viewport will be dropped on the appropriate Y distance away */
	UPROPERTY(EditAnywhere, config, Category = LayerSnapping, AdvancedDisplay)
	bool bEnableLayerSnap;

	/** Will try to use snap settings from SnapLayers */
	UPROPERTY(EditAnywhere, config, Category = LayerSnapping, AdvancedDisplay)
	int32 ActiveSnapLayerIndex;

	/** Snap axis */
	UPROPERTY(EditAnywhere, config, Category = LayerSnapping)
	ELevelEditor2DAxis SnapAxis;

	/** Snap layers that are displayed in the viewport toolbar */
	UPROPERTY(EditAnywhere, config, Category = LayerSnapping)
	TArray<FMode2DLayer> SnapLayers;
};

