// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * When we have an optional value IsSet() returns true, and GetValue() is meaningful.
 * Otherwise GetValue() is not meaningful.
 */
template<typename OptionalType>
struct TOptional
{
public:
	/** Construct an OptionaType with a valid value. */
	TOptional( OptionalType InValue )
		: bIsSet( true )
		, Value( InValue )
	{
	}

	/** Construct an OptionalType with no value; i.e. NULL */
	TOptional()
		: bIsSet( false )
		, Value()
	{
	}

	/** @return true when the value is meaningulf; false if calling GetValue() is undefined. */
	bool IsSet() const { return bIsSet; }

	/** @return The optional value; undefined when IsSet() returns false. */
	const OptionalType& GetValue() const { return Value; }

	/** @return The optional value when set; DefaultValue otherwise. */
	const OptionalType& Get( const OptionalType& DefaultValue ) const { return IsSet() ? Value : DefaultValue; }

private:
	bool bIsSet;
	OptionalType Value;
};