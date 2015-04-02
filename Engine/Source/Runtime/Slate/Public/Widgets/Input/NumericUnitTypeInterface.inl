// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UnitConversion.h"

template<typename NumericType>
TNumericUnitTypeInterface<NumericType>::TNumericUnitTypeInterface(EUnit InUnits, bool bInAllowUnitRangeAdaption)
	: Units(InUnits), bAllowUnitRangeAdaption(bInAllowUnitRangeAdaption)
{}

template<typename NumericType>
TNumericUnitTypeInterface<NumericType>::~TNumericUnitTypeInterface()
{
	FUnitConversion::Settings().OnDisplaySettingsChanged().RemoveAll(this);
}


template<typename NumericType>
void TNumericUnitTypeInterface<NumericType>::UseDefaultInputUnits()
{
	FUnitConversion::Settings().OnDisplaySettingsChanged().AddRaw(this, &TNumericUnitTypeInterface<NumericType>::OnGlobalUnitSettingChanged);
	OnGlobalUnitSettingChanged();
}

template<typename NumericType>
void TNumericUnitTypeInterface<NumericType>::OnGlobalUnitSettingChanged()
{
	const EUnit Default = FUnitConversion::Settings().GetDefaultInputUnit();
	if (FUnitConversion::AreUnitsCompatible(Default, Units))
	{
		DefaultInputUnits = Default;
	}
	else
	{
		DefaultInputUnits.Reset();
	}
}

template<typename NumericType>
FNumericUnit<NumericType> TNumericUnitTypeInterface<NumericType>::QuantizeUnitsToBestFit(const NumericType& InValue, EUnit InUnits) const
{
	// Use the default input units for 0
	if (InValue == 0 && DefaultInputUnits.IsSet() && FUnitConversion::AreUnitsCompatible(DefaultInputUnits.GetValue(), InUnits))
	{
		return FNumericUnit<NumericType>(0, FUnitConversion::ConvertToGlobalDisplayRange(DefaultInputUnits.GetValue()));
	}
	return FUnitConversion::QuantizeUnitsToBestFit(InValue, InUnits);
}

template<typename NumericType>
FString TNumericUnitTypeInterface<NumericType>::ToString(const NumericType& Value) const
{
	using namespace LexicalConversion;

	NumericType ValueToUse = Value;
	EUnit DisplayUnits = FUnitConversion::ConvertToGlobalDisplayRange(Units);

	if (DisplayUnits != Units)
	{
		ValueToUse = FUnitConversion::Convert(ValueToUse, Units, DisplayUnits);
	}
	
	return ToSanitizedString(bAllowUnitRangeAdaption ? QuantizeUnitsToBestFit(ValueToUse, DisplayUnits) : FNumericUnit<NumericType>(ValueToUse, DisplayUnits));
}

template<typename NumericType>
TOptional<NumericType> TNumericUnitTypeInterface<NumericType>::FromString(const FString& InString)
{
	using namespace LexicalConversion;

	// Always parse in as a double, to allow for input of higher-order units with decimal numerals into integral types (eg, inputting 0.5km as 500m)
	FNumericUnit<double> NewValue;
	bool bEvalResult = TryParseString( NewValue, *InString ) && FUnitConversion::AreUnitsCompatible( NewValue.Units, Units );
	if (bEvalResult)
	{
		// Convert the number into the correct units
		EUnit SourceUnits = NewValue.Units;
		if (SourceUnits == EUnit::Unspecified && DefaultInputUnits.IsSet())
		{
			// Use the default supplied input units
			SourceUnits = DefaultInputUnits.GetValue();
		}
		return FUnitConversion::Convert(NewValue.Value, SourceUnits, Units);
	}
	else
	{
		float FloatValue = 0.f;
		if (FMath::Eval( *InString, FloatValue  ))
		{
			return FloatValue;
		}
	}

	return TOptional<NumericType>();
}

template<typename NumericType>
bool TNumericUnitTypeInterface<NumericType>::IsCharacterValid(TCHAR InChar) const
{
	return true;
}