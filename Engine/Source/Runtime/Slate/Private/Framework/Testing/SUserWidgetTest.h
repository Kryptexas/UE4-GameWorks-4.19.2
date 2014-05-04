// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SUserWidgetExample : public SUserWidget
{
	public:

	SLATE_USER_ARGS(SUserWidgetExample)
	: _Title()
	{}
		SLATE_ARGUMENT( FText, Title )
	SLATE_END_ARGS()

	virtual void DoStuff() = 0;
};




