// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"

#include "EngineKismetLibraryClasses.h"

#include "CompilerResultsLog.h"
#include "CallFunctionHandler.h"

#define LOCTEXT_NAMESPACE "K2Node"

UK2Node_CallFunction::UK2Node_CallFunction(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}


bool UK2Node_CallFunction::IsDeprecated() const
{
	UFunction* Function = GetTargetFunction();
	return (Function != NULL) && Function->HasMetaData(FBlueprintMetadata::MD_DeprecatedFunction);
}

bool UK2Node_CallFunction::ShouldWarnOnDeprecation() const
{
	// TEMP:  Do not warn in the case of SpawnActor, as we have a special upgrade path for those nodes
	return (FunctionReference.GetMemberName() != FName(TEXT("BeginSpawningActorFromBlueprint")));
}

FString UK2Node_CallFunction::GetDeprecationMessage() const
{
	UFunction* Function = GetTargetFunction();
	if ((Function != NULL) && Function->HasMetaData(FBlueprintMetadata::MD_DeprecationMessage))
	{
		return FString::Printf(TEXT("%s %s"), *LOCTEXT("CallFunctionDeprecated_Warning", "@@ is deprecated;").ToString(), *Function->GetMetaData(FBlueprintMetadata::MD_DeprecationMessage));
	}

	return Super::GetDeprecationMessage();
}


FString UK2Node_CallFunction::GetFunctionContextString() const
{
	FString ContextString;

	// Don't show 'target is' if no target pin!
	UEdGraphPin* SelfPin = GetDefault<UEdGraphSchema_K2>()->FindSelfPin(*this, EGPD_Input);
	if(SelfPin != NULL && !SelfPin->bHidden)
	{
		const UFunction* Function = GetTargetFunction();
		UClass* CurrentSelfClass = (Function != NULL) ? Function->GetOwnerClass() : NULL;
		const UClass* TrueSelfClass = CurrentSelfClass ? CurrentSelfClass->GetAuthoritativeClass() : NULL;
		FString TargetString = (TrueSelfClass != NULL) ? TrueSelfClass->GetName() : TEXT("None");

		// This action won't be necessary once the new name convention is used.
		if(TargetString.EndsWith(TEXT("_C")))
		{
			TargetString = TargetString.LeftChop(2);
		}

		ContextString = FString(TEXT("\n")) + FString::Printf(*LOCTEXT("CallFunctionOnDifferentContext", "Target is %s").ToString(), *TargetString);
	}

	return ContextString;
}


FString UK2Node_CallFunction::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FString FunctionName;
	FString ContextString;
	FString RPCString;

	if (UFunction* Function = GetTargetFunction())
	{
		RPCString = UK2Node_Event::GetLocalizedNetString(Function->FunctionFlags, true);
		FunctionName = UK2Node_CallFunction::GetUserFacingFunctionName(Function);
		ContextString = GetFunctionContextString();
	}
	else
	{
		FunctionName = FunctionReference.GetMemberName().ToString();
		if ((GEditor != NULL) && (GetDefault<UEditorStyleSettings>()->bShowFriendlyNames))
		{
			FunctionName = EngineUtils::SanitizeDisplayName(FunctionName, false);
		}
	}

	const FString Result = (TitleType == ENodeTitleType::ListView) ? FunctionName : FunctionName + ContextString + RPCString;
	return Result;
}


void UK2Node_CallFunction::AllocateDefaultPins()
{
	UBlueprint* MyBlueprint = GetBlueprint();
	UFunction* Function = GetTargetFunction();

	// First try remap table
	if (Function == NULL)
	{
		UClass* ParentClass = FunctionReference.GetMemberParentClass(this);

		if (ParentClass != NULL)
		{
			if (UFunction* NewFunction = Cast<UFunction>(FindRemappedField(ParentClass, FunctionReference.GetMemberName())))
			{
				// Found a remapped property, update the node
				Function = NewFunction;
				SetFromFunction(NewFunction);
			}
		}
	}

	if (Function == NULL)
	{
		// The function no longer exists in the stored scope
		// Try searching inside all function libraries, in case the function got refactored into one of them.
		for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
		{
			UClass* TestClass = *ClassIt;
			if (TestClass->IsChildOf(UBlueprintFunctionLibrary::StaticClass()))
			{
				Function = FindField<UFunction>(TestClass, FunctionReference.GetMemberName());
				if (Function != NULL)
				{
					UClass* OldClass = FunctionReference.GetMemberParentClass(this);
					Message_Note( FString::Printf(*LOCTEXT("FixedUpFunctionInLibrary", "UK2Node_CallFunction: Fixed up function '%s', originally in '%s', now in library '%s'.").ToString(),
						*FunctionReference.GetMemberName().ToString(),
						 (OldClass != NULL) ? *OldClass->GetName() : TEXT("(null)"), *TestClass->GetName()) );
					SetFromFunction(Function);
					break;
				}
			}
		}
	}

	// Now create the pins if we ended up with a valid function to call
	if (Function != NULL)
	{
		CreatePinsForFunctionCall(Function);
	}

	Super::AllocateDefaultPins();
}

/** Util to find self pin in an array */
UEdGraphPin* FindSelfPin(TArray<UEdGraphPin*>& Pins)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	for(int32 PinIdx=0; PinIdx<Pins.Num(); PinIdx++)
	{
		if(Pins[PinIdx]->PinName == K2Schema->PN_Self)
		{
			return Pins[PinIdx];
		}
	}
	return NULL;
}

void UK2Node_CallFunction::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	// BEGIN TEMP
	// We had a bug where the class was being messed up by copy/paste, but the self pin class was still ok. This code fixes up those cases.
	UFunction* Function = GetTargetFunction();
	if (Function == NULL)
	{
		if (UEdGraphPin* SelfPin = FindSelfPin(OldPins))
		{
			if (UClass* SelfPinClass = Cast<UClass>(SelfPin->PinType.PinSubCategoryObject.Get()))
			{
				if (UFunction* NewFunction = FindField<UFunction>(SelfPinClass, FunctionReference.GetMemberName()))
				{
					SetFromFunction(NewFunction);
				}
			}
		}
	}
	// END TEMP

	Super::ReallocatePinsDuringReconstruction(OldPins);
}

UEdGraphPin* UK2Node_CallFunction::CreateSelfPin(const UFunction* Function)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Chase up the function's Super chain, the function can be called on any object that is at least that specific
	const UFunction* FirstDeclaredFunction = Function;
	while (FirstDeclaredFunction->GetSuperFunction() != NULL)
	{
		FirstDeclaredFunction = FirstDeclaredFunction->GetSuperFunction();
	}

	// Create the self pin
	UClass* FunctionClass = CastChecked<UClass>(FirstDeclaredFunction->GetOuter());

	UEdGraphPin* SelfPin = NULL;
	if ((FunctionClass == GetBlueprint()->GeneratedClass) || (FunctionClass == GetBlueprint()->SkeletonGeneratedClass))
	{
		// This means the function is defined within the blueprint, so the pin should be a true "self" pin
		SelfPin = CreatePin(EGPD_Input, K2Schema->PC_Object, K2Schema->PSC_Self, NULL, false, false, K2Schema->PN_Self);
	}
	else
	{
		// This means that the function is declared in an external class, and should reference that class
		SelfPin = CreatePin(EGPD_Input, K2Schema->PC_Object, TEXT(""), FunctionClass, false, false, K2Schema->PN_Self);
	}
	check(SelfPin != NULL);

	return SelfPin;
}

