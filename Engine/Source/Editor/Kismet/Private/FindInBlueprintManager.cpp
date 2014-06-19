#include "BlueprintEditorPrivatePCH.h"

#include "FindInBlueprintManager.h"
#include "FindInBlueprints.h"
#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"
#include "AssetRegistryModule.h"
#include "DerivedDataCacheInterface.h"
#include "DerivedDataPluginInterface.h"

#include "JsonUtilities.h"

#define LOCTEXT_NAMESPACE "FindInBlueprintManager"

FFindInBlueprintSearchManager* FFindInBlueprintSearchManager::Instance = NULL;

const FText FFindInBlueprintSearchTags::FiB_Properties = LOCTEXT("Properties", "Properties");

const FText FFindInBlueprintSearchTags::FiB_Nodes = LOCTEXT("Nodes", "Nodes");

const FText FFindInBlueprintSearchTags::FiB_SchemaName = LOCTEXT("SchemaName", "SchemaName");

const FText FFindInBlueprintSearchTags::FiB_UberGraphs = LOCTEXT("Uber", "Uber");
const FText FFindInBlueprintSearchTags::FiB_Functions = LOCTEXT("Functions", "Functions");
const FText FFindInBlueprintSearchTags::FiB_Macros = LOCTEXT("Macros", "Macros");
const FText FFindInBlueprintSearchTags::FiB_SubGraphs = LOCTEXT("Sub", "Sub");

const FText FFindInBlueprintSearchTags::FiB_Name = LOCTEXT("Name", "Name");
const FText FFindInBlueprintSearchTags::FiB_ClassName = LOCTEXT("ClassName", "ClassName");
const FText FFindInBlueprintSearchTags::FiB_NodeGuid = LOCTEXT("NodeGuid", "NodeGuid");
const FText FFindInBlueprintSearchTags::FiB_Tooltip = LOCTEXT("Tooltip", "Tooltip");
const FText FFindInBlueprintSearchTags::FiB_DefaultValue = LOCTEXT("DefaultValue", "DefaultValue");
const FText FFindInBlueprintSearchTags::FiB_Description = LOCTEXT("Description", "Description");
const FText FFindInBlueprintSearchTags::FiB_Comment = LOCTEXT("Comment", "Comment");

const FText FFindInBlueprintSearchTags::FiB_Pins = LOCTEXT("Pins", "Pins");
const FText FFindInBlueprintSearchTags::FiB_PinCategory = LOCTEXT("PinCategory", "PinCategory");
const FText FFindInBlueprintSearchTags::FiB_PinSubCategory = LOCTEXT("SubCategory", "SubCategory");
const FText FFindInBlueprintSearchTags::FiB_ObjectClass = LOCTEXT("ObjectClass", "ObjectClass");
const FText FFindInBlueprintSearchTags::FiB_IsArray = LOCTEXT("IsArray", "IsArray");
const FText FFindInBlueprintSearchTags::FiB_IsReference = LOCTEXT("IsReference", "IsReference");
const FText FFindInBlueprintSearchTags::FiB_Glyph = LOCTEXT("Glyph", "Glyph");
const FText FFindInBlueprintSearchTags::FiB_GlyphColor = LOCTEXT("GlyphColor", "GlyphColor");

namespace BlueprintSearchMetaDataHelpers
{
	typedef TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>> SearchMetaDataWriter;

	/** Helper function to set a Json string field using FText's, the FText's will be serialized and converted into hex strings for storage */
	void SetJsonTextField(TSharedRef< FJsonObject > InOutJsonObject, FText InFieldName, FText InFieldValue)
	{
		InOutJsonObject->SetStringField(FFindInBlueprintSearchManager::ConvertFTextToHexString(InFieldName), FFindInBlueprintSearchManager::ConvertFTextToHexString(InFieldValue));
	}

	/**
	 * Checks if Json value is searchable, eliminating data that not considered useful to search for
	 *
	 * @param InJsonValue		The Json value object to examine for searchability
	 * @return					TRUE if the value should be searchable
	 */
	bool CheckIfJsonValueIsSearchable( TSharedPtr< FJsonValue > InJsonValue )
	{
		/** Check for interesting values
		 *  booleans are not interesting, there are a lot of them
		 *  strings are not interesting if they are empty
		 *  numbers are not interesting if they are 0
		 *  arrays are not interesting if they are empty or if they are filled with un-interesting types
		 *  objects may not have interesting values when dug into
		 */
		bool bValidPropetyValue = true;
		if(InJsonValue->Type == EJson::Boolean || InJsonValue->Type == EJson::None || InJsonValue->Type == EJson::Null)
		{
			bValidPropetyValue = false;
		}
		else if(InJsonValue->Type == EJson::String)
		{
			FString temp = InJsonValue->AsString();
			if(InJsonValue->AsString().IsEmpty())
			{
				bValidPropetyValue = false;
			}
		}
		else if(InJsonValue->Type == EJson::Number)
		{
			if(InJsonValue->AsNumber() == 0.0)
			{
				bValidPropetyValue = false;
			}
		}
		else if(InJsonValue->Type == EJson::Array)
		{
			auto JsonArray = InJsonValue->AsArray();
			if(JsonArray.Num() > 0)
			{
				// Some types are never interesting and the contents of the array should be ignored. Other types can be interesting, the contents of the array should be stored (even if
				// the values may not be interesting, so that index values can be obtained)
				if(JsonArray[0]->Type != EJson::Array && JsonArray[0]->Type != EJson::String && JsonArray[0]->Type != EJson::Number && JsonArray[0]->Type != EJson::Object)
				{
					bValidPropetyValue = false;
				}
			}
		}
		else if(InJsonValue->Type == EJson::Object)
		{
			// Start it out as not being valid, if we find any sub-items that are searchable, it will be marked to TRUE
			bValidPropetyValue = false;

			// Go through all value/key pairs to see if any of them are searchable, remove the ones that are not
			auto JsonObject = InJsonValue->AsObject();
			for(auto Iter = JsonObject->Values.CreateIterator(); Iter; ++Iter)
			{
				if(!CheckIfJsonValueIsSearchable(Iter->Value))
				{
					Iter.RemoveCurrent();
				}
				else
				{
					bValidPropetyValue = true;
				}
			}
			
		}

		return bValidPropetyValue;
	}

