// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// PersonaOptions
//
// A configuration class that holds information for the setup of the Persona.
// Supplied so that the editor 'remembers' the last setup the user had.
//=============================================================================

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/EngineBaseTypes.h"
#include "PersonaOptions.generated.h"

/** Persisted camera follow mode */
UENUM()
enum class EAnimationViewportCameraFollowMode : uint8
{
	/** Standard camera controls */
	None,

	/** Follow the bounds of the mesh */
	Bounds,

	/** Follow a bone or socket */
	Bone,
};

/** Persistent per-viewport options */
USTRUCT()
struct FViewportConfigOptions
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, config, Category = "Viewport")
	TEnumAsByte<EViewModeIndex> ViewModeIndex;

	UPROPERTY(EditAnywhere, config, Category = "Viewport")
	float ViewFOV;

	/** Persisted camera follow mode for a viewport */
	UPROPERTY(config)
	EAnimationViewportCameraFollowMode CameraFollowMode;

	UPROPERTY(config)
	FName CameraFollowBoneName;
};

/** Options that should be unique per asset editor (like skeletal mesh or anim sequence editors) */
USTRUCT()
struct FAssetEditorOptions
{
	GENERATED_BODY()

	FAssetEditorOptions()
	{}

	FAssetEditorOptions(const FName& InContext)
		: Context(InContext)
	{
	}

	/** the name of the asset editor properties apply to */
	UPROPERTY(config)
	FName Context;

	/** Per-viewport configuration */
	UPROPERTY(EditAnywhere, config, Category = "Viewport")
	FViewportConfigOptions ViewportConfigs[4];

	bool operator==(const FAssetEditorOptions& InOptions) const
	{
		return InOptions.Context == Context;
	}
};

UCLASS(hidecategories=Object, config=EditorPerProjectUserSettings)
class UNREALED_API UPersonaOptions : public UObject
{
	GENERATED_UCLASS_BODY()
		
	/** Whether or not the floor should be aligned to the Skeletal Mesh's bounds by default for the Animation Editor(s)*/
	UPROPERTY(EditAnywhere, config, Category = "Preview Scene")
	uint32 bAutoAlignFloorToMesh : 1;

	/** Whether or not the grid should be visible by default for the Animation Editor(s)*/
	UPROPERTY(EditAnywhere, config, Category = "Viewport")
	uint32 bShowGrid:1;

	/** Whether or not the XYZ axis at the origin should be highlighted on the grid by default */
	UPROPERTY(EditAnywhere, config, Category = "Viewport")
	uint32 bHighlightOrigin:1;

	/** Whether or not audio should be muted by default for the Animation Editor(s)*/
	UPROPERTY(EditAnywhere, config, Category = "Audio")
	uint32 bMuteAudio:1;

	UPROPERTY(EditAnywhere, config, Category = "Audio")
	uint32 bUseAudioAttenuation:1;

	/** Currently Stats can have None, Basic and Detailed. Please refer to EDisplayInfoMode. */
	UPROPERTY(EditAnywhere, config, Category = "Viewport", meta=(ClampMin ="0", ClampMax = "3", UIMin = "0", UIMax = "3"))
	int32 ShowMeshStats;
	
	/** Index used to determine which ViewMode should be used by default for the Animation Editor(s)*/
	UPROPERTY(EditAnywhere, config, Category = "Viewport")
	uint32 DefaultLocalAxesSelection;

	/** Index used to determine which Bone Draw Mode should be used by default for the Animation Editor(s)*/
	UPROPERTY(EditAnywhere, config, Category = "Viewport")
	uint32 DefaultBoneDrawSelection;

	UPROPERTY(EditAnywhere, config, Category = "Composites and Montages")
	FLinearColor SectionTimingNodeColor;

	UPROPERTY(EditAnywhere, config, Category = "Composites and Montages")
	FLinearColor NotifyTimingNodeColor;

	UPROPERTY(EditAnywhere, config, Category = "Composites and Montages")
	FLinearColor BranchingPointTimingNodeColor;

	/** Whether to use a socket editor that is created in-line inside the skeleton tree, or whether to use the separate details panel */
	UPROPERTY(EditAnywhere, config, Category = "Skeleton Tree")
	bool bUseInlineSocketEditor;

	/** Whether to keep the hierarchy or flatten it when searching for bones, sockets etc. */
	UPROPERTY(EditAnywhere, config, Category = "Skeleton Tree")
	bool bFlattenSkeletonHierarchyWhenFiltering;

	/** Whether to hide parent items when filtering or to display them grayed out */
	UPROPERTY(EditAnywhere, config, Category = "Skeleton Tree")
	bool bHideParentsWhenFiltering;

	UPROPERTY(EditAnywhere, config, Category = "Preview Scene")
	bool bAllowPreviewMeshCollectionsToSelectFromDifferentSkeletons;

	/** Whether or not Skeletal Mesh Section selection should be enabled by default for the Animation Editor(s)*/
	UPROPERTY(EditAnywhere, config, Category = "Mesh")
	bool bAllowMeshSectionSelection;

	/** The number of folder filters to allow at any one time in the animation tool's asset browser */
	UPROPERTY(EditAnywhere, config, Category = "Asset Browser", meta=(ClampMin ="1", ClampMax = "10", UIMin = "1", UIMax = "10"))
	uint32 NumFolderFiltersInAssetBrowser;

	/** Options that should be unique per asset editor (like skeletal mesh or anim sequence editors) */
	UPROPERTY(config)
	TArray<FAssetEditorOptions> AssetEditorOptions;

public:
	void SetShowGrid( bool bInShowGrid );
	void SetHighlightOrigin( bool bInHighlightOrigin );
	void SetAutoAlignFloorToMesh(bool bInAutoAlignFloorToMesh);
	void SetMuteAudio( bool bInMuteAudio );
	void SetUseAudioAttenuation( bool bInUseAudioAttenuation );
	void SetViewModeIndex( FName InContext, EViewModeIndex InViewModeIndex, int32 InViewportIndex );
	void SetViewFOV( FName InContext, float InViewFOV, int32 InViewportIndex );
	void SetViewCameraFollow( FName InContext, EAnimationViewportCameraFollowMode InCameraFollowMode, FName InCameraFollowBoneName, int32 InViewportIndex );
	void SetDefaultLocalAxesSelection( uint32 InDefaultLocalAxesSelection );
	void SetDefaultBoneDrawSelection(uint32 InDefaultBoneAxesSelection);
	void SetShowMeshStats( int32 InShowMeshStats );
	void SetSectionTimingNodeColor(const FLinearColor& InColor);
	void SetNotifyTimingNodeColor(const FLinearColor& InColor);
	void SetBranchingPointTimingNodeColor(const FLinearColor& InColor);
	FAssetEditorOptions& GetAssetEditorOptions(const FName& InContext);
};
