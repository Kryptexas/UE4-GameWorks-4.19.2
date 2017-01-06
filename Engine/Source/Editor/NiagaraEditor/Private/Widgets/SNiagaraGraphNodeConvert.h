// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SGraphNode.h"

class FNiagaraConvertNodeViewModel;
class FNiagaraConvertPinViewModel;

/** A graph node widget representing a niagara convert node. */
class SNiagaraGraphNodeConvert : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SNiagaraGraphNodeConvert) {}
	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs, UEdGraphNode* InGraphNode);

	//~ SGraphNode api
	virtual void UpdateGraphNode() override;

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

protected:
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;

private:
	TSharedPtr<FNiagaraConvertPinViewModel> GetViewModelForPinWidget(TSharedRef<SGraphPin> GraphPin);

private:
	TSharedPtr<FNiagaraConvertNodeViewModel> ConvertNodeViewModel;
};