	/**
	 * Saves a graph pin type to a Json object
	 *
	 * @param InPinType				The pin type to save
	 * @param InOutJsonObject		The Json object to save the pintype to
	 */
	void SavePinTypeToJson(const FEdGraphPinType& InPinType, TSharedRef< FJsonObject > InOutJsonObject)
	{
		// Only save strings that are not empty

		if(!InPinType.PinCategory.IsEmpty())
		{
			SetJsonTextField(InOutJsonObject, FFindInBlueprintSearchTags::FiB_PinCategory, FText::FromString(InPinType.PinCategory));
		}
		
		if(!InPinType.PinSubCategory.IsEmpty())
		{
			SetJsonTextField(InOutJsonObject, FFindInBlueprintSearchTags::FiB_PinSubCategory, FText::FromString(InPinType.PinSubCategory));
		}

		if(InPinType.PinSubCategoryObject.IsValid())
		{
			SetJsonTextField(InOutJsonObject, FFindInBlueprintSearchTags::FiB_ObjectClass, FText::FromString(InPinType.PinSubCategoryObject->GetName()));
		}
		InOutJsonObject->SetBoolField(FFindInBlueprintSearchManager::ConvertFTextToHexString(FFindInBlueprintSearchTags::FiB_IsArray), InPinType.bIsArray);
		InOutJsonObject->SetBoolField(FFindInBlueprintSearchManager::ConvertFTextToHexString(FFindInBlueprintSearchTags::FiB_IsReference), InPinType.bIsReference);
	}

	/** Recursively digs into a JsonValue to correct all Strings into FTexts, including all fields keys. */
	void FixUpJsonValueToStrings(TSharedRef< FJsonValue >& InJsonValue)
	{
		if(InJsonValue->Type == EJson::String)
		{
			InJsonValue = MakeShareable( new FJsonValueString(FFindInBlueprintSearchManager::ConvertFTextToHexString(FText::FromString(InJsonValue->AsString()))));
		}
		else if(InJsonValue->Type == EJson::Object)
		{
			TSharedRef< FJsonObject > NewJsonObject = MakeShareable(new FJsonObject);

			TSharedPtr< FJsonObject > ParentObject = InJsonValue->AsObject();

			if(ParentObject.IsValid())
			{
				// Go through all object sub-values and attempt to potentially fix up any strings
				for( auto MapValues : ParentObject->Values )
				{
					TSharedRef< FJsonValue > JsonValueAsSharedRef = MapValues.Value.ToSharedRef();
					FixUpJsonValueToStrings(JsonValueAsSharedRef);

					NewJsonObject->SetField(FFindInBlueprintSearchManager::ConvertFTextToHexString(FText::FromString(MapValues.Key)), JsonValueAsSharedRef);
				}
			}


			InJsonValue = MakeShareable( new FJsonValueObject(NewJsonObject));
		}
		else if( InJsonValue->Type == EJson::Array)
		{
			// Build a new Json array and replace the old one
			TArray< TSharedPtr< FJsonValue > > JsonArrayData = InJsonValue->AsArray();
			TArray< TSharedPtr< FJsonValue > > NewJsonArrayData;
			for( TSharedPtr< FJsonValue >& JsonArrayItem : JsonArrayData)
			{
				TSharedRef< FJsonValue > JsonValueAsSharedRef = JsonArrayItem.ToSharedRef();
				FixUpJsonValueToStrings(JsonValueAsSharedRef);

				NewJsonArrayData.Add(JsonValueAsSharedRef);
			}

			InJsonValue = MakeShareable(new FJsonValueArray(NewJsonArrayData));
		}
	}

	/**
	 * Helper function to save a variable description to Json
	 *
	 * @param InWriter					Json writer object
	 * @param InBlueprint				Blueprint the property for the variable can be found in, if any
	 * @param InVariableDescription		The variable description being serialized to Json
	 */
	void SaveVariableDescriptionToJson(TSharedRef< SearchMetaDataWriter>& InWriter, const UBlueprint* InBlueprint, const FBPVariableDescription& InVariableDescription)
	{
		FEdGraphPinType VariableType = InVariableDescription.VarType;

		TSharedRef< FJsonObject > JsonObject = MakeShareable(new FJsonObject);
		SetJsonTextField(JsonObject, FFindInBlueprintSearchTags::FiB_Name, FText::FromString(InVariableDescription.FriendlyName));

		// Find the variable's tooltip
		FString TooltipResult;
		FBlueprintEditorUtils::GetBlueprintVariableMetaData(InBlueprint, InVariableDescription.VarName, FBlueprintMetadata::MD_Tooltip, TooltipResult);
		SetJsonTextField(JsonObject, FFindInBlueprintSearchTags::FiB_Tooltip, FText::FromString(TooltipResult));

		// Save the variable's pin type
		BlueprintSearchMetaDataHelpers::SavePinTypeToJson(VariableType, JsonObject);

		// Find the UProperty and convert it into a Json value.
		UProperty* VariableProperty = FindField<UProperty>(InBlueprint->GeneratedClass, InVariableDescription.VarName);
		if(VariableProperty)
		{
			const uint8* PropData = VariableProperty->ContainerPtrToValuePtr<uint8>(InBlueprint->GeneratedClass->GetDefaultObject());
			auto JsonValue = FJsonObjectConverter::UPropertyToJsonValue(VariableProperty, PropData, 0, 0);

			
			// Only use the value if it is searchable
			if(BlueprintSearchMetaDataHelpers::CheckIfJsonValueIsSearchable(JsonValue))
			{
				TSharedRef< FJsonValue > JsonValueAsSharedRef = JsonValue.ToSharedRef();
				FixUpJsonValueToStrings(JsonValueAsSharedRef);
				//FixUpJsonValueToStrings(JsonValue.ToSharedRef());
				JsonObject->SetField(FFindInBlueprintSearchManager::ConvertFTextToHexString(FFindInBlueprintSearchTags::FiB_DefaultValue), JsonValueAsSharedRef);
				//JsonObject->SetStringField(FFindInBlueprintSearchManager::ConvertFTextToHexString(FFindInBlueprintSearchTags::FiB_DefaultValue), FFindInBlueprintSearchManager::ConvertFTextToHexString(FText::FromString(JsonValue->AsString())));
			}
		}

		// Serialize the object out using the writer, so that it turns into raw text in the metadata string being made
		FJsonSerializer::Serialize(JsonObject, InWriter);
	}

