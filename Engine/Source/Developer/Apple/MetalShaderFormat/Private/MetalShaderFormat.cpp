// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MetalShaderFormat.h"
#include "ModuleInterface.h"
#include "ModuleManager.h"
#include "Interfaces/IShaderFormat.h"
#include "Interfaces/IShaderFormatModule.h"
#include "ShaderCore.h"
#include "IShaderFormatArchive.h"
#include "hlslcc.h"
#include "MetalShaderResources.h"
#include "HAL/FileManager.h"
#include "Serialization/Archive.h"

extern bool StripShader_Metal(TArray<uint8>& Code, class FString const& DebugPath, bool const bNative);
extern uint64 AppendShader_Metal(class FName const& Format, class FString const& ArchivePath, const FSHAHash& Hash, TArray<uint8>& Code);
extern bool FinalizeLibrary_Metal(class FName const& Format, class FString const& ArchivePath, class FString const& LibraryPath, TSet<uint64> const& Shaders, class FString const& DebugOutputDir);

static FName NAME_SF_METAL(TEXT("SF_METAL"));
static FName NAME_SF_METAL_MRT(TEXT("SF_METAL_MRT"));
static FName NAME_SF_METAL_SM4(TEXT("SF_METAL_SM4"));
static FName NAME_SF_METAL_SM5(TEXT("SF_METAL_SM5"));
static FName NAME_SF_METAL_MACES3_1(TEXT("SF_METAL_MACES3_1"));
static FName NAME_SF_METAL_MACES2(TEXT("SF_METAL_MACES2"));
static FName NAME_SF_METAL_MRT_MAC(TEXT("SF_METAL_MRT_MAC"));
static FString METAL_LIB_EXTENSION(TEXT(".metallib"));
static FString METAL_MAP_EXTENSION(TEXT(".metalmap"));

class FMetalShaderFormatArchive : public IShaderFormatArchive
{
public:
	FMetalShaderFormatArchive(FName InFormat, FString const& WorkingDirectory)
	: Format(InFormat)
	, WorkingDir(WorkingDirectory)
	{
		check(Format == NAME_SF_METAL || Format == NAME_SF_METAL_MRT || Format == NAME_SF_METAL_SM4 || Format == NAME_SF_METAL_SM5 || Format == NAME_SF_METAL_MACES3_1 || Format == NAME_SF_METAL_MACES2 || Format == NAME_SF_METAL_MRT_MAC);
		ArchivePath = (WorkingDir / Format.GetPlainNameString());
		IFileManager::Get().DeleteDirectory(*ArchivePath, false, true);
		IFileManager::Get().MakeDirectory(*ArchivePath);
		Map.Format = Format.GetPlainNameString();
	}
	
	virtual FName GetFormat( void ) const
	{
		return Format;
	}
	
	virtual bool AddShader( uint8 Frequency, const FSHAHash& Hash, TArray<uint8>& Code )
	{
		uint64 ShaderId = AppendShader_Metal(Format, ArchivePath, Hash, Code);
		if (ShaderId)
		{
			//add Id to our list of shaders processed successfully
			Shaders.Add(ShaderId);
			
			//Note code copy in the map is uncompressed
			Map.HashMap.Add(Hash, TPairInitializer<uint8, TArray<uint8>>(Frequency, Code));
		}
		return (ShaderId > 0);
	}
	
