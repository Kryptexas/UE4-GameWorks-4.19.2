// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBTFunctionLibrary::UBTFunctionLibrary(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
}

UBlackboardComponent* UBTFunctionLibrary::GetBlackboard(UBTNode* NodeOwner)
{
	UBehaviorTreeComponent* BTComp = NodeOwner ? NodeOwner->GetCurrentCallOwner() : NULL;
	return BTComp ? BTComp->GetBlackboardComponent() : NULL;
}

UObject* UBTFunctionLibrary::GetBlackboardValueAsObject(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key)
{
	const UBehaviorTreeComponent* BTComp = NodeOwner ? NodeOwner->GetCurrentCallOwner() : NULL;
	return BTComp ? BTComp->GetBlackboardComponent()->GetValueAsObject(Key.SelectedKeyName) : NULL;
}

AActor* UBTFunctionLibrary::GetBlackboardValueAsActor(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key)
{
	const UBehaviorTreeComponent* BTComp = NodeOwner ? NodeOwner->GetCurrentCallOwner() : NULL;
	return BTComp ? Cast<AActor>(BTComp->GetBlackboardComponent()->GetValueAsObject(Key.SelectedKeyName)) : NULL;
}

UClass* UBTFunctionLibrary::GetBlackboardValueAsClass(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key)
{
	const UBehaviorTreeComponent* BTComp = NodeOwner ? NodeOwner->GetCurrentCallOwner() : NULL;
	return BTComp ? BTComp->GetBlackboardComponent()->GetValueAsClass(Key.SelectedKeyName) : NULL;
}

uint8 UBTFunctionLibrary::GetBlackboardValueAsEnum(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key)
{
	const UBehaviorTreeComponent* BTComp = NodeOwner ? NodeOwner->GetCurrentCallOwner() : NULL;
	return BTComp ? BTComp->GetBlackboardComponent()->GetValueAsEnum(Key.SelectedKeyName) : 0;
}

int32 UBTFunctionLibrary::GetBlackboardValueAsInt(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key)
{
	const UBehaviorTreeComponent* BTComp = NodeOwner ? NodeOwner->GetCurrentCallOwner() : NULL;
	return BTComp ? BTComp->GetBlackboardComponent()->GetValueAsInt(Key.SelectedKeyName) : 0;
}

float UBTFunctionLibrary::GetBlackboardValueAsFloat(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key)
{
	const UBehaviorTreeComponent* BTComp = NodeOwner ? NodeOwner->GetCurrentCallOwner() : NULL;
	return BTComp ? BTComp->GetBlackboardComponent()->GetValueAsFloat(Key.SelectedKeyName) : 0.0f;
}

bool UBTFunctionLibrary::GetBlackboardValueAsBool(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key)
{
	const UBehaviorTreeComponent* BTComp = NodeOwner ? NodeOwner->GetCurrentCallOwner() : NULL;
	return BTComp ? BTComp->GetBlackboardComponent()->GetValueAsBool(Key.SelectedKeyName) : false;
}

FString UBTFunctionLibrary::GetBlackboardValueAsString(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key)
{
	const UBehaviorTreeComponent* BTComp = NodeOwner ? NodeOwner->GetCurrentCallOwner() : NULL;
	return BTComp ? BTComp->GetBlackboardComponent()->GetValueAsString(Key.SelectedKeyName) : FString();
}

FName UBTFunctionLibrary::GetBlackboardValueAsName(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key)
{
	const UBehaviorTreeComponent* BTComp = NodeOwner ? NodeOwner->GetCurrentCallOwner() : NULL;
	return BTComp ? BTComp->GetBlackboardComponent()->GetValueAsName(Key.SelectedKeyName) : NAME_None;
}

FVector UBTFunctionLibrary::GetBlackboardValueAsVector(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key)
{
	const UBehaviorTreeComponent* BTComp = NodeOwner ? NodeOwner->GetCurrentCallOwner() : NULL;
	return BTComp ? BTComp->GetBlackboardComponent()->GetValueAsVector(Key.SelectedKeyName) : FVector::ZeroVector;
}

