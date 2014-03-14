// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateComponentWrappersPrivatePCH.h"

#define LOCTEXT_NAMESPACE "SlateComponentWrappers"

/////////////////////////////////////////////////////
// USpacerComponent

USpacerComponent::USpacerComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, Size(1.0f, 1.0f)
{
}

TSharedRef<SWidget> USpacerComponent::RebuildWidget()
{
	return SNew(SSpacer)
		.Size( BIND_UOBJECT_ATTRIBUTE(FVector2D, GetSpacerSize) );
}

FVector2D USpacerComponent::GetSpacerSize() const
{
	return Size;
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
