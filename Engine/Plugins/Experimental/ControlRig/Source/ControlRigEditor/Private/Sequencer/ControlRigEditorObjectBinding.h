// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ISequencerEditorObjectBinding.h"

class ISequencer;
class AActor;

class FControlRigEditorObjectBinding : public ISequencerEditorObjectBinding
{
public:

	FControlRigEditorObjectBinding(TSharedRef<ISequencer> InSequencer);

	static TSharedRef<ISequencerEditorObjectBinding> CreateEditorObjectBinding(TSharedRef<ISequencer> InSequencer);

	// ISequencerEditorObjectBinding interface
	virtual void BuildSequencerAddMenu(FMenuBuilder& MenuBuilder) override;
	virtual bool SupportsSequence(UMovieSceneSequence* InSequence) const override;

private:
	void AddSpawnControlRigMenuExtensions(FMenuBuilder& MenuBuilder);

	void HandleControlRigClassPicked(UClass* InClass);

private:
	TWeakPtr<ISequencer> Sequencer;
};