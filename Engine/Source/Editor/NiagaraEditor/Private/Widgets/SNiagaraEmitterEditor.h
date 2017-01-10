// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "SCompoundWidget.h"
#include "DeclarativeSyntaxSupport.h"

class FNiagaraEmitterViewModel;
class IDetailsView;

/** A widget for viewing and editing a niagara emitter. */
class SNiagaraEmitterEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNiagaraEmitterEditor)
		{}

	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs, TSharedRef<FNiagaraEmitterViewModel> InViewModel);

private:
	/** Called when the emitter for the emitter view model changes to a different emitter. */
	void EmitterChanged();

private:
	/** A details view which displays the emitter properties. */
	TSharedPtr<IDetailsView> DetailsView;

	/** The view model which exposes the data used by the widget. */
	TSharedPtr<FNiagaraEmitterViewModel> ViewModel;
};