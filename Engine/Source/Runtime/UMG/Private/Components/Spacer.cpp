// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// USpacer

USpacer::USpacer(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, Size(1.0f, 1.0f)
{
	bIsVariable = false;
}

TSharedRef<SWidget> USpacer::RebuildWidget()
{
	return SNew(SSpacer)
		.Size(Size);
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
