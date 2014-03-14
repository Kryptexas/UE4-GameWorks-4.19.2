// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateRHIRendererPrivatePCH.h"
#include "SlateRHIRenderer.h"
#include "SlateRHIResourceManager.h"
#include "ImageWrapper.h"
#include "SlateRHITextureAtlas.h"

typedef TMap<FName,FSlateTexture2DRHIRef*> FSlateTextureMap;

TSharedPtr<FSlateRHIResourceManager::FDynamicTextureResource> FSlateRHIResourceManager::FDynamicTextureResource::NullResource = MakeShareable( new FDynamicTextureResource( NULL ) );


FSlateRHIResourceManager::FDynamicTextureResource::FDynamicTextureResource( FSlateTexture2DRHIRef* ExistingTexture )
	: TextureObject( NULL )
	, Proxy( new FSlateShaderResourceProxy )
	, RHIRefTexture( ExistingTexture != NULL ? ExistingTexture : new FSlateTexture2DRHIRef(NULL,0,0) )
{
}

FSlateRHIResourceManager::FDynamicTextureResource::~FDynamicTextureResource()
{
	if( Proxy )
	{
		delete Proxy;
	}

	if( RHIRefTexture )
	{
		delete RHIRefTexture;
	}
}

FSlateRHIResourceManager::FSlateRHIResourceManager()
{
	if( GIsEditor )
	{
		AtlasSize = 2048;
	}
	else
	{
		AtlasSize = 1024;
		if( GConfig )
		{
			int32 RequestedSize = 1024;
			GConfig->GetInt( TEXT("SlateRenderer"), TEXT("TextureAtlasSize"), RequestedSize, GEngineIni );
			AtlasSize = FMath::Clamp<uint32>( RequestedSize, 0, 2048 );
		}
	}
}

FSlateRHIResourceManager::~FSlateRHIResourceManager()
{
	DeleteResources();
}

void FSlateRHIResourceManager::CreateTextures( const TArray< const FSlateBrush* >& Resources )
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Loading Slate Textures"), STAT_Slate, STATGROUP_LoadTime);

	TMap<FName,FNewTextureInfo> TextureInfoMap;

	const uint32 Stride = GPixelFormats[PF_R8G8B8A8].BlockBytes;
	for( int32 ResourceIndex = 0; ResourceIndex < Resources.Num(); ++ResourceIndex )
	{
		const FSlateBrush& Brush = *Resources[ResourceIndex];
		const FName TextureName = Brush.GetResourceName();
		if( TextureName != NAME_None && !Brush.HasUObject() && !Brush.IsDynamicallyLoaded() && !ResourceMap.Contains(TextureName) )
		{
			// Find the texture or add it if it doesnt exist (only load the texture once)
			FNewTextureInfo& Info = TextureInfoMap.FindOrAdd( TextureName );
	
			Info.bSrgb = (Brush.ImageType != ESlateBrushImageType::Linear);

			// Only atlas the texture if none of the brushes that use it tile it and the image is srgb
		
			Info.bShouldAtlas &= ( Brush.Tiling == ESlateBrushTileType::NoTile && Info.bSrgb && AtlasSize > 0 );

			// Texture has been loaded if the texture data is valid
			if( !Info.TextureData.IsValid() )
			{
				uint32 Width = 0;
				uint32 Height = 0;
				TArray<uint8> RawData;
				bool bSucceeded = LoadTexture( Brush, Width, Height, RawData );

				Info.TextureData = MakeShareable( new FSlateTextureData( Width, Height, Stride, RawData ) );

				const bool bTooLargeForAtlas = (Width >= 256 || Height >= 256 || Width >= AtlasSize || Height >= AtlasSize );

				Info.bShouldAtlas &= !bTooLargeForAtlas;

				if( !bSucceeded || !ensureMsgf( Info.TextureData->GetRawBytes().Num() > 0, TEXT("Slate resource: (%s) contains no data"), *TextureName.ToString() ) )
				{
					TextureInfoMap.Remove( TextureName );
				}
			}
		}
	}

	// Sort textures by size.  The largest textures are atlased first which creates a more compact atlas
	TextureInfoMap.ValueSort( FCompareFNewTextureInfoByTextureSize() );

	for( TMap<FName,FNewTextureInfo>::TConstIterator It(TextureInfoMap); It; ++It )
	{
		const FNewTextureInfo& Info = It.Value();
		FName TextureName = It.Key();
		FString NameStr = TextureName.ToString();

		checkSlow( TextureName != NAME_None );

		FSlateShaderResourceProxy* NewTexture = GenerateTextureResource( Info );

		ResourceMap.Add( TextureName, NewTexture );
	}
}

