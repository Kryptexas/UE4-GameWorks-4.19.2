// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
// ..

#include "CoreMinimal.h"
#include "MetalShaderFormat.h"
#include "ShaderCore.h"
#include "MetalShaderResources.h"
#include "ShaderCompilerCommon.h"
#include "Misc/ConfigCacheIni.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if PLATFORM_WINDOWS
#include "WindowsHWrapper.h"
#include "AllowWindowsPlatformTypes.h"
THIRD_PARTY_INCLUDES_START
	#include "Windows/PreWindowsApi.h"
	#include <objbase.h>
	#include <assert.h>
	#include <stdio.h>
	#include "Windows/PostWindowsApi.h"
	#include "Windows/MinWindows.h"
THIRD_PARTY_INCLUDES_END
#include "HideWindowsPlatformTypes.h"
#endif

#include "ShaderPreprocessor.h"
#include "hlslcc.h"
#include "MetalBackend.h"

DEFINE_LOG_CATEGORY_STATIC(LogMetalShaderCompiler, Log, All); 

static FString	GRemoteBuildServerHost;
static FString	GRemoteBuildServerUser;
static FString	GRemoteBuildServerSSHKey;
static FString	GSSHPath;
static FString	GRSyncPath;
static FString	GMetalToolsPath[2];
static FString	GMetalBinaryPath[2];
static FString	GTempFolderPath;
static bool		GMetalLoggedRemoteCompileNotConfigured;	// This is used to reduce log spam, its not perfect because there is not a place to reset this flag so a log msg will only be given once per editor run

static bool IsRemoteBuildingConfigured()
{
	bool	remoteCompilingEnabled = false;
	GConfig->GetBool(TEXT("/Script/IOSRuntimeSettings.IOSRuntimeSettings"), TEXT("EnableRemoteShaderCompile"), remoteCompilingEnabled, GEngineIni);
	if(!remoteCompilingEnabled)
	{
		return false;
	}
	
	GRemoteBuildServerHost = "";
	GConfig->GetString(TEXT("/Script/IOSRuntimeSettings.IOSRuntimeSettings"), TEXT("RemoteServerName"), GRemoteBuildServerHost, GEngineIni);
	if(GRemoteBuildServerHost.Len() == 0)
	{
		if(!GMetalLoggedRemoteCompileNotConfigured)
		{
			UE_LOG(LogMetalShaderCompiler, Warning, TEXT("Remote Building is not configured: RemoteServerName is not set."));
			GMetalLoggedRemoteCompileNotConfigured = true;
		}
		return false;
	}

	GRemoteBuildServerUser = "";
	GConfig->GetString(TEXT("/Script/IOSRuntimeSettings.IOSRuntimeSettings"), TEXT("RSyncUsername"), GRemoteBuildServerUser, GEngineIni);

	if(GRemoteBuildServerUser.Len() == 0)
	{
		if(!GMetalLoggedRemoteCompileNotConfigured)
		{
			UE_LOG(LogMetalShaderCompiler, Warning, TEXT("Remote Building is not configured: RSyncUsername is not set."));
			GMetalLoggedRemoteCompileNotConfigured = true;
		}
		return false;
	}

	GRemoteBuildServerSSHKey = "";
	GConfig->GetString(TEXT("/Script/IOSRuntimeSettings.IOSRuntimeSettings"), TEXT("SSHPrivateKeyOverridePath"), GRemoteBuildServerSSHKey, GEngineIni);

	if(GRemoteBuildServerSSHKey.Len() == 0)
	{
		// RemoteToolChain.cs in UBT looks in a few more places but the code in FIOSTargetSettingsCustomization::OnGenerateSSHKey() only puts the key in this location so just going with that to keep things simple
		TCHAR Path[4096];
		FPlatformMisc::GetEnvironmentVariable(TEXT("APPDATA"), Path, ARRAY_COUNT(Path));
		GRemoteBuildServerSSHKey = FString::Printf(TEXT("%s\\Unreal Engine\\UnrealBuildTool\\SSHKeys\\%s\\%s\\RemoteToolChainPrivate.key"), Path, *GRemoteBuildServerHost, *GRemoteBuildServerUser);
	}

	if(!FPaths::FileExists(GRemoteBuildServerSSHKey))
	{
		if(!GMetalLoggedRemoteCompileNotConfigured)
		{
			UE_LOG(LogMetalShaderCompiler, Warning, TEXT("Remote Building is not configured: SSH private key was not found."));
			GMetalLoggedRemoteCompileNotConfigured = true;
		}
		return false;
	}

	FString DeltaCopyPath;
	GConfig->GetString(TEXT("/Script/IOSRuntimeSettings.IOSRuntimeSettings"), TEXT("DeltaCopyInstallPath"), DeltaCopyPath, GEngineIni);
	if (DeltaCopyPath.IsEmpty() || !FPaths::DirectoryExists(DeltaCopyPath))
	{
		// If no user specified directory try the UE4 bundled directory
		DeltaCopyPath = FPaths::ConvertRelativePathToFull(FPaths::EngineDir() / TEXT("Extras\\ThirdPartyNotUE\\DeltaCopy\\Binaries"));
	}

	if (!FPaths::DirectoryExists(DeltaCopyPath))
	{
		// if no UE4 bundled version of DeltaCopy, try and use the default install location
		TCHAR ProgramPath[4096];
		FPlatformMisc::GetEnvironmentVariable(TEXT("PROGRAMFILES(X86)"), ProgramPath, ARRAY_COUNT(ProgramPath));
		DeltaCopyPath = FPaths::Combine(ProgramPath, TEXT("DeltaCopy"));
	}
	
	if (!FPaths::DirectoryExists(DeltaCopyPath))
	{
		if(!GMetalLoggedRemoteCompileNotConfigured)
		{
			UE_LOG(LogMetalShaderCompiler, Warning, TEXT("Remote Building is not configured: DeltaCopy was not found."));
			GMetalLoggedRemoteCompileNotConfigured = true;
		}
		return false;
	}

	GSSHPath = FPaths::Combine(*DeltaCopyPath, TEXT("ssh.exe"));
	GRSyncPath = FPaths::Combine(*DeltaCopyPath, TEXT("rsync.exe"));

	return true;
}

static bool ExecRemoteProcess(const TCHAR* Command, int32* OutReturnCode, FString* OutStdOut, FString* OutStdErr)
{
	FString CmdLine = FString(TEXT("-i \"")) + GRemoteBuildServerSSHKey + TEXT("\" ") + GRemoteBuildServerUser + '@' + GRemoteBuildServerHost + TEXT(" ") + Command;
	//UE_LOG(LogMetalShaderCompiler, Fatal, TEXT("CmdLine=%s"), *CmdLine);
	return FPlatformProcess::ExecProcess(*GSSHPath, *CmdLine, OutReturnCode, OutStdOut, OutStdErr);
}

static FString GetXcodePath()
{
#if PLATFORM_MAC
	return FPlatformMisc::GetXcodePath();
#else
	FString XcodePath;
	ExecRemoteProcess(TEXT("/usr/bin/xcode-select --print-path"), nullptr, &XcodePath, nullptr);
	if (XcodePath.Len() > 0)
	{
		XcodePath.RemoveAt(XcodePath.Len() - 1); // Remove \n at the end of the string
	}
	return XcodePath;
#endif
}

static bool RemoteFileExists(const FString& Path)
{
#if PLATFORM_MAC
	return IFileManager::Get().FileExists(*Path);
#else
	int32 ReturnCode = 1;
	FString StdOut;
	FString StdErr;
	ExecRemoteProcess(*FString::Printf(TEXT("test -e %s"), *Path), &ReturnCode, &StdOut, &StdErr);
	return ReturnCode == 0;
#endif
}

static FString MakeRemoteTempFolder()
{
	if(GTempFolderPath.Len() == 0)
	{
		FString TempFolderPath;
		ExecRemoteProcess(TEXT("mktemp -d -t UE4Metal"), nullptr, &TempFolderPath, nullptr);
		if (TempFolderPath.Len() > 0)
		{
			TempFolderPath.RemoveAt(TempFolderPath.Len() - 1); // Remove \n at the end of the string
		}
		GTempFolderPath = TempFolderPath;
	}

	return GTempFolderPath;
}

static FString LocalPathToRemote(const FString& LocalPath, const FString& RemoteFolder)
{
	return RemoteFolder / FPaths::GetCleanFilename(LocalPath);
}

static bool CopyLocalFileToRemote(FString const& LocalPath, FString const& RemotePath)
{
	FString	remoteBasePath;
	FString remoteFileName;
	FString	remoteFileExt;
	FPaths::Split(RemotePath, remoteBasePath, remoteFileName, remoteFileExt);

	FString cygwinLocalPath = TEXT("/cygdrive/") + LocalPath.Replace(TEXT(":"), TEXT(""));

	FString	params = 
		FString::Printf(
			TEXT("-zae \"%s -i '%s'\" --rsync-path=\"mkdir -p %s && rsync\" --chmod=ug=rwX,o=rxX '%s' %s@%s:'%s'"), 
			*GSSHPath,
			*GRemoteBuildServerSSHKey, 
			*remoteBasePath, 
			*cygwinLocalPath, 
			*GRemoteBuildServerUser,
			*GRemoteBuildServerHost,
			*RemotePath);

	int32	returnCode;
	FString	stdOut, stdErr;
	FPlatformProcess::ExecProcess(*GRSyncPath, *params, &returnCode, &stdOut, &stdErr);

	return returnCode == 0;
}

static bool CopyRemoteFileToLocal(FString const& RemotePath, FString const& LocalPath)
{
	FString cygwinLocalPath = TEXT("/cygdrive/") + LocalPath.Replace(TEXT(":"), TEXT(""));

	FString	params = 
		FString::Printf(
			TEXT("-zae \"%s -i '%s'\" %s@%s:'%s' '%s'"), 
			*GSSHPath,
			*GRemoteBuildServerSSHKey, 
			*GRemoteBuildServerUser,
			*GRemoteBuildServerHost,
			*RemotePath, 
			*cygwinLocalPath);

	int32	returnCode;
	FString	stdOut, stdErr;
	FPlatformProcess::ExecProcess(*GRSyncPath, *params, &returnCode, &stdOut, &stdErr);

	return returnCode == 0;
}

static FString GetMetalBinaryPath(uint32 ShaderPlatform)
{
	const bool bIsMobile = (ShaderPlatform == SP_METAL || ShaderPlatform == SP_METAL_MRT);
	if(GMetalBinaryPath[bIsMobile].Len() == 0 || GMetalToolsPath[bIsMobile].Len() == 0)
	{
		FString XcodePath = GetXcodePath();
		if (XcodePath.Len() > 0)
		{
			FString MetalToolsPath = FString::Printf(TEXT("%s/Toolchains/XcodeDefault.xctoolchain/usr/bin"), *XcodePath);
			FString MetalPath = MetalToolsPath + TEXT("/metal");
			if (!RemoteFileExists(MetalPath))
			{
				if (bIsMobile)
				{
					MetalToolsPath = FString::Printf(TEXT("%s/Platforms/iPhoneOS.platform/usr/bin"), *XcodePath);
				}
				else
				{
					MetalToolsPath = FString::Printf(TEXT("%s/Platforms/MacOSX.platform/usr/bin"), *XcodePath);
				}
				MetalPath = MetalToolsPath + TEXT("/metal");
			}

			if (RemoteFileExists(MetalPath))
			{
				GMetalBinaryPath[bIsMobile] = MetalPath;
				GMetalToolsPath[bIsMobile] = MetalToolsPath;
			}
		}
	}

	return GMetalBinaryPath[bIsMobile];
}

