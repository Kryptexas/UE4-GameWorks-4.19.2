// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"
#include "BlueprintPaletteFavorites.h"

/*******************************************************************************
* Static File Helpers
*******************************************************************************/

/** namespace'd to avoid collisions with united builds */
namespace BlueprintFavorites
{
	static FString const ConfigSection("BlueprintEditor.Favorites");
	static FString const NodeNameConfigField("NodeName=");
	static FString const FieldNameConfigField("FieldName=");
	static FString const CustomProfileId("CustomProfile");
	static FString const DefaultProfileConfigKey("DefaultProfile");
}

/**
 * Walks the specified object's outer hierarchy and concatenates all names along
 * the way (composing a path).
 * 
 * @param  Object	The object you want a path for.
 * @return A string representing the object's outer path.
 */
static FString ConstructObjectPath(UObject const* Object)
{
	FString Path;
	if (Object != NULL)
	{
		Path = Object->GetName();

		UObject const* Outer = Object->GetOuter();
		while (Outer != NULL)
		{
			Path = Outer->GetName() + "." + Path;
			Outer = Outer->GetOuter();
		}

	}
	
	return Path;
}

/*******************************************************************************
* FFavoritedBlueprintPaletteItem
*******************************************************************************/

//------------------------------------------------------------------------------
FFavoritedBlueprintPaletteItem::FFavoritedBlueprintPaletteItem(FString const& SerializedAction)
	: NodeClassOuter(NULL)
	, FieldOuter(NULL)
{
	FString ParsedNodeName;
	if (FParse::Value(*SerializedAction, *BlueprintFavorites::NodeNameConfigField, ParsedNodeName))
	{
		// @TODO look for redirects first

		if (ResolveName(NodeClassOuter, ParsedNodeName, false, false))
		{
			NodeClassName = ParsedNodeName;
		}
	}

	FString ParsedFieldName;
	if (FParse::Value(*SerializedAction, *BlueprintFavorites::FieldNameConfigField, ParsedFieldName))
	{
		// @TODO look for redirects first

		if (NodeClassName == UK2Node_MacroInstance::StaticClass()->GetName())
		{
			FieldName = ParsedFieldName;
		}
		else if (ResolveName(FieldOuter, ParsedFieldName, false, false))
		{
			FieldName = ParsedFieldName;
		}
	}	
}

//------------------------------------------------------------------------------
FFavoritedBlueprintPaletteItem::FFavoritedBlueprintPaletteItem(TSharedPtr<FEdGraphSchemaAction> PaletteActionIn)
	: NodeClassOuter(NULL)
	, FieldOuter(NULL)
{
	if (PaletteActionIn.IsValid())
	{
		FString const ActionId = PaletteActionIn->GetTypeId();
		if (ActionId == FEdGraphSchemaAction_K2AddComponent::StaticGetTypeId())
		{
			FEdGraphSchemaAction_K2AddComponent* AddComponentAction = (FEdGraphSchemaAction_K2AddComponent*)PaletteActionIn.Get();
			check(AddComponentAction->NodeTemplate != NULL)

			// grab information on the node's class
			UClass* NodeClass = AddComponentAction->NodeTemplate->GetClass();
			NodeClassOuter = NodeClass->GetOuter();
			NodeClassName  = NodeClass->GetName();

			FieldOuter = AddComponentAction->ComponentClass->GetOuter();
			FieldName  = AddComponentAction->ComponentClass->GetName();
		}
		// if we can pull out a node associated with this action
		else if (UK2Node const* NodeTemplate = FK2SchemaActionUtils::ExtractNodeTemplateFromAction(PaletteActionIn))
		{
			bool bIsSupported = false;
			// now, if we need more info to help identify the node, let's fill 
			// out FieldName/FieldOuter

			// with UK2Node_CallFunction node, we know we can use the function 
			// to discern between them
			if (UK2Node_CallFunction const* CallFuncNode = Cast<UK2Node_CallFunction const>(NodeTemplate))
			{
				FieldName  = CallFuncNode->FunctionReference.GetMemberName().ToString();
				UFunction const* InnerFunction = CallFuncNode->FunctionReference.ResolveMember<UFunction>(CallFuncNode);
				if (InnerFunction)
				{
					FieldOuter = InnerFunction->GetOuter();
	
					bIsSupported = true;
				}
			}
			else if (UK2Node_Event const* EventNode = Cast<UK2Node_Event const>(NodeTemplate))
			{
				FieldName  = EventNode->EventSignatureName.ToString();
				FieldOuter = EventNode->EventSignatureClass;

				bIsSupported = true;
			}
			else if (UK2Node_MacroInstance const* MacroNode = Cast<UK2Node_MacroInstance const>(NodeTemplate))
			{
				FieldName  = ConstructObjectPath(MacroNode->GetMacroGraph());
				FieldOuter = NULL;

				bIsSupported = true;
			}
			else if (Cast<UK2Node_IfThenElse const>(NodeTemplate) || 
			         Cast<UK2Node_MakeArray const>(NodeTemplate)  || 
					 Cast<UK2Node_SpawnActorFromClass const>(NodeTemplate) || 
					 Cast<UK2Node_SpawnActor const>(NodeTemplate) )
			{
				// @TODO eventually we should switch this from being inclusive to exclusive 
				//       (once we figure out what smaller subset is not explicitly supported)
				bIsSupported = true;
			}

			if (bIsSupported)
			{
				// grab information on the node's class
				UClass* NodeClass = NodeTemplate->GetClass();
				NodeClassOuter = NodeClass->GetOuter();
				NodeClassName  = NodeClass->GetName();
			}
		}
	}
}