void UK2Node_CallFunction::CreateExecPinsForFunctionCall(const UFunction* Function)
{
	// If not pure, create exec pins
	if (!bIsPureFunc)
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

		// If we want enum->exec expansion, and it is not disabled, do it now
		if(bWantsEnumToExecExpansion)
		{
			const FString& EnumParamName = Function->GetMetaData(FBlueprintMetadata::MD_ExpandEnumAsExecs);
			UByteProperty* EnumProp = FindField<UByteProperty>(Function, FName(*EnumParamName));
			if(EnumProp != NULL && EnumProp->Enum != NULL)
			{
				// yay, found it! Now create exec pin for each
				int32 NumInputs = (EnumProp->Enum->NumEnums() - 1);
				for(int32 InputIdx=0; InputIdx<NumInputs; InputIdx++)
				{
					FString InputName = EnumProp->Enum->GetEnumName(InputIdx);
					CreatePin(EGPD_Input, K2Schema->PC_Exec, TEXT(""), NULL, false, false, InputName);
				}
			}
		}
		else
		{
			// Single input exec pin
			CreatePin(EGPD_Input, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Execute);
		}

		// Use 'completed' name for output pins on latent functions
		UEdGraphPin* OutputExecPin = CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Then);
		if(Function->HasMetaData(FBlueprintMetadata::MD_Latent))
		{
			OutputExecPin->PinFriendlyName = K2Schema->PN_Completed;
		}
	}
}

void UK2Node_CallFunction::DetermineWantsEnumToExecExpansion(const UFunction* Function)
{
	bWantsEnumToExecExpansion = false;

	if (Function->HasMetaData(FBlueprintMetadata::MD_ExpandEnumAsExecs))
	{
		const FString& EnumParamName = Function->GetMetaData(FBlueprintMetadata::MD_ExpandEnumAsExecs);
		UByteProperty* EnumProp = FindField<UByteProperty>(Function, FName(*EnumParamName));
		if(EnumProp != NULL && EnumProp->Enum != NULL)
		{
			bWantsEnumToExecExpansion = true;
		}
		else
		{
			if (!bHasCompilerMessage)
			{
				//put in warning state
				bHasCompilerMessage = true;
				ErrorType = EMessageSeverity::Warning;
				ErrorMsg = FString::Printf(*LOCTEXT("EnumToExecExpansionFailed", "Unable to find enum parameter with name '%s' to expand for @@").ToString(), *EnumParamName);
			}
		}
	}
}

void UK2Node_CallFunction::GeneratePinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const
{
	ensure(Pin.GetOwningNode() == this);

	UEdGraphSchema const* Schema = GetSchema();
	check(Schema != NULL);
	UEdGraphSchema_K2 const* const K2Schema = Cast<const UEdGraphSchema_K2>(Schema);

	if (K2Schema == NULL)
	{
		Schema->ConstructBasicPinTooltip(Pin, TEXT(""), HoverTextOut);
		return;
	}
	
	// figure what tag we should be parsing for (is this a return-val pin, or a parameter?)
	FString ParamName;
	FString TagStr = TEXT("@param");
	if (Pin.PinName == K2Schema->PN_ReturnValue)
	{
		TagStr = TEXT("@return");
	}
	else
	{
		ParamName = Pin.PinName.ToLower();
	}

	// get the class function object associated with this node
	UFunction* Function = GetTargetFunction();
	if (Function == NULL)
	{
		Schema->ConstructBasicPinTooltip(Pin, TEXT(""), HoverTextOut);
		return;
	}

	// grab the the function's comment block for us to parse
	FString FunctionToolTipText = GetDefaultTooltipForFunction(Function);

	
	int32 CurStrPos = INDEX_NONE;
	int32 FullToolTipLen = FunctionToolTipText.Len();
	// parse the full function tooltip text, looking for tag lines
	do 
	{
		CurStrPos = FunctionToolTipText.Find(TagStr, ESearchCase::IgnoreCase, ESearchDir::FromStart, CurStrPos);
		if (CurStrPos == INDEX_NONE) // if the tag wasn't found
		{
			break;
		}

		// advance past the tag
		CurStrPos += TagStr.Len();

		// advance past whitespace
		while(CurStrPos < FullToolTipLen && FChar::IsWhitespace(FunctionToolTipText[CurStrPos]))
		{
			++CurStrPos;
		}

		// if this is a parameter pin
		if (!ParamName.IsEmpty())
		{
			FString TagParamName;

			// copy the parameter name
			while (CurStrPos < FullToolTipLen && !FChar::IsWhitespace(FunctionToolTipText[CurStrPos]))
			{
				TagParamName.AppendChar(FunctionToolTipText[CurStrPos++]);
			}

			// if this @param tag doesn't match the param we're looking for
			if (TagParamName != ParamName)
			{
				continue;
			}
		}

		// advance past whitespace (get to the meat of the comment)
		while(CurStrPos < FullToolTipLen && FChar::IsWhitespace(FunctionToolTipText[CurStrPos]))
		{
			++CurStrPos;
		}


		FString ParamDesc;
		// collect the param/return-val description
		while (CurStrPos < FullToolTipLen && FunctionToolTipText[CurStrPos] != TEXT('@'))
		{
			// advance past newline
			while(CurStrPos < FullToolTipLen && FChar::IsLinebreak(FunctionToolTipText[CurStrPos]))
			{
				++CurStrPos;

				// advance past whitespace at the start of a new line
				while(CurStrPos < FullToolTipLen && FChar::IsWhitespace(FunctionToolTipText[CurStrPos]))
				{
					++CurStrPos;
				}

				// replace the newline with a single space
				if(!FChar::IsLinebreak(FunctionToolTipText[CurStrPos]))
				{
					ParamDesc.AppendChar(TEXT(' '));
				}
			}

			if (FunctionToolTipText[CurStrPos] != TEXT('@'))
			{
				ParamDesc.AppendChar(FunctionToolTipText[CurStrPos++]);
			}
		}

		// trim any trailing whitespace from the descriptive text
		ParamDesc.TrimTrailing();

		// if we came up with a valid description for the param/return-val
		if (!ParamDesc.IsEmpty())
		{
			HoverTextOut += FString(TEXT("\n")) + ParamDesc;
			break; // we found a match, so there's no need to continue
		}

	} while (CurStrPos < FullToolTipLen);

	Schema->ConstructBasicPinTooltip(Pin, HoverTextOut, HoverTextOut);
}

