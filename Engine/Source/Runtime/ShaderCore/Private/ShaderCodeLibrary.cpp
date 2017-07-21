// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ShaderCodeLibrary.cpp: Bound shader state cache implementation.
=============================================================================*/

#include "ShaderCodeLibrary.h"
#include "Shader.h"
#include "Misc/SecureHash.h"
#include "Misc/Paths.h"
#include "Math/UnitConversion.h"
#include "HAL/FileManagerGeneric.h"

#include "IShaderFormatArchive.h"

#if WITH_EDITORONLY_DATA
#include "Modules/ModuleManager.h"
#include "IShaderFormat.h"
#include "IShaderFormatModule.h"
#include "Interfaces/ITargetPlatform.h"
#include "Interfaces/ITargetPlatformManagerModule.h"
#endif

static const ECompressionFlags ShaderLibraryCompressionFlag = ECompressionFlags::COMPRESS_ZLIB;

static FString GetShaderCodeFilename(const FString& BaseDir, FName Platform)
{
	return BaseDir / TEXT("ShaderCode-") + Platform.ToString();
}

struct FShaderArchiveEntry
{
	uint32 Offset;
	uint32 Size;
	uint32 UncompressedSize;
	uint8 Frequency;
};

static FArchive& operator <<(FArchive& Ar, FShaderArchiveEntry& Ref)
{
	return Ar << Ref.Offset << Ref.Size << Ref.UncompressedSize << Ref.Frequency;
}

static TArray<uint8>& FShaderLibraryHelperUncompressCode(EShaderPlatform Platform, uint32 UncompressedSize, TArray<uint8>& Code, TArray<uint8>& UncompressedCode)
{
	if (RHISupportsShaderCompression(Platform) && Code.Num() != UncompressedSize)
	{
		UncompressedCode.SetNum(UncompressedSize);
		bool bSucceed = FCompression::UncompressMemory(ShaderLibraryCompressionFlag, UncompressedCode.GetData(), UncompressedSize, Code.GetData(), Code.Num());
		check(bSucceed);
		return UncompressedCode;
	}
	else
	{
		return Code;
	}
}

static void FShaderLibraryHelperCompressCode(EShaderPlatform Platform, const TArray<uint8>& UncompressedCode, TArray<uint8>& CompressedCode)
{
	if (RHISupportsShaderCompression(Platform))
	{
		auto CompressedSize = CompressedCode.Num();
		if (FCompression::CompressMemory(ShaderLibraryCompressionFlag, CompressedCode.GetData(), CompressedSize, UncompressedCode.GetData(), UncompressedCode.Num()))
		{
			CompressedCode.SetNum(CompressedSize);
		}
		else
		{
			CompressedCode = UncompressedCode;
		}
		CompressedCode.Shrink();
	}
	else
	{
		CompressedCode = UncompressedCode;
	}
}

struct FShaderCodeEntry
{
	FString FileName;
	EShaderFrequency Frequency;
};

class FShaderCodeArchive final : public FShaderFactoryInterface
{
public:
	FShaderCodeArchive(EShaderPlatform InPlatform, FString const& Filename)
	: FShaderFactoryInterface(InPlatform)
	, FilePath(Filename)
	{
		TArray<FString> ShaderFiles;
		IFileManager::Get().FindFiles(ShaderFiles, *Filename, TEXT("*.ushaderbytecode"));
		for (FString const& FileName : ShaderFiles)
		{
			if (FileName.Len() > 2 && FileName[1] == TEXT('_'))
			{
				FShaderCodeEntry Entry;
				TCHAR FreqChar[2] = {0, 0};
				FreqChar[0] = FileName[0];
				Entry.Frequency = (EShaderFrequency)FCStringWide::Atoi(FreqChar);
				check(Entry.Frequency < SF_NumFrequencies);
				Entry.FileName = FileName;
				
				FSHAHash Hash;
				FString Name = FPaths::GetBaseFilename(FileName);
				HexToBytes(Name.RightChop(2), Hash.Hash);
				
				Shaders.Add(Hash, Entry);
			}
		}
		
		UE_LOG(LogShaders, Display, TEXT("Using %s for material shader code"), *Filename);
	}
	
	virtual ~FShaderCodeArchive()
	{
	}
	
	virtual bool IsLibraryNativeFormat() const {return false;}
	
	bool LookupShaderCode(uint8 Frequency, const FSHAHash& Hash, TArray<uint8>& OutCode, uint32& OutSize)
	{
		FShaderCodeEntry* Entry = Shaders.Find(Hash);
		if (Entry)
		{
			FString Path = FilePath / Entry->FileName;
			FArchive* Ar = IFileManager::Get().CreateFileReader(*Path);
			if (Ar)
			{
				*Ar << OutSize;
				*Ar << OutCode;
				Ar->Close();
				return true;
			}
		}
		return false;
	}
	
	FPixelShaderRHIRef CreatePixelShader(const FSHAHash& Hash) override final
	{
		FPixelShaderRHIRef Shader;
		TArray<uint8> Code;
		uint32 Size = 0;
		if (LookupShaderCode(SF_Pixel, Hash, Code, Size))
		{
			TArray<uint8> UCode;
			TArray<uint8>& UncompressedCode = FShaderLibraryHelperUncompressCode(Platform, Size, Code, UCode);
			Shader = RHICreatePixelShader(UncompressedCode);
		}
		return Shader;
	}
	
