// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "UnrealEd.h"
#include "ObjectTools.h"
#include "Json.h"
#include "AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "AssetSelection.h"
#include "AssetEditorManager.h"
#include "ScopedTransaction.h"
#include "LevelEditor.h"
#include "ModuleManager.h"

#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "BlueprintGraphClasses.h"

#include "SlateWordWrapper.h"


DEFINE_LOG_CATEGORY_STATIC(LogEditorAutomationTests, Log, All);


/**
 * Wait for the given amount of time
 */
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaitLatentCommand, float, Duration);

bool FWaitLatentCommand::Update()
{
	float NewTime = FPlatformTime::Seconds();
	if (NewTime - StartTime >= Duration)
	{
		return true;
	}
	return false;
}

/**
 * Start PIE session
 */
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FStartPIECommand, bool, bSimulateInEditor);

bool FStartPIECommand::Update()
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::Get().GetModuleChecked<FLevelEditorModule>( TEXT("LevelEditor") );
	TSharedPtr<class ILevelViewport> ActiveLevelViewport = LevelEditorModule.GetFirstActiveViewport();

	GUnrealEd->RequestPlaySession( false, ActiveLevelViewport, bSimulateInEditor, NULL, NULL, -1, false );
	return true;
}

/**
 * End PlayMap session
 */
DEFINE_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand);

bool FEndPlayMapCommand::Update()
{
	GUnrealEd->RequestEndPlayMap();
	return true;
}

/**
 * Open editor for a particular asset
 */
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FOpenEditorForAssetCommand, FString, AssetName);

bool FOpenEditorForAssetCommand::Update()
{
	UObject* Object = StaticLoadObject(UObject::StaticClass(), NULL, *AssetName);
	if (Object)
	{
		FAssetEditorManager::Get().OpenEditorForAsset(Object);
		UE_LOG(LogEditorAutomationTests, Log, TEXT("Verified asset editor for: %s."), *AssetName);
	}
	else
	{
		UE_LOG(LogEditorAutomationTests, Error, TEXT("Failed to find object: %s."), *AssetName);
	}

	return true;
}
/**
 * Close all asset editors
 */
DEFINE_LATENT_AUTOMATION_COMMAND(FCloseAllAssetEditorsCommand);

bool FCloseAllAssetEditorsCommand::Update()
{
	FAssetEditorManager::Get().CloseAllAssetEditors();

	return true;
}

//////////////////////////////////////////////////////////////////////////
// FEditorAutomationTestUtilities

class FEditorAutomationTestUtilities
{
public:
	/**
	 * Loads the map specified by an automation test
	 * 
	 * @param MapName - Map to load
	 */
	static void LoadMap(const FString& MapName)
	{
		bool bLoadAsTemplate = false;
		bool bShowProgress = false;
		FEditorFileUtils::LoadMap(MapName, bLoadAsTemplate, bShowProgress);
	}

	/**
	 * Run PIE
	 */
	static void RunPIE()
	{
		bool bInSimulateInEditor = true;
		//once in the editor
		ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(true));
		ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));
		ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());

		//wait between tests
		ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));

		//once not in the editor
		ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
		ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));
		ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	}

	static bool CompileBlueprint(const FString& BlueprintName)
	{
		UBlueprint* BlueprintObj = Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), NULL, *BlueprintName));
		if (!BlueprintObj || !BlueprintObj->ParentClass )
		{
			UE_LOG(LogEditorAutomationTests, Error, TEXT("Failed to compile invalid blueprint, or blueprint parent no longer exists."));
			return false;
		}

		bool bIsRegeneratingOnLoad = false;
		bool bSkipGarbageCollection = true;
		FBlueprintEditorUtils::RefreshAllNodes(BlueprintObj);
		FKismetEditorUtilities::CompileBlueprint(BlueprintObj, bIsRegeneratingOnLoad, bSkipGarbageCollection);

		return true;
	}

	static void CollectTestsByClass(UClass * Class, TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands) 
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		TArray<FAssetData> ObjectList;
		AssetRegistryModule.Get().GetAssetsByClass(Class->GetFName(), ObjectList);

		for (auto ObjIter=ObjectList.CreateConstIterator(); ObjIter; ++ObjIter)
		{
			const FAssetData & Asset = *ObjIter;
			FString Filename = Asset.ObjectPath.ToString();
			//convert to full paths
			Filename = FPackageName::LongPackageNameToFilename(Filename);
			if (FAutomationTestFramework::GetInstance().ShouldTestContent(Filename))
			{
				FString BeautifiedFilename = Asset.AssetName.ToString();
				OutBeautifiedNames.Add(BeautifiedFilename);
				OutTestCommands.Add(Asset.ObjectPath.ToString());
			}
		}
	}
	
};

//////////////////////////////////////////////////////////////////////////

/**
 * FGenericImportAssetsAutomationTest
 * Simple unit test that attempts to import every file (except .txt files) within the unit test directory in a sub-folder
 * named "GenericImport." Used to test the basic codepath that would execute if a user imported a file using the interface
 * in the Content Browser (does not allow for specific settings to be made per import factory). Cannot be run in a commandlet
 * as it executes code that routes through Slate UI.
 */
IMPLEMENT_COMPLEX_AUTOMATION_TEST( FGenericImportAssetsAutomationTest, "Editor.Import", EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_NonNullRHI )

/** 
 * Requests a enumeration of all sample assets to import
 */
void FGenericImportAssetsAutomationTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands) const
{
	FString ImportTestDirectory;
	check( GConfig );
	GConfig->GetString( TEXT("AutomationTesting"), TEXT("ImportTestPath"), ImportTestDirectory, GEngineIni );


	// Find all files in the GenericImport directory
	TArray<FString> FilesInDirectory;
	IFileManager::Get().FindFilesRecursive(FilesInDirectory, *ImportTestDirectory, TEXT("*.*"), true, false);
	
	// Scan all the found files, ignoring .txt files which are likely P4 placeholders for creating directories
	for ( TArray<FString>::TConstIterator FileIter( FilesInDirectory ); FileIter; ++FileIter )
	{
		FString Filename( *FileIter );
		FString Ext = FPaths::GetExtension(Filename, true);
		if ( Ext != TEXT(".txt") && !FPackageName::IsPackageExtension(*Ext) )
		{
			FString FileString = *FileIter;
			OutBeautifiedNames.Add(FPaths::GetBaseFilename(Filename));
			OutTestCommands.Add(FileString);
		}
	}
}

/** 
 * Execute the generic import test
 *
 * @param Parameters - Should specify the asset to import
 * @return	TRUE if the test was successful, FALSE otherwise
 */
bool FGenericImportAssetsAutomationTest::RunTest(const FString& Parameters)
{
	int32 CurErrorIndex = 0, CurWarningIndex = 0, CurLogItemIndex = 0;

	TArray<FString> CurFileToImport;
	CurFileToImport.Add( *Parameters );
	FString CleanFilename = FPaths::GetCleanFilename(CurFileToImport[0]);

	FString PackagePath;
	check( GConfig );
	GConfig->GetString( TEXT("AutomationTesting"), TEXT("ImportTestPackagePath"), PackagePath, GEngineIni );

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	TArray<UObject*> ImportedObjects = AssetToolsModule.Get().ImportAssets(CurFileToImport, PackagePath);
	const bool CurTestSuccessful = ImportedObjects.Num() == 1;

	// Any errors, warnings, or log items that are caught during this unit test aren't guaranteed to include the name of the file that generated them,
	// which can be confusing when reading results. Alleviate the issue by injecting the file name for each error, warning, or log item, where appropriate.
	for ( int32 ErrorStartIndex = CurErrorIndex; ErrorStartIndex < ExecutionInfo.Errors.Num(); ++ErrorStartIndex )
	{
		ExecutionInfo.Errors[ErrorStartIndex] = FString::Printf( TEXT("%s: %s"), *CleanFilename, *ExecutionInfo.Errors[ErrorStartIndex] );
	}
	for ( int32 WarningStartIndex = CurWarningIndex; WarningStartIndex < ExecutionInfo.Warnings.Num(); ++WarningStartIndex )
	{
		ExecutionInfo.Warnings[WarningStartIndex] = FString::Printf( TEXT("%s: %s"), *CleanFilename, *ExecutionInfo.Warnings[WarningStartIndex] );
	}
	for ( int32 LogItemStartIndex = CurLogItemIndex; LogItemStartIndex < ExecutionInfo.LogItems.Num(); ++LogItemStartIndex )
	{
		ExecutionInfo.LogItems[LogItemStartIndex] = FString::Printf( TEXT("%s: %s"), *CleanFilename, *ExecutionInfo.LogItems[LogItemStartIndex] );
	}
	CurErrorIndex = ExecutionInfo.Errors.Num();
	CurWarningIndex = ExecutionInfo.Warnings.Num();
	CurLogItemIndex = ExecutionInfo.LogItems.Num();

	return CurTestSuccessful;
}

//////////////////////////////////////////////////////////////////////////

/**
 * FTextWrappingAutomationTest
 * Simple unit test that wraps text using FSlateFontInfo::WrapTextToClippingWidth(). Cannot be run in a commandlet
 * as it executes code that routes through Slate.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST( FTextWrappingAutomationTest, "Engine.Rendering.Slate.Text Wrapping", EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_NonNullRHI )

/** 
 * Execute the text wrapping test
 *
 * @return	true if the test was successful, false otherwise
 */
