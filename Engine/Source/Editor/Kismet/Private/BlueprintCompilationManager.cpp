// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "BlueprintCompilationManager.h"

#include "BlueprintEditorUtils.h"
#include "Blueprint/BlueprintSupport.h"
#include "CompilerResultsLog.h"
#include "Components/TimelineComponent.h"
#include "Engine/Engine.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/TimelineTemplate.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/KismetReinstanceUtilities.h"
#include "KismetCompiler.h"
#include "ProfilingDebugging/ScopedTimers.h"
#include "Serialization/ArchiveReplaceObjectRef.h"
#include "TickableEditorObject.h"
#include "UObject/UObjectHash.h"
#include "WidgetBlueprint.h"

class FFixupBytecodeReferences : public FArchiveUObject
{
public:
	FFixupBytecodeReferences(UObject* InObject)
	{
		ArIsObjectReferenceCollector = true;
		
		InObject->Serialize(*this);
	}

	FArchive& operator<<( UObject*& Obj )
	{
		if (Obj != NULL)
		{
			if(UClass* RelatedClass = Cast<UClass>(Obj))
			{
				UClass* NewClass = RelatedClass->GetAuthoritativeClass();
				ensure(NewClass);
				if(NewClass != RelatedClass)
				{
					Obj = NewClass;
				}
			}
			else if(UField* AsField = Cast<UField>(Obj))
			{
				UClass* OwningClass = AsField->GetOwnerClass();
				if(OwningClass)
				{
					UClass* NewClass = OwningClass->GetAuthoritativeClass();
					ensure(NewClass);
					if(NewClass != OwningClass)
					{
						// drill into new class finding equivalent object:
						TArray<FName> Names;
						UObject* Iter = Obj;
						while(Iter != OwningClass && Iter)
						{
							CA_SUPPRESS(6011); // warning C6011: Dereferencing NULL pointer 'CurrentPin'.
							Names.Add(Iter->GetFName());
							Iter = Iter->GetOuter();
						}

						UObject* Owner = NewClass;
						UObject* Match = nullptr;
						for(int32 I = Names.Num() - 1; I >= 0; --I)
						{
							UObject* Next = StaticFindObjectFast( UObject::StaticClass(), Owner, Names[I]);
							if( Next )
							{
								if(I == 0)
								{
									Match = Next;
								}
								else
								{
									Owner = Match;
								}
							}
							else
							{
								break;
							}
						}
						
						if(Match)
						{
							Obj = Match;
						}
					}
				}
			}
		}
		return *this;
	}
};

