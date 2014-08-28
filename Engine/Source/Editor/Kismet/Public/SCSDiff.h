// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SBlueprintDiff.h"

class KISMET_API FSCSDiff
{
public:
	FSCSDiff(const class UBlueprint* InBlueprint);

	void HighlightProperty(FSCSDiffEntry Property);
	TSharedRef< SWidget > TreeWidget();
private:
	TSharedPtr< class SWidget > ContainerWidget;
	TSharedPtr< class SSCSEditor > SCSEditor;
};
