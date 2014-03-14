// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

namespace BlueprintNodeHelpers
{
	FString CollectPropertyDescription(const UObject* Ob, const UClass* StopAtClass, const TArray<UProperty*>& PropertyData);
	void CollectPropertyData(const UObject* Ob, const UClass* StopAtClass, TArray<UProperty*>& PropertyData);
	uint16 GetPropertiesMemorySize(const TArray<UProperty*>& PropertyData);

	void CollectBlackboardSelectors(const UObject* Ob, const UClass* StopAtClass, TArray<FName>& KeyNames);

	FString DescribeProperty(const UObject* Ob, const UProperty* Prop);
	void DescribeRuntimeValues(const UObject* Ob, const UClass* StopAtClass,
		const TArray<UProperty*>& PropertyData, uint8* ContextMemory,
		EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& RuntimeValues);

	void CopyPropertiesToContext(const TArray<UProperty*>& PropertyData, uint8* ObjectMemory, uint8* ContextMemory);
	void CopyPropertiesFromContext(const TArray<UProperty*>& PropertyData, uint8* ObjectMemory, uint8* ContextMemory);

	bool FindNodeOwner(AActor* OwningActor, class UBTNode* Node, UBehaviorTreeComponent*& OwningComp, int32& OwningInstanceIdx);

	FORCEINLINE bool HasBlueprintFunction(FName FuncName, const UObject* Ob, const UClass* StopAtClass)
	{
		return (Ob->GetClass()->FindFunctionByName(FuncName)->GetOuter() != StopAtClass);
	}

	FORCEINLINE FString GetNodeName(const UObject* Ob)
	{
		return Ob->GetClass()->GetName().LeftChop(2);
	}
}
