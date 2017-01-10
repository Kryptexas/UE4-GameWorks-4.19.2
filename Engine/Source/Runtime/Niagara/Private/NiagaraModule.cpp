// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "NiagaraModule.h"
#include "Modules/ModuleManager.h"
#include "NiagaraTypes.h"
#include "NiagaraEvents.h"
#include "Class.h"
#include "Package.h"

IMPLEMENT_MODULE(INiagaraModule, Niagara);


const UScriptStruct* FNiagaraTypeDefinition::NumericStruct;
const UScriptStruct* FNiagaraTypeDefinition::FloatStruct;
const UScriptStruct* FNiagaraTypeDefinition::BoolStruct;
const UScriptStruct* FNiagaraTypeDefinition::IntStruct;
const UScriptStruct* FNiagaraTypeDefinition::Matrix4Struct;
const UScriptStruct* FNiagaraTypeDefinition::Vec4Struct;
const UScriptStruct* FNiagaraTypeDefinition::Vec3Struct;
const UScriptStruct* FNiagaraTypeDefinition::Vec2Struct;
const UScriptStruct* FNiagaraTypeDefinition::ColorStruct;

FNiagaraTypeDefinition FNiagaraTypeDefinition::NumericDef;
FNiagaraTypeDefinition FNiagaraTypeDefinition::FloatDef;
FNiagaraTypeDefinition FNiagaraTypeDefinition::BoolDef;
FNiagaraTypeDefinition FNiagaraTypeDefinition::IntDef;
FNiagaraTypeDefinition FNiagaraTypeDefinition::Matrix4Def;
FNiagaraTypeDefinition FNiagaraTypeDefinition::Vec4Def;
FNiagaraTypeDefinition FNiagaraTypeDefinition::Vec3Def;
FNiagaraTypeDefinition FNiagaraTypeDefinition::Vec2Def;
FNiagaraTypeDefinition FNiagaraTypeDefinition::ColorDef;

TSet<const UScriptStruct*> FNiagaraTypeDefinition::NumericStructs;
TArray<FNiagaraTypeDefinition> FNiagaraTypeDefinition::OrderedNumericTypes;

TSet<const UScriptStruct*> FNiagaraTypeDefinition::ScalarStructs;

TSet<const UStruct*> FNiagaraTypeDefinition::FloatStructs;
TSet<const UStruct*> FNiagaraTypeDefinition::IntStructs;
TSet<const UStruct*> FNiagaraTypeDefinition::BoolStructs;

FNiagaraTypeDefinition FNiagaraTypeDefinition::CollisionEventDef;


TArray<FNiagaraTypeDefinition> FNiagaraTypeRegistry::RegisteredTypes;
TArray<FNiagaraTypeDefinition> FNiagaraTypeRegistry::RegisteredParamTypes;
TArray<FNiagaraTypeDefinition> FNiagaraTypeRegistry::RegisteredPayloadTypes;

void INiagaraModule::StartupModule()
{
	FNiagaraTypeDefinition::Init();
}

void FNiagaraTypeDefinition::Init()
{
	static auto* CoreUObjectPkg = FindObjectChecked<UPackage>(nullptr, TEXT("/Script/CoreUObject"));
	static auto* NiagaraPkg = FindObjectChecked<UPackage>(nullptr, TEXT("/Script/Niagara"));
	FNiagaraTypeDefinition::NumericStruct = FindObjectChecked<UScriptStruct>(NiagaraPkg, TEXT("NiagaraNumeric"));
	FNiagaraTypeDefinition::FloatStruct = FindObjectChecked<UScriptStruct>(NiagaraPkg, TEXT("NiagaraFloat"));
	FNiagaraTypeDefinition::BoolStruct = FindObjectChecked<UScriptStruct>(NiagaraPkg, TEXT("NiagaraBool"));
	FNiagaraTypeDefinition::IntStruct = FindObjectChecked<UScriptStruct>(NiagaraPkg, TEXT("NiagaraInt32"));
	FNiagaraTypeDefinition::Vec2Struct = FindObjectChecked<UScriptStruct>(CoreUObjectPkg, TEXT("Vector2D"));
	FNiagaraTypeDefinition::Vec3Struct = FindObjectChecked<UScriptStruct>(CoreUObjectPkg, TEXT("Vector"));
	FNiagaraTypeDefinition::Vec4Struct = FindObjectChecked<UScriptStruct>(CoreUObjectPkg, TEXT("Vector4"));
	FNiagaraTypeDefinition::ColorStruct = FindObjectChecked<UScriptStruct>(CoreUObjectPkg, TEXT("LinearColor"));
	FNiagaraTypeDefinition::Matrix4Struct = FindObjectChecked<UScriptStruct>(CoreUObjectPkg, TEXT("Matrix"));
	
	NumericDef = FNiagaraTypeDefinition(NumericStruct);
	FloatDef = FNiagaraTypeDefinition(FloatStruct);
	BoolDef = FNiagaraTypeDefinition(BoolStruct);
	IntDef = FNiagaraTypeDefinition(IntStruct);
	Vec2Def = FNiagaraTypeDefinition(Vec2Struct);
	Vec3Def = FNiagaraTypeDefinition(Vec3Struct);
	Vec4Def = FNiagaraTypeDefinition(Vec4Struct);
	ColorDef = FNiagaraTypeDefinition(ColorStruct);
	Matrix4Def = FNiagaraTypeDefinition(Matrix4Struct);
	NumericDef = FNiagaraTypeDefinition(NumericStruct);

	CollisionEventDef = FNiagaraTypeDefinition(FNiagaraCollisionEventPayload::StaticStruct());
	FNiagaraTypeRegistry::Register(CollisionEventDef, false, true);

	FNiagaraTypeRegistry::Register(NumericDef, true, false);
	FNiagaraTypeRegistry::Register(FloatDef, true, true);
	FNiagaraTypeRegistry::Register(IntDef, true, true);
	FNiagaraTypeRegistry::Register(BoolDef, true, true);
	FNiagaraTypeRegistry::Register(Vec2Def, true, true);
	FNiagaraTypeRegistry::Register(Vec3Def, true, true);
	FNiagaraTypeRegistry::Register(Vec4Def, true, true);
	FNiagaraTypeRegistry::Register(ColorDef, true, true);
	FNiagaraTypeRegistry::Register(Matrix4Def, true, false);

	UScriptStruct* TestStruct = FindObjectChecked<UScriptStruct>(NiagaraPkg, TEXT("NiagaraTestStruct"));
	FNiagaraTypeDefinition TestDefinition(TestStruct);
	FNiagaraTypeRegistry::Register(TestDefinition, true, false);

	NumericStructs.Add(NumericStruct);
	NumericStructs.Add(FloatStruct);
	NumericStructs.Add(IntStruct);
	NumericStructs.Add(Vec2Struct);
	NumericStructs.Add(Vec3Struct);
	NumericStructs.Add(Vec4Struct);
	NumericStructs.Add(ColorStruct);
	//Make matrix a numeric type?

	FloatStructs.Add(FloatStruct);
	FloatStructs.Add(Vec2Struct);
	FloatStructs.Add(Vec3Struct);
	FloatStructs.Add(Vec4Struct);
	FloatStructs.Add(ColorStruct);

	IntStructs.Add(IntStruct);

	BoolStructs.Add(BoolStruct);

	OrderedNumericTypes.Add(IntStruct);
	OrderedNumericTypes.Add(FloatStruct);
	OrderedNumericTypes.Add(Vec2Struct);
	OrderedNumericTypes.Add(Vec3Struct);
	OrderedNumericTypes.Add(Vec4Struct);
	OrderedNumericTypes.Add(ColorStruct);

	ScalarStructs.Add(BoolStruct);
	ScalarStructs.Add(IntStruct);
	ScalarStructs.Add(FloatStruct);
}

