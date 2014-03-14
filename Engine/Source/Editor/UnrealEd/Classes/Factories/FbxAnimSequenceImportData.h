// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "FbxAnimSequenceImportData.generated.h"

/** 
	* I know these descriptions don't make sense, but the functions I use act a bit different depending on situation. 
	a) FbxAnimStack::GetLocalTimeSpan will return you the start and stop time for this stack (or take). It's simply two time values that can be set to anything regardless of animation curves. Generally speaking, applications set these to the start and stop time of the timeline.
	b) As for FbxNode::GetAnimationInternval, this one will iterate through all properties recursively, and then for all animation curves it finds, for the animation layer index specified. So in other words, if one property has been animated, it will modify this result. This is completely different from GetLocalTimeSpan since it calculates the time span depending on the keys rather than just using the start and stop time that was saved in the file.
*/

/** animation length type when importing*/
UENUM()
enum EFBXAnimationLengthImportType
{
	FBXALIT_ExportedTime			UMETA(DisplayName="Exported Time"),
	FBXALIT_AnimatedKey				UMETA(DisplayName="Animated Time"),
	FBXALIT_SetRange				UMETA(DisplayName="Set Range"),

	FBXALIT_MAX,
};

/**
 * Import data and options used when importing any mesh from FBX
 */
UCLASS()
class UFbxAnimSequenceImportData : public UFbxAssetImportData
{
	GENERATED_UCLASS_BODY()

	/** Type of asset to import from the FBX file */
	UPROPERTY(EditAnywhere, Category=ImportSettings, meta=(DisplayName = "Animation Length"))
	TEnumAsByte<enum EFBXAnimationLengthImportType> AnimationLength;

	/** Type of asset to import from the FBX file */
	UPROPERTY(EditAnywhere, Category=ImportSettings, meta=(DisplayName = "Start Frame"))
	int32 StartFrame;
	/** Type of asset to import from the FBX file */
	UPROPERTY(EditAnywhere, Category=ImportSettings, meta=(DisplayName = "End Frame"))
	int32	EndFrame;

	/** Gets or creates fbx import data for the specified anim sequence */
	static UFbxAnimSequenceImportData* GetImportDataForAnimSequence(UAnimSequence* AnimSequence, UFbxAnimSequenceImportData* TemplateForCreation);
};