bool UK2Node_CallFunction::CreatePinsForFunctionCall(const UFunction* Function)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UClass* FunctionOwnerClass = Function->GetOuterUClass();

	bIsInterfaceCall = FunctionOwnerClass->HasAnyClassFlags(CLASS_Interface);
	bIsPureFunc = (Function->HasAnyFunctionFlags(FUNC_BlueprintPure) != false);
	bIsConstFunc = (Function->HasAnyFunctionFlags(FUNC_Const) != false);
	DetermineWantsEnumToExecExpansion(Function);

	// Create input pins
	CreateExecPinsForFunctionCall(Function);

	UEdGraphPin* SelfPin = CreateSelfPin(Function);

	//Renamed self pin to target
	SelfPin->PinFriendlyName =  TEXT("Target");

	// fill out the self-pin's default tool-tip
	GeneratePinHoverText(*SelfPin, SelfPin->PinToolTip);

	const bool bIsProtectedFunc = Function->GetBoolMetaData(FBlueprintMetadata::MD_Protected);
	const bool bIsStaticFunc = Function->HasAllFunctionFlags(FUNC_Static);

	//@TODO: Can't strictly speaking always hide the self pin for pure and static functions; it'll still be needed if the function belongs to a class not in the blueprint's class hierarchy
	SelfPin->bHidden = ((bIsPureFunc && !bIsConstFunc) || bIsProtectedFunc || bIsStaticFunc);

	if (bIsStaticFunc)
	{
		// Wire up the self to the CDO of the class if it's not us
		if (UBlueprint* BP = GetBlueprint())
		{
			if (!BP->SkeletonGeneratedClass->IsChildOf(FunctionOwnerClass))
			{
				SelfPin->DefaultObject = FunctionOwnerClass->GetDefaultObject();
			}
		}
	}

	// Build a list of the pins that should be hidden for this function (ones that are automagically filled in by the K2 compiler)
	TSet<FString> PinsToHide;
	FBlueprintEditorUtils::GetHiddenPinsForFunction(GetBlueprint(), Function, PinsToHide);

	const bool bShowHiddenSelfPins = ((PinsToHide.Num() > 0) && GetBlueprint()->ParentClass->HasMetaData(FBlueprintMetadata::MD_ShowHiddenSelfPins));

	// Create the inputs and outputs
	bool bAllPinsGood = true;
	for (TFieldIterator<UProperty> PropIt(Function); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
	{
		UProperty* Param = *PropIt;
		const bool bIsFunctionInput = !Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm);
		const bool bIsRefParam = Param->HasAnyPropertyFlags(CPF_ReferenceParm) && bIsFunctionInput;

		const EEdGraphPinDirection Direction = bIsFunctionInput ? EGPD_Input : EGPD_Output;

		UEdGraphPin* Pin = CreatePin(Direction, TEXT(""), TEXT(""), NULL, false, bIsRefParam, Param->GetName());
		const bool bPinGood = (Pin != NULL) && K2Schema->ConvertPropertyToPinType(Param, /*out*/ Pin->PinType);

		if (bPinGood)
		{
			//Flag pin as read only for const reference property
			Pin->bDefaultValueIsIgnored = Param->HasAnyPropertyFlags(CPF_ConstParm | CPF_ReferenceParm) && (!Function->HasMetaData(FBlueprintMetadata::MD_AutoCreateRefTerm) || Pin->PinType.bIsArray);

			const bool bAdvancedPin = Param->HasAllPropertyFlags(CPF_AdvancedDisplay);
			Pin->bAdvancedView = bAdvancedPin;
			if(bAdvancedPin && (ENodeAdvancedPins::NoPins == AdvancedPinDisplay))
			{
				AdvancedPinDisplay = ENodeAdvancedPins::Hidden;
			}

			FString MetadataDefaultValue = Function->GetMetaData(*Param->GetName());
			if (!MetadataDefaultValue.IsEmpty())
			{
				// Specified default value in the metadata
				Pin->DefaultValue = Pin->AutogeneratedDefaultValue = MetadataDefaultValue;
			}
			else
			{
				const FName MetadataCppDefaultValueKey( *(FString(TEXT("CPP_Default_")) + Param->GetName()) );
				const FString MetadataCppDefaultValue = Function->GetMetaData(MetadataCppDefaultValueKey);
				if(!MetadataCppDefaultValue.IsEmpty())
				{
					Pin->DefaultValue = Pin->AutogeneratedDefaultValue = MetadataCppDefaultValue;
				}
				else
				{
					// Set the default value to (T)0
					K2Schema->SetPinDefaultValueBasedOnType(Pin);
				}
			}

			// setup the default tool-tip text for this pin
			GeneratePinHoverText(*Pin, Pin->PinToolTip);
			
			if (PinsToHide.Contains(Pin->PinName))
			{
				FString const DefaultToSelfMetaValue = Function->GetMetaData(FBlueprintMetadata::MD_DefaultToSelf);
				FString const WorldContextMetaValue  = Function->GetMetaData(FBlueprintMetadata::MD_WorldContext);
				bool bIsSelfPin = ((Pin->PinName == DefaultToSelfMetaValue) || (Pin->PinName == WorldContextMetaValue));

				if (!bShowHiddenSelfPins || !bIsSelfPin)
				{
					Pin->bHidden = true;
					Pin->DefaultValue = TEXT("0");
				}
			}

			PostParameterPinCreated(Pin);
		}

		bAllPinsGood = bAllPinsGood && bPinGood;
	}

	// If we have an 'enum to exec' parameter, set its default value to something valid so we don't get warnings
	if(bWantsEnumToExecExpansion)
	{
		FString EnumParamName = Function->GetMetaData(FBlueprintMetadata::MD_ExpandEnumAsExecs);
		UEdGraphPin* EnumParamPin = FindPin(EnumParamName);
		if (UEnum* PinEnum = (EnumParamPin ? Cast<UEnum>(EnumParamPin->PinType.PinSubCategoryObject.Get()) : NULL))
		{
			EnumParamPin->DefaultValue = PinEnum->GetEnumName(0);
		}
	}

	return bAllPinsGood;
}


void UK2Node_CallFunction::PostReconstructNode()
{
	Super::PostReconstructNode();

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	// Fixup self node, may have been overridden from old self node
	UFunction* Function = GetTargetFunction();
	const bool bIsStaticFunc = Function ? Function->HasAllFunctionFlags(FUNC_Static) : false;

	UEdGraphPin* SelfPin = FindPin(K2Schema->PN_Self);
	if (bIsStaticFunc && SelfPin)
	{
		// Wire up the self to the CDO of the class if it's not us
		if (UBlueprint* BP = GetBlueprint())
		{
			UClass* FunctionOwnerClass = Function->GetOuterUClass();
			if (!BP->SkeletonGeneratedClass->IsChildOf(FunctionOwnerClass))
			{
				SelfPin->DefaultObject = FunctionOwnerClass->GetDefaultObject();
			}
		}
	}

	// Set the return type to the right class of component
	UActorComponent* TemplateComp = GetTemplateFromNode();
	UEdGraphPin* ReturnPin = GetReturnValuePin();
	if(TemplateComp && ReturnPin)
	{
		ReturnPin->PinType.PinSubCategoryObject = TemplateComp->GetClass();
	}
}