static FString GetMetalToolsPath(uint32 ShaderPlatform)
{
	GetMetalBinaryPath(ShaderPlatform);
	
	const bool bIsMobile = (ShaderPlatform == SP_METAL || ShaderPlatform == SP_METAL_MRT);
	return GMetalToolsPath[bIsMobile];
}

/*------------------------------------------------------------------------------
	Shader compiling.
------------------------------------------------------------------------------*/

static inline uint32 ParseNumber(const TCHAR* Str)
{
	uint32 Num = 0;
	while (*Str && *Str >= '0' && *Str <= '9')
	{
		Num = Num * 10 + *Str++ - '0';
	}
	return Num;
}

static inline uint32 ParseNumber(const ANSICHAR* Str)
{
	uint32 Num = 0;
	while (*Str && *Str >= '0' && *Str <= '9')
	{
		Num = Num * 10 + *Str++ - '0';
	}
	return Num;
}

struct FHlslccMetalHeader : public CrossCompiler::FHlslccHeader
{
	FHlslccMetalHeader(uint8 const Version, bool const bUsingTessellation);
	virtual ~FHlslccMetalHeader();
	
	// After the standard header, different backends can output their own info
	virtual bool ParseCustomHeaderEntries(const ANSICHAR*& ShaderSource) override;
	
	float  TessellationMaxTessFactor;
	uint32 TessellationOutputControlPoints;
	uint32 TessellationDomain; // 3 = tri, 4 = quad
	uint32 TessellationInputControlPoints;
	uint32 TessellationPatchesPerThreadGroup;
	uint32 TessellationPatchCountBuffer;
	uint32 TessellationIndexBuffer;
	uint32 TessellationHSOutBuffer;
	uint32 TessellationHSTFOutBuffer;
	uint32 TessellationControlPointOutBuffer;
	uint32 TessellationControlPointIndexBuffer;
	EMetalOutputWindingMode TessellationOutputWinding;
	EMetalPartitionMode TessellationPartitioning;
	uint8 Version;
	bool bUsingTessellation;
};

FHlslccMetalHeader::FHlslccMetalHeader(uint8 const InVersion, bool const bInUsingTessellation)
{
	TessellationMaxTessFactor = 0.0f;
	TessellationOutputControlPoints = 0;
	TessellationDomain = 0;
	TessellationInputControlPoints = 0;
	TessellationPatchesPerThreadGroup = 0;
	TessellationOutputWinding = EMetalOutputWindingMode::Clockwise;
	TessellationPartitioning = EMetalPartitionMode::Pow2;
	
	TessellationPatchCountBuffer = UINT_MAX;
	TessellationIndexBuffer = UINT_MAX;
	TessellationHSOutBuffer = UINT_MAX;
	TessellationHSTFOutBuffer = UINT_MAX;
	TessellationControlPointOutBuffer = UINT_MAX;
	TessellationControlPointIndexBuffer = UINT_MAX;
	
	Version = InVersion;
	bUsingTessellation = bInUsingTessellation;
}

FHlslccMetalHeader::~FHlslccMetalHeader()
{
	
}

bool FHlslccMetalHeader::ParseCustomHeaderEntries(const ANSICHAR*& ShaderSource)
{
#define DEF_PREFIX_STR(Str) \
static const ANSICHAR* Str##Prefix = "// @" #Str ": "; \
static const int32 Str##PrefixLen = FCStringAnsi::Strlen(Str##Prefix)
	DEF_PREFIX_STR(TessellationOutputControlPoints);
	DEF_PREFIX_STR(TessellationDomain);
	DEF_PREFIX_STR(TessellationInputControlPoints);
	DEF_PREFIX_STR(TessellationMaxTessFactor);
	DEF_PREFIX_STR(TessellationOutputWinding);
	DEF_PREFIX_STR(TessellationPartitioning);
	DEF_PREFIX_STR(TessellationPatchesPerThreadGroup);
	DEF_PREFIX_STR(TessellationPatchCountBuffer);
	DEF_PREFIX_STR(TessellationIndexBuffer);
	DEF_PREFIX_STR(TessellationHSOutBuffer);
	DEF_PREFIX_STR(TessellationHSTFOutBuffer);
	DEF_PREFIX_STR(TessellationControlPointOutBuffer);
	DEF_PREFIX_STR(TessellationControlPointIndexBuffer);
#undef DEF_PREFIX_STR
	
	// Early out for non-tessellation...
	if (Version < 2 || !bUsingTessellation)
	{
		return true;
	}
	
	auto ParseUInt32Attribute = [&ShaderSource](const ANSICHAR* prefix, int32 prefixLen, uint32& attributeOut)
	{
		if (FCStringAnsi::Strncmp(ShaderSource, prefix, prefixLen) == 0)
		{
			ShaderSource += prefixLen;
			
			if (!CrossCompiler::ParseIntegerNumber(ShaderSource, attributeOut))
			{
				return false;
			}
			
			if (!CrossCompiler::Match(ShaderSource, '\n'))
			{
				return false;
			}
		}
		
		return true;
	};
 
	// Read number of tessellation output control points
	if (!ParseUInt32Attribute(TessellationOutputControlPointsPrefix, TessellationOutputControlPointsPrefixLen, TessellationOutputControlPoints))
	{
		return false;
	}
	
	// Read the tessellation domain (tri vs quad)
	if (FCStringAnsi::Strncmp(ShaderSource, TessellationDomainPrefix, TessellationDomainPrefixLen) == 0)
	{
		ShaderSource += TessellationDomainPrefixLen;
		
		if (FCStringAnsi::Strncmp(ShaderSource, "tri", 3) == 0)
		{
			ShaderSource += 3;
			TessellationDomain = 3;
		}
		else if (FCStringAnsi::Strncmp(ShaderSource, "quad", 4) == 0)
		{
			ShaderSource += 4;
			TessellationDomain = 4;
		}
		else
		{
			return false;
		}
		
		if (!CrossCompiler::Match(ShaderSource, '\n'))
		{
			return false;
		}
	}
	
	// Read number of tessellation input control points
	if (!ParseUInt32Attribute(TessellationInputControlPointsPrefix, TessellationInputControlPointsPrefixLen, TessellationInputControlPoints))
	{
		return false;
	}
	
	// Read max tessellation factor
	if (FCStringAnsi::Strncmp(ShaderSource, TessellationMaxTessFactorPrefix, TessellationMaxTessFactorPrefixLen) == 0)
	{
		ShaderSource += TessellationMaxTessFactorPrefixLen;
		
#if PLATFORM_WINDOWS
		if (sscanf_s(ShaderSource, "%g\n", &TessellationMaxTessFactor) != 1)
#else
		if (sscanf(ShaderSource, "%g\n", &TessellationMaxTessFactor) != 1)
#endif
		{
			return false;
		}
		
		while (*ShaderSource != '\n')
		{
			++ShaderSource;
		}
		++ShaderSource; // to match the newline
	}
	
	// Read tessellation output winding mode
	if (FCStringAnsi::Strncmp(ShaderSource, TessellationOutputWindingPrefix, TessellationOutputWindingPrefixLen) == 0)
	{
		ShaderSource += TessellationOutputWindingPrefixLen;
		
		if (FCStringAnsi::Strncmp(ShaderSource, "cw", 2) == 0)
		{
			ShaderSource += 2;
			TessellationOutputWinding = EMetalOutputWindingMode::Clockwise;
		}
		else if (FCStringAnsi::Strncmp(ShaderSource, "ccw", 3) == 0)
		{
			ShaderSource += 3;
			TessellationOutputWinding = EMetalOutputWindingMode::CounterClockwise;
		}
		else
		{
			return false;
		}
		
		if (!CrossCompiler::Match(ShaderSource, '\n'))
		{
			return false;
		}
	}
	
	// Read tessellation partition mode
	if (FCStringAnsi::Strncmp(ShaderSource, TessellationPartitioningPrefix, TessellationPartitioningPrefixLen) == 0)
	{
		ShaderSource += TessellationPartitioningPrefixLen;
		
		static char const* partitionModeNames[] =
		{
			// order match enum order
			"pow2",
			"integer",
			"fractional_odd",
			"fractional_even",
		};
		
		bool match = false;
		for (size_t i = 0; i < sizeof(partitionModeNames) / sizeof(partitionModeNames[0]); ++i)
		{
			size_t partitionModeNameLen = strlen(partitionModeNames[i]);
			if (FCStringAnsi::Strncmp(ShaderSource, partitionModeNames[i], partitionModeNameLen) == 0)
			{
				ShaderSource += partitionModeNameLen;
				TessellationPartitioning = (EMetalPartitionMode)i;
				match = true;
				break;
			}
		}
		
		if (!match)
		{
			return false;
		}
		
		if (!CrossCompiler::Match(ShaderSource, '\n'))
		{
			return false;
		}
	}
	
	// Read number of tessellation patches per threadgroup
	if (!ParseUInt32Attribute(TessellationPatchesPerThreadGroupPrefix, TessellationPatchesPerThreadGroupPrefixLen, TessellationPatchesPerThreadGroup))
	{
		return false;
	}
	
	if (!ParseUInt32Attribute(TessellationPatchCountBufferPrefix, TessellationPatchCountBufferPrefixLen, TessellationPatchCountBuffer))
	{
		TessellationPatchCountBuffer = UINT_MAX;
	}
	
	if (!ParseUInt32Attribute(TessellationIndexBufferPrefix, TessellationIndexBufferPrefixLen, TessellationIndexBuffer))
	{
		TessellationIndexBuffer = UINT_MAX;
	}
	
	if (!ParseUInt32Attribute(TessellationHSOutBufferPrefix, TessellationHSOutBufferPrefixLen, TessellationHSOutBuffer))
	{
		TessellationHSOutBuffer = UINT_MAX;
	}
	
	if (!ParseUInt32Attribute(TessellationControlPointOutBufferPrefix, TessellationControlPointOutBufferPrefixLen, TessellationControlPointOutBuffer))
	{
		TessellationControlPointOutBuffer = UINT_MAX;
	}
	
	if (!ParseUInt32Attribute(TessellationHSTFOutBufferPrefix, TessellationHSTFOutBufferPrefixLen, TessellationHSTFOutBuffer))
	{
		TessellationHSTFOutBuffer = UINT_MAX;
	}
	
	if (!ParseUInt32Attribute(TessellationControlPointIndexBufferPrefix, TessellationControlPointIndexBufferPrefixLen, TessellationControlPointIndexBuffer))
	{
		TessellationControlPointIndexBuffer = UINT_MAX;
	}
	
	return true;
}

/**
 * Construct the final microcode from the compiled and verified shader source.
 * @param ShaderOutput - Where to store the microcode and parameter map.
 * @param InShaderSource - Metal source with input/output signature.
 * @param SourceLen - The length of the Metal source code.
 */
