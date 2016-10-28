// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

class IDetailsView;

class FMovieSceneEventParametersCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader( TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils ) override;
	virtual void CustomizeChildren( TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils ) override;

private:
	void OnStructChanged(const FAssetData& AssetData);
	void OnEditStructContentsChanged();

	TSharedPtr<IPropertyUtilities> PropertyUtilities;
	TSharedPtr<IPropertyHandle> PropertyHandle;

	TSharedPtr<IStructureDetailsView> EventPayloadDetails;
	TSharedPtr<FStructOnScope> EditStructData;
};