void UK2Node_CallFunction::DestroyNode()
{
	// See if this node has a template
	UActorComponent* Template = GetTemplateFromNode();
	if (Template != NULL)
	{
		// Get the blueprint so we can remove it from it
		UBlueprint* BlueprintObj = GetBlueprint();

		// remove it
		BlueprintObj->ComponentTemplates.Remove(Template);
	}

	Super::DestroyNode();
}

void UK2Node_CallFunction::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	if (bIsBeadFunction)
	{
		if (Pin->LinkedTo.Num() == 0)
		{
			// Commit suicide; bead functions must always have an input and output connection
			DestroyNode();
		}
	}
}

UFunction* UK2Node_CallFunction::GetTargetFunction() const
{
	UFunction* Function = FunctionReference.ResolveMember<UFunction>(this);
	return Function;
}

UEdGraphPin* UK2Node_CallFunction::GetThenPin() const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPin(K2Schema->PN_Then);
	check(Pin == NULL || Pin->Direction == EGPD_Output); // If pin exists, it must be output
	return Pin;
}

UEdGraphPin* UK2Node_CallFunction::GetReturnValuePin() const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPin(K2Schema->PN_ReturnValue);
	check(Pin == NULL || Pin->Direction == EGPD_Output); // If pin exists, it must be output
	return Pin;
}

bool UK2Node_CallFunction::IsLatentFunction() const
{
	if (UFunction* Function = GetTargetFunction())
	{
		if (Function->HasMetaData(FBlueprintMetadata::MD_Latent))
		{
			return true;
		}
	}

	return false;
}

bool UK2Node_CallFunction::AllowMultipleSelfs(bool bInputAsArray) const
{
	if (UFunction* Function = GetTargetFunction())
	{
		const bool bHasReturnParams = (Function->GetReturnProperty() != NULL);
		return !bHasReturnParams && !IsNodePure() && !IsLatentFunction();
	}

	return Super::AllowMultipleSelfs(bInputAsArray);
}

FLinearColor UK2Node_CallFunction::GetNodeTitleColor() const
{
	if (IsNodePure())
	{
		return GEditor->AccessEditorUserSettings().PureFunctionCallNodeTitleColor;
	}
	else
	{
		return Super::GetNodeTitleColor();
	}
}

FString UK2Node_CallFunction::GetTooltip() const
{
	FString Tooltip;

	if (UFunction* Function = GetTargetFunction())
	{
		Tooltip = GetDefaultTooltipForFunction(Function);


		FString ClientString;

		if (Function->HasAllFunctionFlags(FUNC_BlueprintAuthorityOnly))
		{
			ClientString = NSLOCTEXT("K2Node", "ServerFunction", "\n\nAuthority Only. This function will only execute on the server.").ToString();
		}
		else if (Function->HasAllFunctionFlags(FUNC_BlueprintCosmetic))
		{
			ClientString = NSLOCTEXT("K2Node", "ClientEvent", "\n\nCosmetic. This event is only for cosmetic, non-gameplay actions.").ToString();
		} 

		Tooltip += ClientString;
	}
	else
	{
		Tooltip = LOCTEXT("CallUnknownFunction", "Call unknown function ").ToString() + FunctionReference.GetMemberName().ToString();
	}

	return Tooltip;
}

FString UK2Node_CallFunction::GetUserFacingFunctionName(const UFunction* Function)
{
	FString FunctionName = Function->GetMetaData(TEXT("FriendlyName"));
	
	if (FunctionName.IsEmpty())
	{
		FunctionName = Function->GetName();
	}

	if( GEditor && GetDefault<UEditorStyleSettings>()->bShowFriendlyNames )
	{
		FunctionName = EngineUtils::SanitizeDisplayName(FunctionName, false);
	}
	return FunctionName;
}

FString UK2Node_CallFunction::GetDefaultTooltipForFunction(const UFunction* Function)
{
	const FString Tooltip = Function->GetToolTipText().ToString();

	if (!Tooltip.IsEmpty())
	{
		return Tooltip;
	}
	else
	{
		return GetUserFacingFunctionName(Function);
	}
}

FString UK2Node_CallFunction::GetDefaultCategoryForFunction(const UFunction* Function, const FString& BaseCategory)
{
	FString NodeCategory = BaseCategory;
	if( Function->HasMetaData(FBlueprintMetadata::MD_FunctionCategory) )
	{
		// Add seperator if base category is supplied
		if(NodeCategory.Len() > 0)
		{
			NodeCategory += TEXT("|");
		}

		// Add category from function
		FString FuncCategory = Function->GetMetaData(FBlueprintMetadata::MD_FunctionCategory);
		if( GEditor && GetDefault<UEditorStyleSettings>()->bShowFriendlyNames )
		{
			FuncCategory = EngineUtils::SanitizeDisplayName( FuncCategory, false );
		}
		NodeCategory += FuncCategory;
	}
	return NodeCategory;
}


FString UK2Node_CallFunction::GetKeywordsForFunction(const UFunction* Function)
{
	// If the friendly name and real function name do not match add the real function name friendly name as a keyword.
	FString Keywords;
	if( Function->GetName() != GetUserFacingFunctionName(Function) )
	{
		Keywords = Function->GetName();
	}

	if (ShouldDrawCompact(Function))
	{
		Keywords.AppendChar(TEXT(' '));
		Keywords += GetCompactNodeTitle(Function);
	}

	FString MetaKeywords = Function->GetMetaData(FBlueprintMetadata::MD_FunctionKeywords);

	if (!MetaKeywords.IsEmpty())
	{
		Keywords.AppendChar(TEXT(' '));
		Keywords += MetaKeywords;
	}

	return Keywords;
}

void UK2Node_CallFunction::SetFromFunction(const UFunction* Function)
{
	if (Function != NULL)
	{
		bIsPureFunc = Function->HasAnyFunctionFlags(FUNC_BlueprintPure);
		bIsConstFunc = Function->HasAnyFunctionFlags(FUNC_Const);
		DetermineWantsEnumToExecExpansion(Function);

		FunctionReference.SetFromField<UFunction>(Function, this);
	}
}

FString UK2Node_CallFunction::GetDocumentationLink() const
{
	UClass* ParentClass = NULL;
	if (FunctionReference.IsSelfContext())
	{
		if (HasValidBlueprint())
		{
			UFunction* Function = FindField<UFunction>(GetBlueprint()->GeneratedClass, FunctionReference.GetMemberName());
			if (Function != NULL)
			{
				ParentClass = Function->GetOwnerClass();
			}
		}		
	}
	else 
	{
		ParentClass = FunctionReference.GetMemberParentClass(this);
	}
	
	if (ParentClass != NULL)
	{
		return FString::Printf(TEXT("Shared/GraphNodes/Blueprint/%s%s"), ParentClass->GetPrefixCPP(), *ParentClass->GetName());
	}

	return FString("Shared/GraphNodes/Blueprint/UK2Node_CallFunction");
}

FString UK2Node_CallFunction::GetDocumentationExcerptName() const
{
	return FunctionReference.GetMemberName().ToString();
}

FString UK2Node_CallFunction::GetDescriptiveCompiledName() const
{
	return FString(TEXT("CallFunc_")) + FunctionReference.GetMemberName().ToString();
}

