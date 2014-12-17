// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateCorePrivatePCH.h"
#include "ActiveTickHandle.h"


FActiveTickHandle::FActiveTickHandle(float InTickFrequency, FWidgetActiveTickDelegate InTickFunction, double InNextTickTime) 
	: TickFrequency(InTickFrequency)
	, TickFunction(InTickFunction)
	, NextTickTime(InNextTickTime)
	, bExecutionPending(false)
{
}

bool FActiveTickHandle::IsReady(double CurrentTime) const
{
	return CurrentTime >= NextTickTime;
}

bool FActiveTickHandle::IsPendingExecution() const
{
	return bExecutionPending;
}

bool FActiveTickHandle::UpdateExecutionPendingState( double CurrentTime )
{
	bExecutionPending = IsReady( CurrentTime );
	return bExecutionPending;
}

EActiveTickReturnType FActiveTickHandle::ExecuteIfPending( double CurrentTime, float DeltaTime )
{
	// check if the handle timer is ready to fire.
	if ( bExecutionPending )
	{
		bExecutionPending = false;

		// before we do anything, check validity of the delegate.
		if ( !TickFunction.IsBound() )
		{
			// if the handle is no longer valid, we must assume it is invalid (say, owning widget was destroyed) and needs to be removed.
			return EActiveTickReturnType::StopTicking;
		}

		// update time in a while loop so we skip any times that we may have missed due to slow frames.
		if ( TickFrequency > 0.0 )
		{
			while ( CurrentTime >= NextTickTime )
			{
				NextTickTime += TickFrequency;
			}
		}
		// timer is updated, now call the delegate
		return TickFunction.Execute( CurrentTime, DeltaTime );
	}
	// if the handle is not ready, then we need to keep ticking it.
	return EActiveTickReturnType::KeepTicking;
}