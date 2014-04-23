// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealHeaderTool.h"
#include "Classes.h"
#include "Class.h"
#include "StringUtils.h"

namespace
{
	/** 
	 * Returns True if the given class name includes a valid Unreal prefix and matches based on the given class.
	 *
	 * @param InNameToCheck - Name w/ potential prefix to check
	 * @param OriginalClass - Class to check against
	 */
	bool ClassNameHasValidPrefix(const FString InNameToCheck, const FClass* OriginalClass)
	{
		bool bIsLabledDeprecated;
		GetClassPrefix( InNameToCheck, bIsLabledDeprecated );

		// If the class is labeled deprecated, don't try to resolve it during header generation, valid results can't be guaranteed.
		if (bIsLabledDeprecated)
		{
			return true;
		}

		const FString OriginalClassName = OriginalClass->GetNameWithPrefix();

		bool bNamesMatch = (InNameToCheck == OriginalClassName);

		if (!bNamesMatch)
		{
			//@TODO: UCREMOVAL: I/U interface hack - Ignoring prefixing for this call
			if (OriginalClass->HasAnyClassFlags(CLASS_Interface))
			{
				bNamesMatch = InNameToCheck.Mid(1) == OriginalClassName.Mid(1);
			}
		}

		return bNamesMatch;
	}
}

FClasses::FClasses(UPackage* InPackage)
	: UObjectClass((FClass*)UObject::StaticClass())
	, ClassTree   (UObjectClass)
{
	for (UClass* Class : TObjectRange<UClass>())
	{
		if (Class->IsIn(InPackage))
		{
			ClassTree.AddClass(Class);
		}
	}
}

FClass* FClasses::GetRootClass() const
{
	return UObjectClass;
}

bool FClasses::IsDependentOn(const FClass* Suspect, const FClass* Source) const
{
	// A class is not dependent on itself.
	if (Suspect == Source)
		return false;

	// Children are all implicitly dependent on their parent, that is, children require their parent
	// to be compiled first therefore if the source is a parent of the suspect, the suspect is
	// dependent on the source.
	if (Suspect->Inherits(Source))
		return true;

	// Now consider all dependents of the suspect. If any of them are dependent on the source, the
	// suspect is too.
	for (auto It : Suspect->GetDependentNames())
	{
		const FClass* Dependency = FindClass(*It.ToString());

		// Error.
		if (!Dependency)
			continue;

		// the parser disallows declaring the parent class as a dependency, so the only way this could occur is
		// if the parent for a native class has been changed (which causes the new parent to be inserted as a dependency),
		// if this is the case, skip it or we'll go into a loop
		if (Suspect->GetSuperClass() == Dependency)
			continue;

		if (Dependency == Source || IsDependentOn(Dependency, Source))
			return true;
	}

	// Consider all children of the suspect if any of them are dependent on the source, the suspect is
	// too because it itself is dependent on its children.

	if(!Suspect->HasAnyClassFlags(CLASS_Interface))
	{
		if (!ClassTree.FindNode(const_cast<FClass*>(Suspect)))
			FError::Throwf(TEXT("Unparsed class '%s' found while validating DependsOn entries for '%s'"), *Suspect->GetName(), *Source->GetName());
	}

	return false;
}

FClass* FClasses::FindClass(const TCHAR* ClassName) const
{
	check(ClassName);

	UObject* ClassPackage = ANY_PACKAGE;

	if (UClass* Result = FindObject<UClass>(ClassPackage, ClassName))
		return (FClass*)Result;

	if (UObjectRedirector* RenamedClassRedirector = FindObject<UObjectRedirector>(ClassPackage, ClassName))
		return (FClass*)CastChecked<UClass>(RenamedClassRedirector->DestinationObject);

	return NULL;
}

TArray<FClass*> FClasses::GetDerivedClasses(FClass* Parent) const
{
	const FClassTree* ClassLeaf = ClassTree.FindNode(Parent);
	TArray<const FClassTree*> ChildLeaves;
	ClassLeaf->GetChildClasses(ChildLeaves);

	TArray<FClass*> Result;
	for (auto Node : ChildLeaves)
	{
		Result.Add((FClass*)Node->GetClass());
	}

	return Result;
}

FClass* FClasses::FindScriptClass(const FString& InClassName) const
{
	FString ErrorMsg;
	return FindScriptClass(InClassName, ErrorMsg);
}

FClass* FClasses::FindScriptClassOrThrow(const FString& InClassName) const
{
	FString ErrorMsg;
	if (FClass* Result = FindScriptClass(InClassName, ErrorMsg))
		return Result;

	FError::Throwf(*ErrorMsg);

	// Unreachable, but compiler will warn otherwise because FError::Throwf isn't declared noreturn
	return 0;
}

FClass* FClasses::FindScriptClass(const FString& InClassName, FString& OutErrorMsg) const
{
	// Strip the class name of its prefix and then do a search for the class
	FString ClassNameStripped = GetClassNameWithPrefixRemoved(InClassName);
	if (FClass* FoundClass = FindClass(*ClassNameStripped))
	{
		// If the class was found with the stripped class name, verify that the correct prefix was used and throw an error otherwise
		if( !ClassNameHasValidPrefix( InClassName, FoundClass ) )
		{
			OutErrorMsg = FString::Printf(TEXT("Class '%s' has an incorrect prefix, expecting '%s'"), *InClassName, *FoundClass->GetNameWithPrefix());
			return NULL;
		}

		return (FClass*)FoundClass;
	}

	// Couldn't find the class with a class name stripped of prefix (or a prefix was not found)
	// See if the prefix was forgotten by trying to find the class with the given identifier
	if (FClass* FoundClass = FindClass(*InClassName))
	{
		// If the class was found with the given identifier, the user forgot to use the correct Unreal prefix	
		OutErrorMsg = FString::Printf(TEXT("Class '%s' is missing a prefix, expecting '%s'"), *InClassName, *FoundClass->GetNameWithPrefix());
	}
	else
	{
		// If the class was still not found, it wasn't a valid identifier
		OutErrorMsg = FString::Printf(TEXT("Class '%s' not found."), *InClassName );
	}

	return NULL;
}

TArray<FClass*> FClasses::GetClassesInPackage(UPackage* InPackage) const
{
	TArray<FClass*> Result;
	Result.Add(UObjectClass);

	// This cast is evil, but it'll work until we get TArray covariance. ;-)
	ClassTree.GetChildClasses((TArray<UClass*>&)Result, [=](const UClass* Class) { return InPackage == ANY_PACKAGE || Class->GetOuter() == InPackage; }, true);

	return Result;
}

void FClasses::ChangeParentClass(FClass* Class)
{
	ClassTree.ChangeParentClass(Class);
}

void FClasses::Validate()
{
	ClassTree.Validate();
}
