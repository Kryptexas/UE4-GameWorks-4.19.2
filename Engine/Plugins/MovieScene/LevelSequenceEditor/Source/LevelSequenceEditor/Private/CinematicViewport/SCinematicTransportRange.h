// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class SCinematicTransportRange : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SCinematicTransportRange){}
	SLATE_END_ARGS()

	/** Construct this widget */
	void Construct(const FArguments& InArgs);

	/** Assign a new sequencer to this transport */
	void SetSequencer(TWeakPtr<ISequencer> InSequencer);

	virtual FVector2D ComputeDesiredSize(float) const override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseCaptureLost() override;

private:

	void SetTime(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	ISequencer* GetSequencer() const;

	/** The sequencer that we're controlling */
	TWeakPtr<ISequencer> WeakSequencer;

	bool bDraggingTime;
};