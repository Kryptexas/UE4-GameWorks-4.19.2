// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "Sequencer.h"
#include "SSection.h"
#include "IKeyArea.h"
#include "ISequencerSection.h"
#include "MovieSceneSection.h"
#include "MovieSceneShotSection.h"
#include "CommonMovieSceneTools.h"
#include "SequencerHotspots.h"


/** When 0, regeneration of dynamic key layouts is enabled, when non-zero, such behaviour is disabled */
FThreadSafeCounter LayoutRegenerationLock;

void SSection::DisableLayoutRegeneration()
{
	LayoutRegenerationLock.Increment();
}

void SSection::EnableLayoutRegeneration()
{
	LayoutRegenerationLock.Decrement();
}

void SSection::Construct( const FArguments& InArgs, TSharedRef<FTrackNode> SectionNode, int32 InSectionIndex )
{
	SectionIndex = InSectionIndex;
	ParentSectionArea = SectionNode;
	SectionInterface = SectionNode->GetSections()[InSectionIndex];
	Layout = FKeyAreaLayout(*SectionNode, InSectionIndex);

	ResetState();

	ChildSlot
	[
		SectionInterface->GenerateSectionWidget()
	];
}

FVector2D SSection::ComputeDesiredSize(float) const
{
	return FVector2D(100, Layout->GetTotalHeight());
}

FGeometry SSection::GetKeyAreaGeometry( const FKeyAreaLayoutElement& KeyArea, const FGeometry& SectionGeometry ) const
{
	// Compute the geometry for the key area
	return SectionGeometry.MakeChild( FVector2D( 0, KeyArea.GetOffset() ), FVector2D( SectionGeometry.Size.X, KeyArea.GetHeight(SectionGeometry) ) );
}

FSelectedKey SSection::GetKeyUnderMouse( const FVector2D& MousePosition, const FGeometry& AllottedGeometry ) const
{
	UMovieSceneSection& Section = *SectionInterface->GetSectionObject();

	// Search every key area until we find the one under the mouse
	for (const FKeyAreaLayoutElement& Element : Layout->GetElements())
	{
		TSharedPtr<IKeyArea> KeyArea = Element.GetKeyArea();

		// Compute the current key area geometry
		FGeometry KeyAreaGeometryPadded = GetKeyAreaGeometry( Element, AllottedGeometry );

		// Is the key area under the mouse
		if( KeyAreaGeometryPadded.IsUnderLocation( MousePosition ) )
		{
			// Do not display section handles for connected sections
			const bool bDisplaySectionHandles = SectionInterface->AreSectionsConnected();

			const float SectionGripSize = bDisplaySectionHandles ? SectionInterface->GetSectionGripSize() : 0.0f;

			FGeometry SectionGeometry = AllottedGeometry.MakeChild(FVector2D(SectionGripSize, 0), AllottedGeometry.GetDrawSize() - FVector2D(SectionGripSize*2, 0.0f));
			FGeometry KeyAreaGeometry = GetKeyAreaGeometry( Element, SectionGeometry );

			FVector2D LocalSpaceMousePosition = KeyAreaGeometry.AbsoluteToLocal( MousePosition );

			FTimeToPixel TimeToPixelConverter = Section.IsInfinite() ? 			
				FTimeToPixel( ParentGeometry, GetSequencer().GetViewRange()) : 
				FTimeToPixel( KeyAreaGeometry, TRange<float>( Section.GetStartTime(), Section.GetEndTime() ) );

			// Check each key until we find one under the mouse (if any)
			TArray<FKeyHandle> KeyHandles = KeyArea->GetUnsortedKeyHandles();
			for( int32 KeyIndex = 0; KeyIndex < KeyHandles.Num(); ++KeyIndex )
			{
				FKeyHandle KeyHandle = KeyHandles[KeyIndex];
				float KeyPosition = TimeToPixelConverter.TimeToPixel( KeyArea->GetKeyTime(KeyHandle) );

				FGeometry KeyGeometry = KeyAreaGeometry.MakeChild( 
					FVector2D( KeyPosition - FMath::TruncToFloat(SequencerSectionConstants::KeySize.X/2.0f), ((KeyAreaGeometry.Size.Y*.5f)-(SequencerSectionConstants::KeySize.Y*.5f)) ),
					SequencerSectionConstants::KeySize );

				if( KeyGeometry.IsUnderLocation( MousePosition ) )
				{
					// The current key is under the mouse
					return FSelectedKey( Section, KeyArea, KeyHandle );
				}
				
			}

			// no key was selected in the current key area but the mouse is in the key area so it cannot possibly be in any other key area
			return FSelectedKey();
		}
	}

	// No key was selected in any key area
	return FSelectedKey();
}

