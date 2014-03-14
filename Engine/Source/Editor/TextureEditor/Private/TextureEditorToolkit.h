// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	TextureEditorToolkit.h: Declares the FTextureEditorToolkit class.
=============================================================================*/

#pragma once

#include "Toolkits/AssetEditorToolkit.h"


/**
 * Implements an Editor toolkit for textures.
 */
class FTextureEditorToolkit
	: public ITextureEditorToolkit
	, public FEditorUndoClient
	, public FGCObject
{
public:

	/**
	 * Destructor.
	 */
	virtual ~FTextureEditorToolkit( );

public:

	/**
	 * Edits the specified Texture object.
	 *
	 * @param Mode -
	 * @param InitToolkitHost -
	 * @param ObjectToEdit -
	 */
	void InitTextureEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UObject* ObjectToEdit );

public:

	// Begin FAssetEditorToolkit interface

	virtual void RegisterTabSpawners( const TSharedRef<class FTabManager>& TabManager ) OVERRIDE;
	virtual void UnregisterTabSpawners( const TSharedRef<class FTabManager>& TabManager ) OVERRIDE;

	virtual FString GetDocumentationLink() const OVERRIDE
	{
		return FString(TEXT("Engine/Content/Types/Textures/Properties/Interface"));
	}

	// End FAssetEditorToolkit interface

public:

	// Begin ITextureEditorToolkit interface

	virtual void CalculateTextureDimensions( uint32& Width, uint32& Height ) const OVERRIDE;

	virtual ESimpleElementBlendMode GetColourChannelBlendMode( ) const OVERRIDE;

	virtual int32 GetMipLevel( ) const OVERRIDE
	{
		return GetUseSpecifiedMip() ? SpecifiedMipLevel : 0;
	}

	virtual UTexture* GetTexture( ) const OVERRIDE
	{
		return Texture;
	}

	virtual void PopulateQuickInfo( ) OVERRIDE;
	
	// End ITextureEditorToolkit interface

public:

	// Begin IToolkit interface

	virtual FText GetBaseToolkitName( ) const OVERRIDE;

	virtual FName GetToolkitFName( ) const OVERRIDE
	{
		return FName("TextureEditor");
	}

	virtual FLinearColor GetWorldCentricTabColorScale( ) const OVERRIDE
	{
		return FLinearColor(0.3f, 0.2f, 0.5f, 0.5f);
	}

	virtual bool GetUseSpecifiedMip( ) const OVERRIDE;

	virtual FString GetWorldCentricTabPrefix( ) const OVERRIDE;

	virtual double GetZoom( ) const OVERRIDE;
	virtual void SetZoom( double ZoomValue ) OVERRIDE;
	virtual void ZoomIn( ) OVERRIDE;
	virtual void ZoomOut( ) OVERRIDE;

	// End IToolkit interface

public:

	// Begin FGCObject interface

	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE;

	// End FGCObject interface
	
protected:

	// Begin FEditorUndoClient interface

	virtual void PostUndo(bool bSuccess) OVERRIDE { }

	virtual void PostRedo(bool bSuccess) OVERRIDE
	{
		PostUndo(bSuccess);
	}
	
	// End FEditorUndoClient interface

protected:

	/**
	 * Checks whether the texture should be filling the view port.
	 *
	 * @return true if the view port is filled, false otherwise.
	 */
	bool IsFillingViewport( )
	{
		return FMath::IsNearlyZero(Zoom);
	}

private:

	TSharedRef<SDockTab> SpawnTab_Viewport(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Properties(const FSpawnTabArgs& Args);

	/** Creates the texture properties details widget */
	TSharedRef<SWidget> BuildTexturePropertiesWidget();

	/** Creates all internal widgets for the tabs to point at */
	void CreateInternalWidgets();

	/** Builds the toolbar widget for the Texture editor */
	void ExtendToolBar();
	
	/**	Binds our UI commands to delegates */
	void BindCommands();

	/**	Finds the effective in-game resolution of the texture */
	void CalculateEffectiveTextureDimensions(int32 LODBias, uint32& EffectiveTextureWidth, uint32& EffectiveTextureHeight);
	
	/** Toolbar command methods */
	void OnRedChannel();
	bool IsRedChannelChecked() const;
	void OnGreenChannel();
	bool IsGreenChannelChecked() const;
	void OnBlueChannel();
	bool IsBlueChannelChecked() const;
	void OnAlphaChannel();
	bool OnAlphaChannelCanExecute() const;
	bool IsAlphaChannelChecked() const;
	void OnSaturation();
	bool IsSaturationChecked() const;
	void OnFillViewport();
	bool IsFillViewportChecked() const;
	void HandleBackgroundActionExecute( ETextureEditorBackgrounds Background );
	bool HandleBackgroundActionIsChecked( ETextureEditorBackgrounds Background );
	void OnShowBorder();
	bool IsShowBorderChecked() const;
	void OnPreReimport(UObject* InObject);
	void OnPostReimport(UObject* InObject, bool bSuccess);
	void OnCompressNow();
	bool OnCompressNowEnabled() const;
	void OnReimport();
	void HandleSettingsActionExecute();
	bool IsCubeTexture() const;

	/** Methods for the mip level UI control */
	TOptional<int32> GetWidgetMipLevel() const;
	void OnMipLevelChanged(int32 NewMipLevel);
	FReply OnMipLevelDown();
	FReply OnMipLevelUp();

	TOptional<int32> GetMaxMipLevel()const;

	ESlateCheckBoxState::Type OnGetUseSpecifiedMip()const;
	bool OnGetUseSpecifiedMipEnabled()const;
	void OnUseSpecifiedMipChanged(ESlateCheckBoxState::Type InNewState);

private:

	/** The Texture asset being inspected */
	UTexture* Texture;

	/** List of open tool panels; used to ensure only one exists at any one time */
	TMap< FName, TWeakPtr<SDockableTab> > SpawnedToolPanels;

	/** Viewport */
	TSharedPtr<STextureEditorViewport> TextureViewport;

	/** Properties tab */
	TSharedPtr<SVerticalBox> TextureProperties;

	/** Properties tree view */
	TSharedPtr<class IDetailsView> TexturePropertiesWidget;

	/** Quick info text blocks */
	TSharedPtr<STextBlock> ImportedText;
	TSharedPtr<STextBlock> CurrentText;
	TSharedPtr<STextBlock> MaxInGameText;
	TSharedPtr<STextBlock> MethodText;
	TSharedPtr<STextBlock> FormatText;
	TSharedPtr<STextBlock> LODBiasText;

	/** If true, displays the red channel */
	bool bIsRedChannel;

	/** If true, displays the green channel */
	bool bIsGreenChannel;

	/** If true, displays the blue channel */
	bool bIsBlueChannel;

	/** If true, displays the alpha channel */
	bool bIsAlphaChannel;

	/** If true, desaturates the texture */
	bool bIsSaturation;

	/** The maximum width/height at which the texture will render in the preview window */
	uint32 PreviewEffectiveTextureWidth;
	uint32 PreviewEffectiveTextureHeight;

	/** Which mip level should be shown */
	int32 SpecifiedMipLevel;
	/* When true, the specified mip value is used. Top mip is used when false.*/
	bool bUseSpecifiedMipLevel;

	/** During re-import, cache this setting so it can be restored if necessary */
	bool SavedCompressionSetting;

	/** The texture's zoom factor. */
	double Zoom;

private:

	/**	The tab ids for all the tabs used */
	static const FName ViewportTabId;
	static const FName PropertiesTabId;
};
