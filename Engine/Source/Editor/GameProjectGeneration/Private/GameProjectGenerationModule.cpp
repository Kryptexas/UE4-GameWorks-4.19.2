// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GameProjectGenerationPrivatePCH.h"
#include "ModuleManager.h"
#include "GameProjectGenerationModule.h"



IMPLEMENT_MODULE( FGameProjectGenerationModule, GameProjectGeneration );
DEFINE_LOG_CATEGORY(LogGameProjectGeneration);

#define LOCTEXT_NAMESPACE "GameProjectGeneration"

void FGameProjectGenerationModule::StartupModule()
{
	
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


void FGameProjectGenerationModule::CheckForOutOfDateGameProjectFile()
{
	GameProjectUtils::CheckForOutOfDateGameProjectFile();
}


bool FGameProjectGenerationModule::UpdateGameProject(const FString& EngineIdentifier)
{
	return GameProjectUtils::UpdateGameProject(EngineIdentifier);
}


bool FGameProjectGenerationModule::UpdateCodeProject(FText& OutFailReason)
{
	const bool bAllowNewSlowTask = true;
	FStatusMessageContext SlowTaskMessage( LOCTEXT( "UpdatingCodeProject", "Updating code project..." ), bAllowNewSlowTask );

	return GameProjectUtils::GenerateCodeProjectFiles(FPaths::GetProjectFilePath(), OutFailReason);
}


int32 FGameProjectGenerationModule::GetProjectCodeFileCount()
{
	return GameProjectUtils::GetProjectCodeFileCount();
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

#undef LOCTEXT_NAMESPACE
