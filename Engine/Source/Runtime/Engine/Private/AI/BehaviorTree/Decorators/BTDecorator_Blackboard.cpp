// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBTDecorator_Blackboard::UBTDecorator_Blackboard(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = "Blackboard";
}

bool UBTDecorator_Blackboard::CalculateRawConditionValue(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* BlackboardComp = OwnerComp->GetBlackboardComponent();
	return EvaluateOnBlackboard(BlackboardComp);
}

bool UBTDecorator_Blackboard::EvaluateOnBlackboard(const UBlackboardComponent* BlackboardComp) const
{
	if (BlackboardComp)
	{
		//
		// part 1: text key operations: string, name
		//
		if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_String::StaticClass() ||
			BlackboardKey.SelectedKeyType == UBlackboardKeyType_Name::StaticClass())
		{
			const FString BBStringValue = (BlackboardKey.SelectedKeyType == UBlackboardKeyType_String::StaticClass()) ?
				BlackboardComp->GetValueAsString(BlackboardKey.SelectedKeyID) :
				BlackboardComp->GetValueAsName(BlackboardKey.SelectedKeyID).ToString();

			switch (OperationType)
			{
			case ETextKeyOperation::Equal:			return (BBStringValue == StringValue);
			case ETextKeyOperation::NotEqual:		return (BBStringValue != StringValue);
			case ETextKeyOperation::Contain:		return (BBStringValue.Contains(StringValue) == true);
			case ETextKeyOperation::NotContain:		return (BBStringValue.Contains(StringValue) == false);
			default: break;
			}
		}
		//
		// part 2: arithmetic key operations: enum, int, float
		//
		else if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Int::StaticClass())
		{
			const int32 BBIntValue = BlackboardComp->GetValueAsInt(BlackboardKey.SelectedKeyID);
			switch (OperationType)
			{
			case EArithmeticKeyOperation::Equal:			return (BBIntValue == IntValue);
			case EArithmeticKeyOperation::NotEqual:			return (BBIntValue != IntValue);
			case EArithmeticKeyOperation::Less:				return (BBIntValue < IntValue);
			case EArithmeticKeyOperation::LessOrEqual:		return (BBIntValue <= IntValue);
			case EArithmeticKeyOperation::Greater:			return (BBIntValue > IntValue);
			case EArithmeticKeyOperation::GreaterOrEqual:	return (BBIntValue >= IntValue);
			default: break;
			}
		}
		else if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Enum::StaticClass())
		{
			const uint8 EnumValue = BlackboardComp->GetValueAsEnum(BlackboardKey.SelectedKeyID);
			switch (OperationType)
			{
			case EArithmeticKeyOperation::Equal:			return (EnumValue == IntValue);
			case EArithmeticKeyOperation::NotEqual:			return (EnumValue != IntValue);
			case EArithmeticKeyOperation::Less:				return (EnumValue < IntValue);
			case EArithmeticKeyOperation::LessOrEqual:		return (EnumValue <= IntValue);
			case EArithmeticKeyOperation::Greater:			return (EnumValue > IntValue);
			case EArithmeticKeyOperation::GreaterOrEqual:	return (EnumValue >= IntValue);
			default: break;
			}
		}
		else if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_NativeEnum::StaticClass())
		{
			uint8 EnumValue = BlackboardComp->GetValueAsEnum(BlackboardKey.SelectedKeyID);
			switch (OperationType)
			{
			case EArithmeticKeyOperation::Equal:			return (EnumValue == IntValue);
			case EArithmeticKeyOperation::NotEqual:			return (EnumValue != IntValue);
			case EArithmeticKeyOperation::Less:				return (EnumValue < IntValue);
			case EArithmeticKeyOperation::LessOrEqual:		return (EnumValue <= IntValue);
			case EArithmeticKeyOperation::Greater:			return (EnumValue > IntValue);
			case EArithmeticKeyOperation::GreaterOrEqual:	return (EnumValue >= IntValue);
			default: break;
			}
		}
		else if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Float::StaticClass())
		{
			const float BBFloatValue = BlackboardComp->GetValueAsFloat(BlackboardKey.SelectedKeyID);
			switch (OperationType)
			{
			case EArithmeticKeyOperation::Equal:			return (BBFloatValue == FloatValue);
			case EArithmeticKeyOperation::NotEqual:			return (BBFloatValue != FloatValue);
			case EArithmeticKeyOperation::Less:				return (BBFloatValue < FloatValue);
			case EArithmeticKeyOperation::LessOrEqual:		return (BBFloatValue <= FloatValue);
			case EArithmeticKeyOperation::Greater:			return (BBFloatValue > FloatValue);
			case EArithmeticKeyOperation::GreaterOrEqual:	return (BBFloatValue >= FloatValue);
			default: break;
			}
		}
		//
		// part 3: basic key operations: object, class, bool, vector
		// 
		else if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Object::StaticClass())
		{
			UObject* ObjectValue = BlackboardComp->GetValueAsObject(BlackboardKey.SelectedKeyID);
			switch (OperationType)
			{
			case EBasicKeyOperation::Set:		return (ObjectValue != NULL);
			case EBasicKeyOperation::NotSet:	return (ObjectValue == NULL);
			default: break;
			}
		}
		else if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Class::StaticClass())
		{
			UClass* ClassValue = BlackboardComp->GetValueAsClass(BlackboardKey.SelectedKeyID);
			switch (OperationType)
			{
			case EBasicKeyOperation::Set:		return (ClassValue != NULL);
			case EBasicKeyOperation::NotSet:	return (ClassValue == NULL);
			default: break;
			}
		}
		else if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Bool::StaticClass())
		{
			bool BoolValue = BlackboardComp->GetValueAsBool(BlackboardKey.SelectedKeyID);
			switch (OperationType)
			{
			case EBasicKeyOperation::Set:		return (BoolValue == true);
			case EBasicKeyOperation::NotSet:	return (BoolValue == false);
			default: break;
			}
		}
		else if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
		{
			FVector VectorValue = BlackboardComp->GetValueAsVector(BlackboardKey.SelectedKeyID);
			switch (OperationType)
			{
			case EBasicKeyOperation::Set:		return (VectorValue.IsNearlyZero() == false);
			case EBasicKeyOperation::NotSet:	return (VectorValue.IsNearlyZero() == true);
			default: break;
			}
		}
	}

	return false;
}

