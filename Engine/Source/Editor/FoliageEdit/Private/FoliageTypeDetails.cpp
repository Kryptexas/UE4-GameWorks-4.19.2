// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "FoliageEditModule.h"
#include "PropertyEditing.h"
#include "FoliageTypeDetails.h"
#include "FoliageType.h"
#include "FoliageTypeCustomizationHelpers.h"

TSharedRef<IDetailCustomization> FFoliageTypeDetails::MakeInstance()
{
	return MakeShareable(new FFoliageTypeDetails());
}

void CustomizePropertyRowVisibility(IDetailLayoutBuilder& LayoutBuilder, const TSharedRef<IPropertyHandle>& PropertyHandle, IDetailPropertyRow& PropertyRow)
{
	// Properties with a HideBehind property specified should only be shown if that property is true, non-zero, or not empty
	static const FName HideBehindName("HideBehind");
	if (UProperty* Property = PropertyHandle->GetProperty())
	{
		if (Property->HasMetaData(HideBehindName))
		{
			TSharedPtr<IPropertyHandle> HiddenBehindPropertyHandle = LayoutBuilder.GetProperty(*Property->GetMetaData(HideBehindName));
			if (HiddenBehindPropertyHandle.IsValid() && HiddenBehindPropertyHandle->IsValidHandle())
			{
				TAttribute<EVisibility>::FGetter VisibilityGetter;
				FFoliageTypeCustomizationHelpers::BindHiddenPropertyVisibilityGetter(HiddenBehindPropertyHandle, VisibilityGetter);

				PropertyRow.Visibility(TAttribute<EVisibility>::Create(VisibilityGetter));
			}
		}
	}
}

void AddSubcategoryProperties(IDetailLayoutBuilder& LayoutBuilder, const FName CategoryName, TMap<const FName, IDetailPropertyRow*>* OutDetailRowsByPropertyName = nullptr)
{
	IDetailCategoryBuilder& CategoryBuilder = LayoutBuilder.EditCategory(CategoryName);

	TArray<TSharedRef<IPropertyHandle>> CategoryProperties;
	CategoryBuilder.GetDefaultProperties(CategoryProperties, true, true);

	//Build map of subcategories to properties
	static const FName SubcategoryName("Subcategory");
	TMap<FString, TArray<TSharedRef<IPropertyHandle>> > SubcategoryPropertiesMap;
	for (auto& PropertyHandle : CategoryProperties)
	{
		if (UProperty* Property = PropertyHandle->GetProperty())
		{
			if (Property->HasMetaData(SubcategoryName))
			{
				const FString& Subcategory = Property->GetMetaData(SubcategoryName);
				TArray<TSharedRef<IPropertyHandle>>& PropertyHandles = SubcategoryPropertiesMap.FindOrAdd(Subcategory);
				PropertyHandles.Add(PropertyHandle);
			}
			else
			{
				// The property is not in a subcategory, so add it now
				auto& PropertyRow = CategoryBuilder.AddProperty(PropertyHandle);
				CustomizePropertyRowVisibility(LayoutBuilder, PropertyHandle, PropertyRow);

				if (OutDetailRowsByPropertyName)
				{
					OutDetailRowsByPropertyName->Add(Property->GetFName()) = &PropertyRow;
				}
			}
		}
	}

	//Add subgroups
	for (auto Iter = SubcategoryPropertiesMap.CreateConstIterator(); Iter; ++Iter)
	{
		const FString &GroupString = Iter.Key();
		const TArray<TSharedRef<IPropertyHandle>>& PropertyHandles = Iter.Value();
		IDetailGroup& Group = CategoryBuilder.AddGroup(FName(*GroupString), FText::FromString(GroupString));
		for (auto& PropertyHandle : PropertyHandles)
		{
			auto& PropertyRow = Group.AddPropertyRow(PropertyHandle);
			CustomizePropertyRowVisibility(LayoutBuilder, PropertyHandle, PropertyRow);

			if (OutDetailRowsByPropertyName && PropertyHandle->GetProperty())
			{
				OutDetailRowsByPropertyName->Add(PropertyHandle->GetProperty()->GetFName()) = &PropertyRow;
			}
		}
	}
}

void FFoliageTypeDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	const static FName ReapplyName("Reapply");
	const static FName PaintingName("Painting");
	const static FName PlacementName("Placement");
	const static FName ProceduralName("Procedural");
	const static FName InstanceSettingsName("InstanceSettings");

	FFoliageTypeCustomizationHelpers::HideFoliageCategory(DetailBuilder, ReapplyName);
	FFoliageTypeCustomizationHelpers::HideFoliageCategory(DetailBuilder, PaintingName);

	TMap<const FName, IDetailPropertyRow*> PropertyRowsByName;
	AddSubcategoryProperties(DetailBuilder, PlacementName, &PropertyRowsByName);

	// If enabled, show the properties for procedural placement
	if (GetDefault<UEditorExperimentalSettings>()->bProceduralFoliage)
	{
		AddSubcategoryProperties(DetailBuilder, ProceduralName);
	}
	else
	{
		FFoliageTypeCustomizationHelpers::HideFoliageCategory(DetailBuilder, ProceduralName);
	}
	
	AddSubcategoryProperties(DetailBuilder, InstanceSettingsName);

	// Custom visibility overrides for the scale axes
	TSharedPtr<IPropertyHandle> Scaling = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFoliageType, Scaling));

	FFoliageTypeCustomizationHelpers::ModifyFoliagePropertyRow(*PropertyRowsByName.Find(GET_MEMBER_NAME_CHECKED(UFoliageType, ScaleX)),
		TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateStatic(&FFoliageTypeCustomizationHelpers::GetScaleAxisVisibility, EAxis::X, Scaling)),
		TAttribute<bool>());

	FFoliageTypeCustomizationHelpers::ModifyFoliagePropertyRow(*PropertyRowsByName.Find(GET_MEMBER_NAME_CHECKED(UFoliageType, ScaleY)),
		TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateStatic(&FFoliageTypeCustomizationHelpers::GetScaleAxisVisibility, EAxis::Y, Scaling)),
		TAttribute<bool>());

	FFoliageTypeCustomizationHelpers::ModifyFoliagePropertyRow(*PropertyRowsByName.Find(GET_MEMBER_NAME_CHECKED(UFoliageType, ScaleZ)),
		TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateStatic(&FFoliageTypeCustomizationHelpers::GetScaleAxisVisibility, EAxis::Z, Scaling)),
		TAttribute<bool>());
}