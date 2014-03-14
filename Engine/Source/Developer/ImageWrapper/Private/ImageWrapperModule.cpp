// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ImageWrapperModule.cpp: Implements the FImageWrapperModule class.
=============================================================================*/

#include "ImageWrapperPrivatePCH.h"


DEFINE_LOG_CATEGORY(LogImageWrapper);


/**
 * Image Wrapper module
 */
class FImageWrapperModule
	: public IImageWrapperModule
{
public:

	// Begin IImageWrapperModule interface

	virtual IImageWrapperPtr CreateImageWrapper( const EImageFormat::Type InFormat )
	{
		FImageWrapperBase* ImageWrapper = NULL;

		// Allocate a helper for the format type
		switch(InFormat)
		{
#if WITH_UNREALPNG
		case EImageFormat::PNG:
			ImageWrapper = new FPngImageWrapper();
			break;
#endif	// WITH_UNREALPNG
#if WITH_UNREALJPEG
		case EImageFormat::JPEG:
			ImageWrapper = new FJpegImageWrapper();
			break;
#endif	//WITH_UNREALJPEG
#if WITH_EDITOR
		case EImageFormat::BMP:
			ImageWrapper = new FBmpImageWrapper();
			break;
#endif
		default:
			break;
		}

		return MakeShareable(ImageWrapper);
	}

	// End IImageWrapperModule interface


public:

	// Begin IModuleInterface interface

	virtual void StartupModule( ) OVERRIDE { }

	virtual void ShutdownModule( ) OVERRIDE { }

	// End IModuleInterface interface
};


IMPLEMENT_MODULE(FImageWrapperModule, ImageWrapper);
