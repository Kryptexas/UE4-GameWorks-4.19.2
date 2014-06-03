// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "WidgetTemplateButton.h"

#define LOCTEXT_NAMESPACE "UMGEditor"

FWidgetTemplateButton::FWidgetTemplateButton()
{
	Name = LOCTEXT("Button", "Button");
	ToolTip = LOCTEXT("Button_Tooltip", "Button With Text");
	//Icon = 
}

FText FWidgetTemplateButton::GetCategory()
{
	return LOCTEXT("Preset", "Preset");
}

UWidget* FWidgetTemplateButton::Create(UWidgetTree* Tree)
{
	UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());

	Text->Text = FText::FromString(TEXT("Button Text"));
	Text->Font.Size = 10;
	Button->SetContent(Text);

	return Button;
}

#undef LOCTEXT_NAMESPACE
