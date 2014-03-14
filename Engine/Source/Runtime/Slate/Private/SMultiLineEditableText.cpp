// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "TextEditHelper.h"
#include "SlateWordWrapper.h"

SMultiLineEditableText::SMultiLineEditableText()
: CursorPosition( FTextLocation() )
, FontInfo( FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 10 ) 
, NumLinesScrollOffset( 0.0f )
, PreferredCursorOffsetInLine( 0 )
, LastCursorInteractionTime( 0 )
, PreferredSize( FVector2D::ZeroVector )
{
}

void SMultiLineEditableText::Construct( const FArguments& InArgs )
{
	NumLinesScrollOffset = 0.0f;
	OptionalScrollBarPtr = InArgs._ScrollBar;
	PreferredSize = InArgs._PreferredSize;
	if (InArgs._ScrollBar.IsValid())
	{
		InArgs._ScrollBar->SetOnUserScrolled( FOnUserScrolled::CreateSP( this, &SMultiLineEditableText::OnScrollbarScrolled ) );
	}

	TArray<FString> TempTextLines;
	InArgs._Text.ParseIntoArray( &TempTextLines, TEXT("\n"), false );
	TextLines.Reserve( TempTextLines.Num() );

	for ( int32 LineIndex=0; LineIndex < TempTextLines.Num(); ++LineIndex )
	{
		TextLines.Add( MakeShareable( new FString( TempTextLines[LineIndex] ) ) );
	}
}

void SMultiLineEditableText::StartChangingText()
{
}

void SMultiLineEditableText::FinishChangingText()
{
}

bool SMultiLineEditableText::GetIsReadOnly() const
{
	return false;
}

void SMultiLineEditableText::BackspaceChar()
{
}

void SMultiLineEditableText::DeleteChar()
{
}

bool SMultiLineEditableText::CanTypeCharacter(const TCHAR CharInQuestion) const
{
	return true;
}

void SMultiLineEditableText::TypeChar( const int32 Character )
{
}

FReply SMultiLineEditableText::MoveCursor( ECursorMoveMethod::Type Method, int8 Direction, ECursorAction::Type Action )
{
	FTextLocation NewCursorPosition;
	if (Method == ECursorMoveMethod::CharacterHorizontal)
	{
		NewCursorPosition = TranslatedLocation(CursorPosition, Direction);
		PreferredCursorOffsetInLine = AbsoluteToWrapped(NewCursorPosition).GetOffset();
	}
	else if (Method == ECursorMoveMethod::Word)
	{
		NewCursorPosition = ScanForWordBoundary( CursorPosition, Direction );
		PreferredCursorOffsetInLine = AbsoluteToWrapped(NewCursorPosition).GetOffset();
	}
	else
	{
		ensure( Method == ECursorMoveMethod::CharacterVertical );
		NewCursorPosition = TranslateLocationVertical(CursorPosition, Direction);
	}

	if (Action == ECursorAction::SelectText)
	{
		// We are selecting text. Just remember where the selection started.
		// The cursor is implicitly the other endpoint.
		if (!SelectionStart.IsSet())
		{
			this->SelectionStart = CursorPosition;
		}
	}
	else
	{
		// No longer selection text; clear the selection!
		this->ClearSelection();
	}

	if( WrappedText.Num() > 0 )
	{
		const int32 NewWrappedCursorPosition = AbsoluteToWrapped( NewCursorPosition ).GetLineIndex();
		const float LineHeight = WrappedText[NewCursorPosition.GetLineIndex()].Size.Y;
		const float NumLinesOnScreen = CachedLastFrameGeometry.Size.Y / LineHeight;
		const float FractionOfTotalLinesOnScreen = FMath::Clamp(NumLinesOnScreen / WrappedText.Num(), 0.0f, 1.0f);
		if ( NewWrappedCursorPosition < NumLinesScrollOffset )
		{
			NumLinesScrollOffset = NewWrappedCursorPosition;
			UpdateScrollbar( CachedLastFrameGeometry );
		}
		else if ( FMath::Floor(NumLinesScrollOffset + NumLinesOnScreen - 1) < NewWrappedCursorPosition )
		{
			NumLinesScrollOffset = FMath::Clamp( FMath::Floor(NewWrappedCursorPosition - NumLinesOnScreen + 1 ), 0, WrappedText.Num());
			UpdateScrollbar( CachedLastFrameGeometry );
		}
	}

	CursorPosition = NewCursorPosition;

	LastCursorInteractionTime = FSlateApplication::Get().GetCurrentTime();

	return FReply::Handled();

}
void SMultiLineEditableText::JumpTo(ETextLocation::Type JumpLocation, ECursorAction::Type Action)
{
	
}

