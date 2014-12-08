// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AddContentDialogPCH.h"

#include "AssertionMacros.h"
#include "FeaturePackContentSourceProvider.h"
#include "ModuleManager.h"
#include "SDockTab.h"
#include "WidgetCarouselModule.h"

#define LOCTEXT_NAMESPACE "AddContentDialog"

class FAddContentDialogModule : public IAddContentDialogModule
{

public:
	virtual void StartupModule() override
	{
		FModuleManager::LoadModuleChecked<FWidgetCarouselModule>("WidgetCarousel");
		ContentSourceProviderManager = TSharedPtr<FContentSourceProviderManager>(new FContentSourceProviderManager());
		ContentSourceProviderManager->RegisterContentSourceProvider(MakeShareable(new FFeaturePackContentSourceProvider()));
		FAddContentDialogStyle::Initialize();
	}

	virtual void ShutdownModule() override
	{
		FAddContentDialogStyle::Shutdown();
	}

	virtual TSharedRef<FContentSourceProviderManager> GetContentSourceProviderManager() override
	{
		return ContentSourceProviderManager.ToSharedRef();
	}

	virtual TSharedRef<SWindow> CreateDialogWindow() override
	{
		return SNew(SAddContentDialog);
	}

private:
	TSharedPtr<FContentSourceProviderManager> ContentSourceProviderManager;

};

IMPLEMENT_MODULE(FAddContentDialogModule, AddContentDialog);

#undef LOCTEXT_NAMESPACE