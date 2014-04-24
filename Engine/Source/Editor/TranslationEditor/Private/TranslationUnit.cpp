// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "TranslationEditorPrivatePCH.h"

UTranslationUnit::UTranslationUnit( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{

}

void UTranslationUnit::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	TranslationUnitPropertyChangedEvent.Broadcast(Name);
}