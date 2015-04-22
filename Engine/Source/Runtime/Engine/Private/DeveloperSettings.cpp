// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Engine/DeveloperSettings.h"

UDeveloperSettings::UDeveloperSettings(const FObjectInitializer& ObjectInitializer)
	: UObject(ObjectInitializer)
{
	CategoryName = NAME_None;
	Container = EDeveloperSettingsContainer::Project;
}

FName UDeveloperSettings::GetContainerName() const
{
	FName ProjectName(TEXT("Project"));
	FName EditorName(TEXT("Editor"));

	switch ( Container )
	{
	case EDeveloperSettingsContainer::Project:
		return ProjectName;
	case EDeveloperSettingsContainer::Editor:
		return EditorName;
	default:
		check(false);
		return NAME_None;
	}
}

FName UDeveloperSettings::GetCategoryName() const
{
	if ( CategoryName != NAME_None )
	{
		return CategoryName;
	}

	FName ConfigName = GetClass()->ClassConfigName;
	if ( ConfigName == NAME_Engine || ConfigName == NAME_Input )
	{
		return NAME_Engine;
	}
	else if ( ConfigName == NAME_Editor || ConfigName == NAME_EditorSettings || ConfigName == NAME_EditorLayout || ConfigName == NAME_EditorKeyBindings )
	{
		return NAME_Editor;
	}
	else if ( ConfigName == NAME_Game )
	{
		return NAME_Game;
	}

	return NAME_Engine;
}

FName UDeveloperSettings::GetSectionName() const
{
	return GetClass()->GetFName();
}

#if WITH_EDITOR
FText UDeveloperSettings::GetSectionText() const
{
	return GetClass()->GetDisplayNameText();
}

FText UDeveloperSettings::GetSectionDescription() const
{
	return GetClass()->GetToolTipText();
}
#endif

TSharedPtr<SWidget> UDeveloperSettings::GetCustomSettingsWidget() const
{
	return TSharedPtr<SWidget>();
}
