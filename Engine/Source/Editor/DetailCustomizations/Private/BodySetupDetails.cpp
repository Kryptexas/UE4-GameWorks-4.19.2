// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "BodySetupDetails.h"
#include "ScopedTransaction.h"
#include "ObjectEditorUtils.h"
#include "IDocumentation.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "SNumericEntryBox.h"

#define LOCTEXT_NAMESPACE "BodySetupDetails"

TSharedRef<IDetailCustomization> FBodySetupDetails::MakeInstance()
{
	return MakeShareable( new FBodySetupDetails );
}

void FBodySetupDetails::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
	// Customize collision section
	{
		if ( DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UBodySetup, DefaultInstance))->IsValidHandle() )
		{
			DetailBuilder.GetObjectsBeingCustomized(ObjectsCustomized);
			BodyInstanceCustomizationHelper = MakeShareable(new FBodyInstanceCustomizationHelper(ObjectsCustomized));
			BodyInstanceCustomizationHelper->CustomizeDetails(DetailBuilder, DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UBodySetup, DefaultInstance)));

			IDetailCategoryBuilder& CollisionCategory = DetailBuilder.EditCategory("Collision");

			TSharedPtr<IPropertyHandle> BodyInstanceHandler = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UBodySetup, DefaultInstance));
			DetailBuilder.HideProperty(BodyInstanceHandler);

			TSharedPtr<IPropertyHandle> CollisionTraceHandler = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UBodySetup, CollisionTraceFlag));
			DetailBuilder.HideProperty(CollisionTraceHandler);

			// add physics properties to physics category
			uint32 NumChildren = 0;
			BodyInstanceHandler->GetNumChildren(NumChildren);

			static const FName CollisionCategoryName(TEXT("Collision"));

			// add all properties of this now - after adding 
			for (uint32 ChildIndex=0; ChildIndex < NumChildren; ++ChildIndex)
			{
				TSharedPtr<IPropertyHandle> ChildProperty = BodyInstanceHandler->GetChildHandle(ChildIndex);
				FName CategoryName = FObjectEditorUtils::GetCategoryFName(ChildProperty->GetProperty());
				if (CategoryName == CollisionCategoryName)
				{
					CollisionCategory.AddProperty(ChildProperty);
				}
			}

			const bool bInPhat = ObjectsCustomized.Num() && (Cast<USkeletalBodySetup>(ObjectsCustomized[0].Get()) != nullptr);
			if(bInPhat)
			{
				//AddPhysicalAnimation(DetailBuilder);
			}
		}
	}
}

TSharedRef<IDetailCustomization> FSkeletalBodySetupDetails::MakeInstance()
{
	return MakeShareable(new FSkeletalBodySetupDetails);
}


void FSkeletalBodySetupDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilder.GetObjectsBeingCustomized(ObjectsCustomized);

	IDetailCategoryBuilder& Cat = DetailBuilder.EditCategory(TEXT("PhysicalAnimation"));

	const TArray<TWeakObjectPtr<UObject>>& ObjectsCustomizedLocal = ObjectsCustomized;
	auto PhysAnimEditable = [ObjectsCustomizedLocal]() -> bool
	{
		bool bVisible = ObjectsCustomizedLocal.Num() > 0;
		for (TWeakObjectPtr<UObject> WeakObj : ObjectsCustomizedLocal)
		{
			if (USkeletalBodySetup* BS = Cast<USkeletalBodySetup>(WeakObj.Get()))
			{
				if (!BS->FindPhysicalAnimationProfile(BS->GetCurrentPhysicalAnimationProfileName()))
				{
					bVisible = false;
					break;
				}
			}
			else
			{
				bVisible = false;
				break;
			}
		}

		return bVisible;
	};

	auto AddProfileLambda = [ObjectsCustomizedLocal]() 
	{
		const FScopedTransaction Transaction(LOCTEXT("AddProfile", "Add Physical Animation Profile"));
		bool bVisible = ObjectsCustomizedLocal.Num() > 0;
		for (TWeakObjectPtr<UObject> WeakObj : ObjectsCustomizedLocal)
		{
			if (USkeletalBodySetup* BS = Cast<USkeletalBodySetup>(WeakObj.Get()))
			{
				FName ProfileName = BS->GetCurrentPhysicalAnimationProfileName();
				if (!BS->FindPhysicalAnimationProfile(ProfileName))
				{
					BS->AddPhysicalAnimationProfile(ProfileName);
				}
			}
		}

		return FReply::Handled();;
	};

	auto DeleteProfileLambda = [ObjectsCustomizedLocal]()
	{
		const FScopedTransaction Transaction(LOCTEXT("RemoveProfile", "Remove Physical Animation Profile"));
		bool bVisible = ObjectsCustomizedLocal.Num() > 0;
		for (TWeakObjectPtr<UObject> WeakObj : ObjectsCustomizedLocal)
		{
			if (USkeletalBodySetup* BS = Cast<USkeletalBodySetup>(WeakObj.Get()))
			{
				FName ProfileName = BS->GetCurrentPhysicalAnimationProfileName();
				BS->RemovePhysicalAnimationProfile(ProfileName);
			}
		}

		return FReply::Handled();;
	};

	TAttribute<EVisibility> PhysAnimVisible = TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateLambda([PhysAnimEditable]()
	{
		return PhysAnimEditable() == true ? EVisibility::Visible : EVisibility::Collapsed;
	}));

	TAttribute<EVisibility> NewPhysAnimButtonVisible = TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateLambda([PhysAnimEditable]()
	{
		return PhysAnimEditable() == true ? EVisibility::Collapsed : EVisibility::Visible;
	}));

	TAttribute<bool> PhysAnimButtonEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([ObjectsCustomizedLocal]()
	{
		for (TWeakObjectPtr<UObject> WeakObj : ObjectsCustomizedLocal)
		{
			if (USkeletalBodySetup* BS = Cast<USkeletalBodySetup>(WeakObj.Get()))
			{
				if (BS->GetCurrentPhysicalAnimationProfileName() == NAME_None)
				{
					return false;
				}
			}
		}

		return true;
	}));

	Cat.AddCustomRow(LOCTEXT("NewPhysAnim", "NewPhysicalAnimationProfile"))
	.Visibility(NewPhysAnimButtonVisible)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.HAlign(HAlign_Center)
		.Padding(0, 10, 0, 10)
		[
			SNew(SBox)
			.WidthOverride(128)
			.HeightOverride(32)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.IsEnabled(PhysAnimButtonEnabled)
				.OnClicked_Lambda(AddProfileLambda)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("NewPhysAnimButton", "Add To Current Profile"))
					.ToolTipText(LOCTEXT("NewPhysAnimButtonToolTip", "Add to current physical animation profile."))
				]
			]	
		]
		+ SVerticalBox::Slot()
		.HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NewPhysAnimLabel", "At least one body is not in the current physical animation profile."))
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.ToolTipText(LOCTEXT("NewPhysAnimLabelTooltip", "Could not found in the current physical animation profile."))
		]
	];

	TSharedPtr<IPropertyHandle> PhysicalAnimationProfile = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(USkeletalBodySetup, CurrentPhysicalAnimationProfile));
	PhysicalAnimationProfile->MarkHiddenByCustomization();

	uint32 NumChildren = 0;
	TSharedPtr<IPropertyHandle> ProfileData = PhysicalAnimationProfile->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPhysicalAnimationProfile, PhysicalAnimationData));
	ProfileData->GetNumChildren(NumChildren);
	for(uint32 ChildIdx = 0; ChildIdx < NumChildren; ++ChildIdx)
	{
		TSharedPtr<IPropertyHandle> Child = ProfileData->GetChildHandle(ChildIdx);
		if(!Child->IsCustomized())
		{
			Cat.AddProperty(Child)
			.Visibility(PhysAnimVisible);
		}
	}

	Cat.AddCustomRow(LOCTEXT("DeletePhysAnim", "DeletePhysicalAnimationProfile"))
	.Visibility(PhysAnimVisible)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.Padding(0, 10, 0, 10)
		.HAlign(HAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(128)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.OnClicked_Lambda(DeleteProfileLambda)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("DeletePhysAnimButton", "Remove From Current Profile"))
					.ToolTipText(LOCTEXT("DeletePhysAnimButtonToolTip", "Removes the selected body from the current physical animation profile."))
				]
			]
		]
		
	];
}

#undef LOCTEXT_NAMESPACE

