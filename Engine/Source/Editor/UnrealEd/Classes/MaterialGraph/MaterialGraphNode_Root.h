// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MaterialGraphNode_Root.generated.h"

UCLASS(MinimalAPI)
class UMaterialGraphNode_Root : public UMaterialGraphNode_Base
{
	GENERATED_UCLASS_BODY()

	/** Material whose inputs this root node represents */
	UPROPERTY()
	class UMaterial* Material;

	// Begin UEdGraphNode interface.
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual bool CanUserDeleteNode() const OVERRIDE { return false; }
	virtual bool CanDuplicateNode() const OVERRIDE { return false; }
	virtual void PostPlacedNewNode() OVERRIDE;
	// End UEdGraphNode interface.

	// UMaterialGraphNode_Base interface
	virtual void CreateInputPins() OVERRIDE;
	virtual bool IsRootNode() const OVERRIDE {return true;}
	virtual int32 GetInputIndex(const UEdGraphPin* InputPin) const OVERRIDE;
	virtual uint32 GetInputType(const UEdGraphPin* InputPin) const OVERRIDE;
	// End of UMaterialGraphNode_Base interface
};
