// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "IPropertyUtilities.h"

class FPropertyDetailsUtilities : public IPropertyUtilities
{
public:
	FPropertyDetailsUtilities(IDetailsViewPrivate& InDetailsView);
	/** IPropertyUtilities interface */
	virtual class FNotifyHook* GetNotifyHook() const override;
	virtual bool AreFavoritesEnabled() const override;
	virtual void ToggleFavorite( const TSharedRef< class FPropertyEditor >& PropertyEditor ) const override;
	virtual void CreateColorPickerWindow( const TSharedRef< class FPropertyEditor >& PropertyEditor, bool bUseAlpha ) const override;
	virtual void EnqueueDeferredAction( FSimpleDelegate DeferredAction ) override;
	virtual bool IsPropertyEditingEnabled() const override;
	virtual void RequestRefresh() override;
	virtual TSharedPtr<class FAssetThumbnailPool> GetThumbnailPool() const override;
	virtual void NotifyFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent) override;
private:
	IDetailsViewPrivate& DetailsView;
};
