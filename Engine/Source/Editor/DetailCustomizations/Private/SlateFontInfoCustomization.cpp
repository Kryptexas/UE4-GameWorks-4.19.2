// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "SlateFontInfoCustomization.h"
#include "AssetData.h"
#include "Engine/Font.h"

#define LOCTEXT_NAMESPACE "SlateFontInfo"

TSharedRef<IPropertyTypeCustomization> FSlateFontInfoStructCustomization::MakeInstance() 
{
	return MakeShareable(new FSlateFontInfoStructCustomization());
}

void FSlateFontInfoStructCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	FontObjectProperty = StructPropertyHandle->GetChildHandle(TEXT("FontObject"));
	check(FontObjectProperty.IsValid());

	TypefaceFontNameProperty = StructPropertyHandle->GetChildHandle(TEXT("TypefaceFontName"));
	check(TypefaceFontNameProperty.IsValid());

	FontSizeProperty = StructPropertyHandle->GetChildHandle(TEXT("Size"));
	check(FontSizeProperty.IsValid());

	TArray<void*> StructPtrs;
	StructPropertyHandle->AccessRawData(StructPtrs);
	for(auto It = StructPtrs.CreateConstIterator(); It; ++It)
	{
		FSlateFontInfo* const SlateFontInfo = reinterpret_cast<FSlateFontInfo*>(*It);
		SlateFontInfoStructs.Add(SlateFontInfo);
	}

	HeaderRow
	.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(250.0f)
	.MaxDesiredWidth(0.0f)
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Center)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot()
		.FillWidth(2)
		[
			SNew(SObjectPropertyEntryBox)
			.PropertyHandle(FontObjectProperty)
			.AllowedClass(UFont::StaticClass())
			.OnShouldFilterAsset(FOnShouldFilterAsset::CreateStatic(&FSlateFontInfoStructCustomization::OnFilterFontAsset))
			.OnObjectChanged(this, &FSlateFontInfoStructCustomization::OnFontChanged)
			.DisplayUseSelected(false)
			.DisplayBrowse(false)
		]

		+SHorizontalBox::Slot()
		.FillWidth(2)
		.VAlign(VAlign_Center)
		[
			SAssignNew(FontEntryCombo, SComboBox<TSharedPtr<FName>>)
			.OptionsSource(&FontEntryComboData)
			.IsEnabled(this, &FSlateFontInfoStructCustomization::IsFontEntryComboEnabled)
			.OnComboBoxOpening(this, &FSlateFontInfoStructCustomization::OnFontEntryComboOpening)
			.OnSelectionChanged(this, &FSlateFontInfoStructCustomization::OnFontEntrySelectionChanged)
			.OnGenerateWidget(this, &FSlateFontInfoStructCustomization::MakeFontEntryWidget)
			[
				SNew(STextBlock)
				.Text(this, &FSlateFontInfoStructCustomization::GetFontEntryComboText)
				.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			]
		]

		+SHorizontalBox::Slot()
		.FillWidth(1)
		.VAlign(VAlign_Center)
		.Padding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
		[
			SNew(SProperty, FontSizeProperty)
			.ShouldDisplayName(false)
		]
	];
}

void FSlateFontInfoStructCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}

bool FSlateFontInfoStructCustomization::OnFilterFontAsset(const FAssetData& InAssetData)
{
	// We want to filter font assets that aren't valid to use with Slate/UMG
	return Cast<const UFont>(InAssetData.GetAsset())->FontCacheType != EFontCacheType::Runtime;
}

void FSlateFontInfoStructCustomization::OnFontChanged(const FAssetData& InAssetData)
{
	const UFont* const FontAsset = Cast<const UFont>(InAssetData.GetAsset());
	const FName FirstFontName = (FontAsset && FontAsset->CompositeFont.DefaultTypeface.Fonts.Num()) ? FontAsset->CompositeFont.DefaultTypeface.Fonts[0].Name : NAME_None;

	for(FSlateFontInfo* FontInfo : SlateFontInfoStructs)
	{
		// The font has been updated in the editor, so clear the non-UObject pointer so that the two don't conflict
		FontInfo->CompositeFont.Reset();

		// We've changed (or cleared) the font asset, so make sure and update the typeface entry name being used by the font info
		TypefaceFontNameProperty->SetValue(FirstFontName);
	}
}

