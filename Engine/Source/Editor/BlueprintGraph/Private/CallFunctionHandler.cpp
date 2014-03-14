// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "EngineKismetLibraryClasses.h"
#include "EngineLevelScriptClasses.h"

#include "CallFunctionHandler.h"

#define LOCTEXT_NAMESPACE "CallFunctionHandler"

//////////////////////////////////////////////////////////////////////////
// FImportTextErrorContext

// Support class to pipe logs from UProperty->ImportText (for struct literals) to the message log as warnings
class FImportTextErrorContext : public FOutputDevice
{
protected:
	FCompilerResultsLog& MessageLog;
	UObject* TargetObject;
public:
	int32 NumErrors;

	FImportTextErrorContext(FCompilerResultsLog& InMessageLog, UObject* InTargetObject)
		: FOutputDevice()
		, MessageLog(InMessageLog)
		, TargetObject(InTargetObject)
		, NumErrors(0)
	{
	}

	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category) OVERRIDE
	{
		if (TargetObject == NULL)
		{
			MessageLog.Error(V);
		}
		else
		{
			const FString ErrorString = FString::Printf(TEXT("Invalid default on node @@: %s"), V);
			MessageLog.Error(*ErrorString, TargetObject);		
		}
		NumErrors++;
	}
};

//////////////////////////////////////////////////////////////////////////
// FKCHandler_CallFunction

#if _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4750)
#endif

/**
 * Searches for the function referenced by a graph node in the CallingContext class's list of functions,
 * validates that the wiring matches up correctly, and creates an execution statement.
 */
