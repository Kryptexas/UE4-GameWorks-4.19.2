// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateComponentWrappersPrivatePCH.h"

/////////////////////////////////////////////////////
// USlateWrapperComponent

USlateWrapperComponent::USlateWrapperComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void USlateWrapperComponent::OnRegister()
{
	MyWidget = RebuildWidget();
	Super::OnRegister();
}

void USlateWrapperComponent::OnUnregister()
{
	MyWidget = NULL;
	Super::OnUnregister();
}

TSharedRef<SWidget> USlateWrapperComponent::GetWidget()
{
	if (MyWidget.IsValid())
	{
		return MyWidget.ToSharedRef();
	}
	else
	{
		return SNullWidget::NullWidget;
	}
}

TSharedRef<SWidget> USlateWrapperComponent::RebuildWidget()
{
	ensureMsg(false, TEXT("You must implement RebuildWidget() in your child class"));
	return SNullWidget::NullWidget;
}

EVisibility USlateWrapperComponent::ConvertSerializedVisibilityToRuntime(TEnumAsByte<ESlateVisibility::Type> Input)
{
	switch (Input)
	{
	case ESlateVisibility::Visible:
		return EVisibility::Visible;
	case ESlateVisibility::Collapsed:
		return EVisibility::Collapsed;
	case ESlateVisibility::Hidden:
		return EVisibility::Hidden;
	case ESlateVisibility::HitTestInvisible:
		return EVisibility::HitTestInvisible;
	case ESlateVisibility::SelfHitTestInvisible:
		return EVisibility::SelfHitTestInvisible;
	default:
		check(false);
		return EVisibility::Visible;
	}
}

FSizeParam USlateWrapperComponent::ConvertSerializedSizeParamToRuntime(const FSlateChildSize& Input)
{
	switch (Input.SizeRule)
	{
	default:
	case ESlateSizeRule::Automatic:
		return FAuto();
	case ESlateSizeRule::AspectRatio:
		return FAspectRatio();
	case ESlateSizeRule::Stretch:
		return FStretch(Input.StretchValue);
	}
}