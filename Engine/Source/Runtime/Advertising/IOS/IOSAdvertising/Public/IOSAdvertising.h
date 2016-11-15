// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IAdvertisingProvider.h"
#include "Core.h"

class FIOSAdvertisingProvider : public IAdvertisingProvider
{
public:
	virtual void ShowAdBanner( bool bShowOnBottomOfScreen, int32 AdID ) override;
	virtual void HideAdBanner() override;
	virtual void CloseAdBanner() override;
	virtual int32 GetAdIDCount() override;

	//empty functions for now, this is Android-only support until iAd is replaced by AdMob
	virtual void LoadInterstitialAd(int32 adID) override {}
	virtual bool IsInterstitialAdAvailable() override
	{
		return false;
	}
	virtual bool IsInterstitialAdRequested() override
	{
		return false;
	}
	virtual void ShowInterstitialAd() override {}
};
