// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	EnumAsByte.h: FEnumAsByte template
=============================================================================*/

#pragma once

/**
 * Template to store enums as BYTEs with type safety
**/
template<class TEnum>
class TEnumAsByte
{
public:
	typedef TEnum EnumType;

	/**
	 * Default Constructor, like an enum it is uninitialized
	**/
	FORCEINLINE TEnumAsByte()
	{
	}
	/**
	 * Copy constructor
	 * @param InValue value to construct with 
	**/
	FORCEINLINE TEnumAsByte(const TEnumAsByte &InValue)
		: Value(InValue.Value)
	{
	}
	/**
	 * Constructor, initialize to the enum value
	 * @param InValue value to construct with 
	**/
	FORCEINLINE TEnumAsByte(TEnum InValue)
		: Value(InValue)
	{
	}
	/**
	 * Constructor, initialize to the int32 value
	 * @param InValue value to construct with 
	**/
	explicit FORCEINLINE TEnumAsByte(int32 InValue)
		: Value(InValue)
	{
	}
	/**
	 * Constructor, initialize to the int32 value
	 * @param InValue value to construct with 
	**/
	explicit FORCEINLINE TEnumAsByte(uint8 InValue)
		: Value(InValue)
	{
	}
	/**
	 * Assignment operator
	 * @param InValue value to set 
	**/
	FORCEINLINE TEnumAsByte& operator=(TEnumAsByte InValue)
	{
		Value = InValue.Value;
		return *this;
	}
	/**
	 * Assignment operator
	 * @param InValue value to set 
	**/
	FORCEINLINE TEnumAsByte& operator=(TEnum InValue)
	{
		Value = InValue;
		return *this;
	}

	/** Implicit conversion to TEnum. */
	operator TEnum() const
	{
		return TEnum(Value);
	}
	/** Comparison operator **/
	bool operator==(TEnum InValue) const
	{
		return Value == InValue;
	}
	/** Comparison operator **/
	bool operator==(TEnumAsByte InValue) const
	{
		return Value == InValue.Value;
	}
	/** Get the value */
	TEnum GetValue() const
	{
		return TEnum(Value);
	}
private:
	/** The actual value **/
	uint8 Value;
};

template<class T> struct TIsPODType<TEnumAsByte<T> > { enum { Value = true }; };