void SSection::CheckForEdgeInteraction( const FPointerEvent& MouseEvent, const FGeometry& SectionGeometry )
{
	bLeftEdgeHovered = false;
	bRightEdgeHovered = false;
	bLeftEdgePressed = false;
	bRightEdgePressed = false;

	if (!SectionInterface->SectionIsResizable())
	{
		return;
	}

	// Make areas to the left and right of the geometry.  We will use these areas to determine if someone dragged the left or right edge of a section
	FGeometry SectionRectLeft = SectionGeometry.MakeChild(
		FVector2D::ZeroVector,
		FVector2D( SectionInterface->GetSectionGripSize(), SectionGeometry.Size.Y )
		);

	FGeometry SectionRectRight = SectionGeometry.MakeChild(
		FVector2D( SectionGeometry.Size.X - SectionInterface->GetSectionGripSize(), 0 ), 
		SectionGeometry.Size 
		);

	if( SectionRectLeft.IsUnderLocation( MouseEvent.GetScreenSpacePosition() ) )
	{
		if( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
		{
			bLeftEdgePressed = true;
		}
		else
		{
			bLeftEdgeHovered = true;
		}

		GetSequencer().GetEditTool().SetHotspot(MakeShareable( new FSectionResizeHotspot(FSectionResizeHotspot::Left, FSectionHandle(ParentSectionArea, SectionIndex))) );
	}
	else if( SectionRectRight.IsUnderLocation( MouseEvent.GetScreenSpacePosition() ) )
	{
		if( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
		{
			bRightEdgePressed = true;
		}
		else
		{
			bRightEdgeHovered = true;
		}

		GetSequencer().GetEditTool().SetHotspot(MakeShareable( new FSectionResizeHotspot(FSectionResizeHotspot::Right, FSectionHandle(ParentSectionArea, SectionIndex))) );
	}
}

FSequencer& SSection::GetSequencer() const
{
	return ParentSectionArea->GetSequencer();
}

int32 SSection::OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	UMovieSceneSection& SectionObject = *SectionInterface->GetSectionObject();

	bool bEnabled = bParentEnabled && SectionObject.IsActive();

	const ESlateDrawEffect::Type DrawEffects = bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

	int32 StartLayer = SCompoundWidget::OnPaint( Args, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bEnabled );

	FGeometry SectionGeometry = MakeSectionGeometryWithoutHandles( AllottedGeometry, SectionInterface );

	FSlateRect SectionClipRect = SectionGeometry.GetClippingRect().IntersectionWith( MyClippingRect );

	// Ask the interface to draw the section
	int32 PostSectionLayer = SectionInterface->OnPaintSection( SectionGeometry, SectionClipRect, OutDrawElements, LayerId, bEnabled );

	const bool bDisplaySectionHandles = true;

	DrawSectionHandlesAndSelection(AllottedGeometry, MyClippingRect, OutDrawElements, PostSectionLayer, bDisplaySectionHandles, bEnabled );

	PaintKeys( SectionGeometry, MyClippingRect, OutDrawElements, PostSectionLayer, InWidgetStyle, bEnabled );

	// Section name with drop shadow
	FText SectionTitle = SectionInterface->GetSectionTitle();
	if (!SectionTitle.IsEmpty())
	{
		FSlateDrawElement::MakeText(
			OutDrawElements,
			PostSectionLayer+1,
			SectionGeometry.ToOffsetPaintGeometry(FVector2D(6, 6)),
			SectionTitle,
			FEditorStyle::GetFontStyle("NormalFont"),
			MyClippingRect,
			DrawEffects,
			FLinearColor::Black
			);

		FSlateDrawElement::MakeText(
			OutDrawElements,
			PostSectionLayer+2,
			SectionGeometry.ToOffsetPaintGeometry(FVector2D(5, 5)),
			SectionTitle,
			FEditorStyle::GetFontStyle("NormalFont"),
			MyClippingRect,
			DrawEffects,
			FLinearColor::White
			);
	}

	return LayerId;
}

void SSection::PaintKeys( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	const ESlateDrawEffect::Type DrawEffects = bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

	UMovieSceneSection& SectionObject = *SectionInterface->GetSectionObject();

	FSequencer& Sequencer = ParentSectionArea->GetSequencer();

	static const FName BackgroundBrushName("Sequencer.SectionArea.Background");
	static const FName KeyBrushName("Sequencer.Key");

	const FSlateBrush* BackgroundBrush = FEditorStyle::GetBrush(BackgroundBrushName);

	const FSlateBrush* KeyBrush = FEditorStyle::GetBrush(KeyBrushName);

	static const FName SelectionColorName("SelectionColor");
	static const FName SelectionInactiveColorName("SelectionColorInactive");
	static const FName SelectionColorPressedName("SelectionColor_Pressed");

	const FLinearColor PressedKeyColor = FEditorStyle::GetSlateColor(SelectionColorPressedName).GetColor( InWidgetStyle );
	const FLinearColor SelectedKeyColor = FEditorStyle::GetSlateColor(SelectionColorName).GetColor( InWidgetStyle );
	const FLinearColor SelectedInactiveColor = FEditorStyle::GetSlateColor(SelectionInactiveColorName).GetColor( InWidgetStyle )
		* FLinearColor(.25, .25, .25, 1);  // Make the color a little darker since it's not very visible next to white keyframes.

	// @todo Sequencer temp color, make hovered brighter than selected.
	FLinearColor HoveredKeyColor = SelectedKeyColor * FLinearColor(1.5,1.5,1.5,1.0f);

	// Draw all keys in each key area
	for (const FKeyAreaLayoutElement& Element : Layout->GetElements())
	{
		// Get the key area at the same index of the section.  Each section in this widget has the same layout and the same number of key areas
		TSharedPtr<IKeyArea> KeyArea = Element.GetKeyArea();

		FGeometry KeyAreaGeometry = GetKeyAreaGeometry( Element, AllottedGeometry );

		FTimeToPixel TimeToPixelConverter = SectionObject.IsInfinite() ? 			
			FTimeToPixel( ParentGeometry, GetSequencer().GetViewRange()) : 
			FTimeToPixel( KeyAreaGeometry, TRange<float>( SectionObject.GetStartTime(), SectionObject.GetEndTime() ) );

		// Draw a box for the key area 
		// @todo Sequencer - Allow the IKeyArea to do this
		FSlateDrawElement::MakeBox( 
			OutDrawElements,
			LayerId,
			KeyAreaGeometry.ToPaintGeometry(),
			BackgroundBrush,
			MyClippingRect,
			DrawEffects,
			FLinearColor( .1f, .1f, .1f, 0.7f ) ); 


		int32 KeyLayer = LayerId + 1;

		TArray<FKeyHandle> KeyHandles = KeyArea->GetUnsortedKeyHandles();
		for( int32 KeyIndex = 0; KeyIndex < KeyHandles.Num(); ++KeyIndex )
		{
			FKeyHandle KeyHandle = KeyHandles[KeyIndex];
			float KeyTime = KeyArea->GetKeyTime(KeyHandle);

			// Omit keys which would not be visible
			if( SectionObject.IsTimeWithinSection( KeyTime ) )
			{
				const FSlateBrush* BrushToUse = KeyBrush;
				FLinearColor KeyColor( 1.0f, 1.0f, 1.0f, 1.0f );
				FLinearColor KeyTint(1.f, 1.f, 1.f, 1.f);

				if (Element.GetType() == FKeyAreaLayoutElement::Group)
				{
					auto Group = StaticCastSharedPtr<FGroupedKeyArea>(KeyArea);
					KeyTint = Group->GetKeyTint(KeyHandle);
					BrushToUse = Group->GetBrush(KeyHandle);
				}

				// Where to start drawing the key (relative to the section)
				float KeyPosition =  TimeToPixelConverter.TimeToPixel( KeyTime );

				FSelectedKey TestKey( SectionObject, KeyArea, KeyHandle );

				bool bSelected = Sequencer.GetSelection().IsSelected( TestKey );
				bool bActive = Sequencer.GetSelection().GetActiveSelection() == FSequencerSelection::EActiveSelection::KeyAndSection;

				if( TestKey == PressedKey )
				{
					KeyColor = PressedKeyColor;
				}
				else if( TestKey == HoveredKey )
				{
					KeyColor = HoveredKeyColor;
				}
				else if( bSelected )
				{
					if (bActive)
					{
						KeyColor = SelectedKeyColor;
					}
					else
					{
						KeyColor = SelectedInactiveColor;
					}
				}

				KeyColor *= KeyTint;

				// Draw the key
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					// always draw selected keys on top of other keys
					bSelected ? KeyLayer+1 : KeyLayer,
					// Center the key along Y.  Ensure the middle of the key is at the actual key time
					KeyAreaGeometry.ToPaintGeometry( FVector2D( KeyPosition - FMath::CeilToFloat(SequencerSectionConstants::KeySize.X/2.0f), ((KeyAreaGeometry.Size.Y*.5f)-(SequencerSectionConstants::KeySize.Y*.5f)) ), SequencerSectionConstants::KeySize ),
					BrushToUse,
					MyClippingRect,
					DrawEffects,
					KeyColor
					);
			}
		}
	}
}

