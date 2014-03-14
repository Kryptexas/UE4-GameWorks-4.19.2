// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "../EnvironmentQueryEditorPrivatePCH.h"
#include "EnvQueryTest_DotDetails.h"

#define LOCTEXT_NAMESPACE "EnvQueryTest_DotDetails"

TSharedRef<IDetailCustomization> FEnvQueryTest_DotDetails::MakeInstance()
{
	return MakeShareable( new FEnvQueryTest_DotDetails );
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FEnvQueryTest_DotDetails::CustomizeDetails( IDetailLayoutBuilder& DetailLayout )
{
	IDetailCategoryBuilder& DotCategory = DetailLayout.EditCategory(TEXT("Dot"));

	LineAHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest_Dot, LineA));
	FSimpleDelegate OnModeAChangedDelegate = FSimpleDelegate::CreateSP(this, &FEnvQueryTest_DotDetails::OnModeAChanged);
	LineAHandle->SetOnPropertyValueChanged(OnModeAChangedDelegate);
	OnModeAChanged();

	DotCategory.AddProperty(LineAHandle);

	IDetailPropertyRow& LineAFromRow = DotCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest_Dot, LineAFrom));
	LineAFromRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FEnvQueryTest_DotDetails::GetLineASegmentVisibility)));

	IDetailPropertyRow& LineAToRow = DotCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest_Dot, LineATo));
	LineAToRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FEnvQueryTest_DotDetails::GetLineASegmentVisibility)));

	IDetailPropertyRow& LineADirRow = DotCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest_Dot, LineADirection));
	LineADirRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FEnvQueryTest_DotDetails::GetLineADirectionVisibility)));

	LineBHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest_Dot, LineB));
	FSimpleDelegate OnModeBChangedDelegate = FSimpleDelegate::CreateSP(this, &FEnvQueryTest_DotDetails::OnModeBChanged);
	LineBHandle->SetOnPropertyValueChanged(OnModeBChangedDelegate);
	OnModeBChanged();

	DotCategory.AddProperty(LineBHandle);

	IDetailPropertyRow& LineBFromRow = DotCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest_Dot, LineBFrom));
	LineBFromRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FEnvQueryTest_DotDetails::GetLineBSegmentVisibility)));

	IDetailPropertyRow& LineBToRow = DotCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest_Dot, LineBTo));
	LineBToRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FEnvQueryTest_DotDetails::GetLineBSegmentVisibility)));

	IDetailPropertyRow& LineBDirRow = DotCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest_Dot, LineBDirection));
	LineBDirRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FEnvQueryTest_DotDetails::GetLineBDirectionVisibility)));
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void FEnvQueryTest_DotDetails::OnModeAChanged()
{
	uint8 EnumValue = 0;
	LineAHandle->GetValue(EnumValue);

	bModeASegment = (EnumValue == EEnvTestDot::Segment);
}

void FEnvQueryTest_DotDetails::OnModeBChanged()
{
	uint8 EnumValue = 0;
	LineBHandle->GetValue(EnumValue);

	bModeBSegment = (EnumValue == EEnvTestDot::Segment);
}

EVisibility FEnvQueryTest_DotDetails::GetLineASegmentVisibility() const
{
	return bModeASegment ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FEnvQueryTest_DotDetails::GetLineADirectionVisibility() const
{
	return bModeASegment ? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility FEnvQueryTest_DotDetails::GetLineBSegmentVisibility() const
{
	return bModeBSegment ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FEnvQueryTest_DotDetails::GetLineBDirectionVisibility() const
{
	return bModeBSegment ? EVisibility::Collapsed : EVisibility::Visible;
}

#undef LOCTEXT_NAMESPACE