void SMultiLineEditableText::ClearSelection()
{
	SelectionStart = TOptional<FTextLocation>();
}

void SMultiLineEditableText::SelectAllText()
{
	SelectionStart = FTextLocation(0,0);
	CursorPosition = FTextLocation( TextLines.Num() - 1, TextLines[TextLines.Num()-1]->Len() );
}

FReply SMultiLineEditableText::OnEscape()
{
	return FReply::Unhandled();
}

void SMultiLineEditableText::OnEnter()
{
}

bool SMultiLineEditableText::CanExecuteCut() const
{
	return true;
}

void SMultiLineEditableText::CutSelectedTextToClipboard()
{
}

bool SMultiLineEditableText::CanExecuteCopy() const
{
	return true;
}

void SMultiLineEditableText::CopySelectedTextToClipboard()
{
}

bool SMultiLineEditableText::CanExecutePaste() const
{
	return true;
}

void SMultiLineEditableText::PasteTextFromClipboard()
{
}

bool SMultiLineEditableText::CanExecuteUndo() const
{
	return true;
}

void SMultiLineEditableText::Undo()
{
}

void SMultiLineEditableText::Redo()
{
}

void SMultiLineEditableText::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	if ( CachedLastFrameGeometry.Size != AllottedGeometry.Size )
	{
		CachedLastFrameGeometry = AllottedGeometry;
		ReWrap( AllottedGeometry );
		UpdateScrollbar(AllottedGeometry);

	}
}