static void BuildMetalShaderOutput(
	FShaderCompilerOutput& ShaderOutput,
	const FShaderCompilerInput& ShaderInput, 
	const ANSICHAR* InShaderSource,
	uint32 SourceLen,
	uint32 SourceCRCLen,
	uint32 SourceCRC,
	uint8 Version,
	TCHAR const* Standard,
	TArray<FShaderCompilerError>& OutErrors,
	FMetalTessellationOutputs const& TessOutputAttribs
	)
{
	const ANSICHAR* USFSource = InShaderSource;
	
	FString const* UsingTessellationDefine = ShaderInput.Environment.GetDefinitions().Find(TEXT("USING_TESSELLATION"));
	bool bUsingTessellation = (UsingTessellationDefine != nullptr && FString("1") == *UsingTessellationDefine);
	
	FHlslccMetalHeader CCHeader(Version, bUsingTessellation);
	if (!CCHeader.Read(USFSource, SourceLen))
	{
		UE_LOG(LogMetalShaderCompiler, Fatal, TEXT("Bad hlslcc header found"));
	}
	
	const ANSICHAR* SideTableString = FCStringAnsi::Strstr(USFSource, "@SideTable: ");
	
	FMetalCodeHeader Header = {0};
	Header.CompileFlags = (ShaderInput.Environment.CompilerFlags.Contains(CFLAG_Debug) ? (1 << CFLAG_Debug) : 0);
	Header.CompileFlags |= (ShaderInput.Environment.CompilerFlags.Contains(CFLAG_NoFastMath) ? (1 << CFLAG_NoFastMath) : 0);
	Header.CompileFlags |= (ShaderInput.Environment.CompilerFlags.Contains(CFLAG_KeepDebugInfo) ? (1 << CFLAG_KeepDebugInfo) : 0);
	Header.CompileFlags |= (ShaderInput.Environment.CompilerFlags.Contains(CFLAG_ZeroInitialise) ? (1 <<  CFLAG_ZeroInitialise) : 0);
	Header.CompileFlags |= (ShaderInput.Environment.CompilerFlags.Contains(CFLAG_BoundsChecking) ? (1 << CFLAG_BoundsChecking) : 0);
	Header.CompileFlags |= (ShaderInput.Environment.CompilerFlags.Contains(CFLAG_Archive) ? (1 << CFLAG_Archive) : 0);
	Header.Version = Version;
	Header.SideTable = -1;
	Header.SourceLen = SourceCRCLen;
	Header.SourceCRC = SourceCRC;
	
	if (SideTableString)
	{
		int32 SideTableLoc = -1;
		while (*SideTableString && *SideTableString != '\n')
		{
			if (*SideTableString == '(')
			{
				SideTableString++;
				if (*SideTableString && *SideTableString != '\n')
				{
					SideTableLoc = (int32)ParseNumber(SideTableString);
				}
			}
			else
			{
				SideTableString++;
			}
		}
		if (SideTableLoc >= 0)
		{
			Header.SideTable = SideTableLoc;
		}
		else
		{
			UE_LOG(LogMetalShaderCompiler, Fatal, TEXT("Couldn't parse the SideTable buffer index for bounds checking"));
		}
	}
	
	FShaderParameterMap& ParameterMap = ShaderOutput.ParameterMap;
	EShaderFrequency Frequency = (EShaderFrequency)ShaderOutput.Target.Frequency;

	TBitArray<> UsedUniformBufferSlots;
	UsedUniformBufferSlots.Init(false,32);

	// Write out the magic markers.
	Header.Frequency = Frequency;

	// Only inputs for vertex shaders must be tracked.
	if (Frequency == SF_Vertex)
	{
		static const FString AttributePrefix = TEXT("in_ATTRIBUTE");
		for (auto& Input : CCHeader.Inputs)
		{
			// Only process attributes.
			if (Input.Name.StartsWith(AttributePrefix))
			{
				uint8 AttributeIndex = ParseNumber(*Input.Name + AttributePrefix.Len());
				Header.Bindings.InOutMask |= (1 << AttributeIndex);
			}
		}
	}

	// Then the list of outputs.
	static const FString TargetPrefix = "out_Target";
	static const FString GL_FragDepth = "FragDepth";
	// Only outputs for pixel shaders must be tracked.
	if (Frequency == SF_Pixel)
	{
		for (auto& Output : CCHeader.Outputs)
		{
			// Handle targets.
			if (Output.Name.StartsWith(TargetPrefix))
			{
				uint8 TargetIndex = ParseNumber(*Output.Name + TargetPrefix.Len());
				Header.Bindings.InOutMask |= (1 << TargetIndex);
			}
			// Handle depth writes.
			else if (Output.Name.Equals(GL_FragDepth))
			{
				Header.Bindings.InOutMask |= 0x8000;
			}
		}
	}

	bool bHasRegularUniformBuffers = false;

	// Then 'normal' uniform buffers.
	for (auto& UniformBlock : CCHeader.UniformBlocks)
	{
		uint16 UBIndex = UniformBlock.Index;
		if (UBIndex >= Header.Bindings.NumUniformBuffers)
		{
			Header.Bindings.NumUniformBuffers = UBIndex + 1;
		}
		UsedUniformBufferSlots[UBIndex] = true;
		ParameterMap.AddParameterAllocation(*UniformBlock.Name, UBIndex, 0, 0);
		bHasRegularUniformBuffers = true;
	}

	// Packed global uniforms
	const uint16 BytesPerComponent = 4;
	TMap<ANSICHAR, uint16> PackedGlobalArraySize;
	for (auto& PackedGlobal : CCHeader.PackedGlobals)
	{
		ParameterMap.AddParameterAllocation(
			*PackedGlobal.Name,
			PackedGlobal.PackedType,
			PackedGlobal.Offset * BytesPerComponent,
			PackedGlobal.Count * BytesPerComponent
			);

		uint16& Size = PackedGlobalArraySize.FindOrAdd(PackedGlobal.PackedType);
		Size = FMath::Max<uint16>(BytesPerComponent * (PackedGlobal.Offset + PackedGlobal.Count), Size);
	}

	// Packed Uniform Buffers
	TMap<int, TMap<ANSICHAR, uint16> > PackedUniformBuffersSize;
	for (auto& PackedUB : CCHeader.PackedUBs)
	{
		check(PackedUB.Attribute.Index == Header.Bindings.NumUniformBuffers);
		UsedUniformBufferSlots[PackedUB.Attribute.Index] = true;
		ParameterMap.AddParameterAllocation(*PackedUB.Attribute.Name, Header.Bindings.NumUniformBuffers++, 0, 0);

		// Nothing else...
		//for (auto& Member : PackedUB.Members)
		//{
		//}
	}

	// Packed Uniform Buffers copy lists & setup sizes for each UB/Precision entry
	for (auto& PackedUBCopy : CCHeader.PackedUBCopies)
	{
		CrossCompiler::FUniformBufferCopyInfo CopyInfo;
		CopyInfo.SourceUBIndex = PackedUBCopy.SourceUB;
		CopyInfo.SourceOffsetInFloats = PackedUBCopy.SourceOffset;
		CopyInfo.DestUBIndex = PackedUBCopy.DestUB;
		CopyInfo.DestUBTypeName = PackedUBCopy.DestPackedType;
		CopyInfo.DestUBTypeIndex = CrossCompiler::PackedTypeNameToTypeIndex(CopyInfo.DestUBTypeName);
		CopyInfo.DestOffsetInFloats = PackedUBCopy.DestOffset;
		CopyInfo.SizeInFloats = PackedUBCopy.Count;

		Header.UniformBuffersCopyInfo.Add(CopyInfo);

		auto& UniformBufferSize = PackedUniformBuffersSize.FindOrAdd(CopyInfo.DestUBIndex);
		uint16& Size = UniformBufferSize.FindOrAdd(CopyInfo.DestUBTypeName);
		Size = FMath::Max<uint16>(BytesPerComponent * (CopyInfo.DestOffsetInFloats + CopyInfo.SizeInFloats), Size);
	}

	for (auto& PackedUBCopy : CCHeader.PackedUBGlobalCopies)
	{
		CrossCompiler::FUniformBufferCopyInfo CopyInfo;
		CopyInfo.SourceUBIndex = PackedUBCopy.SourceUB;
		CopyInfo.SourceOffsetInFloats = PackedUBCopy.SourceOffset;
		CopyInfo.DestUBIndex = PackedUBCopy.DestUB;
		CopyInfo.DestUBTypeName = PackedUBCopy.DestPackedType;
		CopyInfo.DestUBTypeIndex = CrossCompiler::PackedTypeNameToTypeIndex(CopyInfo.DestUBTypeName);
		CopyInfo.DestOffsetInFloats = PackedUBCopy.DestOffset;
		CopyInfo.SizeInFloats = PackedUBCopy.Count;

		Header.UniformBuffersCopyInfo.Add(CopyInfo);

		uint16& Size = PackedGlobalArraySize.FindOrAdd(CopyInfo.DestUBTypeName);
		Size = FMath::Max<uint16>(BytesPerComponent * (CopyInfo.DestOffsetInFloats + CopyInfo.SizeInFloats), Size);
	}
	Header.Bindings.bHasRegularUniformBuffers = bHasRegularUniformBuffers;

	// Setup Packed Array info
	Header.Bindings.PackedGlobalArrays.Reserve(PackedGlobalArraySize.Num());
	for (auto Iterator = PackedGlobalArraySize.CreateIterator(); Iterator; ++Iterator)
	{
		ANSICHAR TypeName = Iterator.Key();
		uint16 Size = Iterator.Value();
		Size = (Size + 0xf) & (~0xf);
		CrossCompiler::FPackedArrayInfo Info;
		Info.Size = Size;
		Info.TypeName = TypeName;
		Info.TypeIndex = CrossCompiler::PackedTypeNameToTypeIndex(TypeName);
		Header.Bindings.PackedGlobalArrays.Add(Info);
	}

	// Setup Packed Uniform Buffers info
	Header.Bindings.PackedUniformBuffers.Reserve(PackedUniformBuffersSize.Num());
	for (auto Iterator = PackedUniformBuffersSize.CreateIterator(); Iterator; ++Iterator)
	{
		int BufferIndex = Iterator.Key();
		auto& ArraySizes = Iterator.Value();
		TArray<CrossCompiler::FPackedArrayInfo> InfoArray;
		InfoArray.Reserve(ArraySizes.Num());
		for (auto IterSizes = ArraySizes.CreateIterator(); IterSizes; ++IterSizes)
		{
			ANSICHAR TypeName = IterSizes.Key();
			uint16 Size = IterSizes.Value();
			Size = (Size + 0xf) & (~0xf);
			CrossCompiler::FPackedArrayInfo Info;
			Info.Size = Size;
			Info.TypeName = TypeName;
			Info.TypeIndex = CrossCompiler::PackedTypeNameToTypeIndex(TypeName);
			InfoArray.Add(Info);
		}

		Header.Bindings.PackedUniformBuffers.Add(InfoArray);
	}

	uint32 NumTextures = 0;
	
	// Then samplers.
	TMap<FString, uint32> SamplerMap;
	for (auto& Sampler : CCHeader.Samplers)
	{
		ParameterMap.AddParameterAllocation(
			*Sampler.Name,
			0,
			Sampler.Offset,
			Sampler.Count
			);

		NumTextures += Sampler.Count;

		for (auto& SamplerState : Sampler.SamplerStates)
		{
			SamplerMap.Add(SamplerState, Sampler.Count);
		}
	}
	
	Header.Bindings.NumSamplers = CCHeader.SamplerStates.Num();

	// Then UAVs (images in Metal)
	for (auto& UAV : CCHeader.UAVs)
	{
		ParameterMap.AddParameterAllocation(
			*UAV.Name,
			0,
			UAV.Offset,
			UAV.Count
			);

		Header.Bindings.NumUAVs = FMath::Max<uint8>(
			Header.Bindings.NumSamplers,
			UAV.Offset + UAV.Count
			);
	}

	for (auto& SamplerState : CCHeader.SamplerStates)
	{
		ParameterMap.AddParameterAllocation(
			*SamplerState.Name,
			0,
			SamplerState.Index,
			SamplerMap[SamplerState.Name]
			);
	}

	Header.NumThreadsX = CCHeader.NumThreads[0];
	Header.NumThreadsY = CCHeader.NumThreads[1];
	Header.NumThreadsZ = CCHeader.NumThreads[2];
	
	Header.TessellationOutputControlPoints 		= CCHeader.TessellationOutputControlPoints;
	Header.TessellationDomain					= CCHeader.TessellationDomain;
	Header.TessellationInputControlPoints       = CCHeader.TessellationInputControlPoints;
	Header.TessellationMaxTessFactor            = CCHeader.TessellationMaxTessFactor;
	Header.TessellationOutputWinding			= CCHeader.TessellationOutputWinding;
	Header.TessellationPartitioning				= CCHeader.TessellationPartitioning;
	Header.TessellationPatchesPerThreadGroup    = CCHeader.TessellationPatchesPerThreadGroup;
	Header.TessellationPatchCountBuffer         = CCHeader.TessellationPatchCountBuffer;
	Header.TessellationIndexBuffer              = CCHeader.TessellationIndexBuffer;
	Header.TessellationHSOutBuffer              = CCHeader.TessellationHSOutBuffer;
	Header.TessellationHSTFOutBuffer            = CCHeader.TessellationHSTFOutBuffer;
	Header.TessellationControlPointOutBuffer    = CCHeader.TessellationControlPointOutBuffer;
	Header.TessellationControlPointIndexBuffer  = CCHeader.TessellationControlPointIndexBuffer;
	Header.TessellationOutputAttribs            = TessOutputAttribs;
	Header.bFunctionConstants					= (FCStringAnsi::Strstr(USFSource, "[[ function_constant(") != nullptr);
	
	// Build the SRT for this shader.
	{
		// Build the generic SRT for this shader.
		FShaderCompilerResourceTable GenericSRT;
		BuildResourceTableMapping(ShaderInput.Environment.ResourceTableMap, ShaderInput.Environment.ResourceTableLayoutHashes, UsedUniformBufferSlots, ShaderOutput.ParameterMap, GenericSRT);

		// Copy over the bits indicating which resource tables are active.
		Header.Bindings.ShaderResourceTable.ResourceTableBits = GenericSRT.ResourceTableBits;

		Header.Bindings.ShaderResourceTable.ResourceTableLayoutHashes = GenericSRT.ResourceTableLayoutHashes;

		// Now build our token streams.
		BuildResourceTableTokenStream(GenericSRT.TextureMap, GenericSRT.MaxBoundResourceTable, Header.Bindings.ShaderResourceTable.TextureMap);
		BuildResourceTableTokenStream(GenericSRT.ShaderResourceViewMap, GenericSRT.MaxBoundResourceTable, Header.Bindings.ShaderResourceTable.ShaderResourceViewMap);
		BuildResourceTableTokenStream(GenericSRT.SamplerMap, GenericSRT.MaxBoundResourceTable, Header.Bindings.ShaderResourceTable.SamplerMap);
		BuildResourceTableTokenStream(GenericSRT.UnorderedAccessViewMap, GenericSRT.MaxBoundResourceTable, Header.Bindings.ShaderResourceTable.UnorderedAccessViewMap);

		Header.Bindings.NumUniformBuffers = FMath::Max((uint8)GetNumUniformBuffersUsed(GenericSRT), Header.Bindings.NumUniformBuffers);
	}

	// Metal supports 16 across all HW
	const int32 MaxSamplers = 16;

	FString MetalCode = FString(USFSource);
	if (ShaderInput.Environment.CompilerFlags.Contains(CFLAG_KeepDebugInfo) || ShaderInput.Environment.CompilerFlags.Contains(CFLAG_Debug))
	{
		MetalCode.InsertAt(0, FString::Printf(TEXT("// %s\n"), *CCHeader.Name));
		Header.ShaderName = CCHeader.Name;
	}
	
	if (Header.Bindings.NumSamplers > MaxSamplers)
	{
		ShaderOutput.bSucceeded = false;
		FShaderCompilerError* NewError = new(ShaderOutput.Errors) FShaderCompilerError();
		NewError->StrippedErrorMessage =
			FString::Printf(TEXT("shader uses %d samplers exceeding the limit of %d"),
				Header.Bindings.NumSamplers, MaxSamplers);
	}
	else if(ShaderInput.Environment.CompilerFlags.Contains(CFLAG_Debug))
	{
		// Write out the header and shader source code.
		FMemoryWriter Ar(ShaderOutput.ShaderCode.GetWriteAccess(), true);
		uint8 PrecompiledFlag = 0;
		Ar << PrecompiledFlag;
		Ar << Header;
		Ar.Serialize((void*)USFSource, SourceLen + 1 - (USFSource - InShaderSource));
		
		// store data we can pickup later with ShaderCode.FindOptionalData('n'), could be removed for shipping
		ShaderOutput.ShaderCode.AddOptionalData('n', TCHAR_TO_UTF8(*ShaderInput.GenerateShaderName()));

		ShaderOutput.NumInstructions = 0;
		ShaderOutput.NumTextureSamplers = Header.Bindings.NumSamplers;
		ShaderOutput.bSucceeded = true;
	}
	else
	{
		// at this point, the shader source is ready to be compiled
		// We need to use a temp directory path that will be consistent across devices so that debug info
		// can be loaded (as it must be at a consistent location).
#if PLATFORM_MAC
		TCHAR const* TempDir = TEXT("/tmp");
#else
		TCHAR const* TempDir = FPlatformProcess::UserTempDir();
#endif
		FString InputFilename = FPaths::CreateTempFilename(TempDir, TEXT("ShaderIn"), TEXT(""));
		FString ObjFilename = InputFilename + TEXT(".o");
		FString OutputFilename = InputFilename + TEXT(".lib");
		InputFilename = InputFilename + TEXT(".metal");
		FString InputFilePath = InputFilename;
		
		// write out shader source
		FFileHelper::SaveStringToFile(MetalCode, *InputFilename);
		
		FString HashedName = TempDir / FString::Printf(TEXT("%u_%u.metal"), SourceCRCLen, SourceCRC);
		IFileManager::Get().Move(*HashedName, *InputFilePath, false, false, true, true);
		IFileManager::Get().Delete(*InputFilePath);
		InputFilename = HashedName;
		InputFilePath = HashedName;
		
		int32 ReturnCode = 0;
		FString Results;
		FString Errors;
		bool bCompileAtRuntime = true;
		bool bSucceeded = false;

		// metal commandlines
		FString DebugInfo = (ShaderInput.Environment.CompilerFlags.Contains(CFLAG_KeepDebugInfo) || ShaderInput.Environment.CompilerFlags.Contains(CFLAG_Archive)) ? TEXT("-gline-tables-only") : TEXT("");
		FString MathMode = ShaderInput.Environment.CompilerFlags.Contains(CFLAG_NoFastMath) ? TEXT("-fno-fast-math") : TEXT("-ffast-math");

#if METAL_OFFLINE_COMPILE
		const bool bIsMobile = (ShaderInput.Target.Platform == SP_METAL || ShaderInput.Target.Platform == SP_METAL_MRT);
		bool bRemoteBuildingConfigured = IsRemoteBuildingConfigured();
		
		FString MetalPath = GetMetalBinaryPath(ShaderInput.Target.Platform);
		FString MetalToolsPath = GetMetalToolsPath(ShaderInput.Target.Platform);
		
		if(PLATFORM_MAC || bRemoteBuildingConfigured)
		{
			if(MetalPath.Len() == 0 || MetalToolsPath.Len() == 0)
			{
				if(!GMetalLoggedRemoteCompileNotConfigured)
				{
					if (bRemoteBuildingConfigured)
					{
						UE_LOG(LogMetalShaderCompiler, Warning, TEXT("Xcode's metal shader compiler was not found, verify Xcode has been installed on the Mac used for remote compilation. Falling back to online compiled text shaders which will be slower on first launch."));
					}
					else
					{
						UE_LOG(LogMetalShaderCompiler, Warning, TEXT("Xcode's metal shader compiler was not found, verify Xcode has been installed on this Mac. Falling back to online compiled text shaders which will be slower on first launch."));
					}
					GMetalLoggedRemoteCompileNotConfigured = true;
				}
				bRemoteBuildingConfigured = false;
			}
			else
			{
				bCompileAtRuntime = false;
				bSucceeded = true;
			}
		}
		else
		{
			bCompileAtRuntime = true;
			bSucceeded = true;
		}
		
		if (bCompileAtRuntime == false && (PLATFORM_MAC || bRemoteBuildingConfigured))
		{
			bool bUseSharedPCH = false;
			
			FString MetalStdlibPCHFilename = FString::Printf(TEXT("metal_stdlib%s.pch"), Standard);
			// get rid of some not so filename-friendly characters ('=',' ' -> '_')
			MetalStdlibPCHFilename = MetalStdlibPCHFilename.Replace(TEXT("="), TEXT("_")).Replace(TEXT(" "), TEXT("_"));
			FString MetalStdlibPCHPath = FString::Printf(TEXT("%s/%s"), TempDir, *MetalStdlibPCHFilename);

#if PLATFORM_MAC // Only native Mac builds can use a shared PCH
			// Unset the SDKROOT to avoid problems with the incorrect path being used when compiling with the shared PCH.
			TCHAR SdkRoot[4096];
			FPlatformMisc::GetEnvironmentVariable(TEXT("SDKROOT"), SdkRoot, ARRAY_COUNT(SdkRoot));
			if (FCStringWide::Strlen(SdkRoot))
			{
				unsetenv("SDKROOT");
			}
			
			if (IFileManager::Get().FileSize(*MetalStdlibPCHPath) <= 0)
			{
				// build the PCH file for metal_stdlib
				// NOTE: multiple compilers will build this PCH at the same time -- this is ok because metal is atomic for this so one will "win"
				//   that means there is some loss to this method
				FString Params = FString::Printf(TEXT("-x metal-header %s %s/../lib/clang/3.5/include/metal/metal_stdlib -o %s"), Standard, *MetalToolsPath, *MetalStdlibPCHPath);
				
				bUseSharedPCH = FPlatformProcess::ExecProcess( *MetalPath, *Params, &ReturnCode, &Results, &Errors );
				
				if (!bUseSharedPCH || ReturnCode != 0)
				{
					bUseSharedPCH = false;
					UE_LOG(LogMetalShaderCompiler, Warning, TEXT("Metal Shared PCH generation failed - compilation will proceed without a shared PCH: %s."), *Errors);
					// NOTE: if metal fails because of the PCH file -- simply delete the PCH file to make it recreate it
					IFileManager::Get().Delete(*MetalStdlibPCHPath);
				}
			}
#endif
			
			FString Params;
			if (bUseSharedPCH && IFileManager::Get().FileSize(*MetalStdlibPCHPath) > 0)
			{
				Params = FString::Printf(TEXT("-include-pch %s %s %s -Wno-null-character %s %s -o %s"), *MetalStdlibPCHPath, *DebugInfo, *MathMode, Standard, *InputFilename, *ObjFilename);
			}
			else
			{
				Params = FString::Printf(TEXT("%s %s -Wno-null-character %s %s -o %s"), *DebugInfo, *MathMode, Standard, *InputFilename, *ObjFilename);
			}
			
			if (PLATFORM_MAC)
			{
				FPlatformProcess::ExecProcess( *MetalPath, *Params, &ReturnCode, &Results, &Errors );
				
				// handle compile error
				if (ReturnCode != 0 || IFileManager::Get().FileSize(*ObjFilename) <= 0)
				{
					FShaderCompilerError* Error = new(OutErrors) FShaderCompilerError();
					Error->ErrorFile = InputFilename;
					Error->ErrorLineString = TEXT("0");
					Error->StrippedErrorMessage = Results + Errors;
					bSucceeded = false;
				}
				else
				{
					Params = FString::Printf(TEXT("-o %s %s"), *OutputFilename, *ObjFilename);
					FString MetalLibPath = MetalToolsPath + TEXT("/metallib");
					FPlatformProcess::ExecProcess( *MetalLibPath, *Params, &ReturnCode, &Results, &Errors );
					
					// handle compile error
					if (ReturnCode != 0 || IFileManager::Get().FileSize(*OutputFilename) <= 0)
					{
						FShaderCompilerError* Error = new(OutErrors) FShaderCompilerError();
						Error->ErrorFile = InputFilename;
						Error->ErrorLineString = TEXT("0");
						Error->StrippedErrorMessage = Results + Errors;
						bSucceeded = false;
					}
					else
					{
						bCompileAtRuntime = false;
						bSucceeded = true;
					}
				}
				
#if PLATFORM_MAC
				// Reset the SDKROOT environment we unset earlier.
				if (FCStringWide::Strlen(SdkRoot))
				{
					setenv("SDKROOT", TCHAR_TO_UTF8(SdkRoot), 1);
				}
#endif
			}
			else if (bRemoteBuildingConfigured)
			{
				FString MetalLibPath = MetalPath + TEXT("lib");
				
				const FString RemoteFolder = MakeRemoteTempFolder();
				const FString RemoteInputFile = LocalPathToRemote(InputFilename, RemoteFolder);			// Input file to the compiler - Copied from local machine to remote machine
				const FString RemoteObjFile = LocalPathToRemote(ObjFilename, RemoteFolder);				// Output from the compiler -> Input file to the archiver
				const FString RemoteOutputFilename = LocalPathToRemote(OutputFilename, RemoteFolder);	// Output from the library generator - Copied from remote machine to local machine
				CopyLocalFileToRemote(InputFilename, RemoteInputFile);
				InputFilePath = RemoteInputFile;
				
				FString MetalParams = FString::Printf(TEXT("%s %s -Wno-null-character %s %s -o %s"), *DebugInfo, *MathMode, Standard, *RemoteInputFile, *RemoteObjFile);
				FString LibraryParams = FString::Printf(TEXT("-o %s %s"), *RemoteOutputFilename, *RemoteObjFile);
				ExecRemoteProcess(*FString::Printf(TEXT("%s %s && %s %s"), *MetalPath, *MetalParams, *MetalLibPath, *LibraryParams), &ReturnCode, &Results, &Errors);
				
				bSucceeded = (ReturnCode == 0);
				if (bSucceeded)
				{
					if (ShaderInput.Environment.CompilerFlags.Contains(CFLAG_Archive))
					{
						CopyRemoteFileToLocal(RemoteObjFile, ObjFilename);
					}
					CopyRemoteFileToLocal(RemoteOutputFilename, OutputFilename);
					bCompileAtRuntime = false;
				}
				else
				{
					FShaderCompilerError* Error = new(OutErrors) FShaderCompilerError();
					Error->ErrorFile = InputFilename;
					Error->ErrorLineString = TEXT("0");
					Error->StrippedErrorMessage = FString::Printf(TEXT("Failed to compile remotely, code: %d, output: %s %s"), ReturnCode, *Results, *Errors);
				}
			}
		}
#else
		// Assume success if we can't compile shaders offline
		bSucceeded = true;
#endif	// PLATFORM_MAC

		// Write out the header and compiled shader code
		FMemoryWriter Ar(ShaderOutput.ShaderCode.GetWriteAccess(), true);
		uint8 PrecompiledFlag = bCompileAtRuntime ? 0 : 1;
		Ar << PrecompiledFlag;
		Ar << Header;

		if (!bCompileAtRuntime)
		{
			// load output
			TArray<uint8> CompiledShader;
			FFileHelper::LoadFileToArray(CompiledShader, *OutputFilename);
			
			// jam it into the output bytes
			Ar.Serialize(CompiledShader.GetData(), CompiledShader.Num());

			if (ShaderInput.Environment.CompilerFlags.Contains(CFLAG_Archive))
			{
				TArray<uint8> ObjectFile;
				if (FFileHelper::LoadFileToArray(ObjectFile, *ObjFilename))
				{
					ShaderOutput.ShaderCode.AddOptionalData('o', ObjectFile.GetData(), ObjectFile.Num());
				}
				else
				{
					FShaderCompilerError* Error = new(OutErrors) FShaderCompilerError();
					Error->ErrorFile = ObjFilename;
					Error->ErrorLineString = TEXT("0");
					Error->StrippedErrorMessage = TEXT("Failed to load the object file for archiving!");
					bSucceeded = false;
				}
			}
			
			IFileManager::Get().Delete(*ObjFilename);
			IFileManager::Get().Delete(*OutputFilename);
		}
		else
		{
			// Write out the header and shader source code.
			Ar.Serialize((void*)USFSource, SourceLen + 1 - (USFSource - InShaderSource));

			// store data we can pickup later with ShaderCode.FindOptionalData('n'), could be removed for shipping
			// Daniel L: This GenerateShaderName does not generate a deterministic output among shaders as the shader code can be shared. 
			//			uncommenting this will cause the project to have non deterministic materials and will hurt patch sizes
			//ShaderOutput.ShaderCode.AddOptionalData('n', TCHAR_TO_UTF8(*ShaderInput.GenerateShaderName()));

			ShaderOutput.bSucceeded = bSucceeded || ShaderOutput.bSucceeded;
		}

		if (ShaderInput.Environment.CompilerFlags.Contains(CFLAG_KeepDebugInfo))
		{
			// store data we can pickup later with ShaderCode.FindOptionalData('n'), could be removed for shipping
			ShaderOutput.ShaderCode.AddOptionalData('n', TCHAR_TO_UTF8(*ShaderInput.GenerateShaderName()));
			ShaderOutput.ShaderCode.AddOptionalData('c', TCHAR_TO_UTF8(*MetalCode));
			ShaderOutput.ShaderCode.AddOptionalData('p', TCHAR_TO_UTF8(*InputFilePath));
		}
		else if (ShaderInput.Environment.CompilerFlags.Contains(CFLAG_Archive))
		{
			ShaderOutput.ShaderCode.AddOptionalData('c', TCHAR_TO_UTF8(*MetalCode));
			ShaderOutput.ShaderCode.AddOptionalData('p', TCHAR_TO_UTF8(*InputFilePath));
		}
		
		ShaderOutput.NumInstructions = 0;
		ShaderOutput.NumTextureSamplers = Header.Bindings.NumSamplers;
		ShaderOutput.bSucceeded = bSucceeded;
	}
}