void UBTDecorator_Blackboard::OnBlackboardChange(const class UBlackboardComponent* Blackboard, uint8 ChangedKeyID)
{
	UBehaviorTreeComponent* BehaviorComp = Blackboard ? (UBehaviorTreeComponent*)Blackboard->GetBehaviorComponent() : NULL;
	if (BlackboardKey.SelectedKeyID == ChangedKeyID && BehaviorComp)
	{
		if (NotifyObserver != EBTBlackboardRestart::ValueChange)
		{
			const bool bIsExecutingBranch = BehaviorComp->IsExecutingBranch(this, GetChildIndex());
			const bool bPass = EvaluateOnBlackboard(Blackboard);

			UE_VLOG(BehaviorComp->GetOwner(), LogBehaviorTree, Verbose, TEXT("%s, OnBlackboardChange[%s] pass:%d executing:%d => %s"),
				*UBehaviorTreeTypes::DescribeNodeHelper(this),
				*Blackboard->GetKeyName(ChangedKeyID).ToString(), bPass, bIsExecutingBranch,
				(bIsExecutingBranch && !bPass) || (!bIsExecutingBranch && bPass) ? TEXT("restart") : TEXT("skip"));

			if ((bIsExecutingBranch && !bPass) || 
				(!bIsExecutingBranch && bPass))
			{
				BehaviorComp->RequestExecution(this);
			}
		}
		else
		{
			UE_VLOG(BehaviorComp->GetOwner(), LogBehaviorTree, Verbose, TEXT("%s, OnBlackboardChange[%s] => restart"), 
				*UBehaviorTreeTypes::DescribeNodeHelper(this),
				*Blackboard->GetKeyName(ChangedKeyID).ToString());

			BehaviorComp->RequestExecution(this);
		}
	}
}

