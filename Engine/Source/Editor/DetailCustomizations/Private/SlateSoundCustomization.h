// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** Customize the appearance of an FSlateSound */
class FSlateSoundStructCustomization : public IStructCustomization
{
public:
	static TSharedRef<IStructCustomization> MakeInstance();

	/** IStructCustomization interface */
	virtual void CustomizeStructHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils) OVERRIDE;
	virtual void CustomizeStructChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils) OVERRIDE;

protected:
	/** Called when the resource object used by this FSlateSound has been changed */
	void OnObjectChanged(const UObject*);

	/** Array of FSlateSound instances this customization is currently editing */
	TArray<FSlateSound*> SlateSoundStructs;
};
