// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Variant.h: Declares the FVariant class.
=============================================================================*/

#pragma once


namespace EVariantTypes
{
	/**
	 * Enumerates the built-in types that can be stored in instances of FVariant.
	 */
	enum Type
	{
		Empty,
		Ansichar,
		Bool,
		Box,
		BoxSphereBounds,
		ByteArray,
		Color,
		DateTime,
		Double,
		Enum,
		Float,
		Guid,
		Int8,
		Int16,
		Int32,
		Int64,
		IntRect,
		LinearColor,
		Matrix,
		Name,
		Plane,
		Quat,
		RandomStream,
		Rotator,
		String,
		Widechar,
		Timespan,
		Transform,
		TwoVectors,
		UInt8,
		UInt16,
		UInt32,
		UInt64,
		Vector,
		Vector2d,
		Vector4,
		IntPoint,
		IntVector,
		NetworkGUID,
		Custom = 0x40
	};
}


/**
 * Stub for variant type traits.
 *
 * Actual type traits need to be declared through template specialization for custom
 * data types that are to be used in FVariant. Traits for the most commonly used built-in
 * types are declared below.
 *
 * Complex types, such as structures and classes can be serialized into a byte array
 * and then assigned to a variant. Note that you will be responsible for ensuring
 * correct byte ordering when serializing those types.
 *
 * @param T - The type to be used in FVariant.
 */
template<typename T> struct TVariantTraits
{
	static int32 GetType( )
	{
		checkAtCompileTime(!sizeof(T), Variant_trait_must_be_specialized_for_this_type);
	}
};


/**
 * Implements an extensible union of multiple types.
 *
 * Variant types can be used to store a range of different built-in types, as well as user
 * defined types. The values are internally serialized into a byte array, which means that
 * only FArchive serializable types are supported at this time.
 */
class FVariant
{
public:

	/**
	 * Default constructor.
	 */
	FVariant( )
		: Type(EVariantTypes::Empty)
	{ }

	/**
	 * Copy constructor.
	 */
	FVariant( const FVariant& Other )
		: Type(Other.Type)
		, Value(Other.Value)
	{ }

	/**
	 * Creates and initializes a new instance with the specified value.
	 *
	 * @param InValue - The initial value.
	 */
	template<typename T>
	FVariant( T InValue )
	{
		FMemoryWriter writer(Value, true);
		writer << InValue;

		Type = TVariantTraits<T>::GetType();
	}

	/**
	 * Creates and initializes a new instance from a byte array.
	 *
	 * Array values are passed straight through as an optimization. Please note that, if you
	 * serialize any complex types into arrays and then store them in FVariant, you will be
	 * responsible for ensuring byte ordering if the FVariant gets sent over the network.
	 *
	 * @param InValue- The initial value.
	 */
	FVariant( const TArray<uint8>& InArray )
		: Type(EVariantTypes::ByteArray)
		, Value(InArray)
	{ }

	/**
	 * Creates and initializes a new instance from a TCHAR string.
	 *
	 * @param InString - The initial value.
	 */
	FVariant( const TCHAR* InString )
	{
		*this = FString(InString);
	}


public:

	/**
	 * Copy assignment operator.
	 *
	 * @param Other - The value to assign.
	 *
	 * @return This instance.
	 */
	FVariant& operator=( const FVariant& Other )
	{
		if (&Other != this)
		{
			Value = Other.Value;
			Type = Other.Type;
		}

		return *this;
	}

	/**
	 * Assignment operator.
	 *
	 * @param T - The type of the value to assign.
	 * @param InValue - The value to assign.
	 *
	 * @return This instance.
	 */
	template<typename T>
	FVariant& operator=( T InValue )
	{
		FMemoryWriter Writer(Value, true);
		Writer << InValue;

		Type = TVariantTraits<T>::GetType();

		return *this;
	}