	FVertexShaderRHIRef CreateVertexShader(const FSHAHash& Hash) override final
	{
		FVertexShaderRHIRef Shader;
		TArray<uint8> Code;
		uint32 Size = 0;
		if (LookupShaderCode(SF_Vertex, Hash, Code, Size))
		{
			TArray<uint8> UCode;
			TArray<uint8>& UncompressedCode = FShaderLibraryHelperUncompressCode(Platform, Size, Code, UCode);
			Shader = RHICreateVertexShader(UncompressedCode);
		}
		return Shader;
	}
	
	FHullShaderRHIRef CreateHullShader(const FSHAHash& Hash) override final
	{
		FHullShaderRHIRef Shader;
		TArray<uint8> Code;
		uint32 Size = 0;
		if (LookupShaderCode(SF_Hull, Hash, Code, Size))
		{
			TArray<uint8> UCode;
			TArray<uint8>& UncompressedCode = FShaderLibraryHelperUncompressCode(Platform, Size, Code, UCode);
			Shader = RHICreateHullShader(UncompressedCode);
		}
		return Shader;
	}
	
	FDomainShaderRHIRef CreateDomainShader(const FSHAHash& Hash) override final
	{
		FDomainShaderRHIRef Shader;
		TArray<uint8> Code;
		uint32 Size = 0;
		if (LookupShaderCode(SF_Domain, Hash, Code, Size))
		{
			TArray<uint8> UCode;
			TArray<uint8>& UncompressedCode = FShaderLibraryHelperUncompressCode(Platform, Size, Code, UCode);
			Shader = RHICreateDomainShader(UncompressedCode);
		}
		return Shader;
	}
	
	FGeometryShaderRHIRef CreateGeometryShader(const FSHAHash& Hash) override final
	{
		FGeometryShaderRHIRef Shader;
		TArray<uint8> Code;
		uint32 Size = 0;
		if (LookupShaderCode(SF_Geometry, Hash, Code, Size))
		{
			TArray<uint8> UCode;
			TArray<uint8>& UncompressedCode = FShaderLibraryHelperUncompressCode(Platform, Size, Code, UCode);
			Shader = RHICreateGeometryShader(UncompressedCode);
		}
		return Shader;
	}
	
	FGeometryShaderRHIRef CreateGeometryShaderWithStreamOutput(const FSHAHash& Hash, const FStreamOutElementList& ElementList, uint32 NumStrides, const uint32* Strides, int32 RasterizedStream) override final
	{
		FGeometryShaderRHIRef Shader;
		TArray<uint8> Code;
		uint32 Size = 0;
		if (LookupShaderCode(SF_Geometry, Hash, Code, Size))
		{
			TArray<uint8> UCode;
			TArray<uint8>& UncompressedCode = FShaderLibraryHelperUncompressCode(Platform, Size, Code, UCode);
			Shader = RHICreateGeometryShaderWithStreamOutput(UncompressedCode, ElementList, NumStrides, Strides, RasterizedStream);
		}
		return Shader;
	}
	
	FComputeShaderRHIRef CreateComputeShader(const FSHAHash& Hash) override final
	{
		FComputeShaderRHIRef Shader;
		TArray<uint8> Code;
		uint32 Size = 0;
		if (LookupShaderCode(SF_Compute, Hash, Code, Size))
		{
			TArray<uint8> UCode;
			TArray<uint8>& UncompressedCode = FShaderLibraryHelperUncompressCode(Platform, Size, Code, UCode);
			Shader = RHICreateComputeShader(UncompressedCode);
		}
		return Shader;
	}
	
	class FShaderCodeLibraryIterator : public FRHIShaderLibrary::FShaderLibraryIterator
	{
	public:
		FShaderCodeLibraryIterator(FShaderCodeArchive* Owner, EShaderPlatform Plat, TMap<FSHAHash, FShaderCodeEntry>::TIterator It)
		: FRHIShaderLibrary::FShaderLibraryIterator(Owner)
		, Platform(Plat)
		, IteratorImpl(It)
		{}
		
		virtual bool IsValid() const final override
		{
			return !!IteratorImpl;
		}
		
		virtual FRHIShaderLibrary::FShaderLibraryEntry operator*() const final override
		{
			FRHIShaderLibrary::FShaderLibraryEntry Entry;
			TPair<FSHAHash, FShaderCodeEntry> const& Pair = *IteratorImpl;
			Entry.Hash = Pair.Key;
			Entry.Frequency = Pair.Value.Frequency;
			Entry.Platform = Platform;
			return Entry;
		}
		
		virtual FRHIShaderLibrary::FShaderLibraryIterator& operator++() final override
		{
			++IteratorImpl;
			return *this;
		}
		
	private:
		EShaderPlatform Platform;
		TMap<FSHAHash, FShaderCodeEntry>::TIterator IteratorImpl;
	};
	
	virtual TRefCountPtr<FRHIShaderLibrary::FShaderLibraryIterator> CreateIterator(void) override final
	{
		return new FShaderCodeLibraryIterator(this, Platform, Shaders.CreateIterator());
	}
	
	TSet<FShaderCodeLibraryPipeline> const* GetShaderPipelines(EShaderPlatform InPlatform)
	{
		if (Pipelines.Num() == 0 && IsOpenGLPlatform(Platform))
		{
			TArray<FString> PipelineNames;
			IFileManager::Get().FindFiles(PipelineNames, *FilePath, TEXT("*.ushaderpipeline"));
			for (FString const& Name : PipelineNames)
			{
				FString Path = FilePath / Name;
				FArchive* Ar = IFileManager::Get().CreateFileReader(*Path);
				if (Ar)
				{
					FShaderCodeLibraryPipeline Pipeline;
					*Ar << Pipeline;
					Ar->Close();
					Pipelines.Add(Pipeline);
				}
			}
		}
		
		return &Pipelines;
	}
	
