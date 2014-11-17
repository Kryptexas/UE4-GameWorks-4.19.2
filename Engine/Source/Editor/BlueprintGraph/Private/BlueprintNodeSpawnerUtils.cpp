// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintNodeSpawnerUtils.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintVariableNodeSpawner.h"
#include "BlueprintEventNodeSpawner.h"
#include "BlueprintComponentNodeSpawner.h"
#include "BlueprintBoundEventNodeSpawner.h"
#include "BlueprintDelegateNodeSpawner.h"

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
	else if (UBlueprintBoundEventNodeSpawner const* BoundSpawner = Cast<UBlueprintBoundEventNodeSpawner>(BlueprintAction))
	{
		Property = BoundSpawner->GetEventDelegate();
	}
	return Property;
}
