// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

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
