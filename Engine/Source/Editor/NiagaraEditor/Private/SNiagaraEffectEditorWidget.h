// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Visibility.h"
#include "SlateBasics.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Editor/PropertyEditor/Public/IDetailsView.h"


class SNiagaraEffectEditorWidget : public SCompoundWidget, public FNotifyHook
{
private:
	UNiagaraEffect *EffectObj;

	/** Notification list to pass messages to editor users  */
	TSharedPtr<SNotificationList> NotificationListPtr;
	TSharedPtr<class IDetailsView> EmitterDetails;
	SOverlay::FOverlaySlot* DetailsViewSlot;

public:
	SLATE_BEGIN_ARGS(SNiagaraEffectEditorWidget)
		: _EffectObj(nullptr)
	{}
		SLATE_ARGUMENT(UNiagaraEffect*, EffectObj)
		SLATE_ARGUMENT(TSharedPtr<SWidget>, TitleBar)
	SLATE_END_ARGS()

	/** Function to check whether we should show read-only text in the panel */
	EVisibility ReadOnlyVisibility() const
	{
		return EVisibility::Hidden;
	}

	void Construct(const FArguments& InArgs);
};