bool FSlateRHIResourceManager::LoadTexture( const FSlateBrush& InBrush, uint32& Width, uint32& Height, TArray<uint8>& DecodedImage )
{
	FString ResourcePath = GetResourcePath( InBrush );

	return LoadTexture(InBrush.GetResourceName(), ResourcePath, Width, Height, DecodedImage);
}

/** 
 * Loads a UTexture2D from a package and stores it in the cache
 *
 * @param TextureName	The name of the texture to load
 */
bool FSlateRHIResourceManager::LoadTexture( const FName& TextureName, const FString& ResourcePath, uint32& Width, uint32& Height, TArray<uint8>& DecodedImage )
{
	check( IsThreadSafeForSlateRendering() );

	bool bSucceeded = true;
	uint32 BytesPerPixel = 4;

	TArray<uint8> RawFileData;
	if( FFileHelper::LoadFileToArray( RawFileData, *ResourcePath ) )
	{
		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>( FName("ImageWrapper") );
		IImageWrapperPtr ImageWrapper = ImageWrapperModule.CreateImageWrapper( EImageFormat::PNG );
		if ( ImageWrapper.IsValid() && ImageWrapper->SetCompressed( RawFileData.GetData(), RawFileData.Num() ) )
		{
			Width = ImageWrapper->GetWidth();
			Height = ImageWrapper->GetHeight();
			
			const TArray<uint8>* RawData = NULL;
			if (ImageWrapper->GetRaw( ERGBFormat::BGRA, 8, RawData))
			{
				DecodedImage.AddUninitialized( Width*Height*BytesPerPixel );
				DecodedImage = *RawData;
			}
			else
			{
				UE_LOG(LogSlate, Log, TEXT("Invalid texture format for Slate resource only RGBA and RGB pngs are supported: %s"), *TextureName.ToString() );
				bSucceeded = false;
			}
		}
		else
		{
			UE_LOG(LogSlate, Log, TEXT("Only pngs are supported in Slate"));
			bSucceeded = false;
		}
	}
	else
	{
		UE_LOG(LogSlate, Log, TEXT("Could not find file for Slate resource: %s"), *TextureName.ToString() );
		bSucceeded = false;
	}

	return bSucceeded;
}

