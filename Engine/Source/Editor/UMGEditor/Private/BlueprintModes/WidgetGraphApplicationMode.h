// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/////////////////////////////////////////////////////
// FWidgetGraphApplicationMode

class FWidgetGraphApplicationMode : public FWidgetBlueprintApplicationMode
{
public:
	FWidgetGraphApplicationMode(TSharedPtr<FWidgetBlueprintEditor> InWidgetEditor);

	// FApplicationMode interface
	virtual void RegisterTabFactories(TSharedPtr<FTabManager> InTabManager) override;
	virtual void PostActivateMode() override;
	// End of FApplicationMode interface
};
