// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnvironmentQueryEditorPrivatePCH.h"
#include "EnvQueryTestDetails.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/EnvQueryTest.h"

#define LOCTEXT_NAMESPACE "EnvQueryTestDetails"

TSharedRef<IDetailCustomization> FEnvQueryTestDetails::MakeInstance()
{
	return MakeShareable( new FEnvQueryTestDetails );
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FEnvQueryTestDetails::CustomizeDetails( IDetailLayoutBuilder& DetailLayout )
{
	AllowWriting = false;

	TArray<TWeakObjectPtr<UObject> > EditedObjects;
	DetailLayout.GetObjectsBeingCustomized(EditedObjects);
	for (int32 i = 0; i < EditedObjects.Num(); i++)
	{
		const UEnvQueryTest* EditedTest = Cast<const UEnvQueryTest>(EditedObjects[i].Get());
		if (EditedTest)
		{
			MyTest = EditedTest;
			break;
		}
	}

	// Initialize all handles
	ConditionHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest, Condition));
	FilterTypeHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest, FilterType));
	ScoreEquationHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest, ScoringEquation));
	TestPurposeHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest, TestPurpose));
	ScoreHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest, Weight));

	ClampMinTypeHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest, ClampMinType));
	ClampMaxTypeHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest, ClampMaxType));

	ScoreClampingMinHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest, ScoreClampingMin));
	FloatFilterMinHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest, FloatFilterMin));

	ScoreClampingMaxHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest, ScoreClampingMax));
	FloatFilterMaxHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest, FloatFilterMax));

	// Build combo box data values
	BuildConditionValues();
	BuildFilterTestValues();
	BuildScoreEquationValues();
	BuildScoreClampingTypeValues(true, ClampMinTypeValues);
	BuildScoreClampingTypeValues(false, ClampMaxTypeValues);

