// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once


/**
 * Stores a mapping of texture names to their RHI texture resource               
 */
class FSlateRHIResourceManager : public FSlateShaderResourceManager, public FGCObject
{
public:
	/** Represents a dynamic texture resource for rendering */
	struct FDynamicTextureResource
	{
		class UTexture2D* TextureObject;
		FSlateShaderResourceProxy* Proxy;
		FSlateTexture2DRHIRef* RHIRefTexture;

		static TSharedPtr<FDynamicTextureResource> NullResource;

		FDynamicTextureResource( FSlateTexture2DRHIRef* ExistingTexture );
		~FDynamicTextureResource();
	};

public:
	FSlateRHIResourceManager();
	virtual ~FSlateRHIResourceManager();

	/**
	 * Loads and creates rendering resources for all used textures.  
	 * In this implementation all textures must be known at startup time or they will not be found
	 */
	void LoadUsedTextures();

	void LoadStyleResources( const ISlateStyle& Style );

	/**
	 * Clears accessed UTexture resources from the previous frame
	 * The accessed textures is used to determine which textures need be updated on the render thread
	 * so they can be used by slate
	 */
	void ClearAccessedUTextures();

	/**
	 * Updates texture atlases if needed
	 */
	void UpdateTextureAtlases();

	/**
	 * Returns a texture associated with the passed in name. 
	 */
	virtual FSlateShaderResourceProxy* GetTexture( const FSlateBrush& InBrush );
	
	/**
	 * Makes a dynamic texture resource and begins use of it
	 *
	 * @param bHasUTexture			Whether we have a UTexture
	 * @param bIsDynamicallyLoaded	True if resource is to be dynamically loaded
	 * @param ResourcePath			The resource's image path
	 * @param ResourceName			The name identifier of the resource
	 * @param InTextureObject		The utexture object, if applicable
	 * @return						The dynamic texture resource created
	 */
	TSharedPtr<FDynamicTextureResource> MakeDynamicTextureResource(bool bHasUTexture, bool bIsDynamicallyLoaded, FString ResourcePath, FName ResourceName, UTexture2D* InTextureObject);
	
	/**
	 * Makes a dynamic texture resource and begins use of it
	 *
	 * @param ResourceName			The name identifier of the resource
	 * @param Width					The width of the resource
	 * @param Height				The height of the resource
	 * @param Bytes					The payload containing the resource
	 */
	TSharedPtr<FSlateRHIResourceManager::FDynamicTextureResource> MakeDynamicTextureResource( FName ResourceName, uint32 Width, uint32 Height, const TArray< uint8 >& Bytes );

	/**
	 * Returns true if a texture resource with the passed in resource name is available 
	 */
	bool ContainsTexture( const FName& ResourceName ) const;

	/** Releases a specific dynamic resource */
	void ReleaseDynamicResource( const FSlateBrush& InBrush );

	/**
	 * Releases rendering resources
	 */
	void ReleaseResources();

	/**
	 * Reloads texture resources for all used textures.  
	 *
	 * @param InExtraResources     Optional list of textures to create that aren't in the style.
	 */
	void ReloadTextures();

	/** @return The number of texture atlases in the manager */
	uint32 GetNumTextureAtlases() const;

	/** @return The atlas texture at a given index */
	FSlateShaderResource* GetTextureAtlas( uint32 Index );

	/**
	 * Creates an atlas visualizer widget
	 */
	TSharedRef<SWidget> CreateTextureDisplayWidget();
private:
	
	FSlateShaderResourceProxy* AddTexture( const FSlateBrush& InBrush );

	/**
	 * Deletes resources created by the manager
	 */
	void DeleteResources();

	/** 
	 * Creates textures from files on disk and atlases them if possible
	 *
	 * @param Resources	The brush resources to load images for
	 */
	void CreateTextures( const TArray< const FSlateBrush* >& Resources );
	
	/** 
	 * Creates a new texture from the given texture name
	 *
	 * @param TextureName	The name of the texture to load
	 */
	virtual bool LoadTexture( const FName& TextureName, const FString& ResourcePath, uint32& Width, uint32& Height, TArray<uint8>& DecodedImage );
	virtual bool LoadTexture( const FSlateBrush& InBrush, uint32& Width, uint32& Height, TArray<uint8>& DecodedImage );

	/** 
	 * Prepares a dynamic texture resource for use based on the provided inputs
	 */
	TSharedRef< FSlateRHIResourceManager::FDynamicTextureResource > InitializeDynamicTextureResource( const FSlateTextureDataPtr& TextureStorage, UTexture2D* TextureObject );

	/**
	 * Generates rendering resources for a texture
	 * 
	 * @param Info	Information on how to generate the texture
	 */
	FSlateShaderResourceProxy* GenerateTextureResource( const FNewTextureInfo& Info );
	
	/**
	 * Returns a texture rendering resource from for a dynamically loaded texture or utexture object
	 * Note: this will load the UTexture or image if needed 
	 *
	 * @param InBrush	Slate brush for the dynamic resource
	 */
	FSlateShaderResourceProxy* GetDynamicTextureResource( const FSlateBrush& InBrush );

	/** FGCObject */
	void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE;

private:
	/** Map of all active dynamic texture objects being used for brush resources */
	TMap<FName, TSharedPtr<FDynamicTextureResource> > DynamicTextureMap;
	/** Set of dynamic textures that are currently being accessed */
	TSet< TSharedPtr<FDynamicTextureResource> > AccessedUTextures;
	/** List of old dynamic resources that are free to use as new resources */
	TArray< TSharedPtr<FDynamicTextureResource> > DynamicTextureFreeList;
	/** Static Texture atlases which have been created */
	TArray<class FSlateTextureAtlasRHI*> TextureAtlases;
	/** Static Textures created that are not atlased */	
	TArray<class FSlateTexture2DRHIRef*> NonAtlasedTextures;
	/** The size of each texture atlas (square) */
	uint32 AtlasSize;

};

