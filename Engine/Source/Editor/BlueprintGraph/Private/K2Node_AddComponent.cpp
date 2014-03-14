// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"

#include "ParticleDefinitions.h"
#include "CompilerResultsLog.h"
#include "CallFunctionHandler.h"

#define LOCTEXT_NAMESPACE "K2Node_AddComponent"

//////////////////////////////////////////////////////////////////////////
// FKCHandler_AddComponent

class FKCHandler_AddComponent : public FKCHandler_CallFunction
{
public:
	FKCHandler_AddComponent(FKismetCompilerContext& InCompilerContext)
		: FKCHandler_CallFunction(InCompilerContext)
	{
	}

	virtual void RegisterNets(FKismetFunctionContext& Context, UEdGraphNode* Node) OVERRIDE
	{
		FKCHandler_CallFunction::RegisterNets(Context, Node);

		UK2Node_AddComponent* AddComponentNode = CastChecked<UK2Node_AddComponent>(Node);
		UEdGraphPin* TransformPin = AddComponentNode->GetRelativeTransformPin();

		// If the transform pin is not connected, create a term that grabs its value from the template, so that value is used and not replaced with the identity.
		if (TransformPin->LinkedTo.Num() == 0)
		{
			// The context can be NULL if the graph failed validation previously; no point in even trying to register nets, as the skeleton will be used instead
			if (CompilerContext.UbergraphContext != NULL)
			{
				check(CompilerContext.UbergraphContext->NetNameMap);
				// If there's no term, add a dummy identity transform here.  Term is defined in the event graph's scope, because a default value needs to be set
				FBPTerminal* TransformTerm = new (Context.EventGraphLocals) FBPTerminal();
				TransformTerm->CopyFromPin(TransformPin, FString::Printf(TEXT("%s_AddComponentDefaultTransform"), *CompilerContext.UbergraphContext->NetNameMap->MakeValidName(TransformPin)));
				TransformTerm->bIsLocal = true;
				TransformTerm->bIsConst = true;
				TransformTerm->PropertyDefault = TEXT("");

				// Try and find the template and get relative transform from it
				UEdGraphPin* TemplateNamePin = AddComponentNode->GetTemplateNamePinChecked();
				const FString TemplateName = TemplateNamePin->DefaultValue;
				USceneComponent* SceneCompTemplate = Cast<USceneComponent>(Context.GetBlueprint()->FindTemplateByName(FName(*TemplateName)));
				if (SceneCompTemplate != NULL)
				{
					FTransform TemplateTransform = FTransform(SceneCompTemplate->RelativeRotation, SceneCompTemplate->RelativeLocation, SceneCompTemplate->RelativeScale3D);
					TransformTerm->PropertyDefault = TemplateTransform.ToString();
				}

				Context.NetMap.Add(TransformPin, TransformTerm);
			}
		}
	}
};

UK2Node_AddComponent::UK2Node_AddComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsPureFunc = false;
}

void UK2Node_AddComponent::AllocateDefaultPins()
{
	AllocateDefaultPinsWithoutExposedVariables();
	AllocatePinsForExposedVariables();

	UEdGraphPin* ManualAttachmentPin = GetManualAttachmentPin();

	UEdGraphSchema const* Schema = GetSchema();
	check(Schema != NULL);
	Schema->ConstructBasicPinTooltip(*ManualAttachmentPin, LOCTEXT("ManualAttachmentPinTooltip", "Defines whether the component should attach to the root automatically, or be left unattached for the user to manually attach later.").ToString(), ManualAttachmentPin->PinToolTip);

	UEdGraphPin* TransformPin = GetRelativeTransformPin();
	Schema->ConstructBasicPinTooltip(*TransformPin, LOCTEXT("TransformPinTooltip", "Defines where to position the component (relative to its parent). If the component is left unattached, then the transform is relative to the world.").ToString(), TransformPin->PinToolTip);
}

