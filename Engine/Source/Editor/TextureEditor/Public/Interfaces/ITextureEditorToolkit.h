// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ITextureEditorToolkit.h: Declares the ITextureEditorToolkit interface.
=============================================================================*/

#pragma once

#include "Toolkits/IToolkitHost.h"


/*-----------------------------------------------------------------------------
   ITextureEditor
-----------------------------------------------------------------------------*/

class ITextureEditorToolkit
	: public FAssetEditorToolkit
{
public:

	/** Returns the Texture asset being inspected by the Texture editor */
	virtual UTexture* GetTexture() const = 0;

	/** Refreshes the quick info panel */
	virtual void PopulateQuickInfo() = 0;

	/** Calculates the display size of the texture */
	virtual void CalculateTextureDimensions(uint32& Width, uint32& Height) const = 0;

	/** Accessors */
	virtual int32 GetMipLevel() const = 0;
	virtual ESimpleElementBlendMode GetColourChannelBlendMode() const = 0;
	virtual bool GetUseSpecifiedMip() const = 0;
	virtual double GetZoom() const = 0;
	virtual void SetZoom( double ZoomValue ) = 0;
	virtual void ZoomIn() = 0;
	virtual void ZoomOut() = 0;
};
