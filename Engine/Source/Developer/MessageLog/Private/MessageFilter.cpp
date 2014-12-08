// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MessageLogPrivatePCH.h"
#include "MessageFilter.h"

FReply FMessageFilter::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	RefreshCallback.Broadcast();
	return FReply::Handled();
}

ESlateCheckBoxState::Type FMessageFilter::OnGetDisplayCheckState() const
{
	return Display ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void FMessageFilter::OnDisplayCheckStateChanged(ESlateCheckBoxState::Type InNewState)
{
	Display = InNewState == ESlateCheckBoxState::Checked;
	RefreshCallback.Broadcast();
}
