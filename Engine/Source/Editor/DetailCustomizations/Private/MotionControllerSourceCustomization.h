// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "SCompoundWidget.h"

class SWidget;
class IPropertyHandle;

DECLARE_DELEGATE_RetVal(FText, FOnGetMotionSourceText)
DECLARE_DELEGATE_OneParam(FOnMotionSourceChanged, FName)

class SMotionSourceWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMotionSourceWidget)
	{}
		SLATE_EVENT(FOnGetMotionSourceText, OnGetMotionSourceText)
		SLATE_EVENT(FOnMotionSourceChanged, OnMotionSourceChanged)
	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs);

private:

	// Generate the motion source combo box popup menu
	TSharedRef<SWidget> BuildMotionSourceMenu();

	// Handler for setting combo box text
	FText GetMotionSourceText() const;

	// Handler new values from the motion source combo box
	void OnMotionSourceValueTextComitted(const FText& InNewText, ETextCommit::Type InTextCommit);
	void OnMotionSourceComboValueCommited(FName InMotionSource);

	// Delegates for interaction with source
	FOnGetMotionSourceText OnGetMotionSourceText;
	FOnMotionSourceChanged OnMotionSourceChanged;
};