void FKCHandler_CallFunction::CreateFunctionCallStatement(FKismetFunctionContext& Context, UEdGraphNode* Node, UEdGraphPin* SelfPin)
{
	int32 NumErrorsAtStart = CompilerContext.MessageLog.NumErrors;

	// Find the function, starting at the parent class
	UFunction* Function = FindFunction(Context, Node);
	
	if (Function != NULL)
	{
		CheckIfFunctionIsCallable(Function, Context, Node);
		// Make sure the pin mapping is sound (all pins wire up to a matching function parameter, and all function parameters match a pin)

		// Remaining unmatched pins
		TArray<UEdGraphPin*> RemainingPins;
		RemainingPins.Append(Node->Pins);

		// Remove expected exec and self pins
		//@TODO: Check to make sure there is exactly one exec in and one exec out, as well as one self pin
		for (int32 i = 0; i < RemainingPins.Num(); )
		{
			if (CompilerContext.GetSchema()->IsMetaPin(*RemainingPins[i]))
			{
				RemainingPins.RemoveAtSwap(i);
			}
			else
			{
				++i;
			}
		}

		// Check for magic pins
		const bool bIsLatent = Function->HasMetaData(FBlueprintMetadata::MD_Latent);
		if (bIsLatent && (CompilerContext.UbergraphContext != &Context))
		{
			CompilerContext.MessageLog.Error(*LOCTEXT("ContainsLatentCall_Error", "@@ contains a latent call, which cannot exist outside of the event graph").ToString(), Node);
		}

		// Check access specifier
		const uint32 AccessSpecifier = Function->FunctionFlags & FUNC_AccessSpecifiers;
		if(FUNC_Private == AccessSpecifier)
		{
			if(Function->GetOuter() != Context.NewClass)
			{
				CompilerContext.MessageLog.Warning(*LOCTEXT("PrivateFunctionCall_Error", "Function @@ is private and cannot be called outside its class").ToString(), Node);
			}
		}
		else if(FUNC_Protected == AccessSpecifier)
		{
			if( !Context.NewClass->IsChildOf( Cast<UStruct>( Function->GetOuter() ) ) )
			{
				CompilerContext.MessageLog.Warning(*LOCTEXT("ProtectedFunctionCall_Error", "Function @@ is protected and can be called only from its class or subclasses").ToString(), Node);
			}
		}

		UEdGraphPin* LatentInfoPin = NULL;

		TMap<FName, FString>* MetaData = UMetaData::GetMapForObject(Function);
		if (MetaData != NULL)
		{
			for (TMap<FName, FString>::TConstIterator It(*MetaData); It; ++It)
			{
				const FName& Key = It.Key();

				if (Key == TEXT("LatentInfo"))
				{
					UEdGraphPin* Pin = Node->FindPin(It.Value());
					if( (Pin != NULL) && (Pin->Direction == EGPD_Input) && (Pin->LinkedTo.Num() == 0))
					{
						LatentInfoPin = Pin;

						UEdGraphPin* PinToTry = FEdGraphUtilities::GetNetFromPin(Pin);
						FBPTerminal** Term = Context.NetMap.Find(PinToTry);
						if(Term != NULL)
						{
							check((*Term)->bIsLiteral);
						
							const int32 UUID = Context.GetContextUniqueID();
							const FString ExecutionFunctionName = CompilerContext.GetSchema()->FN_ExecuteUbergraphBase.ToString() + TEXT("_") + Context.Blueprint->GetName();
							(*Term)->Name = FString::Printf(TEXT("(Linkage=%s,UUID=%s,ExecutionFunction=%s,CallbackTarget=None)"), *FString::FromInt(INDEX_NONE), *FString::FromInt(UUID), *ExecutionFunctionName);

							// Record the UUID in the debugging information
							UEdGraphNode* TrueSourceNode = Cast<UEdGraphNode>(Context.MessageLog.FindSourceObject(Node));
							Context.NewClass->GetDebugData().RegisterUUIDAssociation(TrueSourceNode, UUID);
						}
					}
					else
					{
						CompilerContext.MessageLog.Error(*FString::Printf(*LOCTEXT("FindPinFromLinkage_Error", "Function %s (called from @@) was specified with LatentInfo metadata but does not have a pin named %s").ToString(),
							*(Function->GetName()), *(It.Value())), Node);
					}
				}
			}
		}

		// Parameter info to be stored, and assigned to all function call statements generated below
		FBPTerminal* LHSTerm = NULL;
		TArray<FBPTerminal*> RHSTerms;
		UEdGraphNode* LatentTargetNode = NULL;
		int32 LatentTargetParamIndex = INDEX_NONE;

		// Grab the special case structs that use their own literal path
		UScriptStruct* VectorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Vector"));
		UScriptStruct* RotatorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Rotator"));
		UScriptStruct* TransformStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Transform"));

		// Check each property
		bool bMatchedAllParams = true;
		for (TFieldIterator<UProperty> It(Function); It && (It->PropertyFlags & CPF_Parm); ++It)
		{
			UProperty* Property = *It;

			bool bFoundParam = false;
			for (int32 i = 0; !bFoundParam && (i < RemainingPins.Num()); ++i)
			{
				UEdGraphPin* PinMatch = RemainingPins[i];
				if (FCString::Stricmp(*Property->GetName(), *PinMatch->PinName) == 0)
				{
					// Found a corresponding pin, does it match in type and direction?
					if (FKismetCompilerUtilities::IsTypeCompatibleWithProperty(PinMatch, Property, CompilerContext.MessageLog, CompilerContext.GetSchema(), Context.NewClass))
					{
						UEdGraphPin* PinToTry = FEdGraphUtilities::GetNetFromPin(PinMatch);
						FBPTerminal** Term = Context.NetMap.Find(PinToTry);

						if (Term != NULL)
						{
							// For literal structs, we have to verify the default here to make sure that it has valid formatting
							if( (*Term)->bIsLiteral && (PinMatch != LatentInfoPin))
							{
								UStructProperty* StructProperty = Cast<UStructProperty>(Property);
								if( StructProperty )
								{
									UScriptStruct* Struct = StructProperty->Struct;
									if( Struct != VectorStruct
										&& Struct != RotatorStruct
										&& Struct != TransformStruct )
									{
										// Ensure all literal struct terms can be imported if its empty
										if ( (*Term)->Name.IsEmpty() )
										{
											(*Term)->Name = TEXT("()");
										}

										int32 StructSize = Struct->GetStructureSize();
										uint8* StructData = (uint8*)FMemory_Alloca(StructSize);
										StructProperty->InitializeValue(StructData);

										// Import the literal text to a dummy struct to verify it's well-formed
										FImportTextErrorContext ErrorPipe(CompilerContext.MessageLog, Node);
										StructProperty->ImportText(*((*Term)->Name), StructData, 0, NULL, &ErrorPipe);
										if(ErrorPipe.NumErrors > 0)
										{
											bMatchedAllParams = false;
										}
									}
									
								}
							}

							if (Property->HasAnyPropertyFlags(CPF_ReturnParm))
							{
								LHSTerm = *Term;
							}
							else
							{
								int32 ParameterIndex = RHSTerms.Add(*Term);

								if (PinMatch == LatentInfoPin)
								{
									// Record the (latent) output impulse from this node
									UEdGraphPin* ThenExecPin = CompilerContext.GetSchema()->FindExecutionPin(*Node, EGPD_Output);

									if( (ThenExecPin != NULL) && (ThenExecPin->LinkedTo.Num() > 0) )
									{
										LatentTargetNode = ThenExecPin->LinkedTo[0]->GetOwningNode();
									}

									if( LatentTargetNode != NULL )
									{
										LatentTargetParamIndex = ParameterIndex;
									}
								}
							}

							// Make sure it isn't trying to modify a const term
							if (Property->HasAnyPropertyFlags(CPF_OutParm) && !((*Term)->IsTermWritable()))
							{
								if (Property->HasAnyPropertyFlags(CPF_ReferenceParm))
								{
									if (!Property->HasAnyPropertyFlags(CPF_ConstParm))
									{
										CompilerContext.MessageLog.Error(*LOCTEXT("PassReadOnlyReferenceParam_Error", "Cannot pass a read-only variable to a reference parameter @@").ToString(), PinMatch);
									}
								}
								else
								{
									CompilerContext.MessageLog.Error(*LOCTEXT("PassReadOnlyOutputParam_Error", "Cannot pass a read-only variable to a output parameter @@").ToString(), PinMatch);
								}
							}
						}
						else
						{
							CompilerContext.MessageLog.Error(*LOCTEXT("ResolveTermPassed_Error", "Failed to resolve term passed into @@").ToString(), PinMatch);
							bMatchedAllParams = false;
						}
					}
					else
					{
						bMatchedAllParams = false;
					}

					bFoundParam = true;
					RemainingPins.RemoveAtSwap(i);
				}
			}

			if (!bFoundParam)
			{
				CompilerContext.MessageLog.Error(*FString::Printf(*LOCTEXT("FindPinParameter_Error", "Could not find a pin for the parameter %s of %s on @@").ToString(), *Property->GetName(), *Function->GetName()), Node);
				bMatchedAllParams = false;
			}
		}

		// At this point, we should have consumed all pins.  If not, there are extras that need to be removed.
		for (int32 i = 0; i < RemainingPins.Num(); ++i)
		{
			CompilerContext.MessageLog.Error(*FString::Printf(*LOCTEXT("PinMismatchParameter_Error", "Pin @@ named %s doesn't match any parameters of function %s").ToString(), *RemainingPins[i]->PinName, *Function->GetName()), RemainingPins[i]);
		}

		if (NumErrorsAtStart == CompilerContext.MessageLog.NumErrors)
		{
			// Build up a list of contexts that this function will be called on
			TArray<FBPTerminal*> ContextTerms;
			if (SelfPin != NULL)
			{
				if( SelfPin->LinkedTo.Num() > 0 )
				{
					for(int32 i = 0; i < SelfPin->LinkedTo.Num(); i++)
					{
						FBPTerminal** pContextTerm = Context.NetMap.Find(SelfPin->LinkedTo[i]);
						ContextTerms.Add((pContextTerm != NULL) ? *pContextTerm : NULL);
					}
				}
				else
				{
					FBPTerminal** pContextTerm = Context.NetMap.Find(SelfPin);
					ContextTerms.Add((pContextTerm != NULL) ? *pContextTerm : NULL);
				}
			}

			// Check for a call into the ubergraph, which will require a patchup later on for the exact state entry point
			UEdGraphNode** pSrcEventNode = NULL;
			if (!bIsLatent)
			{
				pSrcEventNode = CompilerContext.CallsIntoUbergraph.Find(Node);
			}

			// Iterate over all the contexts this functions needs to be called on, and emit a call function statement for each
			for (auto TargetListIt = ContextTerms.CreateIterator(); TargetListIt; ++TargetListIt)
			{
				FBPTerminal* Target = *TargetListIt;

				FBlueprintCompiledStatement& Statement = Context.AppendStatementForNode(Node);
				Statement.FunctionToCall = Function;
				Statement.FunctionContext = Target;
				Statement.Type = KCST_CallFunction;
				Statement.bIsInterfaceContext = IsCalledFunctionFromInterface(Node);
				Statement.bIsParentContext = IsCalledFunctionFinal(Node);

				Statement.LHS = LHSTerm;
				Statement.RHS = RHSTerms;

				if (!bIsLatent)
				{
					// Fixup ubergraph calls
					if (pSrcEventNode != NULL)
					{
						check(CompilerContext.UbergraphContext);
						CompilerContext.UbergraphContext->GotoFixupRequestMap.Add(&Statement, *pSrcEventNode);
						Statement.UbergraphCallIndex = 0;
					}
				}
				else
				{
					// Fixup latent functions
					if ((LatentTargetNode != NULL) && (Target == ContextTerms.Last()))
					{
						check(LatentTargetParamIndex != INDEX_NONE);
						Statement.UbergraphCallIndex = LatentTargetParamIndex;
						Context.GotoFixupRequestMap.Add(&Statement, LatentTargetNode);
					}
				}

				// Check to see if we need to fix up any terms for array property coersion
				if( UK2Node_CallArrayFunction* ArrayNode = Cast<UK2Node_CallArrayFunction>(Node) )
				{
					TArray< FArrayPropertyPinCombo > ArrayPinInfo;
					ArrayNode->GetArrayPins(ArrayPinInfo);

					for(auto Iter = ArrayPinInfo.CreateConstIterator(); Iter; ++Iter)
					{
						UEdGraphPin* ArrayPropPin = Iter->ArrayPropPin;
						UEdGraphPin* ArrayTargetPin = Iter->ArrayPin;
						if( ArrayPropPin && ArrayTargetPin )
						{
							// Find the array property literal term, used for specifying the type of TargetArray
							for( int32 i = 0; i < Context.Literals.Num(); i++ )
							{
								FBPTerminal& Term = Context.Literals[i];
								if( Term.Source == ArrayPropPin )
								{
									// Now, map the array property literal term to the TargetArray term.  The AssociatedVarProperty will later be filled in as the array prop's object literal in the backend
									UEdGraphPin* Net = FEdGraphUtilities::GetNetFromPin(ArrayTargetPin);
									FBPTerminal** TargetTerm = Context.NetMap.Find(Net);
									if( TargetTerm )
									{
										Statement.ArrayCoersionTermMap.Add(&Term, *TargetTerm);
									}
								}
							}
						}
					}
				}

				AdditionalCompiledStatementHandling(Context, Node, Statement);
			}

			// Create the exit from this node if there is one
			if (bIsLatent)
			{
				// End this thread of execution; the latent function will resume it at some point in the future
				FBlueprintCompiledStatement& PopStatement = Context.AppendStatementForNode(Node);
				PopStatement.Type = KCST_EndOfThread;
			}
			else
			{
				// Generate the output impulse from this node
				if (!IsCalledFunctionPure(Node))
				{
					GenerateSimpleThenGoto(Context, *Node);
				}
			}
		}
	}
	else
	{
		FString WarningMessage = FString::Printf(*LOCTEXT("FindFunction_Error", "Could not find the function '%s' called from @@").ToString(), *GetFunctionNameFromNode(Node));
		CompilerContext.MessageLog.Warning(*WarningMessage, Node);
	}
}

