// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "../EnvironmentQueryEditorPrivatePCH.h"
#include "EnvQueryTest_TraceDetails.h"

#define LOCTEXT_NAMESPACE "EnvQueryTest_TraceDetails"

TSharedRef<IDetailCustomization> FEnvQueryTest_TraceDetails::MakeInstance()
{
	return MakeShareable( new FEnvQueryTest_TraceDetails );
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FEnvQueryTest_TraceDetails::CustomizeDetails( IDetailLayoutBuilder& DetailLayout )
{
	IDetailCategoryBuilder& TraceCategory = DetailLayout.EditCategory(TEXT("Trace"));

	TraceCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest_Trace, TraceChannel));

	ModeHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest_Trace, TraceMode));
	FSimpleDelegate OnModeChangedDelegate = FSimpleDelegate::CreateSP(this, &FEnvQueryTest_TraceDetails::OnModeChanged);
	ModeHandle->SetOnPropertyValueChanged(OnModeChangedDelegate);
	OnModeChanged();

	TraceCategory.AddProperty(ModeHandle);

	IDetailPropertyRow& ExtentXRow = TraceCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest_Trace, TraceExtentX));
	ExtentXRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FEnvQueryTest_TraceDetails::GetExtentXVisibility)));

	IDetailPropertyRow& ExtentYRow = TraceCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest_Trace, TraceExtentY));
	ExtentYRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FEnvQueryTest_TraceDetails::GetExtentYVisibility)));

	IDetailPropertyRow& ExtentZRow = TraceCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UEnvQueryTest_Trace, TraceExtentZ));
	ExtentZRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FEnvQueryTest_TraceDetails::GetExtentZVisibility)));
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void FEnvQueryTest_TraceDetails::OnModeChanged()
{
	uint8 EnumValue = 0;
	ModeHandle->GetValue(EnumValue);

	bShowExtentX = (EnumValue == EEnvTestTrace::Box) || (EnumValue == EEnvTestTrace::Sphere) || (EnumValue == EEnvTestTrace::Capsule);
	bShowExtentY = (EnumValue == EEnvTestTrace::Box);
	bShowExtentZ = (EnumValue == EEnvTestTrace::Box) || (EnumValue == EEnvTestTrace::Capsule);
}

EVisibility FEnvQueryTest_TraceDetails::GetExtentXVisibility() const
{
	return bShowExtentX ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FEnvQueryTest_TraceDetails::GetExtentYVisibility() const
{
	return bShowExtentY ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FEnvQueryTest_TraceDetails::GetExtentZVisibility() const
{
	return bShowExtentZ ? EVisibility::Visible : EVisibility::Collapsed;
}

#undef LOCTEXT_NAMESPACE
