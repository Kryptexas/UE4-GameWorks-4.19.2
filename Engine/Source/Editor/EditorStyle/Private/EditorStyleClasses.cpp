// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	EditorStyleClasses.cpp: Implements the constructors for the various classes.
=============================================================================*/

#include "EditorStylePrivatePCH.h"


/* UEditorStyleSettings interface
 *****************************************************************************/

UEditorStyleSettings::UEditorStyleSettings( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{
	SelectionColor = FLinearColor(0.828f, 0.364f, 0.003f);
	InactiveSelectionColor = FLinearColor(0.25f, 0.25f, 0.25f);
	PressedSelectionColor = FLinearColor(0.701f, 0.225f, 0.003f);

	bShowFriendlyNames = true;
}


#if WITH_EDITOR

void UEditorStyleSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (Name == FName(TEXT("bEnableWindowAnimations")))
	{
		FSlateApplication::Get().EnableMenuAnimations(bEnableWindowAnimations);
	}

//	if (!FUnrealEdMisc::Get().IsDeletePreferences())
	{
		SaveConfig();
	}

	SettingChangedEvent.Broadcast(Name);
}

#endif