void SSection::DrawSectionHandlesAndSelection( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, bool bDisplaySectionHandles, bool bParentEnabled ) const
{
	const ESlateDrawEffect::Type DrawEffects = bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

	UMovieSceneSection* SectionObject = SectionInterface->GetSectionObject();

	FSequencerSelection& Selection = ParentSectionArea->GetSequencer().GetSelection();
	const bool bSelected = Selection.IsSelected(SectionObject);
	const bool bActive = Selection.GetActiveSelection() == FSequencerSelection::EActiveSelection::KeyAndSection;

	FLinearColor SelectionColor = FEditorStyle::GetSlateColor(SequencerSectionConstants::SelectionColorName).GetColor(FWidgetStyle());
	FLinearColor SelectionInactiveColor = FEditorStyle::GetSlateColor(SequencerSectionConstants::SelectionInactiveColorName).GetColor(FWidgetStyle());
	FLinearColor TransparentSelectionColor = SelectionColor;

	if( bDisplaySectionHandles )
	{
		const FSlateBrush* LeftGripBrush = FEditorStyle::GetBrush(SectionInterface->GetSectionGripLeftBrushName());
		const FSlateBrush* RightGripBrush = FEditorStyle::GetBrush(SectionInterface->GetSectionGripRightBrushName());

		// Left Grip
		FSlateDrawElement::MakeBox
		(
			OutDrawElements,
			LayerId,
			// Center the key along Y.  Ensure the middle of the key is at the actual key time
			AllottedGeometry.ToPaintGeometry(FVector2D(0.0f, 0.0f), FVector2D(SectionInterface->GetSectionGripSize(), AllottedGeometry.GetDrawSize().Y)),
			LeftGripBrush,
			MyClippingRect,
			DrawEffects,
			(bLeftEdgePressed || bLeftEdgeHovered) ? TransparentSelectionColor : LeftGripBrush->GetTint(FWidgetStyle())
		);
		
		// Right Grip
		FSlateDrawElement::MakeBox
		(
			OutDrawElements,
			LayerId,
			// Center the key along Y.  Ensure the middle of the key is at the actual key time
			AllottedGeometry.ToPaintGeometry(FVector2D(AllottedGeometry.Size.X-SectionInterface->GetSectionGripSize(), 0.0f), FVector2D(SectionInterface->GetSectionGripSize(), AllottedGeometry.GetDrawSize().Y)),
			RightGripBrush,
			MyClippingRect,
			DrawEffects,
			(bRightEdgePressed || bRightEdgeHovered) ? TransparentSelectionColor : RightGripBrush->GetTint(FWidgetStyle())
		);
	}


	// draw selection box
	if(bSelected)
	{
		static const FName SelectionBorder("Sequencer.Section.SelectionBorder");

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId+1,
			AllottedGeometry.ToPaintGeometry(),
			FEditorStyle::GetBrush(SelectionBorder),
			MyClippingRect,
			DrawEffects,
			bActive ? SelectionColor : SelectionInactiveColor
			);
	}

}

