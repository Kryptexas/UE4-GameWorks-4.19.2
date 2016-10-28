// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once


class IProfilerClient;


/**
 * Interface for ProfilerClient modules.
 */
class IProfilerClientModule
	: public IModuleInterface
{
public:

	/**
	 * Creates a Profiler client.
	 *
	 * @return The new client.
	 */
	virtual TSharedPtr<IProfilerClient> CreateProfilerClient() = 0;

protected:

	/** Virtual destructor */
	virtual ~IProfilerClientModule() { }
};
