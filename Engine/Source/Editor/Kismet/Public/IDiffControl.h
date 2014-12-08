// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

class IDiffControl
{
public:
	virtual void NextDiff() = 0;
	virtual void PrevDiff() = 0;
	virtual bool HasNextDifference() const = 0;
	virtual bool HasPrevDifference() const = 0;
};

