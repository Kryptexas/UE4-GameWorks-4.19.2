// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GameProjectGenerationPrivatePCH.h"
#include "ModuleManager.h"
#include "GameProjectGenerationModule.h"
#include "TemplateCategory.h"
#include "SourceCodeNavigation.h"

IMPLEMENT_MODULE( FGameProjectGenerationModule, GameProjectGeneration );
DEFINE_LOG_CATEGORY(LogGameProjectGeneration);

#define LOCTEXT_NAMESPACE "GameProjectGeneration"

FName FTemplateCategory::BlueprintCategoryName = "Blueprint";
FName FTemplateCategory::CodeCategoryName = "C++";

void FGameProjectGenerationModule::StartupModule()
{
	RegisterTemplateCategory(
		FTemplateCategory::BlueprintCategoryName,
		LOCTEXT("BlueprintCategory_Name", "Blueprint"),
		LOCTEXT("BlueprintCategory_Description", "Blueprint templates require no programming knowledge.\nAll game mechanics can be implemented using Blueprint visual scripting.\nEach template includes a basic set of blueprints to use as a starting point for your game."),
		FEditorStyle::GetBrush("GameProjectDialog.BlueprintImage"));

	RegisterTemplateCategory(
		FTemplateCategory::CodeCategoryName,
		LOCTEXT("CodeCategory_Name", "C++"),
		FText::Format(
			LOCTEXT("CodeCategory_Description", "C++ templates offer a good example of how to work with some of the core concepts of the Engine from code.\nYou still have the option of adding your own blueprints to the project at a later date if you want.\nChoosing this template type requires you to have {0} installed."),
			FSourceCodeNavigation::GetSuggestedSourceCodeIDE()
		),
		FEditorStyle::GetBrush("GameProjectDialog.CodeImage"));
}


void FGameProjectGenerationModule::ShutdownModule()
{
	
}


TSharedRef<class SWidget> FGameProjectGenerationModule::CreateGameProjectDialog(bool bAllowProjectOpening, bool bAllowProjectCreate)
{
	return SNew(SGameProjectDialog)
		.AllowProjectOpening(bAllowProjectOpening)
		.AllowProjectCreate(bAllowProjectCreate);
}


TSharedRef<class SWidget> FGameProjectGenerationModule::CreateNewClassDialog(class UClass* InClass)
{
	return SNew(SNewClassDialog).Class(InClass);
}


void FGameProjectGenerationModule::OpenAddCodeToProjectDialog()
{
	GameProjectUtils::OpenAddCodeToProjectDialog();
	AddCodeToProjectDialogOpenedEvent.Broadcast();
}

void FGameProjectGenerationModule::TryMakeProjectFileWriteable(const FString& ProjectFile)
{
	GameProjectUtils::TryMakeProjectFileWriteable(ProjectFile);
}

void FGameProjectGenerationModule::CheckForOutOfDateGameProjectFile()
{
	GameProjectUtils::CheckForOutOfDateGameProjectFile();
}


bool FGameProjectGenerationModule::UpdateGameProject(const FString& ProjectFile, const FString& EngineIdentifier, FText& OutFailReason)
{
	return GameProjectUtils::UpdateGameProject(ProjectFile, EngineIdentifier, OutFailReason);
}


bool FGameProjectGenerationModule::UpdateCodeProject(FText& OutFailReason)
{
	const bool bAllowNewSlowTask = true;
	FScopedSlowTask SlowTaskMessage( LOCTEXT( "UpdatingCodeProject", "Updating code project..." ), bAllowNewSlowTask );

	return GameProjectUtils::GenerateCodeProjectFiles(FPaths::GetProjectFilePath(), OutFailReason);
}


int32 FGameProjectGenerationModule::GetProjectCodeFileCount()
{
	return GameProjectUtils::GetProjectCodeFileCount();
}

void FGameProjectGenerationModule::GetProjectSourceDirectoryInfo(int32& OutNumFiles, int64& OutDirectorySize)
{
	GameProjectUtils::GetProjectSourceDirectoryInfo(OutNumFiles, OutDirectorySize);
}

bool FGameProjectGenerationModule::UpdateCodeResourceFiles(TArray<FString>& OutCreatedFiles, FText& OutFailReason)
{
	const FString GameModuleSourcePath = FPaths::GetPath(FPaths::GetProjectFilePath()) / TEXT("Source") / FApp::GetGameName();
	return GameProjectUtils::GenerateGameResourceFiles(GameModuleSourcePath, FApp::GetGameName(), OutCreatedFiles, OutFailReason);
}


void FGameProjectGenerationModule::CheckAndWarnProjectFilenameValid()
{
	GameProjectUtils::CheckAndWarnProjectFilenameValid();
}


void FGameProjectGenerationModule::UpdateSupportedTargetPlatforms(const FName& InPlatformName, const bool bIsSupported)
{
	GameProjectUtils::UpdateSupportedTargetPlatforms(InPlatformName, bIsSupported);
}

void FGameProjectGenerationModule::ClearSupportedTargetPlatforms()
{
	GameProjectUtils::ClearSupportedTargetPlatforms();
}

bool FGameProjectGenerationModule::RegisterTemplateCategory(FName Type, FText Name, FText Description, const FSlateBrush* Thumbnail)
{
	if (TemplateCategories.Contains(Type))
	{
		return false;
	}

	FTemplateCategory Category = { Name, Description, Thumbnail, Type };
	TemplateCategories.Add(Type, MakeShareable(new FTemplateCategory(Category)));
	return true;
}

void FGameProjectGenerationModule::UnRegisterTemplateCategory(FName Type)
{
	TemplateCategories.Remove(Type);
}

#undef LOCTEXT_NAMESPACE