void UK2Node_AddComponent::AllocatePinsForExposedVariables()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	const UActorComponent* TemplateComponent = GetTemplateFromNode();
	if(const UClass* ComponentClass = (TemplateComponent ? TemplateComponent->GetClass() : NULL))
	{
		for (TFieldIterator<UProperty> PropertyIt(ComponentClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
		{
			UProperty* Property = *PropertyIt;
			const bool bNotDelegate = !Property->IsA(UMulticastDelegateProperty::StaticClass());
			const bool bIsExposedToSpawn = Property->HasMetaData(FBlueprintMetadata::MD_ExposeOnSpawn);
			const bool bIsVisible = Property->HasAllPropertyFlags(CPF_BlueprintVisible);
			const bool bNotParam = !Property->HasAllPropertyFlags(CPF_Parm);
			if(bNotDelegate && bIsExposedToSpawn && bIsVisible && bNotParam)
			{
				FEdGraphPinType PinType;
				K2Schema->ConvertPropertyToPinType(Property, /*out*/ PinType);	
				const bool bIsUnique = (NULL == FindPin(Property->GetName()));
				if(K2Schema->FindSetVariableByNameFunction(PinType) && bIsUnique)
				{
					UEdGraphPin* Pin = CreatePin(EGPD_Input, TEXT(""), TEXT(""), NULL, false, false, Property->GetName());
					Pin->PinType = PinType;
					bHasExposedVariable = true;
				}
			}
		}
	}
}

void UK2Node_AddComponent::AllocateDefaultPinsWithoutExposedVariables()
{
	Super::AllocateDefaultPins();
	
	// set properties on template pin
	UEdGraphPin* TemplateNamePin = GetTemplateNamePinChecked();
	TemplateNamePin->bDefaultValueIsReadOnly = true;
	TemplateNamePin->bNotConnectable = true;
	TemplateNamePin->bHidden = true;

	// set properties on relative transform pin
	UEdGraphPin* RelativeTransformPin = GetRelativeTransformPin();
	RelativeTransformPin->bDefaultValueIsIgnored = true;

	// Override this as a non-ref param, because the identity transform is hooked up in the compiler under the hood
	RelativeTransformPin->PinType.bIsReference = false;
}

void UK2Node_AddComponent::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	Super::ReallocatePinsDuringReconstruction(OldPins);

	UEdGraphPin* TemplateNamePin = GetTemplateNamePinChecked();
	//UEdGraphPin* ReturnValuePin = GetReturnValuePin();
	for(int32 OldPinIdx = 0; TemplateNamePin && OldPinIdx < OldPins.Num(); ++OldPinIdx)
	{
		if(TemplateNamePin && (TemplateNamePin->PinName == OldPins[OldPinIdx]->PinName))
		{
			TemplateNamePin->DefaultValue = OldPins[OldPinIdx]->DefaultValue;
		}
	}

	AllocatePinsForExposedVariables();
}

void UK2Node_AddComponent::ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const
{
	Super::ValidateNodeDuringCompilation(MessageLog);

	UActorComponent* Template = GetTemplateFromNode();
	if (Template)
	{
		UClass* TemplateClass = Template->GetClass();
		if (!TemplateClass->IsChildOf(UActorComponent::StaticClass()) || TemplateClass->HasAnyClassFlags(CLASS_Abstract) || !TemplateClass->HasMetaData(FBlueprintMetadata::MD_BlueprintSpawnableComponent) )
		{
			MessageLog.Error(*FString::Printf(*NSLOCTEXT("KismetCompiler", "InvalidComponentTemplate_Error", "Invalid class '%s' used as template by '%s' for @@").ToString(), *TemplateClass->GetName(), *GetNodeTitle(ENodeTitleType::FullTitle)), this);
		}
	}
	else
	{
		MessageLog.Error(*FString::Printf(*NSLOCTEXT("KismetCompiler", "MissingComponentTemplate_Error", "Unknown template referenced by '%s' for @@").ToString(), *GetNodeTitle(ENodeTitleType::FullTitle)), this);
	}
}

UActorComponent* UK2Node_AddComponent::GetTemplateFromNode() const
{
	UBlueprint* BlueprintObj = GetBlueprint();

	// Find the template name input pin, to get the name from
	UEdGraphPin* TemplateNamePin = GetTemplateNamePin();
	if (TemplateNamePin)
	{
		const FString& TemplateName = TemplateNamePin->DefaultValue;
		return BlueprintObj->FindTemplateByName(FName(*TemplateName));
	}

	return NULL;
}

void UK2Node_AddComponent::DestroyNode()
{
	// See if this node has a template
	UActorComponent* Template = GetTemplateFromNode();
	if (Template != NULL)
	{
		// Get the blueprint so we can remove it from it
		UBlueprint* BlueprintObj = GetBlueprint();

		// remove it
		BlueprintObj->Modify();
		BlueprintObj->ComponentTemplates.Remove(Template);
	}

	Super::DestroyNode();
}

