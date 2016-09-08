// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneToolsPrivatePCH.h"
#include "PropertySection.h"


FPropertySection::FPropertySection(UMovieSceneSection& InSectionObject, const FText& InDisplayName)
	: DisplayName(InDisplayName)
	, SectionObject(InSectionObject)
	, Sequencer(nullptr)
{ }

FPropertySection::FPropertySection(ISequencer* InSequencer, FGuid InObjectBinding, FName InPropertyName, 
	const FString& InPropertyPath, UMovieSceneSection& InSectionObject, const FText& InDisplayName)
	: DisplayName(InDisplayName)
	, SectionObject(InSectionObject)
	, Sequencer(InSequencer)
	, ObjectBinding(InObjectBinding)
	, PropertyBindings(MakeShareable(new FTrackInstancePropertyBindings(InPropertyName, InPropertyPath)))
{

}

UMovieSceneSection* FPropertySection::GetSectionObject()
{
	return &SectionObject;
}

FText FPropertySection::GetDisplayName() const
{
	return DisplayName;
}
	
FText FPropertySection::GetSectionTitle() const
{
	return FText::GetEmpty();
}

int32 FPropertySection::OnPaintSection( FSequencerSectionPainter& Painter ) const
{
	return Painter.PaintSectionBackground();
}

UObject* FPropertySection::GetRuntimeObjectAndUpdatePropertyBindings() const
{
	UObject* RuntimeObject = nullptr;
	if (Sequencer != nullptr && PropertyBindings.IsValid())
	{
		TArray<TWeakObjectPtr<UObject>> RuntimeObjects;
		Sequencer->GetRuntimeObjects(Sequencer->GetFocusedMovieSceneSequenceInstance(), ObjectBinding, RuntimeObjects);
		if (RuntimeObjects.Num() == 1)
		{
			TWeakObjectPtr<UObject> RuntimeObjectPtr = RuntimeObjects[0];
			if (RuntimeObjectPtr.IsValid())
			{
				if (RuntimeObjectPtr != RuntimeObjectCache)
				{
					PropertyBindings->UpdateBindings(RuntimeObjects);
					RuntimeObjectCache = RuntimeObjectPtr;
				}
				RuntimeObject = RuntimeObjectPtr.Get();
			}
		}
	}
	return RuntimeObject;
}

UProperty* FPropertySection::GetProperty() const
{
	UObject* RuntimeObject = GetRuntimeObjectAndUpdatePropertyBindings();
	return PropertyBindings.IsValid() && RuntimeObject != nullptr ? PropertyBindings->GetProperty(RuntimeObject) : nullptr;
}

bool FPropertySection::CanGetPropertyValue() const
{
	return Sequencer != nullptr && PropertyBindings.IsValid();
}

