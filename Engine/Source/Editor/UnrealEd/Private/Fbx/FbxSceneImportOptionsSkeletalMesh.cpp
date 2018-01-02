// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Factories/FbxSceneImportOptionsSkeletalMesh.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "Factories/FbxSceneImportOptions.h"
#include "Factories/FbxSceneImportOptionsStaticMesh.h"


UFbxSceneImportOptionsSkeletalMesh::UFbxSceneImportOptionsSkeletalMesh(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bUpdateSkeletonReferencePose(false)
	, bCreatePhysicsAsset(false)
	, bUseT0AsRefPose(false)
	, bPreserveSmoothingGroups(false)
	, bImportMeshesInBoneHierarchy(true)
	, bImportMorphTargets(false)
	, ThresholdPosition(THRESH_POINTS_ARE_SAME)
	, ThresholdTangentNormal(THRESH_NORMALS_ARE_SAME)
	, ThresholdUV(THRESH_UVS_ARE_SAME)
	, bImportAnimations(true)
	, AnimationLength(EFbxSceneVertexColorImportOption::Replace)
	, FrameImportRange(0, 0)
	, bUseDefaultSampleRate(false)
	, bImportCustomAttribute(true)
	, bPreserveLocalTransform(false)
	, bDeleteExistingMorphTargetCurves(false)
{
}

void UFbxSceneImportOptionsSkeletalMesh::FillSkeletalMeshInmportData(UFbxSkeletalMeshImportData* SkeletalMeshImportData, UFbxAnimSequenceImportData* AnimSequenceImportData, UFbxSceneImportOptions* SceneImportOptions)
{
	check(SkeletalMeshImportData != nullptr);
	SkeletalMeshImportData->bImportMeshesInBoneHierarchy = bImportMeshesInBoneHierarchy;
	SkeletalMeshImportData->bImportMorphTargets = bImportMorphTargets;
	SkeletalMeshImportData->ThresholdPosition = ThresholdPosition;
	SkeletalMeshImportData->ThresholdTangentNormal = ThresholdTangentNormal;
	SkeletalMeshImportData->ThresholdUV = ThresholdUV;
	SkeletalMeshImportData->bPreserveSmoothingGroups = bPreserveSmoothingGroups;
	SkeletalMeshImportData->bUpdateSkeletonReferencePose = bUpdateSkeletonReferencePose;
	SkeletalMeshImportData->bUseT0AsRefPose = bUseT0AsRefPose;

	SkeletalMeshImportData->bImportMeshLODs = SceneImportOptions->bImportSkeletalMeshLODs;
	SkeletalMeshImportData->ImportTranslation = SceneImportOptions->ImportTranslation;
	SkeletalMeshImportData->ImportRotation = SceneImportOptions->ImportRotation;
	SkeletalMeshImportData->ImportUniformScale = SceneImportOptions->ImportUniformScale;
	SkeletalMeshImportData->bTransformVertexToAbsolute = SceneImportOptions->bTransformVertexToAbsolute;
	SkeletalMeshImportData->bBakePivotInVertex = SceneImportOptions->bBakePivotInVertex;
	
	SkeletalMeshImportData->bImportAsScene = true;

	AnimSequenceImportData->bImportMeshesInBoneHierarchy = bImportMeshesInBoneHierarchy;
	AnimSequenceImportData->AnimationLength = AnimationLength;
	AnimSequenceImportData->bDeleteExistingMorphTargetCurves = bDeleteExistingMorphTargetCurves;
	AnimSequenceImportData->bImportCustomAttribute = bImportCustomAttribute;
	AnimSequenceImportData->bPreserveLocalTransform = bPreserveLocalTransform;
	AnimSequenceImportData->bUseDefaultSampleRate = bUseDefaultSampleRate;
	AnimSequenceImportData->FrameImportRange = FrameImportRange;

	AnimSequenceImportData->bImportAsScene = true;
}