bool UK2Node_CallFunction::ShouldDrawCompact(const UFunction* Function)
{
	return (Function != NULL) && Function->HasMetaData(FBlueprintMetadata::MD_CompactNodeTitle);
}

bool UK2Node_CallFunction::ShouldDrawCompact() const
{
	UFunction* Function = GetTargetFunction();

	return ShouldDrawCompact(Function);
}

bool UK2Node_CallFunction::ShouldDrawAsBead() const
{
	return bIsBeadFunction;
}

bool UK2Node_CallFunction::ShouldShowNodeProperties() const
{
	// Show node properties if this corresponds to a function graph
	if (FunctionReference.GetMemberName() != NAME_None)
	{
		return FindObject<UEdGraph>(GetBlueprint(), *(FunctionReference.GetMemberName().ToString())) != NULL;
	}
	return false;
}

FString UK2Node_CallFunction::GetCompactNodeTitle(const UFunction* Function)
{
	static const FString ProgrammerMultiplicationSymbol = TEXT("*");
	static const FString CommonMultiplicationSymbol = TEXT("\xD7");

	static const FString ProgrammerDivisionSymbol = TEXT("/");
	static const FString CommonDivisionSymbol = TEXT("\xF7");

	static const FString ProgrammerConversionSymbol = TEXT("->");
	static const FString CommonConversionSymbol = TEXT("\x2022");

	const FString OperatorTitle = Function->GetMetaData(FBlueprintMetadata::MD_CompactNodeTitle);
	if (!OperatorTitle.IsEmpty())
	{
		if (OperatorTitle == ProgrammerMultiplicationSymbol)
		{
			return CommonMultiplicationSymbol;
		}
		else if (OperatorTitle == ProgrammerDivisionSymbol)
		{
			return CommonDivisionSymbol;
		}
		else if (OperatorTitle == ProgrammerConversionSymbol)
		{
			return CommonConversionSymbol;
		}
		else
		{
			return OperatorTitle;
		}
	}
	
	return Function->GetName();
}

FString UK2Node_CallFunction::GetCompactNodeTitle() const
{
	UFunction* Function = GetTargetFunction();
	if (Function != NULL)
	{
		return GetCompactNodeTitle(Function);
	}
	else
	{
		return Super::GetCompactNodeTitle();
	}
}

void UK2Node_CallFunction::GetRedirectPinNames(const UEdGraphPin& Pin, TArray<FString>& RedirectPinNames) const
{
	Super::GetRedirectPinNames(Pin, RedirectPinNames);

	if (RedirectPinNames.Num() > 0)
	{
		const FString OldPinName = RedirectPinNames[0];

		// first add functionname.param
		RedirectPinNames.Add(FString::Printf(TEXT("%s.%s"), *FunctionReference.GetMemberName().ToString(), *OldPinName));

		// if there is class, also add an option for class.functionname.param
		UClass* FunctionClass = FunctionReference.GetMemberParentClass(this);
		while (FunctionClass)
		{
			RedirectPinNames.Add(FString::Printf(TEXT("%s.%s.%s"), *FunctionClass->GetName(), *FunctionReference.GetMemberName().ToString(), *OldPinName));
			FunctionClass = FunctionClass->GetSuperClass();
		}
	}
}

bool UK2Node_CallFunction::IsSelfPinCompatibleWithBlueprintContext(UEdGraphPin *SelfPin, UBlueprint* BlueprintObj) const
{
	check(BlueprintObj);

	UClass* FunctionClass = FunctionReference.GetMemberParentClass(this);

	bool bIsCompatible = (SelfPin != NULL) ? SelfPin->bHidden : true;
	if (!bIsCompatible && (BlueprintObj->GeneratedClass != NULL))
	{
		bIsCompatible |= BlueprintObj->GeneratedClass->IsChildOf(FunctionClass);
	}

	if (!bIsCompatible && (BlueprintObj->SkeletonGeneratedClass != NULL))
	{
		bIsCompatible |= BlueprintObj->SkeletonGeneratedClass->IsChildOf(FunctionClass);
	}
	return bIsCompatible;
}

void UK2Node_CallFunction::EnsureFunctionIsInBlueprint()
{
	// Ensure we're calling a function in a context related to our blueprint. If not, 
	// reassigning the class and then calling ReconstructNodes will re-wire the pins correctly
	if (UFunction* Function = GetTargetFunction())
	{
		UClass* FunctionOwnerClass = Function->GetOuterUClass();
		UObject* FunctionGenerator = FunctionOwnerClass ? FunctionOwnerClass->ClassGeneratedBy : NULL;

		// If function is generated from a blueprint object then dbl check self pin compatibility
		UEdGraphPin* SelfPin = GetDefault<UEdGraphSchema_K2>()->FindSelfPin(*this, EGPD_Input);
		if ((FunctionGenerator != NULL) && SelfPin)
		{
			UBlueprint* BlueprintObj = FBlueprintEditorUtils::FindBlueprintForNode(this);
			if ((BlueprintObj != NULL) && !IsSelfPinCompatibleWithBlueprintContext(SelfPin, BlueprintObj))
			{
				FunctionReference.SetSelfMember(Function->GetFName());
			}
		}
	}
}

void UK2Node_CallFunction::PostPasteNode()
{
	Super::PostPasteNode();
	EnsureFunctionIsInBlueprint();

	UFunction* Function = GetTargetFunction();
	if(Function != NULL)
	{
		// After pasting we need to go through and ensure the hidden the self pins is correct in case the source blueprint had different metadata
		TSet<FString> PinsToHide;
		FBlueprintEditorUtils::GetHiddenPinsForFunction(GetBlueprint(), Function, PinsToHide);

		const bool bShowHiddenSelfPins = ((PinsToHide.Num() > 0) && GetBlueprint()->ParentClass->HasMetaData(FBlueprintMetadata::MD_ShowHiddenSelfPins));

		FString const DefaultToSelfMetaValue = Function->GetMetaData(FBlueprintMetadata::MD_DefaultToSelf);
		FString const WorldContextMetaValue  = Function->GetMetaData(FBlueprintMetadata::MD_WorldContext);

		for (int32 PinIndex = 0; PinIndex < Pins.Num(); ++PinIndex)
		{
			UEdGraphPin* Pin = Pins[PinIndex];

			bool bIsSelfPin = ((Pin->PinName == DefaultToSelfMetaValue) || (Pin->PinName == WorldContextMetaValue));
			bool bPinShouldBeHidden = PinsToHide.Contains(Pin->PinName) && (!bShowHiddenSelfPins || !bIsSelfPin);

			if (bPinShouldBeHidden && !Pin->bHidden)
			{
				Pin->BreakAllPinLinks();
				Pin->DefaultValue = TEXT("0");
			}
			Pin->bHidden = bPinShouldBeHidden;
		}
	}
}

void UK2Node_CallFunction::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);
	if (!bDuplicateForPIE && (!this->HasAnyFlags(RF_Transient)))
	{
		FunctionReference.InvalidateSelfScope();
		EnsureFunctionIsInBlueprint();
	}
}

