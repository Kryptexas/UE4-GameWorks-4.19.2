// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimGraphNode_BlendListByEnum.generated.h"

UCLASS(MinimalAPI)
class UAnimGraphNode_BlendListByEnum : public UAnimGraphNode_BlendListBase, public INodeDependingOnEnumInterface
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Settings)
	FAnimNode_BlendListByEnum Node;
	
protected:
	/** Name of the enum being switched on */
	UPROPERTY()
	UEnum* BoundEnum;

	UPROPERTY()
	TArray<FName> VisibleEnumEntries;
public:
	// UEdGraphNode interface
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual void PostPlacedNewNode() OVERRIDE;
	// End of UEdGraphNode interface

	// UK2Node interface
	virtual void GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const OVERRIDE;
	// End of UK2Node interface

	// UAnimGraphNode_Base interface
	virtual FString GetNodeCategory() const OVERRIDE;
	virtual void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const OVERRIDE;
	virtual void CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const OVERRIDE;
	virtual void ValidateAnimNodeDuringCompilation(class USkeleton* ForSkeleton, class FCompilerResultsLog& MessageLog) OVERRIDE;
	virtual void BakeDataDuringCompilation(class FCompilerResultsLog& MessageLog) OVERRIDE;
	virtual void PreloadRequiredAssets() OVERRIDE;
	// End of UAnimGraphNode_Base interface

	//@TODO: Generalize this behavior (returning a list of actions/delegates maybe?)
	virtual void RemovePinFromBlendList(UEdGraphPin* Pin);

	// INodeDependingOnEnumInterface
	virtual class UEnum* GetEnum() const OVERRIDE { return BoundEnum; }
	virtual bool ShouldBeReconstructedAfterEnumChanged() const OVERRIDE {return true;}
	// End of INodeDependingOnEnumInterface
protected:
	// Exposes a pin corresponding to the specified element name
	void ExposeEnumElementAsPin(FName EnumElementName);

	// Gets information about the specified pin.  If both bIsPosePin and bIsTimePin are false, the index is meaningless
	static void GetPinInformation(const FString& InPinName, int32& Out_PinIndex, bool& Out_bIsPosePin, bool& Out_bIsTimePin);
};