bool FSlateFontInfoStructCustomization::IsFontEntryComboEnabled() const
{
	const FSlateFontInfo* const FirstSlateFontInfo = SlateFontInfoStructs[0];
	const UFont* const FontObject = Cast<const UFont>(FirstSlateFontInfo->FontObject);
	if(!FontObject)
	{
		return false;
	}

	// Only let people pick an entry if every struct being edited is using the same font object
	for(int32 FontInfoIndex = 1; FontInfoIndex < SlateFontInfoStructs.Num(); ++FontInfoIndex)
	{
		const FSlateFontInfo* const OtherSlateFontInfo = SlateFontInfoStructs[FontInfoIndex];
		const UFont* const OtherFontObject = Cast<const UFont>(OtherSlateFontInfo->FontObject);
		if(FontObject != OtherFontObject)
		{
			return false;
		}
	}

	return true;
}

void FSlateFontInfoStructCustomization::OnFontEntryComboOpening()
{
	const FSlateFontInfo* const FirstSlateFontInfo = SlateFontInfoStructs[0];
	const UFont* const FontObject = Cast<const UFont>(FirstSlateFontInfo->FontObject);
	check(FontObject);

	const FName ActiveFontEntry = GetActiveFontEntry();
	TSharedPtr<FName> SelectedNamePtr;

	FontEntryComboData.Empty();
	for(const FTypefaceEntry& TypefaceEntry : FontObject->CompositeFont.DefaultTypeface.Fonts)
	{
		TSharedPtr<FName> NameEntryPtr = MakeShareable(new FName(TypefaceEntry.Name));
		FontEntryComboData.Add(NameEntryPtr);

		if(!TypefaceEntry.Name.IsNone() && TypefaceEntry.Name == ActiveFontEntry)
		{
			SelectedNamePtr = NameEntryPtr;
		}
	}

	FontEntryComboData.Sort([](const TSharedPtr<FName>& One, const TSharedPtr<FName>& Two) -> bool
	{
		return One->ToString() < Two->ToString();
	});

	FontEntryCombo->ClearSelection();
	FontEntryCombo->RefreshOptions();
	FontEntryCombo->SetSelectedItem(SelectedNamePtr);
}

void FSlateFontInfoStructCustomization::OnFontEntrySelectionChanged(TSharedPtr<FName> InNewSelection, ESelectInfo::Type)
{
	if(InNewSelection.IsValid())
	{
		const FSlateFontInfo* const FirstSlateFontInfo = SlateFontInfoStructs[0];
		if(FirstSlateFontInfo->TypefaceFontName != *InNewSelection)
		{
			TypefaceFontNameProperty->SetValue(*InNewSelection);
		}
	}
}

TSharedRef<SWidget> FSlateFontInfoStructCustomization::MakeFontEntryWidget(TSharedPtr<FName> InFontEntry)
{
	return
		SNew(STextBlock)
		.Text(FText::FromName(*InFontEntry))
		.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")));
}

FText FSlateFontInfoStructCustomization::GetFontEntryComboText() const
{
	return FText::FromName(GetActiveFontEntry());
}

FName FSlateFontInfoStructCustomization::GetActiveFontEntry() const
{
	const FSlateFontInfo* const FirstSlateFontInfo = SlateFontInfoStructs[0];
	const UFont* const FontObject = Cast<const UFont>(FirstSlateFontInfo->FontObject);
	if(FontObject)
	{
		return (FirstSlateFontInfo->TypefaceFontName.IsNone() && FontObject->CompositeFont.DefaultTypeface.Fonts.Num())
			? FontObject->CompositeFont.DefaultTypeface.Fonts[0].Name
			: FirstSlateFontInfo->TypefaceFontName;
	}

	return NAME_None;
}

#undef LOCTEXT_NAMESPACE