int32 SMultiLineEditableText::OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	const int32 TextLayer = LayerId;
	const int32 SelectionLayer = LayerId + 1;
	const TSharedRef< FSlateFontMeasure > FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

	const float CursorWidth = 2.0f;

	const double CurrentTime = FSlateApplication::Get().GetCurrentTime();

	const bool bEnabled = ShouldBeEnabled( bParentEnabled );
	const ESlateDrawEffect::Type DrawEffects = (bEnabled) ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

	const int32 WholeLinesOffset = FMath::Floor( NumLinesScrollOffset );
	const float PartialLineOffset = FMath::Fractional( NumLinesScrollOffset );


	if (WrappedText.Num() > 0)
	{
		float HeightOffsetSoFar = - PartialLineOffset *  WrappedText[FMath::Clamp(WholeLinesOffset, 0, WrappedText.Num()-1)].Size.Y;

		bool FilledVerticalSpace = false;

		const FTextRange Selection = FTextRange(SelectionStart.GetValue(), CursorPosition);
		FTextRange SelectionInWrappedText = (SelectionStart.IsSet())
			? AbsoluteToWrapped( Selection )
			: FTextRange( FTextLocation(), FTextLocation() );

		bool bInSelection = SelectionInWrappedText.First.GetLineIndex() < WholeLinesOffset && SelectionInWrappedText.First.GetLineIndex() > WholeLinesOffset;

		for ( int32 LineIndex=WholeLinesOffset; !FilledVerticalSpace && LineIndex < WrappedText.Num(); ++LineIndex )
		{
			const FWrappedStringSlice& ThisWrappedLine = WrappedText[LineIndex];
			// @todo FontService: Copy is undesirable?
			const FString StringToPaint = ThisWrappedLine.SourceString.Mid( ThisWrappedLine.FirstCharIndex, ThisWrappedLine.LastCharIndex-ThisWrappedLine.FirstCharIndex );

			// Draw this wrapped line of text
			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(FVector2D(0, HeightOffsetSoFar), FVector2D::UnitVector),
				StringToPaint,
				FontInfo,
				MyClippingRect,
				ShouldBeEnabled(bParentEnabled),
				InWidgetStyle.GetColorAndOpacityTint()
			);

			static const FLinearColor SelectionColor = FLinearColor(0,0,1.0f,0.25f);

			// DRAW SELECTION
			if ( SelectionStart.IsSet() )
			{
				const bool bIsFirstSelectionLine =
					(&(TextLines[Selection.First.GetLineIndex()].Get()) == &ThisWrappedLine.SourceString) &&
					Selection.First.GetOffset() >= ThisWrappedLine.FirstCharIndex &&
					Selection.First.GetOffset() <= ThisWrappedLine.LastCharIndex;

				const bool bIsLastSelectionLine =
				 	(&(TextLines[Selection.Last.GetLineIndex()].Get()) == &ThisWrappedLine.SourceString) &&
					Selection.Last.GetOffset() >= ThisWrappedLine.FirstCharIndex &&
					Selection.Last.GetOffset() <= ThisWrappedLine.LastCharIndex;

				if ( bIsFirstSelectionLine && bIsLastSelectionLine )
				{
					// Selection contained within a single line.
					const FString StringBeforeSelection = ThisWrappedLine.SourceString.Left( SelectionInWrappedText.First.GetOffset() );
					const FVector2D SizeBeforeSelection = FontMeasureService->Measure(StringBeforeSelection, FontInfo, AllottedGeometry.Scale);					
					
					const FString SelectedString = ThisWrappedLine.SourceString.Mid( SelectionInWrappedText.First.GetOffset(), SelectionInWrappedText.Last.GetOffset() - SelectionInWrappedText.First.GetOffset() );		
					const FVector2D SelectionSize = FontMeasureService->Measure(SelectedString, FontInfo, AllottedGeometry.Scale);

					FSlateDrawElement::MakeBox(
						OutDrawElements,
						SelectionLayer,
						AllottedGeometry.ToPaintGeometry(FVector2D(SizeBeforeSelection.X, HeightOffsetSoFar), SelectionSize),
						FCoreStyle::Get().GetBrush( "WhiteBrush" ),
						MyClippingRect,
						DrawEffects,
						SelectionColor
					);

				}
				else if ( bIsFirstSelectionLine )
				{
					// Selection starts on this line, ends on another.
					bInSelection = true;

					// @todo FontService: Copy is unnecessary
					const FString StringBeforeSelection = StringToPaint.Left(SelectionInWrappedText.First.GetOffset());
					const FVector2D UnselectedPortionSize = FontMeasureService->Measure(StringBeforeSelection, FontInfo, AllottedGeometry.Scale);
					FSlateDrawElement::MakeBox(
						OutDrawElements,
						SelectionLayer,
						AllottedGeometry.ToPaintGeometry(FVector2D(UnselectedPortionSize.X, HeightOffsetSoFar), ThisWrappedLine.Size - FVector2D(UnselectedPortionSize.X, 0)),
						FCoreStyle::Get().GetBrush( "WhiteBrush" ),
						MyClippingRect,
						DrawEffects,
						SelectionColor
					);
				}
				else if ( bIsLastSelectionLine )
				{
					// Selection starts on some line above, ends on this one.
					bInSelection = false;

					// @todo FontService: Copy is unnecessary
					const FString StringBeforeSelection = StringToPaint.Left(SelectionInWrappedText.Last.GetOffset());
					const FVector2D SelectedPortionSize = FontMeasureService->Measure(StringBeforeSelection, FontInfo, AllottedGeometry.Scale);
					FSlateDrawElement::MakeBox(
						OutDrawElements,
						SelectionLayer,
						AllottedGeometry.ToPaintGeometry(FVector2D(0, HeightOffsetSoFar), FVector2D(SelectedPortionSize.X, ThisWrappedLine.Size.Y)),
						FCoreStyle::Get().GetBrush( "WhiteBrush" ),
						MyClippingRect,
						DrawEffects,
						SelectionColor
					);
				}
				else if (bInSelection)
				{
					// The entirety of this line is selected
					FSlateDrawElement::MakeBox(
						OutDrawElements,
						SelectionLayer,
						AllottedGeometry.ToPaintGeometry(FVector2D(0, HeightOffsetSoFar), ThisWrappedLine.Size),
						FCoreStyle::Get().GetBrush( "WhiteBrush" ),
						MyClippingRect,
						DrawEffects,
						SelectionColor
					);
				}

			}

			// DRAW CURSOR
			if (this->HasKeyboardFocus())
			{
				const bool CursorIsOnThisTextLine = ( &ThisWrappedLine.SourceString == &(TextLines[CursorPosition.GetLineIndex()].Get()) );
				const bool bIsStartOfNewLine = ThisWrappedLine.FirstCharIndex == 0;
				if ( CursorIsOnThisTextLine &&
					((bIsStartOfNewLine && CursorPosition.GetOffset() == 0) || CursorPosition.GetOffset() > ThisWrappedLine.FirstCharIndex) &&
					CursorPosition.GetOffset() <= ThisWrappedLine.LastCharIndex )
				{
					// The cursor is always visible (i.e. not blinking) when we're interacting with it; otherwise it might get lost.
					const bool bForceCursorVisible = (CurrentTime - LastCursorInteractionTime) < EditableTextDefs::CaretBlinkPauseTime;
					const float CursorOpacity = (bForceCursorVisible)
						? 1.0f
						: FMath::Round( FMath::MakePulsatingValue( CurrentTime, EditableTextDefs::BlinksPerSecond ));

					const int32 CursorOffsetInWrappedLine = CursorPosition.GetOffset() - ThisWrappedLine.FirstCharIndex;
					const FVector2D SlateUnitsCursorOffsetInLine = FontMeasureService->Measure( StringToPaint.Left(CursorOffsetInWrappedLine), FontInfo, AllottedGeometry.Scale );

					const FVector2D DrawPosition = FVector2D( SlateUnitsCursorOffsetInLine.X, HeightOffsetSoFar );
					const FVector2D DrawSize = FVector2D( CursorWidth, ThisWrappedLine.Size.Y );
					// @todo: State Styles - make this brush part of the widget style
					const FSlateBrush* StyleInfo = FCoreStyle::Get().GetBrush("EditableText.SelectionBackground");

					FSlateDrawElement::MakeBox(
						OutDrawElements,
						LayerId,
						AllottedGeometry.ToPaintGeometry(DrawPosition, DrawSize),
						StyleInfo,
						MyClippingRect,
						DrawEffects,
						FLinearColor(1,1,1,CursorOpacity) );

				}
			}

			HeightOffsetSoFar += WrappedText[LineIndex].Size.Y;
		}
	}

	return SelectionLayer;
}


