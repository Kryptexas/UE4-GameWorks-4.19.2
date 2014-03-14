// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateComponentWrappersPrivatePCH.h"

/////////////////////////////////////////////////////
// USlateLeafWidgetComponent

USlateLeafWidgetComponent::USlateLeafWidgetComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

bool USlateLeafWidgetComponent::CanAttachAsChild(USceneComponent* ChildComponent, FName SocketName) const
{
	return false;
}
