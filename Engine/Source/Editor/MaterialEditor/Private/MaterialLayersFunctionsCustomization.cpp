// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MaterialLayersFunctionsCustomization.h"
#include "Materials/MaterialExpressionMaterialAttributeLayers.h"
#include "Materials/MaterialFunctionInterface.h"

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SGridPanel.h"
#include "STextBlock.h"
#include "SButton.h"

#include "Engine/GameViewportClient.h"

#include "PropertyHandle.h"
#include "Sound/SoundBase.h"
#include "PropertyCustomizationHelpers.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyTypeCustomization.h"
#include "IPropertyUtilities.h"
#include "DetailLayoutBuilder.h"

#include "Editor.h"
#include "ScopedTransaction.h"
#include "IDetailGroup.h"
#include "SInlineEditableTextBlock.h"
#include "SlateDelegates.h"
#include "Attribute.h"
#include "SlateTypes.h"


#define LOCTEXT_NAMESPACE "MaterialLayerCustomization"

FMaterialLayersFunctionsCustomization::FMaterialLayersFunctionsCustomization(const TSharedPtr<class IPropertyHandle>& StructPropertyHandle, const class IDetailLayoutBuilder* InDetailLayout)
{
	// Save data for later re-use
	SavedStructPropertyHandle = ConstCastSharedPtr<IPropertyHandle>(StructPropertyHandle);
	SavedLayoutBuilder = const_cast<IDetailLayoutBuilder*>(InDetailLayout);
	PropertyUtilities = SavedLayoutBuilder->GetPropertyUtilities();
	TArray<void*> StructPtrs;
	SavedStructPropertyHandle->AccessRawData(StructPtrs);
	auto It = StructPtrs.CreateConstIterator();
	MaterialLayersFunctions = reinterpret_cast<FMaterialLayersFunctions*>(*It);

	FSimpleDelegate RebuildChildrenDelegate = FSimpleDelegate::CreateRaw(this, &FMaterialLayersFunctionsCustomization::RebuildChildren);
	LayerHandle = SavedStructPropertyHandle->GetChildHandle("Layers").ToSharedRef();
	BlendHandle = SavedStructPropertyHandle->GetChildHandle("Blends").ToSharedRef();

	// UI will be refreshed twice, but this means layers and blends can be adjusted in an order-agnostic way
	LayerHandle->AsArray()->SetOnNumElementsChanged(RebuildChildrenDelegate);
	BlendHandle->AsArray()->SetOnNumElementsChanged(RebuildChildrenDelegate);
}

void FMaterialLayersFunctionsCustomization::ResetToDefault()
{
	const FScopedTransaction Transaction(LOCTEXT("ResetMaterialLayersFunctions", "Reset all Layers and Blends"));
	SavedStructPropertyHandle->NotifyPreChange();
	
	// Empty the existing Layers and Blends arrays
	MaterialLayersFunctions->Layers.Empty();
	MaterialLayersFunctions->Blends.Empty();

	// Add a new empty Layer
	MaterialLayersFunctions->Layers.AddDefaulted();

	SavedStructPropertyHandle->NotifyPostChange();

	// Refresh the header so the reset to default button is no longer visible
	SavedLayoutBuilder->ForceRefreshDetails();
}

bool FMaterialLayersFunctionsCustomization::IsResetToDefaultVisible() const
{
	FString DisplayString = TEXT("None");
	if (MaterialLayersFunctions->Layers[0] != nullptr)
	{
		MaterialLayersFunctions->Layers[0]->GetName(DisplayString);
	}
	// Can reset if:
	// There is more than one layer OR
	// There is only one layer but it has a material layer set
	return (MaterialLayersFunctions->Layers.Num() != 1) || 
		(MaterialLayersFunctions->Layers.Num() == 1 && DisplayString != TEXT("None"));
}

void FMaterialLayersFunctionsCustomization::RebuildChildren()
{
	OnRebuildChildren.ExecuteIfBound();
}

