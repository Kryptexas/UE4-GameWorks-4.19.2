// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PrimaryAssetIdCustomization.h"
#include "AssetManagerEditorModule.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Engine/AssetManager.h"
#include "PropertyHandle.h"
#include "PropertyCustomizationHelpers.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "PrimaryAssetIdCustomization"

void FPrimaryAssetIdCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> InStructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	if (!UAssetManager::IsValid())
	{
		HeaderRow
		.NameContent()
		[
			InStructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(250.0f)
		.MaxDesiredWidth(0.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoAssetManager", "Enable Asset Manager to edit Primary Asset Ids"))
		];

		return;
	}

	StructPropertyHandle = InStructPropertyHandle;

	const FString& TypeFilterString = StructPropertyHandle->GetMetaData("AllowedTypes");
	if( !TypeFilterString.IsEmpty() )
	{
		TArray<FString> CustomTypeFilterNames;
		TypeFilterString.ParseIntoArray(CustomTypeFilterNames, TEXT(","), true);

		for(auto It = CustomTypeFilterNames.CreateConstIterator(); It; ++It)
		{
			const FString& TypeName = *It;

			AllowedTypes.Add(*TypeName);
		}
	}

	// Can the field be cleared
	const bool bAllowClear = !(StructPropertyHandle->GetMetaDataProperty()->PropertyFlags & CPF_NoClear);

	HeaderRow
	.NameContent()
	[
		InStructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(250.0f)
	.MaxDesiredWidth(0.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			IAssetManagerEditorModule::MakePrimaryAssetIdSelector(
				FOnGetPrimaryAssetDisplayText::CreateSP(this, &FPrimaryAssetIdCustomization::GetDisplayText),
				FOnSetPrimaryAssetId::CreateSP(this, &FPrimaryAssetIdCustomization::OnIdSelected),
				bAllowClear, AllowedTypes)
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			PropertyCustomizationHelpers::MakeUseSelectedButton(FSimpleDelegate::CreateSP(this, &FPrimaryAssetIdCustomization::OnUseSelected))
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			PropertyCustomizationHelpers::MakeBrowseButton(FSimpleDelegate::CreateSP(this, &FPrimaryAssetIdCustomization::OnBrowseTo))
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			PropertyCustomizationHelpers::MakeClearButton(FSimpleDelegate::CreateSP(this, &FPrimaryAssetIdCustomization::OnClear))
		]
	];
}

void FPrimaryAssetIdCustomization::OnIdSelected(FPrimaryAssetId AssetId)
{
	if (StructPropertyHandle.IsValid() && StructPropertyHandle->IsValidHandle())
	{
		StructPropertyHandle->SetValueFromFormattedString(AssetId.ToString());
	}
}

FText FPrimaryAssetIdCustomization::GetDisplayText() const
{
	FString StringReference;
	if (StructPropertyHandle.IsValid())
	{
		StructPropertyHandle->GetValueAsFormattedString(StringReference);
	}
	else
	{
		StringReference = FPrimaryAssetId().ToString();
	}

	return FText::AsCultureInvariant(StringReference);
}

FPrimaryAssetId FPrimaryAssetIdCustomization::GetCurrentPrimaryAssetId() const
{
	FString StringReference;
	if (StructPropertyHandle.IsValid())
	{
		StructPropertyHandle->GetValueAsFormattedString(StringReference);
	}
	else
	{
		StringReference = FPrimaryAssetId().ToString();
	}

	return FPrimaryAssetId(StringReference);
}

void FPrimaryAssetIdCustomization::OnBrowseTo()
{
	FPrimaryAssetId PrimaryAssetId = GetCurrentPrimaryAssetId();

	if (PrimaryAssetId.IsValid())
	{
		FAssetData FoundData;
		if (UAssetManager::Get().GetPrimaryAssetData(PrimaryAssetId, FoundData))
		{
			TArray<FAssetData> SyncAssets;
			SyncAssets.Add(FoundData);
			GEditor->SyncBrowserToObjects(SyncAssets);
		}
	}	
}

void FPrimaryAssetIdCustomization::OnUseSelected()
{
	TArray<FAssetData> SelectedAssets;
	GEditor->GetContentBrowserSelections(SelectedAssets);

	for (const FAssetData& AssetData : SelectedAssets)
	{
		FPrimaryAssetId PrimaryAssetId = UAssetManager::Get().GetPrimaryAssetIdForData(AssetData);
		if (PrimaryAssetId.IsValid())
		{
			OnIdSelected(PrimaryAssetId);
			return;
		}
	}
}