FSlateShaderResourceProxy* FSlateRHIResourceManager::GenerateTextureResource( const FNewTextureInfo& Info )
{
	FSlateShaderResourceProxy* NewProxy = NULL;
	const uint32 Width = Info.TextureData->GetWidth();
	const uint32 Height = Info.TextureData->GetHeight();


	if( Info.bShouldAtlas )
	{
		// 4 bytes per pixel
		const uint32 AtlasStride = GPixelFormats[PF_R8G8B8A8].BlockBytes;
		const uint8 Padding = 1;
		const FAtlasedTextureSlot* NewSlot = NULL;
		FSlateTextureAtlasRHI* Atlas = NULL;

		// See if any atlases can hold the texture
		for( int32 AtlasIndex = 0; AtlasIndex < TextureAtlases.Num() && !NewSlot; ++AtlasIndex )
		{
			Atlas = TextureAtlases[AtlasIndex];
			NewSlot = Atlas->AddTexture( Width, Height, Info.TextureData->GetRawBytes() );
		}

		if( !NewSlot )
		{
			INC_DWORD_STAT_BY(STAT_SlateNumTextureAtlases, 1);

			Atlas = new FSlateTextureAtlasRHI( AtlasSize, AtlasSize, AtlasStride, Padding );
			TextureAtlases.Add( Atlas );
			NewSlot = TextureAtlases.Last()->AddTexture( Width, Height, Info.TextureData->GetRawBytes() );
		}
		
		check( Atlas && NewSlot );

		// Create a proxy to the atlased texture. The texure being used is the atlas itself with sub uvs to access the correct texture
		NewProxy = new FSlateShaderResourceProxy;
		NewProxy->Resource = Atlas->GetAtlasTexture();
		NewProxy->StartUV = FVector2D( (float)(NewSlot->X+Padding) / Atlas->GetWidth(), (float)(NewSlot->Y+Padding) / Atlas->GetHeight() );
		NewProxy->SizeUV = FVector2D( (float)(NewSlot->Width-Padding*2) / Atlas->GetWidth(), (float)(NewSlot->Height-Padding*2) / Atlas->GetHeight() );
		NewProxy->ActualSize = FIntPoint( Width, Height );
	}
	else
	{
		NewProxy = new FSlateShaderResourceProxy;

		// Create a new standalone texture because we cant atlas this one
		FSlateTexture2DRHIRef* Texture = new FSlateTexture2DRHIRef( Width, Height, PF_B8G8R8A8, Info.TextureData, Info.bSrgb ? TexCreate_SRGB : TexCreate_None );
		// Add it to the list of non atlased textures that we must clean up later
		NonAtlasedTextures.Add( Texture );

		INC_DWORD_STAT_BY( STAT_SlateNumNonAtlasedTextures, 1 );

		BeginInitResource( Texture );

		// The texture proxy only contains a single texture
		NewProxy->Resource = Texture;
		NewProxy->StartUV = FVector2D(0.0f,0.0f);
		NewProxy->SizeUV = FVector2D(1.0f, 1.0f);
		NewProxy->ActualSize = FIntPoint( Width, Height );
	}

	return NewProxy;
}

void FSlateRHIResourceManager::AddReferencedObjects( FReferenceCollector& Collector )
{
	for( TMap<FName, TSharedPtr<FDynamicTextureResource> >::TIterator It(DynamicTextureMap); It; ++It )
	{
		TSharedPtr<FDynamicTextureResource>& Resource = It.Value();
		if( Resource->TextureObject )
		{
			Collector.AddReferencedObject( Resource->TextureObject );
		}
	}
}

FSlateShaderResourceProxy* FSlateRHIResourceManager::GetTexture( const FSlateBrush& InBrush )
{
	check( IsThreadSafeForSlateRendering() );


	FSlateShaderResourceProxy* Texture = NULL;
	if( !InBrush.IsDynamicallyLoaded() && !InBrush.HasUObject() )
	{
		Texture = ResourceMap.FindRef( InBrush.GetResourceName() );
	}
	else if( InBrush.IsDynamicallyLoaded() || ( InBrush.HasUObject() ) )
	{
		Texture = GetDynamicTextureResource( InBrush );
	}

	return Texture;
}

TSharedPtr<FSlateRHIResourceManager::FDynamicTextureResource> FSlateRHIResourceManager::MakeDynamicTextureResource( FName ResourceName, uint32 Width, uint32 Height, const TArray< uint8 >& Bytes )
{
	// Bail out if we already have this texture loaded
	TSharedPtr<FDynamicTextureResource> TextureResource = DynamicTextureMap.FindRef( ResourceName );
	if( TextureResource.IsValid() )
	{
		return TextureResource;
	}

	// Make storage for the image
	FSlateTextureDataRef TextureStorage = MakeShareable( new FSlateTextureData( Width, Height, GPixelFormats[PF_B8G8R8A8].BlockBytes, Bytes ) );

	// Initialize a texture resource
	TextureResource = InitializeDynamicTextureResource( TextureStorage, NULL );

	// Map the new resource so we don't have to load again
	DynamicTextureMap.Add( ResourceName, TextureResource );

	return TextureResource;
}

