// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "IPropertyUtilities.h"

class FPropertyDetailsUtilities : public IPropertyUtilities
{
public:
	FPropertyDetailsUtilities( SDetailsView& InDetailsView );
	/** IPropertyUtilities interface */
	virtual class FNotifyHook* GetNotifyHook() const OVERRIDE;
	virtual bool AreFavoritesEnabled() const OVERRIDE;
	virtual void ToggleFavorite( const TSharedRef< class FPropertyEditor >& PropertyEditor ) const OVERRIDE;
	virtual void CreateColorPickerWindow( const TSharedRef< class FPropertyEditor >& PropertyEditor, bool bUseAlpha ) const OVERRIDE;
	virtual void EnqueueDeferredAction( FSimpleDelegate DeferredAction ) OVERRIDE;
	virtual bool IsPropertyEditingEnabled() const OVERRIDE;
	virtual void RequestRefresh() OVERRIDE;
	virtual TSharedPtr<class FAssetThumbnailPool> GetThumbnailPool() const OVERRIDE;
	virtual void NotifyFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
private:
	SDetailsView& DetailsView;
};