//------------------------------------------------------------------------------
bool FFavoritedBlueprintPaletteItem::IsValid() const 
{
	return (NodeClassOuter != NULL);
}

//------------------------------------------------------------------------------
bool FFavoritedBlueprintPaletteItem::operator==(FFavoritedBlueprintPaletteItem const& Rhs) const
{
	return ((NodeClassOuter == Rhs.NodeClassOuter) && 
	        (FieldOuter == Rhs.FieldOuter)         && 
	        (NodeClassName == Rhs.NodeClassName)   && 
	        (FieldName == Rhs.FieldName));
}

//------------------------------------------------------------------------------
bool FFavoritedBlueprintPaletteItem::operator==(TSharedPtr<FEdGraphSchemaAction> PaletteAction) const
{
	return (*this == FFavoritedBlueprintPaletteItem(PaletteAction));
}

//------------------------------------------------------------------------------
FString FFavoritedBlueprintPaletteItem::ToString() const
{
	FString SerializedFavorite;
	if (IsValid())
	{
		// match the formatting we expect from the .ini files...
		SerializedFavorite += "(";
		SerializedFavorite += BlueprintFavorites::NodeNameConfigField;
		SerializedFavorite += "\"";
		SerializedFavorite += ConstructObjectPath(NodeClassOuter);
		SerializedFavorite += ".";
		SerializedFavorite += NodeClassName;
		if (!FieldName.IsEmpty())
		{
			SerializedFavorite += "\", ";
			SerializedFavorite += BlueprintFavorites::FieldNameConfigField;
			SerializedFavorite += "\"";
			if (FieldOuter != NULL)
			{
				SerializedFavorite += ConstructObjectPath(FieldOuter);
				SerializedFavorite += ":";
			}
			SerializedFavorite += FieldName;
		}
		SerializedFavorite += "\")";
	}

	return SerializedFavorite;
}

/*******************************************************************************
* UBlueprintPaletteFavorites Public Interface
*******************************************************************************/

//------------------------------------------------------------------------------
UBlueprintPaletteFavorites::UBlueprintPaletteFavorites(class FPostConstructInitializeProperties const& PCIP)
	: Super(PCIP)
{
}

//------------------------------------------------------------------------------
void UBlueprintPaletteFavorites::PostInitProperties()
{
	Super::PostInitProperties();

	if (CurrentProfile == BlueprintFavorites::CustomProfileId)
	{
		LoadCustomFavorites();
	}
	else
	{
		LoadSetProfile();
	}
}

//------------------------------------------------------------------------------
void UBlueprintPaletteFavorites::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	CustomFavorites.Empty();
	if (CurrentProfile == BlueprintFavorites::CustomProfileId)
	{
		for (FFavoritedBlueprintPaletteItem const& Favorite : CurrentFavorites) 
		{
			CustomFavorites.Add(Favorite.ToString());
		}
	}

	SaveConfig();
	OnFavoritesUpdated.Broadcast();
}

//------------------------------------------------------------------------------
bool UBlueprintPaletteFavorites::CanBeFavorited(TSharedPtr<FEdGraphSchemaAction> PaletteAction) const
{
	return FFavoritedBlueprintPaletteItem(PaletteAction).IsValid();
}

//------------------------------------------------------------------------------
bool UBlueprintPaletteFavorites::IsFavorited(TSharedPtr<FEdGraphSchemaAction> PaletteAction) const
{
	bool bIsFavorited = false;

	FFavoritedBlueprintPaletteItem ActionAsFavorite(PaletteAction);
	if (ActionAsFavorite.IsValid())
	{
		for (FFavoritedBlueprintPaletteItem const& Favorite : CurrentFavorites)
		{
			if (Favorite == ActionAsFavorite)
			{
				bIsFavorited = true;
				break;
			}
		}
	}
	return bIsFavorited;
}

//------------------------------------------------------------------------------
void UBlueprintPaletteFavorites::AddFavorite(TSharedPtr<FEdGraphSchemaAction> PaletteAction)
{
	if (!IsFavorited(PaletteAction) && CanBeFavorited(PaletteAction))
	{
		CurrentFavorites.Add(FFavoritedBlueprintPaletteItem(PaletteAction));
		SetProfile(BlueprintFavorites::CustomProfileId);
	}
}

