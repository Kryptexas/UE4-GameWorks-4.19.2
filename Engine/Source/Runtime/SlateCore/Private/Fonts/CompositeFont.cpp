// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateCorePrivatePCH.h"
#include "CompositeFont.h"

FFontData::FFontData()
	: FontFilename()
	, BulkDataPtr(nullptr)
	, Hinting(EFontHinting::Default)
	, FontData_DEPRECATED()
{
}

FFontData::FFontData(FString InFontFilename, const UFontBulkData* const InBulkData, const EFontHinting InHinting)
	: FontFilename(MoveTemp(InFontFilename))
	, BulkDataPtr(InBulkData)
	, Hinting(InHinting)
	, FontData_DEPRECATED()
{
}

void FFontData::SetFont(FString InFontFilename, const UFontBulkData* const InBulkData)
{
	FontFilename = MoveTemp(InFontFilename);
	BulkDataPtr = InBulkData;
}

void FStandaloneCompositeFont::AddReferencedObjects(FReferenceCollector& Collector)
{
	for(FTypefaceEntry& TypefaceEntry : DefaultTypeface.Fonts)
	{
		Collector.AddReferencedObject(TypefaceEntry.Font.BulkDataPtr);
	}

	for(FCompositeSubFont& SubFont : SubTypefaces)
	{
		for(FTypefaceEntry& TypefaceEntry : SubFont.Typeface.Fonts)
		{
			Collector.AddReferencedObject(TypefaceEntry.Font.BulkDataPtr);
		}
	}
}