// 	// dynamic Condition combo
// 	IDetailCategoryBuilder& DeprecatedFilterCategory = DetailLayout.EditCategory("Deprecated Filter and Score");
// 	IDetailPropertyRow& ConditionRow = DeprecatedFilterCategory.AddProperty(ConditionHandle);
// 	ConditionRow.CustomWidget()
// 		.NameContent()
// 		[
// 			ConditionHandle->CreatePropertyNameWidget()
// 		]
// 		.ValueContent()
// 		[
// 			SNew(SComboButton)
// 			.OnGetMenuContent(this, &FEnvQueryTestDetails::OnGetConditionContent)
// 			.ContentPadding(FMargin( 2.0f, 2.0f ))
// 			.ButtonContent()
// 			[
// 				SNew(STextBlock) 
// 				.Text(this, &FEnvQueryTestDetails::GetCurrentConditionDesc)
// 				.Font(IDetailLayoutBuilder::GetDetailFont())
// 			]
// 		];
// 	ConditionRow.EditCondition(TAttribute<bool>(this, &FEnvQueryTestDetails::AllowWritingToFiltersFromScore), NULL);
// 
// 	// TODO: Remove!
// 	IDetailPropertyRow& DiscardFailedRow = DeprecatedFilterCategory.AddProperty(DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest, bDiscardFailedItems)));
// 	DiscardFailedRow.EditCondition(TAttribute<bool>(this, &FEnvQueryTestDetails::AllowWritingToFiltersFromScore), NULL);
// 
// 	// DEPRECATED!  Remove as soon as testing is complete!
// 	IDetailPropertyRow& FloatFilterRow = DeprecatedFilterCategory.AddProperty(DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest, FloatFilter)));
// 	FloatFilterRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FEnvQueryTestDetails::GetFloatFilterVisibility)));
// 	FloatFilterRow.EditCondition(TAttribute<bool>(this, &FEnvQueryTestDetails::AllowWritingToFiltersFromScore), NULL);
// 
// 	IDetailPropertyRow& WeightModiferRow = DeprecatedFilterCategory.AddProperty(DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest, WeightModifier)));
// 	WeightModiferRow.EditCondition(TAttribute<bool>(this, &FEnvQueryTestDetails::AllowWritingToFiltersFromScore), NULL);
	
	IDetailCategoryBuilder& TestCategory = DetailLayout.EditCategory("Test");
	IDetailPropertyRow& TestPurposeRow = TestCategory.AddProperty(TestPurposeHandle);

	IDetailCategoryBuilder& FilterCategory = DetailLayout.EditCategory("Filter");

	IDetailPropertyRow& FilterTypeRow = FilterCategory.AddProperty(FilterTypeHandle);
	FilterTypeRow.CustomWidget()
		.NameContent()
		[
			FilterTypeHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(SComboButton)
			.OnGetMenuContent(this, &FEnvQueryTestDetails::OnGetFilterTestContent)
			.ContentPadding(FMargin( 2.0f, 2.0f ))
			.ButtonContent()
			[
				SNew(STextBlock) 
				.Text(this, &FEnvQueryTestDetails::GetCurrentFilterTestDesc)
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
		];
  	FilterTypeRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FEnvQueryTestDetails::GetFloatFilterVisibility)));

	// filters
	IDetailPropertyRow& FloatFilterMinRow = FilterCategory.AddProperty(DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest, FloatFilterMin)));
	FloatFilterMinRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FEnvQueryTestDetails::GetVisibilityOfFloatFilterMin)));

	IDetailPropertyRow& FloatFilterMaxRow = FilterCategory.AddProperty(DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest, FloatFilterMax)));
	FloatFilterMaxRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FEnvQueryTestDetails::GetVisibilityOfFloatFilterMax)));

	IDetailPropertyRow& BoolFilterRow = FilterCategory.AddProperty(DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest, BoolFilter)));
	BoolFilterRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FEnvQueryTestDetails::GetBoolFilterVisibility)));

	IDetailGroup& HackToEnsureFilterCategoryIsVisible = FilterCategory.AddGroup("HackForVisibility", FText::GetEmpty());

	// Scoring
	IDetailCategoryBuilder& ScoreCategory = DetailLayout.EditCategory("Score");

	//----------------------------
	// BEGIN Scoring: Clamping
	IDetailGroup& ClampingGroup = ScoreCategory.AddGroup("Clamping", LOCTEXT("ClampingLabel", "Clamping"));
	
	// Drop-downs for setting type of lower and upper bound normalization
	IDetailPropertyRow& ClampMinTypeRow = ClampingGroup.AddPropertyRow(ClampMinTypeHandle.ToSharedRef());
	ClampMinTypeRow.CustomWidget()
		.NameContent()
		[
			ClampMinTypeHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(SComboButton)
			.OnGetMenuContent(this, &FEnvQueryTestDetails::OnGetClampMinTypeContent)
			.ContentPadding(FMargin( 2.0f, 2.0f ))
			.ButtonContent()
			[
				SNew(STextBlock) 
				.Text(this, &FEnvQueryTestDetails::GetClampMinTypeDesc)
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
		];
	ClampMinTypeRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FEnvQueryTestDetails::GetFloatScoreVisibility)));

	// Lower Bound for normalization of score if specified independently of filtering.
	IDetailPropertyRow& ScoreClampingMinRow = ClampingGroup.AddPropertyRow(ScoreClampingMinHandle.ToSharedRef());
	ScoreClampingMinRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FEnvQueryTestDetails::GetVisibilityOfScoreClampingMinimum)));

	// Lower Bound for scoring when tied to filter minimum.
	if (FloatFilterMinHandle->IsValidHandle())
	{
		IDetailPropertyRow& FloatFilterMinForClampingRow = ClampingGroup.AddPropertyRow(FloatFilterMinHandle.ToSharedRef());
		FloatFilterMinForClampingRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FEnvQueryTestDetails::GetVisibilityOfFilterMinForScoreClamping)));
		FloatFilterMinForClampingRow.ToolTip(LOCTEXT("FloatFilterMinForClampingRowToolTip", "See Filter Thresholds under the Filter tab.  Values lower than this (before clamping) cause the item to be thrown out as invalid.  Values are normalized with this value as the minimum, so items with this value will have a normalized score of 0."));
		FloatFilterMinForClampingRow.EditCondition(TAttribute<bool>(this, &FEnvQueryTestDetails::AllowWritingToFiltersFromScore), NULL);
	}

	if (ClampMaxTypeHandle->IsValidHandle())
	{
		IDetailPropertyRow& ClampMaxTypeRow = ClampingGroup.AddPropertyRow(ClampMaxTypeHandle.ToSharedRef());
		ClampMaxTypeRow.CustomWidget()
			.NameContent()
			[
				ClampMaxTypeHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			[
				SNew(SComboButton)
				.OnGetMenuContent(this, &FEnvQueryTestDetails::OnGetClampMaxTypeContent)
				.ContentPadding(FMargin( 2.0f, 2.0f ))
				.ButtonContent()
				[
					SNew(STextBlock) 
					.Text(this, &FEnvQueryTestDetails::GetClampMaxTypeDesc)
					.Font(IDetailLayoutBuilder::GetDetailFont())
				]
			];
		ClampMaxTypeRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FEnvQueryTestDetails::GetFloatScoreVisibility)));
	}

	// Upper Bound for normalization of score if specified independently of filtering.
	if (ScoreClampingMaxHandle->IsValidHandle())
	{
		IDetailPropertyRow& ScoreClampingMaxRow = ClampingGroup.AddPropertyRow(ScoreClampingMaxHandle.ToSharedRef());
		ScoreClampingMaxRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FEnvQueryTestDetails::GetVisibilityOfScoreClampingMaximum)));
	}
	
	if (FloatFilterMaxHandle->IsValidHandle())
	{
		// Upper Bound for scoring when tied to filter maximum.
		IDetailPropertyRow& FloatFilterMaxForClampingRow = ClampingGroup.AddPropertyRow(FloatFilterMaxHandle.ToSharedRef());
		FloatFilterMaxForClampingRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FEnvQueryTestDetails::GetVisibilityOfFilterMaxForScoreClamping)));
		FloatFilterMaxForClampingRow.ToolTip(LOCTEXT("FloatFilterMaxForClampingRowToolTip", "See Filter Thresholds under the Filter tab.  Values higher than this (before normalization) cause the item to be thrown out as invalid.  Values are normalized with this value as the maximum, so items with this value will have a normalized score of 1."));
		FloatFilterMaxForClampingRow.EditCondition(TAttribute<bool>(this, &FEnvQueryTestDetails::AllowWritingToFiltersFromScore), NULL);
	}
	// END Scoring: Clamping, continue Scoring
	//----------------------------

	IDetailPropertyRow& BoolScoreTestRow = ScoreCategory.AddProperty(DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest, BoolFilter)));
	BoolScoreTestRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FEnvQueryTestDetails::GetBoolFilterVisibilityForScoring)));
	BoolScoreTestRow.DisplayName(LOCTEXT("BoolMatchLabel", "Bool Match"));
	BoolScoreTestRow.ToolTip(LOCTEXT("BoolMatchToolTip", "Boolean value to match in order to grant score of 'Weight'.  Not matching this value will not change score."));

