// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Design constraints
 */
namespace EditableTextDefs
{
	/** Maximum number of undo levels to store */
	static const int32 MaxUndoLevels = 99;

	/** Vertical text offset from top of widget, as a scalar percentage of maximum character height */
	static const float TextVertOffsetPercent = 0.0f;

	/** How quickly the cursor's spring will pull the cursor around.  Larger is faster. */
	static const float CaretSpringConstant = 65.0f;

	/** Vertical offset of caret from text location, in pixels */
	static const float CaretVertOffset = 1.0f;

	/** Width of the caret, as a scalar percentage of the font's maximum character height */
	static const float CaretWidthPercent = 0.08f;

	/** Height of the caret, as a scalar percentage of the font's maximum character height */
	static const float CaretHeightPercent = 0.95f;

	/** How long after the user last interacted with the keyboard should we keep the caret at full opacity? */
	static const float CaretBlinkPauseTime = 0.1f;

	/** How many times should the caret blink per second (full on/off cycles) */
	static const float BlinksPerSecond = 1.0f;

	/** How many pixels to extend the selection rectangle's left side horizontally */
	static const float SelectionRectLeftOffset = 0.0f;

	/** How many pixels to extend the selection rectangle's right side horizontally */
	static const float SelectionRectRightOffset = 0.0f;

	/** How quickly the selection 'targeting' rectangle will slide around.  Larger is faster. */
	static const float SelectionTargetSpringConstant = 25.0f;

	/** Duration of animation selection target effects */
	static const float SelectionTargetEffectDuration = 0.25f;

	/** Opacity of the selection target effect overlay */
	static const float SelectionTargetOpacity = 0.8f;

	/** How large the selection target effect will be when selecting text, as a scalar percentage of font height */
	static const float SelectingAnimOffsetPercent = 0.15f;

	/** How large the selection target effect will be when deselecting text, as a scalar percentage of font height */
	static const float DeselectingAnimOffsetPercent = 0.25f;
}

/**
 * Contains helper routines that translate input into actions for an ITextEditorWidget.
 * Used to share code between SEditableText and SMultiLineEditableText
 */
class FTextEditHelper
{
public:

	/** Someone typed a character, translate the input event into a call to a ITextEditorWidget action */
	static FReply OnKeyChar( const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent, class ITextEditorWidget& TextEditor );

	/** Someone pressed a key, translate the input event into a call to a ITextEditorWidget action */
	static FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent, ITextEditorWidget& TextEditor );

};