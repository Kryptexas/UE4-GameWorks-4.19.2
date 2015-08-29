// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class IChatTabViewModel
{
public:

	// IChatTabViewModel Interface
	virtual bool IsTabVisible() const = 0;
	virtual const FText GetTabText() const = 0;

	/**
	 * Provides an ID Used in Chat Window navigation.
	 *
	 * @returns an uppercase version of the tab ID.
	 */	
	virtual const EChatMessageType::Type GetTabID() const = 0;

	virtual const FSlateBrush* GetTabImage() const = 0;
	virtual const TSharedPtr<FChatViewModel> GetChatViewModel() const = 0;

	DECLARE_EVENT(IChatTabViewModel, FChatTabVisibilityChangedEvent)
	virtual FChatTabVisibilityChangedEvent& OnTabVisibilityChanged() = 0;
};
