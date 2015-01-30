// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LocalizationDashboardPrivatePCH.h"
#include "LocalizationConfigurationScript.h"
#include "Classes/ProjectLocalizationSettings.h"

namespace LocalizationConfigurationScript
{
	FString MakePathRelativeToProjectDirectory(const FString& Path)
	{
		FString Result = Path;
		FPaths::MakePathRelativeTo(Result, *FPaths::GameDir());
		return Result;
	}

	FString GetScriptDirectory()
	{
		return FPaths::GameConfigDir() / TEXT("Localization");
	}

	FString GetDataDirectory(const FLocalizationTargetSettings& Target)
	{
		return FPaths::GameContentDir() / TEXT("Localization") / Target.Name;
	}

	TArray<FString> GetScriptPaths(const FLocalizationTargetSettings& Target)
	{
		TArray<FString> Result;
		Result.Add(GetGatherScriptPath(Target));
		Result.Add(GetImportScriptPath(Target));
		Result.Add(GetExportScriptPath(Target));
		Result.Add(GetReportScriptPath(Target));
		return Result;
	}

	FString GetManifestPath(const FLocalizationTargetSettings& Target)
	{
		return GetDataDirectory(Target) / FString::Printf( TEXT("%s.%s"), *Target.Name, TEXT("manifest") );
	}

	FString GetArchivePath(const FLocalizationTargetSettings& Target, const FString& CultureName)
	{
		return GetDataDirectory(Target) / CultureName / FString::Printf( TEXT("%s.%s"), *Target.Name, TEXT("archive") );
	}

	FString GetDefaultPOFileName(const FLocalizationTargetSettings& Target)
	{
		return FString::Printf( TEXT("%s.%s"), *Target.Name, TEXT("po") );
	}

	FString GetDefaultPOPath(const FLocalizationTargetSettings& Target, const FString& CultureName)
	{
		return GetDataDirectory(Target) / CultureName / GetDefaultPOFileName(Target);
	}

	FString GetLocResPath(const FLocalizationTargetSettings& Target, const FString& CultureName)
	{
		return GetDataDirectory(Target) / CultureName / FString::Printf( TEXT("%s.%s"), *Target.Name, TEXT("locres") );
	}

	FString GetWordCountCSVPath(const FLocalizationTargetSettings& Target)
	{
		return GetDataDirectory(Target) / FString::Printf( TEXT("%s.%s"), *Target.Name, TEXT("csv") );
	}

	FString GetConflictReportPath(const FLocalizationTargetSettings& Target)
	{
		return GetDataDirectory(Target) / FString::Printf( TEXT("%s_Conflicts.%s"), *Target.Name, TEXT("txt") );
	}

