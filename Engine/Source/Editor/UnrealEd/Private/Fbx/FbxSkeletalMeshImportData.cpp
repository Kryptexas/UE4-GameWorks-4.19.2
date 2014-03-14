// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

UFbxSkeletalMeshImportData::UFbxSkeletalMeshImportData(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bImportMeshesInBoneHierarchy = false;
}

UFbxSkeletalMeshImportData* UFbxSkeletalMeshImportData::GetImportDataForSkeletalMesh(USkeletalMesh* SkeletalMesh, UFbxSkeletalMeshImportData* TemplateForCreation)
{
	check(SkeletalMesh);
	
	UFbxSkeletalMeshImportData* ImportData = Cast<UFbxSkeletalMeshImportData>(SkeletalMesh->AssetImportData);
	if ( !ImportData )
	{
		ImportData = ConstructObject<UFbxSkeletalMeshImportData>(UFbxSkeletalMeshImportData::StaticClass(), SkeletalMesh, NAME_None, RF_NoFlags, TemplateForCreation);

		// Try to preserve the source file path if possible
		if ( SkeletalMesh->AssetImportData != NULL )
		{
			ImportData->SourceFilePath = SkeletalMesh->AssetImportData->SourceFilePath;
			ImportData->SourceFileTimestamp = SkeletalMesh->AssetImportData->SourceFileTimestamp;
		}

		SkeletalMesh->AssetImportData = ImportData;
	}

	return ImportData;
}