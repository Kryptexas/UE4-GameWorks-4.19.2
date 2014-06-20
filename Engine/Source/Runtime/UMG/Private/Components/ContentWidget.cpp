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
	ChildSlot = PCIP.CreateDefaultSubobject<UPanelSlot>(this, TEXT("ChildSlot"));
	ChildSlot.Get()->SetFlags(RF_Transactional);
	ChildSlot.Get()->Parent = this;
}

UPanelSlot* UContentWidget::GetContentSlot() const
{
	//if ( ContentSlot == NULL )
	//{
	//	UContentWidget* MutableThis = const_cast<UContentWidget*>( this );
	//	MutableThis->ContentSlot = ConstructObject<UPanelSlot>(UPanelSlot::StaticClass(), MutableThis);

	//	MutableThis->ContentSlot->SetFlags(RF_Transactional);
	//	//MutableThis->ContentSlot->SetFlags(RF_DefaultSubObject);
	//	MutableThis->ContentSlot->Parent = MutableThis;
	//}

	return ChildSlot.Get();
}

UWidget* UContentWidget::GetContent()
{
	return GetContentSlot()->Content;
}

void UContentWidget::SetContent(UWidget* InContent)
{
	GetContentSlot()->Content = InContent;

	if ( InContent )
	{
		InContent->Slot = GetContentSlot();
	}
}

int32 UContentWidget::GetChildrenCount() const
{
	return GetContentSlot()->Content != NULL ? 1 : 0;
}

UWidget* UContentWidget::GetChildAt(int32 Index) const
{
	return GetContentSlot()->Content;
}

bool UContentWidget::AddChild(UWidget* InContent, FVector2D Position)
{
	if ( GetContentSlot()->Content == NULL )
	{
		SetContent(InContent);
		return true;
	}

	return false;
}

bool UContentWidget::RemoveChild(UWidget* Child)
{
	if ( GetContentSlot()->Content == Child )
	{
		SetContent(NULL);
		return true;
	}

	return false;
}

void UContentWidget::ReplaceChildAt(int32 Index, UWidget* Child)
{
	check(Index == 0);
	SetContent(Child);
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
