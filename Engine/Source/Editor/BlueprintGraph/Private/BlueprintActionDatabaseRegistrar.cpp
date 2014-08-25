// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintNodeSpawnerUtils.h"

/*******************************************************************************
 * BlueprintActionDatabaseRegistrarImpl
 ******************************************************************************/

namespace BlueprintActionDatabaseRegistrarImpl
{
	static UObject const* ResolveClassKey(UClass const* ClassKey);
	static UObject const* ResolveActionKey(UObject const* UserPassedKey);
}

//------------------------------------------------------------------------------
static UObject const* BlueprintActionDatabaseRegistrarImpl::ResolveClassKey(UClass const* ClassKey)
{
	UObject const* ResolvedKey = ClassKey;
	if (UBlueprintGeneratedClass const* BlueprintClass = Cast<UBlueprintGeneratedClass>(ClassKey))
	{
		ResolvedKey = CastChecked<UBlueprint>(BlueprintClass->ClassGeneratedBy);
	}
	return ResolvedKey;
}

//------------------------------------------------------------------------------
static UObject const* BlueprintActionDatabaseRegistrarImpl::ResolveActionKey(UObject const* UserPassedKey)
{
	UObject const* ResolvedKey = nullptr;
	if (UClass const* Class = Cast<UClass>(UserPassedKey))
	{
		ResolvedKey = ResolveClassKey(Class);
	}
	// both handled in the IsAsset() case
// 	else if (UUserDefinedEnum const* EnumAsset = Cast<UUserDefinedEnum>(UserPassedKey))
// 	{
// 		ResolvedKey = EnumAsset;
// 	}
// 	else if (UUserDefinedStruct const* StructAsset = Cast<UUserDefinedStruct>(StructOwner))
// 	{
// 		ResolvedKey = StructAsset;
// 	}
	else if (UserPassedKey->IsAsset())
	{
		ResolvedKey = UserPassedKey;
	}
	else if (UField const* MemberField = Cast<UField>(UserPassedKey))
	{
		ResolvedKey = ResolveClassKey(MemberField->GetOwnerClass());
	}

	return ResolvedKey;
}

/*******************************************************************************
 * FBlueprintActionDatabaseRegistrar
 ******************************************************************************/

//------------------------------------------------------------------------------
FBlueprintActionDatabaseRegistrar::FBlueprintActionDatabaseRegistrar(FActionRegistry& Database, FPrimingQueue& PrimingQueue, TSubclassOf<UEdGraphNode> DefaultKey)
	: GeneratingClass(DefaultKey)
	, ActionDatabase(Database)
	, ActionKeyFilter(nullptr)
	, ActionPrimingQueue(PrimingQueue)
{
}

//------------------------------------------------------------------------------
void FBlueprintActionDatabaseRegistrar::AddBlueprintAction(UBlueprintNodeSpawner* NodeSpawner)
{
	UField const* ActionKey = GeneratingClass;
	// if this spawner wraps some member function/property, then we want it 
	// recorded under that class (so we can update the action if the class 
	// changes... like, if the member is deleted, or if one is added)
	if (UField const* MemberField = FBlueprintNodeSpawnerUtils::GetAssociatedField(NodeSpawner))
	{
		ActionKey = MemberField;
	}
	AddActionToDatabase(ActionKey, NodeSpawner);
}

//------------------------------------------------------------------------------
void FBlueprintActionDatabaseRegistrar::AddBlueprintAction(UClass const* ClassOwner, UBlueprintNodeSpawner* NodeSpawner)
{
	// ResolveActionKey() is used on ClassOwner (in AddActionToDatabase), to 
	// convert it into a proper key
	AddActionToDatabase(ClassOwner, NodeSpawner);
}

//------------------------------------------------------------------------------
void FBlueprintActionDatabaseRegistrar::AddBlueprintAction(UEnum const* EnumOwner, UBlueprintNodeSpawner* NodeSpawner)
{
	// ResolveActionKey() is used on EnumOwner (in AddActionToDatabase), to 
	// convert it into a proper key
	AddActionToDatabase(EnumOwner, NodeSpawner);
}

//------------------------------------------------------------------------------
void FBlueprintActionDatabaseRegistrar::AddBlueprintAction(UScriptStruct const* StructOwner, UBlueprintNodeSpawner* NodeSpawner)
{
	// ResolveActionKey() is used on StructOwner (in AddActionToDatabase), to 
	// convert it into a proper key
	AddActionToDatabase(StructOwner, NodeSpawner);
}

//------------------------------------------------------------------------------
void FBlueprintActionDatabaseRegistrar::AddBlueprintAction(UField const* FieldOwner, UBlueprintNodeSpawner* NodeSpawner)
{
	// ResolveActionKey() is used on FieldOwner (in AddActionToDatabase), to 
	// convert it into a proper key
	AddActionToDatabase(FieldOwner, NodeSpawner);
}

//------------------------------------------------------------------------------
void FBlueprintActionDatabaseRegistrar::AddBlueprintAction(UObject const* AssetOwner, UBlueprintNodeSpawner* NodeSpawner)
{
	// cannot record an action under any ol' object (we want to associate them 
	// with asset/class outers that are subject to change; so that we can 
	// refresh/rebuild corresponding actions when that happens).
	check(AssetOwner->IsAsset());
	AddActionToDatabase(AssetOwner, NodeSpawner);
}

//------------------------------------------------------------------------------
bool FBlueprintActionDatabaseRegistrar::IsOpenForRegistration(UObject const* OwnerKey)
{
	UObject const* ActionKey = BlueprintActionDatabaseRegistrarImpl::ResolveActionKey(OwnerKey);
	if (ActionKey == nullptr)
	{
		ActionKey = GeneratingClass;
	}
	return (ActionKey != nullptr) && ((ActionKeyFilter == nullptr) || (ActionKeyFilter == ActionKey));
}

//------------------------------------------------------------------------------
void FBlueprintActionDatabaseRegistrar::AddActionToDatabase(UObject const* ActionKey, UBlueprintNodeSpawner* NodeSpawner)
{
	ensureMsg(NodeSpawner->NodeClass == GeneratingClass, TEXT("We expect a nodes to add only spawners for its own type... Maybe a sub-class is adding nodes it shouldn't?"));
	if (IsOpenForRegistration(ActionKey))
	{
		ActionKey = BlueprintActionDatabaseRegistrarImpl::ResolveActionKey(ActionKey);
		if (ActionKey == nullptr)
		{
			ActionKey = GeneratingClass;
		}
		FBlueprintActionDatabase::FActionList& ActionList = ActionDatabase.FindOrAdd(ActionKey);
		
		int32* QueuedIndex = ActionPrimingQueue.Find(ActionKey);
		if (QueuedIndex == nullptr)
		{
			int32 PrimingIndex = ActionList.Num();
			ActionPrimingQueue.Add(ActionKey, PrimingIndex);
		}

		ActionList.Add(NodeSpawner);
	}
}