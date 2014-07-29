// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UNativeWidgetHost

UNativeWidgetHost::UNativeWidgetHost(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;
}

void UNativeWidgetHost::SetContent(TSharedRef<SWidget> InContent)
{
	MyWidget = InContent;
}

void UNativeWidgetHost::ReleaseNativeWidget()
{
	Super::ReleaseNativeWidget();

	MyWidget.Reset();
}

TSharedRef<SWidget> UNativeWidgetHost::RebuildWidget()
{
	if ( MyWidget.IsValid() )
	{
		return MyWidget.ToSharedRef();
	}
	else
	{
		return SNew(SBorder)
		.Visibility(EVisibility::HitTestInvisible)
		.BorderImage(FUMGStyle::Get().GetBrush("MarchingAnts"))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NativeWidgetHostText", "Slate Widget Host"))
		];
	}
}

#if WITH_EDITOR

const FSlateBrush* UNativeWidgetHost::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.NativeWidgetHost");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