TSharedPtr<SWidget> SSection::OnSummonContextMenu( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	FSelectedKey Key = GetKeyUnderMouse( MouseEvent.GetScreenSpacePosition(), MyGeometry );

	FSequencer& Sequencer = GetSequencer();

	UMovieSceneSection* SceneSection = SectionInterface->GetSectionObject();

	// @todo sequencer replace with UI Commands instead of faking it
	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, NULL);

	if (Key.IsValid() || Sequencer.GetSelection().GetSelectedKeys().Num())
	{
		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("Sequencer", "DeleteKey", "Delete"),
			NSLOCTEXT("Sequencer", "DeleteKeyToolTip", "Deletes the selected keys."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(&Sequencer, &FSequencer::DeleteSelectedKeys))
			);
	}
	else
	{
		SectionInterface->BuildSectionContextMenu(MenuBuilder);

		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("Sequencer", "ToggleSectionActive", "Active"),
			NSLOCTEXT("Sequencer", "ToggleSectionActiveTooltip", "Toggle section active/inactive."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(&Sequencer, &FSequencer::ToggleSectionActive),
				FCanExecuteAction::CreateSP(&Sequencer, &FSequencer::CanToggleSectionActive),
				FIsActionChecked::CreateSP(&Sequencer, &FSequencer::IsToggleSectionActive)),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
			);

		// @todo Sequencer this should delete all selected sections
		// delete/selection needs to be rethought in general
		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("Sequencer", "DeleteSection", "Delete"),
			NSLOCTEXT("Sequencer", "DeleteSectionToolTip", "Deletes this section."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(&Sequencer, &FSequencer::DeleteSection, SceneSection))
			);
	}
	
	return MenuBuilder.MakeWidget();
}


