// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UnrealEd.h"
#include "ModuleInterface.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Toolkits/IToolkit.h"
#include "IUMGEditor.h"

extern const FName UMGEditorAppIdentifier;

class FUMGEditor;

class IUMGEditorModule : public IModuleInterface, public IHasMenuExtensibility, public IHasToolBarExtensibility
{
public:
	/** Creates a new UMGEditor instance */
	virtual TSharedRef<IUMGEditor> CreateUMGEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UParticleSystem* ParticleSystem) = 0;

	/** Removes the specified instance from the list of open UMGEditor toolkits */
	virtual void UMGEditorClosed(FUMGEditor* UMGEditorInstance) = 0;

	/** Refreshes the toolkit inspecting the specified particle system */
	virtual void RefreshUMGEditor(UParticleSystem* ParticleSystem) = 0;

	/** Converts all the modules in the specified particle system to seeded modules */
	virtual void ConvertModulesToSeeded(UParticleSystem* ParticleSystem) = 0;
};
