// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintNodeSpawnerUtils.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintVariableNodeSpawner.h"
#include "BlueprintEventNodeSpawner.h"
#include "BlueprintComponentNodeSpawner.h"
#include "BlueprintBoundNodeSpawner.h"
#include "BlueprintDelegateNodeSpawner.h"

//------------------------------------------------------------------------------
UClass* FBlueprintNodeSpawnerUtils::GetAssociatedClass(UBlueprintNodeSpawner const* BlueprintAction)
{
	UClass* ActionClass = BlueprintAction->NodeClass;
	
	if (UField const* AssociatedField = GetAssociatedField(BlueprintAction))
	{
		ActionClass = AssociatedField->GetOwnerClass();
	}
	else if (UBlueprintComponentNodeSpawner const* ComponentSpawner = Cast<UBlueprintComponentNodeSpawner>(BlueprintAction))
	{
		ActionClass = ComponentSpawner->GetComponentClass();
	}
	else if (UBlueprintBoundNodeSpawner const* BoundSpawner = Cast<UBlueprintBoundNodeSpawner>(BlueprintAction))
	{
		ActionClass = GetAssociatedClass(BoundSpawner->GetSubSpawner());
	}
	else if (UBlueprintVariableNodeSpawner const* VarSpawner = Cast<UBlueprintVariableNodeSpawner>(BlueprintAction))
	{
		UObject const* VarOuter = VarSpawner->GetVarOuter();
		if (VarSpawner->IsLocalVariable())
		{
			UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraphChecked(CastChecked<UEdGraph>(VarOuter));
			ActionClass = (Blueprint->SkeletonGeneratedClass != nullptr) ? Blueprint->SkeletonGeneratedClass : Blueprint->ParentClass;
		}
		// else, should have been caught in GetAssociatedField()
	}
	
	return ActionClass;
}

//------------------------------------------------------------------------------
UField const* FBlueprintNodeSpawnerUtils::GetAssociatedField(UBlueprintNodeSpawner const* BlueprintAction)
{
	UField const* ClassField = nullptr;

	if (UFunction const* Function = GetAssociatedFunction(BlueprintAction))
	{
		ClassField = Function;
	}
	else if (UProperty const* Property = GetAssociatedProperty(BlueprintAction))
	{
		ClassField = Property;
	}
	else if (UBlueprintBoundNodeSpawner const* BoundSpawner = Cast<UBlueprintBoundNodeSpawner>(BlueprintAction))
	{
		ClassField = GetAssociatedField(BoundSpawner->GetSubSpawner());
	}
	return ClassField;
}

//------------------------------------------------------------------------------
UFunction const* FBlueprintNodeSpawnerUtils::GetAssociatedFunction(UBlueprintNodeSpawner const* BlueprintAction)
{
	UFunction const* Function = nullptr;

	if (UBlueprintFunctionNodeSpawner const* FuncNodeSpawner = Cast<UBlueprintFunctionNodeSpawner>(BlueprintAction))
	{
		Function = FuncNodeSpawner->GetFunction();
	}
	else if (UBlueprintEventNodeSpawner const* EventSpawner = Cast<UBlueprintEventNodeSpawner>(BlueprintAction))
	{
		Function = EventSpawner->GetEventFunction();
	}
	else if (UBlueprintBoundNodeSpawner const* BoundSpawner = Cast<UBlueprintBoundNodeSpawner>(BlueprintAction))
	{
		Function = GetAssociatedFunction(BoundSpawner->GetSubSpawner());
	}

	return Function;
}

//------------------------------------------------------------------------------
UProperty const* FBlueprintNodeSpawnerUtils::GetAssociatedProperty(UBlueprintNodeSpawner const* BlueprintAction)
{
	UProperty const* Property = nullptr;

	if (UBlueprintDelegateNodeSpawner const* PropertySpawner = Cast<UBlueprintDelegateNodeSpawner>(BlueprintAction))
	{
		Property = PropertySpawner->GetProperty();
	}
	else if (UBlueprintVariableNodeSpawner const* VarSpawner = Cast<UBlueprintVariableNodeSpawner>(BlueprintAction))
	{
		Property = VarSpawner->GetVarProperty();
	}
	return Property;
}
