// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "UnrealEd.h"
#include "PackageTools.h"
#include "BlueprintUtilities.h"
#include "KismetEditorUtilities.h"
DEFINE_LOG_CATEGORY_STATIC(LogEditorClassHierarchy, Log, All);

struct EditorClassEntry
{
	FString ClassName;
	FString PackageName;
	FString ClassGroupString;
	int32 ParentClassIndex;
	int32 NumChildren;
	int32 NumPlaceableChildren;
	bool bIsHidden;
	bool bIsPlaceable;
	bool bIsAbstract;

	//loaded first time it's requested
	TWeakObjectPtr<UClass> ClassPtr;

	/** Pointer to Blueprint object, if this entry is a Blueprint, instead of a normal .h-file generated class */
	TWeakObjectPtr<UBlueprint> BlueprintPtr;
};

TArray<EditorClassEntry> EditorClassArray;

struct FEditorClassArrayBuilder
{
	TMap<FString, int32> NameAndPackageToIndexMap;
	TMultiMap<UClass*, UClass*> Children;

	FEditorClassArrayBuilder()
	{
		for( TObjectIterator<UClass> ItC; ItC; ++ItC )
		{
			UClass* Class = *ItC;
			if (Class != UObject::StaticClass())
			{
				Children.Add(Class->GetSuperClass(), Class);
			}
		}
		AddClass(UObject::StaticClass());
	}

	int32 AddClass(UClass* Class)
	{
		if ( Class->HasAnyClassFlags(CLASS_Deprecated | CLASS_NewerVersionExists) || FKismetEditorUtilities::IsClassABlueprintSkeleton (Class))
		{
			// Don't add any deprecated classes like stale blueprint generated classes
			return INDEX_NONE;
		}

		FString DotName = Class->GetOutermost()->GetName() + TEXT(".") + Class->GetName();
		int32 *AlreadyThere = NameAndPackageToIndexMap.Find(DotName);
		if (AlreadyThere)
		{
			return *AlreadyThere;
		}
		int32 ParentIndex = INDEX_NONE;
		if (Class != UObject::StaticClass())
		{
			ParentIndex = AddClass(Class->GetSuperClass());
		}
		// look again, we might have been added
		AlreadyThere = NameAndPackageToIndexMap.Find(DotName);
		if (AlreadyThere)
		{
			return *AlreadyThere;
		}
		EditorClassEntry ClassEntry;
		ClassEntry.ClassName = Class->GetName();
		ClassEntry.PackageName = Class->GetOutermost()->GetName();
		TArray<FString> ClassGroupNames;
		Class->GetClassGroupNames(ClassGroupNames);
		for (int32 Index = 0; Index < ClassGroupNames.Num(); Index++)
		{
			if (Index)
			{
				ClassEntry.ClassGroupString += TEXT(",");
			}
			ClassEntry.ClassGroupString += ClassGroupNames[Index];
		}
		ClassEntry.NumChildren = 0;
		ClassEntry.NumPlaceableChildren = 0;
		ClassEntry.bIsHidden = Class->HasAnyClassFlags(CLASS_Hidden);
		ClassEntry.bIsPlaceable = !Class->HasAnyClassFlags(CLASS_NotPlaceable | CLASS_Abstract);
		ClassEntry.bIsAbstract = Class->HasAnyClassFlags(CLASS_Abstract);
		ClassEntry.ClassPtr.Reset();
		ClassEntry.BlueprintPtr.Reset();
		ClassEntry.ParentClassIndex = ParentIndex;

		if (ParentIndex >= 0)
		{
			EditorClassArray[ParentIndex].NumChildren++;
			EditorClassArray[ParentIndex].NumPlaceableChildren += ClassEntry.bIsPlaceable ? 1 : 0;
		}
		int32 ReturnIndex = EditorClassArray.Add(ClassEntry);
		NameAndPackageToIndexMap.Add(DotName, ReturnIndex);

		// add all of my child classes
		TArray<UClass*> ChildrenClasses;
		for(TMultiMap<UClass*,UClass*>::TConstKeyIterator It(Children,Class); It; ++It)
		{
			ChildrenClasses.Add(It.Value());
		}
		ChildrenClasses.Sort();

		for (int32 Index = 0; Index < ChildrenClasses.Num(); Index++)
		{
			AddClass(ChildrenClasses[Index]);
		}

		return ReturnIndex;
	}
};

