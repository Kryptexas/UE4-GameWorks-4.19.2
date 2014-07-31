// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once


#include "OnlineDelegateMacros.h"
#include "OnlineStats.h"


/**
* Delegate fired when a connection attempted has completed
*/
DECLARE_DELEGATE_TwoParams(FOnConnectionCompleteDelegate, const int, const bool); // TODO: pass down status messages...


/**
 *	IOnlineConnection - Interface class for connection management
 */
class IOnlineConnection
{
public:
	virtual ~IOnlineConnection() {}

	//@TODO: Pure virtual?
	virtual void Connect(const FOnConnectionCompleteDelegate & Delegate = FOnConnectionCompleteDelegate()) = 0;
};