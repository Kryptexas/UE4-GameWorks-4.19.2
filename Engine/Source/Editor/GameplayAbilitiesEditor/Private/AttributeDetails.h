// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"

class IPropertyHandle;
class FDetailWidgetRow;
class IDetailLayoutBuilder;

DECLARE_LOG_CATEGORY_EXTERN(LogAttributeDetails, Log, All);

class FAttributeDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

private:
	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) OVERRIDE;

	TSharedPtr<FString> GetPropertyType() const;

	TArray<TSharedPtr<FString>> PropertyOptions;

	TSharedPtr<IPropertyHandle> MyProperty;

	void OnChangeProperty(TSharedPtr<FString> ItemSelected, ESelectInfo::Type SelectInfo);	
};


class FAttributePropertyDetails : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;
	virtual void CustomizeChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;              

private:

	TSharedPtr<IPropertyHandle> MyProperty;
	TArray<TSharedPtr<FString>> PropertyOptions;

	TSharedPtr<FString>	GetPropertyType() const;

	void OnChangeProperty(TSharedPtr<FString> ItemSelected, ESelectInfo::Type SelectInfo);
};

class FScalableFloatDetails : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();


private:

	virtual void CustomizeHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;
	virtual void CustomizeChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils ) OVERRIDE;              

	bool IsEditable( ) const;

	TSharedRef<SWidget> GetListContent();
	void OnSelectionChanged(TSharedPtr<FString> SelectedItem, ESelectInfo::Type SelectInfo);
	FString GetRowNameComboBoxContentText() const;
	TSharedRef<ITableRow> HandleRowNameComboBoxGenarateWidget(TSharedPtr<FString> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void OnFilterTextChanged(const FText& InFilterText);

	class UCurveTable * GetCurveTable();

	TSharedPtr<FString> InitWidgetContent();

	void OnCurveTableChanged();

	TSharedPtr<SComboButton> RowNameComboButton;
	TSharedPtr<SListView<TSharedPtr<FString> > > RowNameComboListView;
	TSharedPtr<FString> CurrentSelectedItem;
	TArray<TSharedPtr<FString> > RowNames;

	TSharedPtr<IPropertyHandle> ValueProperty;
	TSharedPtr<IPropertyHandle> CurveTableHandleProperty;
	TSharedPtr<IPropertyHandle> CurveTableProperty;
	TSharedPtr<IPropertyHandle> RowNameProperty;
};