/*------------------------------------------------------------------------------
	External interface.
------------------------------------------------------------------------------*/

static FString CreateCommandLineHLSLCC( const FString& ShaderFile, const FString& OutputFile, const FString& EntryPoint, EHlslCompileTarget Target, EHlslShaderFrequency Frequency, uint32 CCFlags )
{
	const TCHAR* VersionSwitch = TEXT("-metal");
	switch (Target)
	{
        case HCT_FeatureLevelES2:
		case HCT_FeatureLevelES3_1:
			VersionSwitch = TEXT("-metal");
			break;
		case HCT_FeatureLevelSM4:
			VersionSwitch = TEXT("-metalsm4");
			break;
		case HCT_FeatureLevelSM5:
			VersionSwitch = TEXT("-metalsm5");
			break;
			
		default:
			check(0);
	}
	return CrossCompiler::CreateBatchFileContents(ShaderFile, OutputFile, Frequency, EntryPoint, VersionSwitch, CCFlags, TEXT(""));
}

// For Metal <= 1.1
static const EHlslShaderFrequency FrequencyTable1[] =
{
	HSF_VertexShader,
	HSF_InvalidFrequency,
	HSF_InvalidFrequency,
	HSF_PixelShader,
	HSF_InvalidFrequency,
	HSF_ComputeShader
};

