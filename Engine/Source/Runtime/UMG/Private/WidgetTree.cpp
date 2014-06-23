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
	for ( UWidget* Widget : WidgetTemplates )
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
	for ( UWidget* Template : WidgetTemplates )
	{
		UPanelWidget* NonLeafTemplate = Cast<UPanelWidget>(Template);
		if ( NonLeafTemplate )
		{
			for ( int32 ChildIndex = 0; ChildIndex < NonLeafTemplate->GetChildrenCount(); ChildIndex++ )
			{
				if ( NonLeafTemplate->GetChildAt(ChildIndex) == Widget )
				{
					OutChildIndex = ChildIndex;
					return NonLeafTemplate;
				}
			}
		}
	}

	OutChildIndex = -1;
	return NULL;
}

bool UWidgetTree::RemoveWidgetRecursive(UWidget* InRemovedWidget)
{
	UWidget* Parent = NULL;
	
	//TODO UMG Make the Widget Tree actually a tree, instead of a list, it makes things like removal difficult.
	
	// If the widget being removed is a panel (has children) we need to delete all of its children first.
	if ( UPanelWidget* InNonLeafRemovedWidget = Cast<UPanelWidget>(InRemovedWidget) )
	{
		while ( InNonLeafRemovedWidget->GetChildrenCount() > 0 )
		{
			RemoveWidget(InNonLeafRemovedWidget->GetChildAt(0));
		}
	}

	//TODO UMG This is slow, don't do this.  Need simpler way to manage finding parents

	// Get the parent widget for this widget.
	for ( UWidget* Template : WidgetTemplates )
	{
		UPanelWidget* NonLeafTemplate = Cast<UPanelWidget>(Template);
		if ( NonLeafTemplate )
		{
			for ( int32 ChildIndex = 0; ChildIndex < NonLeafTemplate->GetChildrenCount(); ChildIndex++ )
			{
				if ( NonLeafTemplate->GetChildAt(ChildIndex) == InRemovedWidget )
				{
					NonLeafTemplate->Modify();

					if ( NonLeafTemplate->RemoveChild(InRemovedWidget) )
					{
						return true;
					}
				}
			}
		}
	}
	
	return false;
}

bool UWidgetTree::RemoveWidget(UWidget* InRemovedWidget)
{
	InRemovedWidget->Modify();

	bool bRemoved = RemoveWidgetRecursive(InRemovedWidget);
	int32 IndexRemoved = WidgetTemplates.Remove(InRemovedWidget);
	
	return bRemoved || IndexRemoved != -1;
}
