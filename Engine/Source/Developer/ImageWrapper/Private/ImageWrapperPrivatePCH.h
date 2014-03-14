// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ImageWrapperPrivatePCH.h: Pre-compiled header file for the ImageWrapper module.
=============================================================================*/

#pragma once


#include "../Public/ImageWrapper.h"


/* Dependencies
 *****************************************************************************/

#include "Core.h"
#include "ModuleManager.h"


/* Private includes
 *****************************************************************************/

DECLARE_LOG_CATEGORY_EXTERN(LogImageWrapper, Log, All);


#include "ImageWrapperBase.h"

#if WITH_UNREALJPEG
	#include "JpegImageWrapper.h"
#endif

#if WITH_UNREALPNG
	#include "PngImageWrapper.h"
#endif

#include "BmpImageWrapper.h"
