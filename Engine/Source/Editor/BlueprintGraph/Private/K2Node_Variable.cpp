// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"

#include "CompilerResultsLog.h"
#include "ClassIconFinder.h"

#define LOCTEXT_NAMESPACE "K2Node"

UK2Node_Variable::UK2Node_Variable(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UK2Node_Variable::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	// Fix old content 
	if(Ar.IsLoading())
	{
		if(Ar.UE4Ver() < VER_UE4_VARK2NODE_NULL_VARSRCCLASS_ON_SELF)
		{
			// See if bSelfContext is set but there is still a class assigned
			if(bSelfContext_DEPRECATED && VariableSourceClass_DEPRECATED != NULL)
			{
				VariableSourceClass_DEPRECATED = NULL;
				UE_LOG(LogBlueprint, Log, TEXT("VarNode '%s' Variable '%s': Cleared VariableSourceClass."), *GetPathName(), *GetVarNameString());
			}
		}

		if(Ar.UE4Ver() < VER_UE4_VARK2NODE_USE_MEMBERREFSTRUCT)
		{
			// Copy info into new struct
			VariableReference.SetDirect(VariableName_DEPRECATED, FGuid(), VariableSourceClass_DEPRECATED, bSelfContext_DEPRECATED);
		}

		if(Ar.UE4Ver() < VER_UE4_K2NODE_REFERENCEGUIDS)
		{
			FGuid VarGuid;
			
			if (UBlueprint::GetGuidFromClassByFieldName<UProperty>(GetBlueprint()->GeneratedClass, VariableReference.GetMemberName(), VarGuid))
			{
				const bool bSelf = VariableReference.IsSelfContext();
				VariableReference.SetDirect(VariableReference.GetMemberName(), VarGuid, (bSelf ? NULL : VariableReference.GetMemberParentClass((UClass*)NULL)), bSelf);
			}
		}
	}
}

void UK2Node_Variable::SetFromProperty(const UProperty* Property, bool bSelfContext)
{
	VariableReference.SetFromField<UProperty>(Property, bSelfContext);
}

bool UK2Node_Variable::CreatePinForVariable(EEdGraphPinDirection Direction)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UProperty* VariableProperty = GetPropertyForVariable();
	if (VariableProperty != NULL)
	{
		// Create the pin
		UEdGraphPin* VariablePin = CreatePin(Direction, TEXT(""), TEXT(""), NULL, false, false, GetVarNameString());
		K2Schema->ConvertPropertyToPinType(VariableProperty, /*out*/ VariablePin->PinType);
		K2Schema->SetPinDefaultValueBasedOnType(VariablePin);
	}
	else
	{
		Message_Warn(*FString::Printf(TEXT("CreatePinForVariable: '%s' variable not found. Base class was probably changed."), *GetVarNameString()));
		return false;
	}

	return true;
}

void UK2Node_Variable::CreatePinForSelf()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	UClass* VariableClass = GetVariableSourceClass();
	// Create the self pin
	if (!K2Schema->FindSelfPin(*this, EGPD_Input))
	{
		if (VariableReference.IsSelfContext())
		{
			UEdGraphPin* SelfPin = CreatePin(EGPD_Input, K2Schema->PC_Object, K2Schema->PSC_Self, NULL, false, false, K2Schema->PN_Self);
			SelfPin->bHidden = true; // don't show in 'self' context
		}
		else
		{
			UEdGraphPin* TargetPin = CreatePin(EGPD_Input, K2Schema->PC_Object, TEXT(""), VariableReference.GetMemberParentClass(this), false, false, K2Schema->PN_Self);
			TargetPin->PinFriendlyName =  TEXT("Target");
		}
	}
	else
	{
		//@TODO: Check that the self pin types match!
	}
}