void UK2Node_AddComponent::PrepareForCopying()
{
	TemplateBlueprint = GetBlueprint()->GetPathName();
}

void UK2Node_AddComponent::PostPasteNode()
{
	Super::PostPasteNode();

	// There is a template associated with this node that should be unique, but after a node is pasted, it either points to a
	// template shared by the copied node, or to nothing (when pasting into a different blueprint)
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	UBlueprint* Blueprint = GetBlueprint();

	// Find the template name and return type pins
	UEdGraphPin* TemplateNamePin = GetTemplateNamePin();
	UEdGraphPin* ReturnPin = GetReturnValuePin();
	if ((TemplateNamePin != NULL) && (ReturnPin != NULL))
	{
		// Find the current template if it exists
		FString TemplateName = TemplateNamePin->DefaultValue;
		UActorComponent* SourceTemplate = Blueprint->FindTemplateByName(FName(*TemplateName));

		// Determine the type of the component needed
		UClass* ComponentClass = Cast<UClass>(ReturnPin->PinType.PinSubCategoryObject.Get());
			
		if (ComponentClass)
		{
			ensure(NULL != Cast<UBlueprintGeneratedClass>(Blueprint->GeneratedClass));
			// Create a new template object and update the template pin to point to it
			UActorComponent* NewTemplate = ConstructObject<UActorComponent>(ComponentClass, Blueprint->GeneratedClass);
			NewTemplate->SetFlags(RF_ArchetypeObject);
			Blueprint->ComponentTemplates.Add(NewTemplate);

			TemplateNamePin->DefaultValue = NewTemplate->GetName();

			// Copy the old template data over to the new template if it's compatible
			if ((SourceTemplate != NULL) && (SourceTemplate->GetClass()->IsChildOf(ComponentClass)))
			{
				TArray<uint8> SavedProperties;
				FObjectWriter Writer(SourceTemplate, SavedProperties);
				FObjectReader(NewTemplate, SavedProperties);
			}
			else if(TemplateBlueprint.Len() > 0)
			{
				// try to find/load our blueprint to copy the template
				UBlueprint* SourceBlueprint = FindObject<UBlueprint>(NULL, *TemplateBlueprint);
				if(SourceBlueprint != NULL)
				{
					SourceTemplate = SourceBlueprint->FindTemplateByName(FName(*TemplateName));
					if ((SourceTemplate != NULL) && (SourceTemplate->GetClass()->IsChildOf(ComponentClass)))
					{
						TArray<uint8> SavedProperties;
						FObjectWriter Writer(SourceTemplate, SavedProperties);
						FObjectReader(NewTemplate, SavedProperties);
					}
				}

				TemplateBlueprint.Empty();
			}
		}
		else
		{
			// Clear the template connection; can't resolve the type of the component to create
			ensure(false);
			TemplateNamePin->DefaultValue = TEXT("");
		}
	}
}

FString UK2Node_AddComponent::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	UEdGraphPin* TemplateNamePin = GetTemplateNamePin();
	if (TemplateNamePin != NULL)
	{
		FString TemplateName = TemplateNamePin->DefaultValue;
		UBlueprint* Blueprint = GetBlueprint();
		UActorComponent* SourceTemplate = Blueprint->FindTemplateByName(FName(*TemplateName));
		if(SourceTemplate)
		{
			UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(SourceTemplate);
			USkeletalMeshComponent* SkelMeshComp = Cast<USkeletalMeshComponent>(SourceTemplate);
			UParticleSystemComponent* PSysComp = Cast<UParticleSystemComponent>(SourceTemplate);

			if(StaticMeshComp != NULL && StaticMeshComp->StaticMesh != NULL)
			{
				return FString(TEXT("Add StaticMesh ")) + StaticMeshComp->StaticMesh->GetName();
			}
			else if(SkelMeshComp != NULL && SkelMeshComp->SkeletalMesh != NULL)
			{
				return FString(TEXT("Add SkeletalMesh ")) + SkelMeshComp->SkeletalMesh->GetName();		
			}
			else if(PSysComp != NULL && PSysComp->Template != NULL)
			{
				return FString(TEXT("Add ParticleSystem ")) + PSysComp->Template->GetName();		
			}
			else
			{
				return FString(TEXT("Add ")) + SourceTemplate->GetClass()->GetName();		
			}
		}
	}

	return Super::GetNodeTitle(TitleType);
}

