// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "PersonaPrivatePCH.h"
#include "EditorObjectsTracker.h"

void FEditorObjectTracker::AddReferencedObjects( FReferenceCollector& Collector )
{
	Collector.AddReferencedObjects(EditorObjMap);
}

UObject* FEditorObjectTracker::GetEditorObjectForClass( UClass* EdClass )
{
	UObject *Obj = (EditorObjMap.Contains(EdClass) ? *EditorObjMap.Find(EdClass) : NULL);
	if(Obj == NULL)
	{
		FString ObjName = MakeUniqueObjectName(GetTransientPackage(), EdClass).ToString();
		ObjName += "_EdObj";
		Obj = NewObject<UObject>(GetTransientPackage(), EdClass, FName(*ObjName), RF_Public | RF_Standalone | RF_Transient);
		EditorObjMap.Add(EdClass,Obj);
	}
	return Obj;
}