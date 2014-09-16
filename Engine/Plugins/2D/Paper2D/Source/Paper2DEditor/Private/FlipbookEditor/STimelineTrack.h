// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace FFlipbookUIConstants
{
	const float HandleWidth = 12.0f;
	const float FrameHeight = 48;
	const FMargin FramePadding(0.0f, 7.0f, 0.0f, 7.0f);
};

//////////////////////////////////////////////////////////////////////////
// FFlipbookKeyFrameDragDropOp

class FFlipbookKeyFrameDragDropOp : public FDragDropOperation
{
public:
	DRAG_DROP_OPERATOR_TYPE(FFlipbookKeyFrameDragDropOp, FDragDropOperation)

	float WidgetWidth;
	FPaperFlipbookKeyFrame KeyFrameData;
	int32 SourceFrameIndex;
	TWeakObjectPtr<UPaperFlipbook> SourceFlipbook;
	FScopedTransaction Transaction;

	virtual TSharedPtr<SWidget> GetDefaultDecorator() const override
	{
		const FLinearColor BorderColor = (KeyFrameData.Sprite != nullptr) ? FLinearColor::White : FLinearColor::Black;

		return SNew(SBox)
			.WidthOverride(WidgetWidth - FFlipbookUIConstants::FramePadding.GetTotalSpaceAlong<Orient_Horizontal>())
			.HeightOverride(FFlipbookUIConstants::FrameHeight - FFlipbookUIConstants::FramePadding.GetTotalSpaceAlong<Orient_Vertical>())
			[
				SNew(SBorder)
				.BorderImage(FPaperStyle::Get()->GetBrush("FlipbookEditor.RegionBody"))
				.BorderBackgroundColor(BorderColor)
				[
					SNullWidget::NullWidget
				]
			];
	}

	virtual void OnDragged(const class FDragDropEvent& DragDropEvent) override
	{
		if (CursorDecoratorWindow.IsValid())
		{
			CursorDecoratorWindow->MoveWindowTo(DragDropEvent.GetScreenSpacePosition());
		}
	}

	virtual void Construct() override
	{
		MouseCursor = EMouseCursor::GrabHandClosed;
		FDragDropOperation::Construct();
	}

	virtual void OnDrop(bool bDropWasHandled, const FPointerEvent& MouseEvent) override
	{
		if (!bDropWasHandled)
		{
			// Add us back to our source, the drop fizzled
			InsertInFlipbook(SourceFlipbook.Get(), SourceFrameIndex);
			Transaction.Cancel();
		}
	}

	void AppendToFlipbook(UPaperFlipbook* DestinationFlipbook)
	{
		DestinationFlipbook->Modify();
		FScopedFlipbookMutator EditLock(DestinationFlipbook);
		EditLock.KeyFrames.Add(KeyFrameData);
	}

	void InsertInFlipbook(UPaperFlipbook* DestinationFlipbook, int32 Index)
	{
		DestinationFlipbook->Modify();
		FScopedFlipbookMutator EditLock(DestinationFlipbook);
		EditLock.KeyFrames.Insert(KeyFrameData, Index);
	}

	void SetCanDropHere(bool bCanDropHere)
	{
		MouseCursor = bCanDropHere ? EMouseCursor::TextEditBeam : EMouseCursor::SlashedCircle;
	}

	static TSharedRef<FFlipbookKeyFrameDragDropOp> New(int32 InWidth, UPaperFlipbook* InFlipbook, int32 InFrameIndex)
	{
		// Create the drag-drop op containing the key
		TSharedRef<FFlipbookKeyFrameDragDropOp> Operation = MakeShareable(new FFlipbookKeyFrameDragDropOp);
		Operation->KeyFrameData = InFlipbook->GetKeyFrameChecked(InFrameIndex);
		Operation->SourceFrameIndex = InFrameIndex;
		Operation->SourceFlipbook = InFlipbook;
		Operation->WidgetWidth = InWidth;
		Operation->Construct();

		// Remove the key from the flipbook
		{
			InFlipbook->Modify();
			FScopedFlipbookMutator EditLock(InFlipbook);
			EditLock.KeyFrames.RemoveAt(InFrameIndex);
		}

		return Operation;
	}

protected:
	FFlipbookKeyFrameDragDropOp()
		: Transaction(LOCTEXT("MovedFramesInTimeline", "Reorder key frames"))
	{
	}
};

//////////////////////////////////////////////////////////////////////////
// SFlipbookKeyframeWidget

class SFlipbookKeyframeWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFlipbookKeyframeWidget)
		: _SlateUnitsPerFrame(1)
		, _FlipbookBeingEdited(nullptr)
	{}

		SLATE_ATTRIBUTE( float, SlateUnitsPerFrame )
		SLATE_ATTRIBUTE( class UPaperFlipbook*, FlipbookBeingEdited )
		SLATE_EVENT( FOnFlipbookKeyframeSelectionChanged, OnSelectionChanged )

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, int32 InFrameIndex, TSharedPtr<const FUICommandList> InCommandList)
	{
		FrameIndex = InFrameIndex;
		CommandList = InCommandList;
		SlateUnitsPerFrame = InArgs._SlateUnitsPerFrame;
		FlipbookBeingEdited = InArgs._FlipbookBeingEdited;
		OnSelectionChanged = InArgs._OnSelectionChanged;

		// Color each region based on whether a sprite has been set or not for it
		const auto BorderColorDelegate = [](TAttribute<UPaperFlipbook*> ThisFlipbookPtr, int32 TestIndex) -> FSlateColor
		{
			UPaperFlipbook* FlipbookPtr = ThisFlipbookPtr.Get();
			const bool bFrameValid = (FlipbookPtr != nullptr) && (FlipbookPtr->GetSpriteAtFrame(TestIndex) != nullptr);
			return bFrameValid ? FLinearColor::White : FLinearColor::Black;
		};

		ChildSlot
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBox)
				.Padding(FFlipbookUIConstants::FramePadding)
				.WidthOverride(this, &SFlipbookKeyframeWidget::GetFrameWidth)
				[
					SNew(SBorder)
					.BorderImage(FPaperStyle::Get()->GetBrush("FlipbookEditor.RegionBody"))
					.BorderBackgroundColor_Static(BorderColorDelegate, FlipbookBeingEdited, FrameIndex)
					.OnMouseButtonUp(this, &SFlipbookKeyframeWidget::KeyframeOnMouseButtonUp)
					.ToolTipText(this, &SFlipbookKeyframeWidget::GetKeyframeTooltip)
					[
						SNullWidget::NullWidget
					]
				]
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBox)
				.WidthOverride(FFlipbookUIConstants::HandleWidth)
				[
					SNew(SFlipbookTrackHandle)
					.SlateUnitsPerFrame(SlateUnitsPerFrame)
					.FlipbookBeingEdited(FlipbookBeingEdited)
					.KeyFrameIdx(FrameIndex)
				]
			]
		];
	}

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
		}

		return FReply::Unhandled();
	}

	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
		{
			UPaperFlipbook* Flipbook = FlipbookBeingEdited.Get();
			if ((Flipbook != nullptr) && Flipbook->IsValidKeyFrameIndex(FrameIndex))
			{
				TSharedRef<FFlipbookKeyFrameDragDropOp> Operation = FFlipbookKeyFrameDragDropOp::New(
					GetFrameWidth().Get(), Flipbook, FrameIndex);

				return FReply::Handled().BeginDragDrop(Operation);
			}
		}

		return FReply::Unhandled();
	}

	FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
	{
		bool bWasDropHandled = false;

		UPaperFlipbook* Flipbook = FlipbookBeingEdited.Get();
		if ((Flipbook != nullptr) && Flipbook->IsValidKeyFrameIndex(FrameIndex))
		{

			TSharedPtr<FDragDropOperation> Operation = DragDropEvent.GetOperation();
			if (!Operation.IsValid())
			{
			}
			else if (Operation->IsOfType<FAssetDragDropOp>())
			{
				const auto& AssetDragDropOp = StaticCastSharedPtr<FAssetDragDropOp>(Operation);
				//@TODO: Handle asset inserts

			// 			OnAssetsDropped(*AssetDragDropOp);
			// 			bWasDropHandled = true;
			}
			else if (Operation->IsOfType<FFlipbookKeyFrameDragDropOp>())
			{
				const auto& FrameDragDropOp = StaticCastSharedPtr<FFlipbookKeyFrameDragDropOp>(Operation);
				FrameDragDropOp->InsertInFlipbook(Flipbook, FrameIndex);
				bWasDropHandled = true;
			}
		}

		return bWasDropHandled ? FReply::Handled() : FReply::Unhandled();
	}

	FReply KeyframeOnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			TSharedRef<SWidget> MenuContents = GenerateContextMenu();
			FSlateApplication::Get().PushMenu(AsShared(), MenuContents, MouseEvent.GetScreenSpacePosition(), FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));

			return FReply::Handled();
		}

		return FReply::Unhandled();
	}

	FText GetKeyframeTooltip() const
	{
		UPaperFlipbook* Flipbook = FlipbookBeingEdited.Get();
		if ((Flipbook != nullptr) && Flipbook->IsValidKeyFrameIndex(FrameIndex))
		{
			const FPaperFlipbookKeyFrame& KeyFrame = Flipbook->GetKeyFrameChecked(FrameIndex);

			FText SpriteLine = (KeyFrame.Sprite != nullptr) ? FText::FromString(KeyFrame.Sprite->GetName()) : LOCTEXT("NoSprite", "(none)");

			return FText::Format(LOCTEXT("KeyFrameTooltip", "Sprite: {0}\nIndex: {1}\nDuration: {2} frame(s)"),
				SpriteLine,
				FText::AsNumber(FrameIndex),
				FText::AsNumber(KeyFrame.FrameRun));
		}
		else
		{
			return LOCTEXT("KeyFrameTooltip_Invalid", "Invalid key frame index");
		}
	}

	TSharedRef<SWidget> GenerateContextMenu()
	{
		OnSelectionChanged.ExecuteIfBound(FrameIndex);

		FMenuBuilder MenuBuilder(true, CommandList);
		MenuBuilder.BeginSection("KeyframeActions", LOCTEXT("KeyframeActionsSectionHeader", "Keyframe Actions"));

		// 		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Cut);
		// 		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Copy);
		// 		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Paste);
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Duplicate);
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Delete);

		MenuBuilder.AddMenuSeparator();

		MenuBuilder.AddMenuEntry(FFlipbookEditorCommands::Get().AddNewFrameBefore);
		MenuBuilder.AddMenuEntry(FFlipbookEditorCommands::Get().AddNewFrameAfter);

		MenuBuilder.EndSection();

		return MenuBuilder.MakeWidget();
	}

	FOptionalSize GetFrameWidth() const
	{
		UPaperFlipbook* Flipbook = FlipbookBeingEdited.Get();
		if (Flipbook && Flipbook->IsValidKeyFrameIndex(FrameIndex))
		{
			const FPaperFlipbookKeyFrame& KeyFrame = Flipbook->GetKeyFrameChecked(FrameIndex);
			return FMath::Max<float>(0, KeyFrame.FrameRun * SlateUnitsPerFrame.Get() - FFlipbookUIConstants::HandleWidth);
		}
		else
		{
			return 1;
		}
	}