// For Metal >= 1.2
static const EHlslShaderFrequency FrequencyTable2[] =
{
	HSF_VertexShader,
	HSF_HullShader,
	HSF_DomainShader,
	HSF_PixelShader,
	HSF_InvalidFrequency,
	HSF_ComputeShader
};

void CompileShader_Metal(const FShaderCompilerInput& _Input,FShaderCompilerOutput& Output,const FString& WorkingDirectory)
{
	auto Input = _Input;
	FString PreprocessedShader;
	FShaderCompilerDefinitions AdditionalDefines;
	EHlslCompileTarget HlslCompilerTarget = HCT_FeatureLevelES3_1; // Always ES3.1 for now due to the way RCO has configured the MetalBackend
	EHlslCompileTarget MetalCompilerTarget = HCT_FeatureLevelES3_1; // Varies depending on the actual intended Metal target.
	ECompilerFlags PlatformFlowControl = CFLAG_AvoidFlowControl;

	// Work out which standard we need, this is dependent on the shader platform.
	const bool bIsMobile = (Input.Target.Platform == SP_METAL || Input.Target.Platform == SP_METAL_MRT);
	if (bIsMobile)
	{
		AdditionalDefines.SetDefine(TEXT("IOS"), 1);
	}
	else
	{
		AdditionalDefines.SetDefine(TEXT("MAC"), 1);
		// On OS X it is always better to leave the flow control statements in the MetalSL & let the MetalSL->GPU compilers
		// optimise it appropriately for each GPU. This gives a performance gain on pretty much all vendors & GPUs.
		// @todo The Metal shader compiler shipped for Nvidia GPUs in 10.11 DP7 & later can't deal with flow-control statements and local constant array variables so we have to sacrifice performance for correctness.
		// PlatformFlowControl = CFLAG_PreferFlowControl;
	}
	
	AdditionalDefines.SetDefine(TEXT("COMPILER_HLSLCC"), 1 );
	AdditionalDefines.SetDefine(TEXT("row_major"), TEXT(""));
	AdditionalDefines.SetDefine(TEXT("COMPILER_METAL"), 1);

	static FName NAME_SF_METAL(TEXT("SF_METAL"));
	static FName NAME_SF_METAL_MRT(TEXT("SF_METAL_MRT"));
	static FName NAME_SF_METAL_SM4(TEXT("SF_METAL_SM4"));
	static FName NAME_SF_METAL_SM5(TEXT("SF_METAL_SM5"));
	static FName NAME_SF_METAL_MACES3_1(TEXT("SF_METAL_MACES3_1"));
	static FName NAME_SF_METAL_MACES2(TEXT("SF_METAL_MACES2"));
	static FName NAME_SF_METAL_MRT_MAC(TEXT("SF_METAL_MRT_MAC"));
	
    EMetalGPUSemantics Semantics = EMetalGPUSemanticsMobile;
    TCHAR const* StandardPlatform = TEXT("ios");
    TCHAR const* StandardVersion = nullptr;

	FString const* MaxVersion = Input.Environment.GetDefinitions().Find(TEXT("MAX_SHADER_LANGUAGE_VERSION"));
	uint8 VersionEnum = 0;
	if (MaxVersion)
	{
		if(MaxVersion->IsNumeric())
		{
			LexicalConversion::FromString(VersionEnum, *(*MaxVersion));
			switch(VersionEnum)
			{
				case 2:
					// Enable full SM5 feature support so tessellation & fragment UAVs compile
					StandardVersion = TEXT("1.2");
					HlslCompilerTarget = HCT_FeatureLevelSM5;
					break;
				case 1:
					StandardVersion = TEXT("1.1");
					break;
				case 0:
				default:
					StandardVersion = nullptr;
					break;
			}
		}
	}
	
	if (Input.ShaderFormat == NAME_SF_METAL)
	{
		AdditionalDefines.SetDefine(TEXT("METAL_PROFILE"), 1);
		VersionEnum = StandardVersion ? VersionEnum : 0;
		StandardVersion = VersionEnum > 0 ? StandardVersion : TEXT("1.0"); // May require SHADER_LANGUAGE_VERSION for fragment UAVs and/or tessellation.
		const bool bUseFullPrecisionInPS = Input.Environment.CompilerFlags.Contains(CFLAG_UseFullPrecisionInPS);
		if (bUseFullPrecisionInPS)
		{
			AdditionalDefines.SetDefine(TEXT("FORCE_FLOATS"), (uint32)1);
		}
	}
	else if (Input.ShaderFormat == NAME_SF_METAL_MRT)
	{
		AdditionalDefines.SetDefine(TEXT("METAL_MRT_PROFILE"), 1);
		AdditionalDefines.SetDefine(TEXT("FORCE_FLOATS"), 1); // Force floats to avoid radr://24884199 & radr://24884860
		VersionEnum = StandardVersion ? VersionEnum : 0;
		StandardVersion = VersionEnum > 0 ? StandardVersion : TEXT("1.1"); // May require SHADER_LANGUAGE_VERSION for fragment UAVs and/or tessellation.
		MetalCompilerTarget = HCT_FeatureLevelSM5;
		Semantics = EMetalGPUSemanticsTBDRDesktop;
	}
	else if (Input.ShaderFormat == NAME_SF_METAL_MACES2)
	{
		AdditionalDefines.SetDefine(TEXT("METAL_ES2_PROFILE"), 1);
		AdditionalDefines.SetDefine(TEXT("FORCE_FLOATS"), 1); // Force floats to avoid radr://24884199 & radr://24884860
		StandardPlatform = TEXT("osx");
		StandardVersion = TEXT("1.1"); // Always standard 1.1
		VersionEnum = 1;
		MetalCompilerTarget = HCT_FeatureLevelES2;
		Semantics = EMetalGPUSemanticsImmediateDesktop;
	}
	else if (Input.ShaderFormat == NAME_SF_METAL_MACES3_1)
	{
		AdditionalDefines.SetDefine(TEXT("METAL_PROFILE"), 1);
		AdditionalDefines.SetDefine(TEXT("FORCE_FLOATS"), 1); // Force floats to avoid radr://24884199 & radr://24884860
		StandardPlatform = TEXT("osx");
		VersionEnum = StandardVersion ? VersionEnum : 1;
		VersionEnum = VersionEnum > 0 ? VersionEnum : 1;
		StandardVersion = VersionEnum > 1 ? StandardVersion : TEXT("1.1"); // May require SHADER_LANGUAGE_VERSION for fragment UAVs and/or tessellation.
		MetalCompilerTarget = HCT_FeatureLevelES3_1;
		Semantics = EMetalGPUSemanticsImmediateDesktop;
	}
	else if (Input.ShaderFormat == NAME_SF_METAL_SM4)
	{
		AdditionalDefines.SetDefine(TEXT("METAL_SM4_PROFILE"), 1);
		AdditionalDefines.SetDefine(TEXT("USING_VERTEX_SHADER_LAYER"), 1);
		StandardPlatform = TEXT("osx");
		StandardVersion = TEXT("1.1");
		VersionEnum = 1;
		MetalCompilerTarget = HCT_FeatureLevelSM4;
		Semantics = EMetalGPUSemanticsImmediateDesktop;
	}
	else if (Input.ShaderFormat == NAME_SF_METAL_SM5)
	{
		AdditionalDefines.SetDefine(TEXT("METAL_SM5_PROFILE"), 1);
		AdditionalDefines.SetDefine(TEXT("USING_VERTEX_SHADER_LAYER"), 1);
        StandardPlatform = TEXT("osx");
		VersionEnum = StandardVersion ? VersionEnum : 1;
		VersionEnum = VersionEnum > 0 ? VersionEnum : 1;
        StandardVersion = VersionEnum > 1 ? StandardVersion : TEXT("1.1"); // May require SHADER_LANGUAGE_VERSION for fragment UAVs and/or tessellation.
		MetalCompilerTarget = HCT_FeatureLevelSM5;
		Semantics = EMetalGPUSemanticsImmediateDesktop;
	}
	else if (Input.ShaderFormat == NAME_SF_METAL_MRT_MAC)
	{
		AdditionalDefines.SetDefine(TEXT("METAL_MRT_PROFILE"), 1);
		AdditionalDefines.SetDefine(TEXT("FORCE_FLOATS"), 1); // Force floats to avoid radr://24884199 & radr://24884860
		StandardPlatform = TEXT("osx");
		VersionEnum = StandardVersion ? VersionEnum : 1;
		VersionEnum = VersionEnum > 0 ? VersionEnum : 1;
		StandardVersion = VersionEnum > 1 ? StandardVersion : TEXT("1.1"); // May require SHADER_LANGUAGE_VERSION for fragment UAVs and/or tessellation.
		MetalCompilerTarget = HCT_FeatureLevelSM5;
		Semantics = EMetalGPUSemanticsTBDRDesktop;
	}
	else
	{
		Output.bSucceeded = false;
		new(Output.Errors) FShaderCompilerError(*FString::Printf(TEXT("Invalid shader format '%s' passed to compiler."), *Input.ShaderFormat.ToString()));
		return;
	}
	
	FString Standard = FString::Printf(TEXT("-std=%s-metal%s"), StandardPlatform, StandardVersion);
	
	const bool bDumpDebugInfo = (Input.DumpDebugInfoPath != TEXT("") && IFileManager::Get().DirectoryExists(*Input.DumpDebugInfoPath));

	if(Input.Environment.CompilerFlags.Contains(CFLAG_AvoidFlowControl) || PlatformFlowControl == CFLAG_AvoidFlowControl)
	{
		AdditionalDefines.SetDefine(TEXT("COMPILER_SUPPORTS_ATTRIBUTES"), (uint32)1);
	}
	else
	{
		AdditionalDefines.SetDefine(TEXT("COMPILER_SUPPORTS_ATTRIBUTES"), (uint32)0);
	}

	FString const* UsingTessellationDefine = Input.Environment.GetDefinitions().Find(TEXT("USING_TESSELLATION"));
	bool bUsingTessellation = (UsingTessellationDefine != nullptr && FString("1") == *UsingTessellationDefine);
	if (bUsingTessellation && (Input.Target.Frequency == SF_Vertex))
	{
		// force HULLSHADER on so that VS that is USING_TESSELLATION can be built together with the proper HS
		FString const* VertexShaderDefine = Input.Environment.GetDefinitions().Find(TEXT("VERTEXSHADER"));
		check(VertexShaderDefine && FString("1") == *VertexShaderDefine);
		FString const* HullShaderDefine = Input.Environment.GetDefinitions().Find(TEXT("HULLSHADER"));
		check(HullShaderDefine && FString("0") == *HullShaderDefine);
		Input.Environment.SetDefine(TEXT("HULLSHADER"), 1u);
	}
	if (Input.Target.Frequency == SF_Hull)
	{
		check(bUsingTessellation);
		// force VERTEXSHADER on so that HS that is USING_TESSELLATION can be built together with the proper VS
		FString const* VertexShaderDefine = Input.Environment.GetDefinitions().Find(TEXT("VERTEXSHADER"));
		check(VertexShaderDefine && FString("0") == *VertexShaderDefine);
		FString const* HullShaderDefine = Input.Environment.GetDefinitions().Find(TEXT("HULLSHADER"));
		check(HullShaderDefine && FString("1") == *HullShaderDefine);

		// enable VERTEXSHADER so that this HS will hash uniquely with its associated VS
		// We do not want a given HS to be shared among numerous VS'Sampler
		// this should accomplish that goal -- see GenerateOutputHash
		Input.Environment.SetDefine(TEXT("VERTEXSHADER"), 1u);
	}

	if (PreprocessShader(PreprocessedShader, Output, Input, AdditionalDefines))
	{
		// Disable instanced stereo until supported for metal
		StripInstancedStereo(PreprocessedShader);

		char* MetalShaderSource = NULL;
		char* ErrorLog = NULL;

		const EHlslShaderFrequency Frequency = HlslCompilerTarget < HCT_FeatureLevelSM5 ? FrequencyTable1[Input.Target.Frequency] : FrequencyTable2[Input.Target.Frequency];
		if (Frequency == HSF_InvalidFrequency)
		{
			Output.bSucceeded = false;
			FShaderCompilerError* NewError = new(Output.Errors) FShaderCompilerError();
			NewError->StrippedErrorMessage = FString::Printf(
				TEXT("%s shaders not supported for use in Metal."),
				CrossCompiler::GetFrequencyName((EShaderFrequency)Input.Target.Frequency)
				);
			return;
		}


		// This requires removing the HLSLCC_NoPreprocess flag later on!
		if (!RemoveUniformBuffersFromSource(PreprocessedShader))
		{
			return;
		}

		// Write out the preprocessed file and a batch file to compile it if requested (DumpDebugInfoPath is valid)
		if (bDumpDebugInfo)
		{
			FArchive* FileWriter = IFileManager::Get().CreateFileWriter(*(Input.DumpDebugInfoPath / Input.SourceFilename + TEXT(".usf")));
			if (FileWriter)
			{
				auto AnsiSourceFile = StringCast<ANSICHAR>(*PreprocessedShader);
				FileWriter->Serialize((ANSICHAR*)AnsiSourceFile.Get(), AnsiSourceFile.Length());
				FileWriter->Close();
				delete FileWriter;
			}

			if (Input.bGenerateDirectCompileFile)
			{
				FFileHelper::SaveStringToFile(CreateShaderCompilerWorkerDirectCommandLine(Input), *(Input.DumpDebugInfoPath / TEXT("DirectCompile.txt")));
			}
		}

		uint32 CCFlags = HLSLCC_NoPreprocess | HLSLCC_PackUniforms | HLSLCC_FixAtomicReferences;

		if (bDumpDebugInfo)
		{
			const FString MetalFile = (Input.DumpDebugInfoPath / TEXT("Output.metal"));
			const FString USFFile = (Input.DumpDebugInfoPath / Input.SourceFilename) + TEXT(".usf");
			const FString CCBatchFileContents = CreateCommandLineHLSLCC(USFFile, MetalFile, *Input.EntryPointName, MetalCompilerTarget, Frequency, CCFlags);
			if (!CCBatchFileContents.IsEmpty())
			{
				FFileHelper::SaveStringToFile(CCBatchFileContents, *(Input.DumpDebugInfoPath / TEXT("CrossCompile.bat")));
			}
		}

		// Required as we added the RemoveUniformBuffersFromSource() function (the cross-compiler won't be able to interpret comments w/o a preprocessor)
		CCFlags &= ~HLSLCC_NoPreprocess;

		bool const bZeroInitialise = Input.Environment.CompilerFlags.Contains(CFLAG_ZeroInitialise);
		bool const bBoundsChecks = Input.Environment.CompilerFlags.Contains(CFLAG_BoundsChecking);
		
		FMetalTessellationOutputs Attribs;
		FMetalCodeBackend MetalBackEnd(Attribs, CCFlags, MetalCompilerTarget, VersionEnum, Semantics, bZeroInitialise, bBoundsChecks);
		FMetalLanguageSpec MetalLanguageSpec;

		int32 Result = 0;
		FHlslCrossCompilerContext CrossCompilerContext(CCFlags, Frequency, HlslCompilerTarget);
		if (CrossCompilerContext.Init(TCHAR_TO_ANSI(*Input.SourceFilename), &MetalLanguageSpec))
		{
			Result = CrossCompilerContext.Run(
				TCHAR_TO_ANSI(*PreprocessedShader),
				TCHAR_TO_ANSI(*Input.EntryPointName),
				&MetalBackEnd,
				&MetalShaderSource,
				&ErrorLog
				) ? 1 : 0;
		}

		uint32 CRCLen = MetalShaderSource ? (uint32)FCStringAnsi::Strlen(MetalShaderSource) : 0u;
		uint32 CRC = CRCLen ? FCrc::MemCrc_DEPRECATED(MetalShaderSource, CRCLen) : 0u;
		uint32 SourceLen = CRCLen;
		if(MetalShaderSource)
		{
			ANSICHAR* Main = FCStringAnsi::Strstr(MetalShaderSource, "Main_00000000_00000000");
			check(Main);
			
			ANSICHAR MainCRC[24];
			int32 NewLen = FCStringAnsi::Snprintf(MainCRC, 24, "Main_%0.8x_%0.8x", CRCLen, CRC);
			FMemory::Memcpy(Main, MainCRC, NewLen);
		
			uint32 Len = FCStringAnsi::Strlen(TCHAR_TO_ANSI(*Input.SourceFilename)) + FCStringAnsi::Strlen(TCHAR_TO_ANSI(*Input.DebugGroupName)) + FCStringAnsi::Strlen(TCHAR_TO_ANSI(*Input.EntryPointName)) + FCStringAnsi::Strlen(MetalShaderSource) + 21;
			char* Dest = (char*)malloc(Len);
			FCStringAnsi::Snprintf(Dest, Len, "// ! %s/%s.usf:%s\n%s", (const char*)TCHAR_TO_ANSI(*Input.DebugGroupName), (const char*)TCHAR_TO_ANSI(*Input.SourceFilename), (const char*)TCHAR_TO_ANSI(*Input.EntryPointName), (const char*)MetalShaderSource);
			free(MetalShaderSource);
			MetalShaderSource = Dest;
			SourceLen = (uint32)FCStringAnsi::Strlen(MetalShaderSource);
		}
		if (bDumpDebugInfo)
		{
			if (SourceLen > 0u)
			{
				FArchive* FileWriter = IFileManager::Get().CreateFileWriter(*(Input.DumpDebugInfoPath / Input.SourceFilename + TEXT(".metal")));
				if (FileWriter)
				{
					FileWriter->Serialize(MetalShaderSource, SourceLen + 1);
					FileWriter->Close();
					delete FileWriter;
				}
			}
		}

		if (Result != 0)
		{
			Output.Target = Input.Target;
			BuildMetalShaderOutput(Output, Input, MetalShaderSource, SourceLen, CRCLen, CRC, VersionEnum, *Standard, Output.Errors, Attribs);
		}
		else
		{
			FString Tmp = ANSI_TO_TCHAR(ErrorLog);
			TArray<FString> ErrorLines;
			Tmp.ParseIntoArray(ErrorLines, TEXT("\n"), true);
			for (int32 LineIndex = 0; LineIndex < ErrorLines.Num(); ++LineIndex)
			{
				const FString& Line = ErrorLines[LineIndex];
				CrossCompiler::ParseHlslccError(Output.Errors, Line);
			}
		}

		if (MetalShaderSource)
		{
			free(MetalShaderSource);
		}
		if (ErrorLog)
		{
			free(ErrorLog);
		}
	}
}

