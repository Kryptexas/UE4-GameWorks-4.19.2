// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "GUIPageFactory.h"

#define LOCTEXT_NAMESPACE "UGUIPageFactory"

/////////////////////////////////////////////////////
// UGUIPageFactory

UGUIPageFactory::UGUIPageFactory(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UGUIPage::StaticClass();
}

FText UGUIPageFactory::GetDisplayName() const
{
	return LOCTEXT("GUIPageFactoryDescription", "GUI Page");
}

UObject* UGUIPageFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UGUIPage* NewPage = ConstructObject<UGUIPage>(Class, InParent, Name, Flags);

	return NewPage;
}

#undef LOCTEXT_NAMESPACE