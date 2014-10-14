// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ISlateMetaData.h"

/**
 * Simple tagging metadata
 */
class FNavigationMetaData : public ISlateMetaData
{
public:
	SLATE_METADATA_TYPE(FNavigationMetaData, ISlateMetaData)

	FNavigationMetaData()
	{
	}

private:
	/*
	struct SNavData
	{
		EUINavigationRule BoundaryRule;
		TAttribute<TSharedPtr<SWidget>> FocusRecipient;
	};
	SNavData Rules[EUINavigation::Num];
	*/
};
