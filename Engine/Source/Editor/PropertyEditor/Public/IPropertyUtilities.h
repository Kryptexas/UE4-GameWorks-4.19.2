// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

/**
 * Settings for property editor widgets that call up to the base container for the widgets
 * without knowing information about that container
 */
class IPropertyUtilities
{
public:
	virtual ~IPropertyUtilities(){}
	virtual class FNotifyHook* GetNotifyHook() const = 0;
	virtual bool AreFavoritesEnabled() const = 0;
	virtual void ToggleFavorite( const TSharedRef< class FPropertyEditor >& PropertyEditor ) const = 0;
	virtual void CreateColorPickerWindow( const TSharedRef< class FPropertyEditor >& PropertyEditor, bool bUseAlpha ) const = 0;
	virtual void EnqueueDeferredAction( FSimpleDelegate DeferredAction ) = 0;
	virtual bool IsPropertyEditingEnabled() const = 0;
	virtual void RequestRefresh() = 0;
	virtual TSharedPtr<class FAssetThumbnailPool> GetThumbnailPool() const = 0;
	virtual void NotifyFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent) = 0;
};
