// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UWidgetTree

UWidgetTree::UWidgetTree(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UWidgetTree::RenameWidget(USlateWrapperComponent* Widget, FString& NewName)
{
	Widget->Rename(*NewName);

	// TODO Update nodes in the blueprint!
}

USlateWrapperComponent* UWidgetTree::FindWidget(FString& Name) const
{
	FString ExistingName;
	for ( USlateWrapperComponent* Widget : WidgetTemplates )
	{
		Widget->GetName(ExistingName);
		if ( ExistingName.Equals(Name, ESearchCase::IgnoreCase) )
		{
			return Widget;
		}
	}

	return NULL;
}

USlateNonLeafWidgetComponent* UWidgetTree::FindWidgetParent(USlateWrapperComponent* Widget, int32& OutChildIndex)
{
	for ( USlateWrapperComponent* Template : WidgetTemplates )
	{
		USlateNonLeafWidgetComponent* NonLeafTemplate = Cast<USlateNonLeafWidgetComponent>(Template);
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

bool UWidgetTree::RemoveWidget(USlateWrapperComponent* InRemovedWidget)
{
	USlateWrapperComponent* Parent = NULL;

	bool bRemoved = false;

	//TODO UMG Make the Widget Tree actually a tree, instead of a list, it makes things like removal difficult.

	if ( USlateNonLeafWidgetComponent* InNonLeafRemovedWidget = Cast<USlateNonLeafWidgetComponent>(InRemovedWidget) )
	{
		while ( InNonLeafRemovedWidget->GetChildrenCount() > 0 )
		{
			RemoveWidget(InNonLeafRemovedWidget->GetChildAt(0));
		}
	}

	for ( USlateWrapperComponent* Template : WidgetTemplates )
	{
		USlateNonLeafWidgetComponent* NonLeafTemplate = Cast<USlateNonLeafWidgetComponent>(Template);
		if ( NonLeafTemplate )
		{
			if ( NonLeafTemplate->RemoveChild(InRemovedWidget) )
			{
				bRemoved = true;
				break;
			}
		}
	}

	if ( bRemoved )
	{
		WidgetTemplates.Remove(InRemovedWidget);
	}

	return bRemoved;
}