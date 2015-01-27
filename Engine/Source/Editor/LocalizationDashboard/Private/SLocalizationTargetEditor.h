// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCompoundWidget.h"

class UProjectLocalizationSettings;
class ULocalizationTarget;

class SLocalizationTargetEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLocalizationTargetEditor) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UProjectLocalizationSettings* const InProjectSettings, ULocalizationTarget* const InTarget);
};

