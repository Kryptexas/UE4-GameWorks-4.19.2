// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UScrollBoxComponent

UScrollBoxComponent::UScrollBoxComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;
}

TSharedRef<SWidget> UScrollBoxComponent::RebuildWidget()
{
	return SNew(SScrollBox);
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
