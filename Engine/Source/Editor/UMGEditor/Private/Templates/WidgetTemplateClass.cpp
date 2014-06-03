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
	const FString& Category = WidgetClass->GetMetaData("Category");

	if ( Category.IsEmpty() )
	{
		return LOCTEXT("Misc", "Misc");
	}

	return FText::FromString(Category);
}

UWidget* FWidgetTemplateClass::Create(UWidgetTree* Tree)
{
	return Tree->ConstructWidget<UWidget>(WidgetClass);
}

#undef LOCTEXT_NAMESPACE