TSharedPtr<FSlateRHIResourceManager::FDynamicTextureResource> FSlateRHIResourceManager::MakeDynamicTextureResource(bool bHasUTexture, bool bIsDynamicallyLoaded, FString ResourcePath, FName ResourceName, UTexture2D* InTextureObject)
{
	// Bail out if we already have this texture loaded
	TSharedPtr<FDynamicTextureResource> TextureResource = DynamicTextureMap.FindRef( ResourceName );
	if( TextureResource.IsValid() )
	{
		return TextureResource;
	}

	// Texture object if any
	UTexture2D* TextureObject = NULL;

	// Data for a loaded disk image
	FNewTextureInfo Info;

	bool bSucceeded = false;
	if( bHasUTexture || InTextureObject != NULL )
	{
		if( InTextureObject )
		{
			TextureObject = InTextureObject;
		}
		else
		{
			// Load the utexture
			FString Path = ResourceName.ToString();
			Path = Path.RightChop( FSlateBrush::UTextureIdentifier().Len() );
			TextureObject = LoadObject<UTexture2D>( NULL, *Path, NULL, LOAD_None, NULL );
		}

		bSucceeded = TextureObject != NULL;
	}
	else if( bIsDynamicallyLoaded )
	{
		uint32 Width = 0;
		uint32 Height = 0;
		TArray<uint8> RawData;

		// Load the image from disk
		bSucceeded = LoadTexture( ResourceName, ResourcePath, Width, Height, RawData );

		Info.TextureData = MakeShareable( new FSlateTextureData( Width, Height, GPixelFormats[PF_B8G8R8A8].BlockBytes, RawData ) );
	}

	if( bSucceeded )
	{
		TextureResource = InitializeDynamicTextureResource( Info.TextureData, TextureObject );

		// Map the new resource for the UTexture so we don't have to load again
		DynamicTextureMap.Add( ResourceName, TextureResource );
	}
	else
	{
		// Add the null texture so we don't continuously try to load it.
		DynamicTextureMap.Add( ResourceName, FDynamicTextureResource::NullResource );
	}

	return TextureResource;
}



TSharedRef< FSlateRHIResourceManager::FDynamicTextureResource > FSlateRHIResourceManager::InitializeDynamicTextureResource( const FSlateTextureDataPtr& TextureData, UTexture2D* TextureObject )
{
	TSharedPtr<FDynamicTextureResource> TextureResource = NULL;

	// Get a resource from the free list if possible
	if( DynamicTextureFreeList.Num() > 0 )
	{
		TextureResource = DynamicTextureFreeList.Pop();
	}
	else
	{
		// Free list is empty, we have to allocate a new resource
		TextureResource = MakeShareable( new FDynamicTextureResource( NULL ) );
	}

	checkSlow( !AccessedUTextures.Contains( TextureResource ) );

	// Init game thread data;
	TextureResource->TextureObject = TextureObject;

	if( TextureObject )
	{
		TextureResource->Proxy->ActualSize = FIntPoint( TextureObject->GetSizeX(), TextureObject->GetSizeY() );
	}
	else
	{
		TextureResource->Proxy->ActualSize = FIntPoint( TextureData->GetWidth(), TextureData->GetHeight() );
	}

	// Init render thread data
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER( InitNewSlateDynamicTextureResource,
		FDynamicTextureResource*, TextureResource, TextureResource.Get(),
		FSlateTextureDataPtr, InNewTextureData, TextureData,
	{
		if( TextureResource->TextureObject != NULL )
		{
			TextureResource->RHIRefTexture->SetRHIRef( NULL, 0, 0 );

		}
		else if( InNewTextureData.IsValid() )
		{
			// Set the texture to use as the texture we just loaded
			TextureResource->RHIRefTexture->SetTextureData( InNewTextureData, PF_B8G8R8A8, TexCreate_SRGB );

		}

		TextureResource->Proxy->Resource = TextureResource->RHIRefTexture;

		// Initialize and link the rendering resource
		TextureResource->RHIRefTexture->InitResource();
	})

	return TextureResource.ToSharedRef();
}


