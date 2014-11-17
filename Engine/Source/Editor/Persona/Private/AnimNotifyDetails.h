// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PropertyEditorModule.h"

class FAnimNotifyDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) OVERRIDE;

private:

	/** Array of pointers to the bone name properties */
	TArray<TSharedPtr<IPropertyHandle>> BoneNameProperties;

	/** Adds a Bone Name property to the details layout. */
	void AddBoneNameProperty(IDetailLayoutBuilder& DetailBuilder, const UClass* PropertyClass, const TCHAR* PropertyName, const TCHAR* CategoryName);

	/** Handle the committed text */
	void OnSearchBoxCommitted(const FText& InSearchText, ETextCommit::Type CommitInfo, int32 PropertyIndex);

	/** Get the search suggestions */
	TArray<FString> GetSearchSuggestions() const;
};