UClass* FKCHandler_CallFunction::GetCallingContext(FKismetFunctionContext& Context, UEdGraphNode* Node)
{
	// Find the calling scope
	UClass* SearchScope = Context.NewClass;
	UK2Node_CallFunction* CallFuncNode = Cast<UK2Node_CallFunction>(Node);
	if ((CallFuncNode != NULL) && CallFuncNode->bIsFinalFunction)
	{
		if (UK2Node_CallParentFunction* ParentCall = Cast<UK2Node_CallParentFunction>(Node))
		{
			// Special Case:  super call functions should search up their class hierarchy, and find the first legitimate implementation of the function
			const FName FuncName = CallFuncNode->FunctionReference.GetMemberName();
			UClass* SearchContext = Context.NewClass->GetSuperClass();

			UFunction* ParentFunc = NULL;
			if (SearchContext != NULL)
			{
				ParentFunc = SearchContext->FindFunctionByName(FuncName);
			}

			return ParentFunc ? ParentFunc->GetOuterUClass() : NULL;
		}
		else
		{
			// Final functions need the call context to be the specified class, so don't bother checking for the self pin.   The schema should enforce this.
			return CallFuncNode->FunctionReference.GetMemberParentClass(CallFuncNode);
		}
	}
	else
	{
		UEdGraphPin* SelfPin = CompilerContext.GetSchema()->FindSelfPin(*Node, EGPD_Input);
		if (SelfPin != NULL)
		{
			SearchScope = Cast<UClass>(Context.GetScopeFromPinType(SelfPin->PinType, Context.NewClass));
		}
	}

	return SearchScope;
}

