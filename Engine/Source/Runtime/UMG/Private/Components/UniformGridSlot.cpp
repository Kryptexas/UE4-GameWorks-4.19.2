// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UUniformGridSlot

UUniformGridSlot::UUniformGridSlot(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, Slot(NULL)
{
	HorizontalAlignment = HAlign_Left;
	VerticalAlignment = VAlign_Top;
}

void UUniformGridSlot::BuildSlot(TSharedRef<SUniformGridPanel> GridPanel)
{
	Slot = &GridPanel->AddSlot(Column, Row)
		[
			Content == NULL ? SNullWidget::NullWidget : Content->GetWidget()
		];
}

void UUniformGridSlot::SetRow(int32 InRow)
{
	Row = InRow;
	if ( Slot )
	{
		Slot->Row = InRow;
	}
}

void UUniformGridSlot::SetColumn(int32 InColumn)
{
	Column = InColumn;
	if ( Slot )
	{
		Slot->Column = InColumn;
	}
}

void UUniformGridSlot::SetHorizontalAlignment(EHorizontalAlignment InHorizontalAlignment)
{
	HorizontalAlignment = InHorizontalAlignment;
	if ( Slot )
	{
		Slot->HAlignment = InHorizontalAlignment;
	}
}

void UUniformGridSlot::SetVerticalAlignment(EVerticalAlignment InVerticalAlignment)
{
	VerticalAlignment = InVerticalAlignment;
	if ( Slot )
	{
		Slot->VAlignment = InVerticalAlignment;
	}
}

void UUniformGridSlot::SyncronizeProperties()
{
	SetRow(Row);
	SetColumn(Column);
	SetHorizontalAlignment(HorizontalAlignment);
	SetVerticalAlignment(VerticalAlignment);
}