void StripShader_Metal(TArray<uint8>& Code, class FString const& DebugPath, bool const bNative)
{
	FShaderCodeReader ShaderCode(Code);
	FMemoryReader Ar(Code, true);
	Ar.SetLimitSize(ShaderCode.GetActualShaderCodeSize());
	
	// was the shader already compiled offline?
	uint8 OfflineCompiledFlag;
	Ar << OfflineCompiledFlag;
	check(OfflineCompiledFlag == 0 || OfflineCompiledFlag == 1);
	check(bNative == false || OfflineCompiledFlag == 1);
	
	// get the header
	FMetalCodeHeader Header = { 0 };
	Ar << Header;
	
	// Must be compiled for archiving or something is very wrong.
	check(Header.CompileFlags & (1 << CFLAG_Archive));
	
	// remember where the header ended and code (precompiled or source) begins
	int32 CodeOffset = Ar.Tell();
	const uint8* SourceCodePtr = (uint8*)Code.GetData() + CodeOffset;
	
	// Copy the non-optional shader bytecode
	TArray<uint8> SourceCode;
	SourceCode.Append(SourceCodePtr, ShaderCode.GetActualShaderCodeSize() - CodeOffset);
	
	const ANSICHAR* ShaderSource = ShaderCode.FindOptionalData('c');
	bool const bHasShaderSource = (ShaderSource && FCStringAnsi::Strlen(ShaderSource) > 0);
	
	const ANSICHAR* ShaderPath = ShaderCode.FindOptionalData('p');
	bool const bHasShaderPath = (ShaderPath && FCStringAnsi::Strlen(ShaderPath) > 0);
	
	if (bHasShaderSource && bHasShaderPath)
	{
		FString DebugFilePath = DebugPath / FString(ShaderPath);
		FString DebugFolderPath = FPaths::GetPath(DebugFilePath);
		if (IFileManager::Get().MakeDirectory(*DebugFolderPath, true))
		{
			FString TempPath = FPaths::CreateTempFilename(*DebugFolderPath, TEXT("MetalShaderFile-"), TEXT(".metal"));
			FFileHelper::SaveStringToFile(FString(ShaderSource), *TempPath);
			IFileManager::Get().Move(*DebugFilePath, *TempPath, false, false, true, false);
			IFileManager::Get().Delete(*TempPath);
		}
	}
	
	if (bNative)
	{
		int32 ObjectSize = 0;
		const uint8* ShaderObject = ShaderCode.FindOptionalDataAndSize('o', ObjectSize);
		check(ShaderObject && ObjectSize);

		TArray<uint8> ObjectCodeArray;
		ObjectCodeArray.Append(ShaderObject, ObjectSize);
		SourceCode = ObjectCodeArray;
	}
	
	// Strip any optional data
	if (bNative || ShaderCode.GetOptionalDataSize() > 0)
	{
		// This is going to get serialised into the shader resource archive we don't anything but the header info now with the archive flag set
		Header.CompileFlags |= (1 << CFLAG_Archive);
		
		// Write out the header and compiled shader code
		FShaderCode NewCode;
		FMemoryWriter NewAr(NewCode.GetWriteAccess(), true);
		NewAr << OfflineCompiledFlag;
		NewAr << Header;
		
		// jam it into the output bytes
		NewAr.Serialize(SourceCode.GetData(), SourceCode.Num());
		
		Code = NewCode.GetReadAccess();
	}
}

