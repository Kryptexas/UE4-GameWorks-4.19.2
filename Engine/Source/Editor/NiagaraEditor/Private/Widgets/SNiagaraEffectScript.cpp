// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SNiagaraEffectScript.h"
#include "NiagaraEffectViewModel.h"
#include "NiagaraEffectScriptViewModel.h"
#include "NiagaraScriptViewModel.h"
#include "NiagaraScriptInputCollectionViewModel.h"
#include "SNiagaraParameterCollection.h"
#include "SNiagaraScriptGraph.h"

#include "SSplitter.h"

void SNiagaraEffectScript::Construct(const FArguments& InArgs, TSharedRef<FNiagaraEffectViewModel> InEffectViewModel)
{
	EffectViewModel = InEffectViewModel;
	ChildSlot
	[
		SNew(SSplitter)
		+ SSplitter::Slot()
		.Value(0.3f)
		[
			SNew(SNiagaraParameterCollection, EffectViewModel->GetEffectScriptViewModel()->GetInputCollectionViewModel())
		]
		+ SSplitter::Slot()
		.Value(0.7f)
		[
			SNew(SNiagaraScriptGraph, EffectViewModel->GetEffectScriptViewModel()->GetGraphViewModel())
		]
	];
}