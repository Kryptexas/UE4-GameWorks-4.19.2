// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"


UFbxSceneImportOptionsSkeletalMesh::UFbxSceneImportOptionsSkeletalMesh(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bUpdateSkeletonReferencePose(false)
	, bCreatePhysicsAsset(false)
	, bUseT0AsRefPose(false)
	, bPreserveSmoothingGroups(false)
	, bImportMeshesInBoneHierarchy(true)
	, bImportMorphTargets(false)
	, bKeepOverlappingVertices(false)
{
}

