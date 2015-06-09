// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

template<class TInterface>
struct TWeakInterfacePtr
{
	TWeakInterfacePtr() : InterfaceInstance(nullptr) {}

	TWeakInterfacePtr(const UObject* Object)
	{
		InterfaceInstance = Cast<TInterface>(Object);
		if (InterfaceInstance != nullptr)
		{
			ObjectInstance = Object;
		}
	}

	TWeakInterfacePtr(TInterface& Interace)
	{
		InterfaceInstance = &Interace;
		ObjectInstance = Cast<UObject>(&Interace);
	}

	bool IsValid(bool bEvenIfPendingKill = false, bool bThreadsafeTest = false) const
	{
		return InterfaceInstance != nullptr && ObjectInstance.IsValid();
	}

	FORCEINLINE TInterface& operator*() const
	{
		return *InterfaceInstance;
	}

	/**
	* Dereference the weak pointer
	**/
	FORCEINLINE TInterface* operator->() const
	{
		return InterfaceInstance;
	}

	bool operator==(const UObject* Other)
	{
		return Other == ObjectInstance.Get();
	}

private:
	TWeakObjectPtr<UObject> ObjectInstance;
	TInterface* InterfaceInstance;
};

struct GAMEPLAYTASKS_API FGameplayResourceSet
{
	typedef uint16 FFlagContainer;
	typedef uint8 FResourceID;

	enum
	{
		MaxResources = sizeof(FFlagContainer)* 8
	};

private:
	FFlagContainer Flags;

public:
	/** Mind that this constructor takes _flags_ not individual IDs */
	explicit FGameplayResourceSet(FFlagContainer InFlags = 0) : Flags(InFlags)
	{}
	
	FFlagContainer GetFlags() const 
	{ 
		return Flags; 
	}
	bool IsEmpty() const
	{
		return Flags == 0;
	}
	FGameplayResourceSet& AddID(uint8 ResourceID)
	{
		ensure(ResourceID < MaxResources);
		Flags |= (1 << ResourceID);
		return *this;
	}
	FGameplayResourceSet& RemoveID(uint8 ResourceID)
	{
		ensure(ResourceID < MaxResources);
		Flags &= ~(1 << ResourceID);
		return *this;
	}
	bool HasID(uint8 ResourceID) const
	{
		ensure(ResourceID < MaxResources);
		return (Flags & (1 << ResourceID)) != 0;
	}
	FGameplayResourceSet& AddSet(FGameplayResourceSet Other)
	{
		Flags |= Other.Flags;
		return *this;
	}
	void Clear()
	{
		Flags = FFlagContainer(0);
	}
	bool HasAllIDs(FGameplayResourceSet Other) const
	{
		return (Flags & Other.Flags) == Flags;
	}
	bool HasAnyID(FGameplayResourceSet Other) const
	{
		return (Flags & Other.Flags) != 0;
	}
	FGameplayResourceSet GetOverlap(FGameplayResourceSet Other) const
	{
		return FGameplayResourceSet(Flags & Other.Flags);
	}
	FGameplayResourceSet GetDifference(FGameplayResourceSet Other) const
	{
		return FGameplayResourceSet(Flags & ~(Flags & Other.Flags));
	}

	bool operator==(const FGameplayResourceSet& Other) const
	{
		return Flags == Other.Flags;
	}

	static FGameplayResourceSet AllResources()
	{
		return FGameplayResourceSet(FFlagContainer(-1));
	}

	static FGameplayResourceSet NoResources()
	{
		return FGameplayResourceSet(FFlagContainer(0));
	}

	FString GetDebugDescription() const;
};