void UK2Node_CallFunction::ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const
{
	Super::ValidateNodeDuringCompilation(MessageLog);

	UFunction *Function = GetTargetFunction();
	if (Function == NULL)
	{
		MessageLog.Warning(*FString::Printf(*LOCTEXT("FunctionNotFound", "Unable to find function with name '%s' for @@").ToString(), *FunctionReference.GetMemberName().ToString()), this);
	}
	else if (Function->HasMetaData(FBlueprintMetadata::MD_ExpandEnumAsExecs) && bWantsEnumToExecExpansion == false)
	{
		const FString& EnumParamName = Function->GetMetaData(FBlueprintMetadata::MD_ExpandEnumAsExecs);
		MessageLog.Warning(*FString::Printf(*LOCTEXT("EnumToExecExpansionFailed", "Unable to find enum parameter with name '%s' to expand for @@").ToString(), *EnumParamName), this);
	}

	// enforce UnsafeDuringActorConstruction keyword
	if (Function && Function->HasMetaData(FBlueprintMetadata::MD_UnsafeForConstructionScripts))
	{
		// emit warning if we are in a construction script
		UEdGraph const* const Graph = GetGraph();
		UEdGraphSchema_K2 const* const Schema = Cast<const UEdGraphSchema_K2>(GetSchema());
		bool bNodeIsInConstructionScript = Schema && Schema->IsConstructionScript(Graph);

		if (bNodeIsInConstructionScript == false)
		{
			// IsConstructionScript() can return false if graph was cloned from the construction script
			// in that case, check the function entry
			TArray<const UK2Node_FunctionEntry*> EntryPoints;
			Graph->GetNodesOfClass(EntryPoints);

			if (EntryPoints.Num() == 1)
			{
				UK2Node_FunctionEntry const* const Node = EntryPoints[0];
				if (Node)
				{
					UFunction* const SignatureFunction = FindField<UFunction>(Node->SignatureClass, Node->SignatureName);
					bNodeIsInConstructionScript = SignatureFunction && (SignatureFunction->GetFName() == Schema->FN_UserConstructionScript);
				}
			}
		}

		if ( bNodeIsInConstructionScript )
		{
			MessageLog.Warning(*LOCTEXT("FunctionUnsafeDuringConstruction", "Function '@@' is unsafe to call in a construction script.").ToString(), this);
		}
	}
}

void UK2Node_CallFunction::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsLoading())
	{
		if (Ar.UE4Ver() < VER_UE4_SWITCH_CALL_NODE_TO_USE_MEMBER_REFERENCE)
		{
			UFunction* Function = FindField<UFunction>(CallFunctionClass_DEPRECATED, CallFunctionName_DEPRECATED);
			const bool bProbablySelfCall = (CallFunctionClass_DEPRECATED == NULL) || ((Function != NULL) && (Function->GetOuterUClass()->ClassGeneratedBy == GetBlueprint()));

			FunctionReference.SetDirect(CallFunctionName_DEPRECATED, FGuid(), CallFunctionClass_DEPRECATED, bProbablySelfCall);
		}

		if(Ar.UE4Ver() < VER_UE4_K2NODE_REFERENCEGUIDS)
		{
			FGuid FunctionGuid;

			if (UBlueprint::GetGuidFromClassByFieldName<UFunction>(GetBlueprint()->GeneratedClass, FunctionReference.GetMemberName(), FunctionGuid))
			{
				const bool bSelf = FunctionReference.IsSelfContext();
				FunctionReference.SetDirect(FunctionReference.GetMemberName(), FunctionGuid, (bSelf ? NULL : FunctionReference.GetMemberParentClass((UClass*)NULL)), bSelf);
			}
		}
	}
}

void UK2Node_CallFunction::PostPlacedNewNode()
{
	Super::PostPlacedNewNode();

	// Try re-setting the function given our new parent scope, in case it turns an external to an internal, or vis versa
	FunctionReference.RefreshGivenNewSelfScope<UFunction>(this);
}

FNodeHandlingFunctor* UK2Node_CallFunction::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_CallFunction(CompilerContext);
}

