// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** Delegate fired when a path is picked */
DECLARE_DELEGATE_RetVal_OneParam(bool, FOnPathPicked, FString& /* InOutPath */);

class FFilePathStructCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils ) OVERRIDE;

	virtual void CustomizeChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils ) OVERRIDE;              

	static TSharedPtr<SWidget> CreatePickerWidget(TSharedRef<class IPropertyHandle> StructPropertyHandle,  const FString& FileFilterExtension = FString(TEXT("*")), const FOnPathPicked& OnPathPicked = FOnPathPicked());

private:

	/** Delegate for displaying text value of file */
	FText GetDisplayedText(TSharedRef<IPropertyHandle> PropertyHandle) const;

	/** Delegate used to display a file picker */
	static FReply OnPickFile(TSharedRef<IPropertyHandle> PropertyHandle);
};