FSlateShaderResourceProxy* FSlateRHIResourceManager::GetDynamicTextureResource( const FSlateBrush& InBrush )
{
	check( IsThreadSafeForSlateRendering() );

	const FName ResourceName = InBrush.GetResourceName();
	if ( ResourceName.IsValid() && ResourceName != NAME_None )
	{
		TSharedPtr<FDynamicTextureResource> TextureResource = DynamicTextureMap.FindRef( ResourceName );

		if( !TextureResource.IsValid() )
		{
			TextureResource = MakeDynamicTextureResource(InBrush.HasUObject(), InBrush.IsDynamicallyLoaded(), GetResourcePath(InBrush), ResourceName, Cast<UTexture2D>( InBrush.GetResourceObject() ) );

#if STATS
			if( TextureResource.IsValid() )
			{
				INC_DWORD_STAT_BY( STAT_SlateNumDynamicTextures, 1 );
			}
#endif
		}

		if( TextureResource.IsValid())
		{
			if( TextureResource->TextureObject && !AccessedUTextures.Contains( TextureResource ) && TextureResource->TextureObject->Resource)
			{
				// Set the texture rendering resource that should be used.  The UTexture resource could change at any time so we must do this each frame
				ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER( UpdateSlateUTextureResource,
					FDynamicTextureResource*, DynamicTextureResource, TextureResource.Get(),
					FTexture*, InFTexture, TextureResource->TextureObject->Resource,
				{
					DynamicTextureResource->RHIRefTexture->SetRHIRef( InFTexture->TextureRHI->GetTexture2D(), InFTexture->GetSizeX(), InFTexture->GetSizeY() );
					// Let the streaming manager know we are using this texture now
					InFTexture->LastRenderTime = GCurrentTime;
				});

				AccessedUTextures.Add( TextureResource );
			}

			return TextureResource->Proxy;
		}
	}

	// dynamic texture was not found or loaded
	return  NULL;
}

bool FSlateRHIResourceManager::ContainsTexture( const FName& ResourceName ) const
{
	return ResourceMap.Contains( ResourceName );
}

void FSlateRHIResourceManager::ReleaseDynamicResource( const FSlateBrush& InBrush )
{
	check( IsThreadSafeForSlateRendering() )

	// Note: Only dynamically loaded or utexture brushes can be dynamically released
	if( InBrush.HasUObject() || InBrush.IsDynamicallyLoaded() )
	{
		FName ResourceName = InBrush.GetResourceName();
		TSharedPtr<FDynamicTextureResource> TextureResource = DynamicTextureMap.FindRef(ResourceName);
		if( TextureResource.IsValid() )
		{
			//remove it from the accessed textures
			AccessedUTextures.Remove( TextureResource );

			// Release the rendering resource, its no longer being used
			BeginReleaseResource( TextureResource->RHIRefTexture );

			//remove it from the texture map
			DynamicTextureMap.Remove( ResourceName );

			// Add the resource to the free list so it can be reused
			DynamicTextureFreeList.Add( TextureResource );

			DEC_DWORD_STAT_BY( STAT_SlateNumDynamicTextures, 1 );
		}
	}
}

void FSlateRHIResourceManager::LoadUsedTextures()
{
	TArray< const FSlateBrush* > Resources;
	FSlateStyleRegistry::GetAllResources( Resources );
	 
	CreateTextures( Resources );
}

void FSlateRHIResourceManager::LoadStyleResources( const ISlateStyle& Style )
{
	TArray< const FSlateBrush* > Resources;
	Style.GetResources( Resources );

	CreateTextures( Resources );
}

void FSlateRHIResourceManager::ClearAccessedUTextures()
{
	AccessedUTextures.Empty();
}

void FSlateRHIResourceManager::UpdateTextureAtlases()
{
	for( int32 AtlasIndex = 0; AtlasIndex < TextureAtlases.Num(); ++AtlasIndex )
	{
		TextureAtlases[AtlasIndex]->ConditionalUpdateTexture();
	}
}

void FSlateRHIResourceManager::ReleaseResources()
{
	check( IsThreadSafeForSlateRendering() );

	for( int32 AtlasIndex = 0; AtlasIndex < TextureAtlases.Num(); ++AtlasIndex )
	{
		TextureAtlases[AtlasIndex]->ReleaseAtlasTexture();
	}

	for( int32 ResourceIndex = 0; ResourceIndex < NonAtlasedTextures.Num(); ++ResourceIndex )
	{
		BeginReleaseResource( NonAtlasedTextures[ResourceIndex] );
	}

	
	for( TMap<FName,TSharedPtr<FDynamicTextureResource> >::TIterator It(DynamicTextureMap); It; ++It )
	{
		BeginReleaseResource( It.Value()->RHIRefTexture );
	}

	for( int32 ResourceIndex = 0; ResourceIndex < DynamicTextureFreeList.Num(); ++ResourceIndex )
	{
		BeginReleaseResource( DynamicTextureFreeList[ResourceIndex]->RHIRefTexture );
	}

	// Note the base class has texture proxies only which do not need to be released
}

