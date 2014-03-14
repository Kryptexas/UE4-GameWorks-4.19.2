// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ITextureEditorModule.h: Declares the ITextureEditorModule interface.
=============================================================================*/

#pragma once


/**
 * Interface for texture editor modules.
 */
class ITextureEditorModule
	: public IModuleInterface
	, public IHasMenuExtensibility
	, public IHasToolBarExtensibility
{
public:

	/**
	 * Creates a new Texture editor.
	 *
	 * @param Mode - 
	 * @param InitToolkitHost - 
	 * @param Texture - 
	 */
	virtual TSharedRef<ITextureEditorToolkit> CreateTextureEditor( const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UTexture* Texture ) = 0;
};