/*
struct FBackwardCompatibilityHelper
{
private:
	static void Replace_CreateSaveGameObjectFromBlueprint(UK2Node_CallFunction* OldNode, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
	{
		const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
		check(Schema);

		UK2Node_CallFunction* NewNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(OldNode, SourceGraph); 
		check(NewNode);
		const UFunction* NewFunction = UGameplayStatics::StaticClass()->FindFunctionByName(TEXT("CreateSaveGameObject"));
		check(NewFunction);
		NewNode->SetFromFunction(NewFunction);
		NewNode->AllocateDefaultPins();

		check(OldNode->GetThenPin() && NewNode->GetThenPin());
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*OldNode->GetThenPin(), *NewNode->GetThenPin()), OldNode);

		check(OldNode->GetReturnValuePin() && NewNode->GetReturnValuePin());
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*OldNode->GetReturnValuePin(), *NewNode->GetReturnValuePin()), OldNode);

		UEdGraphPin* OldExecPin = OldNode->FindPinChecked(Schema->PN_Execute);
		UEdGraphPin* NewExecPin = NewNode->FindPinChecked(Schema->PN_Execute);
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*OldExecPin, *NewExecPin), OldNode);

		UEdGraphPin* OldBPPin = OldNode->FindPinChecked(TEXT("SaveGameBlueprint"));
		ensure(0 == OldBPPin->LinkedTo.Num());
		if (UBlueprint* SpawnBlueprint = Cast<UBlueprint>(OldBPPin->DefaultObject))
		{
			UEdGraphPin* NewClassPin = NewNode->FindPinChecked(TEXT("SaveGameClass"));
			NewClassPin->DefaultObject = SpawnBlueprint->GeneratedClass;
		}

		OldNode->BreakAllNodeLinks();
	}

	static void Replace_SetAnimBlueprint(UK2Node_CallFunction* OldNode, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
	{
		const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
		check(Schema);

		UK2Node_CallFunction* NewNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(OldNode, SourceGraph); 
		check(NewNode);
		const UFunction* NewFunction = USkeletalMeshComponent::StaticClass()->FindFunctionByName(TEXT("SetAnimClass"));
		check(NewFunction);
		NewNode->SetFromFunction(NewFunction);
		NewNode->AllocateDefaultPins();

		check(OldNode->GetThenPin() && NewNode->GetThenPin());
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*OldNode->GetThenPin(), *NewNode->GetThenPin()), OldNode);

		UEdGraphPin* OldExecPin = OldNode->FindPinChecked(Schema->PN_Execute);
		UEdGraphPin* NewExecPin = NewNode->FindPinChecked(Schema->PN_Execute);
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*OldExecPin, *NewExecPin), OldNode);

		UEdGraphPin* OldSelfPin = OldNode->FindPinChecked(Schema->PN_Self);
		UEdGraphPin* NewSelfPin = NewNode->FindPinChecked(Schema->PN_Self);
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*OldSelfPin, *NewSelfPin), OldNode);

		UEdGraphPin* OldBPPin = OldNode->FindPinChecked(TEXT("NewBlueprint"));
		ensure(0 == OldBPPin->LinkedTo.Num());
		if (UBlueprint* SpawnBlueprint = Cast<UBlueprint>(OldBPPin->DefaultObject))
		{
			UEdGraphPin* NewClassPin = NewNode->FindPinChecked(TEXT("NewClass"));
			NewClassPin->DefaultObject = SpawnBlueprint->GeneratedClass;
		}

		OldNode->BreakAllNodeLinks();
	}

public:
	static void ReplaceObsoleteFunctionsCall(UK2Node_CallFunction* CallFunctionNode, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
	{
		check(CallFunctionNode && SourceGraph);
		const FMemberReference& MemberReference = CallFunctionNode->FunctionReference;
		static const FName CreateSaveGameObjectFromBlueprint("CreateSaveGameObjectFromBlueprint");
		static const FName SetAnimBlueprint("SetAnimBlueprint");

		if ((MemberReference.GetMemberName() == CreateSaveGameObjectFromBlueprint) &&
			(MemberReference.GetMemberParentClass((UClass *)NULL) == UGameplayStatics::StaticClass()))
		{
			Replace_CreateSaveGameObjectFromBlueprint(CallFunctionNode, CompilerContext, SourceGraph);
		}
		else if ((MemberReference.GetMemberName() == SetAnimBlueprint) &&
			(MemberReference.GetMemberParentClass((UClass *)NULL) == USkeletalMeshComponent::StaticClass()))
		{
			Replace_SetAnimBlueprint(CallFunctionNode, CompilerContext, SourceGraph);
		}
	}
};
*/
void UK2Node_CallFunction::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	if (CompilerContext.bIsFullCompile)
	{
		const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

		// >>> Backwards Compatibility:  VER_UE4_EDITORONLY_BLUEPRINTS
		//FBackwardCompatibilityHelper::ReplaceObsoleteFunctionsCall(this, CompilerContext, SourceGraph);
		// <<< End Backwards Compatibility

		// If we have an enum param that is expanded, we handle that first
		if(bWantsEnumToExecExpansion)
		{
			// Grab the function
			UFunction* Function = GetTargetFunction();
			if(Function)
			{
				// Get the metadata that identifies which param is the enum, and try and find it
				const FString& EnumParamName = Function->GetMetaData(FBlueprintMetadata::MD_ExpandEnumAsExecs);
				UByteProperty* EnumProp = FindField<UByteProperty>(Function, FName(*EnumParamName));
				if(EnumProp != NULL && EnumProp->Enum != NULL)
				{
					// Create normal exec input
					UEdGraphPin* ExecutePin = CreatePin(EGPD_Input, Schema->PC_Exec, TEXT(""), NULL, false, false, Schema->PN_Execute);

					// Create temp enum variable
					UK2Node_TemporaryVariable* TempEnumVarNode = CompilerContext.SpawnIntermediateNode<UK2Node_TemporaryVariable>(this, SourceGraph);
					TempEnumVarNode->VariableType.PinCategory = Schema->PC_Byte;
					TempEnumVarNode->VariableType.PinSubCategoryObject = EnumProp->Enum;
					TempEnumVarNode->AllocateDefaultPins();
					// Get the output pin
					UEdGraphPin* TempEnumVarOutput = TempEnumVarNode->GetVariablePin();

					// Connect temp enum variable to (hidden) enum pin
					UEdGraphPin* EnumParamPin = FindPin(EnumParamName);
					check(EnumParamPin);
					check(EnumParamPin->Direction == EGPD_Input);
					Schema->TryCreateConnection(TempEnumVarOutput, EnumParamPin);

					// Now we want to iterate over other exec inputs...
					for(int32 PinIdx=Pins.Num()-1; PinIdx>=0; PinIdx--)
					{
						UEdGraphPin* Pin = Pins[PinIdx];
						if( Pin != NULL && 
							Pin != ExecutePin &&
							Pin->Direction == EGPD_Input && 
							Pin->PinType.PinCategory == Schema->PC_Exec )
						{
							// Create node to set the temp enum var
							UK2Node_AssignmentStatement* AssignNode = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
							AssignNode->AllocateDefaultPins();

							// Move connections from fake 'enum exec' pint to this assignment node
							CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*Pin, *AssignNode->GetExecPin()), this);

							// Connect this to out temp enum var
							Schema->TryCreateConnection(AssignNode->GetVariablePin(), TempEnumVarOutput);

							// Connect exec output to 'real' exec pin
							Schema->TryCreateConnection(AssignNode->GetThenPin(), ExecutePin);

							// set the literal enum value to set to
							AssignNode->GetValuePin()->DefaultValue = Pin->PinName;

							// Finally remove this 'cosmetic' exec pin
							Pins.RemoveAt(PinIdx);
						}
					}
				}
			}
		}

		// Then we go through and expand out array iteration if necessary
		const bool bAllowMultipleSelfs = AllowMultipleSelfs(true);
		UEdGraphPin* MultiSelf = Schema->FindSelfPin(*this, EEdGraphPinDirection::EGPD_Input);
		if(bAllowMultipleSelfs && MultiSelf && !MultiSelf->PinType.bIsArray)
		{
			const bool bProperInputToExpandForEach = 
				(1 == MultiSelf->LinkedTo.Num()) && 
				(NULL != MultiSelf->LinkedTo[0]) && 
				(MultiSelf->LinkedTo[0]->PinType.bIsArray);
			if(bProperInputToExpandForEach)
			{
				CallForEachElementInArrayExpansion(this, MultiSelf, CompilerContext, SourceGraph);
			}
		}
	}
}

