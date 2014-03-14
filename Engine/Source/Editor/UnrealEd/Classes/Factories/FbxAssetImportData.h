// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "FbxAssetImportData.generated.h"

/**
 * Base class for import data and options used when importing any asset from FBX
 */
UCLASS(config=EditorUserSettings, HideCategories=Object, abstract)
class UFbxAssetImportData : public UAssetImportData
{
	GENERATED_UCLASS_BODY()
};