// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/////////////////////////////////////////////////////
// FAnimBlueprintEditAppMode

class FAnimBlueprintEditAppMode : public FBlueprintEditorApplicationMode
{
protected:
	// Set of spawnable tabs in persona mode (@TODO: Multiple lists!)
	FWorkflowAllowedTabSet PersonaTabFactories;

public:
	FAnimBlueprintEditAppMode(TSharedPtr<FPersona> InPersona);

	// FApplicationMode interface
	virtual void RegisterTabFactories(TSharedPtr<FTabManager> InTabManager) OVERRIDE;
	virtual void PostActivateMode() OVERRIDE;
	// End of FApplicationMode interface
};
