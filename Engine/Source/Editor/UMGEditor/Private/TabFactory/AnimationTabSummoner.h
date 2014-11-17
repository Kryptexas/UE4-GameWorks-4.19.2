// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

struct FAnimationTabSummoner : public FWorkflowTabFactory
{
public:
	static const FName TabID;
	
public:
	FAnimationTabSummoner(TSharedPtr<class FWidgetBlueprintEditor> InBlueprintEditor);
	
	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;
	
protected:
	TWeakPtr<class FWidgetBlueprintEditor> BlueprintEditor;
};
