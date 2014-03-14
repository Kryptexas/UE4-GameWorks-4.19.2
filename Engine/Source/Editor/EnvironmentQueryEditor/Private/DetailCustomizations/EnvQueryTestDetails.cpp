// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "../EnvironmentQueryEditorPrivatePCH.h"
#include "EnvQueryTestDetails.h"

#define LOCTEXT_NAMESPACE "EnvQueryTestDetails"

TSharedRef<IDetailCustomization> FEnvQueryTestDetails::MakeInstance()
{
	return MakeShareable( new FEnvQueryTestDetails );
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FEnvQueryTestDetails::CustomizeDetails( IDetailLayoutBuilder& DetailLayout )
{
	TArray<TWeakObjectPtr<UObject> > EditedObjects;
	DetailLayout.GetObjectsBeingCustomized(EditedObjects);
	for (int32 i = 0; i < EditedObjects.Num(); i++)
	{
		UEnvQueryTest* EditedTest = Cast<UEnvQueryTest>(EditedObjects[i].Get());
		if (EditedTest)
		{
			MyTest = EditedTest;
			break;
		}
	}
	BuildConditionValues();

	ConditionHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest, Condition));
	ModifierHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest,WeightModifier));

	// dynamic Condition combo
	IDetailCategoryBuilder& FilterCategory = DetailLayout.EditCategory( "Filter" );
	IDetailPropertyRow& ConditionRow = FilterCategory.AddProperty(ConditionHandle);
	ConditionRow.CustomWidget()
		.NameContent()
		[
			ConditionHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(SComboButton)
			.OnGetMenuContent(this, &FEnvQueryTestDetails::OnGetConditionContent)
			.ContentPadding(FMargin( 2.0f, 2.0f ))
			.ButtonContent()
			[
				SNew(STextBlock) 
				.Text(this, &FEnvQueryTestDetails::GetCurrentConditionDesc)
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
		];

	// filters
	IDetailPropertyRow& FloatFilterRow = FilterCategory.AddProperty(DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest,FloatFilter)));
	FloatFilterRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FEnvQueryTestDetails::GetFloatFilterVisibility)));

	IDetailPropertyRow& BoolFilterRow = FilterCategory.AddProperty(DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest,BoolFilter)));
	BoolFilterRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FEnvQueryTestDetails::GetBoolFilterVisibility)));

	// helper for weight modifier
	IDetailCategoryBuilder& WeightCategory = DetailLayout.EditCategory( "Weight" );
	IDetailPropertyRow& WeightRow = WeightCategory.AddProperty(DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest,Weight)));

	IDetailPropertyRow& ModifierRow = WeightCategory.AddProperty(ModifierHandle);
	ModifierRow.CustomWidget()
		.NameContent()
		.VAlign(VAlign_Top)
		[
			ModifierHandle->CreatePropertyNameWidget()
		]
		.ValueContent().MaxDesiredWidth(600)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			[
				ModifierHandle->CreatePropertyValueWidget()
			]
			+SVerticalBox::Slot()
			.Padding(0, 2, 0, 0)
			.AutoHeight()
			[
				SNew(STextBlock)
				.IsEnabled(false)
				.Text(this, &FEnvQueryTestDetails::GetWeightModifierInfo)
				.Font( IDetailLayoutBuilder::GetDetailFont() )
			]
		];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void FEnvQueryTestDetails::BuildConditionValues()
{
	UEnum* TestConditionEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EEnvTestCondition"));
	check(TestConditionEnum);

	ConditionValues.Reset();
	ConditionValues.Add(FStringIntPair(TestConditionEnum->GetEnumString(EEnvTestCondition::NoCondition), EEnvTestCondition::NoCondition));

	UEnvQueryTest* EditedTest = (UEnvQueryTest*)(MyTest.Get());
	if (EditedTest)
	{
		if (EditedTest->bWorkOnFloatValues)
		{
			ConditionValues.Add(FStringIntPair(TestConditionEnum->GetEnumString(EEnvTestCondition::AtLeast), EEnvTestCondition::AtLeast));
			ConditionValues.Add(FStringIntPair(TestConditionEnum->GetEnumString(EEnvTestCondition::UpTo), EEnvTestCondition::UpTo));
		}
		else
		{
			ConditionValues.Add(FStringIntPair(TestConditionEnum->GetEnumString(EEnvTestCondition::Match), EEnvTestCondition::Match));
		}
	}
}

void FEnvQueryTestDetails::OnConditionComboChange(int32 Index)
{
	uint8 EnumValue = Index;
	ConditionHandle->SetValue(EnumValue);
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

FString FEnvQueryTestDetails::GetWeightModifierInfo() const
{
	uint8 EnumValue;
	ModifierHandle->GetValue(EnumValue);

	switch (EnumValue)
	{
	case EEnvTestWeight::None:
		return LOCTEXT("WeightNone","Final score = ItemValue * weight").ToString();

	case EEnvTestWeight::Square:
		return LOCTEXT("WeightSquare","Final score = ItemValue * ItemValue * weight\nBias towards items with big values.").ToString();

	case EEnvTestWeight::Inverse:
		return LOCTEXT("WeightInverse","Final score = (1 / ItemValue) * weight\nBias towards items with values close to zero.").ToString();

	case EEnvTestWeight::Absolute:
		return LOCTEXT("WeightAbsolute","Final score = | ItemValue | * weight\nDiscards sign of item's value.").ToString();

	case EEnvTestWeight::Flat:
		return LOCTEXT("WeightFlat","Final score = 1\nOptimized when quering for single result.").ToString();

	default: break;
	}

	return TEXT("");
}

EVisibility FEnvQueryTestDetails::GetFloatFilterVisibility() const
{
	uint8 EnumValue;
	ConditionHandle->GetValue(EnumValue);

	UEnvQueryTest* MyTestOb = Cast<UEnvQueryTest>(MyTest.Get());
	if (MyTestOb && MyTestOb->bWorkOnFloatValues &&
		(EnumValue == EEnvTestCondition::AtLeast || EnumValue == EEnvTestCondition::UpTo))
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

EVisibility FEnvQueryTestDetails::GetBoolFilterVisibility() const
{
	uint8 EnumValue;
	ConditionHandle->GetValue(EnumValue);

	UEnvQueryTest* MyTestOb = Cast<UEnvQueryTest>(MyTest.Get());
	if (MyTestOb && !MyTestOb->bWorkOnFloatValues && EnumValue == EEnvTestCondition::Match)
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}


#undef LOCTEXT_NAMESPACE
