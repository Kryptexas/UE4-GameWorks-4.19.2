// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Customization/BlendSampleDetails.h"

#include "UObject/ObjectMacros.h"
#include "UObject/Class.h"
#include "IDetailsView.h"
#include "EditorStyleSet.h"
#include "Misc/StringAssetReference.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "PropertyHandle.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"

#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SNumericEntryBox.h"

#include "Animation/AnimSequence.h"
#include "Animation/BlendSpaceBase.h"
#include "Animation/BlendSpace1D.h"
#include "SAnimationBlendSpaceGridWidget.h"
#include "PropertyCustomizationHelpers.h"

#include "PackageTools.h"
#include "IDetailGroup.h"

#define LOCTEXT_NAMESPACE "BlendSampleDetails"

FBlendSampleDetails::FBlendSampleDetails(const UBlendSpaceBase* InBlendSpace, class SBlendSpaceGridWidget* InGridWidget)
	: BlendSpace(InBlendSpace)
	, GridWidget(InGridWidget)
{
	// Retrieve the additive animation type enum
	const UEnum* AdditiveTypeEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EAdditiveAnimationType"), true);
	// For each type check whether or not the blend space is compatible with it and cache the result
	for (int32 TypeValue = 0; TypeValue < (int32)EAdditiveAnimationType::AAT_MAX; ++TypeValue)
	{
		EAdditiveAnimationType Type = (EAdditiveAnimationType)TypeValue;
		// In case of non additive type make sure the blendspace is made up out of non additive samples only
		const bool bAdditiveFlag = (Type == EAdditiveAnimationType::AAT_None) ? !BlendSpace->IsValidAdditive() : BlendSpace->IsValidAdditive() && BlendSpace->IsValidAdditiveType(Type);		
		bValidAdditiveTypes.Add(AdditiveTypeEnum->GetNameByValue(TypeValue).ToString(), bAdditiveFlag);
	}
}

void FBlendSampleDetails::CustomizeDetails(class IDetailLayoutBuilder& DetailBuilder)
{
	static const FName CategoryName = FName("BlendSample");
	IDetailCategoryBuilder& CategoryBuilder = DetailBuilder.EditCategory(CategoryName);

	// Hide default properties
	TArray<TSharedRef<IPropertyHandle> > DefaultProperties;
	CategoryBuilder.GetDefaultProperties(DefaultProperties);

	// Hide all default properties
	for (TSharedRef<IPropertyHandle> Property : DefaultProperties)
	{
		Property->MarkHiddenByCustomization();		
	}
	
	TArray < TSharedPtr<FStructOnScope>> Structs;
	DetailBuilder.GetStructsBeingCustomized(Structs);

	TArray<UPackage*> Packages;
	for ( TSharedPtr<FStructOnScope>& Struct : Structs)
	{	
		Packages.Add(Struct->GetPackage());
	}
	
	TArray<UObject*> Objects;
	PackageTools::GetObjectsInPackages(&Packages, Objects);

	UBlendSpaceBase* BlendSpaceBase = nullptr;
	// Find blendspace in found objects
	for ( UObject* Object : Objects )
	{
		BlendSpaceBase = Cast<UBlendSpaceBase>(Object);
		if (BlendSpaceBase)
		{
			break;
		}
	}

	FBlendSampleDetails::GenerateBlendSampleWidget([&CategoryBuilder]() -> FDetailWidgetRow& { return CategoryBuilder.AddCustomRow(FText::FromString(TEXT("SampleValue"))); }, GridWidget->OnSampleMoved, (BlendSpaceBase != nullptr) ? BlendSpaceBase : BlendSpace, GridWidget->GetSelectedSampleIndex(), false );

	TSharedPtr<IPropertyHandle> AnimationProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(FBlendSample, Animation), (UClass*)(FBlendSample::StaticStruct()));
	FDetailWidgetRow& AnimationRow = CategoryBuilder.AddCustomRow(FText::FromString(TEXT("Animation")));
	FBlendSampleDetails::GenerateAnimationWidget(AnimationRow, BlendSpaceBase != nullptr ? BlendSpaceBase : BlendSpace, AnimationProperty);

	TSharedPtr<IPropertyHandle> RateScaleProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(FBlendSample, RateScale), (UClass*)(FBlendSample::StaticStruct()));
	CategoryBuilder.AddProperty(RateScaleProperty);
}