void UK2Node_CallFunction::CallForEachElementInArrayExpansion(UK2Node* Node, UEdGraphPin* MultiSelf, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
	check(Node && MultiSelf && SourceGraph && Schema);
	const bool bProperInputToExpandForEach = 
		(1 == MultiSelf->LinkedTo.Num()) && 
		(NULL != MultiSelf->LinkedTo[0]) && 
		(MultiSelf->LinkedTo[0]->PinType.bIsArray);
	ensure(bProperInputToExpandForEach);

	UEdGraphPin* ThenPin = Node->FindPinChecked(Schema->PN_Then);

	// Create int Iterator
	UK2Node_TemporaryVariable* IteratorVar = CompilerContext.SpawnIntermediateNode<UK2Node_TemporaryVariable>(Node, SourceGraph);
	IteratorVar->VariableType.PinCategory = Schema->PC_Int;
	IteratorVar->AllocateDefaultPins();

	// Initialize iterator
	UK2Node_AssignmentStatement* InteratorInitialize = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(Node, SourceGraph);
	InteratorInitialize->AllocateDefaultPins();
	InteratorInitialize->GetValuePin()->DefaultValue = TEXT("0");
	Schema->TryCreateConnection(IteratorVar->GetVariablePin(), InteratorInitialize->GetVariablePin());
	CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*Node->GetExecPin(), *InteratorInitialize->GetExecPin()), Node);

	// Do loop branch
	UK2Node_IfThenElse* Branch = CompilerContext.SpawnIntermediateNode<UK2Node_IfThenElse>(Node, SourceGraph);
	Branch->AllocateDefaultPins();
	Schema->TryCreateConnection(InteratorInitialize->GetThenPin(), Branch->GetExecPin());
	CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*ThenPin, *Branch->GetElsePin()), Node);

	// Do loop condition
	UK2Node_CallFunction* Condition = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(Node, SourceGraph); 
	Condition->SetFromFunction(UKismetMathLibrary::StaticClass()->FindFunctionByName(TEXT("Less_IntInt")));
	Condition->AllocateDefaultPins();
	Schema->TryCreateConnection(Condition->GetReturnValuePin(), Branch->GetConditionPin());
	Schema->TryCreateConnection(Condition->FindPinChecked(TEXT("A")), IteratorVar->GetVariablePin());

	// Array size
	UK2Node_CallArrayFunction* ArrayLength = CompilerContext.SpawnIntermediateNode<UK2Node_CallArrayFunction>(Node, SourceGraph); 
	ArrayLength->SetFromFunction(UKismetArrayLibrary::StaticClass()->FindFunctionByName(TEXT("Array_Length")));
	ArrayLength->AllocateDefaultPins();
	CompilerContext.CheckConnectionResponse(Schema->CopyPinLinks(*MultiSelf, *ArrayLength->GetTargetArrayPin()), Node);
	ArrayLength->PinConnectionListChanged(ArrayLength->GetTargetArrayPin());
	Schema->TryCreateConnection(Condition->FindPinChecked(TEXT("B")), ArrayLength->GetReturnValuePin());

	// Get Element
	UK2Node_CallArrayFunction* GetElement = CompilerContext.SpawnIntermediateNode<UK2Node_CallArrayFunction>(Node, SourceGraph); 
	GetElement->SetFromFunction(UKismetArrayLibrary::StaticClass()->FindFunctionByName(TEXT("Array_Get")));
	GetElement->AllocateDefaultPins();
	CompilerContext.CheckConnectionResponse(Schema->CopyPinLinks(*MultiSelf, *GetElement->GetTargetArrayPin()), Node);
	GetElement->PinConnectionListChanged(GetElement->GetTargetArrayPin());
	Schema->TryCreateConnection(GetElement->FindPinChecked(TEXT("Index")), IteratorVar->GetVariablePin());

	// Iterator increment
	UK2Node_CallFunction* Increment = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(Node, SourceGraph); 
	Increment->SetFromFunction(UKismetMathLibrary::StaticClass()->FindFunctionByName(TEXT("Add_IntInt")));
	Increment->AllocateDefaultPins();
	Schema->TryCreateConnection(Increment->FindPinChecked(TEXT("A")), IteratorVar->GetVariablePin());
	Increment->FindPinChecked(TEXT("B"))->DefaultValue = TEXT("1");

	// Iterator assigned
	UK2Node_AssignmentStatement* IteratorAssign = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(Node, SourceGraph);
	IteratorAssign->AllocateDefaultPins();
	Schema->TryCreateConnection(IteratorAssign->GetVariablePin(), IteratorVar->GetVariablePin());
	Schema->TryCreateConnection(IteratorAssign->GetValuePin(), Increment->GetReturnValuePin());
	Schema->TryCreateConnection(IteratorAssign->GetThenPin(), Branch->GetExecPin());

	// Connect pins from intermediate nodes back in to the original node
	Schema->TryCreateConnection(Branch->GetThenPin(), Node->GetExecPin());
	Schema->TryCreateConnection(ThenPin, IteratorAssign->GetExecPin());
	Schema->TryCreateConnection(GetElement->FindPinChecked(TEXT("Item")), MultiSelf);
}

FName UK2Node_CallFunction::GetCornerIcon() const
{
	if (const UFunction* Function = GetTargetFunction())
	{
		if (Function->HasAllFunctionFlags(FUNC_BlueprintAuthorityOnly))
		{
			return TEXT("Graph.Replication.AuthorityOnly");		
		}
		else if (Function->HasAllFunctionFlags(FUNC_BlueprintCosmetic))
		{
			return TEXT("Graph.Replication.ClientEvent");
		}
		else if(Function->HasMetaData(FBlueprintMetadata::MD_Latent))
		{
			return TEXT("Graph.Latent.LatentIcon");
		}
	}
	return Super::GetCornerIcon();
}

FName UK2Node_CallFunction::GetPaletteIcon(FLinearColor& OutColor) const
{
	static const FName NativeMakeFunc(TEXT("NativeMakeFunc"));
	static const FName NativeBrakeFunc(TEXT("NativeBreakFunc"));

	auto Function = GetTargetFunction();
	if (Function && Function->HasMetaData(NativeMakeFunc))
	{
		return TEXT("GraphEditor.MakeStruct_16x");
	}
	else if (Function && Function->HasMetaData(NativeBrakeFunc))
	{
		return TEXT("GraphEditor.BreakStruct_16x");
	}
	// Check to see if the function is calling an function that could be an event, display the event icon instead.
	else if (Function && UEdGraphSchema_K2::FunctionCanBePlacedAsEvent(Function))
	{
		return TEXT("GraphEditor.Event_16x");
	}
	else
	{
		OutColor = GetNodeTitleColor();
		return TEXT("Kismet.AllClasses.FunctionIcon");
	}
}

FText UK2Node_CallFunction::GetToolTipHeading() const
{
	FText Heading = Super::GetToolTipHeading();

	struct FHeadingBuilder
	{
		FHeadingBuilder(FText InitialHeading) : ConstructedHeading(InitialHeading) {}

		void Append(FText HeadingAddOn)
		{
			if (ConstructedHeading.IsEmpty())
			{
				ConstructedHeading = HeadingAddOn;
			}
			else 
			{
				ConstructedHeading = FText::Format(FText::FromString("{0}\n{1}"), HeadingAddOn, ConstructedHeading);
			}
		}

		FText ConstructedHeading;
	};
	FHeadingBuilder HeadingBuilder(Super::GetToolTipHeading());

	if (const UFunction* Function = GetTargetFunction())
	{
		if (Function->HasAllFunctionFlags(FUNC_BlueprintAuthorityOnly))
		{
			HeadingBuilder.Append(LOCTEXT("ServerOnlyFunc", "Server Only"));	
		}
		if (Function->HasAllFunctionFlags(FUNC_BlueprintCosmetic))
		{
			HeadingBuilder.Append(LOCTEXT("ClientOnlyFunc", "Client Only"));
		}
		if(Function->HasMetaData(FBlueprintMetadata::MD_Latent))
		{
			HeadingBuilder.Append(LOCTEXT("LatentFunc", "Latent"));
		}
	}

	return HeadingBuilder.ConstructedHeading;
}

bool UK2Node_CallFunction::HasExternalBlueprintDependencies() const
{
	const UClass* SourceClass = FunctionReference.GetMemberParentClass(this);
	const UBlueprint* SourceBlueprint = GetBlueprint();
	return (SourceClass != NULL) && (SourceClass->ClassGeneratedBy != NULL) && (SourceClass->ClassGeneratedBy != SourceBlueprint);
}

UEdGraph* UK2Node_CallFunction::GetFunctionGraph() const
{
	return FindObject<UEdGraph>(GetBlueprint(), *(FunctionReference.GetMemberName().ToString()));
}

#undef LOCTEXT_NAMESPACE
