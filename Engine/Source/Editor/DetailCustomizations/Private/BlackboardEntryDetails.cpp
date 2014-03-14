// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "BlackboardEntryDetails.h"

#define LOCTEXT_NAMESPACE "BlackboardEntryDetails"

TSharedRef<IStructCustomization> FBlackboardEntryDetails::MakeInstance()
{
	return MakeShareable( new FBlackboardEntryDetails );
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FBlackboardEntryDetails::CustomizeStructHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils )
{
	MyStructProperty = StructPropertyHandle;
	MyNameProperty = MyStructProperty->GetChildHandle(TEXT("EntryName"));
	MyValueProperty = MyStructProperty->GetChildHandle(TEXT("KeyType"));

	HeaderRow.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MaxDesiredWidth(400.0f)
		[
			SNew(STextBlock)
			.Text(this, &FBlackboardEntryDetails::GetHeaderDesc)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void FBlackboardEntryDetails::CustomizeStructChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils )
{
	if (StructPropertyHandle->IsValidHandle())
	{
		StructBuilder.AddChildProperty(MyNameProperty.ToSharedRef());
		StructBuilder.AddChildProperty(MyValueProperty.ToSharedRef());
	}
}

FString FBlackboardEntryDetails::GetShortTypeName(const UObject* Ob) const
{
	if (Ob->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
	{
		return Ob->GetClass()->GetName().LeftChop(2);
	}

	FString TypeDesc = Ob->GetClass()->GetName();
	const int32 ShortNameIdx = TypeDesc.Find(TEXT("_"));
	if (ShortNameIdx != INDEX_NONE)
	{
		TypeDesc = TypeDesc.Mid(ShortNameIdx + 1);
	}

	return TypeDesc;
}

FString FBlackboardEntryDetails::GetHeaderDesc() const
{
	FString StrValue;
	UObject* ObValue;

	if (MyNameProperty->GetValue(StrValue) == FPropertyAccess::Success &&
		MyValueProperty->GetValue(ObValue) == FPropertyAccess::Success)
	{
		FString ExtraTypeData = ObValue ? ((UBlackboardKeyType*)ObValue)->DescribeSelf() : FString();
		FString Desc = FString::Printf(TEXT("%s = %s"),
			StrValue.Len() ? *StrValue : TEXT("??"),
			ObValue ? *GetShortTypeName(ObValue) : TEXT("??"));

		if (ExtraTypeData.Len())
		{
			Desc += TEXT(": ");
			Desc += ExtraTypeData;
		}

		return Desc;
	}

	return FString("??");
}

#undef LOCTEXT_NAMESPACE