void FMaterialLayersFunctionsCustomization::GenerateHeaderRowContent(FDetailWidgetRow& NodeRow)
{

	TAttribute<bool>::FGetter IsResetButtonEnabledDelegate = TAttribute<bool>::FGetter::CreateSP(this, &FMaterialLayersFunctionsCustomization::IsResetToDefaultVisible);
	TAttribute<bool> IsEnabledAttribute = TAttribute<bool>::Create(IsResetButtonEnabledDelegate);

	NodeRow
		.NameContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(3.0f))
			[
				SavedStructPropertyHandle->CreatePropertyNameWidget()
			]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			PropertyCustomizationHelpers::MakeResetButton(FSimpleDelegate::CreateSP(this, &FMaterialLayersFunctionsCustomization::ResetToDefault), FText(), IsEnabledAttribute)
		]
		]
		.ValueContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.AutoWidth()
			.VAlign(VAlign_Top)
			[
				PropertyCustomizationHelpers::MakeAddButton(FSimpleDelegate::CreateSP(this, &FMaterialLayersFunctionsCustomization::AddLayer))
			]

		];
}

void FMaterialLayersFunctionsCustomization::GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder)
{
	uint32 LayerChildren;
	LayerHandle->GetNumChildren(LayerChildren);
	uint32 BlendChildren;
	BlendHandle->GetNumChildren(BlendChildren);
	TSharedRef<SWidget> RemoveWidget = SNullWidget::NullWidget;
	int32 InitCounter = LayerChildren - 1;
	FText GroupName = MaterialLayersFunctions->GetLayerName(InitCounter);
	FName GroupFName = FName(*(GroupName.ToString()));
	
	IDetailGroup& Group = ChildrenBuilder.AddGroup(GroupFName, GroupName);

	// Users can only remove layers
	// You can never have fewer than one layer
	DetailGroups.Add(&Group);
	FDetailWidgetRow& NewLayerRow = Group.AddWidgetRow();
	TSharedRef<class FMaterialLayerFunctionElement> LayerLayout = MakeShareable(new FMaterialLayerFunctionElement(this, LayerHandle->AsArray()->GetElement(InitCounter), EMaterialLayerRowType::Layer));
	LayerLayout->GenerateHeaderRowContent(NewLayerRow);
	if (BlendChildren > 0)
	{
		RemoveWidget = PropertyCustomizationHelpers::MakeClearButton(FSimpleDelegate::CreateSP(this, &FMaterialLayersFunctionsCustomization::RemoveLayer, InitCounter));
		Group.HeaderRow()
			.NameContent()
			[
				SNew(SInlineEditableTextBlock)
				.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &FMaterialLayersFunctionsCustomization::GetLayerName, InitCounter)))
				.OnTextCommitted(FOnTextCommitted::CreateSP(this, &FMaterialLayersFunctionsCustomization::OnNameChanged, InitCounter))
				.Font(FEditorStyle::GetFontStyle(TEXT("MaterialEditor.Layers.EditableFont")))
			]
		.ValueContent()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					RemoveWidget
				]
			];
	}
	//Only draw the rows when the correct number of blends and layers are present
	if (BlendChildren > 0 && LayerChildren > BlendChildren)
	{
		for (int32 Counter = BlendChildren - 1; Counter >= 0; Counter--)
		{
			FDetailWidgetRow& NewBlendRow = DetailGroups.Last()->AddWidgetRow();
			TSharedRef<class FMaterialLayerFunctionElement> BlendLayout = MakeShareable(new FMaterialLayerFunctionElement(this, BlendHandle->AsArray()->GetElement(Counter), EMaterialLayerRowType::Blend));
			BlendLayout->GenerateHeaderRowContent(NewBlendRow);
			GroupName = MaterialLayersFunctions->GetLayerName(Counter);
			GroupFName = FName(*(GroupName.ToString()));
			IDetailGroup& NewGroup = ChildrenBuilder.AddGroup(GroupFName, GroupName);
			DetailGroups.Add(&NewGroup);

			if (Counter > 0)
			{
				RemoveWidget = PropertyCustomizationHelpers::MakeClearButton(FSimpleDelegate::CreateSP(this, &FMaterialLayersFunctionsCustomization::RemoveLayer, Counter));

				NewGroup.HeaderRow()
					.NameContent()
					[
						SNew(SInlineEditableTextBlock)
						.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &FMaterialLayersFunctionsCustomization::GetLayerName, Counter)))
						.OnTextCommitted(FOnTextCommitted::CreateSP(this, &FMaterialLayersFunctionsCustomization::OnNameChanged, Counter))
						.Font(FEditorStyle::GetFontStyle(TEXT("MaterialEditor.Layers.EditableFont")))
					]
				.ValueContent()
					[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						RemoveWidget
					]
					];
			}

			FDetailWidgetRow& NextLayerRow = DetailGroups.Last()->AddWidgetRow();
			LayerLayout = MakeShareable(new FMaterialLayerFunctionElement(this, LayerHandle->AsArray()->GetElement(Counter), EMaterialLayerRowType::Layer));
			LayerLayout->GenerateHeaderRowContent(NextLayerRow);
		}
	}

}

