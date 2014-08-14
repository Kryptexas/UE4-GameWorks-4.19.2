// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IntroTutorialsPrivatePCH.h"
#include "EditorTutorialFactory.h"
#include "EditorTutorial.h"
#include "KismetEditorUtilities.h"
#include "SEditorTutorialImporter.h"
#include "IAssetTypeActions.h"

#define LOCTEXT_NAMESPACE "UEditorTutorialFactory"

UEditorTutorialFactory::UEditorTutorialFactory(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UBlueprint::StaticClass();
}

bool UEditorTutorialFactory::ConfigureProperties()
{
	TSharedPtr<SWindow> RootWindow = FGlobalTabmanager::Get()->GetRootWindow();
	if(RootWindow.IsValid())
	{
		// show import dialog
		TSharedRef<SWindow> Window = SNew(SWindow)
				.Title(LOCTEXT("WindowTitle", "Import Tutorial"))
				.SupportsMaximize(false)
				.SupportsMinimize(false)
				.SizingRule(ESizingRule::Autosized);	

		ImporterDialog = SNew(SEditorTutorialImporter)
			.ParentWindow(Window);

		Window->SetContent(ImporterDialog.ToSharedRef());

		FSlateApplication::Get().AddModalWindow(Window, RootWindow.ToSharedRef());
	}

	return true;
}

UObject* UEditorTutorialFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UBlueprint* NewBlueprint = FKismetEditorUtilities::CreateBlueprint(UEditorTutorial::StaticClass(), InParent, Name, BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), NAME_None);
	
	if(ImporterDialog.IsValid() && !ImporterDialog->GetImportPath().IsEmpty())
	{
		ImporterDialog->Import(CastChecked<UEditorTutorial>(NewBlueprint->GeneratedClass->GetDefaultObject()));
		ImporterDialog = nullptr;
	}

	return NewBlueprint;
}

uint32 UEditorTutorialFactory::GetMenuCategories() const
{
	return EAssetTypeCategories::Misc;
}

FText UEditorTutorialFactory::GetDisplayName() const
{
	return LOCTEXT("TutorialMenuEntry", "Tutorial Blueprint");
}

#undef LOCTEXT_NAMESPACE