FVector2D SMultiLineEditableText::ComputeDesiredSize() const
{
	return PreferredSize;
}

bool SMultiLineEditableText::SupportsKeyboardFocus() const
{
	return true;
}

FReply SMultiLineEditableText::OnKeyChar( const FGeometry& MyGeometry,const FCharacterEvent& InCharacterEvent )
{
	return FTextEditHelper::OnKeyChar( MyGeometry, InCharacterEvent, *this );
}

FReply SMultiLineEditableText::OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	FReply Reply = FTextEditHelper::OnKeyDown( MyGeometry, InKeyboardEvent, *this );

	//// Process keybindings if the event wasn't already handled
	//if( !Reply.IsEventHandled() && UICommandList->ProcessCommandBindings( InKeyboardEvent ) )
	//{
	//	Reply = FReply::Handled();
	//}

	return Reply;
}

FReply SMultiLineEditableText::OnKeyUp( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	return FReply::Unhandled();
}


void SMultiLineEditableText::ReWrap( const FGeometry& AllottedGeometry )
{
	SlateWordWrapper::WrapText(TextLines, FontInfo, AllottedGeometry.Scale, AllottedGeometry.Size.X , WrappedText);
}

void SMultiLineEditableText::OnScrollbarScrolled( float OffsetFraction )
{
	NumLinesScrollOffset = FMath::Clamp(OffsetFraction * WrappedText.Num(), 0.0f, WrappedText.Num()-1.0f);
	UpdateScrollbar( CachedLastFrameGeometry );
}