#if WITH_EDITOR


FText FMaterialLayersFunctionsCustomization::GetLayerName(int32 Counter) const
{
	return MaterialLayersFunctions->GetLayerName(Counter);
}

void FMaterialLayersFunctionsCustomization::OnNameChanged(const FText& InText, ETextCommit::Type CommitInfo, int32 Counter)
{
	const FScopedTransaction Transaction(LOCTEXT("RenamedSection", "Renamed layer and blend section"));
	SavedStructPropertyHandle->NotifyPreChange();
	MaterialLayersFunctions->LayerNames[Counter] = InText;
	SavedStructPropertyHandle->NotifyPostChange();
};
#endif

void FMaterialLayersFunctionsCustomization::AddLayer()
{
	const FScopedTransaction Transaction(LOCTEXT("AddLayerAndBlend", "Add a new Layer and a Blend into it"));
	SavedStructPropertyHandle->NotifyPreChange();
	MaterialLayersFunctions->AppendBlendedLayer();
	SavedStructPropertyHandle->NotifyPostChange();
}

void FMaterialLayersFunctionsCustomization::RemoveLayer(int32 Index)
{
	const FScopedTransaction Transaction(LOCTEXT("RemoveLayerAndBlend", "Remove a Layer and the attached Blend"));
	SavedStructPropertyHandle->NotifyPreChange();
	MaterialLayersFunctions->RemoveBlendedLayerAt(Index);
	SavedStructPropertyHandle->NotifyPostChange();
}

FMaterialLayerFunctionElement::FMaterialLayerFunctionElement(FMaterialLayersFunctionsCustomization* InCustomization, TWeakPtr<IPropertyHandle> InPropertyHandle, EMaterialLayerRowType InRowType)
{
	ParentCustomization = InCustomization;
	RowPropertyHandle = InPropertyHandle.Pin();
	RowType = InRowType;
	if (InRowType == EMaterialLayerRowType::Layer)
	{
		RowPropertyHandle->GetProperty()->SetMetaData(FName(TEXT("DisplayThumbnail")), TEXT("true"));
	}
}

void FMaterialLayerFunctionElement::ResetToDefault(TSharedPtr<IPropertyHandle> PropertyHandle, FMaterialLayersFunctionsCustomization* InCustomization, int32 InIndex, EMaterialLayerRowType MaterialType)
{
	const FScopedTransaction Transaction(LOCTEXT("ResetLayerOrBlend", "Reset Layer/Blend Value"));
	PropertyHandle->NotifyPreChange();
	switch (MaterialType)
	{
	case EMaterialLayerRowType::Layer:
	{
		InCustomization->GetMaterialLayersFunctions()->Layers[InIndex] = nullptr;
		break;
	}
	case EMaterialLayerRowType::Blend:
	{
		InCustomization->GetMaterialLayersFunctions()->Blends[InIndex] = nullptr;
		break;
	}
	}
	PropertyHandle->NotifyPostChange();
}

