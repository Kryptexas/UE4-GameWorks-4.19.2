// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** A reply to the OnQueryCursor */
class FCursorReply
{
public:
	/** Makes a NULL response; this response will not change the previous value. */
	static FCursorReply Unhandled()
	{
		return FCursorReply();
	}
		
	/** Respond with a cursor type. */
	static FCursorReply Cursor( EMouseCursor::Type InCursor )
	{
		return FCursorReply( InCursor );
	}
		
	/** @return true if this this reply is a result of the event being handled; false otherwise. */
	bool IsEventHandled(){ return bIsHandled; }
		
	/** @return The requested MouseCursor when the event was handled. Undefined otherwise. */
	EMouseCursor::Type GetCursor() { return MouseCursor; }
		
private:
	/** Internal constructor - makes a NULL result. */
	FCursorReply()
	: bIsHandled(false)
	, MouseCursor( EMouseCursor::Default )
	{		
	}
		
	/** Internal constructor - makes a non-NULL result. */
	FCursorReply( EMouseCursor::Type InCursorType )
	: bIsHandled(true)
	, MouseCursor( InCursorType )
	{		
	}
		
	/** Does this reply have any meaning */ 
	bool bIsHandled;
		
	/** The value of the reply, if bHasValue is true. */
	EMouseCursor::Type MouseCursor;
};