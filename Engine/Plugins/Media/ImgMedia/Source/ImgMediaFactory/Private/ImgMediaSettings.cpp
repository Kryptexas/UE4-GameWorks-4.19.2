// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ImgMediaSettings.h"


/* UImgMediaSettings structors
 *****************************************************************************/

UImgMediaSettings::UImgMediaSettings()
	: DefaultFps(24.0f)
	, CacheBehindPercentage(25)
	, CacheSizeGB(1.0f)
	, CacheThreads(8)
	, CacheThreadStackSizeKB(128)
	, ExrDecoderThreads(0)
	, DefaultProxy(TEXT("proxy"))
	, UseDefaultProxy(false)
{ }
