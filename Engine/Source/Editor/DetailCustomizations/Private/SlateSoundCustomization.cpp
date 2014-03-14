// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "SlateSoundCustomization.h"

TSharedRef<IStructCustomization> FSlateSoundStructCustomization::MakeInstance() 
{
	return MakeShareable(new FSlateSoundStructCustomization());
}

void FSlateSoundStructCustomization::CustomizeStructHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils)
{
	TSharedPtr<IPropertyHandle> ResourceObjectProperty = StructPropertyHandle->GetChildHandle(TEXT("ResourceObject"));
	check(ResourceObjectProperty.IsValid());

	TArray<void*> StructPtrs;
	StructPropertyHandle->AccessRawData(StructPtrs);
	for(auto It = StructPtrs.CreateConstIterator(); It; ++It)
	{
		FSlateSound* const SlateSound = reinterpret_cast<FSlateSound*>(*It);
		SlateSoundStructs.Add(SlateSound);
	}

	HeaderRow
	.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(250.0f)
	[
		SNew(SObjectPropertyEntryBox)
		.PropertyHandle(ResourceObjectProperty)
		.AllowedClass(USoundBase::StaticClass())
		.OnObjectChanged(this, &FSlateSoundStructCustomization::OnObjectChanged)
	];
}

void FSlateSoundStructCustomization::CustomizeStructChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils)
{
}

void FSlateSoundStructCustomization::OnObjectChanged(const UObject*)
{
	// The object has been updated in the editor, so strip out the legacy data now so that the two don't conflict
	for(auto It = SlateSoundStructs.CreateConstIterator(); It; ++It)
	{
		FSlateSound* const SlateSound = *It;
		SlateSound->StripLegacyData_DEPRECATED();
	}
}
