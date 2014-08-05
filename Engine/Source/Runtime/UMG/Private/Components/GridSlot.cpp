// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UGridSlot

UGridSlot::UGridSlot(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, Slot(NULL)
{
	HorizontalAlignment = HAlign_Fill;
	VerticalAlignment = VAlign_Fill;

	Layer = 0;
	Nudge = FVector2D(0, 0);
}

void UGridSlot::ReleaseNativeWidget()
{
	Super::ReleaseNativeWidget();

	Slot = NULL;
}

void UGridSlot::BuildSlot(TSharedRef<SGridPanel> GridPanel)
{
	Slot = &GridPanel->AddSlot(Column, Row)
		[
			Content == NULL ? SNullWidget::NullWidget : Content->TakeWidget()
		];
}

void UGridSlot::SetRow(int32 InRow)
{
	Row = InRow;
	if ( Slot )
	{
		Slot->Row(InRow);
	}
}

void UGridSlot::SetRowSpan(int32 InRowSpan)
{
	RowSpan = InRowSpan;
	if ( Slot )
	{
		Slot->RowSpan(InRowSpan);
	}
}

void UGridSlot::SetColumn(int32 InColumn)
{
	Column = InColumn;
	if ( Slot )
	{
		Slot->Column(InColumn);
	}
}

void UGridSlot::SetColumnSpan(int32 InColumnSpan)
{
	ColumnSpan = InColumnSpan;
	if ( Slot )
	{
		Slot->ColumnSpan(InColumnSpan);
	}
}

void UGridSlot::SetHorizontalAlignment(EHorizontalAlignment InHorizontalAlignment)
{
	HorizontalAlignment = InHorizontalAlignment;
	if ( Slot )
	{
		Slot->HAlignment = InHorizontalAlignment;
	}
}

void UGridSlot::SetVerticalAlignment(EVerticalAlignment InVerticalAlignment)
{
	VerticalAlignment = InVerticalAlignment;
	if ( Slot )
	{
		Slot->VAlignment = InVerticalAlignment;
	}
}

void UGridSlot::SyncronizeProperties()
{
	SetRow(Row);
	SetRowSpan(RowSpan);
	SetColumn(Column);
	SetColumnSpan(ColumnSpan);
	SetHorizontalAlignment(HorizontalAlignment);
	SetVerticalAlignment(VerticalAlignment);
}