// 	IDetailPropertyRow& ScoreMirrorNormalizedScoreRow = ScoreCategory.AddProperty(DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest, bMirrorNormalizedScore)));
// 	ScoreMirrorNormalizedScoreRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FEnvQueryTestDetails::GetFloatScoreVisibility)));

	IDetailPropertyRow& ScoreEquationTypeRow = ScoreCategory.AddProperty(ScoreEquationHandle);
	ScoreEquationTypeRow.CustomWidget()
		.NameContent()
		.VAlign(VAlign_Top)
		[
			ScoreEquationHandle->CreatePropertyNameWidget()
		]
		.ValueContent().MaxDesiredWidth(600)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			[
				SNew(SComboButton)
				.OnGetMenuContent(this, &FEnvQueryTestDetails::OnGetEquationValuesContent)
				.ContentPadding(FMargin( 2.0f, 2.0f ))
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text(this, &FEnvQueryTestDetails::GetEquationValuesDesc)
					.Font(IDetailLayoutBuilder::GetDetailFont())
				]
			]
			+SVerticalBox::Slot()
			.Padding(0, 2, 0, 0)
			.AutoHeight()
			[
				SNew(STextBlock)
				.IsEnabled(false)
				.Text(this, &FEnvQueryTestDetails::GetScoreEquationInfo)
				.Font( IDetailLayoutBuilder::GetDetailFont() )
			]
		];
	ScoreEquationTypeRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FEnvQueryTestDetails::GetFloatScoreVisibility)));

	IDetailPropertyRow& ScoreWeightRow = ScoreCategory.AddProperty(DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest, Weight)));
	ScoreWeightRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FEnvQueryTestDetails::GetScoreVisibility)));

	// scoring & filter function preview
	IDetailCategoryBuilder& PreviewCategory = DetailLayout.EditCategory("Preview");
	PreviewCategory.AddCustomRow(LOCTEXT("Preview", "Preview")).WholeRowWidget
	[
		SAssignNew(PreviewWidget, STestFunctionWidget)
	];

	FSimpleDelegate OnGraphPreviewDataChangedDelegate = FSimpleDelegate::CreateSP(this, &FEnvQueryTestDetails::UpdateTestFunctionPreview);
	TestPurposeHandle->SetOnPropertyValueChanged(OnGraphPreviewDataChangedDelegate);
	FilterTypeHandle->SetOnPropertyValueChanged(OnGraphPreviewDataChangedDelegate);
	ClampMaxTypeHandle->SetOnPropertyValueChanged(OnGraphPreviewDataChangedDelegate);
	ClampMinTypeHandle->SetOnPropertyValueChanged(OnGraphPreviewDataChangedDelegate);
	ScoreEquationHandle->SetOnPropertyValueChanged(OnGraphPreviewDataChangedDelegate);
	ScoreHandle->SetOnPropertyValueChanged(OnGraphPreviewDataChangedDelegate);

	UpdateTestFunctionPreview();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void FEnvQueryTestDetails::BuildFilterTestValues()
{
	UEnum* TestConditionEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EEnvTestFilterType"));
	check(TestConditionEnum);

	FilterTestValues.Reset();

	const UEnvQueryTest* EditedTest = Cast<const UEnvQueryTest>(MyTest.Get());
	if (EditedTest)
	{
		if (EditedTest->GetWorkOnFloatValues())
		{
			FilterTestValues.Add(FStringIntPair(TestConditionEnum->GetEnumText(EEnvTestFilterType::Minimum).ToString(), EEnvTestFilterType::Minimum));
			FilterTestValues.Add(FStringIntPair(TestConditionEnum->GetEnumText(EEnvTestFilterType::Maximum).ToString(), EEnvTestFilterType::Maximum));
			FilterTestValues.Add(FStringIntPair(TestConditionEnum->GetEnumText(EEnvTestFilterType::Range).ToString(), EEnvTestFilterType::Range));
		}
		else
		{
			FilterTestValues.Add(FStringIntPair(TestConditionEnum->GetEnumText(EEnvTestFilterType::Match).ToString(), EEnvTestFilterType::Match));
		}
	}
}

