// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IChatTabViewModel.h"
/**
 * Class containing the friend information - used to build the list view.
 */
class FChatChromeTabViewModel
	:	public TSharedFromThis<FChatChromeTabViewModel>,
		public IChatTabViewModel
{
public:

	virtual ~FChatChromeTabViewModel() {}


};

/**
* Creates the implementation for an ChatChromeViewModel.
*
* @return the newly created ChatChromeViewModel implementation.
*/
FACTORY(TSharedRef< FChatChromeTabViewModel >, FChatChromeTabViewModel, const TSharedRef<class FChatViewModel>& ChatViewModel);