void FPrimaryAssetIdCustomization::OnClear()
{
	OnIdSelected(FPrimaryAssetId());
}

void SPrimaryAssetIdGraphPin::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
}

TSharedRef<SWidget>	SPrimaryAssetIdGraphPin::GetDefaultValueWidget()
{
	FString DefaultString = GraphPinObj->GetDefaultAsString();
	CurrentId = FPrimaryAssetId(DefaultString);

	return SNew(SHorizontalBox)
		.Visibility(this, &SGraphPin::GetDefaultValueVisibility)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			IAssetManagerEditorModule::MakePrimaryAssetIdSelector(
				FOnGetPrimaryAssetDisplayText::CreateSP(this, &SPrimaryAssetIdGraphPin::GetDisplayText),
				FOnSetPrimaryAssetId::CreateSP(this, &SPrimaryAssetIdGraphPin::OnIdSelected),
				true)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(1, 0)
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.ButtonStyle(FEditorStyle::Get(), "NoBorder")
			.ButtonColorAndOpacity(this, &SPrimaryAssetIdGraphPin::OnGetWidgetBackground)
			.OnClicked(this, &SPrimaryAssetIdGraphPin::OnUseSelected)
			.ContentPadding(1.f)
			.ToolTipText(NSLOCTEXT("GraphEditor", "ObjectGraphPin_Use_Tooltip", "Use asset browser selection"))
			[
				SNew(SImage)
				.ColorAndOpacity(this, &SPrimaryAssetIdGraphPin::OnGetWidgetForeground)
				.Image(FEditorStyle::GetBrush(TEXT("PropertyWindow.Button_Use")))
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(1, 0)
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.ButtonStyle(FEditorStyle::Get(), "NoBorder")
			.ButtonColorAndOpacity(this, &SPrimaryAssetIdGraphPin::OnGetWidgetBackground)
			.OnClicked(this, &SPrimaryAssetIdGraphPin::OnBrowseTo)
			.ContentPadding(0)
			.ToolTipText(NSLOCTEXT("GraphEditor", "ObjectGraphPin_Browse_Tooltip", "Browse"))
			[
				SNew(SImage)
				.ColorAndOpacity(this, &SPrimaryAssetIdGraphPin::OnGetWidgetForeground)
				.Image(FEditorStyle::GetBrush(TEXT("PropertyWindow.Button_Browse")))
			]
		];
}

FSlateColor SPrimaryAssetIdGraphPin::OnGetWidgetForeground() const
{
	float Alpha = (IsHovered() || bOnlyShowDefaultValue) ? 1.f : 0.15f;
	return FSlateColor(FLinearColor(1.f, 1.f, 1.f, Alpha));
}

FSlateColor SPrimaryAssetIdGraphPin::OnGetWidgetBackground() const
{
	float Alpha = (IsHovered() || bOnlyShowDefaultValue) ? 0.8f : 0.4f;
	return FSlateColor(FLinearColor(1.f, 1.f, 1.f, Alpha));
}

void SPrimaryAssetIdGraphPin::OnIdSelected(FPrimaryAssetId AssetId)
{
	CurrentId = AssetId;
	GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, CurrentId.ToString());
}

FText SPrimaryAssetIdGraphPin::GetDisplayText() const
{
	return FText::AsCultureInvariant(CurrentId.ToString());
}

FReply SPrimaryAssetIdGraphPin::OnBrowseTo()
{
	if (CurrentId.IsValid())
	{
		FAssetData FoundData;
		if (UAssetManager::Get().GetPrimaryAssetData(CurrentId, FoundData))
		{
			TArray<FAssetData> SyncAssets;
			SyncAssets.Add(FoundData);
			GEditor->SyncBrowserToObjects(SyncAssets);
		}
	}

	return FReply::Handled();
}

FReply SPrimaryAssetIdGraphPin::OnUseSelected()
{
	TArray<FAssetData> SelectedAssets;
	GEditor->GetContentBrowserSelections(SelectedAssets);

	for (const FAssetData& AssetData : SelectedAssets)
	{
		FPrimaryAssetId PrimaryAssetId = UAssetManager::Get().GetPrimaryAssetIdForData(AssetData);
		if (PrimaryAssetId.IsValid())
		{
			OnIdSelected(PrimaryAssetId);
			return FReply::Handled();
		}
	}
	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE
