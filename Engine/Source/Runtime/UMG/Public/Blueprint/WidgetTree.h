// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WidgetTree.generated.h"

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

	template< class T >
	T* ConstructWidget(TSubclassOf<class UWidget> WidgetType)
	{
		UWidget* Widget = (UWidget*)ConstructObject<class UWidget>(WidgetType, this);
		WidgetTemplates.Add(Widget);

		return (T*)Widget;
	}
};
