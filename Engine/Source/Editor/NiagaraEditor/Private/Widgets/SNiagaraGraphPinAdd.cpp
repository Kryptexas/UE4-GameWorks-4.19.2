// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SNiagaraGraphPinAdd.h"
#include "NiagaraNodeWithDynamicPins.h"

#include "ScopedTransaction.h"
#include "SComboButton.h"
#include "SBoxPanel.h"
#include "SImage.h"
#include "MultiBoxBuilder.h"

#define LOCTEXT_NAMESPACE "NiagaraGraphPinAdd"

void SNiagaraGraphPinAdd::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	SetShowLabel(false);
	OwningNode = Cast<UNiagaraNodeWithDynamicPins>(InGraphPinObj->GetOwningNode());
	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
	TSharedPtr<SHorizontalBox> PinBox = GetFullPinHorizontalRowWidget().Pin();
	if (PinBox.IsValid())
	{
		if (InGraphPinObj->Direction == EGPD_Input)
		{
			PinBox->AddSlot()
			[
				ConstructAddButton()
			];
		}
		else
		{
			PinBox->InsertSlot(0)
			[
				ConstructAddButton()
			];
		}
	}
}

TSharedRef<SWidget>	SNiagaraGraphPinAdd::ConstructAddButton()
{
	return SNew(SComboButton)
		.HasDownArrow(false)
		.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
		.ForegroundColor(FSlateColor::UseForeground())
		.OnGetMenuContent(this, &SNiagaraGraphPinAdd::OnGetAddButtonMenuContent)
		.ContentPadding(FMargin(2))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.ToolTipText(LOCTEXT("AddPinButtonToolTip", "Connect this pin to add a new typed pin, or choose from the drop-down."))
		.ButtonContent()
		[
			SNew(SImage)
			.ColorAndOpacity(FSlateColor::UseForeground())
			.Image(FEditorStyle::GetBrush("Plus"))
		];
}

TSharedRef<SWidget> SNiagaraGraphPinAdd::OnGetAddButtonMenuContent()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	for (const FNiagaraTypeDefinition& RegisteredType : FNiagaraTypeRegistry::GetRegisteredTypes())
	{
		if (RegisteredType != FNiagaraTypeDefinition::GetGenericNumericDef() && RegisteredType.GetScriptStruct() != nullptr)
		{
			MenuBuilder.AddMenuEntry(
				RegisteredType.GetStruct()->GetDisplayNameText(),
				FText::Format(LOCTEXT("AddButtonTypeEntryToolTipFormat", "Add a new {0} pin"), RegisteredType.GetStruct()->GetDisplayNameText()),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SNiagaraGraphPinAdd::OnAddType, RegisteredType)));
		}
	}

	return MenuBuilder.MakeWidget();
}

void SNiagaraGraphPinAdd::OnAddType(FNiagaraTypeDefinition Type)
{
	if (OwningNode != nullptr)
	{
		FScopedTransaction AddNewPinTransaction(LOCTEXT("AddNewPinTransaction", "Add pin to convert node"));
		OwningNode->RequestNewTypedPin(GetPinObj()->Direction, Type);
	}
}

#undef LOCTEXT_NAMESPACE
