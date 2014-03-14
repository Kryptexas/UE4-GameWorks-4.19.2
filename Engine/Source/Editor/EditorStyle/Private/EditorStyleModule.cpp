// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	EditorStyleModule.cpp: Implements the FEditorStyleModule class.
=============================================================================*/

#include "EditorStylePrivatePCH.h"
#include "EditorStyle.generated.inl"


/**
 * Implements the Editor style module, loaded by SlateApplication dynamically at startup.
 */
class FEditorStyleModule
	: public IEditorStyleModule
{
public:

	// Begin IModuleInterface interface

	virtual void StartupModule( )
	{
		FSlateEditorStyle::Initialize();
	}

	virtual void ShutdownModule( )
	{
		FSlateEditorStyle::Shutdown();
	}

	virtual TSharedRef<class FSlateStyleSet> CreateEditorStyleInstance( ) const OVERRIDE
	{
		return FSlateEditorStyle::Create(FSlateEditorStyle::Settings);
	}

	// End IModuleInterface interface
};


IMPLEMENT_MODULE(FEditorStyleModule, EditorStyle)