UClass* FKCHandler_CallFunction::GetTrueCallingClass(FKismetFunctionContext& Context, UEdGraphPin* SelfPin)
{
	if (SelfPin != NULL)
	{
		// TODO: here FBlueprintCompiledStatement::GetScopeFromPinType should be called, but since FEdGraphPinType::PinSubCategory is not always initialized properly that function works wrong
		// return Cast<UClass>(Context.GetScopeFromPinType(SelfPin->PinType, Context.NewClass));
		FEdGraphPinType& Type = SelfPin->PinType;
		if ((Type.PinCategory == CompilerContext.GetSchema()->PC_Object) || (Type.PinCategory == CompilerContext.GetSchema()->PC_Class))
		{
			if (!Type.PinSubCategory.IsEmpty() && (Type.PinSubCategory != CompilerContext.GetSchema()->PSC_Self))
			{
				return Cast<UClass>(Type.PinSubCategoryObject.Get());
			}
		}
	}
	return Context.NewClass;
}

void FKCHandler_CallFunction::RegisterNets(FKismetFunctionContext& Context, UEdGraphNode* Node)
{
	UFunction* Function = FindFunction(Context, Node);

	if (Function != NULL)
	{

		// Auto-created term defaults
		TArray<FString> AutoCreateRefTermPinNames;
		const bool bHasAutoCreateRefTerms = Function->HasMetaData(FBlueprintMetadata::MD_AutoCreateRefTerm);
		if (bHasAutoCreateRefTerms)
		{
			CompilerContext.GetSchema()->GetAutoEmitTermParameters(Function, AutoCreateRefTermPinNames);
		}

		TArray<FString> DefaultToSelfParamNames;
		TArray<FString> RequiresSetValue;

		if (Function->HasMetaData(FBlueprintMetadata::MD_DefaultToSelf))
		{
			FString const DefaltToSelfPinName = Function->GetMetaData(FBlueprintMetadata::MD_DefaultToSelf);
			AutoCreateRefTermPinNames.Remove(DefaltToSelfPinName);

			if (Context.Blueprint->ParentClass->HasMetaData(FBlueprintMetadata::MD_ShowHiddenSelfPins))
			{
				RequiresSetValue.Add(DefaltToSelfPinName);
			}
			else
			{
				DefaultToSelfParamNames.Add(DefaltToSelfPinName);
			}
		}
		if (Function->HasMetaData(FBlueprintMetadata::MD_WorldContext))
		{
			const bool bHasIntrinsicWorldContext = Context.Blueprint->ParentClass->GetDefaultObject()->ImplementsGetWorld();

			FString const WorldContextPinName = Function->GetMetaData(FBlueprintMetadata::MD_WorldContext);
			AutoCreateRefTermPinNames.Remove(WorldContextPinName);

			if (bHasIntrinsicWorldContext)
			{
				DefaultToSelfParamNames.Add(WorldContextPinName);
			}
			else
			{
				RequiresSetValue.Add(WorldContextPinName);
			}
		}

		for (auto It = Node->Pins.CreateConstIterator(); It; ++It)
		{
			UEdGraphPin* Pin = (*It);
			const bool bIsConnected = (Pin->LinkedTo.Num() != 0);

			// if this pin could use a default (it doesn't have a connection or default of its own)
			if (!bIsConnected && (Pin->DefaultObject == NULL))
			{
				if (DefaultToSelfParamNames.Contains(Pin->PinName))
				{
					ensure(Pin->PinType.PinSubCategoryObject != NULL);
					ensure(Pin->PinType.PinCategory == CompilerContext.GetSchema()->PC_Object);

					FBPTerminal* Term = Context.RegisterLiteral(Pin);
					Term->Type.PinSubCategory = CompilerContext.GetSchema()->PN_Self;
					Context.NetMap.Add(Pin, Term);
				}
				else if (RequiresSetValue.Contains(Pin->PinName))
				{
					CompilerContext.MessageLog.Error(*NSLOCTEXT("KismetCompiler", "PinMustHaveConnection_Error", "Pin @@ must have a connection").ToString(), Pin);
				}
			}

			// Check for default term for ref parameters
			if (bHasAutoCreateRefTerms && (AutoCreateRefTermPinNames.Contains(Pin->PinName)))
			{
				if (Pin->PinType.bIsReference
					&& !CompilerContext.GetSchema()->IsMetaPin(*Pin)
					&& !CompilerContext.GetSchema()->IsSelfPin(*Pin)
					&& (Pin->Direction == EGPD_Input)
					&& !bIsConnected
					&& (Pin->PinType.bIsArray || !Pin->DefaultValue.IsEmpty() || Pin->DefaultObject))
				{
					// These terms have to be class members, because they hold default values.  If we don't have a valid ubergraph context, we've already failed, so no use creating the term
					if (CompilerContext.UbergraphContext != NULL)
					{
						check(CompilerContext.UbergraphContext->NetNameMap);
						FBPTerminal* Term = new (Context.EventGraphLocals) FBPTerminal();
						Term->CopyFromPin(Pin, *CompilerContext.UbergraphContext->NetNameMap->MakeValidName(Pin));
						Term->Name += TEXT("_RefProperty");
						Term->bIsLocal = true;
						if (!Pin->PinType.bIsArray)
						{
							Term->PropertyDefault = Pin->DefaultObject ? Pin->DefaultObject->GetFullName() : Pin->DefaultValue;
						}

						Context.NetMap.Add(Pin, Term);
					}
					else
					{
						CompilerContext.MessageLog.Error(*NSLOCTEXT("KismetCompiler", "PinMustHaveConnection_Error", "Pin @@ must have a connection").ToString(), Pin);
					}
				}
			}
		}
	}

	FNodeHandlingFunctor::RegisterNets(Context, Node);
}

