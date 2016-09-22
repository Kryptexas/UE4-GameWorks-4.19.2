// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

/** Central registry of skeleton trees */
class FSkeletonTreeManager
{
public:
	/** Singleton access */
	static FSkeletonTreeManager& Get();

	/** Create a skeleton tree for the requested skeleton */
	TSharedRef<class ISkeletonTree> CreateSkeletonTree(class USkeleton* InSkeleton, class USkeletalMesh* InSkeletalMesh, const struct FSkeletonTreeArgs& InSkeletonTreeArgs);

	/** Edit a USkeleton via FEditableSkeleton */
	TSharedRef<class FEditableSkeleton> CreateEditableSkeleton(class USkeleton* InSkeleton, class USkeletalMesh* InSkeletalMesh = nullptr);

private:
	/** Hidden constructor */
	FSkeletonTreeManager() {}

private:
	/** Map from skeletons to editable skeletons */
	TMap<class USkeleton*, TWeakPtr<class FEditableSkeleton>> EditableSkeletons;
};
