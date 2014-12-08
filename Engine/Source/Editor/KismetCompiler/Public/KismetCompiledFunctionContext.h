// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EdGraphCompilerUtilities.h"
#include "BlueprintGraphDefinitions.h"
#include "BPTerminal.h"
#include "BlueprintCompiledStatement.h"

namespace KismetCompilerDebugOptions
{
	//@TODO: K2: Turning this off is probably broken due to state merging not working with the current code generation
	enum { DebuggingCompiler = 1 };

	// Should adjacent states be merged together?
	enum { MergeAdjacentStates = !DebuggingCompiler };

	// Should the compiler emit node comments to the backends?
	enum { EmitNodeComments = DebuggingCompiler };
}

enum ETerminalSpecification
{
	TS_Unspecified,
	TS_Literal,
	TS_ForcedShared,
};

//////////////////////////////////////////////////////////////////////////
// FKismetFunctionContext

struct FKismetFunctionContext
{
public:
	/** Blueprint source */
	UBlueprint* Blueprint;

	UEdGraph* SourceGraph;

	// The nominal function entry point
	UK2Node_FunctionEntry* EntryPoint;

	// The root set of nodes for the purposes of reachability, etc...
	TArray<UEdGraphNode*> RootSet;

	UFunction* Function;
	UBlueprintGeneratedClass* NewClass;

	// Linear execution schedule
	TArray<UEdGraphNode*> LinearExecutionList;

	FCompilerResultsLog& MessageLog;
	UEdGraphSchema_K2* Schema;

	// An UNORDERED listing of all statements (used for cleaning up the dynamically allocated statements)
	TArray< FBlueprintCompiledStatement* > AllGeneratedStatements;

	// Individual execution lists for every node that generated code to be consumed by the backend
	TMap< UEdGraphNode*, TArray<FBlueprintCompiledStatement*> > StatementsPerNode;

	// Goto fixup requests (each statement (key) wants to goto the first statement attached to the exec out-pin (value))
	TMap< FBlueprintCompiledStatement*, UEdGraphPin* > GotoFixupRequestMap;

	// Map from a net to an term (either a literal or a storage location)
	TIndirectArray<FBPTerminal> Parameters;
	TIndirectArray<FBPTerminal> Results;
	TIndirectArray<FBPTerminal> VariableReferences;
	TIndirectArray<FBPTerminal> PersistentFrameVariableReferences;
	TIndirectArray<FBPTerminal> Literals;
	TIndirectArray<FBPTerminal> Locals;
	TIndirectArray<FBPTerminal> EventGraphLocals;
	TIndirectArray<FBPTerminal>	LevelActorReferences;
	TMap<UEdGraphPin*, FBPTerminal*> NetMap;
	TMap<UEdGraphPin*, FBPTerminal*> LiteralHackMap;

	bool bIsUbergraph;
	bool bCannotBeCalledFromOtherKismet;
	bool bIsInterfaceStub;
	bool bIsConstFunction;
	bool bEnforceConstCorrectness;
	bool bCreateDebugData;
	bool bIsSimpleStubGraphWithNoParams;
	uint32 NetFlags;
	FName DelegateSignatureName;

	// If this function is an event stub, then this points to the node in the ubergraph that caused the stub to exist
	UEdGraphNode* SourceEventFromStubGraph;

	// Map from a name to the number of times it's been 'created' (same nodes create the same local variable names, so they need something appended)
	struct FNetNameMapping* NetNameMap;
	bool bAllocatedNetNameMap;

	// Stored calls of latent function (on current class), needed to tell if blueprint should be tickable
	TArray< UK2Node_CallFunction* > LatentFunctionCalls;

	bool bGeneratingCpp;
public:
	FKismetFunctionContext(FCompilerResultsLog& InMessageLog, UEdGraphSchema_K2* InSchema, UBlueprintGeneratedClass* InNewClass, UBlueprint* InBlueprint, bool bInGeneratingCpp);
	
	~FKismetFunctionContext();

	void SetExternalNetNameMap(FNetNameMapping* NewMap);

	bool IsValid() const
	{
		return (Function != NULL) && (EntryPoint != NULL) && (SourceGraph != NULL);
	}

