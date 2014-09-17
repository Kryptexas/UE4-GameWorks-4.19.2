// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IntroTutorialsPrivatePCH.h"
#include "EditorTutorial.h"

UEditorTutorial::UEditorTutorial(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}


void UEditorTutorial::GoToNextTutorialStage()
{
	FIntroTutorials& IntroTutorials = FModuleManager::GetModuleChecked<FIntroTutorials>(TEXT("IntroTutorials"));
	IntroTutorials.GoToNextStage(nullptr);
}


void UEditorTutorial::GoToPreviousTutorialStage()
{
	FIntroTutorials& IntroTutorials = FModuleManager::GetModuleChecked<FIntroTutorials>(TEXT("IntroTutorials"));
	IntroTutorials.GoToPreviousStage();
}


void UEditorTutorial::BeginTutorial(UEditorTutorial* TutorialToStart, bool bRestart)
{
	FIntroTutorials& IntroTutorials = FModuleManager::GetModuleChecked<FIntroTutorials>(TEXT("IntroTutorials"));
	IntroTutorials.LaunchTutorial(TutorialToStart, bRestart);
}


void UEditorTutorial::HandleTutorialStageStarted(FName StageName)
{
	FEditorScriptExecutionGuard ScriptGuard;
	OnTutorialStageStarted(StageName);
}


void UEditorTutorial::HandleTutorialStageEnded(FName StageName)
{
	FEditorScriptExecutionGuard ScriptGuard;
	OnTutorialStageEnded(StageName);
}


void UEditorTutorial::OpenAsset(UObject* Asset)
{
	FAssetEditorManager::Get().OpenEditorForAsset(Asset);
}