void FSlateRHIResourceManager::DeleteResources()
{
	for( int32 AtlasIndex = 0; AtlasIndex < TextureAtlases.Num(); ++AtlasIndex )
	{
		delete TextureAtlases[AtlasIndex];
	}

	for( int32 ResourceIndex = 0; ResourceIndex < NonAtlasedTextures.Num(); ++ResourceIndex )
	{
		delete NonAtlasedTextures[ResourceIndex];
	}
	SET_DWORD_STAT(STAT_SlateNumNonAtlasedTextures, 0 );
	SET_DWORD_STAT(STAT_SlateNumTextureAtlases, 0);
	SET_DWORD_STAT(STAT_SlateNumDynamicTextures, 0);

	AccessedUTextures.Empty();
	DynamicTextureMap.Empty();
	DynamicTextureFreeList.Empty();
	TextureAtlases.Empty();
	NonAtlasedTextures.Empty();

	// Clean up mapping to texture
	ClearTextureMap();
}

void FSlateRHIResourceManager::ReloadTextures()
{
	check( IsThreadSafeForSlateRendering() );

	// Release rendering resources
	ReleaseResources();

	// wait for all rendering resources to be released
	FlushRenderingCommands();

	// Delete allocated resources (cpu)
	DeleteResources();

	// Reload everythng
	LoadUsedTextures();
}

uint32 FSlateRHIResourceManager::GetNumTextureAtlases() const
{
	return TextureAtlases.Num();
}

FSlateShaderResource* FSlateRHIResourceManager::GetTextureAtlas( uint32 Index )
{
	return TextureAtlases[Index]->GetAtlasTexture();
}

struct FAtlasPage
{
	FAtlasPage( FSlateShaderResource* InTexture, uint32 InIndex )
		: Texture( InTexture )
		, Index( InIndex )
	{}

	FSlateShaderResource* Texture;
	uint32 Index;
};

class FAtlasVisualizer : public ISlateViewport, public TSharedFromThis<FAtlasVisualizer>
{
public:
	FAtlasVisualizer( FSlateRHIResourceManager& InTextureManager )
		: TextureManager( InTextureManager )
		, SelectedAtlasPage( NULL )
	{}

	virtual FIntPoint GetSize() const OVERRIDE { return FIntPoint(1024,1024); }

	virtual FSlateShaderResource* GetViewportRenderTargetTexture() const OVERRIDE
	{
		if( SelectedAtlasPage.IsValid() )
		{
			return SelectedAtlasPage.Pin()->Texture;
		}
		else
		{
			return NULL;
		}
	}

	virtual bool RequiresVsync() const OVERRIDE { return false; }

