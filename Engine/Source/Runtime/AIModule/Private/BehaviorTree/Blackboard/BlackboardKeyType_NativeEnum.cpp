// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_NativeEnum.h"

const UBlackboardKeyType_NativeEnum::FDataType UBlackboardKeyType_NativeEnum::InvalidValue = UBlackboardKeyType_NativeEnum::FDataType(0);

UBlackboardKeyType_NativeEnum::UBlackboardKeyType_NativeEnum(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ValueSize = sizeof(uint8);
	SupportedOp = EBlackboardKeyOperation::Arithmetic;
}

uint8 UBlackboardKeyType_NativeEnum::GetValue(const UBlackboardKeyType_NativeEnum* KeyOb, const uint8* RawData)
{
	return GetValueFromMemory<uint8>(RawData);
}

bool UBlackboardKeyType_NativeEnum::SetValue(UBlackboardKeyType_NativeEnum* KeyOb, uint8* RawData, uint8 Value)
{
	return SetValueInMemory<uint8>(RawData, Value);
}

EBlackboardCompare::Type UBlackboardKeyType_NativeEnum::CompareValues(const UBlackboardComponent& OwnerComp, const uint8* MemoryBlock,
	const UBlackboardKeyType* OtherKeyOb, const uint8* OtherMemoryBlock) const
{
	const uint8 MyValue = GetValue(this, MemoryBlock);
	const uint8 OtherValue = GetValue((UBlackboardKeyType_NativeEnum*)OtherKeyOb, OtherMemoryBlock);

	return (MyValue > OtherValue) ? EBlackboardCompare::Greater :
		(MyValue < OtherValue) ? EBlackboardCompare::Less :
		EBlackboardCompare::Equal;
}

FString UBlackboardKeyType_NativeEnum::DescribeValue(const UBlackboardComponent& OwnerComp, const uint8* RawData) const
{
	return EnumType ? EnumType->GetEnumName(GetValue(this, RawData)) : FString("UNKNOWN!");
}

FString UBlackboardKeyType_NativeEnum::DescribeSelf() const
{
	return *GetNameSafe(EnumType);
}

bool UBlackboardKeyType_NativeEnum::IsAllowedByFilter(UBlackboardKeyType* FilterOb) const
{
	UBlackboardKeyType_NativeEnum* FilterEnum = Cast<UBlackboardKeyType_NativeEnum>(FilterOb);
	return (FilterEnum && FilterEnum->EnumName == EnumName);
}

#if WITH_EDITOR
void UBlackboardKeyType_NativeEnum::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property &&
		PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UBlackboardKeyType_NativeEnum, EnumName))
	{
		EnumType = FindObject<UEnum>(ANY_PACKAGE, *EnumName, true);
		bIsEnumNameValid =  (EnumType != NULL);
	}
}
#endif

bool UBlackboardKeyType_NativeEnum::TestArithmeticOperation(const UBlackboardComponent& OwnerComp, const uint8* MemoryBlock, EArithmeticKeyOperation::Type Op, int32 OtherIntValue, float OtherFloatValue) const
{
	const uint8 Value = GetValue(this, MemoryBlock);
	switch (Op)
	{
	case EArithmeticKeyOperation::Equal:			return (Value == OtherIntValue);
	case EArithmeticKeyOperation::NotEqual:			return (Value != OtherIntValue);
	case EArithmeticKeyOperation::Less:				return (Value < OtherIntValue);
	case EArithmeticKeyOperation::LessOrEqual:		return (Value <= OtherIntValue);
	case EArithmeticKeyOperation::Greater:			return (Value > OtherIntValue);
	case EArithmeticKeyOperation::GreaterOrEqual:	return (Value >= OtherIntValue);
	default: break;
	}

	return false;
}

FString UBlackboardKeyType_NativeEnum::DescribeArithmeticParam(int32 IntValue, float FloatValue) const
{
	return EnumType ? EnumType->GetEnumName(IntValue) : FString("UNKNOWN!");
}