bool FTextWrappingAutomationTest::RunTest(const FString& Parameters)
{
	struct FTestStruct
	{
		static bool TestMethod(const FSlateFontInfo& FontInfo, const float WrapWidth, const int32 ArgumentCount, ...)
		{
			va_list Arguments;
			va_start(Arguments, ArgumentCount);

			FString TestString;
			TArray<FString> CorrectLines;
			for(int32 i = 0; i < ArgumentCount; ++i)
			{
				const TCHAR* const Fragment = va_arg(Arguments, const TCHAR* const);
				CorrectLines.Add(Fragment);
				TestString += Fragment;
			}

			va_end(Arguments);

			FString Result = SlateWordWrapper::WrapText(TestString, FontInfo, WrapWidth, 1.0f);
			TArray<FString> ResultLines;
			Result.ParseIntoArray(&ResultLines, TEXT("\n"), false);

			return ResultLines == CorrectLines;
		}
	};

	FSlateFontInfo FontInfo( TEXT("DroidSansMono"), 10 );

	const TSharedRef< FSlateFontMeasure > FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const float GlyphWidth = FontMeasureService->Measure( TEXT(" "), FontInfo).X;

#define TEST(WrapWidth, ArgumentCount, ...)\
	{\
		if( !FTestStruct::TestMethod(FontInfo, (WrapWidth), (ArgumentCount), __VA_ARGS__) )\
		{\
			AddError( TEXT("Testing string wrap failed, did not break line where expected.") );\
		}\
	}

	TEST( GlyphWidth * 4, 3,	TEXT("AAA "),
								TEXT("BBB "),
								TEXT("CCC") );

	TEST( GlyphWidth * 3, 5,	TEXT("AAA"),
								TEXT(" "),
								TEXT("BBB"),
								TEXT(" "),
								TEXT("CCC") );

	TEST( GlyphWidth * 2, 6,	TEXT("AA"),
								TEXT("A "),
								TEXT("BB"),
								TEXT("B "),
								TEXT("CC"),
								TEXT("C") );

	TEST( GlyphWidth * 1, 11,	TEXT("A"),
								TEXT("A"),
								TEXT("A"),
								TEXT(" "),
								TEXT("B"),
								TEXT("B"),
								TEXT("B"),
								TEXT(" "),
								TEXT("C"),
								TEXT("C"),
								TEXT("C") );

#undef TEST

	return true;
}

//////////////////////////////////////////////////////////////////////////

/**
 * FJsonAutomationTest
 * Simple unit test that runs Json's in-built test cases
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST( FJsonAutomationTest, "Engine.FileSystem.JSON", EAutomationTestFlags::ATF_SmokeTest)

typedef TJsonWriterFactory< TCHAR, TCondensedJsonPrintPolicy<TCHAR> > FCondensedJsonStringWriterFactory;
typedef TJsonWriter< TCHAR, TCondensedJsonPrintPolicy<TCHAR> > FCondensedJsonStringWriter;

typedef TJsonWriterFactory< TCHAR, TPrettyJsonPrintPolicy<TCHAR> > FPrettyJsonStringWriterFactory;
typedef TJsonWriter< TCHAR, TPrettyJsonPrintPolicy<TCHAR> > FPrettyJsonStringWriter;

/** 
 * Execute the Json test cases
 *
 * @return	true if the test was successful, false otherwise
 */
