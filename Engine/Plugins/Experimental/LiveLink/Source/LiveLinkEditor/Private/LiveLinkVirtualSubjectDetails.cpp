// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "LiveLinkVirtualSubjectDetails.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "STextComboBox.h"
#include "SCheckBox.h"
#include "GuidStructCustomization.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "LiveLinkVirtualSubjectDetails"

ULiveLinkVirtualSubjectDetails::ULiveLinkVirtualSubjectDetails(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void ULiveLinkVirtualSubjectDetails::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	const bool bIsInteractive = PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive;
	const bool bIsArrayAdd = PropertyChangedEvent.ChangeType == EPropertyChangeType::ArrayAdd;

	if (!bIsArrayAdd && !bIsInteractive)
	{
		Client->UpdateVirtualSubjectProperties(SubjectKey, VirtualSubjectProxy);
	}
}

FLiveLinkVirtualSubjectDetailCustomization::FLiveLinkVirtualSubjectDetailCustomization(FLiveLinkClient* InClient)
	: Client(InClient)
	, MyDetailsBuilder(nullptr)
{
	check(Client);
}

void FLiveLinkVirtualSubjectDetailCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	MyDetailsBuilder = &DetailBuilder;
	
	/*IDetailLayoutBuilder* CaptuerBuilder = &DetailBuilder;
	FSimpleDelegate RefreshDelegate = FSimpleDelegate::CreateLambda([CaptuerBuilder]()
	{ 
		RefreshDetails(CaptuerBuilder);
	});*/

	SourceGuidPropertyHandle = DetailBuilder.GetProperty(TEXT("VirtualSubjectProxy.Source"));
	//SourceGuidPropertyHandle->SetOnChildPropertyValueChanged(RefreshDelegate);

	DetailBuilder.HideProperty(SourceGuidPropertyHandle);
	SubjectsPropertyHandle = DetailBuilder.GetProperty(TEXT("VirtualSubjectProxy.Subjects"));
	DetailBuilder.HideProperty(SubjectsPropertyHandle);
	TSharedPtr<IPropertyHandle> ProxyPropertyHandle = DetailBuilder.GetProperty(TEXT("VirtualSubjectProxy"));
	DetailBuilder.HideProperty(ProxyPropertyHandle);

	SourceListItems.Reset();

 	SourceGuids = Client->GetSourceEntries();
	{
		FLiveLinkClient* CaptureClient = Client; // Local variable for lambda
		SourceGuids.RemoveAll([CaptureClient](const FGuid& Source) { return !CaptureClient->ShowSourceInUI(Source); });
	}

	TArray<void*> GuidRawData;
	SourceGuidPropertyHandle->AccessRawData(GuidRawData);

	check(GuidRawData.Num() == 1);
	check(GuidRawData[0] != nullptr);

	const FGuid& CurrentSourceGuid = *((FGuid*)GuidRawData[0]);

	const int32 FoundIndex = SourceGuids.IndexOfByKey(CurrentSourceGuid);

	for (const FGuid& SourceGuid : SourceGuids)
	{
		FText Source = Client->GetSourceTypeForEntry(SourceGuid);
		SourceListItems.Emplace(MakeShared<FString>(Source.ToString()));
	}

	TSharedPtr<FString> CurrentSelection;
	if (FoundIndex != INDEX_NONE)
	{
		CurrentSelection = SourceListItems[FoundIndex];
	}

	// create combo box
	IDetailCategoryBuilder& SourcePropertyGroup = DetailBuilder.EditCategory(*SourceGuidPropertyHandle->GetMetaData(TEXT("Category")));
	SourcePropertyGroup.AddCustomRow(LOCTEXT("SourceGuidTitleLabel", "Source Guid"))
	.NameContent()
	[
		SourceGuidPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(125.f * 2.f)
	[
		SNew(STextComboBox)
		.OptionsSource(&SourceListItems)
		.OnSelectionChanged(this, &FLiveLinkVirtualSubjectDetailCustomization::OnSourceChanged)
		.InitiallySelectedItem(CurrentSelection)
	];

	SubjectsListItems.Reset();

	if (FoundIndex != INDEX_NONE)
	{
		TArray<FLiveLinkSubjectKey> SubjectKeys = Client->GetSubjects();
		for (const FLiveLinkSubjectKey& SubjectKey : SubjectKeys)
		{
			if (SubjectKey.Source == SourceGuids[FoundIndex])
			{
				SubjectsListItems.Add(MakeShared<FName>(SubjectKey.SubjectName));
			}
		}
	}

	IDetailCategoryBuilder& SubjectPropertyGroup = DetailBuilder.EditCategory(*SubjectsPropertyHandle->GetMetaData(TEXT("Category")));
	SubjectPropertyGroup.AddCustomRow(LOCTEXT("SubjectsTitleLabel", "Subjects"))
	.NameContent()
	[
		SubjectsPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		SAssignNew(SubjectsListView, SListView<FSubjectEntryPtr>)
		.ListItemsSource(&SubjectsListItems)
		.OnGenerateRow(this, &FLiveLinkVirtualSubjectDetailCustomization::OnGenerateWidgetForSubjectItem)
	];
}

