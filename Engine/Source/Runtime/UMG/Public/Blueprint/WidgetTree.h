// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WidgetTree.generated.h"

UCLASS()
class UMG_API UWidgetTree : public UObject
{
	GENERATED_UCLASS_BODY()

	/** Array of templates for widgets */
	UPROPERTY()
	TArray<class USlateWrapperComponent*> WidgetTemplates;

	void RenameWidget(USlateWrapperComponent* Widget, FString& NewName);

	class USlateWrapperComponent* FindWidget(FString& Name) const;

	bool RemoveWidget(class USlateWrapperComponent* Widget);

	class USlateNonLeafWidgetComponent* FindWidgetParent(class USlateWrapperComponent* Widget, int32& OutChildIndex);

	template< class T >
	T* ConstructWidget(TSubclassOf<class USlateWrapperComponent> WidgetType)
	{
		USlateWrapperComponent* Widget = (USlateWrapperComponent*)ConstructObject<class USlateWrapperComponent>(WidgetType, this);
		WidgetTemplates.Add(Widget);

		return (T*)Widget;
	}
};
