// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PaperJsonImporterPrivatePCH.h"
#include "Paper2DClasses.h"
#include "Json.h"
#include "PaperJSONHelpers.h"
#include "AssetToolsModule.h"
#include "AssetRegistryModule.h"
#include "PackageTools.h"

//////////////////////////////////////////////////////////////////////////
// FSpriteFrame

// Represents one parsed frame in a sprite sheet
struct FSpriteFrame
{
	FName FrameName;

	FVector2D SpritePosInSheet;
	FVector2D SpriteSizeInSheet;

	FVector2D SpriteSourcePos;
	FVector2D SpriteSourceSize;

	FVector2D ImageSourceSize;

	bool bTrimmed;
	bool bRotated;
};

//////////////////////////////////////////////////////////////////////////
// UPaperJsonImporterFactory

UPaperJsonImporterFactory::UPaperJsonImporterFactory(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bCreateNew = false;
	//bEditAfterNew = true;
	SupportedClass = UPaperFlipbook::StaticClass();

	bEditorImport = true;
	bText = true;

	Formats.Add(TEXT("json;Spritesheet JSON file"));
}

FText UPaperJsonImporterFactory::GetToolTip() const
{
	return NSLOCTEXT("Paper2D", "PaperJsonImporterFactoryDescription", "Sprite sheets exported from Adobe Flash");
}

static bool ParseFrame(TSharedPtr<FJsonObject> &FrameData, FSpriteFrame &OutFrame)
{
	bool bReadFrameSuccessfully = true;
	// An example frame:
	//   "frame": {"x":210,"y":10,"w":190,"h":223},
	//   "rotated": false,
	//   "trimmed": true,
	//   "spriteSourceSize": {"x":0,"y":11,"w":216,"h":240},
	//   "sourceSize": {"w":216,"h":240}

	OutFrame.bRotated = FPaperJSONHelpers::ReadBoolean(FrameData, TEXT("rotated"), false);
	OutFrame.bTrimmed = FPaperJSONHelpers::ReadBoolean(FrameData, TEXT("trimmed"), false);

	// Put this warning back in until rotation / trim support is implemented
	if (OutFrame.bRotated || !OutFrame.bTrimmed)
	{
		//@TODO: Should learn how to support this combo
		UE_LOG(LogPaperJsonImporter, Warning, TEXT("Frame %s is either rotated or not trimmed, which may not be handled correctly"), *OutFrame.FrameName.ToString());
	}

	bReadFrameSuccessfully = bReadFrameSuccessfully && FPaperJSONHelpers::ReadRectangle(FrameData, "frame", /*out*/ OutFrame.SpritePosInSheet, /*out*/ OutFrame.SpriteSizeInSheet);

	if (OutFrame.bTrimmed)
	{
		bReadFrameSuccessfully = bReadFrameSuccessfully && FPaperJSONHelpers::ReadSize(FrameData, "sourceSize", /*out*/ OutFrame.ImageSourceSize);
		bReadFrameSuccessfully = bReadFrameSuccessfully && FPaperJSONHelpers::ReadRectangle(FrameData, "spriteSourceSize", /*out*/ OutFrame.SpriteSourcePos, /*out*/ OutFrame.SpriteSourceSize);
	}
	else
	{
		OutFrame.SpriteSourcePos = FVector2D::ZeroVector;
		OutFrame.SpriteSourceSize = OutFrame.SpriteSizeInSheet;
		OutFrame.ImageSourceSize = OutFrame.SpriteSizeInSheet;
	}

	// A few more prerequisites to sort out before rotation can be enabled
	//if (OutFrame.bRotated)
	//{
	//	// We rotate the sprite 90 deg CCW
	//	Swap(OutFrame.SpriteSizeInSheet.X, OutFrame.SpriteSizeInSheet.Y);
	//}

	return bReadFrameSuccessfully;
}

