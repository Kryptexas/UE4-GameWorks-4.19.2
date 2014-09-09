// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"

#include "HardwareTargetingSettingsDetails.h"
#include "HardwareTargetingModule.h"

#define LOCTEXT_NAMESPACE "FHardwareTargetingSettingsDetails"

TSharedRef<IDetailCustomization> FHardwareTargetingSettingsDetails::MakeInstance()
{
	return MakeShareable(new FHardwareTargetingSettingsDetails);
}

void FHardwareTargetingSettingsDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{	
	IDetailCategoryBuilder& HardwareTargetingCategory = DetailBuilder.EditCategory(TEXT("Hardware Targeting"));
	
	IHardwareTargetingModule& HardwareTargeting = IHardwareTargetingModule::Get();
	HardwareTargetingCategory.AddCustomRow(FString())
	[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("NoBorder"))
		.Padding(0)
		.Visibility_Static([]()->EVisibility{
			auto* Settings = GetMutableDefault<UHardwareTargetingSettings>();
			return Settings->HasPendingChanges() ? EVisibility::Visible : EVisibility::Collapsed;
		})
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("RestartMessage", "These changes will be applied when this project is reopened."))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(6)
			[
				SNew(SButton)
				.Text(LOCTEXT("ApplyNow", "Apply Now"))
				.OnClicked_Static([](){
				
					IHardwareTargetingModule& Module = IHardwareTargetingModule::Get();
					Module.ApplyHardwareTargetingSettings();

					return FReply::Handled();
				})
			]
		]
	];


	auto PropertyName = GET_MEMBER_NAME_CHECKED(UHardwareTargetingSettings, TargetedHardwareClass);
	DetailBuilder.HideProperty(PropertyName);

	TSharedRef<IPropertyHandle> Property = DetailBuilder.GetProperty(PropertyName);
	HardwareTargetingCategory.AddCustomRow(LOCTEXT("HardwareTargetingOption", "Targeted Hardware:").ToString())
	.NameContent()
	[
		SNew(STextBlock)
		.Text(FText::FromString(Property->GetPropertyDisplayName()))
		.Font(DetailBuilder.GetDetailFont())
	]
	.ValueContent()
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			HardwareTargeting.MakeHardwareClassTargetCombo(
				FOnHardwareClassChanged::CreateStatic([](EHardwareClass::Type NewValue, TSharedRef<IPropertyHandle> Property){
					Property->SetValue(uint8(NewValue));
				}, Property),
				TAttribute<EHardwareClass::Type>::Create(TAttribute<EHardwareClass::Type>::FGetter::CreateStatic([](TSharedRef<IPropertyHandle> Property){
					uint8 Value = 0;
					Property->GetValue(Value);
					return EHardwareClass::Type(Value);
				}, Property))
			)
		]
	];

	PropertyName = GET_MEMBER_NAME_CHECKED(UHardwareTargetingSettings, DefaultGraphicsPerformance);
	DetailBuilder.HideProperty(PropertyName);

	Property = DetailBuilder.GetProperty(PropertyName);
	HardwareTargetingCategory.AddCustomRow(LOCTEXT("GraphicsPreferenceOption", "Graphics Preference:").ToString())
	.NameContent()
	[
		SNew(STextBlock)
		.Text(FText::FromString(Property->GetPropertyDisplayName()))
		.Font(DetailBuilder.GetDetailFont())
	]
	.ValueContent()
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			HardwareTargeting.MakeGraphicsPresetTargetCombo(
				FOnGraphicsPresetChanged::CreateStatic([](EGraphicsPreset::Type NewValue, TSharedRef<IPropertyHandle> Property){
					Property->SetValue(uint8(NewValue));
				}, Property),
				TAttribute<EGraphicsPreset::Type>::Create(TAttribute<EGraphicsPreset::Type>::FGetter::CreateStatic([](TSharedRef<IPropertyHandle> Property){
					uint8 Value = 0;
					Property->GetValue(Value);
					return EGraphicsPreset::Type(Value);
				}, Property))
			)
		]
	];

}

#undef LOCTEXT_NAMESPACE
