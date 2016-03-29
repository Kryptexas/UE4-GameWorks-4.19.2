// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SGraphNodeK2Base.h"

class SAnimationGraphNode : public SGraphNodeK2Base
{
public:
	SLATE_BEGIN_ARGS(SAnimationGraphNode) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UAnimGraphNode_Base* InNode);

protected:
	// SGraphNode interface
	virtual TArray<FOverlayWidgetInfo> GetOverlayWidgets(bool bSelected, const FVector2D& WidgetSize) const override;
	// End of SGraphNode interface

private:
	/** Keep a reference to the indicator widget handing around */
	TSharedPtr<SWidget> IndicatorWidget;
};