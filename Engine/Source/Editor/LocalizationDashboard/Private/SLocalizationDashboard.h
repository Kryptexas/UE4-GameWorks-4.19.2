// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCompoundWidget.h"

class UProjectLocalizationSettings;

class SLocalizationDashboard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLocalizationDashboard) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UProjectLocalizationSettings* const Settings);

public:
	static const FName TabName;
};