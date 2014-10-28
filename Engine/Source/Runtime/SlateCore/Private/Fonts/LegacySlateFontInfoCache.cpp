// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateCorePrivatePCH.h"
#include "LegacySlateFontInfoCache.h"

TSharedPtr<FLegacySlateFontInfoCache> FLegacySlateFontInfoCache::Singleton;

FLegacySlateFontInfoCache& FLegacySlateFontInfoCache::Get()
{
	if (!Singleton.IsValid())
	{
		Singleton = MakeShareable(new FLegacySlateFontInfoCache());
	}
	return *Singleton;
}

TSharedPtr<const FCompositeFont> FLegacySlateFontInfoCache::GetCompositeFont(const FName& InLegacyFontName, const EFontHinting InLegacyFontHinting)
{
	if (InLegacyFontName.IsNone())
	{
		return nullptr;
	}

	FString LegacyFontPath = InLegacyFontName.ToString();

	// Work out what the given path is supposed to be relative to
	if (!FPaths::FileExists(LegacyFontPath))
	{
		// UMG assets specify the path either relative to the game or engine content directories - test both
		LegacyFontPath = FPaths::GameContentDir() / InLegacyFontName.ToString();
		if (!FPaths::FileExists(LegacyFontPath))
		{
			LegacyFontPath = FPaths::EngineContentDir() / InLegacyFontName.ToString();
			if (!FPaths::FileExists(LegacyFontPath))
			{
				// Missing font file - just use what we were given
				LegacyFontPath = InLegacyFontName.ToString();
			}
		}
	}

	const FLegacyFontKey LegacyFontKey(*LegacyFontPath, InLegacyFontHinting);

	{
		TSharedPtr<const FCompositeFont>* const ExistingCompositeFont = LegacyFontNameToCompositeFont.Find(LegacyFontKey);
		if(ExistingCompositeFont)
		{
			return *ExistingCompositeFont;
		}
	}

	TSharedRef<const FCompositeFont> NewCompositeFont = MakeShareable(new FCompositeFont(NAME_None, LegacyFontPath, InLegacyFontHinting));
	LegacyFontNameToCompositeFont.Add(LegacyFontKey, NewCompositeFont);
	return NewCompositeFont;
}

TSharedPtr<const FCompositeFont> FLegacySlateFontInfoCache::GetSystemFont()
{
	if (!SystemFont.IsValid())
	{
		TArray<uint8> FontBytes = FPlatformMisc::GetSystemFontBytes();
		SystemFont = MakeShareable(new FCompositeFont(NAME_None, TEXT("DefaultSystemFont"), MoveTemp(FontBytes), EFontHinting::Default));
	}
	return SystemFont;
}

const FFontData& FLegacySlateFontInfoCache::GetFallbackFont()
{
	const FName FallbackFontName = *(FPaths::EngineContentDir() / TEXT("Slate/Fonts/") / (NSLOCTEXT("Slate", "FallbackFont", "DroidSansFallback").ToString() + TEXT(".ttf")));

	{
		TSharedPtr<const FFontData>* const ExistingFallbackFont = FallbackFonts.Find(FallbackFontName);
		if(ExistingFallbackFont)
		{
			return **ExistingFallbackFont;
		}
	}

	TSharedRef<const FFontData> NewFallbackFont = MakeShareable(new FFontData(FallbackFontName.ToString(), EFontHinting::Default));
	FallbackFonts.Add(FallbackFontName, NewFallbackFont);
	return *NewFallbackFont;
}

const FFontData& FLegacySlateFontInfoCache::GetLastResortFont()
{
	if (!LastResortFont.IsValid())
	{
		const FString LastResortFontPath = FPaths::EngineContentDir() / TEXT("Slate/Fonts/LastResort.ttf");
		LastResortFont = MakeShareable(new FFontData(LastResortFontPath, EFontHinting::Default));
	}
	return *LastResortFont;
}
