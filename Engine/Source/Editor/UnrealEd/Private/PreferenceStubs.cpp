// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.


#include "CoreMinimal.h"
#include "Preferences/CascadeOptions.h"
#include "Preferences/CurveEdOptions.h"
#include "Preferences/MaterialEditorOptions.h"
#include "Preferences/PersonaOptions.h"
#include "Preferences/PhysicsAssetEditorOptions.h"

// @todo find a better place for all of this, preferably in the appropriate modules
// though this would require the classes to be relocated as well

//
// UCascadeOptions
// 
UCascadeOptions::UCascadeOptions(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////// UPhysicsAssetEditorOptions /////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
UPhysicsAssetEditorOptions::UPhysicsAssetEditorOptions(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PhysicsBlend = 1.0f;
	bUpdateJointsFromAnimation = false;
	MaxFPS = -1;

	// These should duplicate defaults from UPhysicsHandleComponent
	HandleLinearDamping = 200.0f;
	HandleLinearStiffness = 750.0f;
	HandleAngularDamping = 500.0f;
	HandleAngularStiffness = 1500.0f;
	InterpolationSpeed = 50.f;

	bShowConstraintsAsPoints = false;
	ConstraintDrawSize = 1.0f;

	// view options
	MeshViewMode = EPhysicsAssetEditorRenderMode::Solid;
	CollisionViewMode = EPhysicsAssetEditorRenderMode::Solid;
	ConstraintViewMode = EPhysicsAssetEditorConstraintViewMode::AllLimits;
	SimulationMeshViewMode = EPhysicsAssetEditorRenderMode::Solid;
	SimulationCollisionViewMode = EPhysicsAssetEditorRenderMode::Solid;
	SimulationConstraintViewMode = EPhysicsAssetEditorConstraintViewMode::None;

	CollisionOpacity = 0.3f;
	bSolidRenderingForSelectedOnly = false;
	bResetClothWhenSimulating = false;
}

UMaterialEditorOptions::UMaterialEditorOptions(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


UCurveEdOptions::UCurveEdOptions(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UPersonaOptions::UPersonaOptions(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, DefaultLocalAxesSelection(2)
	, DefaultBoneDrawSelection(1)
	, bAllowPreviewMeshCollectionsToSelectFromDifferentSkeletons(true)
{
	AssetEditorOptions.AddUnique(FAssetEditorOptions(TEXT("SkeletonEditor")));
	AssetEditorOptions.AddUnique(FAssetEditorOptions(TEXT("SkeletalMeshEditor")));
	AssetEditorOptions.AddUnique(FAssetEditorOptions(TEXT("AnimationEditor")));
	AssetEditorOptions.AddUnique(FAssetEditorOptions(TEXT("AnimationBlueprintEditor")));
	AssetEditorOptions.AddUnique(FAssetEditorOptions(TEXT("PhysicsAssetEditor")));

	for(FAssetEditorOptions& EditorOptions : AssetEditorOptions)
	{
		for(FViewportConfigOptions& ViewportConfig : EditorOptions.ViewportConfigs)
		{
			ViewportConfig.ViewModeIndex = VMI_Lit;
			ViewportConfig.ViewFOV = 53.43f;
			ViewportConfig.CameraFollowMode = EAnimationViewportCameraFollowMode::None;
			ViewportConfig.CameraFollowBoneName = NAME_None;
		}
	}

	SectionTimingNodeColor = FLinearColor(0.0f, 1.0f, 0.0f);
	NotifyTimingNodeColor = FLinearColor(1.0f, 0.0f, 0.0f);
	BranchingPointTimingNodeColor = FLinearColor(0.5f, 1.0f, 1.0f);

	bAutoAlignFloorToMesh = true;

	NumFolderFiltersInAssetBrowser = 2;

	bUseAudioAttenuation = true;
}

void UPersonaOptions::SetShowGrid( bool bInShowGrid )
{
	bShowGrid = bInShowGrid;
	SaveConfig();
}

void UPersonaOptions::SetHighlightOrigin( bool bInHighlightOrigin )
{
	bHighlightOrigin = bInHighlightOrigin;
	SaveConfig();
}

void UPersonaOptions::SetViewModeIndex( FName InContext, EViewModeIndex InViewModeIndex, int32 InViewportIndex )
{
	check(InViewportIndex >= 0 && InViewportIndex < 4);

	FAssetEditorOptions& Options = GetAssetEditorOptions(InContext);
	Options.ViewportConfigs[InViewportIndex].ViewModeIndex = InViewModeIndex;
	SaveConfig();
}

void UPersonaOptions::SetAutoAlignFloorToMesh(bool bInAutoAlignFloorToMesh)
{
	bAutoAlignFloorToMesh = bInAutoAlignFloorToMesh;
	SaveConfig();
}

void UPersonaOptions::SetMuteAudio( bool bInMuteAudio )
{
	bMuteAudio = bInMuteAudio;
	SaveConfig();
}

void UPersonaOptions::SetUseAudioAttenuation( bool bInUseAudioAttenuation )
{
	bUseAudioAttenuation = bInUseAudioAttenuation;
	SaveConfig();
}

void UPersonaOptions::SetViewFOV( FName InContext, float InViewFOV, int32 InViewportIndex )
{
	check(InViewportIndex >= 0 && InViewportIndex < 4);

	FAssetEditorOptions& Options = GetAssetEditorOptions(InContext);
	Options.ViewportConfigs[InViewportIndex].ViewFOV = InViewFOV;
	SaveConfig();
}

void UPersonaOptions::SetViewCameraFollow( FName InContext, EAnimationViewportCameraFollowMode InCameraFollowMode, FName InCameraFollowBoneName, int32 InViewportIndex )
{
	check(InViewportIndex >= 0 && InViewportIndex < 4);

	FAssetEditorOptions& Options = GetAssetEditorOptions(InContext);
	Options.ViewportConfigs[InViewportIndex].CameraFollowMode = InCameraFollowMode;
	Options.ViewportConfigs[InViewportIndex].CameraFollowBoneName = InCameraFollowBoneName;
	SaveConfig();
}

void UPersonaOptions::SetDefaultLocalAxesSelection( uint32 InDefaultLocalAxesSelection )
{
	DefaultLocalAxesSelection = InDefaultLocalAxesSelection;
	SaveConfig();
}

void UPersonaOptions::SetDefaultBoneDrawSelection(uint32 InDefaultBoneDrawSelection)
{
	DefaultBoneDrawSelection = InDefaultBoneDrawSelection;
	SaveConfig();
}

void UPersonaOptions::SetShowMeshStats( int32 InShowMeshStats )
{
	ShowMeshStats = InShowMeshStats;
	SaveConfig();
}


void UPersonaOptions::SetSectionTimingNodeColor(const FLinearColor& InColor)
{
	SectionTimingNodeColor = InColor;
	SaveConfig();
}

void UPersonaOptions::SetNotifyTimingNodeColor(const FLinearColor& InColor)
{
	NotifyTimingNodeColor = InColor;
	SaveConfig();
}

void UPersonaOptions::SetBranchingPointTimingNodeColor(const FLinearColor& InColor)
{
	BranchingPointTimingNodeColor = InColor;
	SaveConfig();
}

FAssetEditorOptions& UPersonaOptions::GetAssetEditorOptions(const FName& InContext)
{
	FAssetEditorOptions* FoundOptions = AssetEditorOptions.FindByPredicate([InContext](const FAssetEditorOptions& InOptions)
	{
		return InOptions.Context == InContext;
	});

	if(!FoundOptions)
	{
		AssetEditorOptions.Add(FAssetEditorOptions(InContext));
		return AssetEditorOptions.Last();
	}
	else
	{
		return *FoundOptions;
	}
}