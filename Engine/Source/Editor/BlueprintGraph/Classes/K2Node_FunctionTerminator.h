// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Engine/MemberReference.h"
#include "Templates/SubclassOf.h"
#include "K2Node_EditablePinBase.h"
#include "K2Node_FunctionTerminator.generated.h"

UCLASS(abstract, MinimalAPI)
class UK2Node_FunctionTerminator : public UK2Node_EditablePinBase
{
	GENERATED_UCLASS_BODY()

	/**
	 * Reference to the function signature.
	 */
	UPROPERTY()
	FMemberReference FunctionReference;

	//~ Begin UObject Interface
	virtual void Serialize(FArchive& Ar) override;
	//~ End UObject Interface

	//~ Begin UEdGraphNode Interface
	virtual bool CanDuplicateNode() const override { return false; }
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FName CreateUniquePinName(FName SourcePinName) const override;
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const override;
	//~ End UEdGraphNode Interface

	//~ Begin UK2Node Interface
	virtual bool NodeCausesStructuralBlueprintChange() const override { return true; }
	virtual bool HasExternalDependencies(TArray<class UStruct*>* OptionalOutput) const override;
	//~ End UK2Node Interface

	//~ Begin UK2Node_EditablePinBase Interface
	virtual bool CanCreateUserDefinedPin(const FEdGraphPinType& InPinType, EEdGraphPinDirection InDesiredDirection, FText& OutErrorMessage) override;
	//~ End UK2Node_EditablePinBase Interface

	/** Promotes the node from being a part of an interface override to a full function that allows for parameter and result pin additions */
	virtual void PromoteFromInterfaceOverride(bool bIsPrimaryTerminator = true);

private:
	/** (DEPRECATED) Function signature class. Replaced by the 'FunctionReference' property. */
	UPROPERTY()
	TSubclassOf<class UObject> SignatureClass_DEPRECATED;

	/** (DEPRECATED) Function signature name. Replaced by the 'FunctionReference' property. */
	UPROPERTY()
	FName SignatureName_DEPRECATED;
};

