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
		SLATE_EVENT( FOnFlipbookKeyframeSelectionChanged, OnSelectionChanged )

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<const FUICommandList> InCommandList)
	{
		CommandList = InCommandList;
		SlateUnitsPerFrame = InArgs._SlateUnitsPerFrame;
		FlipbookBeingEdited = InArgs._FlipbookBeingEdited;
		SelectedFrame = INDEX_NONE;
		OnSelectionChanged = InArgs._OnSelectionChanged;

		HandleWidth = 12;
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

	FReply KeyframeOnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, int32 FrameIndex)
	{
		if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			TSharedRef<SWidget> MenuContents = GenerateContextMenu(FrameIndex);
			FSlateApplication::Get().PushMenu(AsShared(), MenuContents, MouseEvent.GetScreenSpacePosition(), FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));

			return FReply::Handled();
		}

		return FReply::Unhandled();
	}

private:
	void Rebuild()
	{
		MainBoxPtr->ClearChildren();

		// Color each region based on whether a sprite has been set or not for it
		const auto BorderColorDelegate = [](TAttribute<UPaperFlipbook*> ThisFlipbookPtr, int32 TestIndex) -> FSlateColor
		{
			UPaperFlipbook* FlipbookPtr = ThisFlipbookPtr.Get();
			const bool bFrameValid = (FlipbookPtr != nullptr) && (FlipbookPtr->GetSpriteAtFrame(TestIndex) != nullptr);
			return bFrameValid ? FLinearColor::White : FLinearColor::Black;
		};

		// Create the sections for each keyframe
		if (UPaperFlipbook* Flipbook = FlipbookBeingEdited.Get())
		{
			for (int32 KeyFrameIdx = 0; KeyFrameIdx < Flipbook->KeyFrames.Num(); ++KeyFrameIdx)
			{
				MainBoxPtr->AddSlot()
				.AutoWidth()
				.Padding(FMargin(0.0f, 7.0f, 0.0f, 7.0f))
				[
					SNew(SBox)
					.WidthOverride( TAttribute<FOptionalSize>::Create( TAttribute<FOptionalSize>::FGetter::CreateSP(this, &STimelineTrack::GetFrameWidth, KeyFrameIdx) ) )
					[
						SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("FlipbookEditor.RegionBorder"))
						.BorderBackgroundColor_Static(BorderColorDelegate, FlipbookBeingEdited, KeyFrameIdx)
						.OnMouseButtonUp(this, &STimelineTrack::KeyframeOnMouseButtonUp, KeyFrameIdx)
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

	TSharedRef<SWidget> GenerateContextMenu(int32 FrameIndex)
	{
		SelectedFrame = FrameIndex;
		OnSelectionChanged.ExecuteIfBound(SelectedFrame);

		FMenuBuilder MenuBuilder(true, CommandList);
		MenuBuilder.BeginSection("KeyframeActions", LOCTEXT("KeyframeActionsSectionHeader", "Keyframe Actions"));

// 		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Cut);
// 		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Copy);
// 		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Paste);
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Duplicate);
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Delete);

		MenuBuilder.EndSection();

		return MenuBuilder.MakeWidget();
	}
private:
	int32 SelectedFrame;
	TAttribute<float> SlateUnitsPerFrame;
	TAttribute< class UPaperFlipbook* > FlipbookBeingEdited;

	TSharedPtr<SHorizontalBox> MainBoxPtr;

	int32 NumKeyframesFromLastRebuild;
	float HandleWidth;

	FOnFlipbookKeyframeSelectionChanged OnSelectionChanged;
	TSharedPtr<const FUICommandList> CommandList;
};