UClass* FastGenerateSkeletonClass(UBlueprint* BP, FKismetCompilerContext& CompilerContext)
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	FCompilerResultsLog MessageLog;

	UClass* ParentClass = BP->ParentClass;
	if(ParentClass->ClassGeneratedBy)
	{
		if(UBlueprint* ParentBP = Cast<UBlueprint>(ParentClass->ClassGeneratedBy))
		{
			if(ParentBP->SkeletonGeneratedClass)
			{
				ParentClass = ParentBP->SkeletonGeneratedClass;
			}
		}
	}

	ensure(BP->SkeletonGeneratedClass == nullptr);
	FName SkelClassName = *FString::Printf(TEXT("SKEL_%s_C"), *BP->GetName());
	UBlueprintGeneratedClass* Ret = NewObject<UBlueprintGeneratedClass>(BP->GetOutermost(), SkelClassName, RF_Public | RF_Transactional );
	Ret->ClassGeneratedBy = BP;
	
	const auto MakeFunction = [Ret, ParentClass, Schema, BP, &MessageLog](FName FunctionNameFName, UField**& InCurrentFieldStorageLocation, UField**& InCurrentParamStorageLocation, bool bIsStaticFunction, uint32 InFunctionFlags, const TArray<UK2Node_FunctionResult*>& ReturnNodes, const TArray<UEdGraphPin*>& InputPins) -> UFunction*
	{
		if(!ensure(FunctionNameFName != FName())
			|| FindObjectFast<UFunction>(Ret, FunctionNameFName, true ))
		{
			return nullptr;
		}
		
		UFunction* NewFunction = NewObject<UFunction>(Ret, FunctionNameFName, RF_Public);
					
		Ret->AddFunctionToFunctionMap(NewFunction);

		*InCurrentFieldStorageLocation = NewFunction;
		InCurrentFieldStorageLocation = &NewFunction->Next;

		if(bIsStaticFunction)
		{
			NewFunction->FunctionFlags |= FUNC_Static;
		}

		UFunction* ParentFn = ParentClass->FindFunctionByName(NewFunction->GetFName());
		if(ParentFn == nullptr)
		{
			// check for function in implemented interfaces:
			for(const FBPInterfaceDescription& BPID : BP->ImplementedInterfaces)
			{
				if(UFunction* ParentInterfaceFn = BPID.Interface->FindFunctionByName(NewFunction->GetFName()))
				{
					ParentFn = ParentInterfaceFn;
					break;
				}
			}
		}
		NewFunction->SetSuperStruct( ParentFn );
					
		InCurrentParamStorageLocation = &NewFunction->Children;

		// params:
		if(ParentFn)
		{
			NewFunction->FunctionFlags |= (ParentFn->FunctionFlags & (FUNC_FuncInherit | FUNC_Public | FUNC_Protected | FUNC_Private | FUNC_BlueprintPure));
			for (TFieldIterator<UProperty> PropIt(ParentFn); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
			{
				UProperty* ClonedParam = CastChecked<UProperty>(StaticDuplicateObject(*PropIt, NewFunction, PropIt->GetFName(), RF_AllFlags, nullptr, EDuplicateMode::Normal, EInternalObjectFlags::AllFlags & ~(EInternalObjectFlags::Native) ));
				ClonedParam->Next = nullptr;
				*InCurrentParamStorageLocation = ClonedParam;
				InCurrentParamStorageLocation = &ClonedParam->Next;
			}
		}
		else
		{
			NewFunction->FunctionFlags |= InFunctionFlags;
			for(UEdGraphPin* Pin : InputPins)
			{
				if(Pin->Direction == EEdGraphPinDirection::EGPD_Output && !Schema->IsExecPin(*Pin) && Pin->ParentPin == nullptr && Pin->GetName() != UK2Node_Event::DelegateOutputName)
				{
					UProperty* Param = FKismetCompilerUtilities::CreatePropertyOnScope(NewFunction, *(Pin->PinName), Pin->PinType, Ret, 0, Schema, MessageLog);
					Param->PropertyFlags |= CPF_Parm;
					if(Pin->PinType.bIsReference)
					{
						Param->PropertyFlags |= CPF_ReferenceParm | CPF_OutParm;
					}
					if(Pin->PinType.bIsConst)
					{
						Param->PropertyFlags |= CPF_ConstParm;
					}
					*InCurrentParamStorageLocation = Param;
					InCurrentParamStorageLocation = &Param->Next;
				}
			}
			
			if(ReturnNodes.Num() > 0)
			{
				// Gather all input pins on these nodes, these are 
				// the outputs of the function:
				TSet<FString> UsedPinNames;
				for(UK2Node_FunctionResult* Node : ReturnNodes)
				{
					for(UEdGraphPin* Pin : Node->Pins)
					{
						if(!Schema->IsExecPin(*Pin) && Pin->ParentPin == nullptr)
						{								
							if(!UsedPinNames.Contains(Pin->PinName))
							{
								UsedPinNames.Add(Pin->PinName);
							
								UProperty* Param = FKismetCompilerUtilities::CreatePropertyOnScope(NewFunction, *(Pin->PinName), Pin->PinType, Ret, 0, Schema, MessageLog);
								Param->PropertyFlags |= CPF_Parm|CPF_ReturnParm;
								*InCurrentParamStorageLocation = Param;
								InCurrentParamStorageLocation = &Param->Next;
							}
						}
					}
				}
			}
		}
		return NewFunction;
	};

	// helpers:
	const auto AddFunctionForGraphs = [Schema, &MessageLog, ParentClass, Ret, BP, MakeFunction](const TCHAR* FunctionNamePostfix, const TArray<UEdGraph*>& Graphs, UField**& InCurrentFieldStorageLocation, bool bIsStaticFunction)
	{
		for( const UEdGraph* Graph : Graphs )
		{
			TArray<UK2Node_FunctionEntry*> EntryNodes;
			Graph->GetNodesOfClass(EntryNodes);
			if(EntryNodes.Num() > 0)
			{
				TArray<UK2Node_FunctionResult*> ReturnNodes;
				Graph->GetNodesOfClass(ReturnNodes);
				UK2Node_FunctionEntry* EntryNode = EntryNodes[0];
				
				UField** CurrentParamStorageLocation = nullptr;
				UFunction* NewFunction = MakeFunction(
					FName(*(Graph->GetName() + FunctionNamePostfix)), 
					InCurrentFieldStorageLocation, 
					CurrentParamStorageLocation, 
					bIsStaticFunction, 
					EntryNode->GetFunctionFlags(), 
					ReturnNodes, 
					EntryNode->Pins
				);

				if(NewFunction)
				{
					// locals:
					for( const FBPVariableDescription& BPVD : EntryNode->LocalVariables )
					{
						UProperty* NewProperty = FKismetCompilerUtilities::CreatePropertyOnScope(NewFunction, BPVD.VarName, BPVD.VarType, Ret, 0, Schema, MessageLog);
						*CurrentParamStorageLocation = NewProperty;
						CurrentParamStorageLocation = &NewProperty->Next;
					}

					// __WorldContext:
					if(bIsStaticFunction && FindField<UObjectProperty>(NewFunction, TEXT("__WorldContext")) == nullptr)
					{
						FEdGraphPinType WorldContextPinType(Schema->PC_Object, FString(), UObject::StaticClass(), false, false, false, false, FEdGraphTerminalType());
						UProperty* Param = FKismetCompilerUtilities::CreatePropertyOnScope(NewFunction, TEXT("__WorldContext"), WorldContextPinType, Ret, 0, Schema, MessageLog);
						Param->PropertyFlags |= CPF_Parm;
						*CurrentParamStorageLocation = Param;
						CurrentParamStorageLocation = &Param->Next;
					}

					NewFunction->Bind();
					NewFunction->StaticLink(true);
				}
			}
		}
	};

	UField** CurrentFieldStorageLocation = &Ret->Children;

	Ret->SetSuperStruct(ParentClass);

	// link in delegate signatures, variables will reference these 
	AddFunctionForGraphs(HEADER_GENERATED_DELEGATE_SIGNATURE_SUFFIX, BP->DelegateSignatureGraphs, CurrentFieldStorageLocation, false);

	// handle event entry ponts (mostly custom events) - this replaces
	// the skeleton compile pass CreateFunctionStubForEvent call:
	TArray<UEdGraph*> AllEventGraphs;
	
	for (UEdGraph* UberGraph : BP->UbergraphPages)
	{
		AllEventGraphs.Add(UberGraph);
		UberGraph->GetAllChildrenGraphs(AllEventGraphs);
	}

	for( const UEdGraph* Graph : AllEventGraphs )
	{
		TArray<UK2Node_Event*> EventNodes;
		Graph->GetNodesOfClass(EventNodes);
		for( UK2Node_Event* Event : EventNodes )
		{
			FName EventNodeName = CompilerContext.GetEventStubFunctionName(Event);

			UField** CurrentParamStorageLocation = nullptr;

			UFunction* NewFunction = MakeFunction(
				EventNodeName, 
				CurrentFieldStorageLocation, 
				CurrentParamStorageLocation, 
				false, 
				Event->FunctionFlags|FUNC_BlueprintCallable, 
				TArray<UK2Node_FunctionResult*>(), 
				Event->Pins
			);

			if(NewFunction)
			{
				NewFunction->Bind();
				NewFunction->StaticLink(true);
			}
		}
	}

	UBlueprintGeneratedClass* OriginalNewClass = CompilerContext.NewClass;
	CompilerContext.NewClass = Ret;
	CompilerContext.bAssignDelegateSignatureFunction = true;
	CompilerContext.CreateClassVariablesFromBlueprint();
	CompilerContext.bAssignDelegateSignatureFunction = false;
	CompilerContext.NewClass = OriginalNewClass;
	UField* Iter = Ret->Children;
	while(Iter)
	{
		CurrentFieldStorageLocation = &Iter->Next;
		Iter = Iter->Next;
	}
	
	AddFunctionForGraphs(TEXT(""), BP->FunctionGraphs, CurrentFieldStorageLocation, BPTYPE_FunctionLibrary == BP->BlueprintType);

	Ret->Bind();
	Ret->StaticLink(/*bRelinkExistingProperties =*/true);

	return Ret;
}