	/**
	 * Gathers all nodes from a specified graph and serializes their searchable data to Json
	 *
	 * @param InWriter		The Json writer to use for serialization
	 * @param InGraph		The graph to search through
	 */
	void GatherNodesFromGraph(TSharedRef< SearchMetaDataWriter>& InWriter, const UEdGraph* InGraph)
	{
		// Collect all macro graphs
		InWriter->WriteArrayStart(FFindInBlueprintSearchManager::ConvertFTextToHexString(FFindInBlueprintSearchTags::FiB_Nodes));
		{
			for(auto* Node : InGraph->Nodes)
			{
				if(Node)
				{
					// Make sure we don't collect search data for nodes that are going away soon
					if( Node->GetOuter()->IsPendingKill() )
					{
						continue;
					}

					TSharedRef< FJsonObject > JsonNodeObject = MakeShareable(new FJsonObject);

					// Retrieve the search metadata from the node, some node types may have extra metadata to be searchable.
					TArray<struct FSearchTagDataPair> Tags;
					Node->AddSearchMetaDataInfo(Tags);

					// Go through the node metadata tags and put them into the Json object.
					for(const FSearchTagDataPair& SearchData : Tags)
					{
						FString Key = FFindInBlueprintSearchManager::ConvertFTextToHexString(SearchData.Key);
						FString Value = FFindInBlueprintSearchManager::ConvertFTextToHexString(SearchData.Value);
						JsonNodeObject->SetStringField(Key, Value);
					}

					// Find all the pins and extract their metadata
					TArray<TSharedPtr< FJsonValue > > JsonPinList;
					for(UEdGraphPin* Pin : Node->Pins)
					{
						TSharedRef< FJsonObject > JsonPinObject = MakeShareable(new FJsonObject);
						SetJsonTextField(JsonPinObject, FFindInBlueprintSearchTags::FiB_Name, FText::FromString(Pin->GetSchema()->GetPinDisplayName(Pin)));
						SetJsonTextField(JsonPinObject, FFindInBlueprintSearchTags::FiB_DefaultValue, FText::FromString(Pin->GetDefaultAsString()));
						BlueprintSearchMetaDataHelpers::SavePinTypeToJson(Pin->PinType, JsonPinObject);

						// Add the Json pin data to the pin list
						JsonPinList.Add(MakeShareable(new FJsonValueObject(JsonPinObject)));
					}
					JsonNodeObject->SetArrayField(FFindInBlueprintSearchManager::ConvertFTextToHexString(FFindInBlueprintSearchTags::FiB_Pins), JsonPinList);

					// Serialize the object out using the writer, so that it turns into raw text in the metadata string being made
					FJsonSerializer::Serialize(JsonNodeObject, InWriter);
				}
			}
		}
		InWriter->WriteArrayEnd();
	}

	/** 
	 * Gathers all graph's search data (and subojects) and serializes them to Json
	 *
	 * @param InWriter			The Json writer to use for serialization
	 * @param InGraphArray		All the graphs to process
	 * @param InTitle			The array title to place these graphs into
	 * @param InOutSubGraphs	All the subgraphs that need to be processed later
	 */
	void GatherGraphSearchData(TSharedRef< SearchMetaDataWriter>& InWriter, const UBlueprint* InBlueprint, const TArray< UEdGraph* >& InGraphArray, FText InTitle, TArray< UEdGraph* >* InOutSubGraphs)
	{
		if(InGraphArray.Num() > 0)
		{
			// Collect all graphs
			InWriter->WriteArrayStart(FFindInBlueprintSearchManager::ConvertFTextToHexString(InTitle));
			{
				for(const UEdGraph* Graph : InGraphArray)
				{
					InWriter->WriteObjectStart();

					FGraphDisplayInfo DisplayInfo;
					Graph->GetSchema()->GetGraphDisplayInformation(*Graph, DisplayInfo);
					InWriter->WriteValue(FFindInBlueprintSearchManager::ConvertFTextToHexString(FFindInBlueprintSearchTags::FiB_Name), FFindInBlueprintSearchManager::ConvertFTextToHexString(DisplayInfo.PlainName));

					FString GraphDescription = FBlueprintEditorUtils::GetGraphDescription(Graph).ToString();
					if(!GraphDescription.IsEmpty())
					{
						InWriter->WriteValue(FFindInBlueprintSearchManager::ConvertFTextToHexString(FFindInBlueprintSearchTags::FiB_Description), FFindInBlueprintSearchManager::ConvertFTextToHexString(FText::FromString(GraphDescription)));
					}

					// All nodes will appear as children to the graph in search results
					GatherNodesFromGraph(InWriter, Graph);

					// Collect local variables
					TArray<UK2Node_FunctionEntry*> FunctionEntryNodes;
					Graph->GetNodesOfClass<UK2Node_FunctionEntry>(FunctionEntryNodes);

					InWriter->WriteArrayStart(FFindInBlueprintSearchManager::ConvertFTextToHexString(FFindInBlueprintSearchTags::FiB_Properties));
					{
						// Search in all FunctionEntry nodes for their local variables and add them to the list
						FString ActionCategory;
						for (UK2Node_FunctionEntry* const FunctionEntry : FunctionEntryNodes)
						{
							for( const FBPVariableDescription& Variable : FunctionEntry->LocalVariables )
							{
								SaveVariableDescriptionToJson(InWriter, InBlueprint, Variable);
							}
						}
					}
					InWriter->WriteArrayEnd(); // Properties

					InWriter->WriteObjectEnd();

					// Only if asked to do it
					if(InOutSubGraphs)
					{
						Graph->GetAllChildrenGraphs(*InOutSubGraphs);
					}
				}
			}
			InWriter->WriteArrayEnd();
		}
	}
}

