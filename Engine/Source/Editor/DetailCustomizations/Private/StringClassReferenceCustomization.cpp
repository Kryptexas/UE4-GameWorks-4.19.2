// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "StringClassReferenceCustomization.h"

void FStringClassReferenceCustomization::CustomizeStructHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils)
{
	StructPropertyHandle = InStructPropertyHandle;

	ClassNamePropertyHandle = InStructPropertyHandle->GetChildHandle("ClassName");
	check(ClassNamePropertyHandle.IsValid());

	const FString& MetaClassName = InStructPropertyHandle->GetProperty()->GetMetaData("MetaClass");
	const FString& RequiredInterfaceName = InStructPropertyHandle->GetProperty()->GetMetaData("RequiredInterface");
	const bool bAllowAbstract = InStructPropertyHandle->GetProperty()->GetBoolMetaData("AllowAbstract");
	const bool bIsBlueprintBaseOnly = InStructPropertyHandle->GetProperty()->GetBoolMetaData("IsBlueprintBaseOnly");
	const bool bAllowNone = !(InStructPropertyHandle->GetProperty()->PropertyFlags & CPF_NoClear);

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
