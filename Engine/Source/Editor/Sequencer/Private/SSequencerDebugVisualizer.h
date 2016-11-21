// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

struct FTimeToPixel;

class SSequencerDebugSlot : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SSequencerDebugSlot){}
		SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, int32 InSegmentIndex)
	{
		SegmentIndex = InSegmentIndex;

		ChildSlot
		[
			InArgs._Content.Widget
		];
	}

	int32 GetSegmentIndex() const { return SegmentIndex; }

private:
	int32 SegmentIndex;
};

class SSequencerDebugVisualizer : public SPanel
{
public:
	SLATE_BEGIN_ARGS(SSequencerDebugVisualizer){}
		SLATE_ATTRIBUTE( TRange<float>, ViewRange )
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FSequencer> InSequencer);

protected:
	/** SPanel Interface */
	virtual void OnArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const override;
	virtual FVector2D ComputeDesiredSize(float) const override;
	virtual FChildren* GetChildren() override { return &Children; }

protected:
	
	void Refresh();

	FGeometry GetSegmentGeometry(const FGeometry& AllottedGeometry, const SSequencerDebugSlot& Slot, const FTimeToPixel& TimeToPixelConverter) const;

	EVisibility GetSegmentVisibility(TRange<float> Range) const;

	TSharedRef<SWidget> GetTooltipForSegment(int32 SegmentIndex) const;

	void OnSequenceActivated(FMovieSceneSequenceIDRef);

	const FMovieSceneEvaluationTemplateInstance* GetTemplate() const;

private:

	FMovieSceneSequenceID FocusedSequenceID;

	/** The current view range */
	TAttribute<TRange<float>> ViewRange;

	/** All the widgets in the panel */
	TSlotlessChildren<SSequencerDebugSlot> Children;

	TWeakPtr<FSequencer> WeakSequencer;
};