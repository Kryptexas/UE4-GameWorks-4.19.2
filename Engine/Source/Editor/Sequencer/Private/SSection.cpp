// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "SSection.h"
#include "IKeyArea.h"
#include "ISequencerSection.h"
#include "MovieSceneSection.h"
#include "SectionDragOperations.h"
#include "MovieSceneShotSection.h"
#include "CommonMovieSceneTools.h"

namespace SequencerSectionConstants
{
	/** How far the user has to drag the mouse before we consider the action dragging rather than a click */
	const float SectionDragStartDistance = 5.0f;

	/** The size of each key */
	const FVector2D KeySize( 11.0f, 11.0f );

	const float SectionEdgeSize = 15.0f;
}

void SSection::Construct( const FArguments& InArgs, TSharedRef<FTrackNode> SectionNode, int32 InSectionIndex )
{
	ResetState();

	SectionIndex = InSectionIndex;
	ParentSectionArea = SectionNode;
	SectionInterface = SectionNode->GetSections()[InSectionIndex];
}

void SSection::GetKeyAreas( const TSharedPtr<FTrackNode>& SectionAreaNode, TArray<FKeyAreaElement>& OutKeyAreas ) const
{
	float HeightOffset = 0;
	TSharedPtr<FSectionKeyAreaNode> SectionKeyNode = SectionAreaNode->GetTopLevelKeyNode();
	if( SectionKeyNode.IsValid() )
	{
		new( OutKeyAreas ) FKeyAreaElement( *SectionKeyNode, HeightOffset );
	}

	GetKeyAreas_Recursive( SectionAreaNode->GetChildNodes(), HeightOffset, OutKeyAreas );
}

void SSection::GetKeyAreas_Recursive(  const TArray< TSharedRef<FSequencerDisplayNode> >& Nodes, float& HeightOffset, TArray<FKeyAreaElement>& OutKeyAreas ) const
{
	const float Padding = SequencerLayoutConstants::NodePadding;

	for( int32 NodeIndex = 0; NodeIndex < Nodes.Num(); ++NodeIndex )
	{
		FSequencerDisplayNode& Node = *Nodes[NodeIndex];

		if( Node.IsVisible() )
		{
			// Compute the node height.  If this is not a key area it will contribute to the accumulated height offset
			float NodeHeight = Node.GetNodeHeight();
			HeightOffset += Padding + NodeHeight;
			if( Node.GetType() == ESequencerNode::KeyArea )
			{
				// The node is a key area, we need to draw it
				new( OutKeyAreas ) FKeyAreaElement( *StaticCastSharedRef<FSectionKeyAreaNode>( Nodes[NodeIndex] ), HeightOffset );
			}

			// Get any child key areas
			GetKeyAreas_Recursive( Node.GetChildNodes(), HeightOffset, OutKeyAreas );

		}
	}
}


FGeometry SSection::GetKeyAreaGeometry( const struct FKeyAreaElement& KeyArea, const FGeometry& SectionGeometry ) const
{
	// Get the height of the key area node.  If the key area is top level then it is part of the section (and the same height ) and doesn't take up extra space
	float KeyAreaHeight = KeyArea.KeyAreaNode.IsTopLevel() ? SectionGeometry.GetDrawSize().Y : KeyArea.KeyAreaNode.GetNodeHeight();

	// Compute the geometry for the key area
	return SectionGeometry.MakeChild( FVector2D( 0, KeyArea.HeightOffset ), FVector2D( SectionGeometry.Size.X, KeyAreaHeight ) );
}


