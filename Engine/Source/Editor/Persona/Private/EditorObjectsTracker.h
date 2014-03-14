// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

//////////////////////////////////////////////////////////////////////////
// FEditorObjectTracker

class FEditorObjectTracker : public FGCObject
{
public:

	// FGCObject interface
	void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE;
	// End of FGCObject interface

	/** Returns an existing editor object for the specified class or creates one
	    if none exist */
	UObject* GetEditorObjectForClass( UClass* EdClass );

private:

	/** Tracks editor objects created for details panel */
	TMap< UClass*, UObject* >	EditorObjMap;
};