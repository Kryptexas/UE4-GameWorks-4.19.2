// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleInterface.h"
#include "ControlRigBlueprintCompiler.h"

class UBlueprint;
class IAssetTypeActions;
class UControlRigSequence;
class ISequencer;

class FControlRigEditorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** Handle a new animation controller blueprint being created */
	void HandleNewBlueprintCreated(UBlueprint* InBlueprint);

	/** Handle a new sequencer instance being created */
	void HandleSequencerCreated(TSharedRef<ISequencer> InSequencer);

	/** Called to setup a new sequence's defaults */
	static void OnInitializeSequence(UControlRigSequence* Sequence);

	/** Compiler customization for animation controllers */
	FControlRigBlueprintCompiler ControlRigBlueprintCompiler;

	/** Handle for our sequencer track editor */
	FDelegateHandle ControlRigTrackCreateEditorHandle;

	/** Handle for our sequencer binding track editor */
	FDelegateHandle ControlRigBindingTrackCreateEditorHandle;

	/** Handle for our sequencer object binding */
	FDelegateHandle ControlRigEditorObjectBindingHandle;

	/** Handle for our level sequence spawner */
	FDelegateHandle LevelSequenceSpawnerDelegateHandle;

	/** Handle for tracking ISequencer creation */
	FDelegateHandle SequencerCreatedHandle;

	TArray<TSharedRef<IAssetTypeActions>> RegisteredAssetTypeActions;
};