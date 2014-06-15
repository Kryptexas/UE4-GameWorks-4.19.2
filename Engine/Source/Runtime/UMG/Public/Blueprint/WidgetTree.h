// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WidgetTree.generated.h"

/** The widget tree manages the collection of widgets in a blueprint widget. */
UCLASS()
class UMG_API UWidgetTree : public UObject
{
	GENERATED_UCLASS_BODY()

	/** Array of templates for widgets */
	UPROPERTY()
	TArray<class UWidget*> WidgetTemplates;

	void RenameWidget(UWidget* Widget, FString& NewName);

	class UWidget* FindWidget(FString& Name) const;

	bool RemoveWidget(class UWidget* Widget);

	class UPanelWidget* FindWidgetParent(class UWidget* Widget, int32& OutChildIndex);

	/** Constructs the widget, and adds it to the tree. */
	template< class T >
	T* ConstructWidget(TSubclassOf<class UWidget> WidgetType)
	{
		UWidget* Widget = (UWidget*)ConstructObject<UWidget>(WidgetType, this);
		Widget->SetFlags(RF_Transactional);
		WidgetTemplates.Add(Widget);

		return (T*)Widget;
	}
	
private:
	bool RemoveWidgetRecursive(UWidget* InRemovedWidget);
	
};