bool FNiagaraTypeDefinition::IsScalarDefinition(const FNiagaraTypeDefinition& Type)
{
	return ScalarStructs.Contains(Type.GetScriptStruct());
}

bool FNiagaraTypeDefinition::TypesAreAssignable(const FNiagaraTypeDefinition& TypeA, const FNiagaraTypeDefinition& TypeB)
{
	if (const UClass* AClass = TypeA.GetClass())
	{
		if (const UClass* BClass = TypeB.GetClass())
		{
			return AClass == BClass;
			return true;
		}
	}
	
	if (const UClass* BClass = TypeB.GetClass())
	{
		return false;
	}

	return (TypeA == NumericDef && NumericStructs.Contains(TypeB.GetScriptStruct())) ||
		(TypeB == NumericDef && NumericStructs.Contains(TypeA.GetScriptStruct()));
}

FNiagaraTypeDefinition FNiagaraTypeDefinition::GetNumericOutputType(const TArray<FNiagaraTypeDefinition> TypeDefinintions, ENiagaraNumericOutputTypeSelectionMode SelectionMode)
{
	checkf(SelectionMode != ENiagaraNumericOutputTypeSelectionMode::None, TEXT("Can not get numeric output type with selection mode none."));

	//This may need some work. Should work fine for now.
	if (SelectionMode == ENiagaraNumericOutputTypeSelectionMode::Scalar)
	{
		bool bHasFloats = false;
		bool bHasInts = false;
		bool bHasBools = false;
		for (const FNiagaraTypeDefinition& Type : TypeDefinintions)
		{
			bHasFloats |= FloatStructs.Contains(Type.GetStruct());
			bHasInts |= IntStructs.Contains(Type.GetStruct());
			bHasBools |= BoolStructs.Contains(Type.GetStruct());
		}
		//Not sure what to do if we have multiple different types here.
		//Possibly pick this up ealier and throw a compile error?
		if (bHasFloats) return FNiagaraTypeDefinition::GetFloatDef();
		if (bHasInts) return FNiagaraTypeDefinition::GetIntDef();
		if (bHasBools) return FNiagaraTypeDefinition::GetBoolDef();
	}
	// Always return the numeric type definition if it's included since this isn't a valid use case and we don't want to hide it.
	int32 NumericTypeDefinitionIndex = TypeDefinintions.IndexOfByKey(NumericDef);
	if (NumericTypeDefinitionIndex != INDEX_NONE)
	{
		// TODO: Warning here?
		return NumericDef;
	}

	TArray<FNiagaraTypeDefinition> SortedTypeDefinitions = TypeDefinintions;
	SortedTypeDefinitions.Sort([&](const FNiagaraTypeDefinition& TypeA, const FNiagaraTypeDefinition& TypeB)
	{
		int32 AIndex = OrderedNumericTypes.IndexOfByKey(TypeA);
		int32 BIndex = OrderedNumericTypes.IndexOfByKey(TypeB);
		return AIndex < BIndex;
	});

	if (SelectionMode == ENiagaraNumericOutputTypeSelectionMode::Largest)
	{
		return SortedTypeDefinitions.Last();
	}
	else // if (SelectionMode == ENiagaraNumericOutputTypeSelectionMode::Smallest)
	{
		return SortedTypeDefinitions[0];
	}
}

//////////////////////////////////////////////////////////////////////////

FString FNiagaraFunctionSignature::GetSymbol()const
{
	if (bMemberFunction)
	{
		return Name.ToString() + TEXT("_") + Owner.ToString();
	}
	else
	{
		return Name.ToString() + TEXT("_Global");
	}
}