	virtual uint32 GetShaderCount(void) const override final
	{
		return Shaders.Num();
	}
	
private:
	// The path to the library
	FString FilePath;
	
	// The shader files present in the library
	TMap<FSHAHash, FShaderCodeEntry> Shaders;
	
	// De-serialised pipeline map
	TSet<FShaderCodeLibraryPipeline> Pipelines;
};

#if WITH_EDITOR
//TODO: Looks like a copy from private ShaderCompiler.cpp - need to move this and FindShaderFormat below maybe into IShaderFormat helper functions
static const TArray<const IShaderFormat*>& GetShaderFormats()
{
	static bool bInitialized = false;
	static TArray<const IShaderFormat*> Results;
	
	if (!bInitialized)
	{
		bInitialized = true;
		Results.Empty(Results.Num());
		
		TArray<FName> Modules;
		FModuleManager::Get().FindModules(SHADERFORMAT_MODULE_WILDCARD, Modules);
		
		if (!Modules.Num())
		{
			UE_LOG(LogShaders, Error, TEXT("No target shader formats found!"));
		}
		
		for (int32 Index = 0; Index < Modules.Num(); Index++)
		{
			IShaderFormat* Format = FModuleManager::LoadModuleChecked<IShaderFormatModule>(Modules[Index]).GetShaderFormat();
			if (Format != nullptr)
			{
				Results.Add(Format);
			}
		}
	}
	return Results;
}

static const IShaderFormat* FindShaderFormat(FName Name)
{
	const TArray<const IShaderFormat*>& ShaderFormats = GetShaderFormats();
	
	for (int32 Index = 0; Index < ShaderFormats.Num(); Index++)
	{
		TArray<FName> Formats;
		ShaderFormats[Index]->GetSupportedFormats(Formats);
		for (int32 FormatIndex = 0; FormatIndex < Formats.Num(); FormatIndex++)
		{
			if (Formats[FormatIndex] == Name)
			{
				return ShaderFormats[Index];
			}
		}
	}
	
	return nullptr;
}

struct FEditorShaderCodeArchive
{
	struct FEditorShaderCodeEntry
	{
		FString FileName;
		uint8 Frequency;
		TArray<uint8> Code;
		uint32 UncompressedSize;
	};
	
	FEditorShaderCodeArchive(FName InFormat)
	: FormatName(InFormat)
	, Format(nullptr)
	{
		Format = FindShaderFormat(InFormat);
		check(Format);
	}
	
	~FEditorShaderCodeArchive() {}
	
	FName GetFormat( void ) const
	{
		return FormatName;
	}
	
	bool AddShader( uint8 Frequency, const FSHAHash& Hash, TArray<uint8> const& InCode, uint32 const UncompressedSize )
	{
		bool bAdd = false;
		if (!Shaders.Contains(Hash))
		{
			uint8 Count = 0;
			for (uint8 i : InCode)
			{
				Count |= i;
			}
			check(Count > 0);
			
			FEditorShaderCodeEntry Entry;
			Entry.FileName = FString::Printf(TEXT("%d_%s.ushaderbytecode"), (uint32)Frequency, *Hash.ToString());
			Entry.Frequency = Frequency;
			Entry.Code = InCode;
			Entry.UncompressedSize = UncompressedSize;
			
			Shaders.Add(Hash, Entry);
			bAdd = true;
		}
		return bAdd;
	}
	
	bool AddPipeline(FShaderPipeline* Pipeline)
	{
		EShaderPlatform ShaderPlatform = ShaderFormatToLegacyShaderPlatform(FormatName);
		if (IsOpenGLPlatform(ShaderPlatform))
		{
			FShaderCodeLibraryPipeline LibraryPipeline;
			if (IsValidRef(Pipeline->VertexShader))
			{
				LibraryPipeline.VertexShader = Pipeline->VertexShader->GetOutputHash();
			}
			if (IsValidRef(Pipeline->GeometryShader))
			{
				LibraryPipeline.GeometryShader = Pipeline->GeometryShader->GetOutputHash();
			}
			if (IsValidRef(Pipeline->HullShader))
			{
				LibraryPipeline.HullShader = Pipeline->HullShader->GetOutputHash();
			}
			if (IsValidRef(Pipeline->DomainShader))
			{
				LibraryPipeline.DomainShader = Pipeline->DomainShader->GetOutputHash();
			}
			if (IsValidRef(Pipeline->PixelShader))
			{
				LibraryPipeline.PixelShader = Pipeline->PixelShader->GetOutputHash();
			}
			if (!Pipelines.Contains(LibraryPipeline))
			{
				Pipelines.Add(LibraryPipeline);
				return true;
			}
		}
		return false;
	}
	