void UBTDecorator_Blackboard::DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	Super::DescribeRuntimeValues(OwnerComp, NodeMemory, Verbosity, Values);

	const UBlackboardComponent* BlackboardComp = OwnerComp->GetBlackboardComponent();
	FString DescKeyValue;

	if (BlackboardComp)
	{
		DescKeyValue = BlackboardComp->DescribeKeyValue(BlackboardKey.SelectedKeyID, EBlackboardDescription::OnlyValue);
	}

	const bool bResult = EvaluateOnBlackboard(BlackboardComp);
	Values.Add(FString::Printf(TEXT("value: %s (%s)"), *DescKeyValue, bResult ? TEXT("pass") : TEXT("fail")));
}

FString UBTDecorator_Blackboard::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: %s"), *Super::GetStaticDescription(), *CachedDescription);
}

#if WITH_EDITOR
static UEnum* BasicOpEnum = NULL;
static UEnum* ArithmeticOpEnum = NULL;
static UEnum* TextOpEnum = NULL;

static void CacheOperationEnums()
{
	if (BasicOpEnum == NULL)
	{
		BasicOpEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EBasicKeyOperation"));
		check(BasicOpEnum);
	}

	if (ArithmeticOpEnum == NULL)
	{
		ArithmeticOpEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EArithmeticKeyOperation"));
		check(ArithmeticOpEnum);
	}

	if (TextOpEnum == NULL)
	{
		TextOpEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("ETextKeyOperation"));
		check(TextOpEnum);
	}
}

void UBTDecorator_Blackboard::BuildDescription()
{
	UBlackboardData* BlackboardAsset = GetBlackboardAsset();
	const FBlackboardEntry* EntryInfo = BlackboardAsset ? BlackboardAsset->GetKey(BlackboardKey.SelectedKeyID) : NULL;

	FString BlackboardDesc = "invalid";
	if (EntryInfo)
	{
		// safety feature to not crash when changing couple of properties on a bunch
		// while "post edit property" triggers for every each of them
		if (EntryInfo->KeyType->GetClass() == BlackboardKey.SelectedKeyType)
		{
			const FString KeyName = EntryInfo->EntryName.ToString();
			CacheOperationEnums();
		

			//
			// part 1: text operations: string, name
			//
			if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_String::StaticClass() ||
				BlackboardKey.SelectedKeyType == UBlackboardKeyType_Name::StaticClass())
			{
				BlackboardDesc = FString::Printf(TEXT("%s %s [%s]"), *KeyName, *TextOpEnum->GetEnumName(OperationType), *StringValue);
			}
			//
			// part 2: arithmetic key operations: enum, int, float
			//
			else if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Int::StaticClass())
			{
				BlackboardDesc = FString::Printf(TEXT("%s is %s %s %d"), *KeyName, *ArithmeticOpEnum->GetEnumName(OperationType),
					(OperationType == EArithmeticKeyOperation::Equal || OperationType == EArithmeticKeyOperation::NotEqual) ? TEXT("to") : TEXT("than"),
					IntValue);
			}
			else if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Enum::StaticClass())
			{
				BlackboardDesc = FString::Printf(TEXT("%s is %s %s %s"), *KeyName, *ArithmeticOpEnum->GetEnumName(OperationType),
					(OperationType == EArithmeticKeyOperation::Equal || OperationType == EArithmeticKeyOperation::NotEqual) ? TEXT("to") : TEXT("than"),
					((UBlackboardKeyType_Enum*)EntryInfo->KeyType)->EnumType ?
						*(((UBlackboardKeyType_Enum*)EntryInfo->KeyType)->EnumType->GetEnumName(IntValue)) : TEXT("UNKNOWN")
					);
			}
			else if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_NativeEnum::StaticClass())
			{
				BlackboardDesc = FString::Printf(TEXT("%s is %s %s %s"), *KeyName, *ArithmeticOpEnum->GetEnumName(OperationType),
					(OperationType == EArithmeticKeyOperation::Equal || OperationType == EArithmeticKeyOperation::NotEqual) ? TEXT("to") : TEXT("than"),
					((UBlackboardKeyType_NativeEnum*)EntryInfo->KeyType)->EnumType ?
						*(((UBlackboardKeyType_NativeEnum*)EntryInfo->KeyType)->EnumType->GetEnumName(IntValue)) : TEXT("UNKNOWN")
					);
			}
			else if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Float::StaticClass())
			{
				BlackboardDesc = FString::Printf(TEXT("%s is %s %s %f"), *KeyName, *ArithmeticOpEnum->GetEnumName(OperationType),
					(OperationType == EArithmeticKeyOperation::Equal || OperationType == EArithmeticKeyOperation::NotEqual) ? TEXT("to") : TEXT("than"),
					FloatValue);
			}
			//
			// part 3: basic key operations: object, class, bool, vector
			// 
			else
			{
				BlackboardDesc = FString::Printf(TEXT("%s is %s"), *KeyName, *BasicOpEnum->GetEnumName(OperationType));
			}
		}
	}

	CachedDescription = BlackboardDesc;
}

