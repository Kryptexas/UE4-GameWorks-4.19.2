// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "UMG"

FVerticalSlotExtension::FVerticalSlotExtension()
{
	ExtensionId = FName(TEXT("VerticalSlot"));
}

bool FVerticalSlotExtension::IsActive(const TArray< FSelectedWidget >& Selection)
{
	for ( const FSelectedWidget& Widget : Selection )
	{
		if ( !Widget.GetTemplate()->Slot || !Widget.GetTemplate()->Slot->IsA(UVerticalBoxSlot::StaticClass()) )
		{
			return false;
		}
	}

	return Selection.Num() == 1;
}

void FVerticalSlotExtension::BuildWidgetsForSelection(const TArray< FSelectedWidget >& Selection, TArray< TSharedRef<SWidget> >& Widgets)
{
	SelectionCache = Selection;

	if ( !IsActive(Selection) )
	{
		return;
	}

	TSharedRef<SButton> UpButton =
		SNew(SButton)
		.Text(LOCTEXT("UpArrow", "↑"))
		.OnClicked(this, &FVerticalSlotExtension::HandleShiftVertical, -1);

	TSharedRef<SButton> DownButton =
		SNew(SButton)
		.Text(LOCTEXT("DownArrow", "↓"))
		.OnClicked(this, &FVerticalSlotExtension::HandleShiftVertical, 1);

	UpButton->SetEnabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FVerticalSlotExtension::CanShift, -1)));
	DownButton->SetEnabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FVerticalSlotExtension::CanShift, 1)));

	Widgets.Add(UpButton);
	Widgets.Add(DownButton);
}

bool FVerticalSlotExtension::CanShift(int32 ShiftAmount) const
{
	//TODO UMG Provide feedback if shifting is possible.  Tricky with multiple items selected, if we ever support that.
	return true;
}

FReply FVerticalSlotExtension::HandleShiftVertical(int32 ShiftAmount)
{
	BeginTransaction(LOCTEXT("MoveWidget", "Move Widget"));

	for ( FSelectedWidget& Selection : SelectionCache )
	{
		ShiftVertical(Selection.GetPreview(), ShiftAmount);
		ShiftVertical(Selection.GetTemplate(), ShiftAmount);
	}

	//TODO UMG Reorder the live slot without rebuilding the structure
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	return FReply::Handled();
}

void FVerticalSlotExtension::ShiftVertical(UWidget* Widget, int32 ShiftAmount)
{
	UVerticalBoxSlot* Slot = Cast<UVerticalBoxSlot>(Widget->Slot);
	UVerticalBox* Parent = Cast<UVerticalBox>(Slot->Parent);

	int32 CurrentIndex = Parent->GetChildIndex(Widget);
	Parent->Slots.RemoveAt(CurrentIndex);
	Parent->Slots.Insert(Slot, FMath::Clamp(CurrentIndex + ShiftAmount, 0, Parent->GetChildrenCount()));
}

#undef LOCTEXT_NAMESPACE
