// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/** Interface to provide specific functionality for dealing with a numeric type. Currently includes string conversion functionality. */
template<typename NumericType>
struct INumericTypeInterface
{
	virtual ~INumericTypeInterface() {}

	/** Convert the type to/from a string */
	virtual FString ToString(const NumericType& Value) const = 0;
	virtual TOptional<NumericType> FromString(const FString& InString, const NumericType& InCurrentValue) = 0;

	/** Check whether the typed character is valid */
	virtual bool IsCharacterValid(TCHAR InChar) const = 0;
};

/** Default numeric type interface */
template<typename NumericType>
struct TDefaultNumericTypeInterface : INumericTypeInterface<NumericType>
{
	/** Convert the type to/from a string */
	virtual FString ToString(const NumericType& Value) const override
	{
		return LexicalConversion::ToSanitizedString(Value);
	}
	virtual TOptional<NumericType> FromString(const FString& InString, const NumericType& InCurrentValue) override
	{
		NumericType NewValue;
		bool bEvalResult = LexicalConversion::TryParseString( NewValue, *InString );
		if (!bEvalResult)
		{
			float FloatValue = 0.f;
			bEvalResult = FMath::Eval( *InString, FloatValue  );
			NewValue = FloatValue;
		}

		return bEvalResult ? NewValue : TOptional<NumericType>();
	}

	/** Check whether the typed character is valid */
	virtual bool IsCharacterValid(TCHAR InChar) const override;	
};

template<typename T>
bool TDefaultNumericTypeInterface<T>::IsCharacterValid(TCHAR InChar) const
{
	static FString ValidChars(TEXT("1234567890-.")); // for integers and doubles
	int32 OutUnused;
	return ValidChars.FindChar( InChar, OutUnused );
}

/** Specialized for floats */
template<>
inline bool TDefaultNumericTypeInterface<float>::IsCharacterValid(TCHAR InChar) const
{
	static FString ValidChars(TEXT("1234567890()-+=\\/.,*^%%"));
	int32 OutUnused;
	return ValidChars.FindChar( InChar, OutUnused );		
}

/** Forward declaration of types defined in UnitConversion.h */
enum class EUnit;
template<typename> struct FNumericUnit;

/**
 * Numeric interface that specifies how to interact with a number in a specific unit.
 * Include NumericUnitTypeInterface.inl for symbol definitions.
 */
template<typename NumericType>
struct TNumericUnitTypeInterface : TDefaultNumericTypeInterface<NumericType>
{
	/** The units that we should treat this value as */
	EUnit Units;

	/** True if we are allowed to display this unit to the user as a more relevant related unit type (e.g. 10000 m shows as 10 km) */
	bool bAllowUnitRangeAdaption;

	/** Constructor */
	TNumericUnitTypeInterface(EUnit InUnits, bool bInAllowUnitRangeAdaption = true);

	/** Convert this type to a string */
	virtual FString ToString(const NumericType& Value) const override;

	/** Attempt to parse a numeral with our units from the specified string. */
	virtual TOptional<NumericType> FromString(const FString& ValueString, const NumericType& InCurrentValue) override;

	/** Called by this class to quantize the specified value to the best unit */
	virtual FNumericUnit<NumericType> QuantizeUnitsToBestFit(const NumericType& InValue, EUnit InUnits) const;

	/** Check whether the specified typed character is valid */
	virtual bool IsCharacterValid(TCHAR InChar) const override;
};