/** Remember where the cursor was when we started selectin. */
void SMultiLineEditableText::BeginSelecting( const FTextLocation& Endpoint )
{
	SelectionStart = Endpoint;
}


void SMultiLineEditableText::UpdateScrollbar( const FGeometry &AllottedGeometry ) 
{
	TSharedPtr<SScrollBar> OptionalScrollBar = OptionalScrollBarPtr.Pin();
	if (OptionalScrollBar.IsValid() && WrappedText.Num() > 0)
	{
		const float LineHeight = WrappedText[0].Size.Y;
		const float NumLinesOnScreen = AllottedGeometry.Size.Y / LineHeight;
		const float FractionOfTotalLinesOnScreen = FMath::Clamp(NumLinesOnScreen / WrappedText.Num(), 0.0f, 1.0f);
		const float MaxVerticalOffsetInLines = FMath::Max(0.0f, WrappedText.Num() - NumLinesOnScreen);
		NumLinesScrollOffset = FMath::Clamp( NumLinesScrollOffset, 0.0f, MaxVerticalOffsetInLines );
		OptionalScrollBar->SetState(NumLinesScrollOffset / WrappedText.Num(), FractionOfTotalLinesOnScreen);
	}
}

SMultiLineEditableText::FTextLocation SMultiLineEditableText::TranslatedLocation( const FTextLocation& Location, int8 Direction ) const
{
	const int32 OffsetInLine = Location.GetOffset() + Direction;
		
	if ( OffsetInLine > TextLines[Location.GetLineIndex()]->Len() && Location.GetLineIndex() < TextLines.Num()-1 )
	{
		// We're going over the end of the line and we aren't on the last line
		return FTextLocation( Location.GetLineIndex() + 1, 0 );
	}
	else if ( OffsetInLine < 0 && Location.GetLineIndex() > 0 )
	{
		// We're stepping before the beginning of the line, and we're not on the first line.
		return FTextLocation( Location.GetLineIndex() - 1, TextLines[Location.GetLineIndex() - 1]->Len() );	
	}
	else
	{
		return FTextLocation( Location.GetLineIndex(), FMath::Clamp(OffsetInLine, 0, TextLines[Location.GetLineIndex()]->Len()) );
	}
}

/** Find wrapped line on which the cursor is currently located. */
struct FMatchWrappedLine
{
	FMatchWrappedLine( const FString& InStringToMatch, int32 InOffset )
		: StringToMatch( InStringToMatch )
		, Offset( InOffset )
		{}
	bool Matches( const FWrappedStringSlice& CandidateLine  ) const
	{
		return
			(&StringToMatch == &(CandidateLine.SourceString)) &&
			Offset >= CandidateLine.FirstCharIndex &&
			Offset <= CandidateLine.LastCharIndex;
	}
	const FString& StringToMatch;
	const int32 Offset;
};

