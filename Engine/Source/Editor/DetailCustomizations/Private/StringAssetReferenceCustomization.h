// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Customizes a string asset reference to look like a UObject property
 */
class FStringAssetReferenceCustomization : public IStructCustomization
{
public:
	static TSharedRef<IStructCustomization> MakeInstance() 
	{
		return MakeShareable( new FStringAssetReferenceCustomization );
	}

	/** IStructCustomization interface */
	virtual void CustomizeStructHeader( TSharedRef<class IPropertyHandle> InStructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;
	virtual void CustomizeStructChildren( TSharedRef<class IPropertyHandle> InStructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;

private:
	bool OnShouldFilterAsset( const class FAssetData& InAssetData ) const;

private:
	/** Handle to the struct property being customized */
	TSharedPtr<IPropertyHandle> StructPropertyHandle;
	/** Classes that can be used with this property */
	TArray<UClass*> CustomClassFilters;
	/** Whether the classes in the above array must be an exact match, or whether they can also be a derived type; default is false */
	bool bExactClass;
};