	bool Finalize(FString OutputDir, FString DebugDir, bool bNativeFormat)
	{
		bool bSuccess = Shaders.Num() > 0;
		
		EShaderPlatform Platform = ShaderFormatToLegacyShaderPlatform(FormatName);
		FString OutputPath = GetShaderCodeFilename(OutputDir, FormatName);
		FString DebugPath = GetShaderCodeFilename(DebugDir, FormatName);
		
		IFileManager::Get().MakeDirectory(*DebugPath, true);

		for (TPair<FSHAHash, FEditorShaderCodeEntry>& Pair : Shaders)
		{
			// Write to a temporary file
			FString TempFilePath = FPaths::CreateTempFilename(*OutputPath, TEXT("ShaderCodeArchive-"));
			FArchive* FileWriter = IFileManager::Get().CreateFileWriter(*TempFilePath, FILEWRITE_NoFail);
			if (FileWriter)
			{
				check(Format);
				if (Format->CanStripShaderCode())
				{
					uint32 Size = Pair.Value.UncompressedSize;
					TArray<uint8> Code = Pair.Value.Code;
					
					TArray<uint8> UCode;
					TArray<uint8>& UncompressedCode = FShaderLibraryHelperUncompressCode(Platform, Size, Code, UCode);
					
					if(!Format->StripShaderCode(UncompressedCode, DebugPath, bNativeFormat))
					{
						bSuccess = false;
					}
					
					TArray<uint8> CCode;
					FShaderLibraryHelperCompressCode(Platform, UncompressedCode, CCode);
					
					Size = UncompressedCode.Num();
					
					*FileWriter << Size;
					*FileWriter << CCode;
				}
				else
				{
					*FileWriter << Pair.Value.UncompressedSize;
					*FileWriter << Pair.Value.Code;
				}
				FileWriter->Close();
				
				// As on POSIX only file moves on the same device are atomic
				FString DestFilePath = OutputPath / Pair.Value.FileName;
				IFileManager::Get().Move(*DestFilePath, *TempFilePath, false, false, true, true);
				IFileManager::Get().Delete(*TempFilePath);
			}
		}
		
		for (FShaderCodeLibraryPipeline& Pipeline : Pipelines)
		{
			// Write to a temporary file
			FString TempFilePath = FPaths::CreateTempFilename(*OutputPath, TEXT("ShaderCodeArchive-"));
			FArchive* FileWriter = IFileManager::Get().CreateFileWriter(*TempFilePath, FILEWRITE_NoFail);
			if (FileWriter)
			{
				*FileWriter << Pipeline;
				FileWriter->Close();

				// As on POSIX only file moves on the same device are atomic
				FString DestFilePath = OutputPath / FString::Printf(TEXT("%u.ushaderpipeline"), GetTypeHash(Pipeline));
				IFileManager::Get().Move(*DestFilePath, *TempFilePath, false, false, true, true);
				IFileManager::Get().Delete(*TempFilePath);
			}
		}
		
		return bSuccess;
	}
	
	bool PackageNativeShaderLibrary(const FString& ShaderCodeDir, const FString& DebugShaderCodeDir)
	{
		bool bOK = false;
		
		FString IntermediateFormatPath = GetShaderCodeFilename(FPaths::ProjectIntermediateDir(), FormatName);
		FString IntermediateCookedByteCodePath = IntermediateFormatPath / TEXT("NativeCookedByteCode");
		FString TempPath = IntermediateFormatPath / TEXT("NativeLibrary");
		
		EShaderPlatform Platform = ShaderFormatToLegacyShaderPlatform(FormatName);
		IShaderFormatArchive* Archive = Format->CreateShaderArchive(FormatName, TempPath);
		if (Archive)
		{
			FString OutputPath = GetShaderCodeFilename(ShaderCodeDir, FormatName);
			FString DebugPath = GetShaderCodeFilename(DebugShaderCodeDir, FormatName);
			bOK = true;
			
			//Collect previous native cooked bytecode files into this shader files processing directory - keep the rest of the code simpler, don't overwrite in case dest file is newer
			{
				TArray<FString> NativeShaderFiles;
				IFileManager::Get().FindFiles(NativeShaderFiles, *IntermediateCookedByteCodePath, TEXT("*.ushaderbytecode"));
				
				for (FString const& FileName : NativeShaderFiles)
				{
					if (FileName.Len() > 2 && FileName[1] == TEXT('_'))
					{
						IFileManager::Get().Move(*(OutputPath / FileName), *(IntermediateCookedByteCodePath / FileName), false);
					}
				}
			}
			
			TArray<FString> ShaderFiles;
			IFileManager::Get().FindFiles(ShaderFiles, *OutputPath, TEXT("*.ushaderbytecode"));
			
			for (FString const& FileName : ShaderFiles)
			{
				if (FileName.Len() > 2 && FileName[1] == TEXT('_'))
				{
					FShaderCodeEntry Entry;
					TCHAR FreqChar[2] = {0, 0};
					FreqChar[0] = FileName[0];
					Entry.Frequency = (EShaderFrequency)FCStringWide::Atoi(FreqChar);
					check(Entry.Frequency < SF_NumFrequencies);
					
					FSHAHash Hash;
					FString Name = FPaths::GetBaseFilename(FileName);
					HexToBytes(Name.RightChop(2), Hash.Hash);
					
					FString Path = OutputPath / FileName;
					FArchive* Ar = IFileManager::Get().CreateFileReader(*Path);
					if (Ar)
					{
						uint32 UncompressedSize = 0;
						TArray<uint8> CompressedCode;
						
						*Ar << UncompressedSize;
						*Ar << CompressedCode;
						
						Ar->Close();
						
						TArray<uint8> UCode;
						TArray<uint8>& UncompressedCode = FShaderLibraryHelperUncompressCode(Platform, UncompressedSize, CompressedCode, UCode);
						
						if(!Archive->AddShader(Entry.Frequency, Hash, UncompressedCode))
						{
							bOK = false;
							break;
						}
					}
					else
					{
						bOK = false;
						break;
					}
				}
			}
			
			if (bOK)
			{
				bOK = Archive->Finalize(ShaderCodeDir, DebugPath, nullptr);
				
				//Always delete debug directory
				IFileManager::Get().DeleteDirectory(*DebugShaderCodeDir, true, true);
				
				//Move files to intermediate dir for next iterative cook with overwwrite Move mode
				if (bOK)
				{
					for (FString const& FileName : ShaderFiles)
					{
						if (FileName.Len() > 2 && FileName[1] == TEXT('_'))
						{
							IFileManager::Get().Move(*(IntermediateCookedByteCodePath / FileName), *(OutputPath / FileName), true);
						}
					}
					
					//We don't want to keep the shader code library shader files for native cooked content
					IFileManager::Get().DeleteDirectory(*OutputPath, true, true);
				}
			}
		}
		return bOK;
	}
	