void UBTFunctionLibrary::SetBlackboardValueAsObject(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key, UObject* Value)
{
	UBehaviorTreeComponent* BTComp = NodeOwner ? NodeOwner->GetCurrentCallOwner() : NULL;
	if (BTComp != NULL)
	{
		BTComp->GetBlackboardComponent()->SetValueAsObject(Key.SelectedKeyName, Value);
	}
}

void UBTFunctionLibrary::SetBlackboardValueAsClass(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key, UClass* Value)
{
	UBehaviorTreeComponent* BTComp = NodeOwner ? NodeOwner->GetCurrentCallOwner() : NULL;
	if (BTComp != NULL)
	{
		BTComp->GetBlackboardComponent()->SetValueAsClass(Key.SelectedKeyName, Value);
	}
}

void UBTFunctionLibrary::SetBlackboardValueAsEnum(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key, uint8 Value)
{
	UBehaviorTreeComponent* BTComp = NodeOwner ? NodeOwner->GetCurrentCallOwner() : NULL;
	if (BTComp != NULL)
	{
		BTComp->GetBlackboardComponent()->SetValueAsEnum(Key.SelectedKeyName, Value);
	}
}

void UBTFunctionLibrary::SetBlackboardValueAsInt(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key, int32 Value)
{
	UBehaviorTreeComponent* BTComp = NodeOwner ? NodeOwner->GetCurrentCallOwner() : NULL;
	if (BTComp != NULL)
	{
		BTComp->GetBlackboardComponent()->SetValueAsInt(Key.SelectedKeyName, Value);
	}
}

void UBTFunctionLibrary::SetBlackboardValueAsFloat(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key, float Value)
{
	UBehaviorTreeComponent* BTComp = NodeOwner ? NodeOwner->GetCurrentCallOwner() : NULL;
	if (BTComp != NULL)
	{
		BTComp->GetBlackboardComponent()->SetValueAsFloat(Key.SelectedKeyName, Value);
	}
}

void UBTFunctionLibrary::SetBlackboardValueAsBool(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key, bool Value)
{
	UBehaviorTreeComponent* BTComp = NodeOwner ? NodeOwner->GetCurrentCallOwner() : NULL;
	if (BTComp != NULL)
	{
		BTComp->GetBlackboardComponent()->SetValueAsBool(Key.SelectedKeyName, Value);
	}
}

void UBTFunctionLibrary::SetBlackboardValueAsString(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key, FString Value)
{
	UBehaviorTreeComponent* BTComp = NodeOwner ? NodeOwner->GetCurrentCallOwner() : NULL;
	if (BTComp != NULL)
	{
		BTComp->GetBlackboardComponent()->SetValueAsString(Key.SelectedKeyName, Value);
	}
}

void UBTFunctionLibrary::SetBlackboardValueAsName(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key, FName Value)
{
	UBehaviorTreeComponent* BTComp = NodeOwner ? NodeOwner->GetCurrentCallOwner() : NULL;
	if (BTComp != NULL)
	{
		BTComp->GetBlackboardComponent()->SetValueAsName(Key.SelectedKeyName, Value);
	}
}

void UBTFunctionLibrary::SetBlackboardValueAsVector(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key, FVector Value)
{
	UBehaviorTreeComponent* BTComp = NodeOwner ? NodeOwner->GetCurrentCallOwner() : NULL;
	if (BTComp != NULL)
	{
		BTComp->GetBlackboardComponent()->SetValueAsVector(Key.SelectedKeyName, Value);
	}
}

void UBTFunctionLibrary::StartUsingExternalEvent(UBTNode* NodeOwner, AActor* OwningActor)
{
	if (NodeOwner)
	{
		NodeOwner->StartUsingExternalEvent(OwningActor);
	}
}

void UBTFunctionLibrary::StopUsingExternalEvent(UBTNode* NodeOwner)
{
	if (NodeOwner)
	{
		NodeOwner->StopUsingExternalEvent();
	}
}