void FBlendSampleDetails::GenerateBlendSampleWidget(TFunction<FDetailWidgetRow& (void)> InFunctor, FOnSampleMoved OnSampleMoved, const UBlendSpaceBase* BlendSpace, const int32 SampleIndex, bool bShowLabel)
{
	const int32 NumParameters = BlendSpace->IsA<UBlendSpace1D>() ? 1 : 2;
	for (int32 ParameterIndex = 0; ParameterIndex < NumParameters; ++ParameterIndex)
	{
		const FBlendParameter& BlendParameter = BlendSpace->GetBlendParameter(ParameterIndex);
		auto ValueChangedLambda = [BlendSpace, SampleIndex, ParameterIndex, BlendParameter, OnSampleMoved](const float NewValue)
		{
			const FBlendSample& Sample = BlendSpace->GetBlendSample(SampleIndex);
			FVector SampleValue = Sample.SampleValue;

			const float DeltaStep = (BlendParameter.Max - BlendParameter.Min) / BlendParameter.GridNum;
			// Calculate snapped value
			const float MinOffset = NewValue - BlendParameter.Min;
			float GridSteps = MinOffset / DeltaStep;
			int32 FlooredSteps = FMath::FloorToInt(GridSteps);
			GridSteps -= FlooredSteps;
			FlooredSteps = (GridSteps > .5f) ? FlooredSteps + 1 : FlooredSteps;

			// Temporary snap this value to closest point on grid (since the spin box delta does not provide the desired functionality)
			SampleValue[ParameterIndex] = BlendParameter.Min + (FlooredSteps * DeltaStep);
			OnSampleMoved.ExecuteIfBound(SampleIndex, SampleValue);
		};
		
		FDetailWidgetRow& ParameterRow = InFunctor();

		ParameterRow.NameContent()
		[
			SNew(STextBlock)
			.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			.Text(FText::FromString(BlendParameter.DisplayName))
		];

		ParameterRow.ValueContent()
		[
			SNew(SNumericEntryBox<float>)
			.Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
			.Value_Lambda(
			[BlendSpace, SampleIndex, ParameterIndex]() -> float
			{
				if (BlendSpace)
				{
					return BlendSpace->IsValidBlendSampleIndex(SampleIndex) ? BlendSpace->GetBlendSample(SampleIndex).SampleValue[ParameterIndex] : 0.0f;
				}

				return 0.0f;
			})
			.UndeterminedString(LOCTEXT("MultipleValues", "Multiple Values"))
			.OnValueCommitted_Lambda([ValueChangedLambda](const float NewValue, ETextCommit::Type CommitType)
			{
				ValueChangedLambda(NewValue);
			})
			.OnValueChanged_Lambda([ValueChangedLambda](const float NewValue)
			{
				ValueChangedLambda(NewValue);
			})
			.LabelVAlign(VAlign_Center)
			.AllowSpin(true)
			.MinValue_Lambda([BlendParameter]() -> float { return BlendParameter.Min; })
			.MaxValue_Lambda([BlendParameter]() -> float { return BlendParameter.Max; })
			.MinSliderValue_Lambda([BlendParameter]() -> float { return BlendParameter.Min; })
			.MaxSliderValue_Lambda([BlendParameter]() -> float { return BlendParameter.Max; })
			.MinDesiredValueWidth(60.0f)
			.Label()
			[
				SNew(STextBlock)
				.Visibility(bShowLabel ? EVisibility::Visible : EVisibility::Collapsed)
				.Text_Lambda([=]() { return FText::FromString(BlendParameter.DisplayName); })
			]
		];
	}
}

void FBlendSampleDetails::GenerateAnimationWidget(FDetailWidgetRow& Row, const UBlendSpaceBase* BlendSpace, TSharedPtr<IPropertyHandle> AnimationProperty)
{
	Row.NameContent()
		[
			SNew(STextBlock)
			.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			.Text(AnimationProperty->GetPropertyDisplayName())
		];

	Row.ValueContent()
		.MinDesiredWidth(250.f)
		[
			SNew(SObjectPropertyEntryBox)
			.AllowedClass(UAnimSequence::StaticClass())
			.OnShouldFilterAsset(FOnShouldFilterAsset::CreateStatic(&FBlendSampleDetails::ShouldFilterAssetStatic, BlendSpace))
			.PropertyHandle(AnimationProperty)
		];
}