bool FJsonAutomationTest::RunTest(const FString& Parameters)
{
	// Null Case
	{
		const FString InputString = TEXT("");
		TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create( InputString );

		TSharedPtr<FJsonObject> Object;
		check( FJsonSerializer::Deserialize( Reader, Object ) == false );
		check( !Object.IsValid() );
	}

	// Empty Object Case
	{
		const FString InputString = TEXT("{}");
		TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create( InputString );

		TSharedPtr<FJsonObject> Object;
		check( FJsonSerializer::Deserialize( Reader, Object ) );
		check( Object.IsValid() );

		FString OutputString;
		TSharedRef< FCondensedJsonStringWriter > Writer = FCondensedJsonStringWriterFactory::Create( &OutputString );
		check( FJsonSerializer::Serialize( Object.ToSharedRef(), Writer ) );
		check( InputString == OutputString );
	}

	// Empty Array Case
	{
		const FString InputString = TEXT("[]");
		TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create( InputString );

		TArray< TSharedPtr<FJsonValue> > Array;
		check( FJsonSerializer::Deserialize( Reader, Array ) );
		check( Array.Num() == 0 );

		FString OutputString;
		TSharedRef< FCondensedJsonStringWriter > Writer = FCondensedJsonStringWriterFactory::Create( &OutputString );
		check( FJsonSerializer::Serialize( Array, Writer ) );
		check( InputString == OutputString );
	}

	// Simple Array Case
	{
		const FString InputString = 
			TEXT("[")
			TEXT(	"{")
			TEXT(		"\"Value\":\"Some String\"")
			TEXT(	"}")
			TEXT("]");

		TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create( InputString );

		TArray< TSharedPtr<FJsonValue> > Array;
		check( FJsonSerializer::Deserialize( Reader, Array ) );
		check( Array.Num() == 1 );
		check( Array[0].IsValid() )

		TSharedPtr< FJsonObject > Object = Array[0]->AsObject();
		check( Object.IsValid() );
		check( Object->GetStringField( TEXT("Value") ) == TEXT("Some String") )

		FString OutputString;
		TSharedRef< FCondensedJsonStringWriter > Writer = FCondensedJsonStringWriterFactory::Create( &OutputString );
		check( FJsonSerializer::Serialize( Array, Writer ) );
		check( InputString == OutputString );
	}

	// String Test
	{
		const FString InputString =
			TEXT("{")
			TEXT(	"\"Value\":\"Some String, Escape Chars: \\\\, \\\", \\/, \\b, \\f, \\n, \\r, \\t, \\u002B\"")
			TEXT("}");
		TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create( InputString );

		TSharedPtr<FJsonObject> Object;
		check( FJsonSerializer::Deserialize( Reader, Object ) );
		check( Object.IsValid() );

		const TSharedPtr<FJsonValue>* Value = Object->Values.Find(TEXT("Value"));
		check(Value && (*Value)->Type == EJson::String);
		const FString String = (*Value)->AsString();
		check(String == TEXT("Some String, Escape Chars: \\, \", /, \b, \f, \n, \r, \t, +"));

		FString OutputString;
		TSharedRef< FCondensedJsonStringWriter > Writer = FCondensedJsonStringWriterFactory::Create( &OutputString );
		check( FJsonSerializer::Serialize( Object.ToSharedRef(), Writer ) );

		const FString TestOutput =
			TEXT("{")
			TEXT(	"\"Value\":\"Some String, Escape Chars: \\\\, \\\", /, \\b, \\f, \\n, \\r, \\t, +\"")
			TEXT("}");
		check(OutputString == TestOutput);
	}

	// Number Test
	{
		const FString InputString =
			TEXT("{")
			TEXT(	"\"Value1\":2.544e+15,")
			TEXT(	"\"Value2\":-0.544E-2,")
			TEXT(	"\"Value3\":251e3,")
			TEXT(	"\"Value4\":-0.0,")
			TEXT(	"\"Value5\":843")
			TEXT("}");
		TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create( InputString );

		TSharedPtr<FJsonObject> Object;
		check( FJsonSerializer::Deserialize( Reader, Object ) );
		check( Object.IsValid() );

		float TestValues[] = {2.544e+15f, -0.544e-2f, 251e3, -0.0, 843};
		for (int32 i = 0; i < 5; ++i)
		{
			const TSharedPtr<FJsonValue>* Value = Object->Values.Find(FString::Printf(TEXT("Value%i"), i + 1));
			check(Value && (*Value)->Type == EJson::Number);
			const float Float = (*Value)->AsNumber();
			check(FMath::Abs(Float - TestValues[i]) < KINDA_SMALL_NUMBER);
		}

		FString OutputString;
		TSharedRef< FCondensedJsonStringWriter > Writer = FCondensedJsonStringWriterFactory::Create( &OutputString );
		check( FJsonSerializer::Serialize( Object.ToSharedRef(), Writer ) );

		// %g isn't standardized, so we use the same %g format that is used inside PrintJson instead of hardcoding the values here
		const FString TestOutput = FString::Printf(
			TEXT("{")
			TEXT(	"\"Value1\":%g,")
			TEXT(	"\"Value2\":%g,")
			TEXT(	"\"Value3\":%g,")
			TEXT(	"\"Value4\":%g,")
			TEXT(	"\"Value5\":%g")
			TEXT("}"),
			TestValues[0], TestValues[1], TestValues[2], TestValues[3], TestValues[4]);
		check(OutputString == TestOutput);
	}

	// Boolean/Null Test
	{
		const FString InputString =
			TEXT("{")
			TEXT(	"\"Value1\":true,")
			TEXT(	"\"Value2\":true,")
			TEXT(	"\"Value3\":faLsE,")
			TEXT(	"\"Value4\":null,")
			TEXT(	"\"Value5\":NULL")
			TEXT("}");
		TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create( InputString );

		TSharedPtr<FJsonObject> Object;
		check( FJsonSerializer::Deserialize( Reader, Object ) );
		check( Object.IsValid() );

		bool TestValues[] = {true, true, false};
		for (int32 i = 0; i < 5; ++i)
		{
			const TSharedPtr<FJsonValue>* Value = Object->Values.Find(FString::Printf(TEXT("Value%i"), i + 1));
			check(Value);
			if (i < 3)
			{
				check((*Value)->Type == EJson::Boolean);
				const bool Bool = (*Value)->AsBool();
				check(Bool == TestValues[i]);
			}
			else
			{
				check((*Value)->Type == EJson::Null);
				check((*Value)->IsNull());
			}
		}

		FString OutputString;
		TSharedRef< FCondensedJsonStringWriter > Writer = FCondensedJsonStringWriterFactory::Create( &OutputString );
		check( FJsonSerializer::Serialize( Object.ToSharedRef(), Writer ) );

		const FString TestOutput =
			TEXT("{")
			TEXT(	"\"Value1\":true,")
			TEXT(	"\"Value2\":true,")
			TEXT(	"\"Value3\":false,")
			TEXT(	"\"Value4\":null,")
			TEXT(	"\"Value5\":null")
			TEXT("}");
		check(OutputString == TestOutput);
	}

	// Object Test && extra whitespace test
	{
		const FString InputStringWithExtraWhitespace =
			TEXT("		\n\r\n	   {")
			TEXT(	"\"Object\":")
			TEXT(	"{")
			TEXT(		"\"NestedValue\":null,")
			TEXT(		"\"NestedObject\":{}")
			TEXT(	"},")
			TEXT(	"\"Value\":true")
			TEXT("}		\n\r\n	   ");

		const FString InputString =
			TEXT("{")
			TEXT(	"\"Object\":")
			TEXT(	"{")
			TEXT(		"\"NestedValue\":null,")
			TEXT(		"\"NestedObject\":{}")
			TEXT(	"},")
			TEXT(	"\"Value\":true")
			TEXT("}");

		TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create( InputStringWithExtraWhitespace );

		TSharedPtr<FJsonObject> Object;
		check( FJsonSerializer::Deserialize( Reader, Object ) );
		check( Object.IsValid() );

		const TSharedPtr<FJsonValue>* InnerValueFail = Object->Values.Find(TEXT("InnerValue"));
		check(!InnerValueFail);

		const TSharedPtr<FJsonValue>* ObjectValue = Object->Values.Find(TEXT("Object"));
		check(ObjectValue && (*ObjectValue)->Type == EJson::Object);
		const TSharedPtr<FJsonObject> InnerObject = (*ObjectValue)->AsObject();
		check(InnerObject.IsValid());

		{
			const TSharedPtr<FJsonValue>* NestedValueValue = InnerObject->Values.Find(TEXT("NestedValue"));
			check(NestedValueValue && (*NestedValueValue)->Type == EJson::Null);
			check((*NestedValueValue)->IsNull());

			const TSharedPtr<FJsonValue>* NestedObjectValue = InnerObject->Values.Find(TEXT("NestedObject"));
			check(NestedObjectValue && (*NestedObjectValue)->Type == EJson::Object);
			const TSharedPtr<FJsonObject> InnerInnerObject = (*NestedObjectValue)->AsObject();
			check(InnerInnerObject.IsValid());

			{
				const TSharedPtr<FJsonValue>* NestedValueValueFail = InnerInnerObject->Values.Find(TEXT("NestedValue"));
				check(!NestedValueValueFail);
			}
		}

		const TSharedPtr<FJsonValue>* ValueValue = Object->Values.Find(TEXT("Value"));
		check(ValueValue && (*ValueValue)->Type == EJson::Boolean);
		const bool Bool = (*ValueValue)->AsBool();
		check(Bool);

		FString OutputString;
		TSharedRef< FCondensedJsonStringWriter > Writer = FCondensedJsonStringWriterFactory::Create( &OutputString );
		check( FJsonSerializer::Serialize( Object.ToSharedRef(), Writer ) );
		check(OutputString == InputString);
	}

	// Array Test
	{
		const FString InputString =
			TEXT("{")
			TEXT(	"\"Array\":")
			TEXT(	"[")
			TEXT(		"[],")
			TEXT(		"\"Some String\",")
			TEXT(		"\"Another String\",")
			TEXT(		"null,")
			TEXT(		"true,")
			TEXT(		"false,")
			TEXT(		"45,")
			TEXT(		"{}")
			TEXT(	"]")
			TEXT("}");
		TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create( InputString );

		TSharedPtr<FJsonObject> Object;
		check( FJsonSerializer::Deserialize( Reader, Object ) );
		check( Object.IsValid() );

		const TSharedPtr<FJsonValue>* InnerValueFail = Object->Values.Find(TEXT("InnerValue"));
		check(!InnerValueFail);

		const TSharedPtr<FJsonValue>* ArrayValue = Object->Values.Find(TEXT("Array"));
		check(ArrayValue && (*ArrayValue)->Type == EJson::Array);
		const TArray< TSharedPtr<FJsonValue> > Array = (*ArrayValue)->AsArray();
		check(Array.Num() == 8);

		EJson::Type ValueTypes[] = {EJson::Array, EJson::String, EJson::String, EJson::Null,
			EJson::Boolean, EJson::Boolean, EJson::Number, EJson::Object};
		for (int32 i = 0; i < Array.Num(); ++i)
		{
			const TSharedPtr<FJsonValue>& Value = Array[i];
			check(Value.IsValid());
			check(Value->Type == ValueTypes[i]);
		}

		const TArray< TSharedPtr<FJsonValue> >& InnerArray = Array[0]->AsArray();
		check(InnerArray.Num() == 0);
		check(Array[1]->AsString() == TEXT("Some String"));
		check(Array[2]->AsString() == TEXT("Another String"));
		check(Array[3]->IsNull());
		check(Array[4]->AsBool());
		check(!Array[5]->AsBool());
		check(FMath::Abs(Array[6]->AsNumber() - 45.f) < KINDA_SMALL_NUMBER);
		const TSharedPtr<FJsonObject> InnerObject = Array[7]->AsObject();
		check(InnerObject.IsValid());

		FString OutputString;
		TSharedRef< FCondensedJsonStringWriter > Writer = FCondensedJsonStringWriterFactory::Create( &OutputString );
		check( FJsonSerializer::Serialize( Object.ToSharedRef(), Writer ) );
		check(OutputString == InputString);
	}

	// Pretty Print Test
	{
		const FString InputString =
			TEXT("{")									LINE_TERMINATOR
			TEXT("	\"Data1\": \"value\",")				LINE_TERMINATOR
			TEXT("	\"Data2\": \"value\",")				LINE_TERMINATOR
			TEXT("	\"Array\": [")						LINE_TERMINATOR
			TEXT("		{")								LINE_TERMINATOR
			TEXT("			\"InnerData1\": \"value\"")	LINE_TERMINATOR
			TEXT("		},")							LINE_TERMINATOR
			TEXT("		[],")							LINE_TERMINATOR
			TEXT("		[ 1, 2, 3, 4 ],")				LINE_TERMINATOR
			TEXT("		{")								LINE_TERMINATOR
			TEXT("		},")							LINE_TERMINATOR
			TEXT("		\"value\",")					LINE_TERMINATOR
			TEXT("		\"value\"")						LINE_TERMINATOR
			TEXT("	],")								LINE_TERMINATOR
			TEXT("	\"Object\":")						LINE_TERMINATOR
			TEXT("	{")									LINE_TERMINATOR
			TEXT("	}")									LINE_TERMINATOR
			TEXT("}");
		TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create( InputString );

		TSharedPtr<FJsonObject> Object;
		check( FJsonSerializer::Deserialize( Reader, Object ) );
		check( Object.IsValid() );

		FString OutputString;
		TSharedRef< FPrettyJsonStringWriter > Writer = FPrettyJsonStringWriterFactory::Create( &OutputString );
		check( FJsonSerializer::Serialize( Object.ToSharedRef(), Writer ) );
		check(OutputString == InputString);
	}
	  
	// Line and Character # test
	{
		const FString InputString =
			TEXT("{")									LINE_TERMINATOR
			TEXT("	\"Data1\": \"value\",")				LINE_TERMINATOR
			TEXT("	\"Array\":")						LINE_TERMINATOR
			TEXT("	[")									LINE_TERMINATOR
			TEXT("		12345,")						LINE_TERMINATOR
			TEXT("		True")							LINE_TERMINATOR
			TEXT("	],")								LINE_TERMINATOR
			TEXT("	\"Object\":")						LINE_TERMINATOR
			TEXT("	{")									LINE_TERMINATOR
			TEXT("	}")									LINE_TERMINATOR
			TEXT("}");
		TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create( InputString );

		EJsonNotation::Type Notation = EJsonNotation::Null;
		check( Reader->ReadNext( Notation ) && Notation == EJsonNotation::ObjectStart );
		check( Reader->GetLineNumber() == 1 && Reader->GetCharacterNumber() == 1 );

		check( Reader->ReadNext( Notation ) && Notation == EJsonNotation::String );
		check( Reader->GetLineNumber() == 2 && Reader->GetCharacterNumber() == 17 );

		check( Reader->ReadNext( Notation ) && Notation == EJsonNotation::ArrayStart );
		check( Reader->GetLineNumber() == 4 && Reader->GetCharacterNumber() == 2 );

		check( Reader->ReadNext( Notation ) && Notation == EJsonNotation::Number );
		check( Reader->GetLineNumber() == 5 && Reader->GetCharacterNumber() == 7 );

		check( Reader->ReadNext( Notation ) && Notation == EJsonNotation::Boolean );
		check( Reader->GetLineNumber() == 6 && Reader->GetCharacterNumber() == 6 );
	}

	// Failure Cases
	TArray<FString> FailureInputs;

	// Unclosed Object
	FailureInputs.Add(
		TEXT("{"));

	// Values in Object without identifiers
	FailureInputs.Add(
		TEXT("{")
		TEXT(	"\"Value1\",")
		TEXT(	"\"Value2\",")
		TEXT(	"43")
		TEXT("}"));

	// Unexpected End Of Input Found
	FailureInputs.Add(
		TEXT("{")
		TEXT(	"\"Object\":")
		TEXT(	"{")
		TEXT(		"\"NestedValue\":null,"));

	// Missing first brace
	FailureInputs.Add(
		TEXT(	"\"Object\":")
		TEXT(		"{")
		TEXT(		"\"NestedValue\":null,")
		TEXT(		"\"NestedObject\":{}")
		TEXT(	"},")
		TEXT(	"\"Value\":true")
		TEXT("}"));

	// Missing last character
	FailureInputs.Add(
		TEXT("{")
		TEXT(	"\"Object\":")
		TEXT(	"{")
		TEXT(		"\"NestedValue\":null,")
		TEXT(		"\"NestedObject\":{}")
		TEXT(	"},")
		TEXT(	"\"Value\":true"));

	// Extra last character
	FailureInputs.Add(
		TEXT("{")
		TEXT(	"\"Object\":")
		TEXT(	"{")
		TEXT(		"\"NestedValue\":null,")
		TEXT(		"\"NestedObject\":{}")
		TEXT(	"},")
		TEXT(	"\"Value\":true")
		TEXT("}0"));

	// Missing comma
	FailureInputs.Add(
		TEXT("{")
		TEXT(	"\"Value1\":null,")
		TEXT(	"\"Value2\":\"string\"")
		TEXT(	"\"Value3\":65.3")
		TEXT("}"));

	// Extra comma
	FailureInputs.Add(
		TEXT("{")
		TEXT(	"\"Value1\":null,")
		TEXT(	"\"Value2\":\"string\",")
		TEXT(	"\"Value3\":65.3,")
		TEXT("}"));

	// Badly formed true/false/null
	FailureInputs.Add(TEXT("{\"Value\":tru}"));
	FailureInputs.Add(TEXT("{\"Value\":full}"));
	FailureInputs.Add(TEXT("{\"Value\":nulle}"));
	FailureInputs.Add(TEXT("{\"Value\":n%ll}"));

	// Floating Point Failures
	FailureInputs.Add(TEXT("{\"Value\":65.3e}"));
	FailureInputs.Add(TEXT("{\"Value\":65.}"));
	FailureInputs.Add(TEXT("{\"Value\":.7}"));
	FailureInputs.Add(TEXT("{\"Value\":+6}"));
	FailureInputs.Add(TEXT("{\"Value\":01}"));
	FailureInputs.Add(TEXT("{\"Value\":00.56}"));
	FailureInputs.Add(TEXT("{\"Value\":-1.e+4}"));
	FailureInputs.Add(TEXT("{\"Value\":2e+}"));

	// Bad Escape Characters
	FailureInputs.Add(TEXT("{\"Value\":\"Hello\\xThere\"}"));
	FailureInputs.Add(TEXT("{\"Value\":\"Hello\\u123There\"}"));
	FailureInputs.Add(TEXT("{\"Value\":\"Hello\\RThere\"}"));

	for (int32 i = 0; i < FailureInputs.Num(); ++i)
	{
		TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create( FailureInputs[i] );

		TSharedPtr<FJsonObject> Object;
		check( FJsonSerializer::Deserialize( Reader, Object ) == false );
		check( !Object.IsValid() );
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

/**
 * Pie Test
 * Verification PIE works
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST( FPIETest, "Maps.PIE", EAutomationTestFlags::ATF_Editor )

/** 
 * Execute the loading of one map to verify PIE works
 *
 * @param Parameters - Unused for this test
 * @return	TRUE if the test was successful, FALSE otherwise
 */
bool FPIETest::RunTest(const FString& Parameters)
{
	UAutomationTestSettings const* AutomationTestSettings = GetDefault<UAutomationTestSettings>();
	check(AutomationTestSettings);

	FString MapName = AutomationTestSettings->AutomationTestmap.FilePath;

	FEditorAutomationTestUtilities::LoadMap(MapName);
	FEditorAutomationTestUtilities::RunPIE();

	return true;
}

/**
 * LoadAllMaps
 * Verification automation test to make sure loading all maps succeed without crashing
 */
IMPLEMENT_COMPLEX_AUTOMATION_TEST( FLoadAllMapsInEditorTest, "Maps.Load All In Editor", EAutomationTestFlags::ATF_Editor )

/** 
 * Requests a enumeration of all maps to be loaded
 */
void FLoadAllMapsInEditorTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands) const
{
	TArray<FString> FileList;
	FEditorFileUtils::FindAllPackageFiles(FileList);

	// Iterate over all files, adding the ones with the map extension..
	for( int32 FileIndex=0; FileIndex< FileList.Num(); FileIndex++ )
	{
		const FString& Filename = FileList[FileIndex];

		// Disregard filenames that don't have the map extension if we're in MAPSONLY mode.
		if ( FPaths::GetExtension(Filename, true) == FPackageName::GetMapPackageExtension()) 
		{
			if (FAutomationTestFramework::GetInstance().ShouldTestContent(Filename))
			{
				OutBeautifiedNames.Add(FPaths::GetBaseFilename(Filename));
				OutTestCommands.Add(Filename);
			}
		}
	}
}


/** 
 * Execute the loading of each map
 *
 * @param Parameters - Should specify which map name to load
 * @return	TRUE if the test was successful, FALSE otherwise
 */
bool FLoadAllMapsInEditorTest::RunTest(const FString& Parameters)
{
	FString MapName = Parameters;

	FEditorAutomationTestUtilities::LoadMap(MapName);

	return true;
}


//////////////////////////////////////////////////////////////////////////


/**
 * LoadAllBlueprints
 * Verification unit test to make sure loading all blueprints works
 */
IMPLEMENT_COMPLEX_AUTOMATION_TEST( FLoadAllBlueprintsAutomationTest, "Blueprints.Load Blueprints", EAutomationTestFlags::ATF_Editor)

/** 
 * Requests a enumeration of all blueprints to be loaded
 */
void FLoadAllBlueprintsAutomationTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands) const
{
	FEditorAutomationTestUtilities::CollectTestsByClass(UBlueprint::StaticClass(), OutBeautifiedNames, OutTestCommands);
}


