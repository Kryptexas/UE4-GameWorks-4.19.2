// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

UFbxAnimSequenceImportData::UFbxAnimSequenceImportData(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	
}

UFbxAnimSequenceImportData* UFbxAnimSequenceImportData::GetImportDataForAnimSequence(UAnimSequence* AnimSequence, UFbxAnimSequenceImportData* TemplateForCreation)
{
	check(AnimSequence);

	UFbxAnimSequenceImportData* ImportData = Cast<UFbxAnimSequenceImportData>(AnimSequence->AssetImportData);
	if ( !ImportData )
	{
		ImportData = ConstructObject<UFbxAnimSequenceImportData>(UFbxAnimSequenceImportData::StaticClass(), AnimSequence, NAME_None, RF_NoFlags, TemplateForCreation);

		// Try to preserve the source file path if possible
		if ( AnimSequence->AssetImportData != NULL )
		{
			ImportData->SourceFilePath = AnimSequence->AssetImportData->SourceFilePath;
			ImportData->SourceFileTimestamp = AnimSequence->AssetImportData->SourceFileTimestamp;
		}

		AnimSequence->AssetImportData = ImportData;
	}

	return ImportData;
}