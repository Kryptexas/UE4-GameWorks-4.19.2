// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

class IDiffControl
{
public:
	virtual void NextDiff() = 0;
	virtual void PrevDiff() = 0;
	virtual bool HasDifferences() const = 0;
};

