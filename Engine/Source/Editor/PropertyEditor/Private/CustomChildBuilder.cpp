// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PropertyEditorPrivatePCH.h"
#include "CustomChildBuilder.h"
#include "DetailGroup.h"
#include "PropertyHandleImpl.h"
#include "DetailPropertyRow.h"

IDetailChildrenBuilder& FCustomChildrenBuilder::AddChildCustomBuilder( TSharedRef<class IDetailCustomNodeBuilder> InCustomBuilder )
{
	FDetailLayoutCustomization NewCustomization;
	NewCustomization.CustomBuilderRow = MakeShareable( new FDetailCustomBuilderRow( InCustomBuilder ) );

	ChildCustomizations.Add( NewCustomization );
	return *this;
}

IDetailGroup& FCustomChildrenBuilder::AddChildGroup( FName GroupName, const FString& LocalizedDisplayName )
{
	FDetailLayoutCustomization NewCustomization;
	NewCustomization.DetailGroup = MakeShareable( new FDetailGroup( GroupName, ParentCategory.Pin().ToSharedRef(), LocalizedDisplayName ) );

	ChildCustomizations.Add( NewCustomization );

	return *NewCustomization.DetailGroup;
}

FDetailWidgetRow& FCustomChildrenBuilder::AddChildContent( const FString& SearchString )
{
	TSharedRef<FDetailWidgetRow> NewRow = MakeShareable( new FDetailWidgetRow );
	FDetailLayoutCustomization NewCustomization;

	NewRow->FilterString( SearchString );

	NewCustomization.WidgetDecl = NewRow;

	ChildCustomizations.Add( NewCustomization );
	return *NewRow;
}

IDetailPropertyRow& FCustomChildrenBuilder::AddChildProperty( TSharedRef<IPropertyHandle> PropertyHandle )
{
	check( PropertyHandle->IsValidHandle() )

	FDetailLayoutCustomization NewCustomization;
	NewCustomization.PropertyRow = MakeShareable( new FDetailPropertyRow( StaticCastSharedRef<FPropertyHandleBase>( PropertyHandle )->GetPropertyNode(), ParentCategory.Pin().ToSharedRef() ) );

	ChildCustomizations.Add( NewCustomization );

	return *NewCustomization.PropertyRow;
}

class SStandaloneCustomStructValue : public SCompoundWidget, public IStructCustomizationUtils
{
public:
	SLATE_BEGIN_ARGS( SStandaloneCustomStructValue )
	{}
	SLATE_END_ARGS()
	
	void Construct( const FArguments& InArgs, TSharedPtr<IStructCustomization> InCustomizationInterface, TSharedRef<IPropertyHandle> InStructPropertyHandle, TSharedRef<FDetailCategoryImpl> InParentCategory )
	{
		CustomizationInterface = InCustomizationInterface;
		StructPropertyHandle = InStructPropertyHandle;
		ParentCategory = InParentCategory;
		CustomPropertyWidget = MakeShareable(new FDetailWidgetRow);

		CustomizationInterface->CustomizeStructHeader(InStructPropertyHandle, *CustomPropertyWidget, *this);

		ChildSlot
		[
			CustomPropertyWidget->ValueWidget.Widget
		];
	}

	virtual TSharedPtr<FAssetThumbnailPool> GetThumbnailPool() const OVERRIDE
	{
		TSharedPtr<FDetailCategoryImpl> ParentCategoryPinned = ParentCategory.Pin();
		return ParentCategoryPinned.IsValid() ? ParentCategoryPinned->GetParentLayout().GetThumbnailPool() : NULL;
	}

private:
	TWeakPtr<FDetailCategoryImpl> ParentCategory;
	TSharedPtr<IStructCustomization> CustomizationInterface;
	TSharedPtr<IPropertyHandle> StructPropertyHandle;
	TSharedPtr<FDetailWidgetRow> CustomPropertyWidget;
};


TSharedRef<SWidget> FCustomChildrenBuilder::GenerateStructValueWidget( TSharedRef<IPropertyHandle> StructPropertyHandle )
{
	UStructProperty* StructProperty = CastChecked<UStructProperty>( StructPropertyHandle->GetProperty() );

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FOnGetStructCustomizationInstance StructLayoutInstanceDelegate = PropertyEditorModule.GetStructCustomizaton(StructProperty->Struct->GetFName());
	if (StructLayoutInstanceDelegate.IsBound())
	{
		TSharedRef<IStructCustomization> CustomStructInterface = StructLayoutInstanceDelegate.Execute();

		return SNew( SStandaloneCustomStructValue, CustomStructInterface, StructPropertyHandle, ParentCategory.Pin().ToSharedRef() );
	}
	else
	{
		// Uncustomized structs have nothing for their value content
		return SNullWidget::NullWidget;
	}
}

