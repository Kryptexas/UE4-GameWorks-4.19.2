// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "Core.h"
#include "UnrealTypeTraits.h"
#include "Containers/ContainerAllocationPolicies.h"
#include "UnrealTemplate.h"
#include "ArchiveBase.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUnion, Log, All);

/** Used to disambiguate methods that are overloaded for all possible subtypes of a TUnion where the subtypes may not be distinct. */
template<uint32>
struct TDisambiguater
{
	TDisambiguater() {}
};

class FNull
{
public:

	friend uint32 GetTypeHash(FNull)
	{
		return 0;
	}

	bool operator==(const FNull&) const
	{
		return true;
	}
	
	bool operator!=(const FNull&) const
	{
		return false;
	}
};

/**
 * Represents a type which is the union of several other types; i.e. it can have a value whose type is of any the union's subtypes.
 * This differs from C union types by being type-safe, and supporting non-trivial data types as subtypes.
 * Since a value for the union must be of a single subtype, the union stores potential values of different subtypes in overlapped memory, and keeps track of which one is currently valid.
 */
template<typename A,typename B = FNull,typename C = FNull,typename D = FNull,typename E = FNull,typename F = FNull>
class TUnion
{
public:

	/** Default constructor. */
	TUnion()
	:	CurrentSubtypeIndex(-1)
	{}

	/** Initialization constructor. */
	explicit TUnion(typename TCallTraits<A>::ParamType InValue,TDisambiguater<0> Disambiguater = TDisambiguater<0>())
	:	CurrentSubtypeIndex(-1)
	{
		SetSubtype<A>(InValue);
	}
	
	/** Initialization constructor. */
	explicit TUnion(typename TCallTraits<B>::ParamType InValue,TDisambiguater<1> Disambiguater = TDisambiguater<1>())
	:	CurrentSubtypeIndex(-1)
	{
		SetSubtype<B>(InValue);
	}
	
	/** Initialization constructor. */
	explicit TUnion(typename TCallTraits<C>::ParamType InValue,TDisambiguater<2> Disambiguater = TDisambiguater<2>())
	:	CurrentSubtypeIndex(-1)
	{
		SetSubtype<C>(InValue);
	}
	
	/** Initialization constructor. */
	explicit TUnion(typename TCallTraits<D>::ParamType InValue,TDisambiguater<3> Disambiguater = TDisambiguater<3>())
	:	CurrentSubtypeIndex(-1)
	{
		SetSubtype<D>(InValue);
	}
	
	/** Initialization constructor. */
	explicit TUnion(typename TCallTraits<E>::ParamType InValue,TDisambiguater<4> Disambiguater = TDisambiguater<4>())
	:	CurrentSubtypeIndex(-1)
	{
		SetSubtype<E>(InValue);
	}
	
	/** Initialization constructor. */
	explicit TUnion(typename TCallTraits<F>::ParamType InValue,TDisambiguater<5> Disambiguater = TDisambiguater<5>())
	:	CurrentSubtypeIndex(-1)
	{
		SetSubtype<F>(InValue);
	}

	/** Copy constructor. */
	TUnion(const TUnion& Other)
	:	CurrentSubtypeIndex(-1)
	{
		// Copy the value of the appropriate subtype from the other union
		switch(Other.CurrentSubtypeIndex)
		{
		case -1: break;
		case 0: SetSubtype<A>(Other.GetSubtype<A>()); break;
		case 1: SetSubtype<B>(Other.GetSubtype<B>()); break;
		case 2: SetSubtype<C>(Other.GetSubtype<C>()); break;
		case 3: SetSubtype<D>(Other.GetSubtype<D>()); break;
		case 4: SetSubtype<E>(Other.GetSubtype<E>()); break;
		case 5: SetSubtype<F>(Other.GetSubtype<F>()); break;
		default: ThrowError(TEXT("Unrecognized TUnion subtype")); break;
		};
	}

	/** Destructor. */
	~TUnion()
	{
		// Destruct any subtype value the union may have.
		Reset();
	}

	/** @return True if the union's value is of the given subtype. */
	template<typename Subtype>
	bool HasSubtype() const
	{
		// Determine the subtype's index and reference.
		int32 SubtypeIndex;
		const Subtype* SubtypeValuePointer;
		GetSubtypeIndexAndReference<Subtype,const Subtype*>(*this,SubtypeIndex,SubtypeValuePointer);

		return CurrentSubtypeIndex == SubtypeIndex;
	}

