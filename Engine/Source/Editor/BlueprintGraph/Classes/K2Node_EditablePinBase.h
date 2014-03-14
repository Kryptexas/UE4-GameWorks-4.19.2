// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "K2Node_EditablePinBase.generated.h"

USTRUCT()
struct FUserPinInfo
{
	GENERATED_USTRUCT_BODY()

	/** The name of the pin, as defined by the user */
	UPROPERTY()
	FString PinName;

	/** Type info for the pin */
	UPROPERTY()
	struct FEdGraphPinType PinType;

	/** The default value of the pin */
	UPROPERTY()
	FString PinDefaultValue;

	/** Direction of the pin */
	///var EEdGraphPinDirection	PinDirection;


		friend FArchive& operator <<(FArchive& Ar, FUserPinInfo& Info)
		{
			Ar << Info.PinName;

			if (Ar.UE4Ver() >= VER_UE4_ADD_PINTYPE_ARRAY )
			{
				Ar << Info.PinType.bIsArray;
			}

			if (Ar.UE4Ver() >= VER_UE4_ADD_PINTYPE_BYREF )
			{
				Ar << Info.PinType.bIsReference;
			}

			Ar << Info.PinType.PinCategory;
			Ar << Info.PinType.PinSubCategory;

			Ar << Info.PinType.PinSubCategoryObject;
			Ar << Info.PinDefaultValue;

			return Ar;
		}
	
};

// This structure describes metadata associated with a user declared function or macro
// It will be turned into regular metadata during compilation
USTRUCT()
struct FKismetUserDeclaredFunctionMetadata
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString ToolTip;

	UPROPERTY()
	FString Category;

	UPROPERTY()
	FLinearColor InstanceTitleColor;

public:
	FKismetUserDeclaredFunctionMetadata()
		: InstanceTitleColor(FLinearColor::White)
	{
	}
};

UCLASS(abstract, MinimalAPI)
class UK2Node_EditablePinBase : public UK2Node
{
	GENERATED_UCLASS_BODY()

	/** Whether or not this entry node should be user-editable with the function editor */
	UPROPERTY()
	uint32 bIsEditable:1;



#if WITH_EDITORONLY_DATA
	/** Pins defined by the user */
	TArray< TSharedPtr<FUserPinInfo> >UserDefinedPins;
#endif // WITH_EDITORONLY_DATA

	BLUEPRINTGRAPH_API virtual bool IsEditable() const { return bIsEditable; }

	// UObject interface
	BLUEPRINTGRAPH_API virtual void Serialize(FArchive& Ar) OVERRIDE;
	BLUEPRINTGRAPH_API static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	BLUEPRINTGRAPH_API virtual void ExportCustomProperties(FOutputDevice& Out, uint32 Indent) OVERRIDE;
	BLUEPRINTGRAPH_API virtual void ImportCustomProperties(const TCHAR* SourceText, FFeedbackContext* Warn) OVERRIDE;
	// End of UObject interface

	// UEdGraphNode interface
	BLUEPRINTGRAPH_API virtual void AllocateDefaultPins() OVERRIDE;
	// End of UEdGraphNode interface

	// UK2Node interface
	BLUEPRINTGRAPH_API virtual bool ShouldShowNodeProperties() const OVERRIDE { return bIsEditable; }
	// End of UK2Node interface

	/**
	 * Creates a UserPinInfo from the specified information, and also adds a pin based on that description to the node
	 *
	 * @param	InPinName	Name of the pin to create
	 * @param	InPinType	The type info for the pin to create
	 */
	BLUEPRINTGRAPH_API UEdGraphPin* CreateUserDefinedPin(const FString& InPinName, const FEdGraphPinType& InPinType);

	/**
	 * Removes a pin from the user-defined array, and removes the pin with the same name from the Pins array
	 *
	 * @param	PinToRemove	Shared pointer to the pin to remove from the UserDefinedPins array.  Corresponding pin in the Pins array will also be removed
	 */
	BLUEPRINTGRAPH_API void RemoveUserDefinedPin( TSharedPtr<FUserPinInfo> PinToRemove );

	/**
	 * Creates a new pin on the node from the specified user pin info.
	 * Must be overridden so each type of node can ensure that the pin is created in the proper direction, etc
	 * 
	 * @param	NewPinInfo		Shared pointer to the struct containing the info for this pin
	 */
	virtual UEdGraphPin* CreatePinFromUserDefinition(const TSharedPtr<FUserPinInfo> NewPinInfo) { return NULL; }

	// Modifies the default value of an existing pin on the node.
	BLUEPRINTGRAPH_API virtual bool ModifyUserDefinedPinDefaultValue(TSharedPtr<FUserPinInfo> PinInfo, const FString& NewDefaultValue);

	/**
	 * Can this node have execution wires added or removed?
	 */
	virtual bool CanModifyExecutionWires() { return false; }

	/**
	 * Can this node have pass-by-reference parameters?
	 */
	virtual bool CanUseRefParams() const { return false; }
};