	FName FormatName;
	TMap<FSHAHash, FEditorShaderCodeEntry> Shaders;
	TSet<FShaderCodeLibraryPipeline> Pipelines;
	const IShaderFormat* Format;
};

struct FShaderCodeStats
{
	int64 ShadersSize;
	int64 ShadersUniqueSize;
	int32 NumShaders;
	int32 NumUniqueShaders;
	int32 NumPipelines;
	int32 NumUniquePipelines;
};
#endif //WITH_EDITOR

class FShaderCodeLibraryImpl
{
	// At runtime, shader code collection for current shader platform
	FRHIShaderLibraryRef ShaderCodeArchive;
#if WITH_EDITOR
	// At cook time, shader code collection for each shader platform
	FEditorShaderCodeArchive* EditorShaderCodeArchive[EShaderPlatform::SP_NumPlatforms];
	// At cook time, shader stats for each shader platform
	FShaderCodeStats EditorShaderCodeStats[EShaderPlatform::SP_NumPlatforms];
	// At cook time, whether the shader archive supports pipelines (only OpenGL should)
	bool EditorArchivePipelines[EShaderPlatform::SP_NumPlatforms];
#endif //WITH_EDITOR
	bool bSupportsPipelines;
	bool bNativeFormat;

public:
	FShaderCodeLibraryImpl(bool bInNativeFormat)
	: bSupportsPipelines(false)
	, bNativeFormat(bInNativeFormat)
	{
#if WITH_EDITOR
		FMemory::Memzero(EditorShaderCodeArchive);
		FMemory::Memzero(EditorShaderCodeStats);
		FMemory::Memzero(EditorArchivePipelines);
#endif
	}

	~FShaderCodeLibraryImpl()
	{
#if WITH_EDITOR
		for (uint32 i = 0; i < EShaderPlatform::SP_NumPlatforms; i++)
		{
			if (EditorShaderCodeArchive[i])
			{
				delete EditorShaderCodeArchive[i];
			}
		}
		FMemory::Memzero(EditorShaderCodeArchive);
#endif
	}

	// At runtime, open shader code collection for specified shader platform
	bool OpenShaderCode(const FString& ShaderCodeDir, EShaderPlatform ShaderPlatform)
	{
		ShaderCodeArchive = RHICreateShaderLibrary(ShaderPlatform, ShaderCodeDir);
		if(ShaderCodeArchive.IsValid())
		{
			bNativeFormat = true;
			
			UE_LOG(LogTemp, Display, TEXT("Cooked Context: Loaded Native Format Shared Shader Library"));
		}
		else
		{
			FString Filename = GetShaderCodeFilename(ShaderCodeDir, LegacyShaderPlatformToShaderFormat(ShaderPlatform));
			if (IFileManager::Get().DirectoryExists(*Filename))
			{
				ShaderCodeArchive = new FShaderCodeArchive(ShaderPlatform, Filename);
				bSupportsPipelines = (ShaderCodeArchive != nullptr);
				
				UE_LOG(LogTemp, Display, TEXT("Cooked Context: Using Shared Shader Library"));
			}
		}
		return IsValidRef(ShaderCodeArchive);
	}
	
	FVertexShaderRHIRef CreateVertexShader(EShaderPlatform Platform, FSHAHash Hash)
	{
		checkSlow(Platform == GetRuntimeShaderPlatform());
		
		if(bNativeFormat)
		{
			return RHICreateVertexShader(ShaderCodeArchive, Hash);
		}
		
		return ((FShaderCodeArchive*)ShaderCodeArchive.GetReference())->CreateVertexShader(Hash);
	}
	
	FPixelShaderRHIRef CreatePixelShader(EShaderPlatform Platform, FSHAHash Hash)
	{
		checkSlow(Platform == GetRuntimeShaderPlatform());
		
		if(bNativeFormat)
		{
			return RHICreatePixelShader(ShaderCodeArchive, Hash);
		}
		
		return ((FShaderCodeArchive*)ShaderCodeArchive.GetReference())->CreatePixelShader(Hash);
	}
	
	FGeometryShaderRHIRef CreateGeometryShader(EShaderPlatform Platform, FSHAHash Hash)
	{
		checkSlow(Platform == GetRuntimeShaderPlatform());
		
		if(bNativeFormat)
		{
			return RHICreateGeometryShader(ShaderCodeArchive, Hash);
		}
		
		return ((FShaderCodeArchive*)ShaderCodeArchive.GetReference())->CreateGeometryShader(Hash);
	}
	
	FGeometryShaderRHIRef CreateGeometryShaderWithStreamOutput(EShaderPlatform Platform, FSHAHash Hash, const FStreamOutElementList& ElementList, uint32 NumStrides, const uint32* Strides, int32 RasterizedStream)
	{
		checkSlow(Platform == GetRuntimeShaderPlatform());
		
		if(bNativeFormat)
		{
			return RHICreateGeometryShaderWithStreamOutput(ElementList, NumStrides, Strides, RasterizedStream, ShaderCodeArchive, Hash);
		}
		
		return ((FShaderCodeArchive*)ShaderCodeArchive.GetReference())->CreateGeometryShaderWithStreamOutput(Hash, ElementList, NumStrides, Strides, RasterizedStream);
	}
	
