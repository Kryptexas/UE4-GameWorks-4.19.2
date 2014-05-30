// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

FUniformGridSlotExtension::FUniformGridSlotExtension()
{
	ExtensionId = FName(TEXT("UniformGridSlot"));
}

bool FUniformGridSlotExtension::IsActive(const TArray< FSelectedWidget >& Selection)
{
	for ( const FSelectedWidget& Widget : Selection )
	{
		if ( !Widget.Template->Slot || !Widget.Template->Slot->IsA(UUniformGridSlot::StaticClass()) )
		{
			return false;
		}
	}

	return Selection.Num() == 1;
}

void FUniformGridSlotExtension::BuildWidgetsForSelection(const TArray< FSelectedWidget >& Selection, TArray< TSharedRef<SWidget> >& Widgets)
{
	SelectionCache = Selection;

	if ( !IsActive(Selection) )
	{
		return;
	}

	TSharedRef<SButton> UpButton =
		SNew(SButton)
		.Text(LOCTEXT("UpArrow", "↑"))
		.OnClicked(this, &FUniformGridSlotExtension::HandleShiftRow, -1);

	TSharedRef<SButton> DownButton =
		SNew(SButton)
		.Text(LOCTEXT("DownArrow", "↓"))
		.OnClicked(this, &FUniformGridSlotExtension::HandleShiftRow, 1);

	TSharedRef<SButton> LeftButton =
		SNew(SButton)
		.Text(LOCTEXT("LeftArrow", "←"))
		.OnClicked(this, &FUniformGridSlotExtension::HandleShiftColumn, -1);

	TSharedRef<SButton> RightButton =
		SNew(SButton)
		.Text(LOCTEXT("RightArrow", "→"))
		.OnClicked(this, &FUniformGridSlotExtension::HandleShiftColumn, 1);

	Widgets.Add(UpButton);
	Widgets.Add(DownButton);
	Widgets.Add(LeftButton);
	Widgets.Add(RightButton);
}

FReply FUniformGridSlotExtension::HandleShiftRow(int32 ShiftAmount)
{
	for ( FSelectedWidget& Selection : SelectionCache )
	{
		ShiftRow(Selection.Preview, ShiftAmount);
		ShiftRow(Selection.Template, ShiftAmount);
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	return FReply::Handled();
}

FReply FUniformGridSlotExtension::HandleShiftColumn(int32 ShiftAmount)
{
	for ( FSelectedWidget& Selection : SelectionCache )
	{
		ShiftColumn(Selection.Preview, ShiftAmount);
		ShiftColumn(Selection.Template, ShiftAmount);
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	return FReply::Handled();
}

void FUniformGridSlotExtension::ShiftRow(UWidget* Widget, int32 ShiftAmount)
{
	UUniformGridSlot* Slot = Cast<UUniformGridSlot>(Widget->Slot);
	Slot->SetRow(FMath::Max(Slot->Row + ShiftAmount, 0));
}

void FUniformGridSlotExtension::ShiftColumn(UWidget* Widget, int32 ShiftAmount)
{
	UUniformGridSlot* Slot = Cast<UUniformGridSlot>(Widget->Slot);
	Slot->SetColumn(FMath::Max(Slot->Column + ShiftAmount, 0));
}

#undef LOCTEXT_NAMESPACE