SMultiLineEditableText::FTextLocation SMultiLineEditableText::TranslateLocationVertical( const FTextLocation& Location, int8 Direction ) const
{
	const int32 CurrentWrappedLineIndex = WrappedText.FindMatch( FMatchWrappedLine( TextLines[Location.GetLineIndex()].Get(), Location.GetOffset() ) );
	const FWrappedStringSlice& CurrentWrappedLine = WrappedText[CurrentWrappedLineIndex];
	ensure(CurrentWrappedLineIndex != INDEX_NONE);

	const int32 NewWrappedLinedIndex = FMath::Clamp(CurrentWrappedLineIndex + Direction, 0, WrappedText.Num()-1);
	const FWrappedStringSlice& NewWrappedLine = WrappedText[NewWrappedLinedIndex];
	
	// We moved wrapped lines. Did we actually move to a different source line?
	const bool bMovedToNewSourceLine = &CurrentWrappedLine.SourceString != &NewWrappedLine.SourceString;
	const int32 NewSourceLineIndex = Location.GetLineIndex() + ( bMovedToNewSourceLine ? Direction : 0 );
	// Our horizontal position is the clamped version of whatever the user explicitly set with horizontal movement.
	const int32 NewOffsetInLine = FMath::Clamp(NewWrappedLine.FirstCharIndex + PreferredCursorOffsetInLine, 0, NewWrappedLine.LastCharIndex);
	
	return FTextLocation( NewSourceLineIndex, NewOffsetInLine );	
}

SMultiLineEditableText::FTextLocation SMultiLineEditableText::ScanForWordBoundary( const FTextLocation& CurrentLocation, int8 Direction ) const
{
	FTextLocation Location = TranslatedLocation(CurrentLocation, Direction);

	while ( !IsAtStartOfDoc(Location) && !IsAtStartOfLine(Location) && !IsAtEndOfDoc(Location) && !IsAtEndOfLine(Location) && !IsAtWordStart(Location) )
	{
		Location = TranslatedLocation(Location, Direction);
	}	

	return Location;
}

TCHAR SMultiLineEditableText::GetCharacterAt( const FTextLocation& Location ) const
{
	const bool bIsLineEmpty = TextLines[Location.GetLineIndex()]->IsEmpty();
	return (bIsLineEmpty)
		? '\n'
		: (*TextLines[Location.GetLineIndex()])[Location.GetOffset()];
}


bool SMultiLineEditableText::IsAtStartOfDoc( const FTextLocation& Location ) const
{
	return Location.GetLineIndex() == 0 && Location.GetOffset() == 0;
}
	

bool SMultiLineEditableText::IsAtEndOfDoc( const FTextLocation& Location ) const
{
	return TextLines.Num() == 0 || ( TextLines.Num()-1 == Location.GetLineIndex() && TextLines[Location.GetLineIndex()]->Len()-1 == Location.GetOffset());
}

bool SMultiLineEditableText::IsAtStartOfLine( const FTextLocation& Location ) const
{
	return Location.GetOffset() == 0;
}

bool SMultiLineEditableText::IsAtEndOfLine( const FTextLocation& Location ) const
{
	return TextLines[Location.GetLineIndex()]->Len()-1 == Location.GetOffset();
}

bool SMultiLineEditableText::IsAtWordStart( const FTextLocation& Location ) const
{
	const TCHAR CharBeforeCursor = GetCharacterAt(TranslatedLocation(Location, -1));
	const TCHAR CharAfterCursor = GetCharacterAt(TranslatedLocation(Location, +1));

	return FChar::IsWhitespace(CharBeforeCursor) && !FChar::IsWhitespace(CharAfterCursor);
}

SMultiLineEditableText::FTextLocation SMultiLineEditableText::AbsoluteToWrapped( const FTextLocation& Location ) const
{
	const int32 CurrentWrappedLineIndex = WrappedText.FindMatch( FMatchWrappedLine( TextLines[Location.GetLineIndex()].Get(), Location.GetOffset() ) );
	const int32 OffsetInWrappedLine = Location.GetOffset() - WrappedText[CurrentWrappedLineIndex].FirstCharIndex;
	return FTextLocation(CurrentWrappedLineIndex, OffsetInWrappedLine);
}

SMultiLineEditableText::FTextRange SMultiLineEditableText::AbsoluteToWrapped( const FTextRange& Range ) const
{
	return FTextRange( AbsoluteToWrapped(Range.First), AbsoluteToWrapped(Range.Last) );
}