/**
 * Builds the tree from the class manifest
 * @return - whether the class hierarchy was successfully built
 */
bool FEditorClassHierarchy::Init (void)
{
	EditorClassArray.Empty();
	FEditorClassArrayBuilder EditorClassArrayBuilder;
	return true;
}

/**
 * true, if the manifest file loaded classes successfully
 */
bool FEditorClassHierarchy::WasLoadedSuccessfully(void) const
{
	return (EditorClassArray.Num() > 0);
}

/**
 * Gets the direct children of the class
 * @param InClassIndex - the index of the class in question
 * @param OutIndexArray - the array to fill in with child indices
 */
void FEditorClassHierarchy::GetChildrenOfClass(const int32 InClassIndex, TArray<int32> &OutIndexArray)
{
	OutIndexArray.Empty();
	for (int32 i = 0; i < EditorClassArray.Num(); ++i)
	{
		if (EditorClassArray[i].ParentClassIndex == InClassIndex)
		{
			OutIndexArray.Add(i);
		}
	}
}

/** 
 * Adds the node and all children recursively to the list of OutAllClasses 
 * @param InClassIndex - The node in the hierarchy to start with
 * @param OutAllClasses - The list of classes generated recursively
 */
void FEditorClassHierarchy::GetAllClasses(const int32 InClassIndex, OUT TArray<UClass*>& OutAllClasses)
{
	//add this class
	check(FMath::IsWithin(InClassIndex, 0, EditorClassArray.Num()));
	UClass* CurrentClass = GetClass(InClassIndex);
	if (CurrentClass)
	{
		OutAllClasses.Add(CurrentClass);

		//recurse
		TArray<int32> ChildIndexArray;
		GetChildrenOfClass(InClassIndex, ChildIndexArray);
		for (int32 i = 0; i < ChildIndexArray.Num(); ++i)
		{
			int32 ChildClassIndex = ChildIndexArray[i];
			GetAllClasses(ChildClassIndex, OutAllClasses);
		}
	}
}

/**
 * Find the class index that matches the requested name
 * @param InClassName - Name of the class to find
 * @return - The index of the desired class
 */
int32 FEditorClassHierarchy::Find(const FString& InClassName)
{
	for (int32 i = 0; i < EditorClassArray.Num(); ++i)
	{
		if (EditorClassArray[i].ClassName == InClassName)
		{
			return i;
		}
	}
	return -1;
}

/**
 * returns the name of the class in the tree
 * @param InClassIndex - the index of the class in question
 */
FString FEditorClassHierarchy::GetClassName(const int32 InClassIndex)
{
	check(FMath::IsWithin(InClassIndex, 0, EditorClassArray.Num()));
	return EditorClassArray[InClassIndex].ClassName;
}

/**
 * returns the UClass of the class in the tree
 * @param InClassIndex - the index of the class in question
 */
UClass* FEditorClassHierarchy::GetClass(const int32 InClassIndex)
{
	check(FMath::IsWithin(InClassIndex, 0, EditorClassArray.Num()));
	if (!EditorClassArray[InClassIndex].ClassPtr.IsValid())
	{
		// handle blueprint case first - look up class
		if(EditorClassArray[InClassIndex].BlueprintPtr.IsValid())
		{
			UClass* BlueprintClass = EditorClassArray[InClassIndex].BlueprintPtr->GeneratedClass;
			return BlueprintClass;
		}
		else if (EditorClassArray[InClassIndex].ParentClassIndex == -1)
		{
			EditorClassArray[InClassIndex].ClassPtr = UObject::StaticClass();
		}
		else
		{
			UClass* ParentClass = GetClass(EditorClassArray[InClassIndex].ParentClassIndex);
			FString ClassObjectPath = FString::Printf(TEXT("%s.%s"), *EditorClassArray[InClassIndex].PackageName, *EditorClassArray[InClassIndex].ClassName);
			EditorClassArray[InClassIndex].ClassPtr = StaticLoadClass( ParentClass, NULL, *ClassObjectPath, NULL, LOAD_None, NULL );
		}
	}
	return EditorClassArray[InClassIndex].ClassPtr.Get();
}