	FHullShaderRHIRef CreateHullShader(EShaderPlatform Platform, FSHAHash Hash)
	{
		checkSlow(Platform == GetRuntimeShaderPlatform());
		
		if(bNativeFormat)
		{
			return RHICreateHullShader(ShaderCodeArchive, Hash);
		}
		
		return ((FShaderCodeArchive*)ShaderCodeArchive.GetReference())->CreateHullShader(Hash);
	}
	
	FDomainShaderRHIRef CreateDomainShader(EShaderPlatform Platform, FSHAHash Hash)
	{
		checkSlow(Platform == GetRuntimeShaderPlatform());
		
		if(bNativeFormat)
		{
			return RHICreateDomainShader(ShaderCodeArchive, Hash);
		}
		
		return ((FShaderCodeArchive*)ShaderCodeArchive.GetReference())->CreateDomainShader(Hash);
	}
	
	FComputeShaderRHIRef CreateComputeShader(EShaderPlatform Platform, FSHAHash Hash)
	{
		checkSlow(Platform == GetRuntimeShaderPlatform());
		
		if(bNativeFormat)
		{
			return RHICreateComputeShader(ShaderCodeArchive, Hash);
		}
		
		return ((FShaderCodeArchive*)ShaderCodeArchive.GetReference())->CreateComputeShader(Hash);
	}
	
	TRefCountPtr<FRHIShaderLibrary::FShaderLibraryIterator> CreateIterator(void)
	{
		return ShaderCodeArchive->CreateIterator();
	}
	
	uint32 GetShaderCount(void)
	{
		return ShaderCodeArchive->GetShaderCount();
	}
	
	EShaderPlatform GetRuntimeShaderPlatform(void)
	{
		return ShaderCodeArchive->GetPlatform();
	}
	
	TSet<FShaderCodeLibraryPipeline> const* GetShaderPipelines(EShaderPlatform Platform)
	{
		if (bSupportsPipelines)
		{
			checkSlow(Platform == GetRuntimeShaderPlatform());
			return ((FShaderCodeArchive*)ShaderCodeArchive.GetReference())->GetShaderPipelines(Platform);
		}
		return nullptr;
	}

#if WITH_EDITOR
	void AddShaderCode(EShaderPlatform ShaderPlatform, EShaderFrequency Frequency, const FSHAHash& Hash, const TArray<uint8>& InCode, uint32 const UncompressedSize)
	{
		FShaderCodeStats& CodeStats = EditorShaderCodeStats[ShaderPlatform];
		CodeStats.NumShaders++;
		CodeStats.ShadersSize += InCode.Num();
		
		FEditorShaderCodeArchive* CodeArchive = EditorShaderCodeArchive[ShaderPlatform];
		if (!CodeArchive)
		{
			FName Format = LegacyShaderPlatformToShaderFormat(ShaderPlatform);
			CodeArchive = new FEditorShaderCodeArchive(Format);
			EditorShaderCodeArchive[ShaderPlatform] = CodeArchive;
			EditorArchivePipelines[ShaderPlatform] = IsOpenGLPlatform(ShaderPlatform);
		}
		check(CodeArchive);
		
		if (CodeArchive->AddShader((uint8)Frequency, Hash, InCode, UncompressedSize))
		{
			CodeStats.NumUniqueShaders++;
			CodeStats.ShadersUniqueSize += InCode.Num();
		}
	}
	
	bool AddShaderPipeline(FShaderPipeline* Pipeline)
	{
		check(Pipeline);
		
		EShaderPlatform ShaderPlatform = SP_NumPlatforms;
		for (uint8 Freq = 0; Freq < SF_Compute; Freq++)
		{
			FShader* Shader = Pipeline->GetShader((EShaderFrequency)Freq);
			if (Shader)
			{
				if (ShaderPlatform == SP_NumPlatforms)
				{
					ShaderPlatform = (EShaderPlatform)Shader->GetTarget().Platform;
				}
				else
				{
					check(ShaderPlatform == (EShaderPlatform)Shader->GetTarget().Platform);
				}
			}
		}
		
		FShaderCodeStats& CodeStats = EditorShaderCodeStats[ShaderPlatform];
		CodeStats.NumPipelines++;
		
		FEditorShaderCodeArchive* CodeArchive = EditorShaderCodeArchive[ShaderPlatform];
		if (!CodeArchive)
		{
			FName Format = LegacyShaderPlatformToShaderFormat(ShaderPlatform);
			CodeArchive = new FEditorShaderCodeArchive(Format);
			EditorShaderCodeArchive[ShaderPlatform] = CodeArchive;
			EditorArchivePipelines[ShaderPlatform] = IsOpenGLPlatform(ShaderPlatform);
		}
		check(CodeArchive);
		
		bool bAdded = false;
		if (EditorArchivePipelines[ShaderPlatform] && ((FEditorShaderCodeArchive*)CodeArchive)->AddPipeline(Pipeline))
		{
			CodeStats.NumUniquePipelines++;
			bAdded = true;
		}
		return bAdded;
	}

