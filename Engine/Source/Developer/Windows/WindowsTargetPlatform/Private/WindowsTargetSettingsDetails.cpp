// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "WindowsTargetPlatformPrivatePCH.h"


#if WITH_EDITOR

namespace WindowsTargetSettingsDetailsConstants
{
	/** The filename for the game splash screen */
	const FString GameSplashFileName(TEXT("Splash/Splash.bmp"));

	/** The filename for the editor splash screen */
	const FString EditorSplashFileName(TEXT("Splash/EdSplash.bmp"));
}

#define LOCTEXT_NAMESPACE "WindowsTargetSettingsDetails"

TSharedRef<IDetailCustomization> FWindowsTargetSettingsDetails::MakeInstance()
{
	return MakeShareable(new FWindowsTargetSettingsDetails);
}

namespace EImageScope
{
	enum Type
	{
		Engine,
		GameOverride
	};
}

/* Helper function used to generate filenames for splash screens */
static FString GetSplashFilename(EImageScope::Type Scope, bool bIsEditorSplash)
{
	FString Filename;

	if (Scope == EImageScope::Engine)
	{
		Filename = FPaths::EngineContentDir();
	}
	else
	{
		Filename = FPaths::GameContentDir();
	}

	if(bIsEditorSplash)
	{
		Filename /= WindowsTargetSettingsDetailsConstants::EditorSplashFileName;
	}
	else
	{
		Filename /= WindowsTargetSettingsDetailsConstants::GameSplashFileName;
	}

	Filename = FPaths::ConvertRelativePathToFull(Filename);

	return Filename;
}

void FWindowsTargetSettingsDetails::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
	IDetailCategoryBuilder& DetailCategoryBuilder = DetailBuilder.EditCategory(TEXT("Splash"));

	FDetailWidgetRow& EditorSplashWidgetRow = DetailCategoryBuilder.AddCustomRow(TEXT("Editor Splash"));

	const FText EditorSplashDesc(LOCTEXT("EditorSplashLabel", "Editor Splash"));
	const FString EditorSplash_TargetImagePath = GetSplashFilename(EImageScope::GameOverride, true);
	const FString EditorSplash_DefaultImagePath = GetSplashFilename(EImageScope::Engine, true);

	EditorSplashWidgetRow
	.NameContent()
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.Padding( FMargin( 0, 1, 0, 1 ) )
		.FillWidth(1.0f)
		[
			SNew(STextBlock)
			.Text(EditorSplashDesc)
			.Font(DetailBuilder.GetDetailFont())
		]
	]
	.ValueContent()
	.MaxDesiredWidth(500.0f)
	.MinDesiredWidth(100.0f)
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SExternalImageReference, EditorSplash_DefaultImagePath, EditorSplash_TargetImagePath)
			.FileDescription(EditorSplashDesc)
		]
	];

	FDetailWidgetRow& GameSplashWidgetRow = DetailCategoryBuilder.AddCustomRow(TEXT("Game Splash"));

	const FText GameSplashDesc(LOCTEXT("GameSplashLabel", "Game Splash"));
	const FString GameSplash_TargetImagePath = GetSplashFilename(EImageScope::GameOverride, false);
	const FString GameSplash_DefaultImagePath = GetSplashFilename(EImageScope::Engine, false);

	GameSplashWidgetRow
	.NameContent()
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.Padding( FMargin( 0, 1, 0, 1 ) )
		.FillWidth(1.0f)
		[
			SNew(STextBlock)
			.Text(GameSplashDesc)
			.Font(DetailBuilder.GetDetailFont())
		]
	]
	.ValueContent()
	.MaxDesiredWidth(500.0f)
	.MinDesiredWidth(100.0f)
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SExternalImageReference, GameSplash_DefaultImagePath, GameSplash_TargetImagePath)
			.FileDescription(GameSplashDesc)
		]
	];
}

#undef LOCTEXT_NAMESPACE

#endif // WITH_EDITOR