// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	WinRTSurvey.h: WinRT platform hardware-survey classes
==============================================================================================*/

#pragma once

/**
* WinRT implementation of FGenericPlatformSurvey
**/
struct FWinRTPlatformSurvey : public FGenericPlatformSurvey
{
	// default implementation for now
};

typedef FWinRTPlatformSurvey FPlatformSurvey;