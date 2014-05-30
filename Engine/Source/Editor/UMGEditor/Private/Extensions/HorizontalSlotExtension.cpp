// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "UMG"

FHorizontalSlotExtension::FHorizontalSlotExtension()
{
	ExtensionId = FName(TEXT("HorizontalSlot"));
}

bool FHorizontalSlotExtension::IsActive(const TArray< FSelectedWidget >& Selection)
{
	for ( const FSelectedWidget& Widget : Selection )
	{
		if ( !Widget.Template->Slot || !Widget.Template->Slot->IsA(UHorizontalBoxSlot::StaticClass()) )
		{
			return false;
		}
	}

	return Selection.Num() == 1;
}

void FHorizontalSlotExtension::BuildWidgetsForSelection(const TArray< FSelectedWidget >& Selection, TArray< TSharedRef<SWidget> >& Widgets)
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
	for ( FSelectedWidget& Selection : SelectionCache )
	{
		ShiftHorizontal(Selection.Preview, ShiftAmount);
		ShiftHorizontal(Selection.Template, ShiftAmount);
	}

	//TODO UMG Reorder the live slot without rebuilding the structure
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	return FReply::Handled();
}

void FHorizontalSlotExtension::ShiftHorizontal(UWidget* Widget, int32 ShiftAmount)
{
	UHorizontalBoxSlot* Slot = Cast<UHorizontalBoxSlot>(Widget->Slot);
	UHorizontalBoxComponent* Parent = Cast<UHorizontalBoxComponent>(Slot->Parent);

	int32 CurrentIndex = Parent->GetChildIndex(Widget);
	Parent->Slots.RemoveAt(CurrentIndex);
	Parent->Slots.Insert(Slot, FMath::Clamp(CurrentIndex + ShiftAmount, 0, Parent->GetChildrenCount()));
}

#undef LOCTEXT_NAMESPACE
