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

FP4Env::FP4Env(const FString& Port, const FString& User, const FString& Client, const FString& Branch)
	: Port(Port), User(User), Client(Client), Branch(Branch)
{

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

#include "Internationalization/Regex.h"

/**
 * Function that tries to detect P4 workspace name given port, user
 * using known path.
 *
 * @param OutClient Output param. Found P4 workspace name.
 * @param Port P4 port.
 * @param User P4 user.
 *
 * @returns True if succeeded. False otherwise.
 */
bool DetectClient(FString& OutClient, const FString& Port, const FString& User)
{
	FString Output;
	if (!RunProcessOutput(TEXT("p4.exe"), FString::Printf(TEXT("-p%s clients -u%s"), *Port, *User), Output))
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
			if (!RunProcessOutput(TEXT("p4.exe"), FString::Printf(TEXT("-p%s -u%s -c%s info"), *Port, *User, *ClientName), InfoOutput))
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
 * Tries to detect branch from P4 port, user and workspace name
 * using known depot path.
 *
 * @param OutBranch Output parameter. Detected branch.
 * @param Port P4 port.
 * @param User P4 user.
 * @param Client P4 workspace name.
 *
 * @returns True if succeeded. False otherwise.
 */
bool DetectBranch(FString& OutBranch, const FString& Port, const FString& User, const FString& Client)
{
	FString Output;
	if (!RunProcessOutput(TEXT("p4.exe"), FString::Printf(TEXT("-p%s -u%s -c%s files %s"), *Port, *User, *Client, *GetKnownPath()), Output))
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

bool FP4Env::Init(const FString& Port, const FString& User, const FString& Client)
{
	FString TmpClient;
	FString Branch;

	if (Client.IsEmpty())
	{
		if (!DetectClient(TmpClient, Port, User))
		{
			return false;
		}
	}
	else
	{
		TmpClient = Client;
	}

	if (!DetectBranch(Branch, Port, User, TmpClient))
	{
		return false;
	}

	Env = MakeShareable(new FP4Env(Port, User, TmpClient, Branch));
	return true;
}

bool FP4Env::Init(const TCHAR* CommandLine)
{
	// Auto-detect P4.
	return Init(GetParam(CommandLine, TEXT("P4PORT")), GetParam(CommandLine, TEXT("P4USER")), GetParam(CommandLine, TEXT("US_FOUND_P4CLIENT")));
}

bool FP4Env::RunP4Progress(const FString& CommandLine, const FOnP4MadeProgress& OnP4MadeProgress)
{
	return RunProcessProgress(TEXT("p4.exe"), FString::Printf(TEXT("-p%s -c%s -u%s %s"), *Get().GetPort(), *Get().GetClient(), *Get().GetUser(), *CommandLine), OnP4MadeProgress);
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

/* Static variable initialization. */
TSharedPtr<FP4Env> FP4Env::Env;