/**
 * Returns the class index of the provided index's parent class
 *
 * @param	InClassIndex	Class index to find the parent of
 *
 * @return	Class index of the parent class of the provided index, if any; INDEX_NONE if not
 */
int32 FEditorClassHierarchy::GetParentIndex(int32 InClassIndex) const
{
	check( EditorClassArray.IsValidIndex( InClassIndex ) );
	return EditorClassArray[InClassIndex].ParentClassIndex;
}

/** 
 * Returns a list of class group names for the provided class index 
 *
 * @param InClassIndex	The class index to find group names for
 * @param OutGroups		The list of class groups found.
 */
void FEditorClassHierarchy::GetClassGroupNames( int32 InClassIndex, TArray<FString>& OutGroups ) const
{
	const EditorClassEntry& Entry = EditorClassArray[InClassIndex];
	Entry.ClassGroupString.ParseIntoArray( &OutGroups, TEXT(","), true );
}

/**
 * returns if the class is hidden or not
 * @param InClassIndex - the index of the class in question
 */
bool FEditorClassHierarchy::IsHidden(const int32 InClassIndex) const
{
	check(FMath::IsWithin(InClassIndex, 0, EditorClassArray.Num()));
	return EditorClassArray[InClassIndex].bIsHidden;
}
/**
 * returns if the class is placeable or not
 * @param InClassIndex - the index of the class in question
 */
bool FEditorClassHierarchy::IsPlaceable(const int32 InClassIndex) const
{
	check(FMath::IsWithin(InClassIndex, 0, EditorClassArray.Num()));
	return EditorClassArray[InClassIndex].bIsPlaceable;
}
/**
 * returns if the class is abstract or not
 * @param InClassIndex - the index of the class in question
 */
bool FEditorClassHierarchy::IsAbstract(const int32 InClassIndex) const
{
	check(FMath::IsWithin(InClassIndex, 0, EditorClassArray.Num()));
	return EditorClassArray[InClassIndex].bIsAbstract;
}
/**
 * returns if the class is a brush or not
 * @param InClassIndex - the index of the class in question
 */
bool FEditorClassHierarchy::IsBrush(const int32 InClassIndex)
{
	UClass* Class = GetClass( InClassIndex );
	return Class->IsChildOf( ABrush::StaticClass() );
}
/**
 * Returns if the class is visible 
 * @param InClassIndex - the index of the class in question
 * @param bInPlaceable - if true, return number of placeable children, otherwise returns all children
 * @return - Number of children
 */
bool FEditorClassHierarchy::IsClassVisible(const int32 InClassIndex, const bool bInPlaceableOnly)
{
	check(FMath::IsWithin(InClassIndex, 0, EditorClassArray.Num()));

	const EditorClassEntry &TempEntry = EditorClassArray[InClassIndex];

	if ((TempEntry.bIsHidden) || (TempEntry.bIsAbstract))
	{
		return false;
	}
	else if ( !bInPlaceableOnly || (TempEntry.bIsPlaceable) )
	{
		return true;
	}
	return false;
}

/**
 * Returns if the class has any children (placeable or all)
 * @param InClassIndex - the index of the class in question
 * @param bInPlaceable - if true, return if has placeable children, otherwise returns if has any children
 * @return - Whether this node has children (recursively) that are visible
 */
bool FEditorClassHierarchy::HasChildren(const int32 InClassIndex, const bool bInPlaceableOnly)
{
	check(FMath::IsWithin(InClassIndex, 0, EditorClassArray.Num()));

	TArray<int32> ChildIndexArray;
	GetChildrenOfClass(InClassIndex, ChildIndexArray);

	for ( int32 i = 0 ; i < ChildIndexArray.Num() ; ++i )
	{
		int32 ChildClassIndex = ChildIndexArray[i];
		bool bIsClassVisible = IsClassVisible(ChildClassIndex, bInPlaceableOnly);
		if (bIsClassVisible)
		{
			return true;
		}
		else if ( HasChildren( ChildClassIndex, bInPlaceableOnly ) )
		{
			return true;
		}
	}
	return false;
}
