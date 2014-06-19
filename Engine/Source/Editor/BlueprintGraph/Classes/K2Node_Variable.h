// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node.h"
#include "K2Node_Variable.generated.h"

UENUM()
namespace ESelfContextInfo
{
	enum Type
	{
		Unspecified,
		NotSelfContext,
	};
}

UCLASS(MinimalAPI, abstract)
class UK2Node_Variable : public UK2Node
{
	GENERATED_UCLASS_BODY()

	/** Reference to variable we want to set/get */
	UPROPERTY()
	FMemberReference	VariableReference;

	UPROPERTY()
	TEnumAsByte<ESelfContextInfo::Type> SelfContextInfo;

protected:
	/** Class that this variable is defined in. Should be NULL if bSelfContext is true.  */
	UPROPERTY()
	TSubclassOf<class UObject>  VariableSourceClass_DEPRECATED;

	/** Name of variable */
	UPROPERTY()
	FName VariableName_DEPRECATED;

	/** Whether or not this should be a "self" context */
	UPROPERTY()
	uint32 bSelfContext_DEPRECATED:1;

public:
	// Begin UObject interface
	virtual void Serialize(FArchive& Ar) override;
	// End UObject interface

	// Begin UEdGraphNode interface
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual void ReconstructNode() override;
	virtual FString GetDocumentationLink() const override;
	virtual FString GetDocumentationExcerptName() const override;
	virtual FName GetCornerIcon() const override;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const override;
	// End UEdGraphNode interface

	// Begin K2Node interface
	virtual bool HasExternalBlueprintDependencies(TArray<class UStruct*>* OptionalOutput) const override;
	virtual bool DrawNodeAsVariable() const override { return true; }
	virtual ERedirectType DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex)  const override;
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const override;
	virtual FText GetToolTipHeading() const override;
	virtual void GetNodeAttributes( TArray<TKeyValuePair<FString, FString>>& OutNodeAttributes ) const override;
	// End K2Node interface

	/** Set up this variable node from the supplied UProperty */
	BLUEPRINTGRAPH_API void SetFromProperty(const UProperty* Property, bool bSelfContext);

	/** Util to get variable name as a string */
	FString GetVarNameString() const
	{
		return GetVarName().ToString();
	}

	FText GetVarNameText() const
	{
		return FText::FromName( GetVarName() );
	}


	/** Util to get variable name */
	FName GetVarName() const
	{
		return VariableReference.GetMemberName();
	}

	/**
	 * Creates a reader or writer pin for a variable.
	 *
	 * @param	Direction	  	The direction of the variable access.
	 *
	 * @return	true if it succeeds, false if it fails.
	 */
	bool CreatePinForVariable(EEdGraphPinDirection Direction);

	/** Creates 'self' pin */
	void CreatePinForSelf();

	/**
	 * Creates a reader or writer pin for a variable from an old pin.
	 *
	 * @param	Direction	  	The direction of the variable access.
	 * @param	OldPins			Old pins.
	 *
	 * @return	true if it succeeds, false if it fails.
	 */
	bool RecreatePinForVariable(EEdGraphPinDirection Direction, TArray<UEdGraphPin*>& OldPins);

	/** Get the class to look for this variable in */
	BLUEPRINTGRAPH_API UClass* GetVariableSourceClass() const;

	/** Get the UProperty for this variable node */
	BLUEPRINTGRAPH_API UProperty* GetPropertyForVariable() const;

	/** Accessor for the value output pin of the node */
	BLUEPRINTGRAPH_API UEdGraphPin* GetValuePin() const;

	/** Validates there are no errors in the node */
	void CheckForErrors(const class UEdGraphSchema_K2* Schema, class FCompilerResultsLog& MessageLog);

	/**
	 * Utility method intended to serve as a choke point for various slate 
	 * widgets to grab an icon from (for a specified variable).
	 * 
	 * @param  VarScope		The scope that owns the variable in question.
	 * @param  VarName		The name of the variable you're querying for.
	 * @param  IconColorOut	A color out, further discerning the variable's type.
	 * @return A icon representing the specified variable's type.
	 */
	BLUEPRINTGRAPH_API static FName GetVariableIconAndColor(UStruct* VarScope, FName VarName, FLinearColor& IconColorOut);

	/**
	 * Utility method intended to serve as a choke point for various slate 
	 * widgets to grab an icon from (for a specified variable pin type).
	 * 
	 * @param  InPinType	The pin type of the variable in question.
	 * @param  IconColorOut	A color out, further discerning the variable's type.
	 * @return A icon representing the specified variable's type.
	 */
	BLUEPRINTGRAPH_API static FName GetVarIconFromPinType(const FEdGraphPinType& InPinType, FLinearColor& IconColorOut);
};