	/** If the union's current value is of the given subtype, sets the union's value to a NULL value. */
	template<typename Subtype>
	void ResetSubtype()
	{
		// Determine the subtype's index and reference.
		int32 SubtypeIndex;
		Subtype* SubtypeValuePointer;
		GetSubtypeIndexAndReference<Subtype,Subtype*>(*this,SubtypeIndex,SubtypeValuePointer);

		// Only reset the value if it is of the specified subtype.
		if(CurrentSubtypeIndex == SubtypeIndex)
		{
			CurrentSubtypeIndex = -1;

			// Destruct the subtype.
			SubtypeValuePointer->~Subtype();
		}
	}

	/** @return A reference to the union's value of the given subtype.  May only be called if the union's HasSubtype()==true for the given subtype. */
	template<typename Subtype>
	const Subtype& GetSubtype() const
	{
		// Determine the subtype's index and reference.
		int32 SubtypeIndex;
		const Subtype* SubtypeValuePointer;
		GetSubtypeIndexAndReference<Subtype,const Subtype*>(*this,SubtypeIndex,SubtypeValuePointer);

		// Validate that the union has a value of the requested subtype.
		check(CurrentSubtypeIndex == SubtypeIndex);

		return *SubtypeValuePointer;
	}

	/** @return A reference to the union's value of the given subtype.  May only be called if the union's HasSubtype()==true for the given subtype. */
	template<typename Subtype>
	Subtype& GetSubtype()
	{
		// Determine the subtype's index and reference.
		int32 SubtypeIndex;
		Subtype* SubtypeValuePointer;
		GetSubtypeIndexAndReference<Subtype,Subtype*>(*this,SubtypeIndex,SubtypeValuePointer);

		// Validate that the union has a value of the requested subtype.
		check(CurrentSubtypeIndex == SubtypeIndex);

		return *SubtypeValuePointer;
	}

	/** Replaces the value of the union with a value of the given subtype. */
	template<typename Subtype>
	void SetSubtype(typename TCallTraits<Subtype>::ParamType NewValue)
	{
		int32 SubtypeIndex;
		Subtype* SubtypeValuePointer;
		GetSubtypeIndexAndReference<Subtype,Subtype*>(*this,SubtypeIndex,SubtypeValuePointer);

		Reset();

		new(SubtypeValuePointer) Subtype(NewValue);

		CurrentSubtypeIndex = SubtypeIndex;
	}

	/** Sets the union's value to NULL. */
	void Reset()
	{
		switch(CurrentSubtypeIndex)
		{
		case 0: ResetSubtype<A>(); break;
		case 1: ResetSubtype<B>(); break;
		case 2: ResetSubtype<C>(); break;
		case 3: ResetSubtype<D>(); break;
		case 4: ResetSubtype<E>(); break;
		case 5: ResetSubtype<F>(); break;
		};
	}

	/** Hash function. */
	friend uint32 GetTypeHash(const TUnion& Union)
	{
		uint32 Result = GetTypeHash(Union.CurrentSubtypeIndex);

		switch(Union.CurrentSubtypeIndex)
		{
		case 0: Result ^= GetTypeHash(Union.GetSubtype<A>()); break;
		case 1: Result ^= GetTypeHash(Union.GetSubtype<B>()); break;
		case 2: Result ^= GetTypeHash(Union.GetSubtype<C>()); break;
		case 3: Result ^= GetTypeHash(Union.GetSubtype<D>()); break;
		case 4: Result ^= GetTypeHash(Union.GetSubtype<E>()); break;
		case 5: Result ^= GetTypeHash(Union.GetSubtype<F>()); break;
		default: ThrowError(TEXT("Unrecognized TUnion subtype")); break;
		};

		return Result;
	}

	/** Equality comparison. */
	bool operator==(const TUnion& Other) const
	{
		if(CurrentSubtypeIndex == Other.CurrentSubtypeIndex)
		{
			switch(CurrentSubtypeIndex)
			{
			case 0: return GetSubtype<A>() == Other.GetSubtype<A>(); break;
			case 1: return GetSubtype<B>() == Other.GetSubtype<B>(); break;
			case 2: return GetSubtype<C>() == Other.GetSubtype<C>(); break;
			case 3: return GetSubtype<D>() == Other.GetSubtype<D>(); break;
			case 4: return GetSubtype<E>() == Other.GetSubtype<E>(); break;
			case 5: return GetSubtype<F>() == Other.GetSubtype<F>(); break;
			default: ThrowError(TEXT("Unrecognized TUnion subtype")); break;
			};
		}
		else
		{
			return false;
		}
	}

