// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleInterface.h"
#include "TypeContainer.h"


class IPortalService;


/**
 * Interface for the PortalServices module.
 */
class IPortalServicesModule
	: public IModuleInterface
{
public:

	/**
	 * Gets a container with registered Portal services.
	 *
	 * @return The type container.
	 */
	virtual FTypeContainer& GetServiceContainer() = 0;

public:

	/**
	 * Gets a shared pointer to an instance of the specified service.
	 *
	 * @param T The type of service to get an instance for.
	 * @return The service instance, or nullptr if not available.
	 */
	template<class T>
	TSharedPtr<T> GetService()
	{
		return GetServiceContainer().GetInstance<T>();
	}

public:

	/** Virtual destructor. */
	virtual ~IPortalServicesModule() { }
};
