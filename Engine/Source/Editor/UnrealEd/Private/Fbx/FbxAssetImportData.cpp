// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Factories/FbxAssetImportData.h"

UFbxAssetImportData::UFbxAssetImportData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ImportTranslation(0)
	, ImportRotation(0)
	, ImportUniformScale(1.0f)
	, bConvertScene(true)
	, bForceFrontXAxis(false)
	, bConvertSceneUnit(false)
	, bImportAsScene(false)
	, FbxSceneImportDataReference(nullptr)
{
	
}