	FLocalizationConfigurationScript GenerateGatherScript(const FLocalizationTargetSettings& Target)
	{
		FLocalizationConfigurationScript Script;

		const bool HasSourceCode = IFileManager::Get().DirectoryExists( *FPaths::GameSourceDir() );

		const FString ConfigDirRelativeToGameDir = MakePathRelativeToProjectDirectory(FPaths::GameConfigDir());
		const FString SourceDirRelativeToGameDir = MakePathRelativeToProjectDirectory(FPaths::GameSourceDir());
		const FString ContentDirRelativeToGameDir = MakePathRelativeToProjectDirectory(FPaths::GameContentDir());

		// CommonSettings
		{
			FConfigSection& ConfigSection = Script.CommonSettings();

			const UProjectLocalizationSettings* const ProjectLocalizationSettings = GetDefault<UProjectLocalizationSettings>(UProjectLocalizationSettings::StaticClass());
			for (const FString& TargetDependencyName : Target.TargetDependencies)
			{
				ULocalizationTarget* const * OtherTarget = ProjectLocalizationSettings->TargetObjects.FindByPredicate([&TargetDependencyName](ULocalizationTarget* const OtherTarget)->bool{return OtherTarget->Settings.Name == TargetDependencyName;});
				if (OtherTarget)
				{
					ConfigSection.Add( TEXT("ManifestDependencies"), MakePathRelativeToProjectDirectory(GetManifestPath((*OtherTarget)->Settings)) );
				}
			}

			for (const FFilePath& Path : Target.AdditionalManifestDependencies)
			{
				ConfigSection.Add( TEXT("ManifestDependencies"), MakePathRelativeToProjectDirectory(Path.FilePath) );
			}

			const FString SourcePath = ContentDirRelativeToGameDir / TEXT("Localization") / Target.Name;
			ConfigSection.Add( TEXT("SourcePath"), SourcePath );
			const FString DestinationPath = ContentDirRelativeToGameDir / TEXT("Localization") / Target.Name;
			ConfigSection.Add( TEXT("DestinationPath"), DestinationPath );

			ConfigSection.Add( TEXT("ManifestName"), FPaths::GetCleanFilename(GetManifestPath(Target)) );
			ConfigSection.Add( TEXT("ArchiveName"), FPaths::GetCleanFilename(GetArchivePath(Target, FString())) );

			ConfigSection.Add( TEXT("SourceCulture"), Target.NativeCultureStatistics.CultureName );
			ConfigSection.Add( TEXT("CulturesToGenerate"), Target.NativeCultureStatistics.CultureName );
			for (const FCultureStatistics& CultureStatistics : Target.SupportedCulturesStatistics)
			{
				ConfigSection.Add( TEXT("CulturesToGenerate"), CultureStatistics.CultureName );
			}
		}

		uint32 GatherTextStepIndex = 0;
		// GatherTextFromSource
		{
			FConfigSection& ConfigSection = Script.GatherTextStep(GatherTextStepIndex++);

			// CommandletClass
			ConfigSection.Add( TEXT("CommandletClass"), TEXT("GatherTextFromSource") );

			// Include Paths
			if (HasSourceCode) 
			{
				ConfigSection.Add( TEXT("IncludePaths"), SourceDirRelativeToGameDir );
			}
			ConfigSection.Add( TEXT("IncludePaths"), ConfigDirRelativeToGameDir );

			// Exclude Paths
			ConfigSection.Add( TEXT("ExcludePaths"), ConfigDirRelativeToGameDir / TEXT("Localization") );

			// SourceFileSearchFilters
			if (HasSourceCode)
			{
				ConfigSection.Add( TEXT("SourceFileSearchFilters"), TEXT("*.h") );
				ConfigSection.Add( TEXT("SourceFileSearchFilters"), TEXT("*.cpp") );
			}
			ConfigSection.Add( TEXT("SourceFileSearchFilters"), TEXT("*.ini") );
		}

		// GatherTextFromAssets
		{
			FConfigSection& ConfigSection = Script.GatherTextStep(GatherTextStepIndex++);

			// CommandletClass
			ConfigSection.Add( TEXT("CommandletClass"), TEXT("GatherTextFromAssets") );

			// IncludePaths
			ConfigSection.Add( TEXT("IncludePaths"), ContentDirRelativeToGameDir );

			// ExcludePaths
			ConfigSection.Add( TEXT("ExcludePaths"), ContentDirRelativeToGameDir / TEXT("Localization") );

			// PackageExtensions
			ConfigSection.Add( TEXT("PackageExtensions"), TEXT("*.umap") );
			ConfigSection.Add( TEXT("PackageExtensions"), TEXT("*.uasset") );
		}

		// GenerateGatherManifest
		{
			FConfigSection& ConfigSection = Script.GatherTextStep(GatherTextStepIndex++);

			// CommandletClass
			ConfigSection.Add( TEXT("CommandletClass"), TEXT("GenerateGatherManifest") );
		}

		// GenerateGatherArchive
		if (Target.SupportedCulturesStatistics.Num())
		{
			FConfigSection& ConfigSection = Script.GatherTextStep(GatherTextStepIndex++);

			// CommandletClass
			ConfigSection.Add( TEXT("CommandletClass"), TEXT("GenerateGatherArchive") );
		}

		Script.Dirty = true;

		return Script;
	}

	FString GetGatherScriptPath(const FLocalizationTargetSettings& Target)
	{
		return GetScriptDirectory() / FString::Printf( TEXT("%s_Gather.%s"), *(Target.Name), TEXT("ini") );
	}