FSelectedKey SSection::GetKeyUnderMouse( const FVector2D& MousePosition, const FGeometry& SectionGeometry ) const
{
	UMovieSceneSection& Section = *SectionInterface->GetSectionObject();

	// Search every key area until we find the one under the mouse
	for( int32 KeyAreaIndex = 0; KeyAreaIndex < KeyAreas.Num(); ++KeyAreaIndex )
	{
		const FKeyAreaElement& Element = KeyAreas[KeyAreaIndex];
		TSharedRef<IKeyArea> KeyArea = Element.KeyAreaNode.GetKeyArea( SectionIndex ); 

		// Compute the current key area geometry
		FGeometry KeyAreaGeometry = GetKeyAreaGeometry( Element, SectionGeometry );

		// Is the key area under the mouse
		if( KeyAreaGeometry.IsUnderLocation( MousePosition ) )
		{
			FVector2D LocalSpaceMousePosition = KeyAreaGeometry.AbsoluteToLocal( MousePosition );

			FTimeToPixel TimeToPixelConverter( KeyAreaGeometry, TRange<float>( Section.GetStartTime(), Section.GetEndTime() ) );

			// Check each key until we find one under the mouse (if any)
			TArray<FKeyHandle> KeyHandles = KeyArea->GetUnsortedKeyHandles();
			for( int32 KeyIndex = 0; KeyIndex < KeyHandles.Num(); ++KeyIndex )
			{
				FKeyHandle KeyHandle = KeyHandles[KeyIndex];
				float KeyPosition = TimeToPixelConverter.TimeToPixel( KeyArea->GetKeyTime(KeyHandle) );

				FGeometry KeyGeometry = KeyAreaGeometry.MakeChild( 
					FVector2D( KeyPosition - FMath::Trunc(SequencerSectionConstants::KeySize.X/2.0f), ((KeyAreaGeometry.Size.Y*.5f)-(SequencerSectionConstants::KeySize.Y*.5f)) ),
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
		FVector2D( SequencerSectionConstants::SectionEdgeSize, SectionGeometry.Size.Y )
		);

	FGeometry SectionRectRight = SectionGeometry.MakeChild(
		FVector2D( SectionGeometry.Size.X - SequencerSectionConstants::SectionEdgeSize, 0 ), 
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
	}
}

ISequencerInternals& SSection::GetSequencer() const
{
	return ParentSectionArea->GetSequencer();
}

void SSection::CreateDragOperation( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bKeysUnderMouse )
{	
	check( !DragOperation.IsValid() );

	if( bKeysUnderMouse )
	{
		DragOperation = MakeShareable( new FMoveKeys( GetSequencer().GetSelectedKeys(), PressedKey ) );
	}
	else
	{
		UMovieSceneSection* SectionObject = SectionInterface->GetSectionObject();

		if( bLeftEdgePressed || bLeftEdgeHovered )
		{
			// Selected the start of a section
			DragOperation = MakeShareable( new FResizeSection( *SectionObject, false ) );
		}
		else if( bRightEdgePressed || bRightEdgeHovered )
		{
			// Selected the end of a section
			DragOperation = MakeShareable( new FResizeSection( *SectionObject, true ) );
		}
		else
		{
			// Entire selection moved
			DragOperation = MakeShareable( new FMoveSection( *SectionObject ) );
		}
	}
	
}

int32 SSection::OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	int32 StartLayer = SCompoundWidget::OnPaint( AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled );

	UMovieSceneSection* SectionObject = SectionInterface->GetSectionObject();

	const bool bSelected = ParentSectionArea->GetSequencer().IsSectionSelected( SectionObject );

	// Ask the interface to draw the section
	int32 PostSectionLayer = SectionInterface->OnPaintSection( AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, bParentEnabled );

	// @todo Sequencer - Temp indicators of the area of a section which can be dragged.

	PaintKeys( AllottedGeometry, MyClippingRect, OutDrawElements, PostSectionLayer, InWidgetStyle );

	FLinearColor SelectionColor = FEditorStyle::GetSlateColor("SelectionColor").GetColor( InWidgetStyle );
	FLinearColor TransparentSelectionColor = SelectionColor;
	TransparentSelectionColor.A = .5f;
		
	// Section name with drop shadow
	FString SectionTitle = SectionInterface->GetSectionTitle();
	if (!SectionTitle.IsEmpty())
	{
		FSlateDrawElement::MakeText(
			OutDrawElements,
			PostSectionLayer+1,
			AllottedGeometry.ToOffsetPaintGeometry(FVector2D(6, 6)),
			SectionTitle,
			FEditorStyle::GetFontStyle("NormalFont"),
			MyClippingRect,
			ESlateDrawEffect::None,
			FLinearColor::Black
			);
		FSlateDrawElement::MakeText(
			OutDrawElements,
			PostSectionLayer+2,
			AllottedGeometry.ToOffsetPaintGeometry(FVector2D(5, 5)),
			SectionTitle,
			FEditorStyle::GetFontStyle("NormalFont"),
			MyClippingRect,
			ESlateDrawEffect::None,
			FLinearColor::White
			);
	}

	// draw selection box
	if (bSelected)
	{
		FSlateDrawElement::MakeBox( 
			OutDrawElements,
			PostSectionLayer+3,
			AllottedGeometry.ToPaintGeometry(),
			FEditorStyle::GetBrush("PlainBorder"),
			MyClippingRect,
			ESlateDrawEffect::None,
			SelectionColor
			);
	}
	

	if( bLeftEdgePressed || bLeftEdgeHovered )
	{
		FSlateDrawElement::MakeBox( 
			OutDrawElements, 
			PostSectionLayer+3,
			AllottedGeometry.ToPaintGeometry( FVector2D::ZeroVector, FVector2D( 5.0f, AllottedGeometry.Size.Y ) ),
			FEditorStyle::GetBrush( "WhiteBrush" ),
			MyClippingRect,
			ESlateDrawEffect::None,
			TransparentSelectionColor
			);

	}

	if( bRightEdgePressed || bRightEdgeHovered )
	{
		FSlateDrawElement::MakeBox( 
			OutDrawElements, 
			PostSectionLayer+3,
			AllottedGeometry.ToPaintGeometry( FVector2D( AllottedGeometry.Size.X - 5.0f, 0 ), AllottedGeometry.Size ),
			FEditorStyle::GetBrush( "WhiteBrush" ),
			MyClippingRect,
			ESlateDrawEffect::None,
			TransparentSelectionColor
			);

	}

	return LayerId;
}

void SSection::PaintKeys( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle ) const
{
	UMovieSceneSection& SectionObject = *SectionInterface->GetSectionObject();

	const ISequencerInternals& Sequencer = ParentSectionArea->GetSequencer();

	const FSlateBrush* BackgroundBrush = FEditorStyle::GetBrush("Sequencer.SectionArea.Background");

	const FSlateBrush* KeyBrush = FEditorStyle::GetBrush("Sequencer.Key");

	const FLinearColor PressedKeyColor = FEditorStyle::GetSlateColor("SelectionColor_Pressed").GetColor( InWidgetStyle );
	const FLinearColor SelectedKeyColor = FEditorStyle::GetSlateColor("SelectionColor").GetColor( InWidgetStyle );
	// @todo Sequencer temp color, make hovered brighter than selected.
	FLinearColor HoveredKeyColor = SelectedKeyColor * FLinearColor(1.5,1.5,1.5,1.0f);

	// Draw all keys in each key area
	for( int32 KeyAreaIndex = 0; KeyAreaIndex < KeyAreas.Num(); ++KeyAreaIndex )
	{
		const FKeyAreaElement& Element = KeyAreas[KeyAreaIndex];

		// Get the key area at the same index of the section.  Each section in this widget has the same layout and the same number of key areas
		const TSharedRef<IKeyArea>& KeyArea = Element.KeyAreaNode.GetKeyArea( SectionIndex );

		FGeometry KeyAreaGeometry = GetKeyAreaGeometry( Element, AllottedGeometry );

		FTimeToPixel TimeToPixelConverter( KeyAreaGeometry, TRange<float>( SectionObject.GetStartTime(), SectionObject.GetEndTime() ) );

		// Draw a box for the key area 
		// @todo Sequencer - Allow the IKeyArea to do this
		FSlateDrawElement::MakeBox( 
			OutDrawElements,
			LayerId,
			KeyAreaGeometry.ToPaintGeometry(),
			BackgroundBrush,
			MyClippingRect,
			ESlateDrawEffect::None,
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
				FLinearColor KeyColor( 1.0f, 1.0f, 1.0f, 1.0f );

				// Where to start drawing the key (relative to the section)
				float KeyPosition =  TimeToPixelConverter.TimeToPixel( KeyTime );

				FSelectedKey TestKey( SectionObject, KeyArea, KeyHandle );

				bool bSelected = Sequencer.IsKeySelected( TestKey );

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
					KeyColor = SelectedKeyColor;
				}

				// Draw the key
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					// always draw selected keys on top of other keys
					bSelected ? KeyLayer+1 : KeyLayer,
					// Center the key along Y.  Ensure the middle of the key is at the actual key time
					KeyAreaGeometry.ToPaintGeometry( FVector2D( KeyPosition - FMath::Trunc(SequencerSectionConstants::KeySize.X/2.0f), ((KeyAreaGeometry.Size.Y*.5f)-(SequencerSectionConstants::KeySize.Y*.5f)) ), SequencerSectionConstants::KeySize ),
					KeyBrush,
					MyClippingRect,
					ESlateDrawEffect::None,
					KeyColor
					);
			}
		}
	}
}

TSharedPtr<SWidget> SSection::OnSummonContextMenu( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	FSelectedKey Key = GetKeyUnderMouse( MouseEvent.GetScreenSpacePosition(), MyGeometry );

	ISequencerInternals& Sequencer = GetSequencer();

	UMovieSceneSection* SceneSection = SectionInterface->GetSectionObject();

	// @todo sequencer replace with UI Commands instead of faking it
	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, NULL);

	if (Key.IsValid())
	{
		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("Sequencer", "DeleteKey", "Delete"),
			NSLOCTEXT("Sequencer", "DeleteKeyToolTip", "Deletes the selected keys."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(&Sequencer, &ISequencerInternals::DeleteKeys, Sequencer.GetSelectedKeys().Array()))
			);
	}
	else
	{
		bool bIsAShotSection = SceneSection->IsA<UMovieSceneShotSection>();

		if (bIsAShotSection)
		{
			MenuBuilder.AddMenuEntry(
				NSLOCTEXT("Sequencer", "RenameShot", "Rename"),
				NSLOCTEXT("Sequencer", "RenameShotToolTip", "Renames this shot."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(&Sequencer, &ISequencerInternals::RenameShot, SceneSection))
				);

			MenuBuilder.AddMenuEntry(
				NSLOCTEXT("Sequencer", "FilterToShots", "Filter To Shots"),
				NSLOCTEXT("Sequencer", "FilterToShotsToolTip", "Filters to the selected shot sections"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(&Sequencer, &ISequencerInternals::FilterToSelectedShotSections, true))
				);
		}

		// @todo Sequencer this should delete all selected sections
		// delete/selection needs to be rethought in general
		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("Sequencer", "DeleteSection", "Delete"),
			NSLOCTEXT("Sequencer", "DeleteSectionToolTip", "Deletes this section."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(&Sequencer, &ISequencerInternals::DeleteSection, SceneSection))
			);
	}
	
	return MenuBuilder.MakeWidget();
}


void SSection::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{	
	if( GetVisibility() == EVisibility::Visible )
	{
		KeyAreas.Reset();
		GetKeyAreas( ParentSectionArea, KeyAreas );

		SectionInterface->Tick(AllottedGeometry, ParentGeometry, InCurrentTime, InDeltaTime);
	}
}


FReply SSection::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	DistanceDragged = 0;

	DragOperation.Reset();

	bDragging = false;

	if( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
	{
		// Check for clicking on a key and mark it as the pressed key for drag detection (if necessary) later
		PressedKey = GetKeyUnderMouse( MouseEvent.GetScreenSpacePosition(), MyGeometry );

		if( !PressedKey.IsValid() )
		{
			CheckForEdgeInteraction( MouseEvent, MyGeometry );
		}

		return FReply::Handled().CaptureMouse( AsShared() );
	}
	else if( MouseEvent.GetEffectingButton() == EKeys::RightMouseButton )
	{
		return FReply::Handled().CaptureMouse(AsShared());
	}

	return FReply::Handled();
}

void SSection::ResetState()
{
	DistanceDragged = 0;
	bDragging = false;
	DragOperation.Reset();
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
}

FReply SSection::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if( bDragging && DragOperation.IsValid() )
	{
		// If dragging tell the operation we are no longer dragging
		DragOperation->OnEndDrag(ParentSectionArea);
	}
	else
	{
		if( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && HasMouseCapture() && MyGeometry.IsUnderLocation( MouseEvent.GetScreenSpacePosition() ) )
		{
			HandleSelection( MyGeometry, MouseEvent );
		}

		if( MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && HasMouseCapture() )
		{
			TSharedPtr<SWidget> MenuContent = OnSummonContextMenu( MyGeometry, MouseEvent );
			if (MenuContent.IsValid())
			{
				FSlateApplication::Get().PushMenu(
					AsShared(),
					MenuContent.ToSharedRef(),
					MouseEvent.GetScreenSpacePosition(),
					FPopupTransitionEffect( FPopupTransitionEffect::ContextMenu )
					);

				return FReply::Handled().ReleaseMouseCapture().SetKeyboardFocus( MenuContent.ToSharedRef(), EKeyboardFocusCause::SetDirectly );
			}
		}
	}

	ResetState();

	return FReply::Handled().ReleaseMouseCapture();
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
	if( HasMouseCapture() )
	{
		// Have mouse capture and there are drag operations that need to be performed
		if( MouseEvent.IsMouseButtonDown( EKeys::LeftMouseButton ) )
		{
			DistanceDragged += FMath::Abs( MouseEvent.GetCursorDelta().X );
			
			FVector2D LocalMousePos = MyGeometry.AbsoluteToLocal( MouseEvent.GetScreenSpacePosition() );

			if( !bDragging )
			{
				// If we are not dragging determine if the mouse has moved far enough to start a drag
				if( DistanceDragged >= SequencerSectionConstants::SectionDragStartDistance )
				{
					bDragging = true;

					if( PressedKey.IsValid() )
					{
						// Clear selected sections when beginning to drag keys
						GetSequencer().ClearSectionSelection();

						bool bSelectDueToDrag = true;
						HandleKeySelection( PressedKey, MouseEvent, bSelectDueToDrag );

						bool bKeysUnderMouse = true;
						CreateDragOperation( MyGeometry, MouseEvent, bKeysUnderMouse );
					}
					else
					{
						// Clear selected keys when beginning to drag a section
						GetSequencer().ClearKeySelection();

						HandleSectionSelection( MouseEvent );

						bool bKeysUnderMouse = false;
						CreateDragOperation( MyGeometry, MouseEvent, bKeysUnderMouse );
					}
				
					if( DragOperation.IsValid() )
					{
						DragOperation->OnBeginDrag(LocalMousePos, ParentSectionArea);
					}

				}
			}
			else if( DragOperation.IsValid() )
			{
				// Already in a drag, tell all operations to perform their drag implementations
				FTimeToPixel TimeToPixelConverter( MyGeometry, TRange<float>( SectionInterface->GetSectionObject()->GetStartTime(), SectionInterface->GetSectionObject()->GetEndTime() ) );

				DragOperation->OnDrag( MouseEvent, LocalMousePos, TimeToPixelConverter, ParentSectionArea );
			}
		}

		return FReply::Handled();
	}
	else
	{
		// Not dragging


		ResetHoveredState();

		// Checked for hovered key
		// @todo Sequencer - Needs visual cue
		HoveredKey = GetKeyUnderMouse( MouseEvent.GetScreenSpacePosition(), MyGeometry );

		// Only check for edge interaction if not hovering over a key
		if( !HoveredKey.IsValid() )
		{
			CheckForEdgeInteraction( MouseEvent, MyGeometry );
		}
		
	}

	return FReply::Unhandled();
}

void SSection::OnMouseLeave( const FPointerEvent& MouseEvent )
{
	if( !HasMouseCapture() )
	{
		ResetHoveredState();
	}
}

FCursorReply SSection::OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent ) const
{
	if( DragOperation.IsValid() )
	{
		return DragOperation->GetCursor();
	}
	else if( bLeftEdgeHovered || bRightEdgeHovered )
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
	else
	{
		HandleSectionSelection( MouseEvent );
	}
}


void SSection::HandleKeySelection( const FSelectedKey& Key, const FPointerEvent& MouseEvent, bool bSelectDueToDrag )
{
	if( Key.IsValid() )
	{
		// Clear previous key selection if:
		// we are selecting due to drag and the key being dragged is not selected or control is not down
		bool bShouldClearSelectionDueToDrag =  bSelectDueToDrag ? !GetSequencer().IsKeySelected( Key ) : true;
		
		if( !MouseEvent.IsControlDown() && bShouldClearSelectionDueToDrag )
		{
			GetSequencer().ClearKeySelection();
		}
		GetSequencer().SelectKey( Key );
	}
}

void SSection::HandleSectionSelection( const FPointerEvent& MouseEvent )
{
	if( !MouseEvent.IsControlDown() )
	{
		GetSequencer().ClearSectionSelection();
	}

	// handle selecting sections 
	UMovieSceneSection* Section = SectionInterface->GetSectionObject();
	GetSequencer().SelectSection(Section);
}

