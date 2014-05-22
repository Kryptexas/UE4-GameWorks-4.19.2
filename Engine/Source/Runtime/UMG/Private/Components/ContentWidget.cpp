// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#if WITH_EDITOR
#include "MessageLog.h"
#include "UObjectToken.h"
#endif

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UContentWidget

UContentWidget::UContentWidget(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PrimaryComponentTick.bCanEverTick = true;

	bAutoActivate = true;
	bTickInEditor = true;
}

USlateWrapperComponent* UContentWidget::GetContent()
{
	return Content;
}

void UContentWidget::SetContent(USlateWrapperComponent* InContent)
{
	Content = InContent;
}

int32 UContentWidget::GetChildrenCount() const
{
	return Content != NULL ? 1 : 0;
}

USlateWrapperComponent* UContentWidget::GetChildAt(int32 Index) const
{
	return Content;
}

bool UContentWidget::AddChild(USlateWrapperComponent* InContent, FVector2D Position)
{
	Content = InContent;
	return true;
}

bool UContentWidget::RemoveChild(USlateWrapperComponent* Child)
{
	if ( Content == Child )
	{
		Content = NULL;
		return true;
	}

	return false;
}

void UContentWidget::ReplaceChildAt(int32 Index, USlateWrapperComponent* Child)
{
	check(Index == 0);
	Content = Child;
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
