// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IntroTutorialsPrivatePCH.h"
#include "EditorTutorialSettings.h"

UEditorTutorialSettings::UEditorTutorialSettings(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

}

void UEditorTutorialSettings::FindTutorialsForContext(FName InContext, UEditorTutorial*& OutAttractTutorial, UEditorTutorial*& OutLaunchTutorial) const
{
	for (const auto& Context : TutorialContexts)
	{
		if (Context.Context == InContext)
		{
			TSubclassOf<UEditorTutorial> LaunchTutorialClass = LoadClass<UEditorTutorial>(nullptr, *Context.LaunchTutorial.AssetLongPathname, nullptr, LOAD_None, nullptr);
			if (LaunchTutorialClass != nullptr)
			{
				OutLaunchTutorial = LaunchTutorialClass->GetDefaultObject<UEditorTutorial>();
			}

			TSubclassOf<UEditorTutorial> AttractTutorialClass = LoadClass<UEditorTutorial>(nullptr, *Context.AttractTutorial.AssetLongPathname, nullptr, LOAD_None, nullptr);
			if (AttractTutorialClass != nullptr)
			{
				OutAttractTutorial = AttractTutorialClass->GetDefaultObject<UEditorTutorial>();
			}

			return;
		}
	}
}