bool FMaterialLayerFunctionElement::CanResetToDefault(TSharedPtr<IPropertyHandle> PropertyHandle)
{
	FString DisplayString;
	PropertyHandle->GetValueAsDisplayString(DisplayString);
	return DisplayString != TEXT("None");
}

void FMaterialLayerFunctionElement::GenerateHeaderRowContent(FDetailWidgetRow& NodeRow)
{
	Index = 0;
	FText DisplayText = FText();
	if (RowPropertyHandle.IsValid())
	{
		Index = FCString::Atoi(*(RowPropertyHandle.Get()->GetPropertyDisplayName().ToString()));
	}
	switch (RowType)
	{
	case EMaterialLayerRowType::Layer:
	{
		DisplayText = FText(LOCTEXT("LayerAsset", "Layer Asset"));
	}
	break;
	case EMaterialLayerRowType::Blend:
	{
		DisplayText = FText(LOCTEXT("BlendAsset", "Blend Asset"));
	}
	break;
	default:
		break;
	}

	TSharedRef<SWidget> PlaceholderIcon = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.Padding(10.0f);

	uint32 LayerChildren;
	RowPropertyHandle->GetParentHandle().ToSharedRef().Get().GetNumChildren(LayerChildren);

	FMargin AssetPadding = FMargin(0.0f);

	FIsResetToDefaultVisible IsResetVisible = FIsResetToDefaultVisible::CreateStatic(&FMaterialLayerFunctionElement::CanResetToDefault);
	FResetToDefaultHandler ResetHandler = FResetToDefaultHandler::CreateStatic(&FMaterialLayerFunctionElement::ResetToDefault, ParentCustomization, Index, RowType);
	FResetToDefaultOverride ResetOverride = FResetToDefaultOverride::Create(IsResetVisible, ResetHandler);

	FOnShouldFilterAsset AssetFilter = FOnShouldFilterAsset::CreateStatic(&FMaterialLayerFunctionElement::FilterTargets, RowType);

	NodeRow
		.NameContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1)
			.VAlign(VAlign_Center)
			[
				RowPropertyHandle->CreatePropertyNameWidget(DisplayText)
			]
		]
		.ValueContent()
		.MinDesiredWidth(400.0)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(AssetPadding)
			[
				SNew(SObjectPropertyEntryBox)
				.AllowedClass(UMaterialFunctionInterface::StaticClass())
				.PropertyHandle(RowPropertyHandle)
				.OnShouldFilterAsset(AssetFilter)
				.CustomResetToDefault(ResetOverride)
				.ThumbnailPool(ParentCustomization->GetPropertyUtilities()->GetThumbnailPool())
			]
		];
}

bool FMaterialLayerFunctionElement::FilterTargets(const struct FAssetData& InAssetData, EMaterialLayerRowType MaterialType)
{
	bool ShouldAssetBeFilteredOut = false;
	const FName FilterTag = FName(TEXT("MaterialFunctionUsage"));
	const FString* MaterialFunctionUsage = InAssetData.TagsAndValues.Find(FilterTag);
	FString CompareString;
	if (MaterialFunctionUsage)
	{
		switch (MaterialType)
		{
		case EMaterialLayerRowType::Layer:
		{
			CompareString = "MaterialLayer";
		}
		break;
		case EMaterialLayerRowType::Blend:
		{
			CompareString = "MaterialLayerBlend";
		}
		break;
		default:
		break;
		}

		if (*MaterialFunctionUsage != CompareString)
		{
			ShouldAssetBeFilteredOut = true;
		}
	}
	else
	{
		ShouldAssetBeFilteredOut = true;
	}
	return ShouldAssetBeFilteredOut;
}

#undef LOCTEXT_NAMESPACE