void FLiveLinkVirtualSubjectDetailCustomization::OnSourceChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	int32 ItemIndex = SourceListItems.Find(NewSelection);
	if (ItemIndex != INDEX_NONE)
	{
		FGuid& ChosenGuid = SourceGuids[ItemIndex];

		WriteGuidToProperty(SourceGuidPropertyHandle, ChosenGuid);

		MyDetailsBuilder->ForceRefreshDetails();
	}
}

int32 GetArrayPropertyIndex(TSharedPtr<IPropertyHandleArray> ArrayProperty, FName ItemToSearchFor, uint32 NumItems)
{
	for (uint32 Index = 0; Index < NumItems; ++Index)
	{
		FName TestValue;
		ArrayProperty->GetElement(Index)->GetValue(TestValue);
		if (TestValue == ItemToSearchFor)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

TSharedRef<ITableRow> FLiveLinkVirtualSubjectDetailCustomization::OnGenerateWidgetForSubjectItem(FSubjectEntryPtr InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	FLiveLinkVirtualSubjectDetailCustomization* CaptureThis = this;

	return SNew(STableRow<FSubjectEntryPtr>, OwnerTable)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([CaptureThis, InItem]()
				{
					const TSharedPtr<IPropertyHandleArray> SubjectsArrayPropertyHandle = CaptureThis->SubjectsPropertyHandle->AsArray();
					uint32 NumItems;
					SubjectsArrayPropertyHandle->GetNumElements(NumItems);

					int32 RemoveIndex = GetArrayPropertyIndex(SubjectsArrayPropertyHandle, *InItem, NumItems);

					return RemoveIndex != INDEX_NONE ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([CaptureThis, InItem](const ECheckBoxState NewState)
				{
					const TSharedPtr<IPropertyHandleArray> SubjectsArrayPropertyHandle = CaptureThis->SubjectsPropertyHandle->AsArray();
					uint32 NumItems;
					SubjectsArrayPropertyHandle->GetNumElements(NumItems);

					if (NewState == ECheckBoxState::Checked)
					{
						FFormatNamedArguments Arguments;
						Arguments.Add(TEXT("SubjectName"), FText::FromName(*InItem));
						FScopedTransaction Transaction(FText::Format(LOCTEXT("AddSourceToVirtualSubject", "Add {SubjectName} to virtual subject"), Arguments));
						FPropertyAccess::Result Result = SubjectsArrayPropertyHandle->AddItem();
						check(Result == FPropertyAccess::Success);
						SubjectsArrayPropertyHandle->GetElement(NumItems)->SetValue(*InItem, EPropertyValueSetFlags::NotTransactable);
					}
					else
					{
						int32 RemoveIndex = GetArrayPropertyIndex(SubjectsArrayPropertyHandle, *InItem, NumItems);
						if (RemoveIndex != INDEX_NONE)
						{
							SubjectsArrayPropertyHandle->DeleteItem(RemoveIndex);
						}
					}
				})
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			[
				SNew(STextBlock)
				.Text(FText::FromName(*InItem))
			]
		];
}
#undef LOCTEXT_NAMESPACE