class FCacheAllBlueprintsTickableObject
	: public FTickableEditorObject
{

public:

	FCacheAllBlueprintsTickableObject(TArray<FName>& InUncachedBlueprints)
		: TickCacheIndex(0)
		, UncachedBlueprints(InUncachedBlueprints)
	{
		// Start the Blueprint indexing 'progress' notification
		FNotificationInfo Info( LOCTEXT("BlueprintIndexMessage", "Indexing Blueprints...") );
		Info.bFireAndForget = false;
		Info.ButtonDetails.Add(FNotificationButtonInfo(
			LOCTEXT("BlueprintIndexCancel","Cancel"),
			LOCTEXT("BlueprintIndexCancelToolTip","Cancels indexing Blueprints."), FSimpleDelegate::CreateRaw(this, &FCacheAllBlueprintsTickableObject::OnCancelCaching)));

		ProgressNotification = FSlateNotificationManager::Get().AddNotification(Info);
		if (ProgressNotification.IsValid())
		{
			ProgressNotification.Pin()->SetCompletionState(SNotificationItem::CS_Pending);
		}
	}

	~FCacheAllBlueprintsTickableObject()
	{
		
	}

	/** Returns the current cache index of the object */
	int32 GetCurrentCacheIndex() const
	{
		return TickCacheIndex + 1;
	}

	/** Returns the name of the current Blueprint being cached */
	FString GetCurrentCacheBlueprintName() const
	{
		if(UncachedBlueprints.Num() && TickCacheIndex >= 0)
		{
			return UncachedBlueprints[TickCacheIndex].ToString();
		}
		return FString();
	}

	/** Returns the progress as a percent */
	float GetCacheProgress() const
	{
		return (float)TickCacheIndex / (float)UncachedBlueprints.Num();
	}

	/** Cancels caching and destroys this object */
	void OnCancelCaching()
	{
		ProgressNotification.Pin()->SetText(LOCTEXT("BlueprintIndexCancelled", "Cancelled Indexing Blueprints!"));

		ProgressNotification.Pin()->SetCompletionState(SNotificationItem::CS_Fail);
		ProgressNotification.Pin()->ExpireAndFadeout();

		FFindInBlueprintSearchManager::Get().FinishedCachingBlueprints(TickCacheIndex);
	}

protected:
	/** FTickableEditorObject interface */
	virtual void Tick(float DeltaTime) override
	{
		if(GWarn->ReceivedUserCancel())
		{
			FFindInBlueprintSearchManager::Get().FinishedCachingBlueprints(TickCacheIndex);
		}
		else
		{
			FAssetRegistryModule* AssetRegistryModule = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

			FAssetData AssetData = AssetRegistryModule->Get().GetAssetByObjectPath(UncachedBlueprints[TickCacheIndex]);

			bool bIsLoaded = AssetData.IsAssetLoaded();

			UObject* Asset = AssetData.GetAsset();

			// If the Blueprint was not just loaded, cache it again, otherwise it was cached on load.
			if(!bIsLoaded)
			{
				FFindInBlueprintSearchManager::Get().AddOrUpdateBlueprintSearchMetadata(Cast<UBlueprint>(Asset), true);
			}

			++TickCacheIndex;

			// Check if done caching Blueprints
			if(TickCacheIndex == UncachedBlueprints.Num())
			{
				ProgressNotification.Pin()->SetCompletionState(SNotificationItem::CS_Success);
				ProgressNotification.Pin()->ExpireAndFadeout();

				ProgressNotification.Pin()->SetText(LOCTEXT("BlueprintIndexComplete", "Finished indexing Blueprints!"));

				FFindInBlueprintSearchManager::Get().FinishedCachingBlueprints(TickCacheIndex);
			}
			else
			{
				FFormatNamedArguments Args;
				Args.Add(TEXT("Percent"), FText::AsPercent(GetCacheProgress()));
				ProgressNotification.Pin()->SetText(FText::Format(LOCTEXT("BlueprintIndexProgress", "Indexing Blueprints... ({Percent})"), Args));
			}
		}
	}

	virtual bool IsTickable() const override
	{
		return true;
	}

	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FCacheAllBlueprintsTickableObject, STATGROUP_Tickables);
	}

private:

	/** The current index, increases at a rate of once per tick */
	int32 TickCacheIndex;

	/** The list of uncached Blueprints that are in the process of being cached */
	TArray<FName> UncachedBlueprints;

	/** Notification that appears and details progress */
	TWeakPtr<SNotificationItem> ProgressNotification;
};

FFindInBlueprintSearchManager& FFindInBlueprintSearchManager::Get()
{
	if (Instance == NULL)
	{
		Instance = new FFindInBlueprintSearchManager();
		Instance->Initialize();
	}

	return *Instance;
}

FFindInBlueprintSearchManager::FFindInBlueprintSearchManager()
	: bIsPausing(false)
	, CachingObject(NULL)
{
}

FFindInBlueprintSearchManager::~FFindInBlueprintSearchManager()
{
	FinishedCachingBlueprints(0);

	AssetRegistryModule->Get().OnAssetAdded().RemoveAll(this);
	AssetRegistryModule->Get().OnAssetRemoved().RemoveAll(this);
	AssetRegistryModule->Get().OnAssetRenamed().RemoveAll(this);
	FCoreDelegates::PreGarbageCollect.RemoveAll(this);
	FCoreDelegates::PostGarbageCollect.RemoveAll(this);
	FCoreDelegates::OnAssetLoaded.RemoveAll(this);
}

