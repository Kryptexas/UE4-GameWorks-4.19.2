// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

UFbxAssetImportData::UFbxAssetImportData(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, ImportTranslation(0)
	, ImportRotation(0)
	, ImportUniformScale(1.0f)
{
	
}