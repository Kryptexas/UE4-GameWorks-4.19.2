// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Engine.h"

class UNREALED_API FStructureEditorUtils
{
public:

	struct FFindByNameHelper
	{
		const FName Name;

		FFindByNameHelper(FName InName) : Name(InName) { }

		bool operator() (const FBPStructureDescription& Element) const
		{
			return (Name == Element.Name);
		}

		bool operator() (const FBPVariableDescription& Element) const
		{
			return (Name == Element.VarName);
		}
	};

	struct FFindByGuidHelper
	{
		FGuid Guid;

		FFindByGuidHelper(FGuid InGuid) : Guid(InGuid) { }

		bool operator() (const FBPVariableDescription& Element) const
		{
			return (Guid == Element.VarGuid);
		}
	};

	//STRUCTURE
	static bool AddStructure(UBlueprint* Blueprint);

	static bool RemoveStructure(UBlueprint* Blueprint, FName StructName);

	static bool RenameStructure(UBlueprint* Blueprint, FName OldStructName, const FString& NewName);

	static bool DuplicateStructure(UBlueprint* Blueprint, FName StructName);

	//VARIABLE
	static bool AddVariable(UBlueprint* Blueprint, FName StructName, const FEdGraphPinType& VarType);

	static bool RemoveVariable(UBlueprint* Blueprint, FName StructName, FGuid VarGuid);

	static bool RenameVariable(UBlueprint* Blueprint, FName StructName, FGuid VarGuid, const FString& NewDisplayNameStr);

	static bool ChangeVariableType(UBlueprint* Blueprint, FName StructName, FGuid VarGuid, const FEdGraphPinType& NewType);

	static bool ChangeVariableDefaultValue(UBlueprint* Blueprint, FName StructName, FGuid VarGuid, const FString& NewDefaultValue);

	static bool IsUniqueVariableDisplayName(const UBlueprint* Blueprint, FName StructName, const FString& DisplayName);

	static FString GetDisplayName(const UBlueprint* Blueprint, FName StructName, FGuid VarGuid);

	//MISC
	static void GetAllStructureNames(const UBlueprint* Blueprint, TArray<FName>& OutNames);

	static bool StructureEditingEnabled();

	static void CompileStructuresInBlueprint(UBlueprint* Blueprint);

	static void RemoveInvalidStructureMemberVariableFromBlueprint(UBlueprint* Blueprint);

	/*
	 Default values for member variables in User Defined Structure are stored in meta data "MakeStructureDefaultValue"
	 The following functions are used to fill an instance of user defined struct with those default values.
	 */
	static bool Fill_MakeStructureDefaultValue(const UBlueprintGeneratedStruct* Struct, uint8* StructData);

	static bool Fill_MakeStructureDefaultValue(const UProperty* Property, uint8* PropertyData);

	//VALIDATION
	static bool CanHaveAMemberVariableOfType(const UBlueprintGeneratedStruct* Struct, const FEdGraphPinType& VarType, FString* OutMsg = NULL);

	enum EStructureError
	{
		Ok, 
		Recursion,
		FallbackStruct,
		NotCompiled,
		NotBlueprintType,
	};

	/** Can the structure be a member variable for a BPGClass or BPGStruct */
	static EStructureError IsStructureValid(const UScriptStruct* Struct, const UStruct* RecursionParent = NULL, FString* OutMsg = NULL);
private:
	static FName MakeUniqueStructName(UBlueprint* Blueprint, const FString& BaseName);
	static void OnStructureChanged(FBPStructureDescription& StructureDesc, UBlueprint* Blueprint);
};

