// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EdGraph/EdGraphSchema.h" // for FEdGraphSchemaAction
#include "BlueprintEditor.h"       // for FNodeCreationAnalytic
#include "BlueprintActionMenuItem.generated.h"

// Forward declarations
class UBlueprintNodeSpawner;
class FDragDropOperation;

/**
 * Wrapper around a UBlueprintNodeSpawner, which takes care of specialized
 * node spawning. This class should not be sub-classed, any special handling
 * should be done inside a UBlueprintNodeSpawner subclass, which will be
 * invoked from this class (separated to divide ui and node-spawning).
 */
USTRUCT()
struct FBlueprintActionMenuItem : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY();
	
public:
	static FString StaticGetTypeId() { static FString const TypeId = TEXT("FBlueprintActionMenuItem"); return TypeId; }
	
	/** Constructors */
	FBlueprintActionMenuItem() : Action(nullptr) {}
	FBlueprintActionMenuItem(UBlueprintNodeSpawner* NodeSpawner, int32 MenuGrouping = 0);
	
	// FEdGraphSchemaAction interface
	virtual FString       GetTypeId() const final { return StaticGetTypeId(); }
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D const Location, bool bSelectNewNode = true) final;
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, TArray<UEdGraphPin*>& FromPins, FVector2D const Location, bool bSelectNewNode = true) final;
	virtual void          AddReferencedObjects(FReferenceCollector& Collector) final;
	// End FEdGraphSchemaAction interface

	/**
	 * Attempts to create a specific drag/drop action for this menu entry 
	 * (certain menu entries require unique drag/drop handlers... like property
	 * placeholders, where a drop action spawns a sub-menu for the user to pick
	 * a node type from).
	 * 
	 * @param  AnalyticsDelegate	The analytics callback to assign the drag/drop operation.
	 * @return An empty TSharedPtr if this menu item doesn't require a unique one, else a newly instantiated drag/drop op.
	 */
	TSharedPtr<FDragDropOperation> OnDragged(FNodeCreationAnalytic AnalyticsDelegate) const;

	/** Specialized node-spawner, that comprises the action portion of this menu entry. */
	UBlueprintNodeSpawner* Action;

	/** Brush that should be used for the icon on this menu item. */
	FSlateBrush IconBrush;
};
