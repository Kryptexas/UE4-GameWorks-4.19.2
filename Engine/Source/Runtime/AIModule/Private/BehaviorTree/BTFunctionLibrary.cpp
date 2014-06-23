// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "BehaviorTree/BTFunctionLibrary.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyAllTypes.h"
#include "BehaviorTree/BTFunctionLibrary.h"

UBTFunctionLibrary::UBTFunctionLibrary(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
}

UBlackboardComponent* UBTFunctionLibrary::GetBlackboard(UBTNode* NodeOwner)
{
	UBehaviorTreeComponent* BTComp = NodeOwner ? Cast<UBehaviorTreeComponent>(NodeOwner->GetOuter()) : NULL;
	return BTComp ? BTComp->GetBlackboardComponent() : NULL;
}

UObject* UBTFunctionLibrary::GetBlackboardValueAsObject(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key)
{
	const UBlackboardComponent* BlackboardComp = GetBlackboard(NodeOwner);
	return BlackboardComp ? BlackboardComp->GetValueAsObject(Key.SelectedKeyName) : NULL;
}

AActor* UBTFunctionLibrary::GetBlackboardValueAsActor(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key)
{
	const UBlackboardComponent* BlackboardComp = GetBlackboard(NodeOwner);
	return BlackboardComp ? Cast<AActor>(BlackboardComp->GetValueAsObject(Key.SelectedKeyName)) : NULL;
}

UClass* UBTFunctionLibrary::GetBlackboardValueAsClass(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key)
{
	const UBlackboardComponent* BlackboardComp = GetBlackboard(NodeOwner);
	return BlackboardComp ? BlackboardComp->GetValueAsClass(Key.SelectedKeyName) : NULL;
}

uint8 UBTFunctionLibrary::GetBlackboardValueAsEnum(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key)
{
	const UBlackboardComponent* BlackboardComp = GetBlackboard(NodeOwner);
	return BlackboardComp ? BlackboardComp->GetValueAsEnum(Key.SelectedKeyName) : 0;
}

int32 UBTFunctionLibrary::GetBlackboardValueAsInt(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key)
{
	const UBlackboardComponent* BlackboardComp = GetBlackboard(NodeOwner);
	return BlackboardComp ? BlackboardComp->GetValueAsInt(Key.SelectedKeyName) : 0;
}

float UBTFunctionLibrary::GetBlackboardValueAsFloat(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key)
{
	const UBlackboardComponent* BlackboardComp = GetBlackboard(NodeOwner);
	return BlackboardComp ? BlackboardComp->GetValueAsFloat(Key.SelectedKeyName) : 0.0f;
}

bool UBTFunctionLibrary::GetBlackboardValueAsBool(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key)
{
	const UBlackboardComponent* BlackboardComp = GetBlackboard(NodeOwner);
	return BlackboardComp ? BlackboardComp->GetValueAsBool(Key.SelectedKeyName) : false;
}

FString UBTFunctionLibrary::GetBlackboardValueAsString(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key)
{
	const UBlackboardComponent* BlackboardComp = GetBlackboard(NodeOwner);
	return BlackboardComp ? BlackboardComp->GetValueAsString(Key.SelectedKeyName) : FString();
}

FName UBTFunctionLibrary::GetBlackboardValueAsName(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key)
{
	const UBlackboardComponent* BlackboardComp = GetBlackboard(NodeOwner);
	return BlackboardComp ? BlackboardComp->GetValueAsName(Key.SelectedKeyName) : NAME_None;
}

FVector UBTFunctionLibrary::GetBlackboardValueAsVector(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key)
{
	const UBlackboardComponent* BlackboardComp = GetBlackboard(NodeOwner);
	return BlackboardComp ? BlackboardComp->GetValueAsVector(Key.SelectedKeyName) : FVector::ZeroVector;
}

void UBTFunctionLibrary::SetBlackboardValueAsObject(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key, UObject* Value)
{
	UBlackboardComponent* BlackboardComp = GetBlackboard(NodeOwner);
	if (BlackboardComp != NULL)
	{
		BlackboardComp->SetValueAsObject(Key.SelectedKeyName, Value);
	}
}

void UBTFunctionLibrary::SetBlackboardValueAsClass(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key, UClass* Value)
{
	UBlackboardComponent* BlackboardComp = GetBlackboard(NodeOwner);
	if (BlackboardComp != NULL)
	{
		BlackboardComp->SetValueAsClass(Key.SelectedKeyName, Value);
	}
}

void UBTFunctionLibrary::SetBlackboardValueAsEnum(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key, uint8 Value)
{
	UBlackboardComponent* BlackboardComp = GetBlackboard(NodeOwner);
	if (BlackboardComp != NULL)
	{
		BlackboardComp->SetValueAsEnum(Key.SelectedKeyName, Value);
	}
}

void UBTFunctionLibrary::SetBlackboardValueAsInt(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key, int32 Value)
{
	UBlackboardComponent* BlackboardComp = GetBlackboard(NodeOwner);
	if (BlackboardComp != NULL)
	{
		BlackboardComp->SetValueAsInt(Key.SelectedKeyName, Value);
	}
}

void UBTFunctionLibrary::SetBlackboardValueAsFloat(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key, float Value)
{
	UBlackboardComponent* BlackboardComp = GetBlackboard(NodeOwner);
	if (BlackboardComp != NULL)
	{
		BlackboardComp->SetValueAsFloat(Key.SelectedKeyName, Value);
	}
}

void UBTFunctionLibrary::SetBlackboardValueAsBool(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key, bool Value)
{
	UBlackboardComponent* BlackboardComp = GetBlackboard(NodeOwner);
	if (BlackboardComp != NULL)
	{
		BlackboardComp->SetValueAsBool(Key.SelectedKeyName, Value);
	}
}

void UBTFunctionLibrary::SetBlackboardValueAsString(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key, FString Value)
{
	UBlackboardComponent* BlackboardComp = GetBlackboard(NodeOwner);
	if (BlackboardComp != NULL)
	{
		BlackboardComp->SetValueAsString(Key.SelectedKeyName, Value);
	}
}

void UBTFunctionLibrary::SetBlackboardValueAsName(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key, FName Value)
{
	UBlackboardComponent* BlackboardComp = GetBlackboard(NodeOwner);
	if (BlackboardComp != NULL)
	{
		BlackboardComp->SetValueAsName(Key.SelectedKeyName, Value);
	}
}

void UBTFunctionLibrary::SetBlackboardValueAsVector(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key, FVector Value)
{
	UBlackboardComponent* BlackboardComp = GetBlackboard(NodeOwner);
	if (BlackboardComp != NULL)
	{
		BlackboardComp->SetValueAsVector(Key.SelectedKeyName, Value);
	}
}

void UBTFunctionLibrary::ClearBlackboardValueAsVector(UBTNode* NodeOwner, const struct FBlackboardKeySelector& Key)
{
	UBlackboardComponent* BlackboardComp = GetBlackboard(NodeOwner);
	if (BlackboardComp != NULL)
	{
		BlackboardComp->ClearValueAsVector(Key.SelectedKeyName);
	}
}

void UBTFunctionLibrary::StartUsingExternalEvent(UBTNode* NodeOwner, AActor* OwningActor)
{
	// deprecated, not removed yet
}

void UBTFunctionLibrary::StopUsingExternalEvent(UBTNode* NodeOwner)
{
	// deprecated, not removed yet
}
