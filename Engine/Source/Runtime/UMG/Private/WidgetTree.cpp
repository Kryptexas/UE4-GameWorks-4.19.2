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

	int32 IndexRemoved = WidgetTemplates.Remove(InRemovedWidget);
	
	return bRemoved || IndexRemoved != -1;
}
