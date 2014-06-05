// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealSync.h"

#include "P4Env.h"
#include "ProcessHelper.h"

FString FP4Env::GetParam(const TCHAR* CommandLine, const FString& ParamName)
{
	FString Value;
	if (FParse::Value(CommandLine, *(ParamName + "="), Value))
	{
		return Value;
	}

	TCHAR Buf[512];

	FPlatformMisc::GetEnvironmentVariable(*ParamName, Buf, 512);

	return Buf;
}

FP4Env::FP4Env(const TCHAR* CommandLine)
{
	class FParsingParamSerialization
	{
	public:
		void SerializationTask(FString& FieldReference, EP4ParamType::Type Type)
		{
			FieldReference = GetParam(CommandLine, GetParamName(Type));
		}

		const TCHAR* CommandLine;
	};

	FParsingParamSerialization PS;
	PS.CommandLine = CommandLine;

	SerializeParams(FSerializationTask::CreateRaw(&PS, &FParsingParamSerialization::SerializationTask));
}

const FString& FP4Env::GetClient() const
{
	return Client;
}

const FString& FP4Env::GetPort() const
{
	return Port;
}

const FString& FP4Env::GetUser() const
{
	return User;
}

/**
 * Gets known path in the depot (arbitrarily chosen).
 *
 * @returns Known path.
 */
FString GetKnownPath()
{
	return FPaths::ConvertRelativePathToFull(
		FPaths::Combine(*FPaths::EngineConfigDir(), TEXT("BaseEngine.ini")));
}

/**
 * Gets computer host name.
 *
 * @returns Host name.
 */
FString GetHostName()
{
	return FPlatformProcess::ComputerName();
}

const FString& FP4Env::GetBranch() const
{
	return Branch;
}

FP4Env& FP4Env::Get()
{
	return *Env;
}

/**
 * Function that tries to detect P4 executable path.
 *
 * @param OutPath Output param. Found P4 executable path.
 * @param Env P4 environment.
 *
 * @returns True if succeeded. False otherwise.
 */
bool DetectPath(FString& OutPath, const FP4Env& Env)
{
	// Tries to detect in standard environment paths.
	
	FString WhereCommand = "where"; // TODO Mac: I think 'where' command equivalent on Mac is 'whereis'
	FString P4ExecutableName = "p4.exe";
	FString Output;
	
	if (RunProcessOutput(WhereCommand, P4ExecutableName, Output))
	{
		// P4 found, quick exit.
		OutPath = FPaths::ConvertRelativePathToFull(
			Output.Replace(TEXT("\n"), TEXT("")).Replace(TEXT("\r"), TEXT("")));
		return true;
	}
	
	TArray<FString> PossibleLocations;

	PossibleLocations.Add("C:\\Program Files\\Perforce");
	PossibleLocations.Add("C:\\Program Files (x86)\\Perforce");

	for (auto PossibleLocation : PossibleLocations)
	{
		FString LocationCandidate = FPaths::ConvertRelativePathToFull(FPaths::Combine(*PossibleLocation, *P4ExecutableName));
		if (FPaths::FileExists(LocationCandidate))
		{
			OutPath = LocationCandidate;
			return true;
		}
	}

	return false;
}

#include "XmlParser.h"

/**
 * Tries to get last connection string from P4V config file.
 *
 * @param Output Output param.
 * @param LastConnectionStringId Which string to output.
 *
 * @returns True if succeeded, false otherwise.
 */
bool GetP4VLastConnectionStringElement(FString& Output, int32 LastConnectionStringId)
{
	class FLastConnectionStringCache
	{
	public:
		FLastConnectionStringCache()
		{
			bFoundData = false;
			// TODO Mac: This path is going to be different on Mac.
			FString AppSettingsXmlPath = FPaths::ConvertRelativePathToFull(
					FPaths::Combine(FPlatformProcess::UserDir(),
						TEXT(".."), TEXT(".p4qt"),
						TEXT("ApplicationSettings.xml")
					)
				);

			if (!FPaths::FileExists(AppSettingsXmlPath))
			{
				return;
			}

			TSharedPtr<FXmlFile> Doc = MakeShareable(new FXmlFile(AppSettingsXmlPath, EConstructMethod::ConstructFromFile));

			for (FXmlNode* PropertyListNode : Doc->GetRootNode()->GetChildrenNodes())
			{
				if (PropertyListNode->GetTag() != "PropertyList" || PropertyListNode->GetAttribute("varName") != "Connection")
				{
					continue;
				}

				for (FXmlNode* VarNode : PropertyListNode->GetChildrenNodes())
				{
					if (VarNode->GetTag() != "String" || VarNode->GetAttribute("varName") != "LastConnection")
					{
						continue;
					}

					bFoundData = true;

					FString Content = VarNode->GetContent();
					FString Current;
					FString Rest;

					while (Content.Split(",", &Current, &Rest))
					{
						Current.Trim();
						Current.TrimTrailing();

						Data.Add(Current);

						Content = Rest;
					}

					Rest.Trim();
					Rest.TrimTrailing();

					Data.Add(Rest);
				}
			}
		}

		bool bFoundData;
		TArray<FString> Data;
	};

	static FLastConnectionStringCache Cache;

	if (!Cache.bFoundData || Cache.Data.Num() <= LastConnectionStringId)
	{
		return false;
	}

	Output = Cache.Data[LastConnectionStringId];
	return true;
}

