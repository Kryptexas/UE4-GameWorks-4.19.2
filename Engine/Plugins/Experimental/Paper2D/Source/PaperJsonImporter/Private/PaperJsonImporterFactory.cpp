// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PaperJsonImporterPrivatePCH.h"
#include "Paper2DClasses.h"
#include "Json.h"
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

	Formats.Add(TEXT("json;JSON file"));
}

FText UPaperJsonImporterFactory::GetToolTip() const
{
	return NSLOCTEXT("Paper2D", "PaperJsonImporterFactoryDescription", "Sprite sheets exported from Adobe Flash");
}

UObject* UPaperJsonImporterFactory::FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn)
{
	FEditorDelegates::OnAssetPreImport.Broadcast(this, InClass, InParent, InName, Type);

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
		TSharedPtr<FJsonObject> MetaBlock = ReadObject(SpriteDescriptorObject, TEXT("meta"));
		if (MetaBlock.IsValid())
		{
			// Example contents:
			//   "app": "Adobe Flash CS6",
			//   "version": "12.0.0.481",        (ignored)
			//   "image": "MySprite.png",
			//   "format": "RGBA8888",           (ignored)
			//   "size": {"w":2048,"h":2048},    (ignored)
			//   "scale": "1"                    (ignored)

			const FString AppName = ReadString(MetaBlock, TEXT("app"), TEXT(""));
			const FString Image = ReadString(MetaBlock, TEXT("image"), TEXT(""));
			
			const FString FlashPrefix(TEXT("Adobe Flash"));
			if (bLoadedSuccessfully && !AppName.StartsWith(FlashPrefix))
			{
				UE_LOG( LogPaperJsonImporter, Warning, TEXT( "Failed to parse sprite descriptor file '%s'.  Expected 'app' to start with %s" ), *NameForErrors, *FlashPrefix );
				bLoadedSuccessfully = false;
			}

			if (bLoadedSuccessfully)
			{
				if (Image.IsEmpty())
				{
					UE_LOG( LogPaperJsonImporter, Warning, TEXT( "Failed to parse sprite descriptor file '%s'.  Expected valid 'image' tag" ), *NameForErrors );
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

					FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
					TArray<UObject*> ImportedSheets = AssetToolsModule.Get().ImportAssets(SheetFileNames, TargetSubPath);

					ImportedTexture = (ImportedSheets.Num() > 0) ? Cast<UTexture2D>(ImportedSheets[0]) : NULL;

					if (ImportedTexture == NULL)
					{
						UE_LOG( LogPaperJsonImporter, Warning, TEXT( "Failed to import sprite sheet image '%s'." ), *SourceSheetImageFilename );
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
			UE_LOG( LogPaperJsonImporter, Warning, TEXT( "Failed to parse sprite descriptor file '%s'.  Missing meta block" ), *NameForErrors );
			bLoadedSuccessfully = false;
		}

		// Parse the frames
		if (bLoadedSuccessfully)
		{
			TSharedPtr<FJsonObject> FramesBlock = ReadObject(SpriteDescriptorObject, TEXT("frames"));
			if (FramesBlock.IsValid())
			{
				// Parse all of the frames
				for (auto FrameIt = FramesBlock->Values.CreateIterator(); FrameIt; ++FrameIt)
				{
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

						// An example frame:
						//   "frame": {"x":210,"y":10,"w":190,"h":223},
						//   "rotated": false,
						//   "trimmed": true,
						//   "spriteSourceSize": {"x":0,"y":11,"w":216,"h":240},
						//   "sourceSize": {"w":216,"h":240}

						Frame.bRotated = ReadBoolean(FrameData, TEXT("rotated"), false);
						Frame.bTrimmed = ReadBoolean(FrameData, TEXT("trimmed"), false);

						if (Frame.bRotated || !Frame.bTrimmed)
						{
							//@TODO: Should learn how to support this combo
							UE_LOG( LogPaperJsonImporter, Warning, TEXT( "Frame %s is either rotated or not trimmed, which may not be handled correctly" ), *Frame.FrameName.ToString() );
						}

						bReadFrameSuccessfully = bReadFrameSuccessfully && ReadRectangle(FrameData, "frame", /*out*/ Frame.SpritePosInSheet, /*out*/ Frame.SpriteSizeInSheet);

						if (Frame.bTrimmed)
						{
							bReadFrameSuccessfully = bReadFrameSuccessfully && ReadRectangle(FrameData, "spriteSourceSize", /*out*/ Frame.SpriteSourcePos, /*out*/ Frame.SpriteSourceSize);
						}
						else
						{
							Frame.SpriteSourcePos = FVector2D::ZeroVector;
							Frame.SpriteSourceSize = Frame.SpriteSizeInSheet;
						}
					}
					else
					{
						bReadFrameSuccessfully = false;
					}

					if (bReadFrameSuccessfully)
					{
						ParsedFrames.Add(Frame);
					}
					else
					{
						UE_LOG( LogPaperJsonImporter, Warning, TEXT( "Frame %s is in an unexpected format" ), *Frame.FrameName.ToString() );
						bLoadedSuccessfully = false;
					}
				}

				// Create an animated sprite that encompasses the frames
				UPaperFlipbook* Flipbook = NULL;
				if (ParsedFrames.Num() > 0)
				{
					Flipbook = NewNamedObject<UPaperFlipbook>(InParent, InName, Flags);
					Result = Flipbook;
				}

				// Create objects for each successfully parsed frame
				const int32 FrameRun = 1; //@TODO: Don't make a keyframe for every single item if we can help it
				for (int32 FrameIndex = 0; FrameIndex < ParsedFrames.Num(); ++FrameIndex)
				{
					const FSpriteFrame& Frame = ParsedFrames[FrameIndex];

					// Create a package for the frame
					const FString TargetSubPath = LongPackagePath + TEXT("/Frames");

					UObject* OuterForFrame = NULL; // @TODO: Use this if we don't want them to be individual assets - Flipbook;

					// Create a package for each frame
					FString NewPackageName = TargetSubPath + TEXT("/") + Frame.FrameName.ToString();
					NewPackageName = PackageTools::SanitizePackageName(NewPackageName);
					OuterForFrame = CreatePackage(NULL, *NewPackageName);

					// Create a frame in the package
					UPaperSprite* TargetSprite = FindObject<UPaperSprite>(OuterForFrame, *Frame.FrameName.ToString());
					if (TargetSprite == NULL)
					{
						TargetSprite = NewNamedObject<UPaperSprite>(OuterForFrame, Frame.FrameName, Flags);
						FAssetRegistryModule::AssetCreated(TargetSprite);
					}
					TargetSprite->Modify();

					TargetSprite->InitializeSprite(ImportedTexture, Frame.SpritePosInSheet, Frame.SpriteSizeInSheet);

					//@TODO: Need to support pivot behavior - Total guess at pivot behavior
					//const FVector2D SizeDifference = Frame.SpriteSourceSize - Frame.SpriteSizeInSheet;
					//TargetSprite->SpriteData.Destination = FVector(SizeDifference.X * -0.5f, 0.0f, SizeDifference.Y * -0.5f);

					// Create the entry in the animation
					FPaperFlipbookKeyFrame* DestFrame = new (Flipbook->KeyFrames) FPaperFlipbookKeyFrame();
					DestFrame->Sprite = TargetSprite;
					DestFrame->FrameRun = FrameRun;

					TargetSprite->PostEditChange();
				}
			}
			else
			{
				UE_LOG( LogPaperJsonImporter, Warning, TEXT( "Failed to parse sprite descriptor file '%s'.  Missing frames block" ), *NameForErrors );
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
			UE_LOG( LogPaperJsonImporter, Warning, TEXT( "Failed to parse sprite descriptor file '%s'.  Error: '%s'" ), *NameForErrors, *Reader->GetErrorMessage() );
			return NULL;
		}
	}
	else
	{
		UE_LOG( LogPaperJsonImporter, Warning, TEXT( "Sprite descriptor file '%s' was empty.  This sprite cannot be imported." ), *NameForErrors );
		return NULL;
	}
}

// returns the value of Key in Item, or DefaultValue if the Key is missing or the wrong type
FString UPaperJsonImporterFactory::ReadString(TSharedPtr<class FJsonObject> Item, const FString& Key, const FString& DefaultValue)
{
	const TSharedPtr<FJsonValue>& ValuePtr = Item->GetField<EJson::String>(Key);
	return ValuePtr.IsValid() ? ValuePtr->AsString() : DefaultValue;
}

// Returns the object named Key or NULL if it is missing or the wrong type
TSharedPtr<class FJsonObject> UPaperJsonImporterFactory::ReadObject(TSharedPtr<class FJsonObject> Item, const FString& Key)
{
	if (Item->HasTypedField<EJson::Object>(Key))
	{
		return Item->GetObjectField(Key);
	}
	else
	{
		return NULL;
	}
}

// Returns the bool named Key or bDefaultIfMissing if it is missing or the wrong type (note: no way to determine errors!)
bool UPaperJsonImporterFactory::ReadBoolean(const TSharedPtr<class FJsonObject> Item, const FString& Key, bool bDefaultIfMissing)
{
	if (Item->HasTypedField<EJson::Boolean>(Key))
	{
		return Item->GetBoolField(Key);
	}
	else
	{
		return bDefaultIfMissing;
	}
}

bool UPaperJsonImporterFactory::ReadFloatNoDefault(const TSharedPtr<class FJsonObject> Item, const FString& Key, float& Out_Value)
{
	if (Item->HasTypedField<EJson::Number>(Key))
	{
		Out_Value = Item->GetNumberField(Key);
		return true;
	}
	else
	{
		Out_Value = 0.0f;
		return false;
	}
}

// Returns true if the object named Key is a struct containing four floats (x,y,w,h), populating XY and WH with the values)
bool UPaperJsonImporterFactory::ReadRectangle(const TSharedPtr<class FJsonObject> Item, const FString& Key, FVector2D& Out_XY, FVector2D& Out_WH)
{
	const TSharedPtr<FJsonObject> Struct = ReadObject(Item, Key);
	if (Struct.IsValid())
	{
		if (ReadFloatNoDefault(Struct, TEXT("x"), /*out*/ Out_XY.X) &&
			ReadFloatNoDefault(Struct, TEXT("y"), /*out*/ Out_XY.Y) &&
			ReadFloatNoDefault(Struct, TEXT("w"), /*out*/ Out_WH.X) &&
			ReadFloatNoDefault(Struct, TEXT("h"), /*out*/ Out_WH.Y))
		{
			return true;
		}
		else
		{
			Out_XY = FVector2D::ZeroVector;
			Out_WH = FVector2D::ZeroVector;
			return false;
		}
	}
	else
	{
		return false;
	}
}

// Returns true if the object named Key is a struct containing two floats (w,h), populating WH with the values)
bool UPaperJsonImporterFactory::ReadSize(const TSharedPtr<class FJsonObject> Item, const FString& Key, FVector2D& Out_WH)
{
	const TSharedPtr<FJsonObject> Struct = ReadObject(Item, Key);
	if (Struct.IsValid())
	{
		if (ReadFloatNoDefault(Struct, TEXT("w"), /*out*/ Out_WH.X) &&
			ReadFloatNoDefault(Struct, TEXT("h"), /*out*/ Out_WH.Y))
		{
			return true;
		}
		else
		{
			Out_WH = FVector2D::ZeroVector;
			return false;
		}
	}
	else
	{
		return false;
	}
}
