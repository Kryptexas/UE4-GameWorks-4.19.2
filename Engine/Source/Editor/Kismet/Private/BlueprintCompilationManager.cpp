// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "BlueprintCompilationManager.h"
#include "BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/KismetReinstanceUtilities.h"
#include "TickableEditorObject.h"

struct FBlueprintCompilationManagerImpl : public FGCObject, FTickableEditorObject
{
	virtual ~FBlueprintCompilationManagerImpl() {}

	// FGCObject:
	virtual void AddReferencedObjects(FReferenceCollector& Collector);

	// FTickableEditorObject
	virtual void Tick(float DeltaTime) override { TickCompilationQueue(); }
	virtual bool IsTickable() const override { return true; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(FBlueprintCompilationManagerImpl, STATGROUP_Tickables); }

	void QueueForCompilation(UBlueprint* BPToCompile);
	void TickCompilationQueue();
	bool HasBlueprintsToCompile();

	TArray<UBlueprint*> QueuedBPs;
};

void FBlueprintCompilationManagerImpl::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObjects(QueuedBPs);
}

void FBlueprintCompilationManagerImpl::QueueForCompilation(UBlueprint* BPToCompile)
{
	QueuedBPs.Add(BPToCompile);
}

void FBlueprintCompilationManagerImpl::TickCompilationQueue()
{
	TArray<UBlueprint*> QueuedBPsLocal = MoveTemp(QueuedBPs);
	QueuedBPsLocal.Sort(
		[](UBlueprint& A, UBlueprint& B)
	{
		int32 DepthA = 0;
		int32 DepthB = 0;
		UStruct* Iter = *(A.GeneratedClass) ? A.GeneratedClass->GetSuperStruct() : nullptr;
		while (Iter)
		{
			++DepthA;
			Iter = Iter->GetSuperStruct();
		}

		Iter = *(B.GeneratedClass) ? B.GeneratedClass->GetSuperStruct() : nullptr;
		while (Iter)
		{
			++DepthB;
			Iter = Iter->GetSuperStruct();
		}

		if (DepthA == DepthB)
		{
			return A.GetFName() < B.GetFName(); 
		}
		return DepthA < DepthB;
	}
	);

	// Hack - fixes an issue when compiling class hierarchies. Reinstancer recompiles children before they have
	// had their nodes reconstructed. Same for null graphs:
	for (UBlueprint* BP : QueuedBPsLocal)
	{
		UPackage* Package = Cast<UPackage>(BP->GetOutermost());
		bool bIsPackageDirty = Package ? Package->IsDirty() : false;

		FBlueprintEditorUtils::PurgeNullGraphs(BP);
		FBlueprintEditorUtils::ReconstructAllNodes(BP);

		if( Package )
		{
			Package->SetDirtyFlag(bIsPackageDirty);
		}
	}

	TArray<UObject*> ObjsLoaded;
	for (UBlueprint* BP : QueuedBPsLocal)
	{
		FBlueprintEditorUtils::RegenerateBlueprintClass(BP, BP->GeneratedClass, BP->GeneratedClass->ClassDefaultObject, ObjsLoaded);
	}
}

bool FBlueprintCompilationManagerImpl::HasBlueprintsToCompile()
{
	return QueuedBPs.Num() != 0;
}

// Singleton boilperplate:
FBlueprintCompilationManagerImpl* BPCMImpl = nullptr;

void FBlueprintCompilationManager::Initialize()
{
	if(!BPCMImpl)
	{
		BPCMImpl = new FBlueprintCompilationManagerImpl();
	}
}

void FBlueprintCompilationManager::Shutdown()
{
	delete BPCMImpl;
	BPCMImpl = nullptr;
}

// Forward to impl:
void FBlueprintCompilationManager::QueueForCompilation(UBlueprint* BPToCompile)
{
	check(BPToCompile->GetLinker());
	BPCMImpl->QueueForCompilation(BPToCompile);
}
	
void FBlueprintCompilationManager::NotifyBlueprintLoaded(UBlueprint* BPLoaded)
{
	// Blueprints can be loaded before editor modules are on line:
	if(!BPCMImpl)
	{
		FBlueprintCompilationManager::Initialize();
	}
	check(BPLoaded->GetLinker());
	BPCMImpl->QueueForCompilation(BPLoaded);
}


