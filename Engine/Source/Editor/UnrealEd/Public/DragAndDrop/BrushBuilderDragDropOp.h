// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FBrushBuilderDragDropOp : public FDragDropOperation
{
public:
	static FString GetTypeId() {static FString Type = TEXT("FBspBrushDragDropOp"); return Type;}

	static TSharedRef<FBrushBuilderDragDropOp> New(TWeakObjectPtr<UBrushBuilder> InBrushBuilder, const FSlateBrush* InIconBrush, bool bInIsAddtive)
	{
		TSharedRef<FBrushBuilderDragDropOp> Operation = MakeShareable(new FBrushBuilderDragDropOp(InBrushBuilder, InIconBrush, bInIsAddtive));
		FSlateApplication::GetDragDropReflector().RegisterOperation<FBrushBuilderDragDropOp>(Operation);

		Operation->MouseCursor = EMouseCursor::GrabHandClosed;

		Operation->Construct();
		return Operation;
	}

	~FBrushBuilderDragDropOp()
	{
		UWorld* World = GEditor->GetEditorWorldContext().World();
		if(World != nullptr)
		{
			// Deselect & hide the builder brush
			World->GetBrush()->SetIsTemporarilyHiddenInEditor(true);
			GEditor->SelectActor(World->GetBrush(), false, false);
		}
	}

	virtual void OnDrop( bool bDropWasHandled, const FPointerEvent& MouseEvent ) OVERRIDE
	{
		UWorld* World = GEditor->GetEditorWorldContext().World();
		if(World != nullptr)
		{
			// Cut the BSP
			if(bDropWasHandled)
			{
				World->GetBrush()->BrushBuilder = DuplicateObject<UBrushBuilder>(BrushBuilder.Get(), World->GetBrush()->GetOuter());
				const TCHAR* Command = bIsAdditive ? TEXT("BRUSH ADD SELECTNEWBRUSH") : TEXT("BRUSH SUBTRACT SELECTNEWBRUSH");
				GEditor->Exec(World, Command);
			}

			// Deselect & hide the builder brush
			World->GetBrush()->SetIsTemporarilyHiddenInEditor(true);
			GEditor->SelectActor(World->GetBrush(), false, false);
		}

		FDragDropOperation::OnDrop( bDropWasHandled, MouseEvent );
	}

	virtual void OnDragged( const FDragDropEvent& DragDropEvent ) OVERRIDE
	{
		UWorld* World = GEditor->GetEditorWorldContext().World();
		if(World != nullptr)
		{
			if(CursorDecoratorWindow->IsVisible() && !World->GetBrush()->IsTemporarilyHiddenInEditor())
			{
				// Deselect & hide the builder brush if the decorator is visible
				World->GetBrush()->SetIsTemporarilyHiddenInEditor(true);
				GEditor->SelectActor(World->GetBrush(), false, false);
			}
		}

		FDragDropOperation::OnDragged(DragDropEvent);
	}

	virtual TSharedPtr<class SWidget> GetDefaultDecorator() const OVERRIDE
	{
		return SNew(SBox)
			.WidthOverride(100.0f)
			.HeightOverride(100.0f)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("AssetThumbnail.ClassBackground"))
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Image(IconBrush)
				]
			];
	}

	TWeakObjectPtr<UBrushBuilder> GetBrushBuilder() const
	{
		return BrushBuilder;
	}

private:
	FBrushBuilderDragDropOp(TWeakObjectPtr<UBrushBuilder> InBrushBuilder, const FSlateBrush* InIconBrush, bool bInIsAdditive)
		: BrushBuilder(InBrushBuilder)
		, IconBrush(InIconBrush)
		, bIsAdditive(bInIsAdditive)
	{
		// show & select the builder brush
		UWorld* World = GEditor->GetEditorWorldContext().World();
		World->GetBrush()->SetIsTemporarilyHiddenInEditor(false);
		GEditor->SelectActor(World->GetBrush(), true, false);
	}

private:

	/** The brush builder */
	TWeakObjectPtr<UBrushBuilder> BrushBuilder;

	/** The icon to display when dragging */
	const FSlateBrush* IconBrush;

	/** Additive or subtractive mode */
	bool bIsAdditive;
};