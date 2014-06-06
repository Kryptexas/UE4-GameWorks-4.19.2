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
		.OnClicked(this, &FVerticalSlotExtension::HandleUpPressed);

	TSharedRef<SButton> DownButton =
		SNew(SButton)
		.Text(LOCTEXT("DownArrow", "↓"))
		.OnClicked(this, &FVerticalSlotExtension::HandleDownPressed);

	Widgets.Add(UpButton);
	Widgets.Add(DownButton);
}

FReply FVerticalSlotExtension::HandleUpPressed()
{
	for ( FSelectedWidget& Selection : SelectionCache )
	{
		MoveUp(Selection.GetPreview());
		MoveUp(Selection.GetTemplate());
	}

	//TODO UMG Reorder the live slot without rebuilding the structure
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	return FReply::Handled();
}

FReply FVerticalSlotExtension::HandleDownPressed()
{
	for ( FSelectedWidget& Selection : SelectionCache )
	{
		MoveDown(Selection.GetPreview());
		MoveDown(Selection.GetTemplate());
	}

	//TODO UMG Reorder the live slot without rebuilding the structure
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	return FReply::Handled();
}

void FVerticalSlotExtension::MoveUp(UWidget* Widget)
{
	UVerticalBoxSlot* VerticalSlot = Cast<UVerticalBoxSlot>(Widget->Slot);
	UVerticalBox* Parent = Cast<UVerticalBox>(VerticalSlot->Parent);

	int32 CurrentIndex = Parent->GetChildIndex(Widget);
	Parent->Slots.RemoveAt(CurrentIndex);
	Parent->Slots.Insert(VerticalSlot, FMath::Max(CurrentIndex - 1, 0));
}

void FVerticalSlotExtension::MoveDown(UWidget* Widget)
{
	UVerticalBoxSlot* VerticalSlot = Cast<UVerticalBoxSlot>(Widget->Slot);
	UVerticalBox* Parent = Cast<UVerticalBox>(VerticalSlot->Parent);

	int32 CurrentIndex = Parent->GetChildIndex(Widget);
	Parent->Slots.RemoveAt(CurrentIndex);
	Parent->Slots.Insert(VerticalSlot, FMath::Min(CurrentIndex + 1, Parent->GetChildrenCount()));
}

#undef LOCTEXT_NAMESPACE
