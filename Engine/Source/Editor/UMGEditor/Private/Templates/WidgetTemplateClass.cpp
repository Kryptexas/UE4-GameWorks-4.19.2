// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "WidgetTemplateClass.h"

#define LOCTEXT_NAMESPACE "UMGEditor"

FWidgetTemplateClass::FWidgetTemplateClass(TSubclassOf<UWidget> InWidgetClass)
	: WidgetClass(InWidgetClass)
{
}

FText FWidgetTemplateClass::GetCategory()
{
	// TODO this should be pulled from metadata from the class, that describes its categorization, is it a panel, a common control...etc?
	return LOCTEXT("Class", "Class");
}

UWidget* FWidgetTemplateClass::Create(UWidgetTree* Tree)
{
	return Tree->ConstructWidget<UWidget>(WidgetClass);
}

#undef LOCTEXT_NAMESPACE
