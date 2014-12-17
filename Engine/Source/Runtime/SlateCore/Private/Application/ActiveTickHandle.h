// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateCore.h"

/** Stores info about an active tick delegate for a widget. */
class FActiveTickHandle
{
public:
	/** Public ctor that initializes a new tick handle. Not intended to be called by user code. */
	FActiveTickHandle(float InTickFrequency, FWidgetActiveTickDelegate InTickFunction, double InNextTickTime);

	/** @return True if the active tick is pending execution */
	bool IsPendingExecution() const;

	/**
	* Updates the pending state of the active tick based on the current time
	* @return True if the active tick is pending execution
	*/
	bool UpdateExecutionPendingState( double CurrentTime );

	/**
	 * Execute the bound delegate if the active tick is bPendingExecution.
	 * @return ETickWidgetReturnType::StopTicking if the handle should be auto-unregistered
	 *         (either return value indicated this or the handle has become invalid).
	 */
	EActiveTickReturnType ExecuteIfPending( double CurrentTime, float DeltaTime );

private:
	/** @return true if the tick handle is ready to have its delegate executed. */
	bool IsReady( double CurrentTime ) const;

private:
	/** The frequency with which this active tick should execute */
	float TickFrequency;

	/** The delegate to the active tick function */
	FWidgetActiveTickDelegate TickFunction;
	
	/** The next time at which to execute the active tick */
	double NextTickTime;

	/** True if execution of the active tick is pending */
	bool bExecutionPending;
};