	friend FArchive& operator<<(FArchive& Ar,TUnion& Union)
	{
		if(Ar.IsLoading())
		{
			Union.Reset();

			Ar << Union.CurrentSubtypeIndex;

			switch(Union.CurrentSubtypeIndex)
			{
			case 0: Ar << Union.InitSubtype<A>(); break;
			case 1: Ar << Union.InitSubtype<B>(); break;
			case 2: Ar << Union.InitSubtype<C>(); break;
			case 3: Ar << Union.InitSubtype<D>(); break;
			case 4: Ar << Union.InitSubtype<E>(); break;
			case 5: Ar << Union.InitSubtype<F>(); break;
			default: UE_LOG(LogUnion, Fatal,TEXT("Unrecognized TUnion subtype")); break;
			};
		}
		else
		{
			Ar << Union.CurrentSubtypeIndex;

			switch(Union.CurrentSubtypeIndex)
			{
			case 0: Ar << Union.GetSubtype<A>(); break;
			case 1: Ar << Union.GetSubtype<B>(); break;
			case 2: Ar << Union.GetSubtype<C>(); break;
			case 3: Ar << Union.GetSubtype<D>(); break;
			case 4: Ar << Union.GetSubtype<E>(); break;
			case 5: Ar << Union.GetSubtype<F>(); break;
			default: UE_LOG(LogUnion, Fatal,TEXT("Unrecognized TUnion subtype")); break;
			};
		}
		return Ar;
	}

private:

	/** The potential values for each subtype of the union. */
	union
	{
		TTypeCompatibleBytes<A> A;
		TTypeCompatibleBytes<B> B;
		TTypeCompatibleBytes<C> C;
		TTypeCompatibleBytes<D> D;
		TTypeCompatibleBytes<E> E;
		TTypeCompatibleBytes<F> F;
	} Values;

	/** The index of the subtype that the union's current value is of. */
	uint8 CurrentSubtypeIndex;

	/** Sets the union's value to a default value of the given subtype. */
	template<typename Subtype>
	Subtype& InitSubtype()
	{
		Subtype* NewSubtype = &GetSubtype<Subtype>();
		return *new(NewSubtype) Subtype;
	}

	/** Determines the index and reference to the potential value for the given union subtype. */
	template<typename Subtype,typename PointerType>
	static void GetSubtypeIndexAndReference(
		const TUnion& Union,
		int32& OutIndex,
		PointerType& OutValuePointer
		)
	{
		if(TAreTypesEqual<A,Subtype>::Value)
		{
			OutIndex = 0;
			OutValuePointer = (PointerType)&Union.Values.A;
		}
		else if(TAreTypesEqual<B,Subtype>::Value)
		{
			OutIndex = 1;
			OutValuePointer = (PointerType)&Union.Values.B;
		}
		else if(TAreTypesEqual<C,Subtype>::Value)
		{
			OutIndex = 2;
			OutValuePointer = (PointerType)&Union.Values.C;
		}
		else if(TAreTypesEqual<D,Subtype>::Value)
		{
			OutIndex = 3;
			OutValuePointer = (PointerType)&Union.Values.D;
		}
		else if(TAreTypesEqual<E,Subtype>::Value)
		{
			OutIndex = 4;
			OutValuePointer = (PointerType)&Union.Values.E;
		}
		else if(TAreTypesEqual<F,Subtype>::Value)
		{
			OutIndex = 5;
			OutValuePointer = (PointerType)&Union.Values.F;
		}
		else
		{
			checkAtCompileTime(
				TAreTypesEqual<TEMPLATE_PARAMETERS2(A,Subtype)>::Value ||
				TAreTypesEqual<TEMPLATE_PARAMETERS2(B,Subtype)>::Value ||
				TAreTypesEqual<TEMPLATE_PARAMETERS2(C,Subtype)>::Value ||
				TAreTypesEqual<TEMPLATE_PARAMETERS2(D,Subtype)>::Value ||
				TAreTypesEqual<TEMPLATE_PARAMETERS2(E,Subtype)>::Value ||
				TAreTypesEqual<TEMPLATE_PARAMETERS2(F,Subtype)>::Value,
				TypeIsNotSubtypeOfUnion
				);
			OutIndex = -1;
			OutValuePointer = NULL;
		}
	}
};

