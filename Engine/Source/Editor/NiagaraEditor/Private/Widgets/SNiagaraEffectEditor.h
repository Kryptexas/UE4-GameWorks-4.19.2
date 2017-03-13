// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SCompoundWidget.h"
#include "DeclarativeSyntaxSupport.h"

class FNiagaraEffectViewModel;
class FNiagaraEmitterHandleViewModel;
class SScrollBox;
class SWidget;

class SNiagaraEffectEditor : public SCompoundWidget
{
private:
	TSharedPtr<FNiagaraEffectViewModel> EffectViewModel;

	/**Scroll Box for dev emitters view.*/
	TSharedPtr<SScrollBox> EmitterBox;

public:
	SLATE_BEGIN_ARGS(SNiagaraEffectEditor) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FNiagaraEffectViewModel> InEffectViewModel);

private:
	void EmitterHandleViewModelsChanged();

	void ConstructEmittersView();

	TSharedRef<SWidget> OnGetHeaderMenuContent(TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel);

	void OpenEmitter(TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel);
};
