// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "WidgetTemplateCheckBox.h"

#define LOCTEXT_NAMESPACE "UMGEditor"

FWidgetTemplateCheckBox::FWidgetTemplateCheckBox()
{
	Name = LOCTEXT("CheckBoxLabel", "CheckBox & Label");
	ToolTip = LOCTEXT("CheckBox_Tooltip", "CheckBox with Text");
	//Icon = 
}

FText FWidgetTemplateCheckBox::GetCategory()
{
	return LOCTEXT("Preset", "Preset");
}

UWidget* FWidgetTemplateCheckBox::Create(UWidgetTree* Tree)
{
	UHorizontalBox* Horizontal = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UCheckBox* Checkbox = Tree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
	UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Text->Text = LOCTEXT("CheckboxText", "Checkbox Text");
	Text->Font.Size = 10;

	Horizontal->AddSlot(Checkbox)
		->Size = FSlateChildSize(ESlateSizeRule::Automatic);
	Horizontal->AddSlot(Text);

	return Horizontal;
}

#undef LOCTEXT_NAMESPACE