static bool ParseFramesFromSpriteHash(TSharedPtr<FJsonObject> ObjectBlock, TArray<FSpriteFrame>& OutSpriteFrames, TSet<FName>& FrameNames)
{
	GWarn->BeginSlowTask(NSLOCTEXT("Paper2D", "PaperJsonImporterFactory_ParsingSprites", "Parsing Sprite Frame"), true, true);
	bool bLoadedSuccessfully = true;

	// Parse all of the frames
	int32 FrameCount = 0;
	for (auto FrameIt = ObjectBlock->Values.CreateIterator(); FrameIt; ++FrameIt)
	{
		GWarn->StatusUpdate(FrameCount, ObjectBlock->Values.Num(), NSLOCTEXT("Paper2D", "PaperJsonImporterFactory_ParsingSprites", "Parsing Sprite Frames"));

		bool bReadFrameSuccessfully = true;

		FSpriteFrame Frame;
		Frame.FrameName = *FrameIt.Key();

		if (FrameNames.Contains(Frame.FrameName))
		{
			bReadFrameSuccessfully = false;
		}
		else
		{
			FrameNames.Add(Frame.FrameName);
		}

		TSharedPtr<FJsonValue> FrameDataAsValue = FrameIt.Value();
		TSharedPtr<FJsonObject> FrameData;
		if (FrameDataAsValue->Type == EJson::Object)
		{
			FrameData = FrameDataAsValue->AsObject();
			bReadFrameSuccessfully = bReadFrameSuccessfully && ParseFrame(FrameData, /*out*/Frame);
		}
		else
		{
			bReadFrameSuccessfully = false;
		}

		if (bReadFrameSuccessfully)
		{
			OutSpriteFrames.Add(Frame);
		}
		else
		{
			UE_LOG(LogPaperJsonImporter, Warning, TEXT("Frame %s is in an unexpected format"), *Frame.FrameName.ToString());
			bLoadedSuccessfully = false;
		}

		FrameCount++;
	}

	GWarn->EndSlowTask();
	return bLoadedSuccessfully;
}

static bool ParseFramesFromSpriteArray(const TArray<TSharedPtr<FJsonValue>>& ArrayBlock, TArray<FSpriteFrame>& OutSpriteFrames, TSet<FName>& FrameNames)
{
	GWarn->BeginSlowTask(NSLOCTEXT("Paper2D", "PaperJsonImporterFactory_ParsingSprites", "Parsing Sprite Frame"), true, true);
	bool bLoadedSuccessfully = true;

	// Parse all of the frames
	int32 FrameCount = 0;
	for (int32 FrameCount = 0; FrameCount < ArrayBlock.Num(); ++FrameCount)
	{
		GWarn->StatusUpdate(FrameCount, ArrayBlock.Num(), NSLOCTEXT("Paper2D", "PaperJsonImporterFactory_ParsingSprites", "Parsing Sprite Frames"));
		bool bReadFrameSuccessfully = true;
		FSpriteFrame Frame;

		const TSharedPtr<FJsonValue>& FrameDataAsValue = ArrayBlock[FrameCount];
		if (FrameDataAsValue->Type == EJson::Object)
		{
			TSharedPtr<FJsonObject> FrameData;
			FrameData = FrameDataAsValue->AsObject();

			FString FrameFilename = FPaperJSONHelpers::ReadString(FrameData, TEXT("filename"), TEXT(""));
			if (!FrameFilename.IsEmpty())
			{
				Frame.FrameName = FName(*FrameFilename); // case insensitive
				if (FrameNames.Contains(Frame.FrameName))
				{
					bReadFrameSuccessfully = false;
				}
				else
				{
					FrameNames.Add(Frame.FrameName);
				}

				bReadFrameSuccessfully = bReadFrameSuccessfully && ParseFrame(FrameData, /*out*/Frame);
			}
			else
			{
				bReadFrameSuccessfully = false;
			}
		}
		else
		{
			bReadFrameSuccessfully = false;
		}

		if (bReadFrameSuccessfully)
		{
			OutSpriteFrames.Add(Frame);
		}
		else
		{
			UE_LOG(LogPaperJsonImporter, Warning, TEXT("Frame %s is in an unexpected format"), *Frame.FrameName.ToString());
			bLoadedSuccessfully = false;
		}
	}

	GWarn->EndSlowTask();
	return bLoadedSuccessfully;
}

