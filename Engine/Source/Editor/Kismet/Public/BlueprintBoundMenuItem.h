// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EdGraph/EdGraphSchema.h" // for FEdGraphSchemaAction
#include "SlateColor.h"
#include "BlueprintNodeBinder.h" // for IBlueprintNodeBinder::FBindingSet
#include "BlueprintBoundMenuItem.generated.h"

// Forward declarations
class UBlueprintNodeSpawner;
class FDragDropOperation;
struct FSlateBrush;

/**
 * Wrapper around a UBlueprintNodeSpawner, which takes care of specialized
 * node spawning. This class should not be sub-classed, any special handling
 * should be done inside a UBlueprintNodeSpawner subclass, which will be
 * invoked from this class (separated to divide ui and node-spawning).
 */
USTRUCT()
struct FBlueprintBoundMenuItem : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY();
	
public:
	static FName StaticGetTypeId() { static FName const TypeId("FBlueprintBoundMenuItem"); return TypeId; }
	
	/** Constructors */
	FBlueprintBoundMenuItem() : BoundSpawner(nullptr) {}
	FBlueprintBoundMenuItem(UBlueprintNodeSpawner const* BoundSpawner, int32 MenuGrouping = 0);
	
	// FEdGraphSchemaAction interface
	virtual FName         GetTypeId() const final { return StaticGetTypeId(); }
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D const Location, bool bSelectNewNode = true) final;
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, TArray<UEdGraphPin*>& FromPins, FVector2D const Location, bool bSelectNewNode = true) final;
	virtual void          AddReferencedObjects(FReferenceCollector& Collector) final;
	// End FEdGraphSchemaAction interface

	/**
	 * 
	 * 
	 * @param  BindingSet	
	 */
	void AddBindings(IBlueprintNodeBinder::FBindingSet const& BindingSet);

	/**
	 * Retrieves the icon brush for this menu entry (to be displayed alongside
	 * in the menu).
	 *
	 * @param  ColorOut	An output parameter that's filled in with the color to tint the brush with.
	 * @return An slate brush to be used for this menu item in the action menu.
	 */
	FSlateBrush const* GetMenuIcon(FSlateColor& ColorOut);

	/** @return   */
	UBlueprintNodeSpawner const* GetBoundAction() const;

private:
	/** Instanced node-spawner, that comprises the action portion of this menu entry. */
	UBlueprintNodeSpawner const* BoundSpawner;
	/** */
	IBlueprintNodeBinder::FBindingSet BoundObjects;
};