bool UK2Node_Variable::RecreatePinForVariable(EEdGraphPinDirection Direction, TArray<UEdGraphPin*>& OldPins)
{
	// probably the node was pasted to a blueprint without the variable
	// we don't want to beak any connection, so the pin will be recreated from old one, but compiler will throw error

	// find old variable pin
	const UEdGraphPin* OldVariablePin = NULL;
	const FString PinName = GetVarNameString();
	for(auto Iter = OldPins.CreateConstIterator(); Iter; ++Iter)
	{
		if(const UEdGraphPin* Pin = *Iter)
		{
			if(PinName == Pin->PinName)
			{
				OldVariablePin = Pin;
				break;
			}
		}
	}

	if(NULL != OldVariablePin)
	{
		// create new pin from old one
		UEdGraphPin* VariablePin = CreatePin(Direction, TEXT(""), TEXT(""), NULL, false, false, PinName);
		VariablePin->PinType = OldVariablePin->PinType;
		
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
		K2Schema->SetPinDefaultValueBasedOnType(VariablePin);

		Message_Note(*FString::Printf(TEXT("Pin for variable '%s' recreated, but the variable is missing."), *PinName));
		return true;
	}
	else
	{
		Message_Warn(*FString::Printf(TEXT("RecreatePinForVariable: '%s' pin not found"), *PinName));
		return false;
	}
}

FLinearColor UK2Node_Variable::GetNodeTitleColor() const
{
	UProperty* VariableProperty = GetPropertyForVariable();
	if (VariableProperty)
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

		FEdGraphPinType VariablePinType;
		K2Schema->ConvertPropertyToPinType(VariableProperty, VariablePinType);

		return K2Schema->GetPinTypeColor(VariablePinType);
	}

	return FLinearColor::White;
}

UK2Node::ERedirectType UK2Node_Variable::DoPinsMatchForReconstruction( const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex ) const 
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	if( OldPin->PinType.PinCategory == K2Schema->PC_Exec )
	{
		return Super::DoPinsMatchForReconstruction(NewPin, NewPinIndex, OldPin, OldPinIndex);
	}
	else if (K2Schema->ArePinTypesCompatible( NewPin->PinType, OldPin->PinType))
	{
		return ERedirectType_Name;
	}
	else if( (OldPin->PinName == NewPin->PinName) && (NewPin->PinType.PinCategory == K2Schema->PC_Object) && (NewPin->PinType.PinSubCategoryObject == NULL))
	{
		// Special Case:  If we had a pin match, and the class isn't loaded yet because of a cyclic dependency, temporarily cast away the const, and fix up.
		// @TODO:  Fix this up to be less hacky
		UBlueprintGeneratedClass* TypeClass = Cast<UBlueprintGeneratedClass>(OldPin->PinType.PinSubCategoryObject.Get());
		if(TypeClass && TypeClass->ClassGeneratedBy && TypeClass->ClassGeneratedBy->HasAnyFlags(RF_BeingRegenerated))
		{
			UEdGraphPin* NonConstNewPin = (UEdGraphPin*)NewPin;
			NonConstNewPin->PinType.PinSubCategoryObject = OldPin->PinType.PinSubCategoryObject.Get();
			return ERedirectType_Name;
		}
	}
 	else
 	{
		// Special Case:  If we're migrating from old blueprint references to class references, allow pins to be reconnected if coerced
		const UClass* PSCOClass = Cast<UClass>(OldPin->PinType.PinSubCategoryObject.Get());
 		const bool bOldIsBlueprint = PSCOClass && PSCOClass->IsChildOf(UBlueprint::StaticClass());
 		const bool bNewIsClass = (NewPin->PinType.PinCategory == K2Schema->PC_Class);
 		if(bNewIsClass && bOldIsBlueprint)
 		{
 			UEdGraphPin* OldPinNonConst = (UEdGraphPin*)OldPin;
 			OldPinNonConst->PinName = NewPin->PinName;
			return ERedirectType_Name;
 		}
 	}

	return ERedirectType_None;
}

