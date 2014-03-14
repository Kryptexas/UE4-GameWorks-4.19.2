// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleInterface.h"

class FDetailCustomizationsModule : public IModuleInterface
{
public:
	/**
	 * Called right after the module DLL has been loaded and the module object has been created
	 */
	virtual void StartupModule();

	/**
	 * Called before the module is unloaded, right before the module object is destroyed.
	 */
	virtual void ShutdownModule();

	/**
	 * Override this to set whether your module is allowed to be unloaded on the fly
	 *
	 * @return	Whether the module supports shutdown separate from the rest of the engine.
	 */
	virtual bool SupportsDynamicReloading() OVERRIDE
	{
		return true;
	}

private:
	/**
	 * Registers a custom class
	 *
	 * @param Class			The class to register for property customization
	 * @param DetailLayoutDelegate	The delegate to call to get the custom detail layout instance
	 */
	void RegisterCustomPropertyLayout( UClass* Class, FOnGetDetailCustomizationInstance DetailLayoutDelegate );

	/**
	* Registers a custom struct
	*
	* @param StructName				The name of the struct to register for property customization
	* @param StructLayoutDelegate	The delegate to call to get the custom detail layout instance
	*/
	void RegisterStructPropertyLayout(FName StructTypeName, FOnGetStructCustomizationInstance StructLayoutDelegate);
private:
	/** List of registered class that we must unregister when the module shuts down */
	TSet< TWeakObjectPtr<UClass> >	RegisteredClasses;
	TSet< FName >					RegisteredStructs;
};