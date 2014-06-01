// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UScrollBox

UScrollBox::UScrollBox(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;
}

TSharedRef<SWidget> UScrollBox::RebuildWidget()
{
	return SNew(SScrollBox);
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