UClass* UK2Node_Variable::GetVariableSourceClass() const
{
	UClass* Result = VariableReference.GetMemberParentClass(this);
	return Result;
}

UProperty* UK2Node_Variable::GetPropertyForVariable() const
{
	const FName VarName = GetVarName();
	UEdGraphPin* VariablePin = FindPin(GetVarNameString());

	UProperty* VariableProperty = VariableReference.ResolveMember<UProperty>(this);

	// if the variable has been deprecated, don't use it
	if(VariableProperty != NULL)
	{
		if (VariableProperty->HasAllPropertyFlags(CPF_Deprecated))
		{
			VariableProperty = NULL;
		}
		// If the variable has been remapped update the pin
		else if (VariablePin && VarName != GetVarName())
		{
			VariablePin->PinName = GetVarNameString();
		}
	}

	return VariableProperty;
}

UEdGraphPin* UK2Node_Variable::GetValuePin() const
{
	UEdGraphPin* Pin = FindPin(GetVarNameString());
	check(Pin == NULL || Pin->Direction == EGPD_Output);
	return Pin;
}

void UK2Node_Variable::ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const
{
	Super::ValidateNodeDuringCompilation(MessageLog);

	UProperty* VariableProperty = GetPropertyForVariable();
	if (VariableProperty == NULL)
	{
		MessageLog.Warning(*FString::Printf(*LOCTEXT("VariableNotFound", "Unable to find variable with name '%s' for @@").ToString(), *VariableReference.GetMemberName().ToString()), this);
	}
}

FName UK2Node_Variable::GetPaletteIcon(FLinearColor& ColorOut) const
{
	return GetVariableIconAndColor(GetVariableSourceClass(), GetVarName(), ColorOut);
}

FName UK2Node_Variable::GetVarIconFromPinType(FEdGraphPinType& InPinType, FLinearColor& IconColorOut)
{
	FName IconBrush = TEXT("Kismet.AllClasses.VariableIcon");

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	IconColorOut = K2Schema->GetPinTypeColor(InPinType);

	if(InPinType.bIsArray)
	{
		IconBrush = TEXT("Kismet.AllClasses.ArrayVariableIcon");
	}
	else if(InPinType.PinSubCategoryObject.IsValid())
	{
		UClass* VarClass = FindObject<UClass>(ANY_PACKAGE, *InPinType.PinSubCategoryObject->GetName());
		if( VarClass )
		{
			IconBrush = FClassIconFinder::FindIconNameForClass( VarClass );
		}
	}

	return IconBrush;
}

FText UK2Node_Variable::GetToolTipHeading() const
{
	FText Heading = Super::GetToolTipHeading();

	UProperty const* VariableProperty = VariableReference.ResolveMember<UProperty>(this);
	if (VariableProperty && VariableProperty->HasAllPropertyFlags(CPF_Net))
	{
		FText ReplicatedTag = LOCTEXT("ReplicatedVar", "Replicated");
		if (Heading.IsEmpty())
		{
			Heading = ReplicatedTag;
		}
		else 
		{
			Heading = FText::Format(FText::FromString("{0}\n{1}"), ReplicatedTag, Heading);
		}
	}

	return Heading;
}

FName UK2Node_Variable::GetVariableIconAndColor(UClass* VarClass, FName VarName, FLinearColor& IconColorOut)
{
	FName IconBrush = TEXT("Kismet.AllClasses.VariableIcon");

	if(VarClass != NULL)
	{
		UProperty* Property = FindField<UProperty>(VarClass, VarName);
		if(Property != NULL)
		{
			const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

			FEdGraphPinType PinType;
			if(K2Schema->ConvertPropertyToPinType(Property,  PinType)) // use schema to get the color
			{
				IconBrush = GetVarIconFromPinType(PinType, IconColorOut);
			}
		}
	}

	return IconBrush;
}


