// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_ActorBoundEvent.generated.h"

UCLASS(MinimalAPI)
class UK2Node_ActorBoundEvent : public UK2Node_Event
{
	GENERATED_UCLASS_BODY()

	/** Delegate property name that this event is associated with */
	UPROPERTY()
	FName DelegatePropertyName;

	/** The event that this event is bound to */
	UPROPERTY()
	class AActor* EventOwner;


#if WITH_EDITOR
	// Begin UEdGraphNode interface
	virtual void ReconstructNode() OVERRIDE;
	virtual void DestroyNode() OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetDocumentationLink() const OVERRIDE;
	virtual FString GetDocumentationExcerptName() const OVERRIDE;
	// End UEdGraphNode interface

	// Begin K2Node interface
	virtual AActor* GetReferencedLevelActor() const OVERRIDE;
	virtual bool NodeCausesStructuralBlueprintChange() const OVERRIDE { return true; }
	virtual FNodeHandlingFunctor* CreateNodeHandler(FKismetCompilerContext& CompilerContext) const OVERRIDE;
	// End K2Node interface

	virtual bool IsUsedByAuthorityOnlyDelegate() const OVERRIDE;

	/**
	 * Initialized the members of the node, given the specified owner and delegate property.  This will fill out all the required members for the event, such as CustomFunctionName
	 *
	 * @param InEventOwner			The target for this bound event
	 * @param InDelegateProperty	The multicast delegate property associated with the event, which will have a delegate added to it in the level script actor, matching its signature
	 */
	BLUEPRINTGRAPH_API void InitializeActorBoundEventParams(AActor* InEventOwner, const UMulticastDelegateProperty* InDelegateProperty);

	/** Return the delegate property that this event is bound to */
	BLUEPRINTGRAPH_API UMulticastDelegateProperty* GetTargetDelegateProperty();
	/** Const version of GetTargetDelegateProperty that does not search redirect table */
	BLUEPRINTGRAPH_API UMulticastDelegateProperty* GetTargetDelegatePropertyConst() const;

	/** Returns the delegate that this event is bound in */
	BLUEPRINTGRAPH_API FMulticastScriptDelegate* GetTargetDelegate();
#endif
};