	virtual bool Finalize( FString OutputDir, FString DebugOutputDir, TArray<FString>* OutputFiles )
	{
		bool bOK = false;
		FString LibraryPath = (OutputDir / Format.GetPlainNameString()) + METAL_LIB_EXTENSION;
		
		if (FinalizeLibrary_Metal(Format, ArchivePath, LibraryPath, Shaders, DebugOutputDir))
		{
			FString BinaryShaderFile = (OutputDir / Format.GetPlainNameString()) + METAL_MAP_EXTENSION;
			FArchive* BinaryShaderAr = IFileManager::Get().CreateFileWriter(*BinaryShaderFile);
			if( BinaryShaderAr != NULL )
			{
				*BinaryShaderAr << Map;
				BinaryShaderAr->Flush();
				delete BinaryShaderAr;
				
				if (OutputFiles)
				{
					OutputFiles->Add(LibraryPath);
					OutputFiles->Add(BinaryShaderFile);
				}
				
				bOK = true;
			}
		}

		return bOK;
	}
	
public:
	virtual ~FMetalShaderFormatArchive() { }
	
private:
	FName Format;
	FString WorkingDir;
	FString ArchivePath;
	TSet<uint64> Shaders;
	TSet<FString> SourceFiles;
	FMetalShaderMap Map;
};

class FMetalShaderFormat : public IShaderFormat
{
	enum
	{
		HEADER_VERSION = 37,
	};
	
	struct FVersion
	{
		uint16 HLSLCCMinor		: 8;
		uint16 Format			: 7;
		uint16 OfflineCompiled	: 1;
	};
public:
	virtual uint16 GetVersion(FName Format) const override final
	{
		static_assert(sizeof(FVersion) == sizeof(uint16), "Out of bits!");
		union
		{
			FVersion Version;
			uint16 Raw;
		} Version;

		Version.Version.Format = HEADER_VERSION;
		Version.Version.HLSLCCMinor = HLSLCC_VersionMinor;
		Version.Version.OfflineCompiled = METAL_OFFLINE_COMPILE;
		
		// Check that we didn't overwrite any bits
		check(Version.Version.Format == HEADER_VERSION);
		check(Version.Version.HLSLCCMinor == HLSLCC_VersionMinor);
		check(Version.Version.OfflineCompiled == METAL_OFFLINE_COMPILE);

		return Version.Raw;
	}
	virtual void GetSupportedFormats(TArray<FName>& OutFormats) const override final
	{
		OutFormats.Add(NAME_SF_METAL);
		OutFormats.Add(NAME_SF_METAL_MRT);
		OutFormats.Add(NAME_SF_METAL_SM4);
		OutFormats.Add(NAME_SF_METAL_SM5);
		OutFormats.Add(NAME_SF_METAL_MACES3_1);
		OutFormats.Add(NAME_SF_METAL_MACES2);
		OutFormats.Add(NAME_SF_METAL_MRT_MAC);
	}
	virtual void CompileShader(FName Format, const struct FShaderCompilerInput& Input, struct FShaderCompilerOutput& Output,const FString& WorkingDirectory) const override final
	{
		check(Format == NAME_SF_METAL || Format == NAME_SF_METAL_MRT || Format == NAME_SF_METAL_SM4 || Format == NAME_SF_METAL_SM5 || Format == NAME_SF_METAL_MACES3_1 || Format == NAME_SF_METAL_MACES2 || Format == NAME_SF_METAL_MRT_MAC);
		CompileShader_Metal(Input, Output, WorkingDirectory);
	}
	virtual bool CanStripShaderCode(void) const override final
	{
		return true;
	}
	virtual bool StripShaderCode( TArray<uint8>& Code, FString const& DebugOutputDir, bool const bNative ) const override final
	{
		return StripShader_Metal(Code, DebugOutputDir, bNative);
    }
    virtual class IShaderFormatArchive* CreateShaderArchive( FName Format, const FString& WorkingDirectory ) const
    {
        return new FMetalShaderFormatArchive(Format, WorkingDirectory);
    }
};

/**
 * Module for OpenGL shaders
 */

static IShaderFormat* Singleton = NULL;

class FMetalShaderFormatModule : public IShaderFormatModule
{
public:
	virtual ~FMetalShaderFormatModule()
	{
		delete Singleton;
		Singleton = NULL;
	}
	virtual IShaderFormat* GetShaderFormat()
	{
		if (!Singleton)
		{
			Singleton = new FMetalShaderFormat();
		}
		return Singleton;
	}
};

IMPLEMENT_MODULE( FMetalShaderFormatModule, MetalShaderFormat);
