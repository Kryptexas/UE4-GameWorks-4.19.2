// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	IOSPlatformSurvey.h: Apple iOS platform hardware-survey classes
==============================================================================================*/

#pragma once

/**
* iOS implementation of FGenericPlatformSurvey
**/
struct FIOSPlatformSurvey : public FGenericPlatformSurvey
{
	static bool GetSurveyResults(FHardwareSurveyResults& OutResults, bool bWait = false);
};

typedef FIOSPlatformSurvey FPlatformSurvey;