// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTree/Blackboard/BlackboardKeyType.h"
#include "BlackboardKeyType_NativeEnum.generated.h"

UCLASS(EditInlineNew, meta=(DisplayName="Native Enum"))
class AIMODULE_API UBlackboardKeyType_NativeEnum : public UBlackboardKeyType
{
	GENERATED_UCLASS_BODY()

	typedef uint8 FDataType;
	static const FDataType InvalidValue;

	UPROPERTY(Category=Blackboard, EditDefaultsOnly)
	FString EnumName;

	UPROPERTY(Category=Blackboard, VisibleDefaultsOnly)
	bool bIsEnumNameValid;

	UPROPERTY()
	UEnum* EnumType;

	static uint8 GetValue(const UBlackboardKeyType_NativeEnum* KeyOb, const uint8* RawData);
	static bool SetValue(UBlackboardKeyType_NativeEnum* KeyOb, uint8* RawData, uint8 Value);

	virtual EBlackboardCompare::Type CompareValues(const UBlackboardComponent& OwnerComp, const uint8* MemoryBlock,
		const UBlackboardKeyType* OtherKeyOb, const uint8* OtherMemoryBlock) const override;

	virtual bool IsAllowedByFilter(UBlackboardKeyType* FilterOb) const override;
	virtual FString DescribeSelf() const override;
	virtual FString DescribeArithmeticParam(int32 IntValue, float FloatValue) const override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual FString DescribeValue(const UBlackboardComponent& OwnerComp, const uint8* RawData) const override;
	virtual bool TestArithmeticOperation(const UBlackboardComponent& OwnerComp, const uint8* MemoryBlock, EArithmeticKeyOperation::Type Op, int32 OtherIntValue, float OtherFloatValue) const override;
};
