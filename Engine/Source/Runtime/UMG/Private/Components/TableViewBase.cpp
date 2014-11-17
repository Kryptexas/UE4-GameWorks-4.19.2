// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UTableViewBase

UTableViewBase::UTableViewBase(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = true;
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
