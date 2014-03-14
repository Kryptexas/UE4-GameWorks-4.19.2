// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ModuleUIPrivatePCH.h"


/**
 * Implementation of the ModuleUI system
 */
class FModuleUI
	: public IModuleUIInterface
{

public:

	/** IModuleInterface implementation */
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;

	/** IModuleUIInterface implementation */
	virtual TSharedRef< SWidget > GetModuleUIWidget();

private:

};


IMPLEMENT_MODULE( FModuleUI, ModuleUI );


void FModuleUI::StartupModule()
{
}



void FModuleUI::ShutdownModule()
{
}


TSharedRef< SWidget > FModuleUI::GetModuleUIWidget()
{
	return SNew( SModuleUI );
}