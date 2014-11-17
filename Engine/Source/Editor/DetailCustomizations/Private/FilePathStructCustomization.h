// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** Delegate fired when a path is picked */
DECLARE_DELEGATE_RetVal_OneParam(bool, FOnPathPicked, FString& /* InOutPath */);

class FFilePathStructCustomization : public IStructCustomization
{
public:
	static TSharedRef<IStructCustomization> MakeInstance();

	/** IStructCustomization interface */
	virtual void CustomizeStructHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;

	virtual void CustomizeStructChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;              

	static TSharedPtr<SWidget> CreatePickerWidget(TSharedRef<class IPropertyHandle> StructPropertyHandle,  const FString& FileFilterExtension = FString(TEXT("*")), const FOnPathPicked& OnPathPicked = FOnPathPicked());

private:

	/** Delegate for displaying text value of file */
	FText GetDisplayedText(TSharedRef<IPropertyHandle> PropertyHandle) const;

	/** Delegate used to display a file picker */
	static FReply OnPickFile(TSharedRef<IPropertyHandle> PropertyHandle);
};