	bool SaveShaderCode(const FString& ShaderCodeDir, const FString& DebugOutputDir, const TArray<FName>& ShaderFormats)
	{
		bool bOk = ShaderFormats.Num() > 0;
		
		for (int32 i = 0; i < ShaderFormats.Num(); ++i)	
		{
			FName ShaderFormatName = ShaderFormats[i];
			EShaderPlatform ShaderPlatform = ShaderFormatToLegacyShaderPlatform(ShaderFormatName);
			FEditorShaderCodeArchive* CodeArchive = EditorShaderCodeArchive[ShaderPlatform];

			if (CodeArchive)
			{
				bOk &= CodeArchive->Finalize(ShaderCodeDir, DebugOutputDir, bNativeFormat);
			}
		}
		
		return bOk;
	}
	
	bool PackageNativeShaderLibrary(const FString& ShaderCodeDir, const FString& DebugShaderCodeDir, const TArray<FName>& ShaderFormats)
	{
		bool bOK = true;
		for (int32 i = 0; i < ShaderFormats.Num(); ++i)
		{
			FName ShaderFormatName = ShaderFormats[i];
			EShaderPlatform ShaderPlatform = ShaderFormatToLegacyShaderPlatform(ShaderFormatName);
			FEditorShaderCodeArchive* CodeArchive = EditorShaderCodeArchive[ShaderPlatform];
			
			if (CodeArchive)
			{
				bOK &= CodeArchive->PackageNativeShaderLibrary(ShaderCodeDir, DebugShaderCodeDir);
			}
		}
		return bOK;
	}

	void DumpShaderCodeStats()
	{
		int32 PlatformId = 0;
		for (const FShaderCodeStats& CodeStats : EditorShaderCodeStats)	
		{
			if (CodeStats.NumShaders > 0)
			{
				float UniqueSize = CodeStats.ShadersUniqueSize;
				float UniqueSizeMB = FUnitConversion::Convert(UniqueSize, EUnit::Bytes, EUnit::Megabytes);
				float TotalSize = CodeStats.ShadersSize;
				float TotalSizeMB = FUnitConversion::Convert(TotalSize, EUnit::Bytes, EUnit::Megabytes);

				UE_LOG(LogShaders, Display, TEXT(""));
				UE_LOG(LogShaders, Display, TEXT("Shader Code Stats: %s"), *LegacyShaderPlatformToShaderFormat((EShaderPlatform)PlatformId).ToString());
				UE_LOG(LogShaders, Display, TEXT("================="));
				UE_LOG(LogShaders, Display, TEXT("Unique Shaders: %d, Total Shaders: %d"), CodeStats.NumUniqueShaders, CodeStats.NumShaders);
				UE_LOG(LogShaders, Display, TEXT("Unique Shaders Size: %.2fmb, Total Shader Size: %.2fmb"), UniqueSizeMB, TotalSizeMB);
				UE_LOG(LogShaders, Display, TEXT("================="));
			}

			PlatformId++;
		}
	}
#endif// WITH_EDITOR
};

static FShaderCodeLibraryImpl* Impl = nullptr;

void FShaderCodeLibrary::InitForRuntime(EShaderPlatform ShaderPlatform)
{
	check(Impl == nullptr);
	check(FPlatformProperties::RequiresCookedData());

	if (!FPlatformProperties::IsServerOnly() && FApp::CanEverRender())
	{
		Impl = new FShaderCodeLibraryImpl(false);
		if (!Impl->OpenShaderCode(FPaths::ProjectContentDir(), ShaderPlatform))
		{
			Shutdown();
		}
	}
}

void FShaderCodeLibrary::InitForCooking(bool bNativeFormat)
{
	Impl = new FShaderCodeLibraryImpl(bNativeFormat);
}

void FShaderCodeLibrary::Shutdown()
{
#if WITH_EDITOR
	DumpShaderCodeStats();
#endif
	delete Impl;
	Impl = nullptr;
}

bool FShaderCodeLibrary::AddShaderCode(EShaderPlatform ShaderPlatform, EShaderFrequency Frequency, const FSHAHash& Hash, const TArray<uint8>& InCode, uint32 const UncompressedSize)
{
#if WITH_EDITOR
	if (Impl)
	{
		Impl->AddShaderCode(ShaderPlatform, Frequency, Hash, InCode, UncompressedSize);
		return true;
	}
#endif// WITH_EDITOR

	return false;
}
				
bool FShaderCodeLibrary::AddShaderPipeline(FShaderPipeline* Pipeline)
{
#if WITH_EDITOR
	if (Impl && Pipeline)
	{
		Impl->AddShaderPipeline(Pipeline);
		return true;
	}
#endif// WITH_EDITOR
	
	return false;
}

FVertexShaderRHIRef FShaderCodeLibrary::CreateVertexShader(EShaderPlatform Platform, FSHAHash Hash, TArray<uint8> const& Code)
{
	FVertexShaderRHIRef Shader;
	if (Impl && FPlatformProperties::RequiresCookedData())
	{
		Shader = Impl->CreateVertexShader(Platform, Hash);
	}
	if (!IsValidRef(Shader))
	{
		Shader = RHICreateVertexShader(Code);
	}
	SafeAssignHash(Shader, Hash);
	return Shader;
}

FPixelShaderRHIRef FShaderCodeLibrary::CreatePixelShader(EShaderPlatform Platform, FSHAHash Hash, TArray<uint8> const& Code)
{
	FPixelShaderRHIRef Shader;
	if (Impl && FPlatformProperties::RequiresCookedData())
	{
		Shader = Impl->CreatePixelShader(Platform, Hash);
	}
	if (!IsValidRef(Shader))
	{
		Shader = RHICreatePixelShader(Code);
	}
	SafeAssignHash(Shader, Hash);
	return Shader;
}