// free function that we use to cross a module boundary (from CoreUObject to here)
void FlushReinstancingQueueImplWrapper();

struct FBlueprintCompilationManagerImpl : public FGCObject
{
	FBlueprintCompilationManagerImpl();
	virtual ~FBlueprintCompilationManagerImpl();

	// FGCObject:
	virtual void AddReferencedObjects(FReferenceCollector& Collector);

	void QueueForCompilation(UBlueprint* BPToCompile);
	void FlushCompilationQueueImpl();
	void FlushReinstancingQueueImpl();
	bool HasBlueprintsToCompile();
	static void ReinstanceBatch(TArray<TUniquePtr<FBlueprintCompileReinstancer>>& Reinstancers, TMap< UClass*, UClass* >& InOutOldToNewClassMap);

	TArray<UBlueprint*> QueuedBPs;
	TMap<UClass*, UClass*> ClassesToReinstance;
};

FBlueprintCompilationManagerImpl::FBlueprintCompilationManagerImpl()
{
	FBlueprintSupport::SetFlushReinstancingQueueFPtr(&FlushReinstancingQueueImplWrapper);
}

FBlueprintCompilationManagerImpl::~FBlueprintCompilationManagerImpl() 
{ 
	FBlueprintSupport::SetFlushReinstancingQueueFPtr(nullptr); 
}

void FBlueprintCompilationManagerImpl::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObjects(QueuedBPs);
}

void FBlueprintCompilationManagerImpl::QueueForCompilation(UBlueprint* BPToCompile)
{
	QueuedBPs.Add(BPToCompile);
}

