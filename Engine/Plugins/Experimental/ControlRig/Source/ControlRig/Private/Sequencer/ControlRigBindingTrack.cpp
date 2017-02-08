// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ControlRigBindingTrack.h"
#include "Sections/MovieSceneSpawnSection.h"
#include "Casts.h"
#include "ControlRigBindingTemplate.h"

#define LOCTEXT_NAMESPACE "ControlRigBindingTrack"

TWeakObjectPtr<> UControlRigBindingTrack::ObjectBinding;
TArray<UControlRigBindingTrack::FSequenceBinding> UControlRigBindingTrack::SequenceBindingStack;

FMovieSceneEvalTemplatePtr UControlRigBindingTrack::CreateTemplateForSection(const UMovieSceneSection& InSection) const
{
	const UMovieSceneSpawnSection* Section = CastChecked<const UMovieSceneSpawnSection>(&InSection);
	if (SequenceBindingStack.Num() > 0)
	{
		const FSequenceBinding& Binding = SequenceBindingStack.Top();
		return FControlRigBindingTemplate(*Section, Binding.ObjectID, Binding.SequenceID);
	}
#if WITH_EDITORONLY_DATA
	else if (ObjectBinding.IsValid())
	{
		return FControlRigBindingTemplate(*Section, ObjectBinding);
	}
#endif

	return FControlRigBindingTemplate();
}

#if WITH_EDITORONLY_DATA

FText UControlRigBindingTrack::GetDisplayName() const
{
	return LOCTEXT("TrackName", "Bound");
}

#endif

void UControlRigBindingTrack::SetObjectBinding(TWeakObjectPtr<> InObjectBinding)
{
	ObjectBinding = InObjectBinding;
}

void UControlRigBindingTrack::ClearObjectBinding()
{
	ObjectBinding = nullptr;
}

void UControlRigBindingTrack::PushObjectBindingId(FGuid InObjectBindingId, FMovieSceneSequenceIDRef InObjectBindingSequenceID)
{
	SequenceBindingStack.Push({ InObjectBindingSequenceID, InObjectBindingId });
}

void UControlRigBindingTrack::PopObjectBindingId()
{
	SequenceBindingStack.Pop();
}

#undef LOCTEXT_NAMESPACE
