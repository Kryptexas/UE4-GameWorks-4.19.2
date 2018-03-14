// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.


#pragma once

#if PLATFORM_WINDOWS
#include "Windows/VulkanWindowsPlatform.h"
#elif PLATFORM_ANDROID
#include "Android/VulkanAndroidPlatform.h"
#elif PLATFORM_LINUX
#include "Linux/VulkanLinuxPlatform.h"
#endif

