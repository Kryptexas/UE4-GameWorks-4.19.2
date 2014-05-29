// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "ObjectEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "K2ActionMenuBuilder.h"
#include "EdGraphSchema_K2_Actions.h"
#include "K2Node.h"

//------------------------------------------------------------------------------
UDumpBlueprintsInfoCommandlet::UDumpBlueprintsInfoCommandlet(class FPostConstructInitializeProperties const& PCIP)
	: Super(PCIP)
{
}

//------------------------------------------------------------------------------
int32 UDumpBlueprintsInfoCommandlet::Main(FString const& Params)
{
	TArray<FString> Tokens, Switches;
	ParseCommandLine(*Params, Tokens, Switches);

	FString CommandletSaveDir = FPaths::GameSavedDir() + TEXT("Commandlets/");
	IFileManager::Get().MakeDirectory(*CommandletSaveDir);
 
	FString Filename = FString::Printf(TEXT("BlueprintsInfoDump_%s.json"), FPlatformTime::StrTimestamp());
	Filename = Filename.Replace(TEXT(" "), TEXT("_"));
	Filename = Filename.Replace(TEXT("/"), TEXT("-"));
	Filename = Filename.Replace(TEXT(":"), TEXT("."));
	Filename = FPaths::GetCleanFilename(Filename);
 
	FString FilePath = CommandletSaveDir / *Filename;
	FArchive* FileOut = IFileManager::Get().CreateFileWriter(*FilePath);
	if (FileOut != nullptr)
	{
		// Get class name from command line switches
		UClass* ClassFilter = nullptr;
		for (FString& Switch : Switches)
		{
			if (Switch.StartsWith("class="))
			{
				FString ClassSwitch, ClassName;
				Switch.Split(TEXT("="), &ClassSwitch, &ClassName);
				ClassFilter = FindObject<UClass>(ANY_PACKAGE, *ClassName);
			}
		}

		// Build class context container
		FString LeadingFields("{\n\t\"ClassContext\" : \"");
		if (ClassFilter != nullptr)
		{
			LeadingFields += ClassFilter->GetName();
		}
		else
		{
			LeadingFields += "<NO_CONTEXT>";
		}
		// Build palette contents header
		LeadingFields += "\",\n\t\"Palette\" : {";
		
		FileOut->Serialize(TCHAR_TO_ANSI(*LeadingFields), LeadingFields.Len());

		UPackage* TransientPackage = GetTransientPackage();
		FName TempBpName = MakeUniqueObjectName(TransientPackage, UBlueprint::StaticClass(), FName(TEXT("COMMANDLET_TEMP_Blueprint")));
		UBlueprint* TempBlueprint = FKismetEditorUtilities::CreateBlueprint(AActor::StaticClass(), TransientPackage, TempBpName, BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());

		FBlueprintPaletteListBuilder PaletteBuilder(TempBlueprint);
		UEdGraphSchema_K2 const* K2Schema = GetDefault<UEdGraphSchema_K2>();
		FK2ActionMenuBuilder(K2Schema).GetPaletteActions(PaletteBuilder, ClassFilter);

		for (int32 ActionIndex = 0; ActionIndex < PaletteBuilder.GetNumActions(); ++ActionIndex)
		{
			FGraphActionListBuilderBase::ActionGroup& Action = PaletteBuilder.GetAction(ActionIndex);
			if (Action.Actions.Num() <= 0)
			{
				continue;
			}

			// Get action name
			TSharedPtr<FEdGraphSchemaAction> PrimeAction = Action.Actions[0];
			FString ActionName = PrimeAction->MenuDescription.ToString();
			
			// Get action category info
			TArray<FString> MenuHierarchy;
			Action.GetCategoryChain(MenuHierarchy);
			bool bHasCategory = (MenuHierarchy.Num() > 0);
			FString ActionCategory = TEXT("");

			if (bHasCategory)
			{	
				for (FString const& SubCategory : MenuHierarchy)
				{
					ActionCategory += SubCategory + TEXT("|");
				}
			}


			// Build action key data
			FString ActionEntry("\n\t\t\"");
			ActionEntry += ActionCategory + ActionName + TEXT("\" : {");

			// Build action name data
			ActionEntry += TEXT("\n\t\t\t\"Name\" : \"");
			ActionEntry += ActionName;
			ActionEntry += TEXT("\"");

			// Build action category key data
			ActionEntry += TEXT(",\n\t\t\t\"Category\" : \"");
			if (bHasCategory)
			{
				// Build action category data from info
				ActionEntry += ActionCategory;
				ActionEntry.RemoveAt(ActionEntry.Len() - 1); // remove the last '|'
			}
			// End action category data
			ActionEntry += TEXT("\"");

			// Get action node type info
			UK2Node const* NodeTemplate = FK2SchemaActionUtils::ExtractNodeTemplateFromAction(PrimeAction);
			if (NodeTemplate != nullptr)
			{
				// Build action node type data
				ActionEntry += TEXT(",\n\t\t\t\"Node\" : \"");
				ActionEntry += NodeTemplate->GetClass()->GetPathName();
				ActionEntry += TEXT("\"");
			}
			// Finish action entry
			ActionEntry += TEXT("\n\t\t}");

			bool bIsLastEntry = (ActionIndex == PaletteBuilder.GetNumActions()-1);
			if (!bIsLastEntry)
			{
				// Add comma for next entry
				ActionEntry += TEXT(",");
			}
			// Write entry to file
			FileOut->Serialize(TCHAR_TO_ANSI(*ActionEntry), ActionEntry.Len());
		}
		// Close remaining entries and write to file
		FileOut->Serialize(TCHAR_TO_ANSI(TEXT("\n\t}\n}")), 5);
 
		FileOut->Close();
		delete FileOut;
		FileOut = nullptr;
	}
	return 0;
}