// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateComponentWrappersPrivatePCH.h"

#define LOCTEXT_NAMESPACE "SlateComponentWrappers"

/////////////////////////////////////////////////////
// UEditableTextBlockComponent

UEditableTextBlockComponent::UEditableTextBlockComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

TSharedRef<SWidget> UEditableTextBlockComponent::RebuildWidget()
{
	return SNew(SEditableTextBox);
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
