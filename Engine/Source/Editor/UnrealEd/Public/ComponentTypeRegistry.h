// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SComponentClassCombo.h"

DECLARE_MULTICAST_DELEGATE(FOnComponentTypeListChanged);

typedef TSharedPtr<class FComponentClassComboEntry> FComponentClassComboEntryPtr;

struct FComponentTypeRegistry
{
	static FComponentTypeRegistry& Get();

	/**
	* Called when the user changes the text in the search box.
	* @OutComponentList Pointer that will be set to the (globally shared) component type list
	* @return Deleate that can be used to handle change notifications. change notifications are raised when entries are 
	*	added or removed from the component type list
	*/
	FOnComponentTypeListChanged& SubscribeToComponentList(TArray<FComponentClassComboEntryPtr>*& OutComponentList);
	FOnComponentTypeListChanged& GetOnComponentTypeListChanged();

private:
	FComponentTypeRegistry();
	struct FComponentTypeRegistryData* Data;
};
