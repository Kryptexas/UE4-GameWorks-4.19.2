// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BTDecorator_Blackboard.generated.h"

/**
 *  Decorator for accessing blackboard
 *
 *  Allowed types and their operations:
 *  - object:	set, not set
 *  - class:	set, not set
 *  - enum:		equal, not equal, less, less or equal, greater, greater or equal
 *  - int:		equal, not equal, less, less or equal, greater, greater or equal
 *  - float:	equal, not equal, less, less or equal, greater, greater or equal
 *  - bool:		set, not set
 *  - string:	equal, not equal, contain, not contain
 *  - name:		equal, not equal, contain, not contain
 *  - vector:	set, not set
 */

UENUM()
namespace EBasicKeyOperation
{
	enum Type
	{
		Set				UMETA(DisplayName="Is Set"),
		NotSet			UMETA(DisplayName="Is Not Set"),
	};
}

UENUM()
namespace EArithmeticKeyOperation
{
	enum Type
	{
		Equal			UMETA(DisplayName="Is Equal To"),
		NotEqual		UMETA(DisplayName="Is Not Equal To"),
		Less			UMETA(DisplayName="Is Less Than"),
		LessOrEqual		UMETA(DisplayName="Is Less Or Equal To"),
		Greater			UMETA(DisplayName="Is Greater Than"),
		GreaterOrEqual	UMETA(DisplayName="Is Greater Or Equal To"),
	};
}

UENUM()
namespace ETextKeyOperation
{
	enum Type
	{
		Equal			UMETA(DisplayName="Is Equal To"),
		NotEqual		UMETA(DisplayName="Is Not Equal To"),
		Contain			UMETA(DisplayName="Contains"),
		NotContain		UMETA(DisplayName="Not Contains"),
	};
}

UENUM()
namespace EBTBlackboardRestart
{
	enum Type
	{
		ValueChange		UMETA(DisplayName="On Value Change" , ToolTip="Restart on every change of observed blackboard value"),
		ResultChange	UMETA(DisplayName="On Result Change" , ToolTip="Restart only when result of evaluated condition is changed"),
	};
}

UCLASS(HideCategories=(Condition))
class ENGINE_API UBTDecorator_Blackboard : public UBTDecorator_BlackboardBase
{
	GENERATED_UCLASS_BODY()

	virtual bool CalculateRawConditionValue(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;
	virtual void OnBlackboardChange(const class UBlackboardComponent* Blackboard, uint8 ChangedKeyID) OVERRIDE;
	virtual void DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const OVERRIDE;
	virtual FString GetStaticDescription() const OVERRIDE;

protected:

	/** value for arithmetic operations */
	UPROPERTY(Category=Blackboard, EditAnywhere, meta=(DisplayName="Key Value"))
	int32 IntValue;

	/** value for arithmetic operations */
	UPROPERTY(Category=Blackboard, EditAnywhere, meta=(DisplayName="Key Value"))
	float FloatValue;

	/** value for string operations */
	UPROPERTY(Category=Blackboard, EditAnywhere, meta=(DisplayName="Key Value"))
	FString StringValue;

	/** cached description */
	UPROPERTY()
	FString CachedDescription;

	/** operation type */
	UPROPERTY()
	uint8 OperationType;

	/** when observer can try to request abort? */
	UPROPERTY(Category=FlowControl, EditAnywhere)
	TEnumAsByte<EBTBlackboardRestart::Type> NotifyObserver;

#if WITH_EDITORONLY_DATA

	UPROPERTY(Category=Blackboard, EditAnywhere, meta=(DisplayName="Key Query"))
	TEnumAsByte<EBasicKeyOperation::Type> BasicOperation;

	UPROPERTY(Category=Blackboard, EditAnywhere, meta=(DisplayName="Key Query"))
	TEnumAsByte<EArithmeticKeyOperation::Type> ArithmeticOperation;

	UPROPERTY(Category=Blackboard, EditAnywhere, meta=(DisplayName="Key Query"))
	TEnumAsByte<ETextKeyOperation::Type> TextOperation;

#endif

#if WITH_EDITOR

	/** describe decorator and cache it */
	virtual void BuildDescription();

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	virtual void InitializeFromAsset(class UBehaviorTree* Asset) OVERRIDE;

#endif

	/** take blackboard value and evaluate decorator's condition */
	bool EvaluateOnBlackboard(const class UBlackboardComponent* BlackboardComp) const;

#if CPP
	friend class FBlackboardDecoratorDetails;
#endif
};