void FEnvQueryTestDetails::BuildScoreEquationValues()
{
	UEnum* TestScoreEquationEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EEnvTestScoreEquation"));
	check(TestScoreEquationEnum);

	ScoreEquationValues.Reset();

	// Const scoring is always valid.  But other equations are only valid if the score values can be other than boolean values.
	ScoreEquationValues.Add(FStringIntPair(TestScoreEquationEnum->GetEnumText(EEnvTestScoreEquation::Constant).ToString(), EEnvTestScoreEquation::Constant));

	const UEnvQueryTest* EditedTest = Cast<const UEnvQueryTest>(MyTest.Get());
	if (EditedTest)
	{
		if (EditedTest->GetWorkOnFloatValues())
		{
			ScoreEquationValues.Add(FStringIntPair(TestScoreEquationEnum->GetEnumText(EEnvTestScoreEquation::Linear).ToString(), EEnvTestScoreEquation::Linear));
			ScoreEquationValues.Add(FStringIntPair(TestScoreEquationEnum->GetEnumText(EEnvTestScoreEquation::Square).ToString(), EEnvTestScoreEquation::Square));
			ScoreEquationValues.Add(FStringIntPair(TestScoreEquationEnum->GetEnumText(EEnvTestScoreEquation::InverseLinear).ToString(), EEnvTestScoreEquation::InverseLinear));
		}
	}
}

void FEnvQueryTestDetails::BuildConditionValues()
{
	UEnum* TestConditionEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EEnvTestCondition"));
	check(TestConditionEnum);

	ConditionValues.Reset();
	ConditionValues.Add(FStringIntPair(TestConditionEnum->GetEnumText(EEnvTestCondition::NoCondition).ToString(), EEnvTestCondition::NoCondition));

	const UEnvQueryTest* EditedTest = Cast<const UEnvQueryTest>(MyTest.Get());
	if (EditedTest)
	{
		if (EditedTest->GetWorkOnFloatValues())
		{
			ConditionValues.Add(FStringIntPair(TestConditionEnum->GetEnumText(EEnvTestCondition::AtLeast).ToString(), EEnvTestCondition::AtLeast));
			ConditionValues.Add(FStringIntPair(TestConditionEnum->GetEnumText(EEnvTestCondition::UpTo).ToString(), EEnvTestCondition::UpTo));
		}
		else
		{
			ConditionValues.Add(FStringIntPair(TestConditionEnum->GetEnumText(EEnvTestCondition::Match).ToString(), EEnvTestCondition::Match));
		}
	}
}

void FEnvQueryTestDetails::OnFilterTestChange(int32 Index)
{
	uint8 EnumValue = Index;
	FilterTypeHandle->SetValue(EnumValue);
}

void FEnvQueryTestDetails::OnConditionComboChange(int32 Index)
{
	uint8 EnumValue = Index;
	ConditionHandle->SetValue(EnumValue);
}

void FEnvQueryTestDetails::OnScoreEquationChange(int32 Index)
{
	uint8 EnumValue = Index;
	ScoreEquationHandle->SetValue(EnumValue);
}

void FEnvQueryTestDetails::BuildScoreClampingTypeValues(bool bBuildMinValues, TArray<FStringIntPair>& ClampTypeValues) const
{
	UEnum* ScoringNormalizationEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EEnvQueryTestClamping"));
	check(ScoringNormalizationEnum);

	ClampTypeValues.Reset();
	ClampTypeValues.Add(FStringIntPair(ScoringNormalizationEnum->GetEnumText(EEnvQueryTestClamping::None).ToString(), EEnvQueryTestClamping::None));
	ClampTypeValues.Add(FStringIntPair(ScoringNormalizationEnum->GetEnumText(EEnvQueryTestClamping::SpecifiedValue).ToString(), EEnvQueryTestClamping::SpecifiedValue));

	if (IsFiltering())
	{
		bool bSupportFilterThreshold = false;
		if (bBuildMinValues)
		{
			if (UsesFilterMin())
			{
				bSupportFilterThreshold = true;
			}
		}
		else if (UsesFilterMax())
		{
			bSupportFilterThreshold = true;
		}

		if (bSupportFilterThreshold)
		{
			ClampTypeValues.Add(FStringIntPair(ScoringNormalizationEnum->GetEnumText(EEnvQueryTestClamping::FilterThreshold).ToString(), EEnvQueryTestClamping::FilterThreshold));
		}
	}
}

