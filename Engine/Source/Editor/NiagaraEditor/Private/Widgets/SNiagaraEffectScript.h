// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "SCompoundWidget.h"
#include "DeclarativeSyntaxSupport.h"

class FNiagaraEffectViewModel;

/** A widget for editing the effect script. */
class SNiagaraEffectScript : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNiagaraEffectScript) {}
	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs, TSharedRef<FNiagaraEffectViewModel> InEffectViewModel);

private:
	/** The effect view model that owns the effect script being edited. */
	TSharedPtr<FNiagaraEffectViewModel> EffectViewModel;
};