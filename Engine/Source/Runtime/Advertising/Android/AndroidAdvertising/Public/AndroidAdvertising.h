// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IAdvertisingProvider.h"
#include "Core.h"

class FAndroidAdvertisingProvider : public IAdvertisingProvider
{
public:
	virtual void ShowAdBanner( bool bShowOnBottomOfScreen ) OVERRIDE;
	virtual void HideAdBanner() OVERRIDE;
	virtual void CloseAdBanner() OVERRIDE;
};
