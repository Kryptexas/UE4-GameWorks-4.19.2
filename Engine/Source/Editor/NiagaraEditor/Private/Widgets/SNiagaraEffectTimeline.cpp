// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SNiagaraEffectTimeline.h"

#include "SCurveEditor.h"
#include "SOverlay.h"

void SNiagaraTimeline::Construct(const FArguments& InArgs)
{
	Emitter = InArgs._Emitter;
	EffectInstance = InArgs._EffectInstance;
	Effect = InArgs._Effect;

	TSharedPtr<SOverlay> OverlayWidget;
	this->ChildSlot
		[
			SAssignNew(CurveEditor, SCurveEditor)
		];
}

void SNiagaraTimeline::SetCurve(UCurveBase *Curve)
{
	CurveEditor->SetCurveOwner(Curve);
}