	TSharedRef<SWidget> MakeVisualizerWidget()
	{
		bDisplayCheckerboard = false;
		AtlasPages.Empty();

		for( uint32 AtlasIndex = 0; AtlasIndex < TextureManager.GetNumTextureAtlases(); ++AtlasIndex )
		{
			AtlasPages.Add( MakeShareable( new FAtlasPage( TextureManager.GetTextureAtlas(AtlasIndex), AtlasIndex ) ) );
		}

		SelectedAtlasPage = AtlasPages.Num() > 0 ? AtlasPages[0] : NULL;
		TSharedPtr<SViewport> Viewport;

		TSharedRef<SWidget> Widget =
		SNew( SVerticalBox )
		+ SVerticalBox::Slot()
		.Padding(4.0f)
		.HAlign(HAlign_Left)
		.AutoHeight()
		[
			SNew( SHorizontalBox )
			+ SHorizontalBox::Slot()
			.VAlign( VAlign_Center )
			.AutoWidth()
			.Padding(0.0,2.0f,2.0f,2.0f)
			[
				SNew( STextBlock )
				.Text( NSLOCTEXT("TextureManagerVisualizer", "SelectAPageLabel", "Select a page") )
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			.VAlign( VAlign_Center )
			[
				SNew( SComboBox< TSharedPtr<FAtlasPage> > )
				.OptionsSource( &AtlasPages )
				.OnGenerateWidget( this, &FAtlasVisualizer::OnGenerateWidgetForCombo )
				.OnSelectionChanged( this, &FAtlasVisualizer::OnAtlasPageChanged )
				.InitiallySelectedItem( SelectedAtlasPage.IsValid() ? SelectedAtlasPage.Pin() : NULL )
				.Content()
				[
					SNew( STextBlock )
					.Text( this, &FAtlasVisualizer::OnGetSelectedItemText )
				]
			]
			+ SHorizontalBox::Slot()
			.Padding(20.0f,2.0f)
			.AutoWidth()
			.VAlign( VAlign_Center )
			[
				SNew( SCheckBox )
				.OnCheckStateChanged( this, &FAtlasVisualizer::OnDisplayCheckerboardStateChanged )
				.IsChecked( this, &FAtlasVisualizer::OnGetCheckerboardState )
				.Content()
				[
					SNew( STextBlock )
					.Text( NSLOCTEXT("TextureManagerVisualizer", "DisplayCheckerboardCheckboxLabel", "Display Checkerboard") )
				]
			]
		]
		+ SVerticalBox::Slot()
		.Padding(2.0f)
		.AutoHeight()
		[
			SNew( SBox )
			.WidthOverride(1024)
			.HeightOverride(1024)
			[
				SNew( SOverlay )
				+ SOverlay::Slot()
				[
					SNew( SImage )
					.Visibility( this, &FAtlasVisualizer::OnGetCheckerboardVisibility )
					.Image( FCoreStyle::Get().GetBrush("Checkerboard") )
				]
				+ SOverlay::Slot()
				[
					SAssignNew( Viewport, SViewport )
					.IgnoreTextureAlpha(false)
					.EnableBlending(true)
				]
			]
		];

		Viewport->SetViewportInterface( SharedThis( this ) );

		return Widget;

	}

private:

	void OnDisplayCheckerboardStateChanged( ESlateCheckBoxState::Type NewState )
	{
		bDisplayCheckerboard = NewState == ESlateCheckBoxState::Checked;
	}

	ESlateCheckBoxState::Type OnGetCheckerboardState() const
	{
		return bDisplayCheckerboard ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
	}

	EVisibility OnGetCheckerboardVisibility() const
	{
		return bDisplayCheckerboard ? EVisibility::Visible : EVisibility::Collapsed;
	}

	FString OnGetSelectedItemText() const
	{
		if( SelectedAtlasPage.IsValid() )
		{
			return FString::Printf( TEXT("Page %d"), SelectedAtlasPage.Pin()->Index );
		}
		else
		{
			return TEXT("Select a page");
		}
	}

	void OnAtlasPageChanged( TSharedPtr<FAtlasPage> AtlasPage, ESelectInfo::Type SelectionType )
	{
		SelectedAtlasPage = AtlasPage;
	}

	TSharedRef<SWidget> OnGenerateWidgetForCombo( TSharedPtr<FAtlasPage> AtlasPage )
	{
		return 
		SNew( STextBlock )
		.Text( FString::Printf( TEXT("Page %d"), AtlasPage->Index ) );
	}
private:
	TArray< TSharedPtr<FAtlasPage> > AtlasPages;
	FSlateRHIResourceManager& TextureManager;
	TWeakPtr<FAtlasPage> SelectedAtlasPage;
	bool bDisplayCheckerboard;
};



TSharedRef<SWidget> FSlateRHIResourceManager::CreateTextureDisplayWidget()
{
	static TSharedPtr<FAtlasVisualizer> AtlasVisualizer;
	if( !AtlasVisualizer.IsValid() )
	{
		AtlasVisualizer = MakeShareable( new FAtlasVisualizer( *this ) );
	}

	return AtlasVisualizer->MakeVisualizerWidget();
}