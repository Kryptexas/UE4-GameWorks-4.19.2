// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "../BehaviorTreeEditorPrivatePCH.h"
#include "BlackboardSelectorDetails.h"

#define LOCTEXT_NAMESPACE "BlackboardSelectorDetails"

TSharedRef<IStructCustomization> FBlackboardSelectorDetails::MakeInstance()
{
	return MakeShareable( new FBlackboardSelectorDetails );
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FBlackboardSelectorDetails::CustomizeStructHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils )
{
	MyStructProperty = StructPropertyHandle;
	CacheBlackboardData();
	
	HeaderRow.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(SComboButton)
			.OnGetMenuContent(this, &FBlackboardSelectorDetails::OnGetKeyContent)
 			.ContentPadding(FMargin( 2.0f, 2.0f ))
			.IsEnabled_Static(&FBehaviorTreeDebugger::IsPIENotSimulating)
			.ButtonContent()
			[
				SNew(STextBlock) 
				.Text(this, &FBlackboardSelectorDetails::GetCurrentKeyDesc)
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
		];

	InitKeyFromProperty();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void FBlackboardSelectorDetails::CustomizeStructChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils )
{
}

const UBlackboardData* FBlackboardSelectorDetails::FindBlackboardAsset(UObject* InObj)
{
	for (UObject* TestOb = InObj; TestOb; TestOb = TestOb->GetOuter())
	{
		UBTNode* NodeOb = Cast<UBTNode>(TestOb);
		if (NodeOb)
		{
			return NodeOb->GetBlackboardAsset();
		}
	}

	return NULL;
}

void FBlackboardSelectorDetails::CacheBlackboardData()
{
	TSharedPtr<IPropertyHandleArray> MyFilterProperty = MyStructProperty->GetChildHandle(TEXT("AllowedTypes"))->AsArray();
	MyKeyNameProperty = MyStructProperty->GetChildHandle(TEXT("SelectedKeyName"));
	MyKeyIDProperty = MyStructProperty->GetChildHandle(TEXT("SelectedKeyID"));
	MyKeyClassProperty = MyStructProperty->GetChildHandle(TEXT("SelectedKeyType"));
	KeyValues.Reset();

	TArray<UBlackboardKeyType*> FilterObjects;

	uint32 NumElements = 0;
	FPropertyAccess::Result Result = MyFilterProperty->GetNumElements(NumElements);
	if (Result == FPropertyAccess::Success)
	{
		for (uint32 i = 0; i < NumElements; i++)
		{
			UObject* FilterOb;
			Result = MyFilterProperty->GetElement(i)->GetValue(FilterOb);
			if (Result == FPropertyAccess::Success)
			{
				UBlackboardKeyType* CasterFilterOb = Cast<UBlackboardKeyType>(FilterOb);
				if (CasterFilterOb)
				{
					FilterObjects.Add(CasterFilterOb);
				}
			}
		}
	}

	TArray<UObject*> MyObjects;
	MyStructProperty->GetOuterObjects(MyObjects);
	for (int32 iOb = 0; iOb < MyObjects.Num(); iOb++)
	{
		const UBlackboardData* BlackboardAsset = FindBlackboardAsset(MyObjects[iOb]);
		if (BlackboardAsset)
		{
			CachedBlackboardAsset = BlackboardAsset;

			TArray<FName> ProcessedNames;
			for (UBlackboardData* It = (UBlackboardData*)BlackboardAsset; It; It = It->Parent)
			{
				for (int32 iKey = 0; iKey < It->Keys.Num(); iKey++)
				{
					const FBlackboardEntry& EntryInfo = It->Keys[iKey];
					bool bIsKeyOverriden = ProcessedNames.Contains(EntryInfo.EntryName);
					bool bIsEntryAllowed = !bIsKeyOverriden && (EntryInfo.KeyType != NULL);

					ProcessedNames.Add(EntryInfo.EntryName);

					if (bIsEntryAllowed && FilterObjects.Num())
					{
						bool bFilterPassed = false;
						for (int32 iFilter = 0; iFilter < FilterObjects.Num(); iFilter++)
						{
							if (EntryInfo.KeyType->IsAllowedByFilter(FilterObjects[iFilter]))
							{
								bFilterPassed = true;
								break;
							}
						}

						bIsEntryAllowed = bFilterPassed;
					}

					if (bIsEntryAllowed)
					{
						KeyValues.AddUnique(EntryInfo.EntryName);
					}
				}
			}

			break;
		}
	}
}

void FBlackboardSelectorDetails::InitKeyFromProperty()
{
	FName KeyNameValue;
	FPropertyAccess::Result Result = MyKeyNameProperty->GetValue(KeyNameValue);
	if (Result == FPropertyAccess::Success)
	{
		const int32 KeyIdx = KeyValues.IndexOfByKey(KeyNameValue);
		if (KeyIdx == INDEX_NONE)
		{
			OnKeyComboChange(0);
		}
	}
}

TSharedRef<SWidget> FBlackboardSelectorDetails::OnGetKeyContent() const
{
	FMenuBuilder MenuBuilder(true, NULL);

	for (int32 i = 0; i < KeyValues.Num(); i++)
	{
		FUIAction ItemAction( FExecuteAction::CreateSP( this, &FBlackboardSelectorDetails::OnKeyComboChange, i ) );
		MenuBuilder.AddMenuEntry( FText::FromName( KeyValues[i] ), TAttribute<FText>(), FSlateIcon(), ItemAction);
	}

	return MenuBuilder.MakeWidget();
}

FString FBlackboardSelectorDetails::GetCurrentKeyDesc() const
{
	FName NameValue;
	MyKeyNameProperty->GetValue(NameValue);

	const int32 KeyIdx = KeyValues.IndexOfByKey(NameValue);
	return KeyValues.IsValidIndex(KeyIdx) ? KeyValues[KeyIdx].ToString() : NameValue.ToString();
}

void FBlackboardSelectorDetails::OnKeyComboChange(int32 Index)
{
	if (KeyValues.IsValidIndex(Index))
	{
		UBlackboardData* BlackboardAsset = CachedBlackboardAsset.Get();
		if (BlackboardAsset)
		{
			const uint8 KeyID = BlackboardAsset->GetKeyID(KeyValues[Index]);
			const UObject* KeyClass = BlackboardAsset->GetKeyType(KeyID);

			MyKeyClassProperty->SetValue(KeyClass);
			MyKeyIDProperty->SetValue(KeyID);

			MyKeyNameProperty->SetValue(KeyValues[Index]);
		}
	}
}

#undef LOCTEXT_NAMESPACE
