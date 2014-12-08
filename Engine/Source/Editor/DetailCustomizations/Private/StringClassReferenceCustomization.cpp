// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "StringClassReferenceCustomization.h"
#include "EditorClassUtils.h"

void FStringClassReferenceCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	StructPropertyHandle = InStructPropertyHandle;

	ClassNamePropertyHandle = InStructPropertyHandle->GetChildHandle("AssetLongPathname");
	check(ClassNamePropertyHandle.IsValid());

	const FString& MetaClassName = StructPropertyHandle->GetMetaData("MetaClass");
	const FString& RequiredInterfaceName = StructPropertyHandle->GetMetaData("RequiredInterface");
	const bool bAllowAbstract = StructPropertyHandle->GetBoolMetaData("AllowAbstract");
	const bool bIsBlueprintBaseOnly = StructPropertyHandle->GetBoolMetaData("IsBlueprintBaseOnly");
	const bool bAllowNone = !(StructPropertyHandle->GetMetaDataProperty()->PropertyFlags & CPF_NoClear);

	check(!MetaClassName.IsEmpty());
	const UClass* const MetaClass = FEditorClassUtils::GetClassFromString(MetaClassName);
	const UClass* const RequiredInterface = FEditorClassUtils::GetClassFromString(RequiredInterfaceName);

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

void FStringClassReferenceCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}

const UClass* FStringClassReferenceCustomization::OnGetClass() const
{
	FString ClassName;
	ClassNamePropertyHandle->GetValueAsFormattedString(ClassName);

	// Do we have a valid cached class pointer?
	const UClass* Class = CachedClassPtr.Get();
	if(!Class || Class->GetPathName() != ClassName)
	{
		Class = FEditorClassUtils::GetClassFromString(ClassName);
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