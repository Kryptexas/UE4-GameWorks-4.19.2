// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UWidgetTree

UWidgetTree::UWidgetTree(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

UWidget* UWidgetTree::FindWidget(const FString& Name) const
{
	FString ExistingName;

	// TODO UMG Hacky, remove this find widget function, or make it faster.
	TArray<UWidget*> Widgets;
	GetAllWidgets(Widgets);

	for ( UWidget* Widget : Widgets )
	{
		Widget->GetName(ExistingName);
		if ( ExistingName.Equals(Name, ESearchCase::IgnoreCase) )
		{
			return Widget;
		}
	}

	return NULL;
}

UPanelWidget* UWidgetTree::FindWidgetParent(UWidget* Widget, int32& OutChildIndex)
{
	UPanelWidget* Parent = Widget->GetParent();
	if ( Parent != NULL )
	{
		OutChildIndex = Parent->GetChildIndex(Widget);
	}
	else
	{
		OutChildIndex = 0;
	}

	return Parent;
}

bool UWidgetTree::RemoveWidget(UWidget* InRemovedWidget, bool bIsRecursive)
{
	InRemovedWidget->Modify();

	bool bRemoved = false;

	UPanelWidget* InRemovedWidgetParent = InRemovedWidget->GetParent();
	if ( InRemovedWidgetParent )
	{
		InRemovedWidgetParent->Modify();

		if ( InRemovedWidgetParent->RemoveChild(InRemovedWidget) )
		{
			bRemoved = true;
		}
	}

	if ( bIsRecursive )
	{
		if ( UPanelWidget* InNonLeafRemovedWidget = Cast<UPanelWidget>(InRemovedWidget) )
		{
			while ( InNonLeafRemovedWidget->GetChildrenCount() > 0 )
			{
				RemoveWidget(InNonLeafRemovedWidget->GetChildAt(0));
			}
		}
	}

	return bRemoved;
}

void UWidgetTree::GetAllWidgets(TArray<UWidget*>& Widgets) const
{
	if ( RootWidget )
	{
		Widgets.Add(RootWidget);
		GetChildWidgets(RootWidget, Widgets);
	}
}

void UWidgetTree::GetChildWidgets(UWidget* Parent, TArray<UWidget*>& Widgets) const
{
	if ( UPanelWidget* PanelParent = Cast<UPanelWidget>(Parent) )
	{
		for ( int32 ChildIndex = 0; ChildIndex < PanelParent->GetChildrenCount(); ChildIndex++ )
		{
			UWidget* ChildWidget = PanelParent->GetChildAt(ChildIndex);
			Widgets.Add(ChildWidget);

			GetChildWidgets(ChildWidget, Widgets);
		}
	}
}