/** 
 * Execute the loading of each blueprint
 *
 * @param Parameters - Should specify which blueprint name to load
 * @return	TRUE if the test was successful, FALSE otherwise
 */
bool FLoadAllBlueprintsAutomationTest::RunTest(const FString& Parameters)
{
	UE_LOG(LogEditorAutomationTests, Log, TEXT("Beginning compile test for %s"), *Parameters);
	bool bSuccess = FEditorAutomationTestUtilities::CompileBlueprint(Parameters);

	return bSuccess;
}


//////////////////////////////////////////////////////////////////////////
/**
 * LoadAllAnimBlueprints
 * Verification unit test to make sure loading all anim blueprints works
 */
IMPLEMENT_COMPLEX_AUTOMATION_TEST( FLoadAllAnimBlueprintsAutomationTest, "Blueprints.Load Anims", EAutomationTestFlags::ATF_Editor)

/** 
 * Requests a enumeration of all blueprints to be loaded
 */
void FLoadAllAnimBlueprintsAutomationTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands) const
{
	FEditorAutomationTestUtilities::CollectTestsByClass(UAnimBlueprint::StaticClass(), OutBeautifiedNames, OutTestCommands);
}

/** 
 * Execute the loading of each blueprint
 *
 * @param Parameters - Should specify which blueprint name to load
 * @return	TRUE if the test was successful, FALSE otherwise
 */
bool FLoadAllAnimBlueprintsAutomationTest::RunTest(const FString& Parameters)
{
	return FEditorAutomationTestUtilities::CompileBlueprint(Parameters);
}

//////////////////////////////////////////////////////////////////////////