	bool IsEventGraph() const
	{
		return bIsUbergraph;
	}

	void MarkAsEventGraph()
	{
		bIsUbergraph = true;
	}

	void MarkAsInternalOrCppUseOnly()
	{
		bCannotBeCalledFromOtherKismet = true;
	}

	bool CanBeCalledByKismet() const
	{
		return !bCannotBeCalledFromOtherKismet;
	}

	void MarkAsInterfaceStub()
	{
		bIsInterfaceStub = true;
	}

	void MarkAsConstFunction(bool bInEnforceConstCorrectness)
	{
		bIsConstFunction = true;
		bEnforceConstCorrectness = bInEnforceConstCorrectness;
	}

	bool IsInterfaceStub() const
	{
		return bIsInterfaceStub;
	}

	void SetDelegateSignatureName(FName InName)
	{
		check((DelegateSignatureName == NAME_None) && (InName != NAME_None));
		DelegateSignatureName = InName;
	}

	bool IsDelegateSignature()
	{
		return (DelegateSignatureName != NAME_None);
	}

	bool IsConstFunction() const
	{
		return bIsConstFunction;
	}

	bool EnforceConstCorrectness() const
	{
		return bEnforceConstCorrectness;
	}

	void MarkAsNetFunction(uint32 InFunctionFlags)
	{
		NetFlags = InFunctionFlags & (FUNC_NetFuncFlags);
	}

	uint32 GetNetFlags() const
	{
		return NetFlags;
	}

	FBPTerminal* RegisterLiteral(UEdGraphPin* Net)
	{
		FBPTerminal* Term = new (Literals) FBPTerminal();
		Term->CopyFromPin(Net, Net->DefaultValue);
		Term->ObjectLiteral = Net->DefaultObject;
		Term->TextLiteral = Net->DefaultTextValue;
		Term->bIsLiteral = true;
		return Term;
	}

	/** Returns a UStruct scope corresponding to the pin type passed in, if one exists */
	UStruct* GetScopeFromPinType(FEdGraphPinType& Type, UClass* SelfClass)
	{
		if ((Type.PinCategory == Schema->PC_Object) || (Type.PinCategory == Schema->PC_Class) || (Type.PinCategory == Schema->PC_Interface))
		{
			UClass* SubType = (Type.PinSubCategory == Schema->PSC_Self) ? SelfClass : Cast<UClass>(Type.PinSubCategoryObject.Get());
			return SubType;
		}
		else if (Type.PinCategory == Schema->PC_Struct)
		{
			UScriptStruct* SubType = Cast<UScriptStruct>(Type.PinSubCategoryObject.Get());
			return SubType;
		}
		else
		{
			return NULL;
		}
	}

	KISMETCOMPILER_API int32 GetContextUniqueID();

	UBlueprint* GetBlueprint()
	{
		return Blueprint;
	}

	/** Enqueue a statement to be executed when the specified Node is triggered */
	FBlueprintCompiledStatement& AppendStatementForNode(UEdGraphNode* Node)
	{
		FBlueprintCompiledStatement* Result = new FBlueprintCompiledStatement();
		AllGeneratedStatements.Add(Result);

		TArray<FBlueprintCompiledStatement*>& StatementList = StatementsPerNode.FindOrAdd(Node);
		StatementList.Add(Result);

		return *Result;
	}

