// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

namespace nLiveEditorListenServer
{
	class FObjectCache
	{
	public:
		FObjectCache();
		~FObjectCache();

		void FindObjectDependants( const class UObject* EditorObject, TArray< TWeakObjectPtr<UObject> >& OutValues );
		void StartCache();
		void EndCache();
		void OnObjectCreation( const class UObjectBase* ObjectBase );
		void OnObjectDeletion( const class UObjectBase* ObjectBase );
		void EvaluatePendingCreations();

	private:
		bool CacheStarted() const;

	private:
		bool bCacheActive;
		TArray< const class UObject* > TrackedObjects;
		TMultiMap< const class UObject* /*Archetype*/, TWeakObjectPtr<class UObject> /*Descendant*/> ObjectLookupCache;
		TArray< const class UObjectBase* > PendingObjectCreations;
	};
}