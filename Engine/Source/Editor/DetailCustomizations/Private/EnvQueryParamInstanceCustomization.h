// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FEnvQueryParamInstanceCustomization : public IStructCustomization
{
public:

	// Begin IStructCustomization interface
	virtual void CustomizeStructHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;
	virtual void CustomizeStructChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;
	// End IStructCustomization interface

	static TSharedRef<IStructCustomization> MakeInstance( );

protected:

	TSharedPtr<IPropertyHandle> NameProp;
	TSharedPtr<IPropertyHandle> ValueProp;
	TSharedPtr<IPropertyHandle> TypeProp;

	TOptional<float> GetParamNumValue() const;
	void OnParamNumValueChanged(float FloatValue) const;
	EVisibility GetParamNumValueVisibility() const;

	ESlateCheckBoxState::Type GetParamBoolValue() const;
	void OnParamBoolValueChanged(ESlateCheckBoxState::Type BoolValue) const;
	EVisibility GetParamBoolValueVisibility() const;

	FString GetHeaderDesc() const;
	void OnTypeChanged();
	void InitCachedTypes();

	EEnvQueryParam::Type ParamType;
	mutable uint8 CachedBool : 1;
	mutable float CachedFloat;
	mutable int32 CachedInt;
};