	/**
	 * Assignment operator for byte arrays.
	 *
	 * Array values are passed straight through as an optimization. Please note that, if you
	 * serialize any complex types into arrays and then store them in FVariant, you will be
	 * responsible for ensuring byte ordering if the FVariant gets sent over the network.
	 *
	 * @param InArray - The byte array to assign.
	 *
	 * @return This instance.
	 */
	FVariant& operator=( const TArray<uint8> InArray )
	{
		Type = EVariantTypes::ByteArray;
		Value = InArray;

		return *this;
	}

	/**
	 * Assignment operator for TCHAR strings.
	 *
	 * @param InString - The value to assign.
	 *
	 * @return This instance.
	 */
	FVariant& operator=( const TCHAR* InString )
	{
		*this = FString(InString);

		return *this;
	}


	/**
	 * Implicit conversion operator.
	 *
	 * @param T - The type to convert the value to.
	 *
	 * @return The value converted to the specified type.
	 */
	template<typename T>
	operator T( ) const
	{
		return GetValue<T>();
	}


public:

	/**
	 * Comparison operator for equality.
	 *
	 * @param Other - The variant to compare with.
	 *
	 * @return true if the values are equal, false otherwise.
	 */
	bool operator==( const FVariant& Other ) const
	{
		return ((Type == Other.Type) && (Value == Other.Value));
	}

	/**
	 * Comparison operator for inequality.
	 *
	 * @param Other - The variant to compare with.
	 *
	 * @return true if the values are not equal, false otherwise.
	 */
	bool operator!=( const FVariant& Other ) const
	{
		return ((Type != Other.Type) || (Value != Other.Value));
	}


public:

	/**
	 * Empties the value.
	 *
	 * @see IsEmpty
	 */
	void Empty( )
	{
		Type = EVariantTypes::Empty;

		Value.Empty();
	}

	/**
	 * Checks whether the value is empty.
	 *
	 * @return true if the value is empty, false otherwise.
	 *
	 * @see Empty
	 */
	bool IsEmpty( ) const
	{
		return (Type == EVariantTypes::Empty);
	}

	/**
	 * Gets the stored value as a byte array.
	 *
	 * This method returns the internal representation of any value as an
	 * array of raw bytes. To retrieve values of type TArray<uint8> use
	 * GetValue<TArray<uint8>>() instead.
	 *
	 * @return Byte array.
	 *
	 * @see GetValue
	 */
	const TArray<uint8>& GetBytes( ) const
	{
		return Value;
	}

	/**
	 * Gets the stored value's size (in bytes).
	 *
	 * @return Size of the value.
	 *
	 * @see GetType
	 * @see GetValue
	 */
	int32 GetSize( ) const
	{
		return Value.Num();
	}

	/**
	 * Gets the stored value's type.
	 *
	 * @return Type of the value.
	 *
	 * @see GetSize
	 * @see GetValue
	 */
	int32 GetType( ) const
	{
		return Type;
	}

	/**
	 * Gets the stored value.
	 *
	 * This template function does not provide any automatic conversion between
	 * convertible types. The exact type of the value to be extracted must be known.
	 *
	 * @return The value.
	 *
	 * @see GetSize
	 * @see GetType
	 */
	template<typename T>
	T GetValue( ) const
	{
		check((Type == TVariantTraits<T>::GetType()) || ((TVariantTraits<T>::GetType() == EVariantTypes::UInt8) && (Type == EVariantTypes::Enum)));

		T Result;

		FMemoryReader Reader(Value, true);
		Reader << Result;

		return Result;
	}


public:

	/**
	 * Serializes the given variant type from or into the specified archive.
	 *
	 * @param Ar - The archive to serialize from or into.
	 * @param Variant - The value to serialize.
	 *
	 * @return The archive.
	 */
	friend FArchive& operator<<( FArchive& Ar, FVariant& Variant )
	{
		return Ar << Variant.Type << Variant.Value;
	}


private:

	// Holds the type of the variant.
	int32 Type;

	// Holds the serialized value.
	TArray<uint8> Value;
};


