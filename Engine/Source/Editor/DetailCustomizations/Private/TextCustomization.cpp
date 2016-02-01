// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "TextCustomization.h"
#include "IPropertyUtilities.h"
#include "STextPropertyEditableTextBox.h"

namespace
{
	/** Allows STextPropertyEditableTextBox to edit a property handle */
	class FEditableTextPropertyHandle : public IEditableTextProperty
	{
	public:
		FEditableTextPropertyHandle(const TSharedRef<IPropertyHandle>& InPropertyHandle, const TSharedPtr<IPropertyUtilities>& InPropertyUtilities)
			: PropertyHandle(InPropertyHandle)
			, PropertyUtilities(InPropertyUtilities)
		{
			RefreshRawData();
		}

		virtual bool IsMultiLineText() const override
		{
			return PropertyHandle->IsValidHandle() && PropertyHandle->GetBoolMetaData("MultiLine");
		}

		virtual bool IsPassword() const override
		{
			return PropertyHandle->IsValidHandle() && PropertyHandle->GetBoolMetaData("PasswordField");
		}

		virtual bool IsReadOnly() const override
		{
			return !PropertyHandle->IsValidHandle() || PropertyHandle->IsEditConst();
		}

		virtual bool IsDefaultValue() const override
		{
			return PropertyHandle->IsValidHandle() && !PropertyHandle->DiffersFromDefault();
		}

		virtual FText GetToolTipText() const override
		{
			return (PropertyHandle->IsValidHandle())
				? PropertyHandle->GetToolTipText()
				: FText::GetEmpty();
		}

		virtual int32 GetNumTexts() const override
		{
			return (PropertyHandle->IsValidHandle())
				? RawTextData.Num() 
				: 0;
		}

		virtual FText GetText(const int32 InIndex) const override
		{
			if (PropertyHandle->IsValidHandle())
			{
				check(RawTextData.IsValidIndex(InIndex));
				return *RawTextData[InIndex];
			}
			return FText::GetEmpty();
		}

		virtual void SetText(const int32 InIndex, const FText& InText) override
		{
			if (PropertyHandle->IsValidHandle())
			{
				check(RawTextData.IsValidIndex(InIndex));
				*RawTextData[InIndex] = InText;
			}
		}

		virtual void PreEdit() override
		{
			if (PropertyHandle->IsValidHandle())
			{
				PropertyHandle->NotifyPreChange();
			}
		}

		virtual void PostEdit() override
		{
			if (PropertyHandle->IsValidHandle())
			{
				PropertyHandle->NotifyPostChange();
				PropertyHandle->NotifyFinishedChangingProperties();
			}

			RefreshRawData();
		}

		virtual void RequestRefresh() override
		{
			if (PropertyUtilities.IsValid())
			{
				PropertyUtilities->RequestRefresh();
			}
		}

	private:
		void RefreshRawData()
		{
			RawTextData.Empty();

			if (PropertyHandle->IsValidHandle())
			{
				auto& RawData = reinterpret_cast<TArray<void*>&>(RawTextData);
				PropertyHandle->AccessRawData(RawData);
			}
		}

		TSharedRef<IPropertyHandle> PropertyHandle;
		TSharedPtr<IPropertyUtilities> PropertyUtilities;
		TArray<FText*> RawTextData;
	};
}

void FTextCustomization::CustomizeHeader( TSharedRef<class IPropertyHandle> InPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& PropertyTypeCustomizationUtils )
{
	TSharedRef<IEditableTextProperty> EditableTextProperty = MakeShareable(new FEditableTextPropertyHandle(InPropertyHandle, PropertyTypeCustomizationUtils.GetPropertyUtilities()));
	const bool bIsMultiLine = EditableTextProperty->IsMultiLineText();

	HeaderRow.FilterString(InPropertyHandle->GetPropertyDisplayName())
		.NameContent()
		[
			InPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(bIsMultiLine ? 250.f : 125.f)
		.MaxDesiredWidth(600.f)
		[
			SNew(STextPropertyEditableTextBox, EditableTextProperty)
				.Font(FEditorStyle::GetFontStyle("PropertyWindow.NormalFont"))
				.AutoWrapText(true)
		];
}

void FTextCustomization::CustomizeChildren( TSharedRef<class IPropertyHandle> InPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& PropertyTypeCustomizationUtils )
{

}