/**
 * Function that tries to detect P4 port.
 *
 * @param OutPort Output param. Found P4 port.
 * @param Env P4 environment.
 *
 * @returns True if succeeded. False otherwise.
 */
bool DetectPort(FString& OutPort, const FP4Env& Env)
{
	if (GetP4VLastConnectionStringElement(OutPort, 0))
	{
		return true;
	}

	// Fallback to default port. If it's not valid auto-detection will fail later.
	OutPort = "perforce:1666";
	return true;
}

/**
 * Function that tries to detect P4 user name.
 *
 * @param OutUser Output param. Found P4 user name.
 * @param Env P4 environment.
 *
 * @returns True if succeeded. False otherwise.
 */
bool DetectUser(FString& OutUser, const FP4Env& Env)
{
	if (GetP4VLastConnectionStringElement(OutUser, 1))
	{
		return true;
	}

	FString Output;
	RunProcessOutput(Env.GetPath(), FString::Printf(TEXT("-p%s info"), *Env.GetPort()), Output);

	const FRegexPattern UserNamePattern(TEXT("User name:\\s*([^ \\t\\n\\r]+)\\s*"));

	FRegexMatcher Matcher(UserNamePattern, Output);
	if (Matcher.FindNext())
	{
		OutUser = Output.Mid(Matcher.GetCaptureGroupBeginning(1), Matcher.GetCaptureGroupEnding(1) - Matcher.GetCaptureGroupBeginning(1));
		return true;
	}

	return false;
}

#include "Internationalization/Regex.h"

/**
 * Function that tries to detect P4 workspace name.
 *
 * @param OutClient Output param. Found P4 workspace name.
 * @param Env P4 environment.
 *
 * @returns True if succeeded. False otherwise.
 */
bool DetectClient(FString& OutClient, const FP4Env& Env)
{
	FString Output;
	if (!RunProcessOutput(Env.GetPath(), FString::Printf(TEXT("-p%s clients -u%s"), *Env.GetPort(), *Env.GetUser()), Output))
	{
		return false;
	}

	FString KnownPath = GetKnownPath();
	FString HostName = GetHostName();

	const FRegexPattern ClientsPattern(TEXT("Client ([^ ]+) \\d{4}/\\d{2}/\\d{2} root (.+) '.*'"));
	FRegexMatcher Matcher(ClientsPattern, Output);

	while (Matcher.FindNext())
	{
		auto ClientName = Output.Mid(Matcher.GetCaptureGroupBeginning(1), Matcher.GetCaptureGroupEnding(1) - Matcher.GetCaptureGroupBeginning(1));
		auto Root = Output.Mid(Matcher.GetCaptureGroupBeginning(2), Matcher.GetCaptureGroupEnding(2) - Matcher.GetCaptureGroupBeginning(2));

		if (KnownPath.StartsWith(FPaths::ConvertRelativePathToFull(Root)))
		{
			FString InfoOutput;
			if (!RunProcessOutput(Env.GetPath(), FString::Printf(TEXT("-p%s -u%s -c%s info"), *Env.GetPort(), *Env.GetUser(), *ClientName), InfoOutput))
			{
				return false;
			}

			const FRegexPattern InfoPattern(TEXT("Client host: ([^\\r\\n ]+)\\s"));
			FRegexMatcher InfoMatcher(InfoPattern, InfoOutput);

			if (InfoMatcher.FindNext())
			{
				if (InfoOutput.Mid(
					InfoMatcher.GetCaptureGroupBeginning(1),
					InfoMatcher.GetCaptureGroupEnding(1) - InfoMatcher.GetCaptureGroupBeginning(1)
					).Equals(HostName))
				{
					OutClient = ClientName;
					return true;
				}
			}
		}
	}

	return false;
}

/**
 * Tries to detect branch using known depot path.
 *
 * @param OutBranch Output parameter. Detected branch.
 * @param Env P4 environment.
 *
 * @returns True if succeeded. False otherwise.
 */