void SSection::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{	
	if( GetVisibility() == EVisibility::Visible )
	{
		if (LayoutRegenerationLock.GetValue() == 0)
		{
			Layout = FKeyAreaLayout(*ParentSectionArea, SectionIndex);
		}
		FGeometry SectionGeometry = MakeSectionGeometryWithoutHandles( AllottedGeometry, SectionInterface );

		SectionInterface->Tick(SectionGeometry, ParentGeometry, InCurrentTime, InDeltaTime);
	}
}


FReply SSection::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	FSequencer& Sequencer = GetSequencer();

	if( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton || MouseEvent.GetEffectingButton() == EKeys::RightMouseButton )
	{
		// Check for clicking on a key and mark it as the pressed key for drag detection (if necessary) later
		PressedKey = GetKeyUnderMouse( MouseEvent.GetScreenSpacePosition(), MyGeometry );
	}

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if( !PressedKey.IsValid() )
		{
			CheckForEdgeInteraction( MouseEvent, MyGeometry );
		}
	}
	else if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && (PressedKey.IsValid() || Sequencer.GetSelection().GetSelectedKeys().Num() || Sequencer.GetSelection().GetSelectedSections().Num()))
	{
		// Don't return handled if we didn't click on a key so that right-click panning gets a look in
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void SSection::ResetState()
{
	ResetHoveredState();
	PressedKey = FSelectedKey();
}

void SSection::ResetHoveredState()
{
	bLeftEdgeHovered = false;
	bRightEdgeHovered = false;
	bLeftEdgePressed = false;
	bRightEdgePressed = false;
	HoveredKey = FSelectedKey();

	GetSequencer().GetEditTool().SetHotspot(nullptr);
}

FGeometry SSection::MakeSectionGeometryWithoutHandles( const FGeometry& AllottedGeometry, const TSharedPtr<ISequencerSection>& InSectionInterface ) const
{
	const bool bSectionsAreConnected = InSectionInterface->AreSectionsConnected();

	const float SectionGripSize = !bSectionsAreConnected ? InSectionInterface->GetSectionGripSize() : 0.0f;

	return AllottedGeometry.MakeChild( FVector2D( SectionGripSize, 0 ), AllottedGeometry.GetDrawSize() - FVector2D( SectionGripSize*2, 0.0f ) );
}

FReply SSection::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if( ( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton || MouseEvent.GetEffectingButton() == EKeys::RightMouseButton ) && MyGeometry.IsUnderLocation( MouseEvent.GetScreenSpacePosition() ) )
	{
		// Snap time to the key under the mouse if shift is down
		if (MouseEvent.IsShiftDown() && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			FSelectedKey Key = GetKeyUnderMouse( MouseEvent.GetScreenSpacePosition(), MyGeometry );
			if (Key.IsValid())
			{
				float KeyTime = Key.KeyArea->GetKeyTime(Key.KeyHandle.GetValue());
				GetSequencer().SetGlobalTime(KeyTime);
			}
		}

		HandleSelection( MyGeometry, MouseEvent );
	}

	if( MouseEvent.GetEffectingButton() == EKeys::RightMouseButton )
	{
		TSharedPtr<SWidget> MenuContent = OnSummonContextMenu( MyGeometry, MouseEvent );
		if (MenuContent.IsValid())
		{
			FWidgetPath WidgetPath = MouseEvent.GetEventPath() != nullptr ? *MouseEvent.GetEventPath() : FWidgetPath();

			FSlateApplication::Get().PushMenu(
				AsShared(),
				WidgetPath,
				MenuContent.ToSharedRef(),
				MouseEvent.GetScreenSpacePosition(),
				FPopupTransitionEffect( FPopupTransitionEffect::ContextMenu )
				);

			return FReply::Handled().SetUserFocus(MenuContent.ToSharedRef(), EFocusCause::SetDirectly);
		}
	}

	ResetState();

	return FReply::Handled();
}