void FKCHandler_CallFunction::RegisterNet(FKismetFunctionContext& Context, UEdGraphPin* Net)
{
	// This net is an output from a function call
	FBPTerminal* Term = new (Context.IsEventGraph() ? Context.EventGraphLocals : Context.Locals) FBPTerminal();

	FString NetName = Context.NetNameMap->MakeValidName(Net);

	Term->CopyFromPin(Net, NetName);

	Context.NetMap.Add(Net, Term);
}

UFunction* FKCHandler_CallFunction::FindFunction(FKismetFunctionContext& Context, UEdGraphNode* Node)
{
	UClass* CallingContext = GetCallingContext(Context, Node);
	FString FunctionName = GetFunctionNameFromNode(Node);

	return CallingContext->FindFunctionByName(*FunctionName);
}

void FKCHandler_CallFunction::Transform(FKismetFunctionContext& Context, UEdGraphNode* Node)
{
	// Add an object reference pin for this call
	//UEdGraphPin* OperatingOn = Node->CreatePin(EGPD_Input, Schema->PC_Object, TEXT(""), TEXT("OperatingContext"));

	if (IsCalledFunctionPure(Node))
	{
		// Flag for removal if pure and there are no consumers of the outputs
		//@TODO: This isn't recursive (and shouldn't be here), it'll just catch the last node in a line of pure junk
		bool bAnyOutputsUsed = false;
		for (int32 PinIndex = 0; PinIndex < Node->Pins.Num(); ++PinIndex)
		{
			UEdGraphPin* Pin = Node->Pins[PinIndex];
			if ((Pin->Direction == EGPD_Output) && (Pin->LinkedTo.Num() > 0))
			{
				bAnyOutputsUsed = true;
				break;
			}
		}

		if (!bAnyOutputsUsed)
		{
			//@TODO: Remove this node, not just warn about it

		}
	}

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Find the function, starting at the parent class
	UFunction* Function = FindFunction(Context, Node);
	if ((Function != NULL) && (Function->HasMetaData(FBlueprintMetadata::MD_Latent)))
	{
		UK2Node_CallFunction* CallFuncNode = CastChecked<UK2Node_CallFunction>(Node);
		UEdGraphPin* OldOutPin = K2Schema->FindExecutionPin(*CallFuncNode, EGPD_Output);

		if ((OldOutPin != NULL) && (OldOutPin->LinkedTo.Num() > 0))
		{
			// Create a dummy execution sequence that will be the target of the return call from the latent action
			UK2Node_ExecutionSequence* DummyNode = CallFuncNode->GetGraph()->CreateBlankNode<UK2Node_ExecutionSequence>();
			CompilerContext.MessageLog.NotifyIntermediateObjectCreation(DummyNode, CallFuncNode);
			DummyNode->AllocateDefaultPins();

			// Wire in the dummy node
			UEdGraphPin* NewInPin = K2Schema->FindExecutionPin(*DummyNode, EGPD_Input);
			UEdGraphPin* NewOutPin = K2Schema->FindExecutionPin(*DummyNode, EGPD_Output);

			if ((NewInPin != NULL) && (NewOutPin != NULL))
			{
				while (OldOutPin->LinkedTo.Num() > 0)
				{
					UEdGraphPin* LinkedPin = OldOutPin->LinkedTo[0];

					LinkedPin->BreakLinkTo(OldOutPin);
					LinkedPin->MakeLinkTo(NewOutPin);
				}

				OldOutPin->MakeLinkTo(NewInPin);
			}
		}
	}
}