bool DetectBranch(FString& OutBranch, const FP4Env& Env)
{
	FString Output;
	if (!RunProcessOutput(Env.GetPath(), FString::Printf(TEXT("-p%s -u%s -c%s files %s"),
		*Env.GetPort(), *Env.GetUser(), *Env.GetClient(), *GetKnownPath()), Output))
	{
		return false;
	}

	FString Rest;

	if (!Output.Split("/Engine/", &OutBranch, &Rest))
	{
		return false;
	}

	return true;
}

bool FP4Env::Init(const TCHAR* CommandLine)
{
	TSharedPtr<FP4Env> Env = MakeShareable(new FP4Env(CommandLine));

	if (!Env->AutoDetectMissingParams())
	{
		return false;
	}

	FP4Env::Env = Env;

	return true;
}

bool FP4Env::RunP4Progress(const FString& CommandLine, const FOnP4MadeProgress& OnP4MadeProgress)
{
	return RunProcessProgress(Get().GetPath(), FString::Printf(TEXT("-p%s -c%s -u%s %s"), *Get().GetPort(), *Get().GetClient(), *Get().GetUser(), *CommandLine), OnP4MadeProgress);
}

bool FP4Env::RunP4Output(const FString& CommandLine, FString& Output)
{
	class FOutputCollector
	{
	public:
		bool Progress(const FString& Chunk)
		{
			Output += Chunk;

			return true;
		}

		FString Output;
	};

	FOutputCollector OC;

	if (!RunP4Progress(CommandLine, FOnP4MadeProgress::CreateRaw(&OC, &FOutputCollector::Progress)))
	{
		return false;
	}

	Output = OC.Output;
	return true;
}

bool FP4Env::RunP4(const FString& CommandLine)
{
	return RunP4Progress(CommandLine, nullptr);
}

void FP4Env::SerializeParams(const FSerializationTask& SerializationTask)
{
	SerializationTask.ExecuteIfBound(Path, EP4ParamType::Path);
	SerializationTask.ExecuteIfBound(Port, EP4ParamType::Port);
	SerializationTask.ExecuteIfBound(User, EP4ParamType::User);
	SerializationTask.ExecuteIfBound(Client, EP4ParamType::Client);
	SerializationTask.ExecuteIfBound(Branch, EP4ParamType::Branch);
}

FString FP4Env::GetParamName(EP4ParamType::Type Type)
{
	switch (Type)
	{
	case EP4ParamType::Path:
		return "P4PATH";
	case EP4ParamType::Port:
		return "P4PORT";
	case EP4ParamType::User:
		return "P4USER";
	case EP4ParamType::Client:
		return "P4CLIENT";
	case EP4ParamType::Branch:
		return "P4BRANCH";
	}

	checkNoEntry();
	return "";
}

bool FP4Env::AutoDetectMissingParams()
{
	class FAutoDetectTasks
	{
	public:
		void AutoDetectParam(FString& FieldReference, EP4ParamType::Type Type)
		{
			// If parameter already found then stop.
			if (!FieldReference.IsEmpty())
			{
				return;
			}

			// If previous detection failed then stop.
			if (Stop)
			{
				return;
			}

			switch (Type)
			{
			case EP4ParamType::Path:
				Stop = !DetectPath(FieldReference, Env);
				break;
			case EP4ParamType::Port:
				Stop = !DetectPort(FieldReference, Env);
				break;
			case EP4ParamType::User:
				Stop = !DetectUser(FieldReference, Env);
				break;
			case EP4ParamType::Client:
				Stop = !DetectClient(FieldReference, Env);
				break;
			case EP4ParamType::Branch:
				Stop = !DetectBranch(FieldReference, Env);
				break;
			}
		}

		FAutoDetectTasks(FP4Env& Env)
			: Env(Env)
		{
			Stop = false;
		}

		FP4Env& Env;
		bool Stop;
	};

	FAutoDetectTasks AT(*this);
	SerializeParams(FSerializationTask::CreateRaw(&AT, &FAutoDetectTasks::AutoDetectParam));

	return !AT.Stop;
}

FString FP4Env::GetCommandLine()
{
	class CommandLineOutput
	{
	public:
		void AppendParam(FString& FieldReference, EP4ParamType::Type Type)
		{
			Output += " -" + GetParamName(Type) + "=\"" + FieldReference + "\"";
		}

		FString Output;
	};

	CommandLineOutput Output;

	SerializeParams(FSerializationTask::CreateRaw(&Output, &CommandLineOutput::AppendParam));

	return Output.Output;
}

const FString& FP4Env::GetPath() const
{
	return Path;
}

/* Static variable initialization. */
TSharedPtr<FP4Env> FP4Env::Env;