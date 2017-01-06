// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "SViewportToolBar.h"
#include "SCommonEditorViewportToolbarBase.h"
#include "SNiagaraEffectViewport.h"

// In-viewport toolbar widget used in the Niagara editor
class SNiagaraEffectViewportToolBar : public SCommonEditorViewportToolbarBase
{
public:
	SLATE_BEGIN_ARGS(SNiagaraEffectViewportToolBar) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<class SNiagaraEffectViewport> InViewport);

	// SCommonEditorViewportToolbarBase interface
	virtual TSharedRef<SWidget> GenerateShowMenu() const override;
	// End of SCommonEditorViewportToolbarBase

	virtual bool IsViewModeSupported(EViewModeIndex ViewModeIndex) const override; 
};