/**
 * Gets the stored value for byte arrays.
 *
 * Array values are passed straight through as an optimization. Please note that, if you serialize
 * any complex types into arrays and then store them in FVariant, you will be responsible for
 * ensuring byte ordering if the FVariant gets send over the network.
 *
 * To retrieve any value as an array of serialized bytes, use GetBytes() instead.
 *
 * @return The byte array.
 *
 * @see GetBytes
 */
template<>
FORCEINLINE TArray<uint8> FVariant::GetValue<TArray<uint8> >( ) const
{
	check(Type == EVariantTypes::ByteArray);

	return Value;
}


/* Default FVariant traits for built-in types
 *****************************************************************************/

/**
 * Implements variant type traits for the built-in ANSICHAR type.
 */
template<> struct TVariantTraits<ANSICHAR>
{
	static int32 GetType( ) { return EVariantTypes::Ansichar; }
};

/**
 * Implements variant type traits for the built-in bool type.
 */
template<> struct TVariantTraits<bool>
{
	static int32 GetType( ) { return EVariantTypes::Bool; }
};

/**
 * Implements variant type traits for the built-in FBox type.
 */
template<> struct TVariantTraits<FBox>
{
	static int32 GetType( ) { return EVariantTypes::Box; }
};

/**
 * Implements variant type traits for the built-in FBoxSphereBounds type.
 */
template<> struct TVariantTraits<FBoxSphereBounds>
{
	static int32 GetType( ) { return EVariantTypes::BoxSphereBounds; }
};

/**
 * Implements variant type traits for byte arrays.
 */
template<> struct TVariantTraits<TArray<uint8> >
{
	static int32 GetType( ) { return EVariantTypes::ByteArray; }
};

/**
 * Implements variant type traits for the built-in FColor type.
 */
template<> struct TVariantTraits<FColor>
{
	static int32 GetType( ) { return EVariantTypes::Color; }
};

/**
 * Implements variant type traits for the built-in FDateTime type.
 */
template<> struct TVariantTraits<FDateTime>
{
	static int32 GetType( ) { return EVariantTypes::DateTime; }
};

/**
 * Implements variant type traits for the built-in double type.
 */
template<> struct TVariantTraits<double>
{
	static int32 GetType( ) { return EVariantTypes::Double; }
};

/**
 * Implements variant type traits for enumeration types.
 */
template<typename EnumType> struct TVariantTraits<TEnumAsByte<EnumType> >
{
	static int32 GetType( ) { return EVariantTypes::Enum; }
};

/**
 * Implements variant type traits for the built-in float type.
 */
template<> struct TVariantTraits<float>
{
	static int32 GetType( ) { return EVariantTypes::Float; }
};

/**
 * Implements variant type traits for the built-in FGuid type.
 */
template<> struct TVariantTraits<FGuid>
{
	static int32 GetType( ) { return EVariantTypes::Guid; }
};

/**
 * Implements variant type traits for the built-in int8 type.
 */
template<> struct TVariantTraits<int8>
{
	static int32 GetType( ) { return EVariantTypes::Int8; }
};

/**
 * Implements variant type traits for the built-in int16 type.
 */
template<> struct TVariantTraits<int16>
{
	static int32 GetType( ) { return EVariantTypes::Int16; }
};

/**
 * Implements variant type traits for the built-in int32 type.
 */
template<> struct TVariantTraits<int32>
{
	static int32 GetType( ) { return EVariantTypes::Int32; }
};

/**
 * Implements variant type traits for the built-in int64 type.
 */
template<> struct TVariantTraits<int64>
{
	static int32 GetType( ) { return EVariantTypes::Int64; }
};

/**
 * Implements variant type traits for the built-in FIntPoint type.
 */
template<> struct TVariantTraits<FIntPoint>
{
	static int32 GetType( ) { return EVariantTypes::IntPoint; }
};

/**
 * Implements variant type traits for the built-in FIntVector type.
 */
template<> struct TVariantTraits<FIntVector>
{
	static int32 GetType( ) { return EVariantTypes::IntVector; }
};

/**
 * Implements variant type traits for the built-in FIntRect type.
 */
template<> struct TVariantTraits<FIntRect>
{
	static int32 GetType( ) { return EVariantTypes::IntRect; }
};

