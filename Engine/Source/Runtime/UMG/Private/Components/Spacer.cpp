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
	MySpacer = SNew(SSpacer);

	//TODO UMG COnsider using a design time wrapper for spacer to show expandy arrows or some other
	// indicator that there's a widget at work here.
	
	return MySpacer.ToSharedRef();
}

void USpacer::SyncronizeProperties()
{
	Super::SyncronizeProperties();
	
	MySpacer->SetSize(Size);
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
