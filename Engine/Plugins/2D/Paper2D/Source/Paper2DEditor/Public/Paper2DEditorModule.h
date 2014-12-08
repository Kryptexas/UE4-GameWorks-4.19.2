// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#define PAPER2D_EDITOR_MODULE_NAME "Paper2DEditor"

//////////////////////////////////////////////////////////////////////////
// IPaper2DEditorModule

class IPaper2DEditorModule : public IModuleInterface
{
public:
	virtual TSharedPtr<class FExtensibilityManager> GetSpriteEditorMenuExtensibilityManager() { return NULL; }
	virtual TSharedPtr<class FExtensibilityManager> GetSpriteEditorToolBarExtensibilityManager() { return NULL; }

	virtual TSharedPtr<class FExtensibilityManager> GetFlipbookEditorMenuExtensibilityManager() { return NULL; }
	virtual TSharedPtr<class FExtensibilityManager> GetFlipbookEditorToolBarExtensibilityManager() { return NULL; }
};

