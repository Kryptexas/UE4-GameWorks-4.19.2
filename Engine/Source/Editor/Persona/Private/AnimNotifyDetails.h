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

	/** Pointer to the bone name property */
	TSharedPtr<IPropertyHandle> BoneNameProperty;

	/** Get the PropertyHandle corresponding to the socket name variable */
	static TSharedPtr<IPropertyHandle> GetBoneNameProperty( IDetailLayoutBuilder& DetailBuilder );

	/** Handle the committed text */
	void OnSearchBoxCommitted(const FText& InSearchText, ETextCommit::Type CommitInfo);

	/** Get the search suggestions */
	TArray<FString> GetSearchSuggestions() const;
};