// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "UMG"

FHorizontalSlotExtension::FHorizontalSlotExtension()
{
	ExtensionId = FName(TEXT("HorizontalSlot"));
}

bool FHorizontalSlotExtension::IsActive(const TArray< FWidgetReference >& Selection)
{
	for ( const FWidgetReference& Widget : Selection )
	{
		if ( !Widget.GetTemplate()->Slot || !Widget.GetTemplate()->Slot->IsA(UHorizontalBoxSlot::StaticClass()) )
		{
			return false;
		}
	}

	return Selection.Num() == 1;
}

void FHorizontalSlotExtension::BuildWidgetsForSelection(const TArray< FWidgetReference >& Selection, TArray< TSharedRef<SWidget> >& Widgets)
{
	SelectionCache = Selection;

	if ( !IsActive(Selection) )
	{
		return;
	}

	TSharedRef<SButton> LeftButton =
		SNew(SButton)
		.Text(LOCTEXT("LeftArrow", "←"))
		.OnClicked(this, &FHorizontalSlotExtension::HandleShift, -1);

	TSharedRef<SButton> RightButton =
		SNew(SButton)
		.Text(LOCTEXT("RightArrow", "→"))
		.OnClicked(this, &FHorizontalSlotExtension::HandleShift, 1);

	Widgets.Add(LeftButton);
	Widgets.Add(RightButton);
}

FReply FHorizontalSlotExtension::HandleShift(int32 ShiftAmount)
{
	BeginTransaction(LOCTEXT("MoveWidget", "Move Widget"));

	for ( FWidgetReference& Selection : SelectionCache )
	{
		ShiftHorizontal(Selection.GetPreview(), ShiftAmount);
		ShiftHorizontal(Selection.GetTemplate(), ShiftAmount);
	}

	EndTransaction();

	//TODO UMG Reorder the live slot without rebuilding the structure
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	return FReply::Handled();
}

void FHorizontalSlotExtension::ShiftHorizontal(UWidget* Widget, int32 ShiftAmount)
{
	UHorizontalBoxSlot* Slot = CastChecked<UHorizontalBoxSlot>(Widget->Slot);
	UHorizontalBox* Parent = CastChecked<UHorizontalBox>(Slot->Parent);

	int32 CurrentIndex = Parent->GetChildIndex(Widget);
	Parent->Slots.RemoveAt(CurrentIndex);
	Parent->Slots.Insert(Slot, FMath::Clamp(CurrentIndex + ShiftAmount, 0, Parent->GetChildrenCount()));
}

#undef LOCTEXT_NAMESPACE
