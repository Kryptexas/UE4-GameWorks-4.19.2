// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateComponentWrappersPrivatePCH.h"

#define LOCTEXT_NAMESPACE "SlateComponentWrappers"

/////////////////////////////////////////////////////
// UScrollBoxComponent

UScrollBoxComponent::UScrollBoxComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

TSharedRef<SWidget> UScrollBoxComponent::RebuildWidget()
{
	return SNew(SScrollBox);
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
