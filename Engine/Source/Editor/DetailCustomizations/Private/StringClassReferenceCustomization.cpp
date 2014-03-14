// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "StringClassReferenceCustomization.h"

void FStringClassReferenceCustomization::CustomizeStructHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils)
{
	// If we are part of an array, we need to take our meta-data from the array property
	const UProperty* MetaDataProperty = InStructPropertyHandle->GetProperty();
	if(InStructPropertyHandle->GetIndexInArray() != INDEX_NONE)
	{
		TSharedPtr<IPropertyHandle> ParentPropertyHandle = InStructPropertyHandle->GetParentHandle();
		check(ParentPropertyHandle.IsValid() && ParentPropertyHandle->AsArray().IsValid());
		MetaDataProperty = ParentPropertyHandle->GetProperty();
	}

	StructPropertyHandle = InStructPropertyHandle;

	ClassNamePropertyHandle = InStructPropertyHandle->GetChildHandle("ClassName");
	check(ClassNamePropertyHandle.IsValid());

	const FString& MetaClassName = MetaDataProperty->GetMetaData("MetaClass");
	const FString& RequiredInterfaceName = MetaDataProperty->GetMetaData("RequiredInterface");
	const bool bAllowAbstract = MetaDataProperty->GetBoolMetaData("AllowAbstract");
	const bool bIsBlueprintBaseOnly = MetaDataProperty->GetBoolMetaData("IsBlueprintBaseOnly");
	const bool bAllowNone = !(MetaDataProperty->PropertyFlags & CPF_NoClear);

	check(!MetaClassName.IsEmpty());
	const UClass* const MetaClass = StringToClass(MetaClassName);
	const UClass* const RequiredInterface = StringToClass(RequiredInterfaceName);

	HeaderRow
	.NameContent()
	[
		InStructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(250.0f)
	.MaxDesiredWidth(0.0f)
	[
		// Add a class entry box.  Even though this isn't an class entry, we will simulate one
		SNew(SClassPropertyEntryBox)
			.MetaClass(MetaClass)
			.RequiredInterface(RequiredInterface)
			.AllowAbstract(bAllowAbstract)
			.IsBlueprintBaseOnly(bIsBlueprintBaseOnly)
			.AllowNone(bAllowNone)
			.SelectedClass(this, &FStringClassReferenceCustomization::OnGetClass)
			.OnSetClass(this, &FStringClassReferenceCustomization::OnSetClass)
	];
}

void FStringClassReferenceCustomization::CustomizeStructChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils)
{
}

const UClass* FStringClassReferenceCustomization::OnGetClass() const
{
	FString ClassName;
	ClassNamePropertyHandle->GetValue(ClassName);

	// Do we have a valid cached class pointer?
	const UClass* Class = CachedClassPtr.Get();
	if(!Class || Class->GetPathName() != ClassName)
	{
		Class = StringToClass(ClassName);
		CachedClassPtr = Class;
	}
	return Class;
}

void FStringClassReferenceCustomization::OnSetClass(const UClass* NewClass)
{
	if(ClassNamePropertyHandle->SetValueFromFormattedString((NewClass) ? NewClass->GetPathName() : "None") == FPropertyAccess::Result::Success)
	{
		CachedClassPtr = NewClass;
	}
}

const UClass* FStringClassReferenceCustomization::StringToClass(const FString& ClassName)
{
	if(ClassName.IsEmpty() || ClassName == "None")
	{
		return nullptr;
	}

	const UClass* Class = FindObject<UClass>(ANY_PACKAGE, *ClassName);
	if(!Class)
	{
		Class = LoadObject<UClass>(nullptr, *ClassName);
	}
	return Class;
}
