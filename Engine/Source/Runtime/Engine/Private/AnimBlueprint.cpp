// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "BlueprintUtilities.h"
#include "LatentActions.h"
#include "EngineLevelScriptClasses.h"

//////////////////////////////////////////////////////////////////////////
// UAnimBlueprint

UAnimBlueprint::UAnimBlueprint(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

UAnimBlueprintGeneratedClass* UAnimBlueprint::GetAnimBlueprintGeneratedClass() const
{
	UAnimBlueprintGeneratedClass* Result = Cast<UAnimBlueprintGeneratedClass>(*GeneratedClass);
	return Result;
}

UAnimBlueprintGeneratedClass* UAnimBlueprint::GetAnimBlueprintSkeletonClass() const
{
	UAnimBlueprintGeneratedClass* Result = Cast<UAnimBlueprintGeneratedClass>(*SkeletonGeneratedClass);
	return Result;
}

#if WITH_EDITOR

int32 UAnimBlueprint::FindOrAddGroup(FName GroupName)
{
	if (GroupName == NAME_None)
	{
		return INDEX_NONE;
	}
	else
	{
		// Look for an existing group
		for (int32 Index = 0; Index < Groups.Num(); ++Index)
		{
			if (Groups[Index].Name == GroupName)
			{
				return Index;
			}
		}

		// Create a new group
		MarkPackageDirty();
		FAnimGroupInfo& NewGroup = *(new (Groups) FAnimGroupInfo());
		NewGroup.Name = GroupName;

		return Groups.Num() - 1;
	}
}


/** Returns the most base anim blueprint for a given blueprint (if it is inherited from another anim blueprint, returning null if only native / non-anim BP classes are it's parent) */
UAnimBlueprint* UAnimBlueprint::FindRootAnimBlueprint(UAnimBlueprint* DerivedBlueprint)
{
	UAnimBlueprint* ParentBP = NULL;

	// Determine if there is an anim blueprint in the ancestry of this class
	for (UClass* ParentClass = DerivedBlueprint->ParentClass; ParentClass != UObject::StaticClass(); ParentClass = ParentClass->GetSuperClass())
	{
		if (UAnimBlueprint* TestBP = Cast<UAnimBlueprint>(ParentClass->ClassGeneratedBy))
		{
			ParentBP = TestBP;
		}
	}

	return ParentBP;
}

#endif