//------------------------------------------------------------------------------
void UBlueprintPaletteFavorites::AddFavorites(TArray< TSharedPtr<FEdGraphSchemaAction> > PaletteActions)
{
	bool bAnyAdded = false;
	for (TSharedPtr<FEdGraphSchemaAction>& NewFave : PaletteActions)
	{
		if (!IsFavorited(NewFave) && CanBeFavorited(NewFave))
		{
			CurrentFavorites.Add(FFavoritedBlueprintPaletteItem(NewFave));
			bAnyAdded = true;
		}
	}

	if (bAnyAdded)
	{ 
		SetProfile(BlueprintFavorites::CustomProfileId);
	}
}

//------------------------------------------------------------------------------
void UBlueprintPaletteFavorites::RemoveFavorite(TSharedPtr<FEdGraphSchemaAction> PaletteAction)
{
	if (IsFavorited(PaletteAction))
	{
		CurrentFavorites.Remove(PaletteAction);
		SetProfile(BlueprintFavorites::CustomProfileId);
	}
}

//------------------------------------------------------------------------------
void UBlueprintPaletteFavorites::RemoveFavorites(TArray< TSharedPtr<FEdGraphSchemaAction> > PaletteActions)
{
	bool bAnyRemoved = false;
	for (TSharedPtr<FEdGraphSchemaAction>& OldFave : PaletteActions)
	{
		if (IsFavorited(OldFave))
		{
			CurrentFavorites.Remove(OldFave);
			bAnyRemoved = true;
		}
	}

	if (bAnyRemoved)
	{ 
		SetProfile(BlueprintFavorites::CustomProfileId);
	}
}

//------------------------------------------------------------------------------
void UBlueprintPaletteFavorites::LoadProfile(FString const& ProfileName)
{
	PreEditChange(FindField<UProperty>(GetClass(), TEXT("CurrentProfile")));
	{
		CurrentProfile = ProfileName;
		LoadSetProfile();
	}
	PostEditChange();
}

//------------------------------------------------------------------------------
bool UBlueprintPaletteFavorites::IsUsingCustomProfile() const
{
	return (GetCurrentProfile() == BlueprintFavorites::CustomProfileId);
}

//------------------------------------------------------------------------------
FString const& UBlueprintPaletteFavorites::GetCurrentProfile() const
{
	if (CurrentProfile.IsEmpty())
	{
		return GetDefaultProfileId();
	}
	return CurrentProfile;
}

//------------------------------------------------------------------------------
void UBlueprintPaletteFavorites::ClearAllFavorites()
{
	if (CurrentFavorites.Num() > 0) 
	{
		CurrentFavorites.Empty();
		SetProfile(BlueprintFavorites::CustomProfileId);		
	}
}

/*******************************************************************************
* UBlueprintPaletteFavorites Private Methods
*******************************************************************************/

//------------------------------------------------------------------------------
FString const& UBlueprintPaletteFavorites::GetDefaultProfileId() const
{
	static FString DefaultProfileId("DefaultFavorites");
	GConfig->GetString(*BlueprintFavorites::ConfigSection, *BlueprintFavorites::DefaultProfileConfigKey, DefaultProfileId, GEditorIni);
	return DefaultProfileId;
}

//------------------------------------------------------------------------------
void UBlueprintPaletteFavorites::LoadSetProfile()
{
	CustomFavorites.Empty();
	CurrentFavorites.Empty();

	TArray<FString> ProfileFavorites;
	// if this profile doesn't exist anymore
	if (CurrentProfile.IsEmpty() || (GConfig->GetArray(*BlueprintFavorites::ConfigSection, *CurrentProfile, ProfileFavorites, GEditorIni) == 0))
	{
		// @TODO log warning
		GConfig->GetArray(*BlueprintFavorites::ConfigSection, *GetDefaultProfileId(), ProfileFavorites, GEditorIni);
	}

	for (FString const& Favorite : ProfileFavorites)
	{
		FFavoritedBlueprintPaletteItem FavoriteItem(Favorite);
		if (ensure(FavoriteItem.IsValid()))
		{
			CurrentFavorites.Add(FavoriteItem);
		}
	}
}

//------------------------------------------------------------------------------
void UBlueprintPaletteFavorites::LoadCustomFavorites()
{
	CurrentFavorites.Empty();
	check(CurrentProfile == BlueprintFavorites::CustomProfileId);

	for (FString const& Favorite : CustomFavorites)
	{
		FFavoritedBlueprintPaletteItem FavoriteItem(Favorite);
		if (ensure(FavoriteItem.IsValid()))
		{
			CurrentFavorites.Add(FavoriteItem);
		}
	}	
}

//------------------------------------------------------------------------------
void UBlueprintPaletteFavorites::SetProfile(FString const& ProfileName)
{
	PreEditChange(FindField<UProperty>(GetClass(), TEXT("CurrentProfile")));
	{
		CurrentProfile = ProfileName;
	}
	PostEditChange();
}