void UK2Node_Variable::CheckForErrors(const UEdGraphSchema_K2* Schema, FCompilerResultsLog& MessageLog)
{
	if(!VariableReference.IsSelfContext() && VariableReference.GetMemberParentClass(this) != NULL)
	{
		// Check to see if we're not a self context, if we have a valid context.  It may have been purged because of a dead execution chain
		UEdGraphPin* ContextPin = Schema->FindSelfPin(*this, EGPD_Input);
		if((ContextPin != NULL) && (ContextPin->LinkedTo.Num() == 0) && (ContextPin->DefaultObject == NULL))
		{
			MessageLog.Error(*LOCTEXT("VarNodeError_InvalidVarTarget", "Variable node @@ uses an invalid target.  It may depend on a node that is not connected to the execution chain, and got purged.").ToString(), this);
		}
	}
}

void UK2Node_Variable::ReconstructNode()
{
	// update the variable reference if the property was renamed
	UClass* const VarClass = GetVariableSourceClass();
	if (VarClass)
	{
		UClass* SearchClass = VarClass;
		while (SearchClass != NULL)
		{
			const TMap<FName, FName>* const ClassTaggedPropertyRedirects = UStruct::TaggedPropertyRedirects.Find( SearchClass->GetFName() );
			if (ClassTaggedPropertyRedirects)
			{
				const FName* const NewPropertyName = ClassTaggedPropertyRedirects->Find( VariableReference.GetMemberName() );
				if (NewPropertyName)
				{
					if (VariableReference.IsSelfContext())
					{
						VariableReference.SetSelfMember( *NewPropertyName );
					}
					else
					{
						VariableReference.SetExternalMember( *NewPropertyName, VarClass );
					}

					// found, can break
					break;
				}
			}

			SearchClass = SearchClass->GetSuperClass();
		}
	}

	const FGuid VarGuid = VariableReference.GetMemberGuid();
	if (VarGuid.IsValid())
	{
		const FName VarName = UBlueprint::GetFieldNameFromClassByGuid<UProperty>(VarClass, VarGuid);
		if (VarName != NAME_None && VarName != VariableReference.GetMemberName())
		{
			if (VariableReference.IsSelfContext())
			{
				VariableReference.SetSelfMember( VarName );
			}
			else
			{
				VariableReference.SetExternalMember( VarName, VarClass );
			}
		}
	}

	Super::ReconstructNode();
}

FName UK2Node_Variable::GetCornerIcon() const
{
	const UProperty* VariableProperty = VariableReference.ResolveMember<UProperty>(this);
	if (VariableProperty && VariableProperty->HasAllPropertyFlags(CPF_Net))
	{
		return TEXT("Graph.Replication.Replicated");
	}

	return Super::GetCornerIcon();
}

bool UK2Node_Variable::HasExternalBlueprintDependencies() const
{
	UClass* SourceClass = GetVariableSourceClass();
	UBlueprint* SourceBlueprint = GetBlueprint();
	return (SourceClass && (SourceClass->ClassGeneratedBy != NULL) && (SourceClass->ClassGeneratedBy != SourceBlueprint));
}

FString UK2Node_Variable::GetDocumentationLink() const
{
	if( UProperty* Property = GetPropertyForVariable() )
	{
		// discover if the variable property is a non blueprint user variable
		UClass* SourceClass = Property->GetOwnerClass();
		if( SourceClass && SourceClass->ClassGeneratedBy == NULL )
		{
			UStruct* OwnerStruct = Property->GetOwnerStruct();

			if( OwnerStruct )
			{
				return FString::Printf( TEXT("Shared/Types/%s%s"), OwnerStruct->GetPrefixCPP(), *OwnerStruct->GetName() );
			}
		}
	}
	return TEXT( "" );
}

FString UK2Node_Variable::GetDocumentationExcerptName() const
{
	return GetVarName().ToString();
}

#undef LOCTEXT_NAMESPACE
