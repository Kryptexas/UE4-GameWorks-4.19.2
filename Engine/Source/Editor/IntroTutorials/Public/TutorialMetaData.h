// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ISlateMetaData.h"

class INTROTUTORIALS_API FBlueprintGraphNodeMetaData : public FTagMetaData
{
public:
	SLATE_METADATA_TYPE(FBlueprintGraphNodeMetaData, FTagMetaData)

		FBlueprintGraphNodeMetaData(FName InTag, FString InOuterName, FGuid InGUID)
		: FTagMetaData(InTag)
		, GUID(InGUID)
		, OuterName(InOuterName)
		{
		}

	
	/** GUID for the node */
	FGuid GUID;

	/** Name of the outer (which should be the blueprint) */
	FString OuterName;

	/** User friendly display name */
	FString FriendlyName;
};