	/** Prepends the statements corresponding to Source to the set of statements corresponding to Dest */
	void CopyAndPrependStatements(UEdGraphNode* Destination, UEdGraphNode* Source)
	{
		TArray<FBlueprintCompiledStatement*>* SourceStatementList = StatementsPerNode.Find(Source);
		if (SourceStatementList)
		{
			// Mapping of jump targets for when kismet compile statements want to jump to other statements
			TMap<FBlueprintCompiledStatement*, int32> JumpTargetIndexTable;

			TArray<FBlueprintCompiledStatement*>& TargetStatementList = StatementsPerNode.FindOrAdd(Destination);

			TargetStatementList.InsertUninitialized(0, SourceStatementList->Num());
			for (int32 i = 0; i < SourceStatementList->Num(); ++i)
			{
				FBlueprintCompiledStatement* CopiedStatement = new FBlueprintCompiledStatement();
				AllGeneratedStatements.Add(CopiedStatement);

				*CopiedStatement = *((*SourceStatementList)[i]);

				TargetStatementList[i] = CopiedStatement;

				// If the statement is a jump target, keep a mapping of it so we can fix it up after this loop
				if (CopiedStatement->bIsJumpTarget)
				{
					JumpTargetIndexTable.Add((*SourceStatementList)[i], i);
				}
			}

			// Loop through all statements and remap the target labels to the mapped ones
			for (int32 i = 0; i < TargetStatementList.Num(); i++)
			{
				FBlueprintCompiledStatement* Statement = TargetStatementList[i];
				int32* JumpTargetIdx = JumpTargetIndexTable.Find(Statement->TargetLabel);
				if (JumpTargetIdx)
				{
					Statement->TargetLabel = TargetStatementList[*JumpTargetIdx];
				}
			}
		}
		else
		{
			MessageLog.Warning(TEXT("A node that generated no code of it's own (@@) tried to inject code into @@"), Source, Destination);
		}
	}

	/** Returns true if Node generated code, and false otherwise */
	bool DidNodeGenerateCode(UEdGraphNode* Node)
	{
		TArray<FBlueprintCompiledStatement*>* SourceStatementList = StatementsPerNode.Find(Node);
		return (SourceStatementList != NULL) && (SourceStatementList->Num() > 0);
	}

	// Sorts the 'linear execution list' again by likely execution order; the list should only contain impure nodes by this point.
	void FinalSortLinearExecList()
	{
		// By this point, all of the pure data dependencies have been taken care of and the position in the list 
		// doesn't represent much.
		// Doing this sort will enable more gotos to be optimized away, and make the C++ backend generated code easier to read
		for (int32 TestIndex = 0; TestIndex < LinearExecutionList.Num(); )
		{
			UEdGraphNode* CurrentNode = LinearExecutionList[TestIndex];
			TArray<FBlueprintCompiledStatement*>* CurStatementList = StatementsPerNode.Find(CurrentNode);

			if ((CurStatementList == NULL) || (CurStatementList->Num() == 0))
			{
				// Node generated nothing for the backend to consume, remove it
				LinearExecutionList.RemoveAtSwap(TestIndex);
			}
			else
			{
				//@TODO: Actually do some real sorting here!
				//FBlueprintCompiledStatement& LastStatementInCurrentNode = *(CurStatementList->Last());
				//UEdGraphNode* JumpTargetNode = GotoFixupRequestMap.FindChecked(&LastStatementInCurrentNode);

				// Sort so the entry point node is always first
				if ((CurrentNode == EntryPoint) && (TestIndex > 0))
				{
					LinearExecutionList.SwapMemory(0, TestIndex);
				}
				else
				{
					TestIndex++;
				}
			}
		}
	}

	// Optimize out any useless jumps (jump to the very next statement, where the control flow can just fall through)
	void MergeAdjacentStates()
	{
		for (int32 i = 0; i < LinearExecutionList.Num(); ++i)
		{
			UEdGraphNode* CurrentNode = LinearExecutionList[i];
			TArray<FBlueprintCompiledStatement*>* CurStatementList = StatementsPerNode.Find(CurrentNode);

			if ((CurStatementList != NULL) && (CurStatementList->Num() > 0))
			{
				FBlueprintCompiledStatement& LastStatementInCurrentNode = *(CurStatementList->Last());
				if (LastStatementInCurrentNode.Type == KCST_UnconditionalGoto)
				{
					// Last statement is an unconditional goto; see if we can remove it

					// Find the next node that generated statements
					UEdGraphNode* NextNode = NULL;
					for (int32 j = i+1; j < LinearExecutionList.Num(); ++j)
					{
						UEdGraphNode* TestNode = LinearExecutionList[j];
						if (StatementsPerNode.Find(TestNode) != NULL)
						{
							NextNode = TestNode;
							break;
						}
					}

					if (NextNode != NULL)
					{
						UEdGraphNode* JumpTargetNode = NULL;

						UEdGraphPin const* JumpPin = GotoFixupRequestMap.FindChecked(&LastStatementInCurrentNode);
						if ((JumpPin != NULL) && (JumpPin->LinkedTo.Num() > 0))
						{
							JumpTargetNode = JumpPin->LinkedTo[0]->GetOwningNode();
						}

						// Is this node the target of the last statement in the previous node?
						if (NextNode == JumpTargetNode)
						{
							// Jump and label are adjacent, can optimize it out
							GotoFixupRequestMap.Remove(&LastStatementInCurrentNode);
							CurStatementList->RemoveAt(CurStatementList->Num() - 1);
						}
					}
				}
			}
		}
	}

