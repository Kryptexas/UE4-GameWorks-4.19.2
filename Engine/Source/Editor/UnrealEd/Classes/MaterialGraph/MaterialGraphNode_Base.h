// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MaterialGraphNode_Base.generated.h"

UCLASS(MinimalAPI)
class UMaterialGraphNode_Base : public UEdGraphNode
{
	GENERATED_UCLASS_BODY()

	/** Create all of the input pins required */
	virtual void CreateInputPins() {};
	/** Create all of the output pins required */
	virtual void CreateOutputPins() {};
	/** Is this the undeletable root node */
	virtual bool IsRootNode() const {return false;}
	/** Get a single Input Pin via its index */
	class UEdGraphPin* GetInputPin(int32 InputIndex) const;
	/** Get all of the Input Pins */
	UNREALED_API void GetInputPins(TArray<class UEdGraphPin*>& OutInputPins) const;
	/** Get a single Output Pin via its index */
	class UEdGraphPin* GetOutputPin(int32 OutputIndex) const;
	/** Get all of the Output Pins */
	UNREALED_API void GetOutputPins(TArray<class UEdGraphPin*>& OutOutputPins) const;
	/** Replace a given node with this one, changing all pin links */
	UNREALED_API void ReplaceNode(UMaterialGraphNode_Base* OldNode);

	/** Get the Material Expression input index from an input pin */
	virtual int32 GetInputIndex(const UEdGraphPin* InputPin) const {return -1;}
	/** Get the Material value type of an input pin */
	virtual uint32 GetInputType(const UEdGraphPin* InputPin) const {return MCT_Unknown;}

	// Begin UEdGraphNode interface.
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual void ReconstructNode() OVERRIDE;
	virtual void AutowireNewNode(UEdGraphPin* FromPin) OVERRIDE;
	virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const OVERRIDE;
	// End UEdGraphNode interface.

protected:
	void ModifyAndCopyPersistentPinData(UEdGraphPin& TargetPin, const UEdGraphPin& SourcePin) const;
};
