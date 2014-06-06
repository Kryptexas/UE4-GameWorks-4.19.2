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

void USpacer::SetSize(FVector2D InSize)
{
	Size = InSize;

	if ( MySpacer.IsValid() )
	{
		MySpacer->SetSize(InSize);
	}
}

TSharedRef<SWidget> USpacer::RebuildWidget()
{
	MySpacer = SNew(SSpacer)
		.Size(Size);

	return MySpacer.ToSharedRef();
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