void UBTDecorator_Blackboard::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.Property == NULL)
	{
		return;
	}

	const FName ChangedPropName = PropertyChangedEvent.Property->GetFName();
	if (ChangedPropName == GET_MEMBER_NAME_CHECKED(UBTDecorator_Blackboard, BlackboardKey.SelectedKeyName))
	{
		if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Enum::StaticClass() ||
			BlackboardKey.SelectedKeyType == UBlackboardKeyType_NativeEnum::StaticClass())
		{
			IntValue = 0;
		}
	}

#if WITH_EDITORONLY_DATA

	if (ChangedPropName == GET_MEMBER_NAME_CHECKED(UBTDecorator_Blackboard, BasicOperation))
	{
		if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Object::StaticClass() ||
			BlackboardKey.SelectedKeyType == UBlackboardKeyType_Class::StaticClass() ||
			BlackboardKey.SelectedKeyType == UBlackboardKeyType_Bool::StaticClass() ||
			BlackboardKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
		{
			OperationType = BasicOperation;
		}
	}
	else if (ChangedPropName == GET_MEMBER_NAME_CHECKED(UBTDecorator_Blackboard, ArithmeticOperation))
	{
		if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Enum::StaticClass() ||
			BlackboardKey.SelectedKeyType == UBlackboardKeyType_NativeEnum::StaticClass() ||
			BlackboardKey.SelectedKeyType == UBlackboardKeyType_Int::StaticClass() ||
			BlackboardKey.SelectedKeyType == UBlackboardKeyType_Float::StaticClass())
		{
			OperationType = ArithmeticOperation;
		}
	}
	else if (ChangedPropName == GET_MEMBER_NAME_CHECKED(UBTDecorator_Blackboard, TextOperation))
	{
		if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_String::StaticClass() ||
			BlackboardKey.SelectedKeyType == UBlackboardKeyType_Name::StaticClass())
		{
			OperationType = TextOperation;
		}
	}

#endif // WITH_EDITORONLY_DATA

	BuildDescription();
}

void UBTDecorator_Blackboard::InitializeFromAsset(class UBehaviorTree* Asset)
{
	Super::InitializeFromAsset(Asset);
	BuildDescription();

	// numeric types and booleans will always trigger on result change (performance reasons)
	if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Enum::StaticClass() ||
		BlackboardKey.SelectedKeyType == UBlackboardKeyType_NativeEnum::StaticClass() ||
		BlackboardKey.SelectedKeyType == UBlackboardKeyType_Int::StaticClass() ||
		BlackboardKey.SelectedKeyType == UBlackboardKeyType_Float::StaticClass() ||
		BlackboardKey.SelectedKeyType == UBlackboardKeyType_Bool::StaticClass())
	{
		NotifyObserver = EBTBlackboardRestart::ResultChange;
	}
}

#endif	// WITH_EDITOR