/**
 * Reinitialize all RHI resources
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReinitializeRHIResources, "Engine.Rendering.Reinit Resources", EAutomationTestFlags::ATF_Editor);

bool FReinitializeRHIResources::RunTest(const FString& Parameters)
{
	GEditor->Exec( NULL, TEXT("ReinitRHIResources"));
	return true;
}


//////////////////////////////////////////////////////////////////////////
/**
 * QA BSP Regression Testing
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBSPValidation, "QA.BSP Validation", EAutomationTestFlags::ATF_Editor);

bool FBSPValidation::RunTest(const FString& Parameters)
{
	UWorld* World = GEditor->NewMap();
	GEditor->Exec( World, TEXT("BRUSH Scale 1 1 1"));

	for( int32 i = 0; i < GEditor->LevelViewportClients.Num(); i++ )
	{
		FLevelEditorViewportClient* ViewportClient = GEditor->LevelViewportClients[i];
		if(!ViewportClient->IsOrtho() )
		{
			ViewportClient->SetViewLocation( FVector(176, 2625, 2075) );
			ViewportClient->SetViewRotation( FRotator(319, 269, 1) );
		}
	}

	//Cube Additive Brush
	UCubeBuilder* CubeAdditiveBrushBuilder = Cast<UCubeBuilder>(GEditor->FindBrushBuilder( UCubeBuilder::StaticClass() ));
	CubeAdditiveBrushBuilder->X = 4096.0f;
	CubeAdditiveBrushBuilder->Y = 4096.0f;
	CubeAdditiveBrushBuilder->Z = 128.0f;
	CubeAdditiveBrushBuilder->Build(World);
	GEditor->Exec( World, TEXT("BRUSH MOVETO X=0 Y=0 Z=0"));
	GEditor->Exec( World, TEXT("BRUSH ADD"));

	//Cone Additive Brush
	UConeBuilder* ConeAdditiveBrushBuilder = Cast<UConeBuilder>(GEditor->FindBrushBuilder( UConeBuilder::StaticClass() ));
	ConeAdditiveBrushBuilder->Z = 1024.0f;
	ConeAdditiveBrushBuilder->CapZ = 256.0f;
	ConeAdditiveBrushBuilder->OuterRadius = 512.0f;
	ConeAdditiveBrushBuilder->InnerRadius = 384;
	ConeAdditiveBrushBuilder->Sides = 32;
	ConeAdditiveBrushBuilder->Build(World);
	GEditor->Exec( World, TEXT("BRUSH MOVETO X=-1525 Y=-1777 Z=64"));
	GEditor->Exec( World, TEXT("BRUSH ADD"));

	//Sphere Additive Brush
	UTetrahedronBuilder* TetraAdditiveBrushBuilder = Cast<UTetrahedronBuilder>(GEditor->FindBrushBuilder( UTetrahedronBuilder::StaticClass() ));
	TetraAdditiveBrushBuilder->Radius = 512.0f;
	TetraAdditiveBrushBuilder->SphereExtrapolation = 3;
	TetraAdditiveBrushBuilder->Build(World);
	GEditor->Exec( World, TEXT("BRUSH MOVETO X=-88 Y=-1777 Z=535"));
	GEditor->Exec( World, TEXT("BRUSH ADD"));

	//Cylinder Additive Brush
	UCylinderBuilder* CylinderAdditiveBrushBuilder = Cast<UCylinderBuilder>(GEditor->FindBrushBuilder( UCylinderBuilder::StaticClass() ));
	CylinderAdditiveBrushBuilder->Z = 1024.0f;
	CylinderAdditiveBrushBuilder->OuterRadius = 512.0f;
	CylinderAdditiveBrushBuilder->InnerRadius = 384.0f;
	CylinderAdditiveBrushBuilder->Sides = 16;
	CylinderAdditiveBrushBuilder->Build(World);
	GEditor->Exec( World, TEXT("BRUSH MOVETO X=1338 Y=-1776 Z=535"));
	GEditor->Exec( World, TEXT("BRUSH ADD"));

	//Cylinder Additive Brush
	USheetBuilder* SheetAdditiveBrushBuilder = Cast<USheetBuilder>(GEditor->FindBrushBuilder( USheetBuilder::StaticClass() ));
	SheetAdditiveBrushBuilder->X = 512.0f;
	SheetAdditiveBrushBuilder->Y = 512.0f;
	SheetAdditiveBrushBuilder->XSegments = 1;
	SheetAdditiveBrushBuilder->YSegments = 1;
	SheetAdditiveBrushBuilder->Axis = AX_YAxis;
	SheetAdditiveBrushBuilder->Build(World);
	GEditor->Exec( World, TEXT("BRUSH MOVETO X=-760 Y=-346 Z=535"));
	GEditor->Exec( World, TEXT("BRUSH ADD"));

	//Volume Additive Brush
	UVolumetricBuilder* VolumetricAdditiveBrushBuilder = Cast<UVolumetricBuilder>(GEditor->FindBrushBuilder( UVolumetricBuilder::StaticClass() ));
	VolumetricAdditiveBrushBuilder->Z = 512.0f;
	VolumetricAdditiveBrushBuilder->Radius = 128.0f;
	VolumetricAdditiveBrushBuilder->NumSheets = 3;
	VolumetricAdditiveBrushBuilder->Build(World);
	GEditor->Exec( World, TEXT("BRUSH MOVETO X=445 Y=-345 Z=535"));
	GEditor->Exec( World, TEXT("BRUSH ADD"));

	//Linear Stair Additive Brush
	ULinearStairBuilder* LinearStairAdditiveBrushBuilder = Cast<ULinearStairBuilder>(GEditor->FindBrushBuilder( ULinearStairBuilder::StaticClass() ));
	LinearStairAdditiveBrushBuilder->StepLength = 64.0f;
	LinearStairAdditiveBrushBuilder->StepHeight = 16.0f;
	LinearStairAdditiveBrushBuilder->StepWidth = 256.0f;
	LinearStairAdditiveBrushBuilder->NumSteps = 8;
	LinearStairAdditiveBrushBuilder->AddToFirstStep = 0;
	LinearStairAdditiveBrushBuilder->Build(World);
	GEditor->Exec( World, TEXT("BRUSH MOVETO X=1464 Y=-345 Z=-61"));
	GEditor->Exec( World, TEXT("BRUSH ADD"));

	//Curved Stair Additive Brush
	UCurvedStairBuilder* CurvedStairAdditiveBrushBuilder = Cast<UCurvedStairBuilder>(GEditor->FindBrushBuilder( UCurvedStairBuilder::StaticClass() ));
	CurvedStairAdditiveBrushBuilder->InnerRadius = 240.0f;
	CurvedStairAdditiveBrushBuilder->StepHeight = 16.0f;
	CurvedStairAdditiveBrushBuilder->StepWidth = 256.0f;
	CurvedStairAdditiveBrushBuilder->AngleOfCurve = 90.0f;
	CurvedStairAdditiveBrushBuilder->NumSteps = 4;
	CurvedStairAdditiveBrushBuilder->AddToFirstStep = 0;
	CurvedStairAdditiveBrushBuilder->CounterClockwise = false;
	CurvedStairAdditiveBrushBuilder->Build(World);
	GEditor->Exec( World, TEXT("BRUSH MOVETO X=-1290 Y=263 Z=193"));
	GEditor->Exec( World, TEXT("BRUSH ADD"));

	//Spiral Stair Additive Brush
	USpiralStairBuilder* SpiralStairAdditiveBrushBuilder = Cast<USpiralStairBuilder>(GEditor->FindBrushBuilder( USpiralStairBuilder::StaticClass() ));
	SpiralStairAdditiveBrushBuilder->InnerRadius = 64;
	SpiralStairAdditiveBrushBuilder->StepWidth = 256.0f;
	SpiralStairAdditiveBrushBuilder->StepHeight = 16.0f;
	SpiralStairAdditiveBrushBuilder->StepThickness = 32.0f;
	SpiralStairAdditiveBrushBuilder->NumStepsPer360 = 8;
	SpiralStairAdditiveBrushBuilder->NumSteps = 8;
	SpiralStairAdditiveBrushBuilder->SlopedCeiling = true;
	SpiralStairAdditiveBrushBuilder->Build(World);
	GEditor->Exec( World, TEXT("BRUSH MOVETO X=850 Y=263 Z=193"));
	GEditor->Exec( World, TEXT("BRUSH ADD"));

	//Cylinder Additive Brush
	UCylinderBuilder* CylinderSubtractiveBrushBuilder = Cast<UCylinderBuilder>(GEditor->FindBrushBuilder( UCylinderBuilder::StaticClass() ));
	CylinderSubtractiveBrushBuilder->Z = 256;
	CylinderSubtractiveBrushBuilder->OuterRadius = 512.0f;
	CylinderSubtractiveBrushBuilder->InnerRadius = 384.0f;
	CylinderSubtractiveBrushBuilder->Sides = 3;
	CylinderSubtractiveBrushBuilder->Build(World);
	GEditor->Exec( World, TEXT("BRUSH MOVETO X=0 Y=0 Z=128"));
	GEditor->Exec( World, TEXT("BRUSH SUBTRACT"));

	//Cube Subtractive Brush
	UCubeBuilder* CubeSubtractiveBrushBuilder = Cast<UCubeBuilder>(GEditor->FindBrushBuilder( UCubeBuilder::StaticClass() ));
	CubeSubtractiveBrushBuilder->X = 256.0f;
	CubeSubtractiveBrushBuilder->Y = 1024.0f;
	CubeSubtractiveBrushBuilder->Z = 256.0f;
	CubeSubtractiveBrushBuilder->Build(World);
	GEditor->Exec( World, TEXT("BRUSH MOVETO X=-88 Y=-1777 Z=535"));
	GEditor->Exec( World, TEXT("BRUSH SUBTRACT"));

	//Directional Light
	ADirectionalLight* DirectionalLight = Cast<ADirectionalLight>(GEditor->AddActor(World->GetCurrentLevel(), ADirectionalLight::StaticClass(), FVector(-611.0f, 242.0f, 805.0f)));
	DirectionalLight->SetMobility(EComponentMobility::Movable);
	DirectionalLight->SetActorRotation(FRotator(300, 250, -91));
	DirectionalLight->SetBrightness(3.142f);
	DirectionalLight->SetLightColor(FColor::White);

	GEditorModeTools().MapChangeNotify();
	
	return true;
}


//////////////////////////////////////////////////////////////////////////
/**
 * QA Static Mesh Regression Testing
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStaticMeshValidation, "QA.Mesh Factory Validation", EAutomationTestFlags::ATF_Editor);

bool FStaticMeshValidation::RunTest(const FString& Parameters)
{
	UWorld* World = GEditor->NewMap();

	//Adjust camera in viewports
	for( int32 i = 0; i < GEditor->LevelViewportClients.Num(); i++ )
	{
		FLevelEditorViewportClient* ViewportClient = GEditor->LevelViewportClients[i];
		if(!ViewportClient->IsOrtho())
		{
			ViewportClient->SetViewLocation( FVector(67, 1169, 1130) );
			ViewportClient->SetViewRotation( FRotator(321, 271, 0) );
		}
	}

	//Gather assets
	UObject* EditorCubeMesh = (UStaticMesh*)StaticLoadObject(UStaticMesh::StaticClass(),NULL,TEXT("/Engine/EditorMeshes/EditorCube.EditorCube"),NULL,LOAD_None,NULL);
	UObject* EditorSkeletalMesh = (USkeletalMesh*)StaticLoadObject(USkeletalMesh::StaticClass(),NULL,TEXT("/Engine/EditorMeshes/SkeletalMesh/DefaultSkeletalMesh.DefaultSkeletalMesh"),NULL,LOAD_None,NULL);

	// Static Mesh 0
	AActor* StaticMesh = FActorFactoryAssetProxy::AddActorForAsset( EditorCubeMesh );
	StaticMesh->TeleportTo(FVector(0.0f, 0.0f, 0.0f), FRotator(0, 0, 0));
	StaticMesh->SetActorRelativeScale3D(FVector(50.0f, 50.0f, 1.0f));

	//Static Mesh 1
	StaticMesh = FActorFactoryAssetProxy::AddActorForAsset( EditorCubeMesh );
	StaticMesh->TeleportTo(FVector(-816.0f, -512.0f, 382.0f), FRotator(64, -64, 32));
	StaticMesh->SetActorRelativeScale3D(FVector(1.0f, 1.0f, 2.0f));

	//Interp Actor
	AActor* InterpActor = FActorFactoryAssetProxy::AddActorForAsset( EditorCubeMesh );
	InterpActor->GetRootComponent()->SetMobility(EComponentMobility::Movable);
	{
		bool bIsATest = false;
		bool bNoCheck = true;
		InterpActor->TeleportTo(FVector(-900.0f, 196.0f, 256.0f), FRotator(0, 0, 0), bIsATest, bNoCheck);
	}

	//Physics Actor
	AActor* PhysicsActor = FActorFactoryAssetProxy::AddActorForAsset( EditorCubeMesh );
	PhysicsActor->SetActorRelativeScale3D(FVector(2.0f, 2.0f, .5f));
	PhysicsActor->GetRootComponent()->SetMobility(EComponentMobility::Movable);
	PhysicsActor->GetRootPrimitiveComponent()->SetSimulatePhysics(true);
	PhysicsActor->TeleportTo(FVector(-96.0f, 128.0f, 256.0f), FRotator(0, 0, 0));

	//Skeletal Mesh
	AActor* SkeletalMesh = FActorFactoryAssetProxy::AddActorForAsset( EditorSkeletalMesh );
	SkeletalMesh->SetActorLocationAndRotation(FVector(640.0f, 196.0f, 256.0f), FRotator(12, .5, 24));
	SkeletalMesh->SetActorRelativeScale3D(FVector(2.0f, 3.0f, 2.5f));

	//Single Anim Skeletal Mesh
	//UActorFactory* SingleAnimSkeletalActorFactory = GEditor->FindActorFactoryForActorClass( ASingleAnimSkeletalActor::StaticClass() );
	//SkeletalMesh = FActorFactoryAssetProxy::AddActorForAsset( EditorSkeletalMesh, NULL, false, true, RF_Transactional, SingleAnimSkeletalActorFactory );
	//SkeletalMesh->TeleportTo(FVector(1152.0f, 256.0f, 256.0f), FRotator(0, 0, 0));

	//Directional Light
	ADirectionalLight* DirectionalLight = Cast<ADirectionalLight>(GEditor->AddActor(World->GetCurrentLevel(), ADirectionalLight::StaticClass(), FVector(-611.0f, 242.0f, 805.0f)));
	DirectionalLight->SetMobility(EComponentMobility::Movable);
	DirectionalLight->SetActorRotation(FRotator(329, 346, -105));
	DirectionalLight->SetBrightness(3.142f);
	DirectionalLight->SetLightColor(FColor::White);

	GEditorModeTools().MapChangeNotify();

	return true;
}



//////////////////////////////////////////////////////////////////////////
/**
 * QA Convert Meshes Regression Testing
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConvertToValidation, "QA.Convert Meshes", EAutomationTestFlags::ATF_Editor);

//gather all brushes that exist now
void ConvertTestFindAllBrushes(TArray<ABrush*> &PreviousBrushes)
{
	for( TObjectIterator<ABrush> It; It; ++It )
	{
		ABrush* BrushActor = *It;
		PreviousBrushes.Add(BrushActor);
	}
}

//find brush that was just added by finding the brush not in our previous list
ABrush* ConvertTestFindNewBrush (const TArray<ABrush*> &PreviousBrushes)
{
	ABrush* NewBrush = NULL;
	for( TObjectIterator<ABrush> It; It; ++It )
	{
		ABrush* BrushActor = *It;
		if (!PreviousBrushes.Contains(BrushActor))
		{
			NewBrush = BrushActor;
			break;
		}
	}
	check( NewBrush );
	return NewBrush;
}

bool FConvertToValidation::RunTest(const FString& Parameters)
{
	UWorld* World = GEditor->NewMap();

	//Adjust camera in viewports
	for( int32 i = 0; i < GEditor->LevelViewportClients.Num(); i++ )
	{
		FLevelEditorViewportClient* ViewportClient = GEditor->LevelViewportClients[i];
		if(!ViewportClient->IsOrtho())
		{
			ViewportClient->SetViewLocation( FVector(448, 902, 423) );
			ViewportClient->SetViewRotation( FRotator(0, 270, 0) );
		}
	}

	//Gather assets
	UObject* EditorCubeMesh = (UStaticMesh*)StaticLoadObject(UStaticMesh::StaticClass(),NULL,TEXT("/Engine/EditorMeshes/EditorCube.EditorCube"),NULL,LOAD_None,NULL);

	//BSP TO BLOCKING VOLUME
	{
		TArray<ABrush*> PreviousBrushes;
		ConvertTestFindAllBrushes(PreviousBrushes);

		//Add the new brush
		UCubeBuilder* CubeAdditiveBrushBuilder = Cast<UCubeBuilder>(GEditor->FindBrushBuilder( UCubeBuilder::StaticClass() ));
		CubeAdditiveBrushBuilder->X = 256.0f;
		CubeAdditiveBrushBuilder->Y = 256.0f;
		CubeAdditiveBrushBuilder->Z = 256.0f;
		CubeAdditiveBrushBuilder->Build(World);
		GEditor->Exec( World, TEXT("BRUSH MOVETO X=768 Y=0 Z=768"));
		GEditor->Exec( World, TEXT("BRUSH ADD"));

		//find brush that was just added by finding the brush not in our previous list
		ABrush* NewBrush = ConvertTestFindNewBrush(PreviousBrushes);
		check( NewBrush );

		//modify selection - convert to blocking volume
		const bool bNoteSelectionChange = true;
		const bool bDeselectBSPSurfaces = true;
		GEditor->SelectNone(bNoteSelectionChange, bDeselectBSPSurfaces);
		GEditor->GetSelectedActors()->Modify();
		GEditor->GetSelectedActors()->Select( NewBrush );	
		GEditor->ConvertSelectedBrushesToVolumes( ABlockingVolume::StaticClass() );
		GEditor->RebuildAlteredBSP();
	}

	//convert to static mesh
	{
		TArray<ABrush*> PreviousBrushes;
		ConvertTestFindAllBrushes(PreviousBrushes);

		//Add the new brush
		UCubeBuilder* CubeAdditiveBrushBuilder = Cast<UCubeBuilder>(GEditor->FindBrushBuilder( UCubeBuilder::StaticClass() ));
		CubeAdditiveBrushBuilder->X = 256.0f;
		CubeAdditiveBrushBuilder->Y = 256.0f;
		CubeAdditiveBrushBuilder->Z = 256.0f;
		CubeAdditiveBrushBuilder->Build(World);
		GEditor->Exec( World, TEXT("BRUSH MOVETO X=0 Y=0 Z=0"));
		GEditor->Exec( World, TEXT("BRUSH ADD"));

		//find brush that was just added by finding the brush not in our previous list
		ABrush* NewBrush = ConvertTestFindNewBrush(PreviousBrushes);
		check( NewBrush );
		TArray<AActor*> ToStaticMeshActors;
		ToStaticMeshActors.Add(NewBrush);

		//generate static mesh package name
		FString PackageName = FPackageName::FilenameToLongPackageName(FPaths::AutomationTransientDir() + TEXT("ConvertToBSPToStaticMesh"));
		//Convert brush to specific package name
		GEditor->DoConvertActors(ToStaticMeshActors, AStaticMeshActor::StaticClass(), TSet<FString>(), true, FText::FromString(PackageName));

		//find the package
		UPackage* NewPackage = FindPackage(NULL, *PackageName);
		if (NewPackage)
		{
			TArray<UPackage*> PackagesToSave;
			PackagesToSave.Add(NewPackage);

			//save the package
			const bool bCheckDirty = false;
			const bool bPromptToSave = false;
			const FEditorFileUtils::EPromptReturnCode Return = FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, bCheckDirty, bPromptToSave);
		}
		else
		{
			UE_LOG(LogEditorAutomationTests, Error, TEXT("Failed to save ConvertToBSPToStaticMesh."));
		}
	}


	//Directional Light
	ADirectionalLight* DirectionalLight = Cast<ADirectionalLight>(GEditor->AddActor(World->GetCurrentLevel(), ADirectionalLight::StaticClass(), FVector(384, 0, 384)));
	DirectionalLight->SetMobility(EComponentMobility::Movable);
	DirectionalLight->SetActorRotation(FRotator(314, 339, 0));
	DirectionalLight->SetBrightness(3.142f);
	DirectionalLight->SetLightColor(FColor::White);

	GEditorModeTools().MapChangeNotify();

	return true;
}



//////////////////////////////////////////////////////////////////////////
/**
 * QA Test to open editor windows
 */
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenAssetEditors, "QA.Open Asset Editors", EAutomationTestFlags::ATF_Editor);

void FOpenAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands) const
{
	UAutomationTestSettings const* AutomationTestSettings = GetDefault<UAutomationTestSettings>();
	check(AutomationTestSettings);

	bool bUnAttended = FApp::IsUnattended();

	TArray<FString> AssetNames;
	for (auto Iter = AutomationTestSettings->TestAssetsToOpen.CreateConstIterator(); Iter; ++Iter)
	{
		if (Iter->bSkipTestWhenUnAttended && bUnAttended)
		{
			continue;
		}

		if ( Iter->AssetToOpen.FilePath.Len() > 0 )
		{
			AssetNames.AddUnique(Iter->AssetToOpen.FilePath);
		}
	}


	for (int32 i = 0; i < AssetNames.Num(); ++i)
	{
		FString Filename = FPaths::ConvertRelativePathToFull(AssetNames[i]);

		if (FPaths::MakePathRelativeTo( Filename, *FPaths::EngineContentDir() ))
		{
			FString ShortName = FPaths::GetBaseFilename(Filename);
			FString PathName =  FPaths::GetPath(Filename);
			OutBeautifiedNames.Add(ShortName);
			FString AssetName = FString::Printf(TEXT("/Engine/%s/%s.%s"), *PathName, *ShortName, *ShortName);
			OutTestCommands.Add(AssetName);
		}
		else if (FPaths::MakePathRelativeTo( Filename, *FPaths::GameContentDir() ))
		{
			FString ShortName = FPaths::GetBaseFilename(Filename);
			FString PathName =  FPaths::GetPath(Filename);
			OutBeautifiedNames.Add(ShortName);
			FString AssetName = FString::Printf(TEXT("/Game/%s/%s.%s"), *PathName, *ShortName, *ShortName);
			OutTestCommands.Add(AssetName);
		}
		else
		{
			UE_LOG(LogEditorAutomationTests, Error, TEXT("Invalid asset path: %s."), *Filename);
		}
	}
}

bool FOpenAssetEditors::RunTest(const FString& Parameters)
{
	//start with all editors closed
	FAssetEditorManager::Get().CloseAllAssetEditors();

	// below is all latent action, so before sending there, verify the asset exists
	UObject* Object = StaticLoadObject(UObject::StaticClass(), NULL, *Parameters);
	if (!Object)
	{
		UE_LOG(LogEditorAutomationTests, Error, TEXT("Failed to find object: %s."), *Parameters);
		return false;
	}

	ADD_LATENT_AUTOMATION_COMMAND(FOpenEditorForAssetCommand(*Parameters));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));
	ADD_LATENT_AUTOMATION_COMMAND(FCloseAllAssetEditorsCommand());

	return true;
}