void FFindInBlueprintSearchManager::Initialize()
{
	AssetRegistryModule = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule->Get().OnAssetAdded().AddRaw(this, &FFindInBlueprintSearchManager::OnAssetAdded);
	AssetRegistryModule->Get().OnAssetRemoved().AddRaw(this, &FFindInBlueprintSearchManager::OnAssetRemoved);
	AssetRegistryModule->Get().OnAssetRenamed().AddRaw(this, &FFindInBlueprintSearchManager::OnAssetRenamed);
	FCoreDelegates::PreGarbageCollect.AddRaw(this, &FFindInBlueprintSearchManager::PauseFindInBlueprintSearch);
	FCoreDelegates::PostGarbageCollect.AddRaw(this, &FFindInBlueprintSearchManager::UnpauseFindInBlueprintSearch);
	FCoreDelegates::OnAssetLoaded.AddRaw(this, &FFindInBlueprintSearchManager::OnAssetLoaded);

	// Do an immediate load of the cache to catch any Blueprints that were discovered by the asset registry before we initialized.
	BuildCache();
}

void FFindInBlueprintSearchManager::OnAssetAdded(const FAssetData& InAssetData)
{
	if(InAssetData.GetClass()->IsChildOf(UBlueprint::StaticClass()))
	{
		FString BlueprintPackagePath = FPaths::GetPath(InAssetData.ObjectPath.ToString()) / FPaths::GetBaseFilename(InAssetData.ObjectPath.ToString());

		// Confirm that the Blueprint has not been added already, this can occur during duplication of Blueprints.
		int32* IndexPtr = SearchMap.Find(BlueprintPackagePath);
		if(!IndexPtr)
		{
			FString SearchGuid;
			const FString* SearchGuidPtr = InAssetData.TagsAndValues.Find("SearchGuid");
			if(SearchGuidPtr)
			{
				SearchGuid = *SearchGuidPtr;
			}
			else
			{
				SearchGuid = FMD5::HashAnsiString(*InAssetData.ObjectPath.ToString());

				if(InAssetData.IsAssetLoaded())
				{
					AddOrUpdateBlueprintSearchMetadata(Cast<UBlueprint>(InAssetData.GetAsset()));
				}
			}

			FDerivedDataCacheInterface& DDC = GetDerivedDataCacheRef();
			FSearchData NewSearchData;
			if(InAssetData.IsAssetLoaded())
			{
				NewSearchData.Blueprint = Cast<UBlueprint>(InAssetData.GetAsset());
			}
			NewSearchData.BlueprintPath = InAssetData.ObjectPath.ToString();
			NewSearchData.DDCRetrievalID = DDC.GetAsynchronous(*SearchGuid);
			NewSearchData.bMarkedForDeletion = false;

			int32 ArrayIndex = SearchArray.Add(NewSearchData);

			// Add the asset file path to the map along with the index into the array
			SearchMap.Add(BlueprintPackagePath, ArrayIndex);
		}
	}
}

void FFindInBlueprintSearchManager::OnAssetRemoved(const class FAssetData& InAssetData)
{
	if(InAssetData.IsAssetLoaded())
	{
		int32* SearchIdx = SearchMap.Find(InAssetData.GetAsset()->GetOutermost()->GetPathName());
		if(SearchIdx)
		{
			SearchArray[*SearchIdx].bMarkedForDeletion = true;
		}
	}
}

void FFindInBlueprintSearchManager::OnAssetRenamed(const class FAssetData& InAssetData, const FString& InOldName)
{
	if(InAssetData.IsAssetLoaded())
	{
		int32* SearchIdx = SearchMap.Find(FPaths::GetPath(InOldName) / FPaths::GetBaseFilename(InOldName));
		if(SearchIdx)
		{
			SearchArray[*SearchIdx].bMarkedForDeletion = true;
		}
	}
}

void FFindInBlueprintSearchManager::OnAssetLoaded(UObject* InAsset)
{
	if(UBlueprint* Blueprint = Cast<UBlueprint>(InAsset))
	{
		// Find and update the item in the search array. Searches may currently be active, this will do no harm to them

		FString BlueprintPackagePath = InAsset->GetPathName();

		// Confirm that the Blueprint has not been added already, this can occur during duplication of Blueprints.
		int32* IndexPtr = SearchMap.Find(BlueprintPackagePath);

		// The asset registry might not have informed us of this asset yet.
		if(IndexPtr)
		{
			// That index should never have a Blueprint already
			check(!SearchArray[*IndexPtr].Blueprint);

			SearchArray[*IndexPtr].Blueprint = Blueprint;
		}
	}
}

FString FFindInBlueprintSearchManager::GatherBlueprintSearchMetadata(const UBlueprint* Blueprint)
{
	FString SearchMetaData;

	// The search registry tags for a Blueprint are all in Json
	TSharedRef< BlueprintSearchMetaDataHelpers::SearchMetaDataWriter > Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create( &SearchMetaData );

	TMap<FString, TMap<FString,int>> AllPaths;
	Writer->WriteObjectStart();

	// Pull out all nodes and their pins to be searchable
	TArray<UEdGraphNode*> Nodes;
	FBlueprintEditorUtils::GetAllNodesOfClass(Blueprint, Nodes );

	// Only pull properties if the Blueprint has been compiled
	if(Blueprint->SkeletonGeneratedClass)
	{
		Writer->WriteArrayStart(FFindInBlueprintSearchManager::ConvertFTextToHexString(FFindInBlueprintSearchTags::FiB_Properties));
		{
			for( const FBPVariableDescription& Variable : Blueprint->NewVariables )
			{
				BlueprintSearchMetaDataHelpers::SaveVariableDescriptionToJson(Writer, Blueprint, Variable);
			}
		}
		Writer->WriteArrayEnd(); // Properties
	}

	// Gather all graph searchable data
	TArray< UEdGraph* > SubGraphs;
	BlueprintSearchMetaDataHelpers::GatherGraphSearchData(Writer, Blueprint, Blueprint->UbergraphPages, FFindInBlueprintSearchTags::FiB_UberGraphs, &SubGraphs);
	BlueprintSearchMetaDataHelpers::GatherGraphSearchData(Writer, Blueprint, Blueprint->FunctionGraphs, FFindInBlueprintSearchTags::FiB_Functions, &SubGraphs);
	BlueprintSearchMetaDataHelpers::GatherGraphSearchData(Writer, Blueprint, Blueprint->MacroGraphs, FFindInBlueprintSearchTags::FiB_Macros, &SubGraphs);
	// Sub graphs are processed separately so that they do not become children in the TreeView, cluttering things up if the tree is deep
	BlueprintSearchMetaDataHelpers::GatherGraphSearchData(Writer, Blueprint, SubGraphs, FFindInBlueprintSearchTags::FiB_SubGraphs, NULL);

	Writer->WriteObjectEnd();
	Writer->Close();

	return SearchMetaData;
}

