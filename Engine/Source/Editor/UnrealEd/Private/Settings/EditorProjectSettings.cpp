// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "EditorProjectSettings.h"


EUnit ConvertDefaultInputUnits(UEditorProjectAppearanceSettings::EDefaultLocationUnit In)
{
	typedef UEditorProjectAppearanceSettings::EDefaultLocationUnit EDefaultLocationUnit;

	switch(In)
	{
	case EDefaultLocationUnit::Micrometers:		return EUnit::Micrometers;
	case EDefaultLocationUnit::Millimeters:		return EUnit::Millimeters;
	case EDefaultLocationUnit::Centimeters:		return EUnit::Centimeters;
	case EDefaultLocationUnit::Meters:			return EUnit::Meters;
	case EDefaultLocationUnit::Kilometers:		return EUnit::Kilometers;
	case EDefaultLocationUnit::Inches:			return EUnit::Inches;
	case EDefaultLocationUnit::Feet:			return EUnit::Feet;
	case EDefaultLocationUnit::Yards:			return EUnit::Yards;
	case EDefaultLocationUnit::Miles:			return EUnit::Miles;
	default:									return EUnit::Centimeters;
	}
}

EGlobalUnitDisplay ConvertGlobalUnitDisplay(EUnitDisplay In)
{
	switch(In)
	{
	case EUnitDisplay::None:		return EGlobalUnitDisplay::None;
	case EUnitDisplay::Metric:		return EGlobalUnitDisplay::Metric;
	case EUnitDisplay::Imperial:	return EGlobalUnitDisplay::Imperial;
	default:				 		return EGlobalUnitDisplay::Metric;
	}
}

UEditorProjectAppearanceSettings::UEditorProjectAppearanceSettings(const FObjectInitializer&)
	: UnitDisplay(EUnitDisplay::Metric)
	, DefaultInputUnits(EDefaultLocationUnit::Centimeters)
{

}

void UEditorProjectAppearanceSettings::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent )
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (Name == GET_MEMBER_NAME_CHECKED(UEditorProjectAppearanceSettings, UnitDisplay))
	{
		FUnitConversion::Settings().SetGlobalUnitDisplay(ConvertGlobalUnitDisplay(UnitDisplay));
	}
	else if (Name == GET_MEMBER_NAME_CHECKED(UEditorProjectAppearanceSettings, DefaultInputUnits))
	{
		FUnitConversion::Settings().SetDefaultInputUnit(ConvertDefaultInputUnits(DefaultInputUnits));
	}

	SaveConfig();
}

void UEditorProjectAppearanceSettings::PostInitProperties()
{
	Super::PostInitProperties();

	FUnitConversion::Settings().SetGlobalUnitDisplay(ConvertGlobalUnitDisplay(UnitDisplay));
	FUnitConversion::Settings().SetDefaultInputUnit(ConvertDefaultInputUnits(DefaultInputUnits));
}