	FLocalizationConfigurationScript GenerateImportScript(const FLocalizationTargetSettings& Target, const TOptional<FString> CultureName, const TOptional<FString> OutputPathOverride)
	{
		FLocalizationConfigurationScript Script;

		const FString ContentDirRelativeToGameDir = MakePathRelativeToProjectDirectory(FPaths::GameContentDir());

		// GatherTextStep0 - InternationalizationExport
		{
			FConfigSection& ConfigSection = Script.GatherTextStep(0);

			// CommandletClass
			ConfigSection.Add( TEXT("CommandletClass"), TEXT("InternationalizationExport") );

			ConfigSection.Add( TEXT("bImportLoc"), TEXT("true") );

			FString SourcePath;
			// Overriding output path changes the source directory for the PO file.
			if (OutputPathOverride.IsSet())
			{
				// The output path for a specific culture is a file path.
				if (CultureName.IsSet())
				{
					SourcePath = MakePathRelativeToProjectDirectory( FPaths::GetPath(OutputPathOverride.GetValue()) );
				}
				// Otherwise, it is a directory path.
				else
				{
					SourcePath = MakePathRelativeToProjectDirectory( OutputPathOverride.GetValue() );
				}
			}
			// Use the default PO file's directory path.
			else
			{
				SourcePath = ContentDirRelativeToGameDir / TEXT("Localization") / Target.Name;
			}
			ConfigSection.Add( TEXT("SourcePath"), SourcePath );

			const FString DestinationPath = ContentDirRelativeToGameDir / TEXT("Localization") / Target.Name;
			ConfigSection.Add( TEXT("DestinationPath"), DestinationPath );

			const auto& AddCultureToGenerate = [&](const int32 Index)
			{
				ConfigSection.Add( TEXT("CulturesToGenerate"), Target.SupportedCulturesStatistics[Index].CultureName );
			};

			// Export for a specific culture.
			if (CultureName.IsSet())
			{
				ConfigSection.Add( TEXT("CulturesToGenerate"), CultureName.GetValue() );
			}
			// Export for all cultures.
			else
			{
				for (const FCultureStatistics& CultureStatistics : Target.SupportedCulturesStatistics)
				{
					ConfigSection.Add( TEXT("CulturesToGenerate"), CultureStatistics.CultureName );
				}
			}

			// Do not use culture subdirectories if importing a single culture to a specific directory.
			if (CultureName.IsSet() && OutputPathOverride.IsSet())
			{
				ConfigSection.Add( TEXT("bUseCultureDirectory"), "false" );
			}

			ConfigSection.Add( TEXT("ManifestName"), FPaths::GetCleanFilename(GetManifestPath(Target)) );
			ConfigSection.Add( TEXT("ArchiveName"), FPaths::GetCleanFilename(GetArchivePath(Target, FString())) );

			FString POFileName;
			// The output path for a specific culture is a file path.
			if (CultureName.IsSet() && OutputPathOverride.IsSet())
			{
				POFileName =  FPaths::GetCleanFilename( OutputPathOverride.GetValue() );
			}
			// Use the default PO file's name.
			else
			{
				POFileName = FPaths::GetCleanFilename( GetDefaultPOFileName( Target ) );
			}
			ConfigSection.Add( TEXT("PortableObjectName"), POFileName );
		}

		Script.Dirty = true;

		return Script;
	}

	FString GetImportScriptPath(const FLocalizationTargetSettings& Target, const TOptional<FString> CultureName)
	{
		const FString ConfigFileDirectory = GetScriptDirectory();
		FString ConfigFilePath;
		if (CultureName.IsSet())
		{
			ConfigFilePath = ConfigFileDirectory / FString::Printf( TEXT("%s_Import_%s.%s"), *Target.Name, *CultureName.GetValue(), TEXT("ini") );
		}
		else
		{
			ConfigFilePath = ConfigFileDirectory / FString::Printf( TEXT("%s_Import.%s"), *Target.Name, TEXT("ini") );
		}
		return ConfigFilePath;
	}