void FFindInBlueprintSearchManager::AddOrUpdateBlueprintSearchMetadata(UBlueprint* InBlueprint, bool bInForceReCache/* = false*/)
{
	check(InBlueprint);

	// Allow only one thread to query the DDC at a single time, this is important in the case that two threads are waiting for the same DDC information, once the information is pulled, it is deleted and is no longer accessible.
	FScopeLock ScopeLock(&SafeModifyCacheCriticalSection);

	FString BlueprintPackagePath = InBlueprint->GetOutermost()->GetPathName();

	int32* IndexPtr = SearchMap.Find(BlueprintPackagePath);
	int32 Index = 0;
	if(!IndexPtr)
	{
		FSearchData SearchData;
		SearchData.Blueprint = InBlueprint;
		SearchData.BlueprintPath = InBlueprint->GetPathName();
		Index = SearchArray.Add(SearchData);

		SearchMap.Add(BlueprintPackagePath, Index);
	}
	else
	{
		Index = *IndexPtr;
	}

	FDerivedDataCacheInterface& DDC = GetDerivedDataCacheRef();

	FAssetData AssetData = AssetRegistryModule->Get().GetAssetByObjectPath(*InBlueprint->GetPathName());

	FString* SearchGuidPtr = AssetData.TagsAndValues.Find("SearchGuid");
	FString SearchGuid;

	// If there is no search Guid stored, generate one using the asset's path. This transitions Blueprints from the old search system to the new one without resaving
	if(!SearchGuidPtr)
	{
		SearchGuid = FMD5::HashAnsiString(*AssetData.ObjectPath.ToString());
	}
	else
	{
		SearchGuid = *SearchGuidPtr;
	}

	// If the Blueprint has been cached, don't cache it again, it's wasted time
	if(!DDC.CachedDataProbablyExists(*SearchGuid) || bInForceReCache)
	{
		// The data is valid, but we may still need to wait on the DDC.
		if(SearchArray[Index].DDCRetrievalID != INDEX_NONE)
		{
			// Allow only one thread to query the DDC at a single time, this is important in the case that two threads are waiting for the same DDC information, once the information is pulled, it is deleted and is no longer accessible.
			FScopeLock ScopeLock(&SafeModifyCacheCriticalSection);

			// If there was a DDC retrieval request, we have to see it through
			if(SearchArray[Index].DDCRetrievalID != INDEX_NONE)
			{
				// Wait for completion of the DDC retrieval, we are on a separate thread so this will not stall the editor
				DDC.WaitAsynchronousCompletion(SearchArray[Index].DDCRetrievalID);

				// Fetch the data, we don't need it but the DDC doesn't need to manage the query anymore
				TArray<uint8> DerivedData;
				DDC.GetAsynchronousResults(SearchArray[Index].DDCRetrievalID, DerivedData);

				SearchArray[Index].DDCRetrievalID = INDEX_NONE;
			}
		}

		// Build the search data
		SearchArray[Index].BlueprintPath = InBlueprint->GetPathName();
		SearchArray[Index].Value = GatherBlueprintSearchMetadata(InBlueprint);
		SearchArray[Index].bMarkedForDeletion = false;

		// Save the search data to the DDC
		TArray<uint8> DerivedData;
		FMemoryWriter Ar(DerivedData, /*bIsPersistent=*/ true);
		Ar << SearchArray[Index].BlueprintPath;
		Ar << SearchArray[Index].Value;

		// Insert the search data into the DDC
		check(DerivedData.Num());
		DDC.Put(*SearchGuid, DerivedData);

		Ar.Close();
	}
}

void FFindInBlueprintSearchManager::BeginSearchQuery(const FStreamSearch* InSearchOriginator)
{
	// Cannot begin a search thread while saving
	FScopeLock ScopeLock(&PauseThreadsCriticalSection);
	FScopeLock ScopeLock2(&SafeQueryModifyCriticalSection);

	ActiveSearchCounter.Increment();
	ActiveSearchQueries.FindOrAdd(InSearchOriginator) = 0;
}

bool FFindInBlueprintSearchManager::ContinueSearchQuery(const FStreamSearch* InSearchOriginator, FSearchData& OutSearchData)
{
	// Check if the thread has been told to pause, this occurs for the Garbage Collector and for saving to disk
	if(bIsPausing == true)
	{
		// Pause all searching, the GC is running and we will also be saving the database
		ActiveSearchCounter.Decrement();
		FScopeLock ScopeLock(&PauseThreadsCriticalSection);
		ActiveSearchCounter.Increment();
	}

	int32* SearchIdxPtr = NULL;

	{
		// Must lock this behind a critical section to ensure that no other thread is accessing it at the same time
		FScopeLock ScopeLock(&SafeQueryModifyCriticalSection);
		SearchIdxPtr = ActiveSearchQueries.Find(InSearchOriginator);
	}

	if(SearchIdxPtr)
	{
		int32& SearchIdx = *SearchIdxPtr;
		while(SearchIdx < SearchArray.Num())
		{
			// If the Blueprint is not marked for deletion, and the asset is valid, we will check to see if we want to refresh the searchable data.
			if( SearchArray[SearchIdx].bMarkedForDeletion || (SearchArray[SearchIdx].Blueprint && SearchArray[SearchIdx].Blueprint->HasAllFlags(RF_PendingKill)) )
			{
				// Mark it for deletion, it will be removed on next save
				SearchArray[SearchIdx].bMarkedForDeletion = true;
			}
			else if(SearchArray[SearchIdx].Blueprint)
			{
				// Don't rebuild the search data if the Blueprint isn't dirty (the package) or possibly dirty (compilation).
				if(Cast<UPackage>(SearchArray[SearchIdx].Blueprint->GetOutermost())->IsDirty() || SearchArray[SearchIdx].Blueprint->IsPossiblyDirty())
				{
					// Update the Blueprint's search metadata so our search is up-to-date.
					SearchArray[SearchIdx].Blueprint->SearchGuid = FGuid::NewGuid();
					AddOrUpdateBlueprintSearchMetadata(SearchArray[SearchIdx].Blueprint);
				}
				else
				{
					RetrieveDDCData(SearchArray[SearchIdx], *SearchArray[SearchIdx].Blueprint->GetPathName());
				}

 				OutSearchData = SearchArray[SearchIdx++];
				return true;
			}
			else
			{
				RetrieveDDCData(SearchArray[SearchIdx], *SearchArray[SearchIdx].BlueprintPath);
				OutSearchData = SearchArray[SearchIdx++];
				return true;
			}

			++SearchIdx;
		}
	}

	{
		// Must lock this behind a critical section to ensure that no other thread is accessing it at the same time
		FScopeLock ScopeLock(&SafeQueryModifyCriticalSection);
		ActiveSearchQueries.Remove(InSearchOriginator);
	}
	ActiveSearchCounter.Decrement();

	return false;
}

