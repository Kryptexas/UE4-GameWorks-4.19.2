// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "IAssetTypeActions.h"

// This data is shared by the various controls (graph view, components view, and defaults view) 
// that are presented as part of the merge tool. Each view keeps its own shallow copy of the data:
struct FBlueprintMergeData
{
	FBlueprintMergeData()
		: OwningEditor()
		, BlueprintLocal(NULL)
		, BlueprintBase(NULL)
		, RevisionBase()
		, BlueprintRemote(NULL)
		, RevisionRemote()
	{
	}

	FBlueprintMergeData( 
		TWeakPtr<class FBlueprintEditor>	InOwningEditor
		, const class UBlueprint*			InBlueprintLocal
		, const class UBlueprint*			InBlueprintBase
		, FRevisionInfo						InRevisionBase
		, const class UBlueprint*			InBlueprintRemote
		, FRevisionInfo						InRevisionRemote
	)
		: OwningEditor(		InOwningEditor		)
		, BlueprintLocal(	InBlueprintLocal	)
		, BlueprintBase(	InBlueprintBase		)
		, RevisionBase(		InRevisionBase		)
		, BlueprintRemote(	InBlueprintRemote	)
		, RevisionRemote(		InRevisionRemote)
	{
	}

	TWeakPtr<class FBlueprintEditor>	OwningEditor;
	
	const class UBlueprint*				BlueprintLocal;
	const class UBlueprint*				BlueprintBase;
	FRevisionInfo						RevisionBase;

	const class UBlueprint*				BlueprintRemote;
	FRevisionInfo						RevisionRemote;
};

namespace EMergeParticipant
{
	enum Type
	{
		Remote,
		Base,
		Local,

		Max_None,
	};
}

class IMergeControl
{
public:
	virtual void HighlightNextConflict() = 0;
	virtual void HighlightPrevConflict() = 0;
	virtual bool HasNextConflict() const = 0;
	virtual bool HasPrevConflict() const = 0;
};
