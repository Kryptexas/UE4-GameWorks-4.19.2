// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateCorePrivatePCH.h"
#include "CompositeFont.h"

FFontData::FFontData(FString InFontFilename, const EFontHinting InHinting)
	: FontFilename(MoveTemp(InFontFilename))
	, FontData()
	, Hinting(InHinting)
{
	if (!FFileHelper::LoadFileToArray(FontData, *FontFilename, FILEREAD_Silent))
	{
		UE_LOG(LogSlate, Warning, TEXT("Failed to load font data from '%s'"), *FontFilename);
	}
}

bool FFontData::SetFont(FString InFontFilename)
{
	TArray<uint8> TempFontData;
	if (!FFileHelper::LoadFileToArray(TempFontData, *InFontFilename, FILEREAD_Silent))
	{
		UE_LOG(LogSlate, Warning, TEXT("Failed to load font data from '%s'"), *InFontFilename);
		return false;
	}

	FontFilename = MoveTemp(InFontFilename);
	FontData = MoveTemp(TempFontData);
	return true;
}

void FFontData::SetFont(FString InFontFilename, TArray<uint8> InFontData)
{
	FontFilename = MoveTemp(InFontFilename);
	FontData = MoveTemp(InFontData);
}
