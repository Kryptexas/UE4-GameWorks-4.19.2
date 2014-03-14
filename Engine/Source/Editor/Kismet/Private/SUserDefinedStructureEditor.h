// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintEditor.h"

class SUserDefinedStructureEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SUserDefinedStructureEditor ){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TWeakPtr<FBlueprintEditor> InBlueprintEditor);
};