	FLocalizationConfigurationScript GenerateExportScript(const FLocalizationTargetSettings& Target, const TOptional<FString> CultureName, const TOptional<FString> OutputPathOverride)
	{
		FLocalizationConfigurationScript Script;

		const FString ContentDirRelativeToGameDir = MakePathRelativeToProjectDirectory(FPaths::GameContentDir());

		// GatherTextStep0 - InternationalizationExport
		{
			FConfigSection& ConfigSection = Script.GatherTextStep(0);

			// CommandletClass
			ConfigSection.Add( TEXT("CommandletClass"), TEXT("InternationalizationExport") );

			ConfigSection.Add( TEXT("bExportLoc"), TEXT("true") );

			const FString SourcePath = ContentDirRelativeToGameDir / TEXT("Localization") / Target.Name;
			ConfigSection.Add( TEXT("SourcePath"), SourcePath );

			FString DestinationPath;
			// Overriding output path changes the destination directory for the PO file.
			if (OutputPathOverride.IsSet())
			{
				// The output path for a specific culture is a file path.
				if (CultureName.IsSet())
				{
					DestinationPath = MakePathRelativeToProjectDirectory( FPaths::GetPath(OutputPathOverride.GetValue()) );
				}
				// Otherwise, it is a directory path.
				else
				{
					DestinationPath = MakePathRelativeToProjectDirectory( OutputPathOverride.GetValue() );
				}
			}
			// Use the default PO file's directory path.
			else
			{
				DestinationPath = ContentDirRelativeToGameDir / TEXT("Localization") / Target.Name;
			}
			ConfigSection.Add( TEXT("DestinationPath"), DestinationPath );

			TArray<const FCultureStatistics*> AllCultureStatistics;
			AllCultureStatistics.Add(&Target.NativeCultureStatistics);
			for (const FCultureStatistics& SupportedCultureStatistics : Target.SupportedCulturesStatistics)
			{
				AllCultureStatistics.Add(&SupportedCultureStatistics);
			}

			const auto& AddCultureToGenerate = [&](const int32 Index)
			{
				ConfigSection.Add( TEXT("CulturesToGenerate"), AllCultureStatistics[Index]->CultureName );
			};

			// Export for a specific culture.
			if (CultureName.IsSet())
			{
				const int32 CultureIndex = AllCultureStatistics.IndexOfByPredicate([CultureName](const FCultureStatistics* Culture) { return Culture->CultureName == CultureName.GetValue(); });
				AddCultureToGenerate(CultureIndex);
			}
			// Export for all cultures.
			else
			{
				for (int32 CultureIndex = 0; CultureIndex < AllCultureStatistics.Num(); ++CultureIndex)
				{
					AddCultureToGenerate(CultureIndex);
				}
			}

			// Do not use culture subdirectories if exporting a single culture to a specific directory.
			if (CultureName.IsSet() && OutputPathOverride.IsSet())
			{
				ConfigSection.Add( TEXT("bUseCultureDirectory"), "false" );
			}


			ConfigSection.Add( TEXT("ManifestName"), FPaths::GetCleanFilename(GetManifestPath(Target)) );
			ConfigSection.Add( TEXT("ArchiveName"), FPaths::GetCleanFilename(GetArchivePath(Target, FString())) );
			FString POFileName;
			// The output path for a specific culture is a file path.
			if (CultureName.IsSet() && OutputPathOverride.IsSet())
			{
				POFileName =  FPaths::GetCleanFilename( OutputPathOverride.GetValue() );
			}
			// Use the default PO file's name.
			else
			{
				POFileName = FPaths::GetCleanFilename( GetDefaultPOPath( Target, CultureName.Get( TEXT("") ) ) );
			}
			ConfigSection.Add( TEXT("PortableObjectName"), POFileName );
		}

		Script.Dirty = true;

		return Script;
	}

	FString GetExportScriptPath(const FLocalizationTargetSettings& Target, const TOptional<FString> CultureName)
	{
		const FString ConfigFileDirectory = GetScriptDirectory();
		FString ConfigFilePath;
		if (CultureName.IsSet())
		{
			ConfigFilePath = ConfigFileDirectory / FString::Printf( TEXT("%s_Export_%s.%s"), *Target.Name, *CultureName.GetValue(), TEXT("ini") );
		}
		else
		{
			ConfigFilePath = ConfigFileDirectory / FString::Printf( TEXT("%s_Export.%s"), *Target.Name, TEXT("ini") );
		}
		return ConfigFilePath;
	}

