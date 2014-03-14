// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AutomationControllerModule.h: Declares the FAutomationControllerModule class.
=============================================================================*/

#pragma once


/**
 * Implements the AutomationController module.
 */
class FAutomationControllerModule
	: public IAutomationControllerModule
{
public:

	// Begin IAutomationControllerModule Interface

	virtual IAutomationControllerManagerRef GetAutomationController( ) OVERRIDE;

	virtual void Init() OVERRIDE;

	virtual void Tick() OVERRIDE;

	// End IAutomationControllerModule Interface


public:

	// Begin IModuleInterface interface

	virtual void StartupModule() OVERRIDE;

	virtual void ShutdownModule() OVERRIDE;

	virtual bool SupportsDynamicReloading() OVERRIDE;

	// End IModuleInterface interface


private:

	// Holds the automation controller singleton.
	static IAutomationControllerManagerPtr AutomationControllerSingleton;
};