FString UK2Node_AddComponent::GetDocumentationLink() const
{
	return TEXT("Shared/GraphNodes/Blueprint/UK2Node_AddComponent");
}

FString UK2Node_AddComponent::GetDocumentationExcerptName() const
{
	UEdGraphPin* TemplateNamePin = GetTemplateNamePin();
	UBlueprint* Blueprint = GetBlueprint();

	if ((TemplateNamePin != NULL) && (Blueprint != NULL))
	{
		FString TemplateName = TemplateNamePin->DefaultValue;
		UActorComponent* SourceTemplate = Blueprint->FindTemplateByName(FName(*TemplateName));

		if (SourceTemplate != NULL)
		{
			return SourceTemplate->GetClass()->GetName();
		}
	}

	return Super::GetDocumentationExcerptName();
}

FNodeHandlingFunctor* UK2Node_AddComponent::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_AddComponent(CompilerContext);
}

void UK2Node_AddComponent::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	if (CompilerContext.bIsFullCompile && bHasExposedVariable)
	{
		static FString ObjectParamName = FString(TEXT("Object"));
		static FString ValueParamName = FString(TEXT("Value"));
		static FString PropertyNameParamName = FString(TEXT("PropertyName"));

		const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

		UK2Node_AddComponent* NewNode = CompilerContext.SpawnIntermediateNode<UK2Node_AddComponent>(this, SourceGraph); 
		NewNode->SetFromFunction(GetTargetFunction());
		NewNode->AllocateDefaultPinsWithoutExposedVariables();

		// function parameters
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*GetTemplateNamePinChecked(), *NewNode->GetTemplateNamePinChecked()), this);
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*GetRelativeTransformPin(), *NewNode->GetRelativeTransformPin()), this);
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*GetManualAttachmentPin(), *NewNode->GetManualAttachmentPin()), this);

		UEdGraphPin* ReturnPin = NewNode->GetReturnValuePin();
		UEdGraphPin* OriginalReturnPin = GetReturnValuePin();
		check((NULL != ReturnPin) && (NULL != OriginalReturnPin));
		ReturnPin->PinType = OriginalReturnPin->PinType;
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*OriginalReturnPin, *ReturnPin), this);
		// exec in
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*GetExecPin(), *NewNode->GetExecPin()), this);

		UEdGraphPin* LastThen = NewNode->GetThenPin();
		for(int32 PinIndex = 0; PinIndex < Pins.Num(); PinIndex++)
		{
			UEdGraphPin* OrgPin = Pins[PinIndex];
			UFunction* SetByNameFunction = Schema->FindSetVariableByNameFunction(OrgPin->PinType);
			if((NULL == NewNode->FindPin(OrgPin->PinName)) && SetByNameFunction)
			{
				UK2Node_CallFunction* SetVarNode = NULL;
				if(OrgPin->PinType.bIsArray)
				{
					SetVarNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallArrayFunction>(this, SourceGraph);
				}
				else
				{
					SetVarNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
				}
				SetVarNode->SetFromFunction(SetByNameFunction);
				SetVarNode->AllocateDefaultPins();

				// Connect this node into the exec chain
				Schema->TryCreateConnection(LastThen, SetVarNode->GetExecPin());
				LastThen = SetVarNode->GetThenPin();

				// Connect the new actor to the 'object' pin
				UEdGraphPin* ObjectPin = SetVarNode->FindPinChecked(ObjectParamName);
				Schema->TryCreateConnection(ReturnPin, ObjectPin);

				// Fill in literal for 'property name' pin - name of pin is property name
				UEdGraphPin* PropertyNamePin = SetVarNode->FindPinChecked(PropertyNameParamName);
				PropertyNamePin->DefaultValue = OrgPin->PinName;

				// Move connection from the variable pin on the spawn node to the 'value' pin
				UEdGraphPin* ValuePin = SetVarNode->FindPinChecked(ValueParamName);
				CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*OrgPin, *ValuePin), this);
				if(OrgPin->PinType.bIsArray)
				{
					SetVarNode->PinConnectionListChanged(ValuePin);
				}
			}
		}
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*GetThenPin(), *LastThen), this);
		BreakAllNodeLinks();
	}
}

#undef LOCTEXT_NAMESPACE