bool FBlendSampleDetails::ShouldFilterAssetStatic(const FAssetData& AssetData, const UBlendSpaceBase* BlendSpaceBase)
{
	/** Cached flags to check whether or not an additive animation type is compatible with the blend space*/
	TMap<FString, bool> bValidAdditiveTypes;
	// Retrieve the additive animation type enum
	const UEnum* AdditiveTypeEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EAdditiveAnimationType"), true);
	// For each type check whether or not the blend space is compatible with it and cache the result
	for (int32 TypeValue = 0; TypeValue < (int32)EAdditiveAnimationType::AAT_MAX; ++TypeValue)
	{
		EAdditiveAnimationType Type = (EAdditiveAnimationType)TypeValue;
		// In case of non additive type make sure the blendspace is made up out of non additive samples only
		const bool bAdditiveFlag = (Type == EAdditiveAnimationType::AAT_None) ? !BlendSpaceBase->IsValidAdditive() : BlendSpaceBase->IsValidAdditive() && BlendSpaceBase->IsValidAdditiveType(Type);
		bValidAdditiveTypes.Add(AdditiveTypeEnum->GetNameByValue(TypeValue).ToString(), bAdditiveFlag);
	}

	bool bShouldFilter = true;

	// Skeleton is a private member so cannot use GET_MEMBER_NAME_CHECKED and friend class seemed unjustified to add
	const FName SkeletonTagName = "Skeleton";
	FString SkeletonName;
	if (AssetData.GetTagValue(SkeletonTagName, SkeletonName))
	{
		FStringAssetReference StringAssetReference(SkeletonName);
		// Check whether or not the skeletons match
		if (StringAssetReference.ToString() == BlendSpaceBase->GetSkeleton()->GetPathName())
		{
			// If so check if the additive animation tpye is compatible with the blend space
			const FName AdditiveTypeTagName = GET_MEMBER_NAME_CHECKED(UAnimSequence, AdditiveAnimType);
			FString AnimationTypeName;
			if (AssetData.GetTagValue(AdditiveTypeTagName, AnimationTypeName))
			{
				bShouldFilter = !bValidAdditiveTypes.FindChecked(AnimationTypeName);
			}
			else
			{
				// If the asset does not contain the required tag value retrieve the asset and validate it
				const UAnimSequence* AnimSequence = Cast<UAnimSequence>(AssetData.GetAsset());
				if (AnimSequence)
				{
					bShouldFilter = !(AnimSequence && BlendSpaceBase->ValidateAnimationSequence(AnimSequence));
				}
			}
		}
	}

	return bShouldFilter;
}

bool FBlendSampleDetails::ShouldFilterAsset(const FAssetData& AssetData) const
{
	bool bShouldFilter = true;

	// Skeleton is a private member so cannot use GET_MEMBER_NAME_CHECKED and friend class seemed unjustified to add
	const FName SkeletonTagName = "Skeleton";
	FString SkeletonName;
	if (AssetData.GetTagValue(SkeletonTagName, SkeletonName))
	{
		FStringAssetReference StringAssetReference(SkeletonName);
		// Check whether or not the skeletons match
		if (StringAssetReference.ToString() == BlendSpace->GetSkeleton()->GetPathName())
		{
			// If so check if the additive animation tpye is compatible with the blend space
			const FName AdditiveTypeTagName = GET_MEMBER_NAME_CHECKED(UAnimSequence, AdditiveAnimType);
			FString AnimationTypeName;
			if (AssetData.GetTagValue(AdditiveTypeTagName, AnimationTypeName))
			{
				bShouldFilter = !bValidAdditiveTypes.FindChecked(AnimationTypeName);
			}
			else
			{
				// If the asset does not contain the required tag value retrieve the asset and validate it
				const UAnimSequence* AnimSequence = Cast<UAnimSequence>(AssetData.GetAsset());
				if (AnimSequence)
				{
					bShouldFilter = !(AnimSequence && BlendSpace->ValidateAnimationSequence(AnimSequence));
				}
			}
		}
	}
	
	return bShouldFilter;
}

#undef LOCTEXT_NAMESPACE
