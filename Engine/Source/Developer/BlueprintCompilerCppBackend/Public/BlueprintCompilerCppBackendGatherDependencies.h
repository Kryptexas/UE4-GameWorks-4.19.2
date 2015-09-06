// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma  once

#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/UserDefinedStruct.h"
#include "Engine/UserDefinedEnum.h"

struct BLUEPRINTCOMPILERCPPBACKEND_API FGatherConvertedClassDependencies : public FReferenceCollector
{
protected:
	TSet<UObject*> SerializedObjects;
	UStruct* OriginalStruct;

public:
	TSet<UObject*> Assets;
	TSet<UBlueprintGeneratedClass*> ConvertedClasses;
	TSet<UUserDefinedStruct*> ConvertedStructs;
	TSet<UUserDefinedEnum*> ConvertedEnum;

	TSet<UField*> IncludeInHeader;
	TSet<UField*> DeclareInHeader;
	TSet<UField*> IncludeInBody;
public:
	FGatherConvertedClassDependencies(UStruct* InStruct);

	virtual bool IsIgnoringArchetypeRef() const override { return false; }
	virtual bool IsIgnoringTransient() const override { return true; }

	virtual void HandleObjectReference(UObject*& InObject, const UObject* InReferencingObject, const UProperty* InReferencingProperty) override;

public:
	bool WillClassBeConverted(const UBlueprintGeneratedClass* InClass) const;

protected:
	void FindReferences(UObject* Object);
	void DependenciesForHeader();
	bool ShouldIncludeHeaderFor(UObject* Object) const;
};
