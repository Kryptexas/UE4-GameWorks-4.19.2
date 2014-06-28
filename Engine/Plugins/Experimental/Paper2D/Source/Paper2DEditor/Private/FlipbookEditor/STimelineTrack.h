// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//////////////////////////////////////////////////////////////////////////
// STimelineTrack

class STimelineTrack : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STimelineTrack)
		: _SlateUnitsPerFrame(1)
		, _FlipbookBeingEdited(nullptr)
	{}

		SLATE_ATTRIBUTE( float, SlateUnitsPerFrame )
		SLATE_ATTRIBUTE( class UPaperFlipbook*, FlipbookBeingEdited )

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		SlateUnitsPerFrame = InArgs._SlateUnitsPerFrame;
		FlipbookBeingEdited = InArgs._FlipbookBeingEdited;

		HandleWidth = 4;
		NumKeyframesFromLastRebuild = 0;

		ChildSlot
		[
			SAssignNew(MainBoxPtr, SHorizontalBox)
		];

		Rebuild();
	}

	void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
	{
		SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

		UPaperFlipbook* Flipbook = FlipbookBeingEdited.Get();
		int32 NewNumKeyframes = (Flipbook != NULL ? Flipbook->KeyFrames.Num() : 0);
		if (NewNumKeyframes != NumKeyframesFromLastRebuild)
		{
			Rebuild();
		}
	}

private:
	void Rebuild()
	{
		MainBoxPtr->ClearChildren();

		if (UPaperFlipbook* Flipbook = FlipbookBeingEdited.Get())
		{
			for (int32 KeyFrameIdx = 0; KeyFrameIdx < Flipbook->KeyFrames.Num(); ++KeyFrameIdx)
			{
				MainBoxPtr->AddSlot()
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride( TAttribute<FOptionalSize>::Create( TAttribute<FOptionalSize>::FGetter::CreateSP(this, &STimelineTrack::GetFrameWidth, KeyFrameIdx) ) )
					[
						SNew(SBorder)
						.BorderImage( FEditorStyle::GetBrush("PropertyTable.CurrentCellBorder") )
						[
							SNullWidget::NullWidget
						]
					]
				];

				MainBoxPtr->AddSlot()
				.AutoWidth()
				[
					SNew(SBox).WidthOverride(HandleWidth)
					[
						SNew(SFlipbookTrackHandle)
						.SlateUnitsPerFrame(SlateUnitsPerFrame)
						.FlipbookBeingEdited(FlipbookBeingEdited)
						.KeyFrameIdx(KeyFrameIdx)
					]
				];
			}

			NumKeyframesFromLastRebuild = Flipbook->KeyFrames.Num();
		}
		else
		{
			NumKeyframesFromLastRebuild = 0;
		}
	}

	FOptionalSize GetFrameWidth(int32 KeyFrameIdx) const
	{
		UPaperFlipbook* Flipbook = FlipbookBeingEdited.Get();
		if (Flipbook && Flipbook->KeyFrames.IsValidIndex(KeyFrameIdx))
		{
			const FPaperFlipbookKeyFrame& KeyFrame = Flipbook->KeyFrames[KeyFrameIdx];
			return FMath::Max<float>(0, KeyFrame.FrameRun * SlateUnitsPerFrame.Get() - HandleWidth);
		}
		else
		{
			return 1;
		}
	}

private:
	TAttribute<float> SlateUnitsPerFrame;
	TAttribute< class UPaperFlipbook* > FlipbookBeingEdited;

	TSharedPtr<SHorizontalBox> MainBoxPtr;

	int32 NumKeyframesFromLastRebuild;
	float HandleWidth;
};
