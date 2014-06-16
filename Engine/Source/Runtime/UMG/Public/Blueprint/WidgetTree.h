// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WidgetTree.generated.h"

/** The widget tree manages the collection of widgets in a blueprint widget. */
UCLASS()
class UMG_API UWidgetTree : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	/** Array of templates for widgets */
	UPROPERTY()
	TArray<UWidget*> WidgetTemplates;

public:

	void RenameWidget(UWidget* Widget, FString& NewName);

	UWidget* FindWidget(FString& Name) const;

	bool RemoveWidget(UWidget* Widget);

	class UPanelWidget* FindWidgetParent(UWidget* Widget, int32& OutChildIndex);

	/** Constructs the widget, and adds it to the tree. */
	template< class T >
	T* ConstructWidget(TSubclassOf<UWidget> WidgetType)
	{
		Modify();

		UWidget* Widget = (UWidget*)ConstructObject<UWidget>(WidgetType, this);
		Widget->SetFlags(RF_Transactional);
		WidgetTemplates.Add(Widget);

		return (T*)Widget;
	}
	
private:
	bool RemoveWidgetRecursive(UWidget* InRemovedWidget);
};
