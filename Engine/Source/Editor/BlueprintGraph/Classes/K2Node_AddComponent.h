// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_AddComponent.generated.h"

UCLASS(MinimalAPI)
class UK2Node_AddComponent : public UK2Node_CallFunction
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	uint32 bHasExposedVariable:1;

	/** The blueprint name we came from, so we can lookup the template after a paste */
	UPROPERTY()
	FString TemplateBlueprint;

#if WITH_EDITOR
	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual void DestroyNode() OVERRIDE;
	virtual void PrepareForCopying() OVERRIDE;
	virtual void PostPasteNode() OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FString GetDocumentationLink() const OVERRIDE;
	virtual FString GetDocumentationExcerptName() const OVERRIDE;
	// End UEdGraphNode interface

	// Begin K2Node interface
	virtual UActorComponent* GetTemplateFromNode() const OVERRIDE;
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const OVERRIDE;
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) OVERRIDE;
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) OVERRIDE;
	// End of UK2Node interface

	BLUEPRINTGRAPH_API void AllocateDefaultPinsWithoutExposedVariables();
	BLUEPRINTGRAPH_API void AllocatePinsForExposedVariables();

	UEdGraphPin* GetTemplateNamePinChecked() const
	{
		UEdGraphPin* FoundPin = GetTemplateNamePin();
		check(FoundPin != NULL);
		return FoundPin;
	}

	UEdGraphPin* GetRelativeTransformPin() const
	{
		return FindPinChecked(TEXT("RelativeTransform"));
	}

	UEdGraphPin* GetManualAttachmentPin() const
	{
		return FindPinChecked(TEXT("bManualAttachment"));
	}

	/** Static name of function to call */
	static FName GetAddComponentFunctionName()
	{
		static const FName AddComponentFunctionName(GET_FUNCTION_NAME_CHECKED(AActor, AddComponent));
		return AddComponentFunctionName;
	}

private: 
	UEdGraphPin* GetTemplateNamePin() const
	{
		return FindPin(TEXT("TemplateName"));
	}
#endif
};



