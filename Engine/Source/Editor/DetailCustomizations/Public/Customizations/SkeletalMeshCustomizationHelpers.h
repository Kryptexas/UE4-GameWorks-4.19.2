// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Templates/SharedPointer.h"
#include "PropertyHandle.h"
#include "Widgets/SWidget.h"

namespace SkeletalMeshCustomizationHelpers
{
	/** Creates a warning widget along with the appropriate "Use async scene" property value widget.
	* This warns the user that the property will have no effect if the project is not set to use an async scene. */
	DETAILCUSTOMIZATIONS_API TSharedRef<SWidget> CreateAsyncSceneValueWidgetWithWarning(const TSharedPtr<IPropertyHandle>& AsyncScenePropertyHandle);
}