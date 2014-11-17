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

bool UWidgetTree::RemoveWidget(USlateWrapperComponent* Widget)
{
	USlateWrapperComponent* Parent = NULL;

	bool bRemoved = false;

	for ( USlateWrapperComponent* Template : WidgetTemplates )
	{
		USlateNonLeafWidgetComponent* NonLeafTemplate = Cast<USlateNonLeafWidgetComponent>(Template);
		if ( NonLeafTemplate )
		{
			if ( NonLeafTemplate->RemoveChild(Widget) )
			{
				bRemoved = true;
				break;
			}
		}
	}

	if ( bRemoved )
	{
		WidgetTemplates.Remove(Widget);
	}

	return bRemoved;
}