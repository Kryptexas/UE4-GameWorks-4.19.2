// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "BlackboardKeyType.generated.h"

UCLASS(EditInlineNew, Abstract, CollapseCategories, AutoExpandCategories=(Blackboard))
class ENGINE_API UBlackboardKeyType : public UObject
{
	GENERATED_UCLASS_BODY()
		
	/** get ValueSize */
	FORCEINLINE uint16 GetValueSize() const { return ValueSize; }

	/** does it match settings in filter? */
	virtual bool IsAllowedByFilter(UBlackboardKeyType* FilterOb) const;

	/** convert value to text */
	virtual FString DescribeValue(const uint8* MemoryBlock) const;

	/** description of params for property view */
	virtual FString DescribeSelf() const;

	/** extract location from entry */
	virtual bool GetLocation(const uint8* MemoryBlock, FVector& Location) const;

	enum CompareResult
	{
		Less = -1,
		Equal = 0,
		Greater = 1,
		NotEqual = 1 // important, do not change
	};

	/** interprets given two memory blocks as "my" type and compares them */
	virtual int32 Compare(const uint8* MemoryBlockA, const uint8* MemoryBlockB) const;

protected:

	/** size of value for this type */
	uint16 ValueSize;

	/** helper function for reading typed data from memory block */
	template<typename T>
	static T GetValueFromMemory(const uint8* MemoryBlock)
	{
		return *((T*)MemoryBlock);
	}

	/** helper function for writing typed data to memory block, returns true if value has changed */
	template<typename T>
	static bool SetValueInMemory(uint8* MemoryBlock, const T& Value)
	{
		const bool bChanged = *((T*)MemoryBlock) != Value;
		*((T*)MemoryBlock) = Value;
		
		return bChanged;
	}
};