//////////////////////////////////////////////////////////////////////////
/**
 * QA Static Mesh Regression Testing
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStaticMeshPlacement, "QA.Static Mesh Placement", EAutomationTestFlags::ATF_Editor);

bool FStaticMeshPlacement::RunTest(const FString& Parameters)
{
	FString MapName = TEXT("/Engine/Maps/Templates/Template_Default");
	FEditorAutomationTestUtilities::LoadMap(MapName);

	//Gather assets
	UObject* EditorCylinderMesh = (UStaticMesh*)StaticLoadObject(UStaticMesh::StaticClass(),NULL,TEXT("/Engine/EditorMeshes/EditorCylinder.EditorCylinder"),NULL,LOAD_None,NULL);

	// Add cylinder to world
	AStaticMeshActor* StaticMesh = Cast<AStaticMeshActor>(FActorFactoryAssetProxy::AddActorForAsset(EditorCylinderMesh));
	StaticMesh->TeleportTo(FVector(-16.0f, 448.0f, 608.0f), FRotator(0, 0, 0));
	StaticMesh->SetActorRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));

	{
		FScopedTransaction DuplicateMeshScope( NSLOCTEXT("UnrealEd.Test", "UndoStaticMeshPlacementTest", "Undo Static Mesh Placement Test") );

		bool bOffsetLocations = false;
		//Duplicate the mesh
		GEditor->edactDuplicateSelected(StaticMesh->GetLevel(), bOffsetLocations);
		for ( FSelectionIterator It( GEditor->GetSelectedActorIterator() ) ; It ; ++It )
		{
			AActor* Actor = static_cast<AActor*>( *It );
			Actor->TeleportTo(FVector(304.0f,448.0f,608.0f), FRotator(0, 0, 0));
		}
	}

	GEditor->UndoTransaction();

	FString MaterialName = TEXT("/Engine/MapTemplates/Materials/BasicAsset01.BasicAsset01");
	UMaterialInterface* Material = (UMaterialInterface*)StaticLoadObject(UMaterialInterface::StaticClass(),NULL,*MaterialName,NULL,LOAD_None,NULL);
	if (Material)
	{
		FActorFactoryAssetProxy::ApplyMaterialToActor( StaticMesh, Material );
	}
	else
	{
		UE_LOG(LogEditorAutomationTests, Error, TEXT("Failed to find material: %s"), *MaterialName);
	}

	StaticMesh->TeleportTo(FVector(160.0f,448.0f, 608.0f), FRotator(0, 280, 0));
	StaticMesh->SetActorRelativeScale3D(FVector(1.5f, 1.5f, 1.5f));

	return true;
}




//////////////////////////////////////////////////////////////////////////
/**
 * QA Light Placement Regression Testing
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLightPlacement, "QA.Point Light Placement", EAutomationTestFlags::ATF_Editor);

bool FLightPlacement::RunTest(const FString& Parameters)
{
	FString MapName = TEXT("/Engine/Maps/Templates/Template_Default");
	FEditorAutomationTestUtilities::LoadMap(MapName);

	UWorld *World = NULL;
	{
		//Gather assets
		UObject* EditorCylinderMesh = (UStaticMesh*)StaticLoadObject(UStaticMesh::StaticClass(),NULL,TEXT("/Engine/EditorMeshes/EditorCylinder.EditorCylinder"),NULL,LOAD_None,NULL);
		// Add cylinder to world
		AStaticMeshActor* StaticMesh = Cast<AStaticMeshActor>(FActorFactoryAssetProxy::AddActorForAsset(EditorCylinderMesh));
		World = StaticMesh->GetWorld();
		//delete static mesh now that we have the world.  Not ideal. :/
		GEditor->edactDeleteSelected(World);
	}


	//Point Light
	APointLight* PointLight = Cast<APointLight>(GEditor->AddActor(World->GetCurrentLevel(), APointLight::StaticClass(), FVector(-80.0f, 400.0f, 544.0f)));
	PointLight->SetMobility(EComponentMobility::Movable);
	PointLight->SetBrightness(116.0f);
	PointLight->SetLightColor(FColor(200, 100, 200));

	{
		FScopedTransaction DuplicateLightScope( NSLOCTEXT("UnrealEd.Test", "DuplicateLightScope", "Duplicate Light Scope") );

		//Duplicate the light
		bool bOffsetLocations = false;
		GEditor->edactDuplicateSelected(PointLight->GetLevel(), bOffsetLocations);
		for ( FSelectionIterator It( GEditor->GetSelectedActorIterator() ) ; It ; ++It )
		{
			AActor* Actor = static_cast<AActor*>( *It );
			Actor->TeleportTo(FVector(192.0f,400.0f,544.0f), FRotator(0, 0, 0));
		}
	}

	GEditor->UndoTransaction();

	PointLight->TeleportTo(FVector(-464.0f,400.0f, 544.0f), FRotator(0, 164, 0));
	PointLight->SetRadius( 5000.0f );

	return true;
}



/**
 * TraceAllTimelines
 * Unit test to find all timelines in blueprints and list the events that can trigger them.
 * Timelines implicitly tick and are usually used for cosmetic events, so they can cause performance problems on dedicated servers.
 */
IMPLEMENT_COMPLEX_AUTOMATION_TEST( FTraceAllTimelinesAutomationTest, "Performance Audits.Find Timelines On Server", EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser)

/** 
 * Requests an enumeration of all blueprints to be loaded
 */
void FTraceAllTimelinesAutomationTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands) const
{
	FEditorAutomationTestUtilities::CollectTestsByClass(UBlueprint::StaticClass(), OutBeautifiedNames, OutTestCommands);
}


namespace TraceHelper
{
	static const FName CosmeticMacroName(TEXT("Can Execute Cosmetic Events"));
	static const FString CosmeticCheckedPinName(TEXT("True"));
	static const FName SetTimerName(TEXT("K2_SetTimer"));
	static const FString SetTimerFunctionFieldName(TEXT("FunctionName"));

	typedef TArray<UK2Node_MacroInstance*, TInlineAllocator<2>> TContextStack;
	typedef TMap<UK2Node_MacroInstance*, struct FVisitedTracker> TGraphNodesVisited;
	typedef TArray<struct FNodeScope, TInlineAllocator<32>> TNodeScopeStack;
	typedef TSet<struct FVisitedNode> TVisitedNodeSet;

	// converts a bool indicating whether we are looking at cosmetic or non-cosmetic nodes into an index in an array.
	inline int32 ToIndex(bool bCosmeticChain)
	{
		return bCosmeticChain ? 0 : 1;
	}

	// Node we have visited (and pin we visited through)
	struct FVisitedNode
	{
		FVisitedNode(UK2Node* InNode = NULL, const UEdGraphPin* InPin = NULL)
			: Node(InNode)
			, Pin(InPin)
		{
		}

		bool operator==(const FVisitedNode& Other) const
		{
			return Node == Other.Node && Pin == Other.Pin;
		}

		UK2Node* Node;
		const UEdGraphPin* Pin;
	};

	inline uint32 GetTypeHash( const struct FVisitedNode& VisitedNode )
	{
		const UPTRINT NodePtr = reinterpret_cast<UPTRINT>(VisitedNode.Node);
		const UPTRINT PinPtr = reinterpret_cast<UPTRINT>(VisitedNode.Pin);
		return ::GetTypeHash(reinterpret_cast<void*>(NodePtr ^ PinPtr));
	}

	// Track nodes that have been visited within a context, either along a cosmetic execution chain or non-cosmetic (checked).
	// This is necessary to avoid infinite loops when tracing a sequence in a graph.
	struct FVisitedTracker
	{
		bool IsVisited(UK2Node* Node, bool bCosmeticChain, const UEdGraphPin* Pin) const
		{
			const int32 Index = ToIndex(bCosmeticChain);
			return Nodes[Index].Contains(FVisitedNode(Node, Pin));
		}

		void AddNode(UK2Node* Node, bool bCosmeticChain, const UEdGraphPin* Pin)
		{
			const int32 Index = ToIndex(bCosmeticChain);
			Nodes[Index].Add(FVisitedNode(Node, Pin));
		}

		TVisitedNodeSet Nodes[2];
	};

	// K2Node and macro context stack within which the node exists
	struct FNodeScope
	{
		FNodeScope(UK2Node* InNode, const TContextStack& InContextStack, bool bInCosmeticChain, const UEdGraphPin* InPin)
			: Node(InNode)
			, Pin(InPin)
			, ContextStack(InContextStack)
			, bCosmeticChain(bInCosmeticChain)
		{
			check(InNode != NULL);
		}

		UK2Node*			Node;
		const UEdGraphPin*	Pin; // If NULL, then consider all pins for this node.
		TContextStack		ContextStack;
		bool				bCosmeticChain;
	};

	// Add a node to the NodeScopeStack if it has not been visited already.
	bool AddNode(UK2Node* Node, TNodeScopeStack& NodeScopeStack, TGraphNodesVisited& GraphNodesVisited, const FNodeScope& NodeScope, bool bCosmeticChain, const UEdGraphPin* Pin = NULL)
	{
		check(Node != NULL);

		FVisitedTracker* VisitedTracker = NULL;
		if (NodeScope.ContextStack.Num() > 0)
		{
			UK2Node_MacroInstance* MacroInstance = NodeScope.ContextStack.Top();
			VisitedTracker = &GraphNodesVisited.FindOrAdd(MacroInstance);
		}
		else
		{
			// Not a macro, just a plain graph; we use NULL for that.
			VisitedTracker = GraphNodesVisited.Find(NULL);
			check(VisitedTracker);
		}

		if (!VisitedTracker->IsVisited(Node, bCosmeticChain, Pin))
		{
			VisitedTracker->AddNode(Node, bCosmeticChain, Pin);
			NodeScopeStack.Add(FNodeScope(Node, NodeScope.ContextStack, bCosmeticChain, Pin));
			return true;
		}

		return false;
	}
}

/** 
 * Execute the loading of each blueprint
 *
 * @param Parameters - Should specify which blueprint name to load
 * @return	TRUE if the test was successful, FALSE otherwise
 */

