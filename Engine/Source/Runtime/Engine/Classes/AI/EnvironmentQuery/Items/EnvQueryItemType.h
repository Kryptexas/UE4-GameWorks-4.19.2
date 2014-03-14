// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvQueryItemType.generated.h"

UCLASS(Abstract)
class ENGINE_API UEnvQueryItemType : public UObject
{
	GENERATED_UCLASS_BODY()

	/** get ValueSize */
	FORCEINLINE uint16 GetValueSize() const { return ValueSize; }

	/** add filters for blackboard key selector */
	virtual void AddBlackboardFilters(struct FBlackboardKeySelector& KeySelector, UObject* FilterOwner) const;

	/** store value in blackboard entry */
	virtual bool StoreInBlackboard(struct FBlackboardKeySelector& KeySelector, class UBlackboardComponent* Blackboard, const uint8* RawData) const;

	/** unregister from known types in EnvQueryManager */
	virtual void FinishDestroy() OVERRIDE;

protected:

	/** size of value for this type */
	uint16 ValueSize;

	/** helper function for reading typed data from memory block */
	template<typename T>
	static T GetValueFromMemory(const uint8* MemoryBlock)
	{
		return *((T*)MemoryBlock);
	}

	/** helper function for writing typed data to memory block */
	template<typename T>
	static void SetValueInMemory(uint8* MemoryBlock, const T& Value)
	{
		*((T*)MemoryBlock) = Value;
	}
};