void FKCHandler_CallFunction::Compile(FKismetFunctionContext& Context, UEdGraphNode* Node)
{
	check(NULL != Node);

	//@TODO: Can probably move this earlier during graph verification instead of compilation, but after island pruning
	if (!IsCalledFunctionPure(Node))
	{
		// For imperative nodes, make sure the exec function was actually triggered and not just included due to an output data dependency
		UEdGraphPin* ExecTriggeringPin = CompilerContext.GetSchema()->FindExecutionPin(*Node, EGPD_Input);
		if (ExecTriggeringPin == NULL)
		{
			CompilerContext.MessageLog.Error(*FString::Printf(*NSLOCTEXT("KismetCompiler", "NoValidExecutionPinForCallFunc_Error", "@@ must have a valid execution pin").ToString()), Node);
			return;
		}
		else if (ExecTriggeringPin->LinkedTo.Num() == 0)
		{
			CompilerContext.MessageLog.Warning(*FString::Printf(*NSLOCTEXT("KismetCompiler", "NodeNeverExecuted_Warning", "@@ will never be executed").ToString()), Node);
			return;
		}
	}

	// Validate the self pin again if it is disconnected, because pruning isolated nodes could have caused an invalid target
	UEdGraphPin* SelfPin = CompilerContext.GetSchema()->FindSelfPin(*Node, EGPD_Input);
	if (SelfPin && (SelfPin->LinkedTo.Num() == 0))
	{
		FEdGraphPinType SelfType;
		SelfType.PinCategory = CompilerContext.GetSchema()->PC_Object;
		SelfType.PinSubCategory = CompilerContext.GetSchema()->PSC_Self;

		if (!CompilerContext.GetSchema()->ArePinTypesCompatible(SelfType, SelfPin->PinType, Context.NewClass) && (SelfPin->DefaultObject == NULL))
		{
			CompilerContext.MessageLog.Error(*NSLOCTEXT("KismetCompiler", "PinMustHaveConnectionPruned_Error", "Pin @@ must have a connection.  Self pins cannot be connected to nodes that are culled.").ToString(), SelfPin);
		}
	}

	// Make sure the function node is valid to call
	CreateFunctionCallStatement(Context, Node, SelfPin);
}

