// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "FoliageEditModule.h"
#include "PropertyEditing.h"
#include "FoliageTypeDetails.h"

TSharedRef<IDetailCustomization> FFoliageTypeDetails::MakeInstance()
{
	return MakeShareable(new FFoliageTypeDetails());

}

void AddSubcategoryProperties(IDetailCategoryBuilder& CategoryBuilder)
{
	TArray<TSharedRef<IPropertyHandle>> CategoryProperties;
	CategoryBuilder.GetDefaultProperties(CategoryProperties, true, true);

	//Build map of subcategories to properties
	const FName SubcategoryName("Subcategory");
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
			Group.AddPropertyRow(PropertyHandle);
		}
	}
}

void FFoliageTypeDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	AddSubcategoryProperties(DetailBuilder.EditCategory("General"));
	AddSubcategoryProperties(DetailBuilder.EditCategory("Ecosystem"));
	AddSubcategoryProperties(DetailBuilder.EditCategory("Painting"));
}