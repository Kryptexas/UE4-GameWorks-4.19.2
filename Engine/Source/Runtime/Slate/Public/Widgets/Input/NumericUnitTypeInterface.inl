// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UnitConversion.h"

template<typename NumericType>
TNumericUnitTypeInterface<NumericType>::TNumericUnitTypeInterface(EUnit InUnits, bool bInAllowUnitRangeAdaption)
	: Units(InUnits), bAllowUnitRangeAdaption(bInAllowUnitRangeAdaption)
{}

template<typename NumericType>
FNumericUnit<NumericType> TNumericUnitTypeInterface<NumericType>::QuantizeUnitsToBestFit(const NumericType& InValue, EUnit InUnits) const
{
	return FUnitConversion::QuantizeUnitsToBestFit(InValue, InUnits);
}

template<typename NumericType>
FString TNumericUnitTypeInterface<NumericType>::ToString(const NumericType& Value) const
{
	using namespace LexicalConversion;

	const FNumericUnit<NumericType> Default(Value, Units);
	return ToSanitizedString(bAllowUnitRangeAdaption ? QuantizeUnitsToBestFit(Default.Value, Default.Units) : Default);
}

template<typename NumericType>
TOptional<NumericType> TNumericUnitTypeInterface<NumericType>::FromString(const FString& InString, const NumericType& InCurrentValue)
{
	using namespace LexicalConversion;

	// Always parse in as a double, to allow for input of higher-order units with decimal numerals into integral types (eg, inputting 0.5km as 500m)
	FNumericUnit<double> NewValue;
	bool bEvalResult = TryParseString( NewValue, *InString ) && FUnitConversion::AreUnitsCompatible( NewValue.Units, Units );
	if (bEvalResult)
	{
		// Convert the number into the correct units
		EUnit SourceUnits = NewValue.Units;
		if (SourceUnits == EUnit::Unspecified && bAllowUnitRangeAdaption)
		{
			// Use the display units of the current value
			SourceUnits = QuantizeUnitsToBestFit(InCurrentValue, Units).Units;
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