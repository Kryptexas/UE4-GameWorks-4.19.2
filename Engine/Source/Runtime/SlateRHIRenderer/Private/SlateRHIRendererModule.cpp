// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateRHIRendererPrivatePCH.h"

class FSlateRHIFontAtlasFactory : public ISlateFontAtlasFactory
{
public:
	FSlateRHIFontAtlasFactory()
	{
		/** Size of each font texture, width and height */
		AtlasSize = 1024;
		if (!GIsEditor && GConfig)
		{
			GConfig->GetInt(TEXT("SlateRenderer"), TEXT("FontAtlasSize"), AtlasSize, GEngineIni);
			AtlasSize = FMath::Clamp(AtlasSize, 0, 2048);
		}
	}

	virtual ~FSlateRHIFontAtlasFactory()
	{
	}


	virtual TSharedRef<FSlateFontAtlas> CreateFontAtlas() const override
	{
		return MakeShareable(new FSlateFontAtlasRHI(AtlasSize, AtlasSize));
	}
private:
	int32 AtlasSize;
};


/**
 * Implements the Slate RHI Renderer module.
 */
class FSlateRHIRendererModule
	: public ISlateRHIRendererModule
{
public:

	// ISlateRHIRendererModule interface
	virtual TSharedRef<FSlateRenderer> CreateSlateRHIRenderer( ) override
	{
		ConditionalCreateResources();

		return MakeShareable( new FSlateRHIRenderer( ResourceManager, FontCache, FontMeasure ) );
	}


	virtual void StartupModule( ) override { }
	virtual void ShutdownModule( ) override { }

private:
	/** Creates resource managers if they do not exist */
	void ConditionalCreateResources()
	{
		if( !ResourceManager.IsValid() )
		{
			ResourceManager = MakeShareable( new FSlateRHIResourceManager );
		}

		if( !FontCache.IsValid() )
		{
			FontCache = MakeShareable(new FSlateFontCache(MakeShareable(new FSlateRHIFontAtlasFactory)));
		}

		if( !FontMeasure.IsValid() )
		{
			FontMeasure = FSlateFontMeasure::Create(FontCache.ToSharedRef());
		}

	}
private:
	/** Resource manager used for all renderers */
	TSharedPtr<FSlateRHIResourceManager> ResourceManager;

	/** Font cache used for all renderers */
	TSharedPtr<FSlateFontCache> FontCache;

	/** Font measure interface used for all renderers */
	TSharedPtr<FSlateFontMeasure> FontMeasure;
};


IMPLEMENT_MODULE( FSlateRHIRendererModule, SlateRHIRenderer ) 
