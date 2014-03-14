// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FBlackboardEntryDetails : public IStructCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IStructCustomization> MakeInstance();

	/** IStructCustomization interface */
	virtual void CustomizeStructHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;
	virtual void CustomizeStructChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;

private:

	FString GetHeaderDesc() const;
	FString GetShortTypeName(const UObject* Ob) const;
	
	TSharedPtr<IPropertyHandle> MyStructProperty;
	TSharedPtr<IPropertyHandle> MyNameProperty;
	TSharedPtr<IPropertyHandle> MyValueProperty;
};
