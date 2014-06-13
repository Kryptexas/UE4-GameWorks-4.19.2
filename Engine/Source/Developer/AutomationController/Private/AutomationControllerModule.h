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

	virtual IAutomationControllerManagerRef GetAutomationController( ) override;

	virtual void Init() override;

	virtual void Tick() override;

	// End IAutomationControllerModule Interface


public:

	// Begin IModuleInterface interface

	virtual void StartupModule() override;

	virtual void ShutdownModule() override;

	virtual bool SupportsDynamicReloading() override;

	// End IModuleInterface interface


private:

	// Holds the automation controller singleton.
	static IAutomationControllerManagerPtr AutomationControllerSingleton;
};
