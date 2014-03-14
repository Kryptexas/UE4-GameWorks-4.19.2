// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class ILogVisualizer
{
public:
	/** Virtual destructor */
	virtual ~ILogVisualizer() {}

	/** spawns LogVisualizer's UI */
	virtual void SummonUI(UWorld* InWorld) = 0;

	/** closes LogVisualizer's UI */
	virtual void CloseUI(UWorld* InWorld) = 0;
};