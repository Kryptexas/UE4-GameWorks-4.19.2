// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintGraphDefinitions.h"
#include "K2Node_TutorialExcerptComplete.generated.h"


/** Delegate used to complete a specific excerpt */
DECLARE_MULTICAST_DELEGATE_OneParam(FTutorialExcerptComplete, const FString&);


/** Complete a specified tutorial excerpt */
UCLASS(MinimalAPI)
class UK2Node_TutorialExcerptComplete : public UK2Node_CallFunction
{
	GENERATED_UCLASS_BODY()
		 
	/** Complete a specified tutorial excerpt */
	UFUNCTION(BlueprintCallable, Category="Development|Tutorials", meta=(BlueprintInternalUseOnly = "true"))
	static void TutorialExcerptComplete(const FString& Excerpt);

	UPROPERTY(Transient, NonTransactional)
	FString CachedExcerpt;

	// Begin UK2Node_CallFunction interface
	virtual void PostParameterPinCreated(UEdGraphPin *Pin) OVERRIDE;
	// End UK2Node_CallFunction interface

	// Begin UEdGraphNode interface
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual void PostPlacedNewNode() OVERRIDE;
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const OVERRIDE;
	// End UEdGraphNode interface

	// Begin UK2Node interface
	virtual void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const OVERRIDE;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) OVERRIDE;
	// End UK2Node interface

	// Begin UObject interface
	virtual void PostLoad() OVERRIDE;
	// End UObject interface

	static bool IsExcerptInteractive(const FString& InExcerptName);

public:
	/** Delegate used to complete a specific excerpt */
	static FTutorialExcerptComplete OnTutorialExcerptComplete;

private:
	/** Binds the TutorialExcerptComplete funciton to this node */
	void BindFunction();

private:
	/** Whether the excerpt cache needs rebuilding */
	static bool bExcerptCacheDirty;

	/** Set of all excerpts "excerpt complete" nodes currently use */
	static TSet<FString> AllBlueprintExcerpts;
};