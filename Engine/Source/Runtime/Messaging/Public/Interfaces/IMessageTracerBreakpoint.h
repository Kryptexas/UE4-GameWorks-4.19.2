// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IMessageTracerBreakpoint.h: Declares the IMessageTracerBreakpoint interface.
=============================================================================*/

#pragma once


/**
 * Type definition for shared pointers to instances of IMessageTracerBreakpoint.
 */
typedef TSharedPtr<class IMessageTracerBreakpoint, ESPMode::ThreadSafe> IMessageTracerBreakpointPtr;

/**
 * Type definition for shared references to instances of IMessageTracerBreakpoint.
 */
typedef TSharedRef<class IMessageTracerBreakpoint, ESPMode::ThreadSafe> IMessageTracerBreakpointRef;


/**
 * Interface for message tracer breakpoints.
 */
class IMessageTracerBreakpoint
{
public:

	/**
	 * Checks whether this breakpoint is enabled.
	 *
	 * @return true if the breakpoint is enabled, false otherwise.
	 */
	virtual bool IsEnabled( ) const = 0;

	/**
	 * Checks whether the tracer should break on the given message.
	 *
	 * @param Context - The message to break on.
	 *
	 * @return true if the tracer should break, false otherwise.
	 */
	virtual bool ShouldBreak( const IMessageContextRef& Context ) const = 0;

protected:

	/**
	 * Hidden destructor.
	 */
	virtual ~IMessageTracerBreakpoint( ) { }
};
