// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "VREditorModule.h"
#include "VREditorWidgetComponent.h"

UVREditorWidgetComponent::UVREditorWidgetComponent(const FObjectInitializer& ObjectInitializer)
	: Super( ObjectInitializer )
{
	bAllowDrawing = false;
	bHasEverDrawn = false;
}

bool UVREditorWidgetComponent::ShouldDrawWidget() const
{
	if ( bAllowDrawing || !bHasEverDrawn )
	{
		return Super::ShouldDrawWidget();
	}

	return false;
}

void UVREditorWidgetComponent::DrawWidgetToRenderTarget(float DeltaTime)
{
	bHasEverDrawn = true;

	Super::DrawWidgetToRenderTarget(DeltaTime);
}