static double GTimeCompiling = 0.f;
static double GTimeReinstancing = 0.f;

void FBlueprintCompilationManagerImpl::FlushCompilationQueueImpl()
{
	bool bDidSomething = QueuedBPs.Num() != 0;
	
	struct FCompilerData
	{
		FCompilerData(UBlueprint* InBP)
		{
			check(InBP);
			BP = InBP;
			ResultsLog = MakeUnique<FCompilerResultsLog>();
			ResultsLog->BeginEvent(TEXT("BlueprintCompilationManager Compile"));
			ResultsLog->SetSourcePath(InBP->GetPathName());
			Options.bRegenerateSkelton = false;
			Options.bReinstanceAndStubOnFailure = false;
			if( UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(BP))
			{
				Compiler = UWidgetBlueprint::GetCompilerForWidgetBP(WidgetBP, *ResultsLog, Options);
			}
			else
			{
				Compiler = FKismetCompilerContext::GetCompilerForBP(BP, *ResultsLog, Options);
			}
		}

		UBlueprint* BP;
		TUniquePtr<FCompilerResultsLog> ResultsLog;
		TUniquePtr<FKismetCompilerContext> Compiler;

		FKismetCompilerOptions Options;
	};
	
	TArray<FCompilerData> CurrentlyCompilingBPs;
	// Data only blueprints and interface blueprints can just go through a single fast skeleton path:
	TArray<FCompilerData> CurrentlyCompilingBPsSkeletonOnly;
	TArray<TUniquePtr<FBlueprintCompileReinstancer>> Reinstancers;
	{ // begin GTimeCompiling scope 
		FScopedDurationTimer SetupTimer(GTimeCompiling); 
	
		for(int32 I = 0; I < QueuedBPs.Num(); ++I)
		{
			// filter out data only and interface blueprints:
			bool bSkipCompile = false;
			UBlueprint* QueuedBP = QueuedBPs[I];
			if( FBlueprintEditorUtils::IsDataOnlyBlueprint(QueuedBP) )
			{
				const UClass* ParentClass = QueuedBP->ParentClass;
				if (ParentClass && ParentClass->HasAllClassFlags(CLASS_Native))
				{
					bSkipCompile = true;
				}
				else
				{
					const UClass* CurrentClass = QueuedBP->GeneratedClass;
					if(FStructUtils::TheSameLayout(CurrentClass, CurrentClass->GetSuperStruct()))
					{
						bSkipCompile = true;
					}
				}
			}
			else if (FBlueprintEditorUtils::IsInterfaceBlueprint(QueuedBP))
			{
				bSkipCompile = true;
			}

			if(bSkipCompile)
			{
				CurrentlyCompilingBPsSkeletonOnly.Add(FCompilerData(QueuedBP));
				QueuedBP->GeneratedClass->ClearFunctionMapsCaches();
				QueuedBPs.RemoveAtSwap(I);
				--I;
			}
			else
			{
				CurrentlyCompilingBPs.Add(FCompilerData(QueuedBP));
			}
		}
	
		QueuedBPs.Empty();

		// sort into sane compilation order. Not needed if we introduce compilation phases:
		auto HierarchyDepthSortFn = [](const FCompilerData& CompilerDataA, const FCompilerData& CompilerDataB)
		{
			UBlueprint& A = *(CompilerDataA.BP);
			UBlueprint& B = *(CompilerDataB.BP);
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
		};
		CurrentlyCompilingBPs.Sort( HierarchyDepthSortFn );
		CurrentlyCompilingBPsSkeletonOnly.Sort(HierarchyDepthSortFn);

		for (FCompilerData& CompilerData : CurrentlyCompilingBPs)
		{
			UBlueprint* BP = CompilerData.BP;
		
			// even ValidateVariableNames will trigger compilation if 
			// it thinks the BP is not being compiled:
			BP->bBeingCompiled = true;
			CompilerData.Compiler->ValidateVariableNames();
			BP->bBeingCompiled = false;
		}

		// purge null graphs, could be done only on load
		for (FCompilerData& CompilerData : CurrentlyCompilingBPs)
		{
			UBlueprint* BP = CompilerData.BP;
			UPackage* Package = Cast<UPackage>(BP->GetOutermost());
			bool bIsPackageDirty = Package ? Package->IsDirty() : false;

			FBlueprintEditorUtils::PurgeNullGraphs(BP);

			if( Package )
			{
				Package->SetDirtyFlag(bIsPackageDirty);
			}
		}

		// recompile skeleton
		for (FCompilerData& CompilerData : CurrentlyCompilingBPs)
		{
			UBlueprint* BP = CompilerData.BP;
			UPackage* Package = Cast<UPackage>(BP->GetOutermost());
			bool bIsPackageDirty = Package ? Package->IsDirty() : false;
		
			BP->SkeletonGeneratedClass = FastGenerateSkeletonClass(BP, *(CompilerData.Compiler) );

			if( Package )
			{
				Package->SetDirtyFlag(bIsPackageDirty);
			}
		}
		
		for (FCompilerData& CompilerData : CurrentlyCompilingBPsSkeletonOnly)
		{
			UBlueprint* BP = CompilerData.BP;
			UPackage* Package = Cast<UPackage>(BP->GetOutermost());
			bool bIsPackageDirty = Package ? Package->IsDirty() : false;
		
			BP->SkeletonGeneratedClass = FastGenerateSkeletonClass(BP, *(CompilerData.Compiler) );

			if( Package )
			{
				Package->SetDirtyFlag(bIsPackageDirty);
			}
		}

		// reconstruct nodes
		for (FCompilerData& CompilerData : CurrentlyCompilingBPs)
		{
			UBlueprint* BP = CompilerData.BP;
			UPackage* Package = Cast<UPackage>(BP->GetOutermost());
			bool bIsPackageDirty = Package ? Package->IsDirty() : false;

			// Setting bBeingCompiled to make sure that FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified
			// doesn't run skeleton compile while the compilation manager is compiling things:
			BP->bBeingCompiled = true;
			FBlueprintEditorUtils::ReconstructAllNodes(BP);
			FBlueprintEditorUtils::ReplaceDeprecatedNodes(BP);
			BP->bBeingCompiled = false;

			// we are regenerated, tag ourself as such so that
			// old logic to 'fix' circular dependencies doesn't
			// cause redundant regeneration (e.g. bForceRegenNodes
			// in ExpandTunnelsAndMacros):
			BP->bHasBeenRegenerated = true;
		
			if( Package )
			{
				Package->SetDirtyFlag(bIsPackageDirty);
			}
		}
	
		// reinstance every blueprint that is queued, note that this means classes in the hierarchy that are *not* being 
		// compiled will be parented to REINST versions of the class, so type checks (IsA, etc) involving those types
		// will be incoherent!
		TMap<UBlueprint*, UObject*> OldCDOs;
		{
			for (FCompilerData& CompilerData : CurrentlyCompilingBPs)
			{
				UBlueprint* BP = CompilerData.BP;
				UPackage* Package = Cast<UPackage>(BP->GetOutermost());
				bool bIsPackageDirty = Package ? Package->IsDirty() : false;

				OldCDOs.Add(BP, BP->GeneratedClass->ClassDefaultObject);
				Reinstancers.Emplace(TUniquePtr<FBlueprintCompileReinstancer>( 
					new FBlueprintCompileReinstancer(
						BP->GeneratedClass, 
						EBlueprintCompileReinstancerFlags::AutoInferSaveOnCompile|EBlueprintCompileReinstancerFlags::AvoidCDODuplication
					)
				));
			
				if( Package )
				{
					Package->SetDirtyFlag(bIsPackageDirty);
				}
			}
		}

		// Reinstancing done, lets fix up child->parent pointers:
		for (FCompilerData& CompilerData : CurrentlyCompilingBPs)
		{
			UBlueprint* BP = CompilerData.BP;
			if(BP->GeneratedClass->GetSuperClass()->HasAnyClassFlags(CLASS_NewerVersionExists))
			{
				BP->GeneratedClass->SetSuperStruct(BP->GeneratedClass->GetSuperClass()->GetAuthoritativeClass());
			}
		}

		// recompile every blueprint
		for (FCompilerData& CompilerData : CurrentlyCompilingBPs)
		{
			UBlueprint* BP = CompilerData.BP;
			UPackage* Package = Cast<UPackage>(BP->GetOutermost());
			bool bIsPackageDirty = Package ? Package->IsDirty() : false;

			ensure(BP->GeneratedClass->ClassDefaultObject == nullptr || 
				BP->GeneratedClass->ClassDefaultObject->GetClass() != BP->GeneratedClass);
			// default value propagation occurrs below:
			BP->GeneratedClass->ClassDefaultObject = nullptr;
			BP->bIsRegeneratingOnLoad = true;
			BP->bBeingCompiled = true;
			// Reset the flag, so if the user tries to use PIE it will warn them if the BP did not compile
			BP->bDisplayCompilePIEWarning = true;
		
			FKismetCompilerContext& CompilerContext = *(CompilerData.Compiler);
			CompilerContext.CompileClassLayout();
			BP->bIsRegeneratingOnLoad = false;

			if( Package )
			{
				Package->SetDirtyFlag(bIsPackageDirty);
			}
		}
	
		for (FCompilerData& CompilerData : CurrentlyCompilingBPs)
		{
			UBlueprint* BP = CompilerData.BP;
			UPackage* Package = Cast<UPackage>(BP->GetOutermost());
			bool bIsPackageDirty = Package ? Package->IsDirty() : false;

			ensure(BP->GeneratedClass->ClassDefaultObject == nullptr || 
				BP->GeneratedClass->ClassDefaultObject->GetClass() != BP->GeneratedClass);
			// default value propagation occurrs below:
			BP->GeneratedClass->ClassDefaultObject = nullptr;
			BP->bIsRegeneratingOnLoad = true;
		
			FKismetCompilerContext& CompilerContext = *(CompilerData.Compiler);
			CompilerContext.CompileFunctions();
		
			BP->bBeingCompiled = false;

			if (CompilerData.ResultsLog->NumErrors == 0)
			{
				// Blueprint is error free.  Go ahead and fix up debug info
				BP->Status = (0 == CompilerData.ResultsLog->NumWarnings) ? BS_UpToDate : BS_UpToDateWithWarnings;
			}
			else
			{
				BP->Status = BS_Error; // do we still have the old version of the class?
			}

			BP->bIsRegeneratingOnLoad = false;
			ensure(BP->GeneratedClass->ClassDefaultObject->GetClass() == *(BP->GeneratedClass));
		
			if( Package )
			{
				Package->SetDirtyFlag(bIsPackageDirty);
			}
		}
	} // end GTimeCompiling scope

	// and now we can finish the first stage of the reinstancing operation, moving old classes to new classes:
	{
		FScopedDurationTimer ReinstTimer(GTimeReinstancing);
		ReinstanceBatch(Reinstancers, ClassesToReinstance);
		
		// make sure no junk in bytecode, this can happen only for blueprints that were in QueuedBPsLocal because
		// the reinstancer can detect all other references (see UpdateBytecodeReferences):
		for (FCompilerData& CompilerData : CurrentlyCompilingBPs)
		{
			UBlueprint* BP = CompilerData.BP;
			for( TFieldIterator<UFunction> FuncIter(BP->GeneratedClass, EFieldIteratorFlags::ExcludeSuper); FuncIter; ++FuncIter )
			{
				UFunction* CurrentFunction = *FuncIter;
				if( CurrentFunction->Script.Num() > 0 )
				{
					FFixupBytecodeReferences ValidateAr(CurrentFunction);
				}
			}
		}
	}

	for (FCompilerData& CompilerData : CurrentlyCompilingBPs)
	{
		CompilerData.ResultsLog->EndEvent();
	}

	if(bDidSomething)
	{
		UE_LOG(LogBlueprint, Display, TEXT("Time Compiling: %f, Time Reinstancing: %f"),  GTimeCompiling, GTimeReinstancing);
		//GTimeCompiling = 0.0;
		//GTimeReinstancing = 0.0;
	}
	ensure(QueuedBPs.Num() == 0);
}

