// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "SlateColorCustomization.h"

TSharedRef<IPropertyTypeCustomization> FSlateColorCustomization::MakeInstance() 
{
	return MakeShareable( new FSlateColorCustomization() );
}

void FSlateColorCustomization::CustomizeHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils )
{
	static const FName ColorUseRuleKey(TEXT("ColorUseRule"));
	static const FName SpecifiedColorKey(TEXT("SpecifiedColor"));

	ColorRuleHandle = StructPropertyHandle->GetChildHandle(ColorUseRuleKey);
	SpecifiedColorHandle = StructPropertyHandle->GetChildHandle(SpecifiedColorKey);

	check(ColorRuleHandle.IsValid());
	check(SpecifiedColorHandle.IsValid());
}

void FSlateColorCustomization::CustomizeChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils )
{
	SpecifiedColorHandle->SetOnPropertyValueChanged( FSimpleDelegate::CreateSP( SharedThis( this ), &FSlateColorCustomization::OnValueChanged ) );
	
	IDetailPropertyRow& ColorRow = StructBuilder.AddChildProperty(SpecifiedColorHandle.ToSharedRef());

	TSharedPtr<SWidget> NameWidget;
	TSharedPtr<SWidget> ValueWidget;
	FDetailWidgetRow Row;
	ColorRow.GetDefaultWidgets(NameWidget, ValueWidget, Row);

	ColorRow.CustomWidget(/*bShowChildren*/ true)
		.NameContent()
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.Text(StructPropertyHandle->GetPropertyDisplayName())
			.ToolTipText(StructPropertyHandle->GetToolTipText())
		]
		.ValueContent()
		.MinDesiredWidth(250.0f)
		.MaxDesiredWidth(250.0f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				ValueWidget.ToSharedRef()
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2, 0, 0, 0)
			[
				SNew(SCheckBox)
				.IsChecked(this, &FSlateColorCustomization::GetForegroundCheckState)
				.OnCheckStateChanged(this, &FSlateColorCustomization::HandleForegroundChanged)
				[
					SNew(STextBlock)
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.Text(NSLOCTEXT("SlateColorCustomization", "Inherit", "Inherit"))
					.ToolTipText(NSLOCTEXT("SlateColorCustomization", "InheritToolTip", "Uses the foreground color inherited down the widget hierarchy"))
				]
			]
		];
}

void FSlateColorCustomization::OnValueChanged()
{
	ColorRuleHandle->SetValueFromFormattedString(TEXT("UseColor_Specified"));
}

ESlateCheckBoxState::Type FSlateColorCustomization::GetForegroundCheckState() const
{
	FString ColorRuleValue;
	ColorRuleHandle->GetValueAsFormattedString(ColorRuleValue);

	if ( ColorRuleValue == TEXT("UseColor_Foreground") )
	{
		return ESlateCheckBoxState::Checked;
	}

	return ESlateCheckBoxState::Unchecked;
}

void FSlateColorCustomization::HandleForegroundChanged(ESlateCheckBoxState::Type CheckedState)
{
	if ( CheckedState == ESlateCheckBoxState::Checked )
	{
		ColorRuleHandle->SetValueFromFormattedString(TEXT("UseColor_Foreground"));
	}
	else
	{
		ColorRuleHandle->SetValueFromFormattedString(TEXT("UseColor_Specified"));
	}
}
