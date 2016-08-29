// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "CorePrivatePCH.h"
#include "SlowTaskStack.h"
#include "SlowTask.h"

float FSlowTaskStack::GetProgressFraction(int32 Index) const
{
	const int32 StartIndex = Num() - 1;
	const int32 EndIndex = Index;

	float Progress = 0.f;
	for (int32 CurrentIndex = StartIndex; CurrentIndex >= EndIndex; --CurrentIndex)
	{
		const FSlowTask* Scope = (*this)[CurrentIndex];
		
		const float ThisScopeCompleted = float(Scope->CompletedWork) / Scope->TotalAmountOfWork;
		const float ThisScopeCurrentFrame = float(Scope->CurrentFrameScope) / Scope->TotalAmountOfWork;

		Progress = ThisScopeCompleted + ThisScopeCurrentFrame*Progress;
	}

	return Progress;
}