void FKCHandler_CallFunction::CheckIfFunctionIsCallable(UFunction* Function, FKismetFunctionContext& Context, UEdGraphNode* Node)
{
	// Verify that the function is a Blueprint callable function (in case a BlueprintCallable specifier got removed)
	if (!Function->HasAnyFunctionFlags(FUNC_BlueprintCallable) && (Function->GetOuter() != Context.NewClass))
	{
		if (!IsCalledFunctionFinal(Node) && Function->GetName().Find(CompilerContext.GetSchema()->FN_ExecuteUbergraphBase.ToString()))
		{
			CompilerContext.MessageLog.Error(*FString::Printf(*NSLOCTEXT("KismetCompiler", "ShouldNotCallFromBlueprint_Error", "Function '%s' called from @@ should not be called from a Blueprint").ToString(), *Function->GetName()), Node);
		}
	}
}

// Get the name of the function to call from the node
FString FKCHandler_CallFunction::GetFunctionNameFromNode(UEdGraphNode* Node) const
{
	UK2Node_CallFunction* CallFuncNode = Cast<UK2Node_CallFunction>(Node);
	if (CallFuncNode)
	{
		return CallFuncNode->FunctionReference.GetMemberName().ToString();
	}
	else
	{
		CompilerContext.MessageLog.Error(*NSLOCTEXT("KismetCompiler", "UnableResolveFunctionName_Error", "Unable to resolve function name for @@").ToString(), Node);
		return TEXT("");
	}
}

#if _MSC_VER
#pragma warning(pop)
#endif

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
