// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/////////////////////////////////////////////////////
// FWidgetDesignerApplicationMode

class FWidgetDesignerApplicationMode : public FWidgetBlueprintApplicationMode
{
public:
	FWidgetDesignerApplicationMode(TSharedPtr<FWidgetBlueprintEditor> InWidgetEditor);

	// FApplicationMode interface
	virtual void RegisterTabFactories(TSharedPtr<FTabManager> InTabManager) override;
	virtual void PreDeactivateMode() override;
	virtual void PostActivateMode() override;
	// End of FApplicationMode interface
};
