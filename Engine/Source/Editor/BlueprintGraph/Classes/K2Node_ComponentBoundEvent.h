// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_ComponentBoundEvent.generated.h"

UCLASS(MinimalAPI)
class UK2Node_ComponentBoundEvent : public UK2Node_Event
{
	GENERATED_UCLASS_BODY()

	/** Delegate property name that this event is associated with */
	UPROPERTY()
	FName DelegatePropertyName;

	/** Name of property in Blueprint class that pointer to component we want to bind to */
	UPROPERTY()
	FName ComponentPropertyName;

#if WITH_EDITOR
	// Begin UEdGraphNode interface
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetDocumentationLink() const OVERRIDE;
	virtual FString GetDocumentationExcerptName() const OVERRIDE;
	// End UEdGraphNode interface

	// Begin K2Node interface
	virtual bool NodeCausesStructuralBlueprintChange() const OVERRIDE { return true; }
	virtual UClass* GetDynamicBindingClass() const OVERRIDE;
	virtual void RegisterDynamicBinding(UDynamicBlueprintBinding* BindingObject) const OVERRIDE;
	// End K2Node interface

	virtual bool IsUsedByAuthorityOnlyDelegate() const OVERRIDE;

	/** Return the delegate property that this event is bound to */
	BLUEPRINTGRAPH_API UMulticastDelegateProperty* GetTargetDelegateProperty() const;

	BLUEPRINTGRAPH_API void InitializeComponentBoundEventParams(UObjectProperty* InComponentProperty, const UMulticastDelegateProperty* InDelegateProperty);
#endif
};

