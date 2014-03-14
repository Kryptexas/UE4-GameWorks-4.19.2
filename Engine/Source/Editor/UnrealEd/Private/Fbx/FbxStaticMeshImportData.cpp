// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

UFbxStaticMeshImportData::UFbxStaticMeshImportData(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	StaticMeshLODGroup = NAME_None;
	bRemoveDegenerates = true;
	bOneConvexHullPerUCX = true;
}

UFbxStaticMeshImportData* UFbxStaticMeshImportData::GetImportDataForStaticMesh(UStaticMesh* StaticMesh, UFbxStaticMeshImportData* TemplateForCreation)
{
	check(StaticMesh);
	
	UFbxStaticMeshImportData* ImportData = Cast<UFbxStaticMeshImportData>(StaticMesh->AssetImportData);
	if ( !ImportData )
	{
		ImportData = ConstructObject<UFbxStaticMeshImportData>(UFbxStaticMeshImportData::StaticClass(), StaticMesh, NAME_None, RF_NoFlags, TemplateForCreation);

		// Try to preserve the source file path if possible
		if ( StaticMesh->AssetImportData != NULL )
		{
			ImportData->SourceFilePath = StaticMesh->AssetImportData->SourceFilePath;
			ImportData->SourceFileTimestamp = StaticMesh->AssetImportData->SourceFileTimestamp;
		}

		StaticMesh->AssetImportData = ImportData;
	}

	return ImportData;
}