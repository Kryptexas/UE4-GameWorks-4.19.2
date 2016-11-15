// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Misc/FilterCollection.h"

/**
* The screen shot filter collection - used for updating the screen shot list
*/
typedef TFilterCollection< const TSharedPtr< class IScreenShotData >& > ScreenShotFilterCollection;
