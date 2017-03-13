// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "SCompoundWidget.h"
#include "DeclarativeSyntaxSupport.h"

class SCurveEditor;
struct FNiagaraSimulation;
class FNiagaraEffectInstance;
class UNiagaraEffect;
class UCurveBase;

class SNiagaraTimeline : public SCompoundWidget
{
private:
	TSharedPtr<FNiagaraSimulation> Emitter;
	UNiagaraEffect *Effect;
	FNiagaraEffectInstance *EffectInstance;
	TSharedPtr<SCurveEditor>CurveEditor;

public:
	SLATE_BEGIN_ARGS(SNiagaraTimeline) 
		: _Emitter(nullptr)
		{ }

		SLATE_ARGUMENT(TSharedPtr<FNiagaraSimulation>, Emitter)
		SLATE_ARGUMENT(FNiagaraEffectInstance*, EffectInstance)
		SLATE_ARGUMENT(UNiagaraEffect*, Effect)

	SLATE_END_ARGS()

	virtual void Construct(const FArguments& InArgs);

	void SetCurve(UCurveBase *Curve);

};