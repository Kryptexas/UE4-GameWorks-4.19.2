// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "UMG"

FVerticalSlotExtension::FVerticalSlotExtension()
{
	ExtensionId = FName(TEXT("VerticalSlot"));
}

bool FVerticalSlotExtension::IsActive(const TArray< UWidget* >& Selection)
{
	for ( UWidget* Widget : Selection )
	{
		if ( !Widget->Slot || !Widget->Slot->IsA(UVerticalBoxSlot::StaticClass()) )
		{
			return false;
		}
	}

	return Selection.Num() == 1;
}

void FVerticalSlotExtension::BuildWidgetsForSelection(const TArray< UWidget* >& Selection, TArray< TSharedRef<SWidget> >& Widgets)
{
	SelectionCache = Selection;

	if ( !IsActive(Selection) )
	{
		return;
	}

	TSharedRef<SButton> UpButton =
		SNew(SButton)
		.Text(LOCTEXT("UpArrow", "?"))
		.OnClicked(this, &FVerticalSlotExtension::HandleUpPressed);

	TSharedRef<SButton> DownButton =
		SNew(SButton)
		.Text(LOCTEXT("DownArrow", "?"))
		.OnClicked(this, &FVerticalSlotExtension::HandleDownPressed);

	Widgets.Add(UpButton);
	Widgets.Add(DownButton);
}

FReply FVerticalSlotExtension::HandleUpPressed()
{
	for ( UWidget* Widget : SelectionCache )
	{
		UVerticalBoxSlot* VerticalSlot = Cast<UVerticalBoxSlot>(Widget->Slot);
		UVerticalBoxComponent* Parent = Cast<UVerticalBoxComponent>(VerticalSlot->Parent);

		int32 CurrentIndex = Parent->GetChildIndex(Widget);
		Parent->Slots.RemoveAt(CurrentIndex);
		Parent->Slots.Insert(VerticalSlot, FMath::Max(CurrentIndex - 1, 0));
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	return FReply::Handled();
}

FReply FVerticalSlotExtension::HandleDownPressed()
{
	for ( UWidget* Widget : SelectionCache )
	{
		UVerticalBoxSlot* VerticalSlot = Cast<UVerticalBoxSlot>(Widget->Slot);
		UVerticalBoxComponent* Parent = Cast<UVerticalBoxComponent>(VerticalSlot->Parent);

		int32 CurrentIndex = Parent->GetChildIndex(Widget);
		Parent->Slots.RemoveAt(CurrentIndex);
		Parent->Slots.Insert(VerticalSlot, FMath::Min(CurrentIndex + 1, Parent->GetChildrenCount()));
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
