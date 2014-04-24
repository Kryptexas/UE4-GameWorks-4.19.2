#include "UnrealVersionSelector.h"
#include "FileAssociation.h"
#include "PlatformInstallation.h"
#include "Runtime/Core/Public/Serialization/Json/Json.h"

TSharedPtr<FJsonObject> LoadJson(const FString &FileName)
{
	FString FileContents;

	if (!FFileHelper::LoadFileToString(FileContents, *FileName))
	{
		return TSharedPtr<FJsonObject>(NULL);
	}

	TSharedPtr< FJsonObject > JsonObject;
	TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create(FileContents);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return TSharedPtr<FJsonObject>(NULL);
	}

	return JsonObject;
}

bool SaveJson(const FString &FileName, TSharedPtr<FJsonObject> Object)
{
	FString FileContents;

	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&FileContents);
	if (!FJsonSerializer::Serialize(Object.ToSharedRef(), Writer))
	{
		return false;
	}

	if (!FFileHelper::SaveStringToFile(FileContents, *FileName))
	{
		return false;
	}

	return true;
}

bool SetEngineIdForProject(const FString &ProjectFileName, const FString &Id)
{
	bool bRes = false;

	TSharedPtr<FJsonObject> Object = LoadJson(ProjectFileName);
	if (Object.IsValid())
	{
		Object->SetStringField(L"EngineAssociation", Id);
		bRes = SaveJson(ProjectFileName, Object);
	}

	return bRes;
}

bool GetEngineIdForProject(const FString &ProjectFileName, FString &OutId)
{
	bool bRes = false;

	TSharedPtr<FJsonObject> Object = LoadJson(ProjectFileName);
	if (Object.IsValid())
	{
		OutId = Object->GetStringField(L"EngineAssociation");
		bRes = (OutId.Len() > 0);
	}

	return bRes;
}

bool GetEngineRootDirForProject(const FString &ProjectFileName, FString &OutRootDir)
{
	FString Id;
	if (GetEngineIdForProject(ProjectFileName, Id) && GetEngineRootDirFromId(Id, OutRootDir))
	{
		return true;
	}
	else
	{
		return GetDefaultEngineRootDir(OutRootDir);
	}
}