void FEnvQueryTestDetails::OnClampMinTestChange(int32 Index)
{
	check(ClampMinTypeHandle.IsValid());
	uint8 EnumValue = Index;
	ClampMinTypeHandle->SetValue(EnumValue);
}

void FEnvQueryTestDetails::OnClampMaxTestChange(int32 Index)
{
	check(ClampMaxTypeHandle.IsValid());
	uint8 EnumValue = Index;
	ClampMaxTypeHandle->SetValue(EnumValue);
}

TSharedRef<SWidget> FEnvQueryTestDetails::OnGetClampMinTypeContent()
{
 	BuildScoreClampingTypeValues(true, ClampMinTypeValues);
	FMenuBuilder MenuBuilder(true, NULL);

 	for (int32 i = 0; i < ClampMinTypeValues.Num(); i++)
 	{
 		FUIAction ItemAction(FExecuteAction::CreateSP(this, &FEnvQueryTestDetails::OnClampMinTestChange, ClampMinTypeValues[i].Int));
 		MenuBuilder.AddMenuEntry(FText::FromString(ClampMinTypeValues[i].Str), TAttribute<FText>(), FSlateIcon(), ItemAction);
 	}

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> FEnvQueryTestDetails::OnGetClampMaxTypeContent()
{
 	BuildScoreClampingTypeValues(false, ClampMaxTypeValues);
	FMenuBuilder MenuBuilder(true, NULL);

 	for (int32 i = 0; i < ClampMaxTypeValues.Num(); i++)
 	{
 		FUIAction ItemAction(FExecuteAction::CreateSP(this, &FEnvQueryTestDetails::OnClampMaxTestChange, ClampMaxTypeValues[i].Int));
 		MenuBuilder.AddMenuEntry(FText::FromString(ClampMaxTypeValues[i].Str), TAttribute<FText>(), FSlateIcon(), ItemAction);
 	}

	return MenuBuilder.MakeWidget();
}

FString FEnvQueryTestDetails::GetClampMinTypeDesc() const
{
	check(ClampMinTypeHandle.IsValid());
	uint8 EnumValue;
	ClampMinTypeHandle->GetValue(EnumValue);

	for (int32 i = 0; i < ClampMinTypeValues.Num(); i++)
	{
		if (ClampMinTypeValues[i].Int == EnumValue)
		{
			return ClampMinTypeValues[i].Str;
		}
	}

	return FString();
}

FString FEnvQueryTestDetails::GetClampMaxTypeDesc() const
{
	check(ClampMaxTypeHandle.IsValid());
	uint8 EnumValue;
	ClampMaxTypeHandle->GetValue(EnumValue);

	for (int32 i = 0; i < ClampMaxTypeValues.Num(); i++)
	{
		if (ClampMaxTypeValues[i].Int == EnumValue)
		{
			return ClampMaxTypeValues[i].Str;
		}
	}

	return FString();
}

FString FEnvQueryTestDetails::GetEquationValuesDesc() const
{
	check(ScoreEquationHandle.IsValid());
	uint8 EnumValue;
	ScoreEquationHandle->GetValue(EnumValue);

	for (int32 i = 0; i < ScoreEquationValues.Num(); i++)
	{
		if (ScoreEquationValues[i].Int == EnumValue)
		{
			return ScoreEquationValues[i].Str;
		}
	}

	return FString();
}

TSharedRef<SWidget> FEnvQueryTestDetails::OnGetFilterTestContent()
{
	BuildFilterTestValues();
	FMenuBuilder MenuBuilder(true, NULL);

	for (int32 i = 0; i < FilterTestValues.Num(); i++)
	{
		FUIAction ItemAction(FExecuteAction::CreateSP(this, &FEnvQueryTestDetails::OnFilterTestChange, FilterTestValues[i].Int));
		MenuBuilder.AddMenuEntry(FText::FromString(FilterTestValues[i].Str), TAttribute<FText>(), FSlateIcon(), ItemAction);
	}

	return MenuBuilder.MakeWidget();
}

FString FEnvQueryTestDetails::GetCurrentFilterTestDesc() const
{
	uint8 EnumValue;
	FilterTypeHandle->GetValue(EnumValue);

	for (int32 i = 0; i < FilterTestValues.Num(); i++)
	{
		if (FilterTestValues[i].Int == EnumValue)
		{
			return FilterTestValues[i].Str;
		}
	}

	return FString();
}

TSharedRef<SWidget> FEnvQueryTestDetails::OnGetConditionContent()
{
	BuildConditionValues();
	FMenuBuilder MenuBuilder(true, NULL);

	for (int32 i = 0; i < ConditionValues.Num(); i++)
	{
		FUIAction ItemAction( FExecuteAction::CreateSP( this, &FEnvQueryTestDetails::OnConditionComboChange, ConditionValues[i].Int ) );
		MenuBuilder.AddMenuEntry( FText::FromString( ConditionValues[i].Str ), TAttribute<FText>(), FSlateIcon(), ItemAction);
	}

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> FEnvQueryTestDetails::OnGetEquationValuesContent()
{
	BuildScoreEquationValues();
	FMenuBuilder MenuBuilder(true, NULL);

	for (int32 i = 0; i < ScoreEquationValues.Num(); i++)
	{
		FUIAction ItemAction(FExecuteAction::CreateSP(this, &FEnvQueryTestDetails::OnScoreEquationChange, ScoreEquationValues[i].Int));
		MenuBuilder.AddMenuEntry(FText::FromString(ScoreEquationValues[i].Str), TAttribute<FText>(), FSlateIcon(), ItemAction);
	}

	return MenuBuilder.MakeWidget();
}

FString FEnvQueryTestDetails::GetCurrentConditionDesc() const
{
	uint8 EnumValue;
	ConditionHandle->GetValue(EnumValue);

	for (int32 i = 0; i < ConditionValues.Num(); i++)
	{
		if (ConditionValues[i].Int == EnumValue)
		{
			return ConditionValues[i].Str;
		}
	}

	return FString();
}

FString FEnvQueryTestDetails::GetScoreEquationInfo() const
{
	uint8 EnumValue;
	ScoreEquationHandle->GetValue(EnumValue);

	switch (EnumValue)
	{
		case EEnvTestScoreEquation::Linear:
			return LOCTEXT("Linear","Final score = Weight * NormalizedItemValue").ToString();

		case EEnvTestScoreEquation::Square:
			return LOCTEXT("Square","Final score = Weight * (NormalizedItemValue * NormalizedItemValue)\nBias towards items with big values.").ToString();

		case EEnvTestScoreEquation::InverseLinear:
			return LOCTEXT("Inverse","Final score = Weight * (1.0 - NormalizedItemValue)\nBias towards items with values close to zero.  (Linear, but flipped from 1 to 0 rather than 0 to 1.").ToString();

		case EEnvTestScoreEquation::Constant:
			return LOCTEXT("Constant", "Final score (for values that 'pass') = Weight\nNOTE: In this case, the score is normally EITHER the Weight value or zero.\nThe score will be zero if the Normalized Test Value is zero (or if the test value is false for a boolean query).\nOtherwise, score will be the Weight.").ToString();

		default:
			break;
	}

	return TEXT("");
}

EVisibility FEnvQueryTestDetails::GetVisibilityOfFilterMinForScoreClamping() const
{
	if (GetFloatScoreVisibility() == EVisibility::Visible)
	{
		const UEnvQueryTest* MyTestOb = Cast<const UEnvQueryTest>(MyTest.Get());
		if ((MyTestOb != NULL) && (MyTestOb->ClampMinType == EEnvQueryTestClamping::FilterThreshold))
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}

EVisibility FEnvQueryTestDetails::GetVisibilityOfFilterMaxForScoreClamping() const
{
	if (GetFloatScoreVisibility() == EVisibility::Visible)
	{
		const UEnvQueryTest* MyTestOb = Cast<const UEnvQueryTest>(MyTest.Get());
		if ((MyTestOb != NULL) && (MyTestOb->ClampMaxType == EEnvQueryTestClamping::FilterThreshold))
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}

EVisibility FEnvQueryTestDetails::GetVisibilityOfScoreClampingMinimum() const
{
	if (GetFloatScoreVisibility() == EVisibility::Visible)
	{
		const UEnvQueryTest* MyTestOb = Cast<const UEnvQueryTest>(MyTest.Get());
		if ((MyTestOb != NULL) && (MyTestOb->ClampMinType == EEnvQueryTestClamping::SpecifiedValue))
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}

EVisibility FEnvQueryTestDetails::GetVisibilityOfScoreClampingMaximum() const
{
	if (GetFloatScoreVisibility() == EVisibility::Visible)
	{
		const UEnvQueryTest* MyTestOb = Cast<const UEnvQueryTest>(MyTest.Get());
		if ((MyTestOb != NULL) && (MyTestOb->ClampMaxType == EEnvQueryTestClamping::SpecifiedValue))
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}

EVisibility FEnvQueryTestDetails::GetFloatTestVisibility() const
{
	const UEnvQueryTest* MyTestOb = Cast<const UEnvQueryTest>(MyTest.Get());
	if ((MyTestOb != NULL) && MyTestOb->GetWorkOnFloatValues())
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

bool FEnvQueryTestDetails::IsFiltering() const
{
	const UEnvQueryTest* MyTestOb = Cast<const UEnvQueryTest>(MyTest.Get());
	if (MyTestOb == NULL)
	{
		return false;
	}

	return MyTestOb->IsFiltering();
}

bool FEnvQueryTestDetails::IsScoring() const
{
	const UEnvQueryTest* MyTestOb = Cast<const UEnvQueryTest>(MyTest.Get());
	if (MyTestOb == NULL)
	{
		return false;
	}

	return MyTestOb->IsScoring();
}

EVisibility FEnvQueryTestDetails::GetFloatFilterVisibility() const
{
	if (IsFiltering())
	{
		const UEnvQueryTest* MyTestOb = Cast<const UEnvQueryTest>(MyTest.Get());
		if (MyTestOb && MyTestOb->GetWorkOnFloatValues())
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}

EVisibility FEnvQueryTestDetails::GetScoreVisibility() const
{
	if (IsScoring())
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

EVisibility FEnvQueryTestDetails::GetFloatScoreVisibility() const
{
	if (IsScoring())
	{
		const UEnvQueryTest* MyTestOb = Cast<const UEnvQueryTest>(MyTest.Get());
		if (MyTestOb && MyTestOb->GetWorkOnFloatValues())
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}

bool FEnvQueryTestDetails::UsesFilterMin() const
{
	uint8 EnumValue;
	check(FilterTypeHandle.IsValid());
	FilterTypeHandle->GetValue(EnumValue);

	return ((EnumValue == EEnvTestFilterType::Minimum) || (EnumValue == EEnvTestFilterType::Range));
}

bool FEnvQueryTestDetails::UsesFilterMax() const
{
	uint8 EnumValue;
	check(FilterTypeHandle.IsValid());
	FilterTypeHandle->GetValue(EnumValue);

	return ((EnumValue == EEnvTestFilterType::Maximum) || (EnumValue == EEnvTestFilterType::Range));
}

EVisibility FEnvQueryTestDetails::GetVisibilityOfFloatFilterMin() const
{
	if (IsFiltering())
	{
		uint8 EnumValue;
		FilterTypeHandle->GetValue(EnumValue);

		const UEnvQueryTest* MyTestOb = Cast<const UEnvQueryTest>(MyTest.Get());
		if (MyTestOb && MyTestOb->GetWorkOnFloatValues() &&
			((EnumValue == EEnvTestFilterType::Minimum) || (EnumValue == EEnvTestFilterType::Range)))
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}

EVisibility FEnvQueryTestDetails::GetVisibilityOfFloatFilterMax() const
{
	if (IsFiltering())
	{
		uint8 EnumValue;
		FilterTypeHandle->GetValue(EnumValue);

		const UEnvQueryTest* MyTestOb = Cast<const UEnvQueryTest>(MyTest.Get());
		if (MyTestOb && MyTestOb->GetWorkOnFloatValues() &&
			((EnumValue == EEnvTestFilterType::Maximum) || (EnumValue == EEnvTestFilterType::Range)))
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}

bool FEnvQueryTestDetails::IsMatchingBoolValue() const
{
	// TODO: Decide whether this complex function needs to be so complex!  At the moment, if it's not working on floats,
	// it must be working on bools, in which case it MUST be using a Match test.  So probably it could stop much earlier!
	const UEnvQueryTest* MyTestOb = Cast<const UEnvQueryTest>(MyTest.Get());
	if (MyTestOb && !MyTestOb->GetWorkOnFloatValues())
	{
		if (FilterTypeHandle.IsValid())
		{
			uint8 EnumValue;
			FilterTypeHandle->GetValue(EnumValue);
			if (EnumValue == EEnvTestFilterType::Match)
			{
				return true;
			}
		}
	}

	return false;
}

EVisibility FEnvQueryTestDetails::GetBoolFilterVisibilityForScoring() const
{
	if (!IsFiltering())
	{
		if (IsMatchingBoolValue())
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}

EVisibility FEnvQueryTestDetails::GetBoolFilterVisibility() const
{
	if (IsFiltering())
	{
		if (IsMatchingBoolValue())
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}

EVisibility FEnvQueryTestDetails::GetDiscardFailedVisibility() const
{
	uint8 EnumValue;
	ConditionHandle->GetValue(EnumValue);

	return (EnumValue == EEnvTestCondition::NoCondition) ? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility FEnvQueryTestDetails::GetTestPreviewVisibility() const
{
	const UEnvQueryTest* MyTestOb = Cast<const UEnvQueryTest>(MyTest.Get());
	if ((MyTestOb != NULL) && MyTestOb->GetWorkOnFloatValues())
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

void FEnvQueryTestDetails::UpdateTestFunctionPreview() const
{
	// read values of properties
	uint8 TestPurpose = 0;
	uint8 FilterType = 0;
	uint8 ClampMinType = 0;
	uint8 ClampMaxType = 0;
	uint8 ScoreEquation = 0;
	float Score = 0.0f;

	TestPurposeHandle->GetValue(TestPurpose);
	FilterTypeHandle->GetValue(FilterType);
	ClampMinTypeHandle->GetValue(ClampMinType);
	ClampMaxTypeHandle->GetValue(ClampMaxType);
	ScoreEquationHandle->GetValue(ScoreEquation);
	ScoreHandle->GetValue(Score);

	// fill score equation samples
	if (TestPurpose == EEnvTestPurpose::Filter)
	{
		// pure filtering won't apply any scoring, draw flat line
		ScoreEquation = EEnvTestScoreEquation::Constant;
	}

	FillEquationSamples(ScoreEquation, Score < 0.0f, PreviewWidget->ScoreValues);

	// fill everything else
	const bool bCanFilter = (TestPurpose != EEnvTestPurpose::Score);
	PreviewWidget->bShowClampMin = (ClampMinType != EEnvQueryTestClamping::None);
	PreviewWidget->bShowClampMax = (ClampMaxType != EEnvQueryTestClamping::None);
	PreviewWidget->bShowLowPassFilter = ((FilterType == EEnvTestFilterType::Minimum) || (FilterType == EEnvTestFilterType::Range)) && bCanFilter;
	PreviewWidget->bShowHiPassFilter = ((FilterType == EEnvTestFilterType::Maximum) || (FilterType == EEnvTestFilterType::Range)) && bCanFilter;
	PreviewWidget->FilterLowX = 0.2f;
	PreviewWidget->FilterHiX = 0.8f;
	PreviewWidget->ClampMinX = (ClampMinType == EEnvQueryTestClamping::FilterThreshold) ? PreviewWidget->FilterLowX : 0.3f;
	PreviewWidget->ClampMaxX = (ClampMaxType == EEnvQueryTestClamping::FilterThreshold) ? PreviewWidget->FilterHiX : 0.7f;
	
	// postprocess samples for clamping
	if (PreviewWidget->bShowClampMin)
	{
		const int32 FixedIdx = FMath::TruncToInt(PreviewWidget->ClampMinX * 10.0f);
		for (int32 Idx = 0; Idx < FixedIdx; Idx++)
		{
			PreviewWidget->ScoreValues[Idx] = PreviewWidget->ScoreValues[FixedIdx];
		}
	}
	
	if (PreviewWidget->bShowClampMax)
	{
		const int32 FixedIdx = FMath::TruncToInt(PreviewWidget->ClampMaxX * 10.0f) + 1;
		for (int32 Idx = FixedIdx + 1; Idx < PreviewWidget->ScoreValues.Num(); Idx++)
		{
			PreviewWidget->ScoreValues[Idx] = PreviewWidget->ScoreValues[FixedIdx];
		}
	}
}

void FEnvQueryTestDetails::FillEquationSamples(uint8 EquationType, bool bInversed, TArray<float>& Samples) const
{
	const int32 MaxSamples = 11;
	static float SamplesLinear[MaxSamples] = { 0.0f };
	static float SamplesSquare[MaxSamples] = { 0.0f };
	static float SamplesConstant[MaxSamples] = { 0.0f };
	static bool bSamplesInitialized = false;

	if (!bSamplesInitialized)
	{
		bSamplesInitialized = true;

		for (int32 Idx = 0; Idx < MaxSamples; Idx++)
		{
			const float XValue = 1.0f * Idx / (MaxSamples - 1);
			SamplesLinear[Idx] = XValue;
			SamplesSquare[Idx] = XValue * XValue;
			SamplesConstant[Idx] = 0.5f;				// just for looks on preview, not the actual value
		}
	}

	const float* AllSamples[] = { SamplesLinear, SamplesSquare, SamplesLinear, SamplesConstant };
	if (EquationType >= ARRAY_COUNT(AllSamples))
	{
		EquationType = EEnvTestScoreEquation::Constant;
	}

	const float* SamplesArray = AllSamples[EquationType];
	if (EquationType == EEnvTestScoreEquation::InverseLinear)
	{
		bInversed = !bInversed;
	}

	Samples.Reset();
	Samples.AddZeroed(MaxSamples);
	for (int32 Idx = 0; Idx < MaxSamples; Idx++)
	{
		Samples[Idx] = bInversed ? (1.0f - SamplesArray[Idx]) : SamplesArray[Idx];
	}
}

#undef LOCTEXT_NAMESPACE
