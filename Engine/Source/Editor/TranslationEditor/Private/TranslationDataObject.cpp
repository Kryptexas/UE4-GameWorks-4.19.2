// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "TranslationEditorPrivatePCH.h"
#include "TranslationDataObject.h"
#include "TranslationEditor.generated.inl"

UTranslationDataObject::UTranslationDataObject( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{

}

void UTranslationDataObject::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	TranslationDataPropertyChangedEvent.Broadcast(Name);
}