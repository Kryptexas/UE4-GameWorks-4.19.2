// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MaterialProxySettingsCustomizations.h"
#include "Misc/Attribute.h"
#include "UObject/UnrealType.h"
#include "Engine/MaterialMerging.h"
#include "PropertyHandle.h"
#include "IDetailChildrenBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"

#define LOCTEXT_NAMESPACE "MaterialProxySettingsCustomizations"

TSharedRef<IPropertyTypeCustomization> FMaterialProxySettingsCustomizations::MakeInstance()
{
	return MakeShareable(new FMaterialProxySettingsCustomizations);
}

void FMaterialProxySettingsCustomizations::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	HeaderRow.
		NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			StructPropertyHandle->CreatePropertyValueWidget()
		];
}

void FMaterialProxySettingsCustomizations::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// Retrieve structure's child properties
	uint32 NumChildren;
	StructPropertyHandle->GetNumChildren( NumChildren );	
	TMap<FName, TSharedPtr< IPropertyHandle > > PropertyHandles;	
	for( uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex )
	{
		TSharedRef<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle( ChildIndex ).ToSharedRef();
		const FName PropertyName = ChildHandle->GetProperty()->GetFName();

		PropertyHandles.Add(PropertyName, ChildHandle);
	}

	// Retrieve special case properties
	EnumHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FMaterialProxySettings, TextureSizingType));
	TextureSizeHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FMaterialProxySettings, TextureSize));
	PropertyTextureSizeHandles.Add(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FMaterialProxySettings, DiffuseTextureSize)));
	PropertyTextureSizeHandles.Add(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FMaterialProxySettings, NormalTextureSize)));
	PropertyTextureSizeHandles.Add(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FMaterialProxySettings, MetallicTextureSize)));
	PropertyTextureSizeHandles.Add(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FMaterialProxySettings, RoughnessTextureSize)));
	PropertyTextureSizeHandles.Add(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FMaterialProxySettings, SpecularTextureSize)));
	PropertyTextureSizeHandles.Add(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FMaterialProxySettings, EmissiveTextureSize)));
	PropertyTextureSizeHandles.Add(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FMaterialProxySettings, OpacityTextureSize)));
	PropertyTextureSizeHandles.Add(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FMaterialProxySettings, OpacityMaskTextureSize)));
	PropertyTextureSizeHandles.Add(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FMaterialProxySettings, AmbientOcclusionTextureSize)));

	if (PropertyHandles.Contains(GET_MEMBER_NAME_CHECKED(FMaterialProxySettings, MaterialMergeType)))
	{
		MergeTypeHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FMaterialProxySettings, MaterialMergeType));
	}
	
	GutterSpaceHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FMaterialProxySettings, GutterSpace));

	auto Parent = StructPropertyHandle->GetParentHandle();
	
	for( auto Iter(PropertyHandles.CreateConstIterator()); Iter; ++Iter  )
	{
		// Handle special property cases (done inside the loop to maintain order according to the struct
		if (PropertyTextureSizeHandles.Contains(Iter.Value()))
		{
			IDetailPropertyRow& SizeRow = ChildBuilder.AddProperty(Iter.Value().ToSharedRef());
			SizeRow.Visibility(TAttribute<EVisibility>(this, &FMaterialProxySettingsCustomizations::AreManualOverrideTextureSizesEnabled));
		}
		else if (Iter.Value() == TextureSizeHandle)
		{
			IDetailPropertyRow& SettingsRow = ChildBuilder.AddProperty(Iter.Value().ToSharedRef());
			SettingsRow.Visibility(TAttribute<EVisibility>(this, &FMaterialProxySettingsCustomizations::IsTextureSizeEnabled));
		}
		else if (Iter.Value() == GutterSpaceHandle)
		{
			IDetailPropertyRow& SettingsRow = ChildBuilder.AddProperty(Iter.Value().ToSharedRef());
			SettingsRow.Visibility(TAttribute<EVisibility>(this, &FMaterialProxySettingsCustomizations::IsSimplygonMaterialMergingVisible));
		}
		// Do not show the merge type property
		else if (Iter.Value() != MergeTypeHandle)
		{
			IDetailPropertyRow& SettingsRow = ChildBuilder.AddProperty(Iter.Value().ToSharedRef());
		}
	}	

}

EVisibility FMaterialProxySettingsCustomizations::AreManualOverrideTextureSizesEnabled() const
{
	uint8 TypeValue;
	EnumHandle->GetValue(TypeValue);

	if (TypeValue == TextureSizingType_UseManualOverrideTextureSize)
	{
		return EVisibility::Visible;
	}

	return EVisibility::Hidden;
}

EVisibility FMaterialProxySettingsCustomizations::IsTextureSizeEnabled() const
{
	uint8 TypeValue;
	EnumHandle->GetValue(TypeValue);

	if (TypeValue == TextureSizingType_UseSimplygonAutomaticSizing || TypeValue == TextureSizingType_UseManualOverrideTextureSize)
	{		
		return EVisibility::Hidden;
	}

	return EVisibility::Visible;	
}

EVisibility FMaterialProxySettingsCustomizations::IsSimplygonMaterialMergingVisible() const
{
	uint8 MergeType = EMaterialMergeType::MaterialMergeType_Default;
	if (MergeTypeHandle.IsValid())
	{
		MergeTypeHandle->GetValue(MergeType);
	}

	return ( MergeType == EMaterialMergeType::MaterialMergeType_Simplygon ) ? EVisibility::Visible : EVisibility::Hidden;
}

#undef LOCTEXT_NAMESPACE