static EShaderPlatform MetalShaderFormatToLegacyShaderPlatform(FName ShaderFormat)
{
	static FName NAME_SF_METAL(TEXT("SF_METAL"));
	static FName NAME_SF_METAL_MRT(TEXT("SF_METAL_MRT"));
	static FName NAME_SF_METAL_SM4(TEXT("SF_METAL_SM4"));
	static FName NAME_SF_METAL_SM5(TEXT("SF_METAL_SM5"));
	static FName NAME_SF_METAL_MRT_MAC(TEXT("SF_METAL_MRT_MAC"));
	static FName NAME_SF_METAL_MACES3_1(TEXT("SF_METAL_MACES3_1"));
	static FName NAME_SF_METAL_MACES2(TEXT("SF_METAL_MACES2"));
	
	if (ShaderFormat == NAME_SF_METAL)				return SP_METAL;
	if (ShaderFormat == NAME_SF_METAL_MRT)			return SP_METAL_MRT;
	if (ShaderFormat == NAME_SF_METAL_MRT_MAC)		return SP_METAL_MRT_MAC;
	if (ShaderFormat == NAME_SF_METAL_SM5)			return SP_METAL_SM5;
	if (ShaderFormat == NAME_SF_METAL_SM4)			return SP_METAL_SM4;
	if (ShaderFormat == NAME_SF_METAL_MACES3_1)		return SP_METAL_MACES3_1;
	if (ShaderFormat == NAME_SF_METAL_MACES2)		return SP_METAL_MACES2;
	
	return SP_NumPlatforms;
}