UObject* UPaperJsonImporterFactory::FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn)
{
	Flags |= RF_Transactional;

	FEditorDelegates::OnAssetPreImport.Broadcast(this, InClass, InParent, InName, Type);

	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");

	bool bLoadedSuccessfully = true;

	const FString CurrentFilename = UFactory::GetCurrentFilename();
	FString CurrentSourcePath;
	FString FilenameNoExtension;
	FString UnusedExtension;
	FPaths::Split(CurrentFilename, CurrentSourcePath, FilenameNoExtension, UnusedExtension);

	const FString LongPackagePath = FPackageName::GetLongPackagePath(InParent->GetOutermost()->GetPathName());

	const FString NameForErrors(InName.ToString());
	const FString FileContent(BufferEnd - Buffer, Buffer);
	TSharedPtr<FJsonObject> SpriteDescriptorObject = ParseJSON(FileContent, NameForErrors);

	TSet<FName> FrameNames;
	TArray<FSpriteFrame> ParsedFrames;

	UPaperFlipbook* Result = NULL;

	if (SpriteDescriptorObject.IsValid())
	{
		UTexture2D* ImportedTexture = NULL;

		// Validate the meta block
		TSharedPtr<FJsonObject> MetaBlock = FPaperJSONHelpers::ReadObject(SpriteDescriptorObject, TEXT("meta"));
		if (MetaBlock.IsValid())
		{
			// Example contents:
			//   "app": "Adobe Flash CS6",
			//   "version": "12.0.0.481",        (ignored)
			//   "image": "MySprite.png",
			//   "format": "RGBA8888",           (ignored)
			//   "size": {"w":2048,"h":2048},    (ignored)
			//   "scale": "1"                    (ignored)

			const FString AppName = FPaperJSONHelpers::ReadString(MetaBlock, TEXT("app"), TEXT(""));
			const FString Image = FPaperJSONHelpers::ReadString(MetaBlock, TEXT("image"), TEXT(""));
			
 			if (bLoadedSuccessfully)
			{
				const FString FlashPrefix(TEXT("Adobe Flash"));
				const FString TexturePackerPrefix(TEXT("http://www.codeandweb.com/texturepacker"));

				if (AppName.StartsWith(FlashPrefix) || AppName.StartsWith(TexturePackerPrefix))
				{
					// Cool, we (mostly) know how to handle these sorts of files!
					UE_LOG(LogPaperJsonImporter, Log, TEXT("Parsing sprite sheet exported from '%s'"), *AppName);
				}
				else if (!AppName.IsEmpty())
				{
					// It's got an app tag inside a meta block, so we'll take a crack at it
					UE_LOG(LogPaperJsonImporter, Warning, TEXT("Unexpected 'app' named '%s' while parsing sprite descriptor file '%s'.  Parsing will continue but the format may not be fully supported"), *AppName, *NameForErrors);
				}
				else
				{
					// Probably not a sprite sheet
					UE_LOG(LogPaperJsonImporter, Warning, TEXT("Failed to parse sprite descriptor file '%s'.  Expected 'app' key indicating the exporter (might not be a sprite sheet)"), *NameForErrors);
					bLoadedSuccessfully = false;
				}
 			}

			if (bLoadedSuccessfully)
			{
				if (Image.IsEmpty())
				{
					UE_LOG(LogPaperJsonImporter, Warning, TEXT("Failed to parse sprite descriptor file '%s'.  Expected valid 'image' tag"), *NameForErrors);
					bLoadedSuccessfully = false;
				}
				else
				{
					// Import the texture
					//@TODO: Avoid the first compression, since we're going to recompress
					const FString TargetSubPath = LongPackagePath + TEXT("/Textures");

					const FString SourceSheetImageFilename = FPaths::Combine(*CurrentSourcePath, *Image);
					TArray<FString> SheetFileNames;
					SheetFileNames.Add(SourceSheetImageFilename);

					TArray<UObject*> ImportedSheets = AssetToolsModule.Get().ImportAssets(SheetFileNames, TargetSubPath);

					ImportedTexture = (ImportedSheets.Num() > 0) ? Cast<UTexture2D>(ImportedSheets[0]) : NULL;

					if (ImportedTexture == NULL)
					{
						UE_LOG(LogPaperJsonImporter, Warning, TEXT("Failed to import sprite sheet image '%s'."), *SourceSheetImageFilename);
						bLoadedSuccessfully = false;
					}
					else
					{
						// Change the compression settings
						ImportedTexture->Modify();
						ImportedTexture->LODGroup = TEXTUREGROUP_UI;
						ImportedTexture->CompressionSettings = TC_EditorIcon;
						ImportedTexture->Filter = TF_Nearest;
						ImportedTexture->PostEditChange();
					}
				}
			}
		}
		else
		{
			// Missing meta tag
			UE_LOG(LogPaperJsonImporter, Warning, TEXT("Failed to parse sprite descriptor file '%s'.  Missing meta block"), *NameForErrors);
			bLoadedSuccessfully = false;
		}

		// Parse the frames
		if (bLoadedSuccessfully)
		{
			TSharedPtr<FJsonObject> ObjectFrameBlock = FPaperJSONHelpers::ReadObject(SpriteDescriptorObject, TEXT("frames"));
			if (ObjectFrameBlock.IsValid())
			{
				bLoadedSuccessfully = ParseFramesFromSpriteHash(ObjectFrameBlock, /*out*/ParsedFrames, /*inout*/FrameNames);
			}
			else
			{
				// Try loading as an array
				TArray<TSharedPtr<FJsonValue>> ArrayBlock = FPaperJSONHelpers::ReadArray(SpriteDescriptorObject, TEXT("frames"));
				if (ArrayBlock.Num() > 0)
				{
					bLoadedSuccessfully = ParseFramesFromSpriteArray(ArrayBlock, /*out*/ParsedFrames, /*inout*/FrameNames);
				}
				else
				{
					UE_LOG(LogPaperJsonImporter, Warning, TEXT("Failed to parse sprite descriptor file '%s'.  Missing frames block"), *NameForErrors);
				}
			}


			// Create an animated sprite that encompasses the frames
			UPaperFlipbook* Flipbook = NULL;
			if (ParsedFrames.Num() > 0)
			{
				Flipbook = NewNamedObject<UPaperFlipbook>(InParent, InName, Flags);
				Result = Flipbook;

				GWarn->BeginSlowTask(NSLOCTEXT("Paper2D", "PaperJsonImporterFactory_ImportingSprites", "Importing Sprite Frame"), true, true);
				FScopedFlipbookMutator EditLock(Flipbook);

				// Create objects for each successfully parsed frame
				const int32 FrameRun = 1; //@TODO: Don't make a keyframe for every single item if we can help it
				for (int32 FrameIndex = 0; FrameIndex < ParsedFrames.Num(); ++FrameIndex)
				{
					GWarn->StatusUpdate(FrameIndex, ParsedFrames.Num(), NSLOCTEXT("Paper2D", "PaperJsonImporterFactory_ImportingSprites", "Importing Sprite Frames"));

					// Check for user canceling the import
					if (GWarn->ReceivedUserCancel())
					{
						break;
					}

					const FSpriteFrame& Frame = ParsedFrames[FrameIndex];

					// Create a package for the frame
					const FString TargetSubPath = LongPackagePath + TEXT("/Frames");

					UObject* OuterForFrame = NULL; // @TODO: Use this if we don't want them to be individual assets - Flipbook;

					// Create a unique package name and asset name for the frame
					const FString TentativePackagePath = PackageTools::SanitizePackageName(TargetSubPath + TEXT("/") + Frame.FrameName.ToString());
					FString DefaultSuffix;
					FString AssetName;
					FString PackageName;
					AssetToolsModule.Get().CreateUniqueAssetName(TentativePackagePath, /*out*/ DefaultSuffix, /*out*/ PackageName, /*out*/ AssetName);

					// Create a package for the frame
					OuterForFrame = CreatePackage(NULL, *PackageName);

					// Create a frame in the package
					UPaperSprite* TargetSprite = NewNamedObject<UPaperSprite>(OuterForFrame, *AssetName, Flags);
					FAssetRegistryModule::AssetCreated(TargetSprite);

					TargetSprite->Modify();

					// TargetSprite->InitializeSprite(ImportedTexture, Frame.SpritePosInSheet, Frame.SpriteSizeInSheet, GetDefault<UPaperRuntimeSettings>()->DefaultPixelsPerUnrealUnit, Frame.bRotated);
					TargetSprite->InitializeSprite(ImportedTexture, Frame.SpritePosInSheet, Frame.SpriteSizeInSheet, GetDefault<UPaperRuntimeSettings>()->DefaultPixelsPerUnrealUnit);

					//@TODO: Need to support pivot behavior - Total guess at pivot behavior
					//const FVector2D SizeDifference = Frame.SpriteSourceSize - Frame.SpriteSizeInSheet;
					//TargetSprite->SpriteData.Destination = FVector(SizeDifference.X * -0.5f, 0.0f, SizeDifference.Y * -0.5f);

					// Create the entry in the animation
					FPaperFlipbookKeyFrame* DestFrame = new (EditLock.KeyFrames) FPaperFlipbookKeyFrame();
					DestFrame->Sprite = TargetSprite;
					DestFrame->FrameRun = FrameRun;

					TargetSprite->PostEditChange();
				}

				GWarn->EndSlowTask();
			}

		}
	}
	else
	{
		// Failed to parse the JSON
		bLoadedSuccessfully = false;
	}

	FEditorDelegates::OnAssetPostImport.Broadcast(this, Result);

	return Result;
}

TSharedPtr<FJsonObject> UPaperJsonImporterFactory::ParseJSON(const FString& FileContents, const FString& NameForErrors)
{
	// Load the file up (JSON format)

	if (!FileContents.IsEmpty())
	{
		const TSharedRef< TJsonReader<> >& Reader = TJsonReaderFactory<>::Create(FileContents);

		TSharedPtr<FJsonObject> SpriteDescriptorObject;
		if (FJsonSerializer::Deserialize(Reader, SpriteDescriptorObject) && SpriteDescriptorObject.IsValid())
		{
			// File was loaded and deserialized OK!
			return SpriteDescriptorObject;
		}
		else
		{
			UE_LOG(LogPaperJsonImporter, Warning, TEXT("Failed to parse sprite descriptor file '%s'.  Error: '%s'"), *NameForErrors, *Reader->GetErrorMessage());
			return NULL;
		}
	}
	else
	{
		UE_LOG(LogPaperJsonImporter, Warning, TEXT("Sprite descriptor file '%s' was empty.  This sprite cannot be imported."), *NameForErrors);
		return NULL;
	}
}
