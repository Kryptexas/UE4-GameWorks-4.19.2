// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PropertyEditing.h"
#include "PropertyCustomizationHelpers.h"

class FEnvQueryParamSetupCustomization : public IStructCustomization
{
public:

	// Begin IStructCustomization interface
	virtual void CustomizeStructHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;
	virtual void CustomizeStructChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;
	// End IStructCustomization interface

	static TSharedRef<IStructCustomization> MakeInstance( );

protected:

	TSharedPtr<IPropertyHandle> PropertyHandle;
	TSharedPtr<IPropertyHandle> ValueProp;
	TSharedPtr<IPropertyHandle> NameProp;
	TSharedPtr<SEditableTextBox> TextBox;

	FName LastPropName;
	bool bIsNamed;
	EEnvQueryParam::Type ParamType;

	void CacheMyValues();

	TOptional<float> GetParamNumValue() const;
	void OnParamNumValueChanged(float FloatValue) const;
	EVisibility GetParamNumValueVisibility() const;

	ESlateCheckBoxState::Type GetParamBoolValue() const;
	void OnParamBoolValueChanged(ESlateCheckBoxState::Type BoolValue) const;
	EVisibility GetParamBoolValueVisibility() const;

	void OnParamNameCommitted(const FText& ParamName, ETextCommit::Type CommitInfo);
	EVisibility GetParamNameVisibility() const;

	FReply ToggleMode();
	void OnModeChanged();
	FString GetComboText() const;
	FString GetComboTooltip() const;
};
