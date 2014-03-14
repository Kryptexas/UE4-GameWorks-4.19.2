// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	ApplePlatformRunnableThread.h: Apple platform threading functions
==============================================================================================*/

#pragma once

#include "Runtime/Core/Private/HAL/PThreadRunnableThread.h"

/**
* Apple implementation of the Process OS functions
**/
class FRunnableThreadApple : public FRunnableThreadPThread
{
public:
    FRunnableThreadApple()
    :   Pool(NULL)
    {
    }
    
private:
	/**
	 * Allows a platform subclass to setup anything needed on the thread before running the Run function
	 */
	virtual void PreRun()
	{
		//@todo - zombie - This and the following build somehow. It makes no sense to me. -astarkey
		Pool = FPlatformMisc::CreateAutoreleasePool();
		pthread_setname_np(TCHAR_TO_ANSI(*ThreadName));
	}

	/**
	 * Allows a platform subclass to teardown anything needed on the thread after running the Run function
	 */
	virtual void PostRun()
	{
		FPlatformMisc::ReleaseAutoreleasePool(Pool);
	}
    
    void*      Pool;
};

