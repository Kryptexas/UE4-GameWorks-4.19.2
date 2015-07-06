// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "SkeletalControlNodeDetails.h"

#define LOCTEXT_NAMESPACE "SkeletalControlNodeDetails"

/////////////////////////////////////////////////////
// FSkeletalControlNodeDetails 

TSharedRef<IDetailCustomization> FSkeletalControlNodeDetails::MakeInstance()
{
	return MakeShareable(new FSkeletalControlNodeDetails());
}

void FSkeletalControlNodeDetails::CustomizeDetails(class IDetailLayoutBuilder& DetailBuilder)
{
	DetailCategory = &DetailBuilder.EditCategory("PinOptions");
	TSharedRef<IPropertyHandle> AvailablePins = DetailBuilder.GetProperty("ShowPinForProperties");
	TSharedPtr<IPropertyHandleArray> ArrayProperty = AvailablePins->AsArray();

	TSet<FName> UniqueCategoryNames;
	{
		int32 NumElements = 0;
		{
			uint32 UnNumElements = 0;
			if (ArrayProperty.IsValid() && (FPropertyAccess::Success == ArrayProperty->GetNumElements(UnNumElements)))
			{
				NumElements = static_cast<int32>(UnNumElements);
			}
		}
		for (int32 Index = 0; Index < NumElements; ++Index)
		{
			auto StructPropHandle = ArrayProperty->GetElement(Index);
			auto CategoryPropeHandle = StructPropHandle->GetChildHandle("CategoryName");
			check(CategoryPropeHandle.IsValid());
			FName CategoryNameValue;
			const auto Result = CategoryPropeHandle->GetValue(CategoryNameValue);
			if (ensure(FPropertyAccess::Success == Result))
			{
				UniqueCategoryNames.Add(CategoryNameValue);
			}
		}
	}

	//@TODO: Shouldn't show this if the available pins array is empty!
	const bool bGenerateHeader = true;
	const bool bDisplayResetToDefault = false;
	const bool bDisplayElementNum = false;
	const bool bForAdvanced = false;
	for (auto CategoryName : UniqueCategoryNames)
	{
		TSharedRef<FDetailArrayBuilder> AvailablePinsBuilder = MakeShareable(new FDetailArrayBuilder(AvailablePins, bGenerateHeader, bDisplayResetToDefault, bDisplayElementNum));
		AvailablePinsBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateSP(this, &FSkeletalControlNodeDetails::OnGenerateElementForPropertyPin, CategoryName));
		AvailablePinsBuilder->SetDisplayName((CategoryName == NAME_None) ? LOCTEXT("DefaultCategory", "Default Category") : FText::FromName(CategoryName));
		DetailCategory->AddCustomBuilder(AvailablePinsBuilder, bForAdvanced);
	}
}

void FSkeletalControlNodeDetails::OnGenerateElementForPropertyPin(TSharedRef<IPropertyHandle> ElementProperty, int32 ElementIndex, IDetailChildrenBuilder& ChildrenBuilder, FName CategoryName)
{
	{
		auto CategoryPropeHandle = ElementProperty->GetChildHandle("CategoryName");
		check(CategoryPropeHandle.IsValid());
		FName CategoryNameValue;
		const auto Result = CategoryPropeHandle->GetValue(CategoryNameValue);
		const bool bProperCategory = ensure(FPropertyAccess::Success == Result) && (CategoryNameValue == CategoryName);
		if (!bProperCategory)
		{
			return;
		}
	}

	TSharedPtr<IPropertyHandle> PropertyNameHandle = ElementProperty->GetChildHandle("PropertyFriendlyName");
	FText PropertyFriendlyName(LOCTEXT("Invalid", "Invalid"));

	if (PropertyNameHandle.IsValid())
	{
		FString DisplayFriendlyName;
		switch (PropertyNameHandle->GetValue(/*out*/ DisplayFriendlyName))
		{
		case FPropertyAccess::Success:
		{
			PropertyFriendlyName = FText::FromString(DisplayFriendlyName);

			//DetailBuilder.EditCategory
			//DisplayNameAsName
		}
			break;
		case FPropertyAccess::MultipleValues:
			ChildrenBuilder.AddChildContent(FText::GetEmpty())
				[
					SNew(STextBlock).Text(LOCTEXT("OnlyWorksInSingleSelectMode", "Multiple types selected"))
				];
			return;
		case FPropertyAccess::Fail:
		default:
			check(false);
			break;
		}
	}
	
	ChildrenBuilder.AddChildContent( PropertyFriendlyName )
	[
		SNew(SProperty, ElementProperty->GetChildHandle("bShowPin"))
		.DisplayName(PropertyFriendlyName)
	];
}


/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE

