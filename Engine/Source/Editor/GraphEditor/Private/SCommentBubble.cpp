// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "GraphEditorCommon.h"
#include "SCommentBubble.h"
#include "SInlineEditableTextBlock.h"

namespace SCommentBubbleDefs
{
	/** Mouse double click disable */
	static const float DoubleClickDisable = -1.f;

	/** Mouse double click delay */
	static const float DoubleClickDelay = 0.5f;

	/** Bubble Full opacity */
	static const float FullOpacity = 1.f;

	/** Bubble Toggle Icon Fade Down Speed */
	static const float FadeDownSpeed = 5.f;

	/** Height of the arrow connecting the bubble to the node */
	static const float BubbleArrowHeight = 8.f;
};


void SCommentBubble::Construct( const FArguments& InArgs )
{
	check( InArgs._Text.IsBound() );
	check( InArgs._GraphNode );

	GraphNode				= InArgs._GraphNode;
	CommentAttribute		= InArgs._Text;
	ColorAndOpacity			= InArgs._ColorAndOpacity;
	bAllowPinning			= InArgs._AllowPinning;
	bEnableTitleBarBubble	= InArgs._EnableTitleBarBubble;
	bEnableBubbleCtrls		= InArgs._EnableBubbleCtrls;
	GraphLOD				= InArgs._GraphLOD;
	IsGraphNodeHovered		= InArgs._IsGraphNodeHovered;

	DoubleClickDelayTime	= SCommentBubbleDefs::DoubleClickDisable;
	OpacityValue			= 0.f;

	// Create Widget
	UpdateBubble();
}

FCursorReply SCommentBubble::OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent ) const
{
	const FVector2D Size( GetDesiredSize().X, GetDesiredSize().Y - SCommentBubbleDefs::BubbleArrowHeight );
	const FSlateRect TestRect( MyGeometry.AbsolutePosition, MyGeometry.AbsolutePosition + Size );

	if( TestRect.ContainsPoint( CursorEvent.GetScreenSpacePosition() ))
	{
		return FCursorReply::Cursor( EMouseCursor::TextEditBeam );
	}
	return FCursorReply::Cursor( EMouseCursor::Default );
}

FReply SCommentBubble::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent )
{
	if( TextBlock.IsValid() )
	{
		const FVector2D Size( GetDesiredSize().X, GetDesiredSize().Y - SCommentBubbleDefs::BubbleArrowHeight );
		const FSlateRect TestRect( MyGeometry.AbsolutePosition, MyGeometry.AbsolutePosition + Size );

		if( TestRect.ContainsPoint( CursorEvent.GetScreenSpacePosition() ))
		{
			if( DoubleClickDelayTime >= SCommentBubbleDefs::DoubleClickDelay )
			{
				TextBlock->EnterEditingMode();
				DoubleClickDelayTime = SCommentBubbleDefs::DoubleClickDisable;
			}
			else
			{
				FSlateApplication::Get().SetKeyboardFocus( TextBlock, EKeyboardFocusCause::SetDirectly );
				DoubleClickDelayTime = 0.f;
			}
		}
	}
	return FReply::Handled();
}

FReply SCommentBubble::OnMouseButtonDoubleClick( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return FReply::Handled();
}

void SCommentBubble::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	SCompoundWidget::Tick( AllottedGeometry, InCurrentTime, InDeltaTime );

	bool bIsHovered = IsHovered();

	if( DoubleClickDelayTime >= 0.f )
	{
		DoubleClickDelayTime += InDeltaTime;
		
		if( !bIsHovered )
		{
			DoubleClickDelayTime = -1.f;
		}
	}
	if( bEnableTitleBarBubble && IsGraphNodeHovered.IsBound() )
	{
		bIsHovered |= IsGraphNodeHovered.Execute();

		if( !GraphNode->bCommentBubbleVisible && !bIsHovered )
		{
			if( OpacityValue > 0.f )
			{
				const float FadeDownAmt = InDeltaTime * SCommentBubbleDefs::FadeDownSpeed;
				OpacityValue = FMath::Max( OpacityValue - FadeDownAmt, 0.f );
			}
		}
		else
		{
			OpacityValue = SCommentBubbleDefs::FullOpacity;
		}
	}
}

void SCommentBubble::UpdateBubble()
{
	if( GraphNode->bCommentBubbleVisible )
	{
		const FSlateBrush* CommentCalloutArrowBrush = FEditorStyle::GetBrush(TEXT("Graph.Node.CommentArrow"));
		const FMargin BubblePadding = FEditorStyle::GetMargin( TEXT("Graph.Node.Comment.BubblePadding"));
		const FMargin PinIconPadding = FEditorStyle::GetMargin( TEXT("Graph.Node.Comment.PinIconPadding"));
		const FMargin BubbleOffset = FEditorStyle::GetMargin( TEXT("Graph.Node.Comment.BubbleOffset"));
		// Conditionally create bubble controls
		TSharedPtr<SWidget> BubbleControls = SNullWidget::NullWidget;

		if( bEnableBubbleCtrls )
		{
			if( bAllowPinning )
			{
				SAssignNew( BubbleControls, SVerticalBox )
				+SVerticalBox::Slot()
				.Padding( 1.f )
				.AutoHeight()
				.VAlign( VAlign_Top )
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding( PinIconPadding )
					[
						SNew( SCheckBox )
						.Style( &FEditorStyle::Get().GetWidgetStyle<FCheckBoxStyle>( "CommentBubblePin" ))
						.IsChecked( GraphNode->bCommentBubblePinned ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked )
						.OnCheckStateChanged( this, &SCommentBubble::OnPinStateToggle )
						.ToolTipText( this, &SCommentBubble::GetScaleButtonTooltip )
						.Cursor( EMouseCursor::Default )
					]
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding( 1.f )
				.VAlign( VAlign_Top )
				[
					SNew( SCheckBox )
					.Style( &FEditorStyle::Get().GetWidgetStyle< FCheckBoxStyle >( "CommentBubbleButton" ))
					.IsChecked( GraphNode->bCommentBubbleVisible ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked )
					.OnCheckStateChanged( this, &SCommentBubble::OnCommentBubbleToggle )
					.ToolTipText( NSLOCTEXT( "CommentBubble", "ToggleCommentTooltip", "Toggle Comment Bubble" ))
					.Cursor( EMouseCursor::Default )
					.ForegroundColor( this, &SCommentBubble::GetToggleButtonColour )
				];
			}
			else
			{
				SAssignNew( BubbleControls, SVerticalBox )
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding( 1.f )
				.VAlign( VAlign_Top )
				[
					SNew( SCheckBox )
					.Style( &FEditorStyle::Get().GetWidgetStyle< FCheckBoxStyle >( "CommentBubbleButton6" ))
					.IsChecked( GraphNode->bCommentBubbleVisible ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked )
					.OnCheckStateChanged( this, &SCommentBubble::OnCommentBubbleToggle )
					.ToolTipText( NSLOCTEXT( "CommentBubble", "ToggleCommentTooltip", "Toggle Comment Bubble" ))
					.Cursor( EMouseCursor::Default )
					.ForegroundColor( this, &SCommentBubble::GetToggleButtonColour )
				];
			}
		}
		// Create the comment bubble widget
		ChildSlot
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SOverlay)
					+SOverlay::Slot()
					[
						SNew(SImage)
						.Image( FEditorStyle::GetBrush( TEXT("Graph.Node.CommentBubble")) )
						.ColorAndOpacity( ColorAndOpacity )
					]
					+SOverlay::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.Padding( BubblePadding )
						.AutoWidth()
						[
							SAssignNew(TextBlock, SInlineEditableTextBlock)
							.Text( this, &SCommentBubble::GetCommentText )
							.Font( FEditorStyle::GetFontStyle( TEXT("Graph.Node.CommentFont")) )
							.ColorAndOpacity( FEditorStyle::GetColor("Graph.Node.Comment.TextColor"))
							.OnTextCommitted( this, &SCommentBubble::OnCommentTextCommitted )
						]
						+SHorizontalBox::Slot()
						.AutoWidth()
						.HAlign( HAlign_Right )
						.Padding( 0.f )
						[
							BubbleControls.ToSharedRef()
						]
					]
				]
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.Padding( BubbleOffset )
				.MaxWidth( CommentCalloutArrowBrush->ImageSize.X )
				[
					SNew(SImage)
					.Image( CommentCalloutArrowBrush )
					.ColorAndOpacity( ColorAndOpacity )
				]
			]
		];
	}
	else 
	{
		TSharedPtr<SWidget> TitleBarBubble = SNullWidget::NullWidget;

		if( bEnableTitleBarBubble )
		{
			const FMargin BubbleOffset = FEditorStyle::GetMargin( TEXT("Graph.Node.Comment.BubbleOffset"));
			// Create Title bar bubble toggle widget
			SAssignNew( TitleBarBubble, SHorizontalBox )
			.Visibility( this, &SCommentBubble::GetToggleButtonVisibility )
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign( HAlign_Center )
			.VAlign( VAlign_Top )
			.Padding( BubbleOffset )
			[
				SNew( SCheckBox )
				.Style( &FEditorStyle::Get().GetWidgetStyle< FCheckBoxStyle >( "CommentTitleButton" ))
				.IsChecked( ESlateCheckBoxState::Unchecked )
				.OnCheckStateChanged( this, &SCommentBubble::OnCommentBubbleToggle )
				.ToolTipText( NSLOCTEXT( "CommentBubble", "ToggleCommentTooltip", "Toggle Comment Bubble" ))
				.Cursor( EMouseCursor::Default )
				.ForegroundColor( this, &SCommentBubble::GetToggleButtonColour )
			];
		}
		ChildSlot
		[
			TitleBarBubble.ToSharedRef()
		];
	}
}


FVector2D SCommentBubble::GetOffset() const
{
	return FVector2D( 0.f, -GetDesiredSize().Y );
}

FVector2D SCommentBubble::GetSize() const
{
	return GetDesiredSize() + FVector2D( 20.f, 0.f );
}

bool SCommentBubble::IsBubbleVisible() const
{
	EGraphRenderingLOD::Type CurrLOD = GraphLOD.Get();
	const bool bShowScaled = CurrLOD > EGraphRenderingLOD::LowestDetail;
	const bool bShowPinned = CurrLOD <= EGraphRenderingLOD::MediumDetail;

	if( bAllowPinning )
	{
		return GraphNode->bCommentBubblePinned ? true : bShowScaled;
	}
	return bShowPinned;
}

bool SCommentBubble::IsScalingAllowed() const
{
	return !GraphNode->bCommentBubblePinned || !GraphNode->bCommentBubbleVisible;
}

FText SCommentBubble::GetCommentText() const
{
	if( CachedComment != CommentAttribute.Get() )
	{
		CachedComment = CommentAttribute.Get();
		CachedCommentText = FText::FromString( CachedComment );
	}
	return CachedCommentText;
}

FText SCommentBubble::GetScaleButtonTooltip() const
{
	if( GraphNode->bCommentBubblePinned )
	{
		return NSLOCTEXT( "CommentBubble", "AllowScaleButtonTooltip", "Allow this bubble to scale with zoom" );
	}
	else
	{
		return NSLOCTEXT( "CommentBubble", "PreventScaleButtonTooltip", "Prevent this bubble scaling with zoom" );
	}
}

FSlateColor SCommentBubble::GetToggleButtonColour() const
{
	const FLinearColor BubbleColor = ColorAndOpacity.Get().GetSpecifiedColor();
	return FLinearColor( BubbleColor.R, BubbleColor.G, BubbleColor.B, OpacityValue * OpacityValue );
}

void SCommentBubble::OnCommentTextCommitted( const FText& NewText, ETextCommit::Type CommitInfo )
{
	if( GraphNode )
	{
		const FScopedTransaction Transaction( NSLOCTEXT( "CommentBubble", "CommentCommitted", "Comment Changed" ) );
		GraphNode->Modify();
		GraphNode->NodeComment = NewText.ToString();
	}
}

EVisibility SCommentBubble::GetToggleButtonVisibility() const
{
	EVisibility Visibility = EVisibility::Hidden;

	if( OpacityValue > 0.f && !GraphNode->bCommentBubbleVisible )
	{
		Visibility = EVisibility::Visible;
	}
	return Visibility;
}

EVisibility SCommentBubble::GetBubbleVisibility() const
{
	const bool bIsVisible = !CachedCommentText.IsEmpty() && IsBubbleVisible();
	return bIsVisible ? EVisibility::Visible : EVisibility::Hidden;
}

void SCommentBubble::OnCommentBubbleToggle( ESlateCheckBoxState::Type State )
{
	if( GraphNode )
	{
		const FScopedTransaction Transaction( NSLOCTEXT( "CommentBubble", "BubbleVisibility", "Comment Bubble Visibilty" ) );
		GraphNode->Modify();
		GraphNode->bCommentBubbleVisible = !GraphNode->bCommentBubbleVisible;
		OpacityValue = 0.f;
		UpdateBubble();
	}
}

void SCommentBubble::OnPinStateToggle( ESlateCheckBoxState::Type State ) const
{
	if( GraphNode )
	{
		const FScopedTransaction Transaction( NSLOCTEXT( "CommentBubble", "BubblePinned", "Comment Bubble Pin" ) );
		GraphNode->Modify();
		GraphNode->bCommentBubblePinned = State == ESlateCheckBoxState::Checked;
	}
}