	FLocalizationConfigurationScript GenerateReportScript(const FLocalizationTargetSettings& Target)
	{
		FLocalizationConfigurationScript Script;

		const FString ContentDirRelativeToGameDir = MakePathRelativeToProjectDirectory(FPaths::GameContentDir());

		// GatherTextStep0 - GenerateTextLocalizationReport
		{
			FConfigSection& ConfigSection = Script.GatherTextStep(0);

			// CommandletClass
			ConfigSection.Add( TEXT("CommandletClass"), TEXT("GenerateTextLocalizationReport") );

			const FString SourcePath = ContentDirRelativeToGameDir / TEXT("Localization") / Target.Name;
			ConfigSection.Add( TEXT("SourcePath"), SourcePath );
			const FString DestinationPath = ContentDirRelativeToGameDir / TEXT("Localization") / Target.Name;
			ConfigSection.Add( TEXT("DestinationPath"), DestinationPath );

			ConfigSection.Add( TEXT("ManifestName"), FString::Printf( TEXT("%s.%s"), *Target.Name, TEXT("manifest") ) );

			for (const FCultureStatistics& CultureStatistics : Target.SupportedCulturesStatistics)
			{
				ConfigSection.Add( TEXT("CulturesToGenerate"), CultureStatistics.CultureName );
			}

			ConfigSection.Add( TEXT("bWordCountReport"), TEXT("true") );

			ConfigSection.Add( TEXT("WordCountReportName"), FPaths::GetCleanFilename( GetWordCountCSVPath(Target) ) );

			ConfigSection.Add( TEXT("bConflictReport"), TEXT("true") );

			ConfigSection.Add( TEXT("ConflictReportName"), FPaths::GetCleanFilename( GetConflictReportPath(Target) ) );
		}

		Script.Dirty = true;

		return Script;
	}

	FString GetReportScriptPath(const FLocalizationTargetSettings& Target)
	{
		return GetScriptDirectory() / FString::Printf( TEXT("%s_GenerateReports.%s"), *(Target.Name), TEXT("ini") );
	}

	FLocalizationConfigurationScript GenerateCompileScript(const FLocalizationTargetSettings& Target)
	{
		FLocalizationConfigurationScript Script;

		const FString ContentDirRelativeToGameDir = MakePathRelativeToProjectDirectory(FPaths::GameContentDir());

		// GatherTextStep0 - GenerateTextLocalizationResource
		{
			FConfigSection& ConfigSection = Script.GatherTextStep(0);

			// CommandletClass
			ConfigSection.Add( TEXT("CommandletClass"), TEXT("GenerateTextLocalizationResource") );

			const FString SourcePath = ContentDirRelativeToGameDir / TEXT("Localization") / Target.Name;
			ConfigSection.Add( TEXT("SourcePath"), SourcePath );
			const FString DestinationPath = ContentDirRelativeToGameDir / TEXT("Localization") / Target.Name;
			ConfigSection.Add( TEXT("DestinationPath"), DestinationPath );

			ConfigSection.Add( TEXT("ManifestName"), FString::Printf( TEXT("%s.%s"), *Target.Name, TEXT("manifest") ) );
			ConfigSection.Add( TEXT("ResourceName"), FString::Printf( TEXT("%s.%s"), *Target.Name, TEXT("locres") ) );

			TArray<const FCultureStatistics*> AllCultureStatistics;
			AllCultureStatistics.Add(&Target.NativeCultureStatistics);
			for (const FCultureStatistics& SupportedCultureStatistics : Target.SupportedCulturesStatistics)
			{
				AllCultureStatistics.Add(&SupportedCultureStatistics);
			}

			for (const FCultureStatistics* CultureStatistics : AllCultureStatistics)
			{
				ConfigSection.Add( TEXT("CulturesToGenerate"), CultureStatistics->CultureName );
			}
		}

		Script.Dirty = true;

		return Script;
	}

	FString GetCompileScriptPath(const FLocalizationTargetSettings& Target)
	{
		return GetScriptDirectory() / FString::Printf( TEXT("%s_Compile.%s"), *(Target.Name), TEXT("ini") );
	}
}