bool FTraceAllTimelinesAutomationTest::RunTest(const FString& BlueprintName)
{
	UBlueprint* BlueprintObj = Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), NULL, *BlueprintName));
	if (!BlueprintObj || !BlueprintObj->ParentClass )
	{
		UE_LOG(LogEditorAutomationTests, Error, TEXT("Failed to load invalid blueprint, or blueprint parent no longer exists."));
		return false;
	}

	bool bPassed = true;

	// List all timelines
	TArray<UK2Node_Timeline*> AllTimelines;
	FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_Timeline>(BlueprintObj, AllTimelines);
	if (AllTimelines.Num() > 0)
	{
		// Cached list of all CallFunction nodes in this blueprint.
		bool bFoundCallFunctionNodes = false;
		TArray<UK2Node_CallFunction*> AllCallFunctionNodes;

		for (auto TimelineIter = AllTimelines.CreateIterator(); TimelineIter; ++TimelineIter)
		{
			const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
			UK2Node_Timeline* TimelinePtr = *TimelineIter;
			UE_LOG(LogEditorAutomationTests, Log, TEXT("TraceTimeline: %s [%s]"), *TimelinePtr->GetPathName(), *TimelinePtr->TimelineName.ToString());

			// Walk up the execution chain and find the list of events that can trigger the timeline
			TraceHelper::TNodeScopeStack NodeScopeStack;
			TraceHelper::TGraphNodesVisited GraphNodesVisited;
			typedef TArray<UK2Node_Event*, TInlineAllocator<16>> TEventList;
			TEventList UncheckedEventNodes;
			TEventList CheckedEventNodes;
			TEventList WarningEventNodes;

			// Initial starting node is the Timeline itself.
			NodeScopeStack.Add(TraceHelper::FNodeScope(TimelinePtr, TraceHelper::TContextStack(), true, NULL));
			GraphNodesVisited.Add(NULL, TraceHelper::FVisitedTracker()).AddNode(TimelinePtr, true, NULL);

			while (NodeScopeStack.Num() > 0)
			{
				TraceHelper::FNodeScope CurrentScope = NodeScopeStack.Pop();
				UK2Node* CurrentNode = CurrentScope.Node;

				// Check if this is an event node we're looking for.
				UK2Node_Event* Event = Cast<UK2Node_Event>(CurrentNode);
				if (Event)
				{
					if (UK2Node_CustomEvent* CustomEvent = Cast<UK2Node_CustomEvent>(CurrentNode))
					{
						// Build list of all CallFunction nodes
						if (!bFoundCallFunctionNodes)
						{
							bFoundCallFunctionNodes = true;
							FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_CallFunction>(BlueprintObj, AllCallFunctionNodes);
						}

						// Expand all CallFunction nodes that reference this event.
						bool bFoundMatch = false;
						for (int32 CallIndex = 0; CallIndex < AllCallFunctionNodes.Num(); ++CallIndex)
						{
							UK2Node_CallFunction* CallFunctionNode = AllCallFunctionNodes[CallIndex];
							if (CustomEvent->GetFunctionName() == CallFunctionNode->FunctionReference.GetMemberName())
							{
								bFoundMatch = true;
								TraceHelper::AddNode(CallFunctionNode, NodeScopeStack, GraphNodesVisited, CurrentScope, CurrentScope.bCosmeticChain);
							}
							else if (CallFunctionNode->FunctionReference.GetMemberName() == TraceHelper::SetTimerName)
							{
								UEdGraphPin* FunctionPin = CallFunctionNode->FindPin(TraceHelper::SetTimerFunctionFieldName);
								if (FunctionPin && CustomEvent->GetFunctionName().ToString() == FunctionPin->DefaultValue)
								{
									bFoundMatch = true;
									TraceHelper::AddNode(CallFunctionNode, NodeScopeStack, GraphNodesVisited, CurrentScope, CurrentScope.bCosmeticChain);
								}
							}
						}

						if (!bFoundMatch)
						{
							WarningEventNodes.AddUnique(CustomEvent);
						}

						continue;
					}
					else
					{
						// This is a native event, which is an entry point to the BP
						if (CurrentScope.bCosmeticChain)
						{
							UncheckedEventNodes.AddUnique(Event);
						}
						else
						{
							CheckedEventNodes.AddUnique(Event);
						}

						continue;
					}
				}
				else if (UK2Node_Tunnel* Tunnel = Cast<UK2Node_Tunnel>(CurrentNode))
				{
					// Handle tunnel nodes (collapsed graphs and macros)
					if (UK2Node_MacroInstance* MacroInstance = Cast<UK2Node_MacroInstance>(CurrentNode))
					{
						// Enter a macro
						if (UEdGraph* MacroGraph = MacroInstance->GetMacroGraph())
						{
							// We don't want to expand into this special macro.
							if (MacroGraph->GetFName() != TraceHelper::CosmeticMacroName)
							{
								// Jump to the output node of the macro
								TArray<UK2Node_Tunnel*> TunnelNodes;
								MacroGraph->GetNodesOfClass(TunnelNodes);
								for (int32 i = 0; i < TunnelNodes.Num(); i++)
								{
									UK2Node_Tunnel* Node = TunnelNodes[i];
									if (Node->bCanHaveInputs && !Node->bCanHaveOutputs)
									{
										// Push this macro on the context stack and add the output node.
										CurrentScope.ContextStack.Push(MacroInstance);
										
										// Visit the pin on the node matching the one we are entering the macro with.
										const UEdGraphPin* MatchingPin = CurrentScope.Pin ? Node->FindPin(CurrentScope.Pin->PinName) : NULL;										
										TraceHelper::AddNode(Node, NodeScopeStack, GraphNodesVisited, CurrentScope, CurrentScope.bCosmeticChain, MatchingPin);
										break;
									}
								}

								// Done with this node, we expanded the macro.
								continue;
							}	
						}
					}
					else if (UK2Node_Composite* CompositeNode = Cast<UK2Node_Composite>(CurrentNode))
					{
						// Jump to the output node within the composite graph.
						UK2Node_Tunnel* Node = CompositeNode->GetExitNode();
						if (Node)
						{
							// Visit the pin on the node matching the one we are entering the macro with.
							const UEdGraphPin* MatchingPin = CurrentScope.Pin ? Node->FindPin(CurrentScope.Pin->PinName) : NULL;
							TraceHelper::AddNode(Node, NodeScopeStack, GraphNodesVisited, CurrentScope, CurrentScope.bCosmeticChain, MatchingPin);
						}

						// Done with this node, we expanded the graph.
						continue;
					}
					else if (Tunnel->bCanHaveOutputs && !Tunnel->bCanHaveInputs)
					{
						// Exiting a composite graph or macro
						UK2Node_Tunnel* TunnelSource = Tunnel->GetOutputSource();
						
						// We get a null tunnel source for macros
						if (TunnelSource == NULL)
						{
							TunnelSource = CurrentScope.ContextStack.Pop();
							check(TunnelSource);
						}

						// The tunnel node has input pins we can follow, now that we've dug down through the macro itself.
						CurrentNode = TunnelSource;
						CurrentScope.Node = CurrentNode;
						CurrentScope.Pin = CurrentScope.Pin ? CurrentNode->FindPin(CurrentScope.Pin->PinName) : NULL;

						// Expand the source node immediately (do not restart the loop)
					}
				}

				if (CurrentNode)
				{
					//
					// General Nodes
					// Expand all input exec pins (timelines have more than one, for example)
					//
					for (auto PinIter = CurrentNode->Pins.CreateConstIterator(); PinIter; ++PinIter)
					{
						const UEdGraphPin* CurrentPin = CurrentScope.Pin ? CurrentScope.Pin : *PinIter;
						if (CurrentPin->Direction == EGPD_Input && CurrentPin->PinType.PinCategory == K2Schema->PC_Exec)
						{
							for (auto LinkedToIter = CurrentPin->LinkedTo.CreateConstIterator(); LinkedToIter; ++LinkedToIter)
							{
								const UEdGraphPin* OtherPin = *LinkedToIter;
								UK2Node* OtherPinNode = Cast<UK2Node>(OtherPin->GetOuter());
								bool bCosmeticChain = CurrentScope.bCosmeticChain;

								UK2Node_Tunnel* Tunnel = Cast<UK2Node_Tunnel>(OtherPinNode);
								if (Tunnel)
								{
									// See if this is an explicit check for allowable cosmetic actions.
									UK2Node_MacroInstance* MacroInstance = Cast<UK2Node_MacroInstance>(OtherPinNode);
									if (MacroInstance)
									{
										UEdGraph* MacroGraph = MacroInstance->GetMacroGraph();
										if (MacroGraph && MacroGraph->GetFName() == TraceHelper::CosmeticMacroName)
										{
											if (bCosmeticChain)
											{
												// This execution chain is checking that it is safe to execute cosmetic events.
												bCosmeticChain = (OtherPin->PinName != TraceHelper::CosmeticCheckedPinName);
											}
											// Don't bother trying to identify pins on this special node, all it does is change the cosmetic chain state.
											OtherPin = NULL;
										}
									}
								}

								// We only really care to distinguish pins for tunnel nodes,
								// because those might have different input pins hooked up to the logic ending in this output pin
								TraceHelper::AddNode(OtherPinNode, NodeScopeStack, GraphNodesVisited, CurrentScope, bCosmeticChain, Tunnel ? OtherPin : NULL);
							}
						}

						// We were restricted to only this one pin.
						if (CurrentScope.Pin)
						{
							break;
						}
					}
				}				
			}

			// Build list of all unique events.
			TEventList AllEventNodes;
			AllEventNodes.Append(UncheckedEventNodes);
			for (int32 EventIndex = 0; EventIndex < CheckedEventNodes.Num(); ++EventIndex)
			{
				AllEventNodes.AddUnique(CheckedEventNodes[EventIndex]);
			}
			AllEventNodes.Append(WarningEventNodes);

			// Now list all the event nodes
			for (int32 EventIndex = 0; EventIndex < AllEventNodes.Num(); ++EventIndex)
			{
				UK2Node_Event* Event = AllEventNodes[EventIndex];
				UFunction* Function = FindField<UFunction>(Event->EventSignatureClass, Event->EventSignatureName);

				const bool bIsCosmeticEvent = (Function && Function->HasAllFunctionFlags(FUNC_BlueprintCosmetic)) || Event->IsCosmeticTickEvent();
				const bool bIsCosmeticChain = UncheckedEventNodes.Contains(Event);
				const bool bIsBadEvent = (!bIsCosmeticEvent && bIsCosmeticChain);
				const bool bIsWarningEvent = WarningEventNodes.Contains(Event);
				const TCHAR SymbolString = bIsBadEvent ? TEXT('-') : (bIsWarningEvent ? TEXT('?') : TEXT('+'));
				const TCHAR* CosmeticString = bIsCosmeticEvent ? TEXT("Client") : TEXT("Server");
				FString OutputText = FString::Printf(TEXT("TraceTimeline:   %c %sEvent '%s' -> %s"), SymbolString, CosmeticString, *Event->GetNodeTitle(ENodeTitleType::EditableTitle), *TimelinePtr->TimelineName.ToString());

				if (bIsBadEvent)
				{
					// This is an error if we have not branched on a condition checking whether cosmetic events are allowed.
					UE_LOG(LogEditorAutomationTests, Error, TEXT("%s"), *OutputText);
					bPassed = false;
				}
				else if (bIsWarningEvent)
				{
					UE_LOG(LogEditorAutomationTests, Warning, TEXT("%s"), *(OutputText + TEXT(" (could not find function calling this event)")));
				}

				// I always want it in the log as well as the automation test log.
				UE_LOG(LogEditorAutomationTests, Log, TEXT("%s"), *OutputText);
			}
		}
	}

	return bPassed;
}

