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
	//PrimaryComponentTick.bCanEverTick = true;

	//bAutoActivate = true;
	//bTickInEditor = true;
}

UWidget* UContentWidget::GetContent()
{
	return Content;
}

void UContentWidget::SetContent(UWidget* InContent)
{
	Content = InContent;
}

int32 UContentWidget::GetChildrenCount() const
{
	return Content != NULL ? 1 : 0;
}

UWidget* UContentWidget::GetChildAt(int32 Index) const
{
	return Content;
}

bool UContentWidget::AddChild(UWidget* InContent, FVector2D Position)
{
	Content = InContent;
	return true;
}

bool UContentWidget::RemoveChild(UWidget* Child)
{
	if ( Content == Child )
	{
		Content = NULL;
		return true;
	}

	return false;
}

void UContentWidget::ReplaceChildAt(int32 Index, UWidget* Child)
{
	check(Index == 0);
	Content = Child;
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