	/**
	 * Makes sure an KCST_WireTraceSite is inserted before the specified 
	 * statement, and associates the specified pin with the inserted wire-trace 
	 * (so we can backwards engineer which pin triggered the goto).
	 * 
	 * @param  GotoStatement		The statement to insert a goto before.
	 * @param  AssociatedExecPin	The pin responsible for executing the goto.
	 */
	void InsertWireTrace(FBlueprintCompiledStatement* GotoStatement, UEdGraphPin* AssociatedExecPin)
	{
		// only need wire traces if we're debugging the blueprint is available (not for cooked builds)
		if (bCreateDebugData && (AssociatedExecPin != NULL))
		{
			UEdGraphNode* PreJumpNode = AssociatedExecPin->GetOwningNode();

			TArray<FBlueprintCompiledStatement*>* NodeStatementList = StatementsPerNode.Find(PreJumpNode);
			if (NodeStatementList != NULL)
			{
				// @TODO: this Find() is potentially costly (if the node initially generated a lot of statements)
				int32 GotoIndex = NodeStatementList->Find(GotoStatement);
				if (GotoIndex != INDEX_NONE)
				{
					FBlueprintCompiledStatement* PrevStatement = NULL;
					if (GotoIndex > 0)
					{
						PrevStatement = (*NodeStatementList)[GotoIndex - 1];
					}

					// if a wire-trace has already been inserted for us
					if ((PrevStatement != NULL) && (PrevStatement->Type == KCST_WireTraceSite))
					{
						PrevStatement->ExecContext = AssociatedExecPin;
					}
					else if (PrevStatement != NULL)
					{
						FBlueprintCompiledStatement* TraceStatement = new FBlueprintCompiledStatement();
						TraceStatement->Type        = KCST_WireTraceSite;
						TraceStatement->Comment     = PreJumpNode->NodeComment.IsEmpty() ? PreJumpNode->GetName() : PreJumpNode->NodeComment;
						TraceStatement->ExecContext = AssociatedExecPin;

						NodeStatementList->Insert(TraceStatement, GotoIndex);
						// AllGeneratedStatements is an unordered list, so it doesn't matter that we throw this at the end
						AllGeneratedStatements.Add(TraceStatement);
					}
				}
			}
		}
	}