void FBlueprintCompilationManagerImpl::FlushReinstancingQueueImpl()
{
	// we can finalize reinstancing now:
	if(ClassesToReinstance.Num() == 0)
	{
		return;
	}

	{
		FScopedDurationTimer ReinstTimer(GTimeReinstancing);

		FBlueprintCompileReinstancer::BatchReplaceInstancesOfClass(ClassesToReinstance);
		ClassesToReinstance.Empty();
	}
	
	UE_LOG(LogBlueprint, Display, TEXT("Time Compiling: %f, Time Reinstancing: %f"),  GTimeCompiling, GTimeReinstancing);
}

bool FBlueprintCompilationManagerImpl::HasBlueprintsToCompile()
{
	return QueuedBPs.Num() != 0;
}

void FBlueprintCompilationManagerImpl::ReinstanceBatch(TArray<TUniquePtr<FBlueprintCompileReinstancer>>& Reinstancers, TMap< UClass*, UClass* >& InOutOldToNewClassMap)
{
	const auto FilterOutOfDateClasses = [](TArray<UClass*>& ClassList)
	{
		ClassList.RemoveAllSwap( [](UClass* Class) { return Class->HasAnyClassFlags(CLASS_NewerVersionExists); } );
	};

	const auto HasChildren = [FilterOutOfDateClasses](UClass* InClass) -> bool
	{
		TArray<UClass*> ChildTypes;
		GetDerivedClasses(InClass, ChildTypes, false);
		FilterOutOfDateClasses(ChildTypes);
		return ChildTypes.Num() > 0;
	};

	TSet<UClass*> ClassesToReparent;
	TSet<UClass*> ClassesToReinstance;

	// Reinstancers may contain *part* of a class hierarchy, so we first need to reparent any child types that 
	// haven't already been reinstanced:
	for (const TUniquePtr<FBlueprintCompileReinstancer>& Reinstancer : Reinstancers)
	{
		UClass* OldClass = Reinstancer->DuplicatedClass;
		InOutOldToNewClassMap.Add(Reinstancer->DuplicatedClass, Reinstancer->ClassToReinstance);
		if(!OldClass)
		{
			continue;
		}

		if(!HasChildren(OldClass))
		{
			continue;
		}

		bool bParentLayoutChanged = FStructUtils::TheSameLayout(OldClass, Reinstancer->ClassToReinstance);
		if(bParentLayoutChanged)
		{
			// we need *all* derived types:
			TArray<UClass*> ClassesToReinstanceList;
			GetDerivedClasses(OldClass, ClassesToReinstanceList);
			FilterOutOfDateClasses(ClassesToReinstanceList);
			
			for(UClass* ClassToReinstance : ClassesToReinstanceList)
			{
				ClassesToReinstance.Add(ClassToReinstance);
			}
		}
		else
		{
			// parent layout did not change, we can just relink the direct children:
			TArray<UClass*> ClassesToReparentList;
			GetDerivedClasses(OldClass, ClassesToReparentList, false);
			FilterOutOfDateClasses(ClassesToReparentList);
			
			for(UClass* ClassToReparent : ClassesToReparentList)
			{
				ClassesToReparent.Add(ClassToReparent);
			}
		}
	}

	for(UClass* Class : ClassesToReparent)
	{
		UClass** NewParent = InOutOldToNewClassMap.Find(Class->GetSuperClass());
		check(NewParent && *NewParent);
		Class->SetSuperStruct(*NewParent);
	}

	// make new hierarchy
	for(UClass* Class : ClassesToReinstance)
	{
		UObject* OriginalCDO = Class->ClassDefaultObject;
		//ensure(OriginalCDO);
		Reinstancers.Emplace(TUniquePtr<FBlueprintCompileReinstancer>( 
			new FBlueprintCompileReinstancer(
				Class, 
				EBlueprintCompileReinstancerFlags::AutoInferSaveOnCompile|EBlueprintCompileReinstancerFlags::AvoidCDODuplication
			)
		));

		// relink the new class:
		Reinstancers.Last()->ClassToReinstance->Bind();
		Reinstancers.Last()->ClassToReinstance->StaticLink(true);
	}

	// run UpdateBytecodeReferences:
	for (const TUniquePtr<FBlueprintCompileReinstancer>& Reinstancer : Reinstancers)
	{
		InOutOldToNewClassMap.Add( Reinstancer->DuplicatedClass, Reinstancer->ClassToReinstance );
			
		UBlueprint* CompiledBlueprint = UBlueprint::GetBlueprintFromClass(Reinstancer->ClassToReinstance);
		CompiledBlueprint->bIsRegeneratingOnLoad = true;
		Reinstancer->UpdateBytecodeReferences();

		CompiledBlueprint->bIsRegeneratingOnLoad = false;
	}
	
	// Now we can update templates and archetypes - note that we don't look for direct references to archetypes - doing
	// so is very expensive and it will be much faster to directly update anything that cares to cache direct references
	// to an archetype here (e.g. a UClass::ClassDefaultObject member):
	
	// 1. Sort classes so that most derived types are updated last - right now the only caller of this function
	// also sorts, but we don't want to make too many assumptions about caller. We could refine this API so that
	// we're not taking a raw list of reinstancers:
	Reinstancers.Sort(
		[](const TUniquePtr<FBlueprintCompileReinstancer>& CompilerDataA, const TUniquePtr<FBlueprintCompileReinstancer>& CompilerDataB)
		{
			UClass* A = CompilerDataA->ClassToReinstance;
			UClass* B = CompilerDataB->ClassToReinstance;
			int32 DepthA = 0;
			int32 DepthB = 0;
			UStruct* Iter = A ? A->GetSuperStruct() : nullptr;
			while (Iter)
			{
				++DepthA;
				Iter = Iter->GetSuperStruct();
			}

			Iter = B ? B->GetSuperStruct() : nullptr;
			while (Iter)
			{
				++DepthB;
				Iter = Iter->GetSuperStruct();
			}

			if (DepthA == DepthB && A && B)
			{
				return A->GetFName() < B->GetFName(); 
			}
			return DepthA < DepthB;
		}
	);

	// 2. Copy defaults from old CDO - CDO may be missing if this class was reinstanced and relinked here,
	// so use GetDefaultObject(true):
	for( const TUniquePtr<FBlueprintCompileReinstancer>& Reinstancer : Reinstancers )
	{
		UObject* OldCDO = Reinstancer->DuplicatedClass->ClassDefaultObject;
		if(OldCDO)
		{
			FBlueprintCompileReinstancer::CopyPropertiesForUnrelatedObjects(OldCDO,  Reinstancer->ClassToReinstance->GetDefaultObject(true));
			
			if (UBlueprintGeneratedClass* BPGClass = CastChecked<UBlueprintGeneratedClass>(Reinstancer->ClassToReinstance))
			{
				BPGClass->UpdateCustomPropertyListForPostConstruction();
			}
		}
	}

	TSet<UObject*> ObjectsThatShouldUseOldStuff;
	TMap<UObject*, UObject*> OldArchetypeToNewArchetype;

	// 3. Update any remaining instances that are tagged as RF_ArchetypeObject or RF_InheritableComponentTemplate - 
	// we may need to do further sorting to ensure that interdependent archetypes are initialized correctly:
	for( const TUniquePtr<FBlueprintCompileReinstancer>& Reinstancer : Reinstancers )
	{
		UClass* OldClass = Reinstancer->DuplicatedClass;
		if(ensure(OldClass))
		{
			TArray<UObject*> ArchetypeObjects;
			GetObjectsOfClass(OldClass, ArchetypeObjects, false);
			
			// filter out non-archetype instances, note that WidgetTrees and some component
			// archetypes do not have RF_ArchetypeObject or RF_InheritableComponentTemplate so
			// we simply detect that they are outered to a UBPGC or UBlueprint and assume that 
			// they are archetype objects in practice:
			ArchetypeObjects.RemoveAllSwap(
				[](UObject* Obj) 
				{ 
					bool bIsArchetype = 
						Obj->HasAnyFlags(RF_ArchetypeObject|RF_InheritableComponentTemplate)
						|| Obj->GetTypedOuter<UBlueprintGeneratedClass>()
						|| Obj->GetTypedOuter<UBlueprint>();
					// remove if this is not an archetype or its an archetype in the transient package:
					return !bIsArchetype || Obj->GetOutermost() == GetTransientPackage(); 
				}
			);

			// for each archetype:
			for(UObject* Archetype : ArchetypeObjects )
			{
				// move aside:
				FName OriginalName = Archetype->GetFName();
				UObject* OriginalOuter = Archetype->GetOuter();
				EObjectFlags OriginalFlags = Archetype->GetFlags();
				Archetype->Rename(
					nullptr,
					// destination - this is the important part of this call. Moving the object 
					// out of the way so we can reuse its name:
					GetTransientPackage(), 
					// Rename options:
					REN_DoNotDirty | REN_DontCreateRedirectors | REN_ForceNoResetLoaders );

				// reconstruct
				FMakeClassSpawnableOnScope TemporarilySpawnable(Reinstancer->ClassToReinstance);
				const EObjectFlags FlagMask = RF_Public | RF_ArchetypeObject | RF_Transactional | RF_Transient | RF_TextExportTransient | RF_InheritableComponentTemplate | RF_Standalone; //TODO: what about RF_RootSet?
				UObject* NewArchetype = NewObject<UObject>(OriginalOuter, Reinstancer->ClassToReinstance, OriginalName, OriginalFlags & FlagMask);

				// copy old data:
				FBlueprintCompileReinstancer::CopyPropertiesForUnrelatedObjects(Archetype, NewArchetype);

				OldArchetypeToNewArchetype.Add(Archetype, NewArchetype);
				ObjectsThatShouldUseOldStuff.Add(Archetype);

				Archetype->MarkPendingKill();
			}
		}
	}

	// 4. update known references to archetypes (e.g. component templates, WidgetTree). We don't want to run the normal 
	// reference finder to update these because searching the entire object graph is time consuming. Instead we just replace
	// all references in our UBlueprint and its generated class:
	TSet<UObject*> ArchetypeReferencers;
	for( const TUniquePtr<FBlueprintCompileReinstancer>& Reinstancer : Reinstancers )
	{
		ArchetypeReferencers.Add(Reinstancer->ClassToReinstance);
		ArchetypeReferencers.Add(Reinstancer->ClassToReinstance->ClassGeneratedBy);
		if(UBlueprint* BP = Cast<UBlueprint>(Reinstancer->ClassToReinstance->ClassGeneratedBy))
		{
			ensure(BP->bCachedDependenciesUpToDate);
			for(TWeakObjectPtr<UBlueprint> Dependendency : BP->CachedDependencies)
			{
				ArchetypeReferencers.Add(Dependendency.Get());
			}
		}
	}

	for(UObject* ArchetypeReferencer : ArchetypeReferencers)
	{
		FArchiveReplaceObjectRef< UObject > ReplaceAr(ArchetypeReferencer, OldArchetypeToNewArchetype, false, false, false);
	}
}

// Singleton boilperplate:
FBlueprintCompilationManagerImpl* BPCMImpl = nullptr;

void FlushReinstancingQueueImplWrapper()
{
	BPCMImpl->FlushReinstancingQueueImpl();
}

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
void FBlueprintCompilationManager::FlushCompilationQueue()
{
	if(BPCMImpl)
	{
		BPCMImpl->FlushCompilationQueueImpl();
	}
}

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