protected:
	int32 FrameIndex;
	TAttribute<float> SlateUnitsPerFrame;
	TAttribute<class UPaperFlipbook*> FlipbookBeingEdited;
	FOnFlipbookKeyframeSelectionChanged OnSelectionChanged;
	TSharedPtr<const FUICommandList> CommandList;
};

//////////////////////////////////////////////////////////////////////////
// SFlipbookTimelineTrack

class SFlipbookTimelineTrack : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFlipbookTimelineTrack)
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
		OnSelectionChanged = InArgs._OnSelectionChanged;

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
		int32 NewNumKeyframes = (Flipbook != nullptr) ? Flipbook->GetNumKeyFrames() : 0;
		if (NewNumKeyframes != NumKeyframesFromLastRebuild)
		{
			Rebuild();
		}
	}

private:
	void Rebuild()
	{
		MainBoxPtr->ClearChildren();

		// Create the sections for each keyframe
		if (UPaperFlipbook* Flipbook = FlipbookBeingEdited.Get())
		{
			for (int32 KeyFrameIdx = 0; KeyFrameIdx < Flipbook->GetNumKeyFrames(); ++KeyFrameIdx)
			{
				//@TODO: Draggy bits go here
				MainBoxPtr->AddSlot()
				.AutoWidth()
				[
					SNew(SFlipbookKeyframeWidget, KeyFrameIdx, CommandList)
					.SlateUnitsPerFrame(this->SlateUnitsPerFrame)
					.FlipbookBeingEdited(this->FlipbookBeingEdited)
					.OnSelectionChanged(this->OnSelectionChanged)
				];
			}

			NumKeyframesFromLastRebuild = Flipbook->GetNumKeyFrames();
		}
		else
		{
			NumKeyframesFromLastRebuild = 0;
		}
	}

private:
	TAttribute<float> SlateUnitsPerFrame;
	TAttribute< class UPaperFlipbook* > FlipbookBeingEdited;

	TSharedPtr<SHorizontalBox> MainBoxPtr;

	int32 NumKeyframesFromLastRebuild;
	float HandleWidth;

	FOnFlipbookKeyframeSelectionChanged OnSelectionChanged;
	TSharedPtr<const FUICommandList> CommandList;
};