/**
 * Implements variant type traits for the built-in FLinearColor type.
 */
template<> struct TVariantTraits<FLinearColor>
{
	static int32 GetType( ) { return EVariantTypes::LinearColor; }
};

/**
 * Implements variant type traits for the built-in FMatrix type.
 */
template<> struct TVariantTraits<FMatrix>
{
	static int32 GetType( ) { return EVariantTypes::Matrix; }
};

/**
 * Implements variant type traits for the built-in FPlane type.
 */
template<> struct TVariantTraits<FPlane>
{
	static int32 GetType( ) { return EVariantTypes::Plane; }
};

/**
 * Implements variant type traits for the built-in FQuat type.
 */
template<> struct TVariantTraits<FQuat>
{
	static int32 GetType( ) { return EVariantTypes::Quat; }
};

/**
 * Implements variant type traits for the built-in FName type.
 */
template<> struct TVariantTraits<FName>
{
	static int32 GetType( ) { return EVariantTypes::Name; }
};

/**
 * Implements variant type traits for the built-in FRandomStream type.
 */
template<> struct TVariantTraits<FRandomStream>
{
	static int32 GetType( ) { return EVariantTypes::RandomStream; }
};

/**
 * Implements variant type traits for the built-in FRotator type.
 */
template<> struct TVariantTraits<FRotator>
{
	static int32 GetType( ) { return EVariantTypes::Rotator; }
};

/**
 * Implements variant type traits for the built-in FString type.
 */
template<> struct TVariantTraits<FString>
{
	static int32 GetType( ) { return EVariantTypes::String; }
};

/**
 * Implements variant type traits for the built-in WIDECHAR type.
 */
template<> struct TVariantTraits<WIDECHAR>
{
	static int32 GetType( ) { return EVariantTypes::Widechar; }
};

/**
 * Implements variant type traits for the built-in FTimespan type.
 */
template<> struct TVariantTraits<FTimespan>
{
	static int32 GetType( ) { return EVariantTypes::Timespan; }
};

/**
 * Implements variant type traits for the built-in FTransform type.
 */
template<> struct TVariantTraits<FTransform>
{
	static int32 GetType( ) { return EVariantTypes::Transform; }
};

/**
 * Implements variant type traits for the built-in FTwoVectors type.
 */
template<> struct TVariantTraits<FTwoVectors>
{
	static int32 GetType( ) { return EVariantTypes::TwoVectors; }
};

/**
 * Implements variant type traits for the built-in uint8 type.
 */
template<> struct TVariantTraits<uint8>
{
	static int32 GetType( ) { return EVariantTypes::UInt8; }
};

/**
 * Implements variant type traits for the built-in uint16 type.
 */
template<> struct TVariantTraits<uint16>
{
	static int32 GetType( ) { return EVariantTypes::UInt16; }
};

/**
 * Implements variant type traits for the built-in uint32 type.
 */
template<> struct TVariantTraits<uint32>
{
	static int32 GetType( ) { return EVariantTypes::UInt32; }
};

/**
 * Implements variant type traits for the built-in uint64 type.
 */
template<> struct TVariantTraits<uint64>
{
	static int32 GetType( ) { return EVariantTypes::UInt64; }
};

/**
 * Implements variant type traits for the built-in FVector type.
 */
template<> struct TVariantTraits<FVector>
{
	static int32 GetType( ) { return EVariantTypes::Vector; }
};

/**
 * Implements variant type traits for the built-in FVector2D type.
 */
template<> struct TVariantTraits<FVector2D>
{
	static int32 GetType( ) { return EVariantTypes::Vector2d; }
};

/**
 * Implements variant type traits for the built-in FVector4 type.
 */
template<> struct TVariantTraits<FVector4>
{
	static int32 GetType( ) { return EVariantTypes::Vector4; }
};

/**
 * Implements variant type traits for the built-in NetworkGUID type.
 */
template<> struct TVariantTraits<FNetworkGUID>
{
	static int32 GetType( ) { return EVariantTypes::NetworkGUID; }
};

