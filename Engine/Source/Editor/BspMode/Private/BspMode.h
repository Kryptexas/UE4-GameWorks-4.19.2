// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

/**
 * Mode Toolkit for the Bsp
 */
class FBspMode : public FEdMode
{
public:
	static TSharedRef< FBspMode > Create();

	~FBspMode();
	virtual bool UsesToolkits() const override;

	virtual void Enter() override;
	virtual void Exit() override;

	virtual bool UsesPropertyWidgets() const override { return true; }

private:
	FBspMode();
	void Initialize();

	void CreateBspModeItems( FToolBarBuilder& Builder );

	void EditorModeChanged( FEdMode* Mode, bool IsEntering );

	void HandleModulesChanged( FName ModuleThatChanged, EModuleChangeReason::Type ReasonForChange );

private:

	bool IsActive;
	bool bNeedsToRegisterCommands;
	FName LastActiveModeIdentifier;
};