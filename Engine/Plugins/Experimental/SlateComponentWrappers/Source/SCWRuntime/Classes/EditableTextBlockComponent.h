// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EditableTextBlockComponent.generated.h"

/** Editable text box widget */
UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class SCWRUNTIME_API UEditableTextBlockComponent : public USlateWrapperComponent
{
	GENERATED_UCLASS_BODY()

protected:


protected:
	// USlateWrapperComponent interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of USlateWrapperComponent
};
