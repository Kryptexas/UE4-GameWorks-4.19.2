// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AsyncResult.h"
#include "IPortalService.h"


/**
 * Interface for the Portal application's marketplace services.
 */
class IPortalAppWindow
	: public IPortalService
{
public:

	/**
	 * Opens the Portal application and shows the Friends window.
	 *
	 * @return true on success, false otherwise.
	 */
	virtual TAsyncResult<bool> OpenFriends() = 0;

	/**
	 * Opens the Portal application and shows the Library window.
	 *
	 * @return true on success, false otherwise.
	 */
	virtual TAsyncResult<bool> OpenLibrary() = 0;

	/**
	 * Opens the Portal application and shows the Marketplace.
	 *
	 * @return true on success, false otherwise.
	 */
	virtual TAsyncResult<bool> OpenMarketplace() = 0;

	/**
	 * Opens the Portal application and shows a particular marketplace item.
	 *
	 * @param ItemId The identifier of the marketplace item to open.
	 * @return true on success, false otherwise.
	 */
	virtual TAsyncResult<bool> OpenMarketItem(const FString& ItemId) = 0;

public:

	/** Virtual destructor. */
	virtual ~IPortalAppWindow() { }
};


Expose_TNameOf(IPortalAppWindow)