FReply SSection::OnMouseButtonDoubleClick( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	ResetState();

	UMovieSceneSection* SectionObject = SectionInterface->GetSectionObject();
	if( GetSequencer().IsSectionVisible( SectionObject ) )
	{
		FReply Reply = SectionInterface->OnSectionDoubleClicked( MyGeometry, MouseEvent );

		if (Reply.IsEventHandled()) {return Reply;}

		if( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
		{
			GetSequencer().ZoomToSelectedSections();

			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}


FReply SSection::OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	ResetHoveredState();

	// Checked for hovered key
	// @todo Sequencer - Needs visual cue
	HoveredKey = GetKeyUnderMouse( MouseEvent.GetScreenSpacePosition(), MyGeometry );

	auto& EditTool = GetSequencer().GetEditTool();

	EditTool.SetHotspot( MakeShareable( new FSectionHotspot(FSectionHandle(ParentSectionArea, SectionIndex))) );

	if ( HoveredKey.IsValid() )
	{
		EditTool.SetHotspot( MakeShareable( new FKeyHotspot(HoveredKey, ParentSectionArea) ) );
	}
	else
	{
		// Only check for edge interaction if not hovering over a key
		CheckForEdgeInteraction( MouseEvent, MyGeometry );
	}

	return FReply::Unhandled();
}

void SSection::OnMouseLeave( const FPointerEvent& MouseEvent )
{
	SCompoundWidget::OnMouseLeave( MouseEvent );
	ResetHoveredState();
}

FCursorReply SSection::OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent ) const
{
	if( bLeftEdgeHovered || bRightEdgeHovered )
	{
		return FCursorReply::Cursor( EMouseCursor::ResizeLeftRight );
	}

	return FCursorReply::Cursor( EMouseCursor::Default );
}


void SSection::HandleSelection( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	FSelectedKey Key = GetKeyUnderMouse( MouseEvent.GetScreenSpacePosition(), MyGeometry );

	// If we have a currently pressed down and the key under the mouse is the pressed key, select that key now
	if( PressedKey.IsValid() && Key.IsValid() && PressedKey == Key )
	{
		bool bSelectDueToDrag = false;
		HandleKeySelection( Key, MouseEvent, bSelectDueToDrag );
	}
	else if( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton ) // Only select anything but keys on left mouse button
	{
		bool bSelectDueToDrag = false;
		HandleSectionSelection( MouseEvent, bSelectDueToDrag );
	}
}


void SSection::HandleKeySelection( const FSelectedKey& Key, const FPointerEvent& MouseEvent, bool bSelectDueToDrag )
{
	if( Key.IsValid() )
	{
		bool bKeyIsSelected = GetSequencer().GetSelection().IsSelected( Key );

		// Clear previous key selection if:
		// we are selecting due to drag and the key being dragged is not selected
		bool bShouldClearSelectionDueToDrag =  bSelectDueToDrag ? !bKeyIsSelected : true;
		
		// Keep key selection if right clicking to bring up a menu and the current key is selected
		bool bKeepKeySelection = MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && bKeyIsSelected;

		// Toggle selection if control is down and key is selected
		if (MouseEvent.IsControlDown() && bKeyIsSelected)
		{
			GetSequencer().GetSelection().RemoveFromSelection( Key );
		}
		else
		{
			if( (!MouseEvent.IsShiftDown() && !MouseEvent.IsControlDown() && bShouldClearSelectionDueToDrag) && !bKeepKeySelection )
			{
				GetSequencer().GetSelection().EmptySelectedKeys();
			}
			GetSequencer().GetSelection().AddToSelection( Key );
		}
	}
}

void SSection::HandleSectionSelection( const FPointerEvent& MouseEvent, bool bSelectDueToDrag )
{
	// handle selecting sections 
	UMovieSceneSection* Section = SectionInterface->GetSectionObject();

	bool bSectionIsSelected = GetSequencer().GetSelection().IsSelected(Section);

	// Clear previous section selection if:
	// we are selecting due to drag and the section being dragged is not selected
	bool bShouldClearSelectionDueToDrag =  bSelectDueToDrag ? !bSectionIsSelected : true;

	// Keep key selection if right clicking to bring up a menu and the current key is selected
	bool bKeepSectionSelection = MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && bSectionIsSelected;
	
	// Don't clear selection edge is being dragged
	bool bEdgePressed = bLeftEdgePressed || bRightEdgePressed;

	if (MouseEvent.IsControlDown() && bSectionIsSelected && !bEdgePressed)
	{
		GetSequencer().GetSelection().RemoveFromSelection(Section);
	}
	else
	{
		if ( (!MouseEvent.IsShiftDown() && !MouseEvent.IsControlDown() && bShouldClearSelectionDueToDrag) && !bKeepSectionSelection)
		{
			GetSequencer().GetSelection().EmptySelectedSections();
		}
	}
	
	if( GetSequencer().GetSelection().IsSelected(Section) )
	{
		// Control should toggle the selection
		GetSequencer().GetSelection().RemoveFromSelection(Section);
	}
	else
	{
		GetSequencer().GetSelection().AddToSelection(Section);
	}
}