void FFindInBlueprintSearchManager::EnsureSearchQueryEnds(const class FStreamSearch* InSearchOriginator)
{
	// Must lock this behind a critical section to ensure that no other thread is accessing it at the same time
	FScopeLock ScopeLock(&SafeQueryModifyCriticalSection);
	int32* SearchIdxPtr = ActiveSearchQueries.Find(InSearchOriginator);

	// If the search thread is still considered active, remove it
	if(SearchIdxPtr)
	{
		ActiveSearchQueries.Remove(InSearchOriginator);
		ActiveSearchCounter.Decrement();
	}
}

float FFindInBlueprintSearchManager::GetPercentComplete(const FStreamSearch* InSearchOriginator) const
{
	FScopeLock ScopeLock(&SafeQueryModifyCriticalSection);
	const int32* SearchIdxPtr = ActiveSearchQueries.Find(InSearchOriginator);

	float ReturnPercent = 0.0f;

	if(SearchIdxPtr)
	{
		ReturnPercent = (float)*SearchIdxPtr / (float)SearchArray.Num();
	}

	return ReturnPercent;
}

void FFindInBlueprintSearchManager::RetrieveDDCData(FSearchData& InOutSearchData, FName InBlueprintPath)
{
	FDerivedDataCacheInterface& DDC = GetDerivedDataCacheRef();

	// The data is valid, but we may still need to wait on the DDC.
	if(InOutSearchData.DDCRetrievalID != INDEX_NONE)
	{
		// Allow only one thread to query the DDC at a single time, this is important in the case that two threads are waiting for the same DDC information, once the information is pulled, it is deleted and is no longer accessible.
		FScopeLock ScopeLock(&SafeModifyCacheCriticalSection);

		// Check to see if the data was pulled by another thread while we waited
		if(InOutSearchData.DDCRetrievalID != INDEX_NONE)
		{
			// Wait for completion of the DDC retrieval, we are on a separate thread so this will not stall the editor
			DDC.WaitAsynchronousCompletion(InOutSearchData.DDCRetrievalID);

			TArray<uint8> DerivedData;

			if( !DDC.GetAsynchronousResults(InOutSearchData.DDCRetrievalID, DerivedData) ) 
			{
				// Since the asset cannot be loaded at this point, cache the Blueprint, we'll index it later.
				UncachedBlueprints.AddUnique(InBlueprintPath);
			}
			else
			{
				check(DerivedData.Num());

				FMemoryReader DDCAr(DerivedData, /*bIsPersistent=*/ true);
				DDCAr << InOutSearchData.BlueprintPath;
				DDCAr << InOutSearchData.Value;
				DDCAr.Close();
			}

			InOutSearchData.DDCRetrievalID = INDEX_NONE;
		}
	}
}

FString FFindInBlueprintSearchManager::QuerySingleBlueprint(UBlueprint* InBlueprint)
{
	// Update the Blueprint, make sure it is fully up-to-date
	AddOrUpdateBlueprintSearchMetadata(InBlueprint, true);

	FString Key = InBlueprint->GetOutermost()->GetPathName();

	int32* ArrayIdx = SearchMap.Find(Key);
	// This should always be true since we make sure to refresh the search data for this Blueprint when doing the search
	check(ArrayIdx && *ArrayIdx < SearchArray.Num());

	RetrieveDDCData(SearchArray[*ArrayIdx], *InBlueprint->GetPathName());

	return SearchArray[*ArrayIdx].Value;
}

void FFindInBlueprintSearchManager::PauseFindInBlueprintSearch()
{
	// Lock the critical section and flag that threads need to pause, they will pause when they can
	PauseThreadsCriticalSection.Lock();
	bIsPausing = true;

	// It is UNSAFE to lock any other critical section here, threads need them to finish a cycle of searching. Next cycle they will pause

	// Wait until all threads have come to a stop, it won't take long
	while(ActiveSearchCounter.GetValue() > 0)
	{
		FPlatformProcess::Sleep(0.1f);
	}
}

void FFindInBlueprintSearchManager::UnpauseFindInBlueprintSearch()
{
	// Before unpausing, we clean the cache of any excess data to keep it from bloating in size
	CleanCache();
	bIsPausing = false;

	// Release the threads to continue searching.
	PauseThreadsCriticalSection.Unlock();
}