	/** Resolves all pending goto fixups; Should only be called after all nodes have had a chance to generate code! */
	void ResolveGotoFixups()
	{
		/**
		 * This local lambda is intended to save on code duplication within this
		 * function. It tests the supplied statement to see if it is an event 
		 * entry point.
		 *
		 * @param GotoStatement		The compiled statement you wish to test.
		 * @return True if the passed statement is an event entry point, false if not.
		 */
		auto IsUberGraphEventStatement = [](FBlueprintCompiledStatement* GotoStatement) -> bool 
		{
			return ((GotoStatement->Type == KCST_CallFunction) && (GotoStatement->UbergraphCallIndex == 0));
		};

		if (bCreateDebugData)
		{
			// if we're debugging, go through an insert a wire trace before  
			// every "goto" statement so we can trace what execution pin a node
			// was executed from
			for (auto GotoIt = GotoFixupRequestMap.CreateIterator(); GotoIt; ++GotoIt)
			{
				FBlueprintCompiledStatement* GotoStatement = GotoIt.Key();
				if (IsUberGraphEventStatement(GotoStatement))
				{
					continue;
				}

				InsertWireTrace(GotoIt.Key(), GotoIt.Value());
			}
		}

		if (KismetCompilerDebugOptions::MergeAdjacentStates)
		{
			MergeAdjacentStates();
		}

		// Resolve the remaining fixups
		for (auto GotoIt = GotoFixupRequestMap.CreateIterator(); GotoIt; ++GotoIt)
		{
			FBlueprintCompiledStatement* GotoStatement = GotoIt.Key();

			UEdGraphPin const* ExecNet = GotoIt.Value();
			if (ExecNet == NULL)
			{
				// Execution flow ended here; pop the stack
				GotoStatement->Type = (GotoStatement->Type == KCST_GotoIfNot) ? KCST_EndOfThreadIfNot : KCST_EndOfThread;
				continue;
			}

			UEdGraphNode* TargetNode = NULL;
			if (IsUberGraphEventStatement(GotoStatement))
			{
				TargetNode = ExecNet->GetOwningNode();
			}
			else if (ExecNet->LinkedTo.Num() > 0)
			{
				TargetNode = ExecNet->LinkedTo[0]->GetOwningNode();
			}

			if (TargetNode == NULL)
			{
				// Execution flow ended here; pop the stack
				GotoStatement->Type = (GotoStatement->Type == KCST_GotoIfNot) ? KCST_EndOfThreadIfNot : KCST_EndOfThread;
			}
			else
			{
				// Try to resolve the goto
				TArray<FBlueprintCompiledStatement*>* StatementList = StatementsPerNode.Find(TargetNode);

				if ((StatementList == NULL) || (StatementList->Num() == 0))
				{
					MessageLog.Error(TEXT("Statement tried to pass control flow to a node @@ that generates no code"), TargetNode);
					GotoStatement->Type = KCST_CompileError;
				}
				else
				{
					// Wire up the jump target and notify the target that it is targeted
					FBlueprintCompiledStatement& FirstStatement = *((*StatementList)[0]);
					GotoStatement->TargetLabel = &FirstStatement;
					FirstStatement.bIsJumpTarget = true;
				}
			}
		}

		// Clear out the pending fixup map
		GotoFixupRequestMap.Empty();

		//@TODO: Remove any wire debug sites where the next statement is a stack pop
	}
	
	/** Looks for a pin of the given name, erroring if the pin is not found or if the direction doesn't match (doesn't verify the pin type) */
	UEdGraphPin* FindRequiredPinByName(const UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection RequiredDirection = EGPD_MAX)
	{
		for (int32 PinIndex = 0; PinIndex < Node->Pins.Num(); ++PinIndex)
		{
			UEdGraphPin* Pin = Node->Pins[PinIndex];

			if (Pin->PinName == PinName)
			{
				if ((Pin->Direction == RequiredDirection) || (RequiredDirection == EGPD_MAX))
				{
					return Pin;
				}
				else
				{
					MessageLog.Error(*FString::Printf(TEXT("Expected @@ to be an %s"), (RequiredDirection == EGPD_Output) ? TEXT("output") : TEXT("input")), Pin);
					return NULL;
				}
			}
		}

		MessageLog.Error(*FString::Printf(TEXT("Expected to find a pin named %s on @@"), *PinName), Node);
		return NULL;
	}

	/** Checks to see if a pin is of the requested type */
	bool ValidatePinType(const UEdGraphPin* Pin, const FEdGraphPinType& TestType)
	{
		if (Pin == NULL)
		{
			// No need to error, the previous call that tried to find a pin has already errored by this point
			return false;
		}
		else
		{
			if (Pin->PinType == TestType)
			{
				return true;
			}
			else
			{
				MessageLog.Error(*FString::Printf(TEXT("Expected @@ to %s instead of %s"), *Schema->TypeToText(TestType).ToString(), *Schema->TypeToText(Pin->PinType).ToString()), Pin);
				return false;
			}
		}
	}

	KISMETCOMPILER_API FBPTerminal* CreateLocalTerminalFromPinAutoChooseScope(UEdGraphPin* Net, const FString& NewName);
	KISMETCOMPILER_API FBPTerminal* CreateLocalTerminal(ETerminalSpecification Spec = ETerminalSpecification::TS_Unspecified);
};

//////////////////////////////////////////////////////////////////////////
