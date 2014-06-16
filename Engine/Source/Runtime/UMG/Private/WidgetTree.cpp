// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UWidgetTree

UWidgetTree::UWidgetTree(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UWidgetTree::RenameWidget(UWidget* Widget, FString& NewName)
{
	Widget->Modify();
	Widget->Rename(*NewName);

	// TODO UMG Update nodes in the blueprint!
}

UWidget* UWidgetTree::FindWidget(FString& Name) const
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
	
	if ( UPanelWidget* InNonLeafRemovedWidget = Cast<UPanelWidget>(InRemovedWidget) )
	{
		while ( InNonLeafRemovedWidget->GetChildrenCount() > 0 )
		{
			RemoveWidget(InNonLeafRemovedWidget->GetChildAt(0));
		}
	}
	
	for ( UWidget* Template : WidgetTemplates )
	{
		UPanelWidget* NonLeafTemplate = Cast<UPanelWidget>(Template);
		if ( NonLeafTemplate )
		{
			NonLeafTemplate->Modify();
			if ( NonLeafTemplate->RemoveChild(InRemovedWidget) )
			{
				return true;
			}
		}
	}
	
	return false;
}

bool UWidgetTree::RemoveWidget(UWidget* InRemovedWidget)
{
	Modify();

	bool bRemoved = RemoveWidgetRecursive(InRemovedWidget);

	int32 IndexRemoved = WidgetTemplates.Remove(InRemovedWidget);
	
	return bRemoved || IndexRemoved != -1;
}
