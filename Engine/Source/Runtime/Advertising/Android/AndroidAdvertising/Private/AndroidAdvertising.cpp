// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AndroidAdvertising.h"


DEFINE_LOG_CATEGORY_STATIC( LogAdvertising, Display, All );

IMPLEMENT_MODULE( FAndroidAdvertisingProvider, AndroidAdvertising );


void FAndroidAdvertisingProvider::ShowAdBanner(bool bShowOnBottomOfScreen, int32 adID)
{
	FString AdUnitID;
	bool found = GConfig->GetString(TEXT("/Script/AndroidRuntimeSettings.AndroidRuntimeSettings"), TEXT("AdMobAdUnitID"), AdUnitID, GEngineIni);

	TArray<FString> AdUnitIDs;
	int32 count = GConfig->GetArray(TEXT("/Script/AndroidRuntimeSettings.AndroidRuntimeSettings"), TEXT("AdMobAdUnitID"), AdUnitIDs, GEngineIni);
	
	if (count == 0 || adID >= count)
		return;

	extern void AndroidThunkCpp_ShowAdBanner(const FString&, bool);
	AndroidThunkCpp_ShowAdBanner(AdUnitIDs[adID], bShowOnBottomOfScreen);
}

void FAndroidAdvertisingProvider::HideAdBanner() 
{
	extern void AndroidThunkCpp_HideAdBanner();
	AndroidThunkCpp_HideAdBanner();
}

void FAndroidAdvertisingProvider::CloseAdBanner() 
{
	extern void AndroidThunkCpp_CloseAdBanner();
	AndroidThunkCpp_CloseAdBanner();
}

int32 FAndroidAdvertisingProvider::GetAdIDCount()
{
	TArray<FString> AdUnitIDs;
	int32 count = GConfig->GetArray(TEXT("/Script/AndroidRuntimeSettings.AndroidRuntimeSettings"), TEXT("AdMobAdUnitID"), AdUnitIDs, GEngineIni);

	return count;
}