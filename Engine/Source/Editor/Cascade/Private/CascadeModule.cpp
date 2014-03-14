// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "CascadeModule.h"
#include "ModuleManager.h"
#include "Cascade.h"

const FName CascadeAppIdentifier = FName(TEXT("CascadeApp"));


/*-----------------------------------------------------------------------------
   FCascadeModule
-----------------------------------------------------------------------------*/

class FCascadeModule : public ICascadeModule
{
public:
	/** Constructor, set up console commands and variables **/
	FCascadeModule()
	{
	}

	/** Called right after the module DLL has been loaded and the module object has been created */
	virtual void StartupModule() OVERRIDE
	{
		MenuExtensibilityManager = MakeShareable(new FExtensibilityManager);
		ToolBarExtensibilityManager = MakeShareable(new FExtensibilityManager);
	}

	/** Called before the module is unloaded, right before the module object is destroyed. */
	virtual void ShutdownModule() OVERRIDE
	{
		MenuExtensibilityManager.Reset();
		ToolBarExtensibilityManager.Reset();
	}

	virtual TSharedRef<ICascade> CreateCascade(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UParticleSystem* ParticleSystem) OVERRIDE
	{
		TSharedRef<FCascade> NewCascade(new FCascade());
		NewCascade->InitCascade(Mode, InitToolkitHost, ParticleSystem);
		CascadeToolkits.Add(&*NewCascade);
		return NewCascade;
	}

	virtual void CascadeClosed(FCascade* CascadeInstance) OVERRIDE
	{
		CascadeToolkits.Remove(CascadeInstance);
	}

	virtual void RefreshCascade(UParticleSystem* ParticleSystem) OVERRIDE
	{
		for (int32 Idx = 0; Idx < CascadeToolkits.Num(); ++Idx)
		{
			if (CascadeToolkits[Idx]->GetParticleSystem() == ParticleSystem)
			{
				CascadeToolkits[Idx]->ForceUpdate();
			}
		}
	}

	virtual void ConvertModulesToSeeded(UParticleSystem* ParticleSystem) OVERRIDE
	{
		FCascade::ConvertAllModulesToSeeded( ParticleSystem );
	}

	/** Gets the extensibility managers for outside entities to extend static mesh editor's menus and toolbars */
	virtual TSharedPtr<FExtensibilityManager> GetMenuExtensibilityManager() {return MenuExtensibilityManager;}
	virtual TSharedPtr<FExtensibilityManager> GetToolBarExtensibilityManager() {return ToolBarExtensibilityManager;}

private:
	TSharedPtr<FExtensibilityManager> MenuExtensibilityManager;
	TSharedPtr<FExtensibilityManager> ToolBarExtensibilityManager;

	/** List of open Cascade toolkits */
	TArray<FCascade*> CascadeToolkits;
};

IMPLEMENT_MODULE(FCascadeModule, Cascade);