uint64 AppendShader_Metal(FName const& Format, FString const& WorkingDir, const FSHAHash& Hash, TArray<uint8>& InShaderCode)
{
	uint64 Id = 0;;
#if METAL_OFFLINE_COMPILE
#if PLATFORM_MAC
	EShaderPlatform Platform = MetalShaderFormatToLegacyShaderPlatform(Format);
	FString MetalPath = GetMetalBinaryPath(Platform);
	FString MetalToolsPath = GetMetalToolsPath(Platform);
	if (MetalPath.Len() > 0 && MetalToolsPath.Len() > 0)
	{
		if (IFileManager::Get().FileSize(*MetalPath) > 0)
		{
			// Parse the existing data and extract the source code. We have to recompile it
			FShaderCodeReader ShaderCode(InShaderCode);
			FMemoryReader Ar(InShaderCode, true);
			Ar.SetLimitSize(ShaderCode.GetActualShaderCodeSize());
			
			// was the shader already compiled offline?
			uint8 OfflineCompiledFlag;
			Ar << OfflineCompiledFlag;
			check(OfflineCompiledFlag == 0 || OfflineCompiledFlag == 1);
			
			// get the header
			FMetalCodeHeader Header = { 0 };
			Ar << Header;
			
			// Must be compiled for archiving or something is very wrong.
			check(Header.CompileFlags & (1 << CFLAG_Archive));
			
			// remember where the header ended and code (precompiled or source) begins
			int32 CodeOffset = Ar.Tell();
			const uint8* SourceCodePtr = (uint8*)InShaderCode.GetData() + CodeOffset;
			
			// Copy the non-optional shader bytecode
			int32 ObjectCodeDataSize = ShaderCode.GetActualShaderCodeSize() - CodeOffset;
			TArrayView<const uint8> ObjectCodeArray(SourceCodePtr, ObjectCodeDataSize);
			
			// Object code segment
			FString ObjFilename = WorkingDir / FString::Printf(TEXT("Main_%0.8x_%0.8x.o"), Header.SourceLen, Header.SourceCRC);
			
			bool const bHasObjectData = (ObjectCodeDataSize > 0) || IFileManager::Get().FileExists(*ObjFilename);
			if (bHasObjectData)
			{
				// metal commandlines
				int32 ReturnCode = 0;
				FString Results;
				FString Errors;
				
				bool bHasObjectFile = IFileManager::Get().FileExists(*ObjFilename);
				if (ObjectCodeDataSize > 0)
				{
					// write out shader object code source (IR) for archiving to a single library file later
					if( FFileHelper::SaveArrayToFile(ObjectCodeArray, *ObjFilename) )
					{
						bHasObjectFile = true;
					}
				}
				
				if (bHasObjectFile)
				{
					Id = ((uint64)Header.SourceLen << 32) | Header.SourceCRC;
					
					// This is going to get serialised into the shader resource archive we don't anything but the header info now with the archive flag set
					Header.CompileFlags |= (1 << CFLAG_Archive);
					
					// Write out the header and compiled shader code
					FShaderCode NewCode;
					FMemoryWriter NewAr(NewCode.GetWriteAccess(), true);
					NewAr << OfflineCompiledFlag;
					NewAr << Header;
					
					InShaderCode = NewCode.GetReadAccess();
					
					UE_LOG(LogShaders, Display, TEXT("Archiving succeeded: shader %s (Len: %0.8x, CRC: %0.8x, SHA: %s)"), *Header.ShaderName, Header.SourceLen, Header.SourceCRC, *Hash.ToString());
				}
				else
				{
					UE_LOG(LogShaders, Error, TEXT("Archiving failed: failed to write temporary file %s for shader %s (Len: %0.8x, CRC: %0.8x, SHA: %s)"), *ObjFilename, *Header.ShaderName, Header.SourceLen, Header.SourceCRC, *Hash.ToString());
				}
			}
			else
			{
				UE_LOG(LogShaders, Error, TEXT("Archiving failed: shader %s (Len: %0.8x, CRC: %0.8x, SHA: %s) has no object data"), *Header.ShaderName, Header.SourceLen, Header.SourceCRC, *Hash.ToString());
			}
		}
		else
		{
			UE_LOG(LogShaders, Error, TEXT("Archiving failed: no 'metal' path: %s"), *MetalPath);
		}
	}
	else
#endif
#endif
	{
		UE_LOG(LogShaders, Error, TEXT("Archiving failed: no Xcode install."));
	}
	return Id;
}

bool FinalizeLibrary_Metal(FName const& Format, FString const& WorkingDir, FString const& LibraryPath, TSet<uint64> const& Shaders, class FString const& DebugOutputDir)
{
	bool bOK = false;
	
#if METAL_OFFLINE_COMPILE
#if PLATFORM_MAC
	EShaderPlatform Platform = MetalShaderFormatToLegacyShaderPlatform(Format);
	FString MetalPath = GetMetalBinaryPath(Platform);
	FString MetalToolsPath = GetMetalToolsPath(Platform);
	if (MetalPath.Len() > 0 && MetalToolsPath.Len() > 0)
	{
		if (IFileManager::Get().FileSize(*MetalPath) > 0)
		{
			// metal commandlines
			int32 ReturnCode = 0;
			FString Results;
			FString Errors;
			
			FString ArchivePath = WorkingDir + TEXT(".metalar");
			
			IFileManager::Get().Delete(*ArchivePath);
			IFileManager::Get().Delete(*LibraryPath);
			
			UE_LOG(LogShaders, Display, TEXT("Archiving %d shaders for shader platform: %s"), Shaders.Num(), *Format.GetPlainNameString());
			
			int32 Index = 0;
			FString MetalArPath = MetalToolsPath + TEXT("/metal-ar");
			FString Params = FString::Printf(TEXT("q \"%s\""), *ArchivePath);
			
			for (auto Shader : Shaders)
			{
				uint32 Len = (Shader >> 32);
				uint32 CRC = (Shader & 0xffffffff);
				UE_LOG(LogShaders, Display, TEXT("[%d/%d] %s Main_%0.8x_%0.8x.o"), ++Index, Shaders.Num(), *Format.GetPlainNameString(), Len, CRC);
				FString Param = FString::Printf(TEXT(" \"%s/Main_%0.8x_%0.8x.o\""), *WorkingDir, Len, CRC);
				
				if (Params.Len() + Param.Len() + MetalArPath.Len() + 2 < (ARG_MAX / 2))
				{
					Params += Param;
				}
				else
				{
					FPlatformProcess::ExecProcess( *MetalArPath, *Params, &ReturnCode, &Results, &Errors );
					if (ReturnCode != 0 || IFileManager::Get().FileSize(*ArchivePath) <= 0)
					{
						UE_LOG(LogShaders, Error, TEXT("Archiving failed: metal-ar failed with code %d: %s"), ReturnCode, *Errors);
						Params.Empty();
						break;
					}
					else
					{
						Params = FString::Printf(TEXT("q \"%s\" \"%s/Main_%0.8x_%0.8x.o\""), *ArchivePath, *WorkingDir, Len, CRC);
					}
				}
			}
			
			if (!Params.IsEmpty())
			{
				FPlatformProcess::ExecProcess( *MetalArPath, *Params, &ReturnCode, &Results, &Errors );
				if (ReturnCode != 0 || IFileManager::Get().FileSize(*ArchivePath) <= 0)
				{
					UE_LOG(LogShaders, Error, TEXT("Archiving failed: metal-ar failed with code %d: %s"), ReturnCode, *Errors);
				}
			}
			
			// handle compile error
			if (ReturnCode == 0 && IFileManager::Get().FileSize(*ArchivePath) > 0)
			{
				UE_LOG(LogShaders, Display, TEXT("Post-processing archive for shader platform: %s"), *Format.GetPlainNameString());
				
				Params = FString::Printf(TEXT("-o=\"%s\" \"%s\""), *LibraryPath, *ArchivePath);
				FString MetalLibPath = MetalToolsPath + TEXT("/metallib");
				FPlatformProcess::ExecProcess( *MetalLibPath, *Params, &ReturnCode, &Results, &Errors );
				
				// handle compile error
				if (ReturnCode == 0 && IFileManager::Get().FileSize(*LibraryPath) > 0)
				{
					bOK = true;
				}
				else
				{
					UE_LOG(LogShaders, Error, TEXT("Archiving failed: metallib failed with code %d: %s"), ReturnCode, *Errors);
				}
			}
		}
		else
		{
			UE_LOG(LogShaders, Error, TEXT("Archiving failed: no 'metal' path: %s"), *MetalPath);
		}
	}
	else
#endif
#endif
	{
		UE_LOG(LogShaders, Error, TEXT("Archiving failed: no Xcode install."));
	}
	
#if METAL_OFFLINE_COMPILE
#if PLATFORM_MAC
	if(bOK)
	{
		//TODO add a check in here - this will only work if we have shader archiving with debug info set.
		
		//We want to archive all the metal shader source files so that they can be unarchived into a debug location
		//This allows the debugging of optimised metal shaders within the xcode tool set
		//Currently using the 'tar' system tool to create a compressed tape archive
		
		//Place the archive in the same position as the .metallib file
		FString CompressedPath = LibraryPath;
		
		//Strip an trailing path extension if it has one
		int32 TrailingSlashIdx;
		if(LibraryPath.FindLastChar('.', TrailingSlashIdx))
		{
			CompressedPath = LibraryPath.Mid( 0, TrailingSlashIdx );
		}
		
		//Add our preferred archive extension
		CompressedPath += ".tgz";
		
		//Due to the limitations of the 'tar' command and running through NSTask,
		//the most reliable way is to feed it a list of local file name (-T) with a working path set (-C)
		//if we built the list with absolute paths without -C then we'd get the full folder structure in the archive
		//I don't think we want this here
		
		//Build a file list that 'tar' can access
		const FString FileListPath = DebugOutputDir / TEXT("ArchiveInput.txt");
		IFileManager::Get().Delete( *FileListPath );
		
		{
			//Find the metal source files
			TArray<FString> FilesToArchive;
			IFileManager::Get().FindFilesRecursive( FilesToArchive, *DebugOutputDir, TEXT("*.metal"), true, false, false );
			
			//Write the local file names into the target file
			FArchive* FileListHandle = IFileManager::Get().CreateFileWriter( *FileListPath );
			if(FileListHandle)
			{
				const FString NewLine = TEXT("\n");
				
				const FString DebugDir = DebugOutputDir / *Format.GetPlainNameString();
				
				for(FString FileName : FilesToArchive)
				{
					FPaths::MakePathRelativeTo(FileName, *DebugDir);
					
					FString TextLine = FileName + NewLine;
					
					//We don't want the string to archive through the << operator otherwise we'd be creating a binary file - we need text
					auto AnsiFullPath = StringCast<ANSICHAR>( *TextLine );
					FileListHandle->Serialize( (ANSICHAR*)AnsiFullPath.Get(), AnsiFullPath.Length() );
				}
				
				//Clean up
				FileListHandle->Close();
				delete FileListHandle;
			}
		}
		
		//Setup the NSTask command and parameter list, Archive (-c) and Compress (-z) to target file (-f) the metal file list (-T) using a local dir in archive (-C).
		FString ArchiveCommand = TEXT("/usr/bin/tar");
		FString ArchiveCommandParams = FString::Printf( TEXT("czf \"%s\" -C \"%s\" -T \"%s\""), *CompressedPath, *DebugOutputDir, *FileListPath );
		
		int32 ReturnCode = 0;
		FString Result;
		FString Errors;
		
		//Execute command, this should end up with a .tgz file in the same location at the .metallib file
		FPlatformProcess::ExecProcess( *ArchiveCommand, *ArchiveCommandParams, &ReturnCode, &Result, &Errors );
		
		
		if(ReturnCode != 0)
		{
			UE_LOG(LogShaders, Error, TEXT("Archive Shader Source failed %d: %s"), ReturnCode, *Errors);
		}
	}
#endif
#endif
	
	return bOK;
}
