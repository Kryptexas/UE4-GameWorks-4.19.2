// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEmitterTrackEditor.h"
#include "NiagaraEmitterSection.h"
#include "NiagaraEmitter.h"
#include "ViewModels/NiagaraSystemViewModel.h"
#include "Sequencer/NiagaraSequence/NiagaraSequence.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#define LOCTEXT_NAMESPACE "NiagaraEmitterTrackEditor"

FNiagaraEmitterTrackEditor::FNiagaraEmitterTrackEditor(TSharedPtr<ISequencer> Sequencer) 
	: FMovieSceneTrackEditor(Sequencer.ToSharedRef())
{
}

TSharedRef<ISequencerTrackEditor> FNiagaraEmitterTrackEditor::CreateTrackEditor(TSharedRef<ISequencer> InSequencer)
{
	return MakeShareable(new FNiagaraEmitterTrackEditor(InSequencer));
}

bool FNiagaraEmitterTrackEditor::SupportsType(TSubclassOf<UMovieSceneTrack> TrackClass) const 
{
	if (TrackClass == UMovieSceneNiagaraEmitterTrack::StaticClass())
	{
		return true;
	}
	return false;
}

TSharedRef<ISequencerSection> FNiagaraEmitterTrackEditor::MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding)
{
	return MakeShareable(new FNiagaraEmitterSection(SectionObject));
}

bool FNiagaraEmitterTrackEditor::HandleAssetAdded(UObject* Asset, const FGuid& TargetObjectGuid)
{
	UNiagaraEmitter* EmitterAsset = Cast<UNiagaraEmitter>(Asset);
	UNiagaraSequence* NiagaraSequence = Cast<UNiagaraSequence>(GetSequencer()->GetRootMovieSceneSequence());
	if (EmitterAsset != nullptr && NiagaraSequence != nullptr && NiagaraSequence->GetSystemViewModel().GetCanModifyEmittersFromTimeline())
	{
		NiagaraSequence->GetSystemViewModel().AddEmitter(*EmitterAsset);
	}
	return false;
}

void FNiagaraEmitterTrackEditor::BuildTrackContextMenu( FMenuBuilder& MenuBuilder, UMovieSceneTrack* Track )
{
	UNiagaraSequence* NiagaraSequencer = Cast<UNiagaraSequence>(GetSequencer()->GetRootMovieSceneSequence());
	if (NiagaraSequencer != nullptr)
	{
		FNiagaraSystemViewModel* SystemViewModel = &NiagaraSequencer->GetSystemViewModel();
		if (SystemViewModel->GetEditMode() == ENiagaraSystemViewModelEditMode::SystemAsset)
		{
			MenuBuilder.BeginSection("Niagara", LOCTEXT("NiagaraContextMenuSectionName", "Niagara"));
			{
				MenuBuilder.AddMenuEntry(
					LOCTEXT("IsolateSelectedEmitters", "Isolate Selected"),
					LOCTEXT("IsolateSelectedEmittersTooltip", "Force selected emitters to only be isolated."),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateSP(SystemViewModel, &FNiagaraSystemViewModel::IsolateSelectedEmitters),
						FCanExecuteAction::CreateSP(SystemViewModel, &FNiagaraSystemViewModel::CanExecuteToggleIsolation)
					)
				);
				MenuBuilder.AddMenuEntry(
					LOCTEXT("IsolateEmitter", "Isolated"),
					LOCTEXT("IsolateEmitterTooltip", "Toggle the isolation state of the selected emitters."),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateSP(SystemViewModel, &FNiagaraSystemViewModel::IsolateEmitterToggle),
						FCanExecuteAction::CreateSP(SystemViewModel, &FNiagaraSystemViewModel::CanExecuteToggleIsolation),
						FIsActionChecked::CreateSP(SystemViewModel, &FNiagaraSystemViewModel::IsEmitterIsolationActive)
					),
					NAME_None,
					EUserInterfaceActionType::ToggleButton
				);
			}
			MenuBuilder.EndSection();
		}
	}
}

#undef LOCTEXT_NAMESPACE