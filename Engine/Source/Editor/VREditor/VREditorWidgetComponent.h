// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/WidgetComponent.h"
#include "VREditorWidgetComponent.generated.h"

/**
 * A specialized WidgetComponent for the VREditor
 */
UCLASS(hidedropdown)
class UVREditorWidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	/** Default constructor that sets up CDO properties */
	UVREditorWidgetComponent(const FObjectInitializer& ObjectInitializer);
};