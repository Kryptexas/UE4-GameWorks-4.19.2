// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * A reply to the OnQueryCursor event.
 */
class FCursorReply : public TReplyBase<FCursorReply>
{
public:

	/**
	 * Makes a NULL response meaning no prefersce.
	 * i.e. If your widget returns this, its parent will get to decide what the cursor shoudl be.
	 * This is the default behavior for a widget.
	 */
	static FCursorReply Unhandled()
	{
		return FCursorReply();
	}
		
	/**
	 * Respond with a specific cursor.
	 * This cursor will be used and no other widgets will be asked.
	 */
	static FCursorReply Cursor( EMouseCursor::Type InCursor )
	{
		return FCursorReply( InCursor );
	}
		
	/** @return The requested MouseCursor when the event was handled. Undefined otherwise. */
	EMouseCursor::Type GetCursor() { return MouseCursor; }

private:

	/** Internal constructor - makes a NULL result. */
	FCursorReply()
		: TReplyBase<FCursorReply>(false)
		, MouseCursor( EMouseCursor::Default )
	{ }
		
	/** Internal constructor - makes a non-NULL result. */
	FCursorReply( EMouseCursor::Type InCursorType )
		: TReplyBase<FCursorReply>(true)
		, MouseCursor( InCursorType )
	{ }
	
		
	/** The value of the reply, if bHasValue is true. */
	EMouseCursor::Type MouseCursor;
};