void FFindInBlueprintSearchManager::CleanCache()
{
	// *NOTE* SaveCache is a thread safe operation by design, all searching threads are paused during the operation so there is no critical section locking

	// We need to cache where the active queries are so that we can put them back in a safe and expected position
	TMap< const FStreamSearch*, FString > CacheQueries;
	for( auto It = ActiveSearchQueries.CreateIterator() ; It ; ++It )
	{
	 	const FStreamSearch* ActiveSearch = It.Key();
	 	check(ActiveSearch);
	 	{
			FSearchData searchData;
	 		ContinueSearchQuery(ActiveSearch, searchData);

			// We will be looking the path up in the map, which uses the package path instead of the Blueprint, so rebuild the package path from the Blueprint path
			FString CachePath = FPaths::GetPath(searchData.BlueprintPath) / FPaths::GetBaseFilename(searchData.BlueprintPath);
	 		CacheQueries.Add(ActiveSearch, CachePath);
	 	}
	}

	FDerivedDataCacheInterface& DDC = GetDerivedDataCacheRef();

	TMap<FString, int> NewSearchMap;
	TArray<FSearchData> NewSearchArray;

	for(auto& SearchValuePair : SearchMap)
	{
		// Here it builds the new map/array, clean of deleted content.
		if( SearchArray[SearchValuePair.Value].bMarkedForDeletion || (SearchArray[SearchValuePair.Value].Blueprint && SearchArray[SearchValuePair.Value].Blueprint->HasAllFlags(RF_PendingKill)) )
		{
			// Item should not be kept, is is being destroyed and removed
			int x = 5;
			x = x;
		}
		else
		{
			// Build the new map/array
			NewSearchMap.Add(SearchValuePair.Key, NewSearchArray.Add(SearchArray[SearchValuePair.Value]) );
		}
	}

	SearchMap = NewSearchMap;
	SearchArray = NewSearchArray;

	// After the search, we have to place the active search queries where they belong
	for( auto& CacheQuery : CacheQueries )
	{
	 	int32 NewMappedIndex = 0;
	 	// If the CachePath has a length, it means it is valid, otherwise we are at the end and there are no more search results, leave the query there so it can handle shutdown on it's own
	 	if(CacheQuery.Value.Len() > 0)
	 	{
	 		int32* NewMappedIndexPtr = SearchMap.Find(CacheQuery.Value);
	 		check(NewMappedIndexPtr);
	 
	 		NewMappedIndex = *NewMappedIndexPtr;
	 	}
	 	else
	 	{
	 		NewMappedIndex = SearchArray.Num();
	 	}
	 
		// Update the active search to the new index of where it is at in the search
	 	*(ActiveSearchQueries.Find(CacheQuery.Key)) = NewMappedIndex;
	}
}

void FFindInBlueprintSearchManager::BuildCache()
{
	AssetRegistryModule = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	TArray< FAssetData > BlueprintAssets;
	AssetRegistryModule->Get().GetAssetsByClass(UBlueprint::StaticClass()->GetFName(), BlueprintAssets, true);
	
	FDerivedDataCacheInterface& DDC = GetDerivedDataCacheRef();
	for( FAssetData& Asset : BlueprintAssets )
	{
		if(FString* SearchGuid = Asset.TagsAndValues.Find("SearchGuid"))
		{
			OnAssetAdded(Asset);
		}
	}
}

void FFindInBlueprintSearchManager::CacheAllUncachedBlueprints(TWeakPtr< SFindInBlueprints > InSourceWidget)
{
	// Do not start another caching process if one is in progress
	if(!IsCacheInProgress())
	{
		// Multiple threads can be adding to this at the same time
		FScopeLock ScopeLock(&SafeModifyCacheCriticalSection);

		CachingObject = new FCacheAllBlueprintsTickableObject(UncachedBlueprints);
	}

	// Add the widget to the callback list, to be called when caching is complete
	OnCachingCompleteCallbackWidgets.Add(InSourceWidget);
}

void FFindInBlueprintSearchManager::CancelCacheAll()
{
	if(IsCacheInProgress())
	{
		CachingObject->OnCancelCaching();
	}
}

int32 FFindInBlueprintSearchManager::GetCurrentCacheIndex() const
{
	int32 CachingIndex = 0;
	if(CachingObject)
	{
		CachingIndex = CachingObject->GetCurrentCacheIndex();
	}

	return CachingIndex;
}

FString FFindInBlueprintSearchManager::GetCurrentCacheBlueprintName() const
{
	FString CachingBPName;
	if(CachingObject)
	{
		CachingBPName = CachingObject->GetCurrentCacheBlueprintName();
	}

	return CachingBPName;
}

float  FFindInBlueprintSearchManager::GetCacheProgress() const
{
	float ReturnCacheValue = 1.0f;

	if(CachingObject)
	{
		ReturnCacheValue = CachingObject->GetCacheProgress();
	}

	return ReturnCacheValue;
}

void FFindInBlueprintSearchManager::FinishedCachingBlueprints(int32 InNumberCached)
{
	// Multiple threads could be adding to this at the same time
	FScopeLock ScopeLock(&SafeModifyCacheCriticalSection);

	// Clean out the list by the count that were successfully cached, this allows us to continue the caching later.
	UncachedBlueprints.RemoveAt(0, InNumberCached);

	// Signal the callback, so all FindInBlueprints can resubmit their search queries
	for(TWeakPtr< SFindInBlueprints > FindInBlueprintsWidget : OnCachingCompleteCallbackWidgets)
	{
		if(FindInBlueprintsWidget.IsValid())
		{
			FindInBlueprintsWidget.Pin()->OnCacheComplete();
		}
	}
	OnCachingCompleteCallbackWidgets.Empty();

	// Delete the object and NULL it out so we can do it again in the future if needed (if it was canceled)
	delete CachingObject;
	CachingObject = NULL;
}

bool FFindInBlueprintSearchManager::IsCacheInProgress() const
{
	return CachingObject != NULL;
}

FText FFindInBlueprintSearchManager::ConvertHexStringToFText(FString InHexString)
{
	TArray<uint8> SerializedData;
	SerializedData.AddZeroed(InHexString.Len());

	HexToBytes(InHexString, SerializedData.GetData());

	FText ResultText;

	FMemoryReader Ar(SerializedData, /*bIsPersistent=*/ true);
	Ar << ResultText;
	Ar.Close();

	return ResultText;
}

FString FFindInBlueprintSearchManager::ConvertFTextToHexString(FText InValue)
{
	TArray<uint8> SerializedData;
	FMemoryWriter Ar(SerializedData, /*bIsPersistent=*/ true);

	Ar << InValue;
	Ar.Close();

	return BytesToHex(SerializedData.GetData(), SerializedData.Num());
}