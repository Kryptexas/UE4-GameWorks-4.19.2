// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

struct FDetailsTabSummoner : public FWorkflowTabFactory
{
public:
	static const FName TabID;
	
public:
	FDetailsTabSummoner(TSharedPtr<class FWidgetBlueprintEditor> InBlueprintEditor);
	
	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;
	
protected:
	TWeakPtr<class FWidgetBlueprintEditor> BlueprintEditor;
};