FGeometryShaderRHIRef FShaderCodeLibrary::CreateGeometryShader(EShaderPlatform Platform, FSHAHash Hash, TArray<uint8> const& Code)
{
	FGeometryShaderRHIRef Shader;
	if (Impl && FPlatformProperties::RequiresCookedData())
	{
		Shader = Impl->CreateGeometryShader(Platform, Hash);
	}
	if (!IsValidRef(Shader))
	{
		Shader = RHICreateGeometryShader(Code);
	}
	SafeAssignHash(Shader, Hash);
	return Shader;
}

FGeometryShaderRHIRef FShaderCodeLibrary::CreateGeometryShaderWithStreamOutput(EShaderPlatform Platform, FSHAHash Hash, const TArray<uint8>& Code, const FStreamOutElementList& ElementList, uint32 NumStrides, const uint32* Strides, int32 RasterizedStream)
{
	FGeometryShaderRHIRef Shader;
	if (Impl && FPlatformProperties::RequiresCookedData())
	{
		Shader = Impl->CreateGeometryShaderWithStreamOutput(Platform, Hash, ElementList, NumStrides, Strides, RasterizedStream);
	}
	if (!IsValidRef(Shader))
	{
		Shader = RHICreateGeometryShaderWithStreamOutput(Code, ElementList, NumStrides, Strides, RasterizedStream);
	}
	SafeAssignHash(Shader, Hash);
	return Shader;
}

FHullShaderRHIRef FShaderCodeLibrary::CreateHullShader(EShaderPlatform Platform, FSHAHash Hash, TArray<uint8> const& Code)
{
	FHullShaderRHIRef Shader;
	if (Impl && FPlatformProperties::RequiresCookedData())
	{
		Shader = Impl->CreateHullShader(Platform, Hash);
	}
	if (!IsValidRef(Shader))
	{
		Shader = RHICreateHullShader(Code);
	}
	SafeAssignHash(Shader, Hash);
	return Shader;
}

FDomainShaderRHIRef FShaderCodeLibrary::CreateDomainShader(EShaderPlatform Platform, FSHAHash Hash, TArray<uint8> const& Code)
{
	FDomainShaderRHIRef Shader;
	if (Impl && FPlatformProperties::RequiresCookedData())
	{
		Shader = Impl->CreateDomainShader(Platform, Hash);
	}
	if (!IsValidRef(Shader))
	{
		Shader = RHICreateDomainShader(Code);
	}
	SafeAssignHash(Shader, Hash);
	return Shader;
}

FComputeShaderRHIRef FShaderCodeLibrary::CreateComputeShader(EShaderPlatform Platform, FSHAHash Hash, TArray<uint8> const& Code)
{
	FComputeShaderRHIRef Shader;
	if (Impl && FPlatformProperties::RequiresCookedData())
	{
		Shader = Impl->CreateComputeShader(Platform, Hash);
	}
	if (!IsValidRef(Shader))
	{
		Shader = RHICreateComputeShader(Code);
	}
	SafeAssignHash(Shader, Hash);
	return Shader;
}

TRefCountPtr<FRHIShaderLibrary::FShaderLibraryIterator> FShaderCodeLibrary::CreateIterator(void)
{
	TRefCountPtr<FRHIShaderLibrary::FShaderLibraryIterator> It;
	if (Impl && FPlatformProperties::RequiresCookedData())
	{
		It = Impl->CreateIterator();
	}
	return It;
}

uint32 FShaderCodeLibrary::GetShaderCount(void)
{
	uint32 Num = 0;
	if (Impl && FPlatformProperties::RequiresCookedData())
	{
		Num = Impl->GetShaderCount();
	}
	return Num;
}
				
TSet<FShaderCodeLibraryPipeline> const* FShaderCodeLibrary::GetShaderPipelines(EShaderPlatform Platform)
{
	TSet<FShaderCodeLibraryPipeline> const* Pipelines = nullptr;
	if (Impl && FPlatformProperties::RequiresCookedData() && IsOpenGLPlatform(Platform))
	{
		Pipelines = Impl->GetShaderPipelines(Platform);
	}
	return Pipelines;
}

EShaderPlatform FShaderCodeLibrary::GetRuntimeShaderPlatform(void)
{
	EShaderPlatform Platform = SP_NumPlatforms;
	if (Impl && FPlatformProperties::RequiresCookedData())
	{
		Platform = Impl->GetRuntimeShaderPlatform();
	}
	return Platform;
}

#if WITH_EDITOR
bool FShaderCodeLibrary::SaveShaderCode(const FString& OutputDir, const FString& DebugDir, const TArray<FName>& ShaderFormats)
{
	if (Impl)
	{
		return Impl->SaveShaderCode(OutputDir, DebugDir, ShaderFormats);
	}
	
	return false;
}

bool FShaderCodeLibrary::PackageNativeShaderLibrary(const FString& ShaderCodeDir, const FString& DebugShaderCodeDir, const TArray<FName>& ShaderFormats)
{
	if (Impl)
	{
		return Impl->PackageNativeShaderLibrary(ShaderCodeDir, DebugShaderCodeDir, ShaderFormats);
	}
	
	return false;
}

void FShaderCodeLibrary::DumpShaderCodeStats()
{
	if (Impl)
	{
		Impl->DumpShaderCodeStats();
	}
}
#endif// WITH_EDITOR

void FShaderCodeLibrary::SafeAssignHash(FRHIShader* InShader, const FSHAHash& Hash)
{
	if (InShader)
	{
		InShader->SetHash(Hash);
	}
}
