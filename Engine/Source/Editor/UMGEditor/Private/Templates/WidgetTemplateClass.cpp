// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "WidgetTemplateClass.h"

#define LOCTEXT_NAMESPACE "UMGEditor"

FWidgetTemplateClass::FWidgetTemplateClass(TSubclassOf<UWidget> InWidgetClass)
	: WidgetClass(InWidgetClass)
{
}

FText FWidgetTemplateClass::GetCategory() const
{
	if ( Category.IsEmpty() )
	{
		const FString& MetadatCategory = WidgetClass->GetMetaData("Category");

		if ( MetadatCategory.IsEmpty() )
		{
			if ( WidgetClass->IsChildOf(UUserWidget::StaticClass()) )
			{
				return LOCTEXT("UserControls", "User Controls");
			}

			return LOCTEXT("Misc", "Misc");
		}
		else
		{
			Category = FText::FromString(MetadatCategory);
		}
	}

	return Category;
}

UWidget* FWidgetTemplateClass::Create(UWidgetTree* Tree)
{
	return Tree->ConstructWidget<UWidget>(WidgetClass);
}

const FSlateBrush* FWidgetTemplateClass::GetIcon() const
{
	UWidget* DefaultWidget = WidgetClass->GetDefaultObject<UWidget>();
	return DefaultWidget->GetEditorIcon();
}

#undef LOCTEXT_NAMESPACE
