// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/Blueprint.h"
#include "AnimBlueprint.generated.h"

USTRUCT()
struct FAnimGroupInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FName Name;

	UPROPERTY()
	FLinearColor Color;

	FAnimGroupInfo()
		: Color(FLinearColor::White)
	{
	}
};

/**
 * An Anim Blueprint is essentially a specialized Blueprint whose graphs control the animation of a Skeletal Mesh.
 * It can perform blending of animations, directly control the bones of the skeleton, and output a final pose
 * for a Skeletal Mesh each frame.
 */
UCLASS(dependson=(UBlueprint, UAnimInstance), BlueprintType)
class ENGINE_API UAnimBlueprint : public UBlueprint
{
	GENERATED_UCLASS_BODY()

	/** The kind of skeleton that animation graphs compiled from the blueprint will animate */
	UPROPERTY(AssetRegistrySearchable)
	class USkeleton* TargetSkeleton;

	// List of animation sync groups
	UPROPERTY()
	TArray<FAnimGroupInfo> Groups;

	// @todo document
	class UAnimBlueprintGeneratedClass* GetAnimBlueprintGeneratedClass() const;

	// @todo document
	class UAnimBlueprintGeneratedClass* GetAnimBlueprintSkeletonClass() const;

#if WITH_EDITOR
	// UBlueprint interface
	virtual bool SupportedByDefaultBlueprintFactory() const OVERRIDE
	{
		return false;
	}
	// End of UBlueprint interface

	// Finds the index of the specified group, or creates a new entry for it (unless the name is NAME_None, which will return INDEX_NONE)
	int32 FindOrAddGroup(FName GroupName);

	/** Returns the most base anim blueprint for a given blueprint (if it is inherited from another anim blueprint, returning null if only native / non-anim BP classes are it's parent) */
	static UAnimBlueprint* FindRootAnimBlueprint(UAnimBlueprint* DerivedBlueprint);
#endif
};
