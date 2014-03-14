// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IProfilerServiceModule.h: Declares the IProfileServiceModule interface.
=============================================================================*/

#pragma once


/**
 * Interface for ProfilerService modules.
 */
class IProfilerServiceModule
	: public IModuleInterface
{
public:
	/** 
	 * Creates a profiler service.
	 *
	 * @param Capture - specifies whether to start capturing immediately
	 *
	 * @return A new profiler service.
	 */
	virtual IProfilerServiceManagerPtr CreateProfilerServiceManager( ) = 0;

protected:

	/**
	 * Virtual destructor
	 */
	virtual ~IProfilerServiceModule( ) { }
};
