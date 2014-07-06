// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

struct FDesignerTabSummoner : public FWorkflowTabFactory
{
public:
	static const FName TabID;
	
public:
	FDesignerTabSummoner(TSharedPtr<class FWidgetBlueprintEditor> InBlueprintEditor);
	
	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;
	
protected:
	TWeakPtr<class FWidgetBlueprintEditor> BlueprintEditor;
};
