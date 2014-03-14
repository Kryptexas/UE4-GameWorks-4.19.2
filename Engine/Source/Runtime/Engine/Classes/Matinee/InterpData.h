// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "InterpData.generated.h"

USTRUCT()
struct FAnimSetBakeAndPruneStatus
{
	GENERATED_USTRUCT_BODY()

	/** Name of the anim set referenced in Matinee */
	UPROPERTY(Category=AnimSetBakeAndPruneStatus, VisibleAnywhere, BlueprintReadWrite)
	FString AnimSetName;

	/** If true, the animation is in a GroupAnimSets array, but is unused */
	UPROPERTY(Category=AnimSetBakeAndPruneStatus, VisibleAnywhere, BlueprintReadWrite)
	uint32 bReferencedButUnused:1;

	/** If true, skip BakeAndPrune on this anim set during cooking */
	UPROPERTY(EditAnywhere, Category=AnimSetBakeAndPruneStatus)
	uint32 bSkipBakeAndPrune:1;


	FAnimSetBakeAndPruneStatus()
		: bReferencedButUnused(false)
		, bSkipBakeAndPrune(false)
	{
	}

};

/**
 * Interpolation data, containing keyframe tracks, event tracks etc.
 * This does not contain any  AActor  references or state, so can safely be stored in
 * packages, shared between multiple MatineeActors etc.
 */
UCLASS(hidecategories=Object, MinimalAPI)
class UInterpData : public UObject
{
	GENERATED_UCLASS_BODY()

	/** Duration of interpolation sequence - in seconds. */
	UPROPERTY()
	float InterpLength;

	/** Position in Interp to move things to for path-building in editor. */
	UPROPERTY()
	float PathBuildTime;

	/** Actual interpolation data. Groups of InterpTracks. */
	UPROPERTY(export)
	TArray<class UInterpGroup*> InterpGroups;

	/** Used for curve editor to remember curve-editing setup. Only loaded in editor. */
	UPROPERTY(export)
	class UInterpCurveEdSetup* CurveEdSetup;

#if WITH_EDITORONLY_DATA
	/** Used for filtering which tracks are currently visible. */
	UPROPERTY()
	TArray<class UInterpFilter*> InterpFilters;

	/** The currently selected filter. */
	UPROPERTY()
	class UInterpFilter* SelectedFilter;

	/** Array of default filters. */
	UPROPERTY(transient)
	TArray<class UInterpFilter*> DefaultFilters;

#endif // WITH_EDITORONLY_DATA
	/** Used in editor for defining sections to loop, stretch etc. */
	UPROPERTY()
	float EdSectionStart;

	/** Used in editor for defining sections to loop, stretch etc. */
	UPROPERTY()
	float EdSectionEnd;

	/** If true, then the matinee should be baked and pruned at cook time. */
	UPROPERTY(EditAnywhere, Category=InterpData)
	uint32 bShouldBakeAndPrune:1;

	/** AnimSets referenced by this matinee, and whether to allow bake and prune on them during cooking. */
	UPROPERTY(EditAnywhere, editfixedsize, Category=InterpData)
	TArray<struct FAnimSetBakeAndPruneStatus> BakeAndPruneStatus;

	/** Cached version of the director group, if any, for easy access while in game */
	UPROPERTY(transient)
	class UInterpGroupDirector* CachedDirectorGroup;

	/** Unique names of all events contained across all event tracks */
	UPROPERTY()
	TArray<FName> AllEventNames;


	// Begin UObject interface
	virtual void PostLoad(void) OVERRIDE;
	virtual void PostInitProperties() OVERRIDE;
	// End UObject interface

	/** Search through all InterpGroups in this InterpData to find a group whose GroupName matches the given name. Returns INDEX_NONE if group not found. */
	int32 FindGroupByName( FName GroupName );

	/** Search through all InterpGroups in this InterpData to find a group whose GroupName matches the given name. Returns INDEX_NONE if not group found. */
	int32 FindGroupByName( const FString& InGroupName );

	/** Search through all groups to find all tracks of the given class. */
	ENGINE_API void FindTracksByClass(UClass* TrackClass, TArray<class UInterpTrack*>& OutputTracks);
	
	/** Find a DirectorGroup in the data. Should only ever be 0 or 1 of these! */
	ENGINE_API class UInterpGroupDirector* FindDirectorGroup();

	/** Get the list of all unique event names */
	ENGINE_API void GetAllEventNames(TArray<FName>& OutEventNames);

	/** Update the AllEventNames array */
	ENGINE_API void UpdateEventNames();

#if WITH_EDITOR
	void UpdateBakeAndPruneStatus();
#endif
#if WITH_EDITORONLY_DATA
	void CreateDefaultFilters();
#endif
};



