// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LocalizationDashboardPrivatePCH.h"
#include "LocalizationCommandletTasks.h"
#include "LocalizationCommandletExecution.h"
#include "LocalizationConfigurationScript.h"

bool LocalizationCommandletTasks::GatherTargets(const TSharedRef<SWindow>& ParentWindow, const TArray<FLocalizationTargetSettings*>& TargetsSettings)
{
	TArray<LocalizationCommandletExecution::FTask> Tasks;

	for (FLocalizationTargetSettings* TargetSettings : TargetsSettings)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("TargetName"), FText::FromString(TargetSettings->Name));

		const FText GatherTaskName = FText::Format(NSLOCTEXT("LocalizationDashboard", "GatherTaskNameFormat", "Gather Text for {TargetName}"), Arguments);
		const FString GatherScriptPath = LocalizationConfigurationScript::GetGatherScriptPath(*TargetSettings);
		LocalizationConfigurationScript::GenerateGatherScript(*TargetSettings).Write(GatherScriptPath);
		Tasks.Add(LocalizationCommandletExecution::FTask(GatherTaskName, GatherScriptPath));

		const FText ReportTaskName = FText::Format(NSLOCTEXT("LocalizationDashboard", "ReportTaskNameFormat", "Generate Reports for {TargetName}"), Arguments);
		const FString ReportScriptPath = LocalizationConfigurationScript::GetReportScriptPath(*TargetSettings);
		LocalizationConfigurationScript::GenerateReportScript(*TargetSettings).Write(ReportScriptPath);
		Tasks.Add(LocalizationCommandletExecution::FTask(ReportTaskName, ReportScriptPath));
	}

	return LocalizationCommandletExecution::Execute(ParentWindow, NSLOCTEXT("LocalizationDashboard", "GatherAllTargetsWindowTitle", "Gather Text for All Targets"), Tasks);
}

bool LocalizationCommandletTasks::GatherTarget(const TSharedRef<SWindow>& ParentWindow, FLocalizationTargetSettings& TargetSettings)
{
	TArray<LocalizationCommandletExecution::FTask> Tasks;

	const FString GatherScriptPath = LocalizationConfigurationScript::GetGatherScriptPath(TargetSettings);
	LocalizationConfigurationScript::GenerateGatherScript(TargetSettings).Write(GatherScriptPath);
	Tasks.Add(LocalizationCommandletExecution::FTask(NSLOCTEXT("LocalizationDashboard", "GatherTaskName", "Gather Text"), GatherScriptPath));

	const FString ReportScriptPath = LocalizationConfigurationScript::GetReportScriptPath(TargetSettings);
	LocalizationConfigurationScript::GenerateReportScript(TargetSettings).Write(ReportScriptPath);
	Tasks.Add(LocalizationCommandletExecution::FTask(NSLOCTEXT("LocalizationDashboard", "ReportTaskName", "Generate Reports"), ReportScriptPath));

	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("TargetName"), FText::FromString(TargetSettings.Name));

	const FText WindowTitle = FText::Format(NSLOCTEXT("LocalizationDashboard", "GatherTargetWindowTitle", "Gather Text for Target {TargetName}"), Arguments);
	return LocalizationCommandletExecution::Execute(ParentWindow, WindowTitle, Tasks);
}


bool LocalizationCommandletTasks::ImportTargets(const TSharedRef<SWindow>& ParentWindow, const TArray<FLocalizationTargetSettings*>& TargetsSettings)
{
	TArray<LocalizationCommandletExecution::FTask> Tasks;

	for (FLocalizationTargetSettings* TargetSettings : TargetsSettings)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("TargetName"), FText::FromString(TargetSettings->Name));

		const FText ImportTaskName = FText::Format(NSLOCTEXT("LocalizationDashboard", "ImportTaskNameFormat", "Import Translations for {TargetName}"), Arguments);
		const FString ImportScriptPath = LocalizationConfigurationScript::GetImportScriptPath(*TargetSettings);
		LocalizationConfigurationScript::GenerateImportScript(*TargetSettings).Write(ImportScriptPath);
		Tasks.Add(LocalizationCommandletExecution::FTask(ImportTaskName, ImportScriptPath));

		const FText ReportTaskName = FText::Format(NSLOCTEXT("LocalizationDashboard", "ReportTaskNameFormat", "Generate Reports for {TargetName}"), Arguments);
		const FString ReportScriptPath = LocalizationConfigurationScript::GetReportScriptPath(*TargetSettings);
		LocalizationConfigurationScript::GenerateReportScript(*TargetSettings).Write(ReportScriptPath);
		Tasks.Add(LocalizationCommandletExecution::FTask(ReportTaskName, ReportScriptPath));
	}

	return LocalizationCommandletExecution::Execute(ParentWindow, NSLOCTEXT("LocalizationDashboard", "ImportForAllTargetsWindowTitle", "Import Translations for All Targets"), Tasks);
}

bool LocalizationCommandletTasks::ImportTarget(const TSharedRef<SWindow>& ParentWindow, FLocalizationTargetSettings& TargetSettings)
{
	TArray<LocalizationCommandletExecution::FTask> Tasks;

	const FString ImportScriptPath = LocalizationConfigurationScript::GetImportScriptPath(TargetSettings);
	LocalizationConfigurationScript::GenerateImportScript(TargetSettings).Write(ImportScriptPath);
	Tasks.Add(LocalizationCommandletExecution::FTask(NSLOCTEXT("LocalizationDashboard", "ImportTaskName", "Import Translations"), ImportScriptPath));

	const FString ReportScriptPath = LocalizationConfigurationScript::GetReportScriptPath(TargetSettings);
	LocalizationConfigurationScript::GenerateReportScript(TargetSettings).Write(ReportScriptPath);
	Tasks.Add(LocalizationCommandletExecution::FTask(NSLOCTEXT("LocalizationDashboard", "ReportTaskName", "Generate Reports"), ReportScriptPath));

	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("TargetName"), FText::FromString(TargetSettings.Name));

	const FText WindowTitle = FText::Format(NSLOCTEXT("LocalizationDashboard", "ImportForTargetWindowTitle", "Import Translations for Target {TargetName}"), Arguments);
	return LocalizationCommandletExecution::Execute(ParentWindow, WindowTitle, Tasks);
}

bool LocalizationCommandletTasks::ImportCulture(const TSharedRef<SWindow>& ParentWindow, FLocalizationTargetSettings& TargetSettings, const FString& CultureName)
{
	FCulturePtr Culture = FInternationalization::Get().GetCulture(CultureName);
	if (!Culture.IsValid())
	{
		return false;
	}

	TArray<LocalizationCommandletExecution::FTask> Tasks;

	const FString ImportScriptPath = LocalizationConfigurationScript::GetImportScriptPath(TargetSettings, &CultureName);
	LocalizationConfigurationScript::GenerateImportScript(TargetSettings, &CultureName).Write(ImportScriptPath);
	Tasks.Add(LocalizationCommandletExecution::FTask(NSLOCTEXT("LocalizationDashboard", "ImportTaskName", "Import Translations"), ImportScriptPath));

	const FString ReportScriptPath = LocalizationConfigurationScript::GetReportScriptPath(TargetSettings);
	LocalizationConfigurationScript::GenerateReportScript(TargetSettings).Write(ReportScriptPath);
	Tasks.Add(LocalizationCommandletExecution::FTask(NSLOCTEXT("LocalizationDashboard", "ReportTaskName", "Generate Reports"), ReportScriptPath));

	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("CultureName"), FText::FromString(Culture->GetDisplayName()));
	Arguments.Add(TEXT("TargetName"), FText::FromString(TargetSettings.Name));

	const FText WindowTitle = FText::Format(NSLOCTEXT("LocalizationDashboard", "ImportCultureForTargetWindowTitle", "Import {CultureName} Translations for Target {TargetName}"), Arguments);
	return LocalizationCommandletExecution::Execute(ParentWindow, WindowTitle, Tasks);
}

bool LocalizationCommandletTasks::ExportTargets(const TSharedRef<SWindow>& ParentWindow, const TArray<FLocalizationTargetSettings*>& TargetsSettings)
{
	TArray<LocalizationCommandletExecution::FTask> Tasks;

	for (FLocalizationTargetSettings* TargetSettings : TargetsSettings)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("TargetName"), FText::FromString(TargetSettings->Name));

		const FText ExportTaskName = FText::Format(NSLOCTEXT("LocalizationDashboard", "ExportTaskNameFormat", "Export Translations for {TargetName}"), Arguments);
		const FString ExportScriptPath = LocalizationConfigurationScript::GetExportScriptPath(*TargetSettings);
		LocalizationConfigurationScript::GenerateExportScript(*TargetSettings).Write(ExportScriptPath);
		Tasks.Add(LocalizationCommandletExecution::FTask(ExportTaskName, ExportScriptPath));
	}

	return LocalizationCommandletExecution::Execute(ParentWindow, NSLOCTEXT("LocalizationDashboard", "ExportForAllTargetsWindowTitle", "Export Translations for All Targets"), Tasks);
}

bool LocalizationCommandletTasks::ExportTarget(const TSharedRef<SWindow>& ParentWindow, FLocalizationTargetSettings& TargetSettings)
{
	TArray<LocalizationCommandletExecution::FTask> Tasks;

	const FString ExportScriptPath = LocalizationConfigurationScript::GetExportScriptPath(TargetSettings);
	LocalizationConfigurationScript::GenerateExportScript(TargetSettings).Write(ExportScriptPath);
	Tasks.Add(LocalizationCommandletExecution::FTask(NSLOCTEXT("LocalizationDashboard", "ExportTaskName", "Export Translations"), ExportScriptPath));

	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("TargetName"), FText::FromString(TargetSettings.Name));

	const FText WindowTitle = FText::Format(NSLOCTEXT("LocalizationDashboard", "ExportForTargetWindowTitle", "Export Translations for Target {TargetName}"), Arguments);
	return LocalizationCommandletExecution::Execute(ParentWindow, WindowTitle, Tasks);
}

bool LocalizationCommandletTasks::ExportCulture(const TSharedRef<SWindow>& ParentWindow, FLocalizationTargetSettings& TargetSettings, const FString& CultureName)
{
	FCulturePtr Culture = FInternationalization::Get().GetCulture(CultureName);
	if (!Culture.IsValid())
	{
		return false;
	}

	TArray<LocalizationCommandletExecution::FTask> Tasks;

	const FString ExportScriptPath = LocalizationConfigurationScript::GetExportScriptPath(TargetSettings, &CultureName);
	LocalizationConfigurationScript::GenerateExportScript(TargetSettings, &CultureName).Write(ExportScriptPath);
	Tasks.Add(LocalizationCommandletExecution::FTask(NSLOCTEXT("LocalizationDashboard", "ExportTaskName", "Export Translations"), ExportScriptPath));

	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("CultureName"), FText::FromString(Culture->GetDisplayName()));
	Arguments.Add(TEXT("TargetName"), FText::FromString(TargetSettings.Name));

	const FText WindowTitle = FText::Format(NSLOCTEXT("LocalizationDashboard", "ExportCultureForTargetWindowTitle", "Export {CultureName} Translations for Target {TargetName}"), Arguments);
	return LocalizationCommandletExecution::Execute(ParentWindow, WindowTitle, Tasks);
}

bool LocalizationCommandletTasks::GenerateReportsForTargets(const TSharedRef<SWindow>& ParentWindow, const TArray<FLocalizationTargetSettings*>& TargetsSettings)
{
	TArray<LocalizationCommandletExecution::FTask> Tasks;

	for (FLocalizationTargetSettings* TargetSettings : TargetsSettings)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("TargetName"), FText::FromString(TargetSettings->Name));

		const FText ReportTaskName = FText::Format(NSLOCTEXT("LocalizationDashboard", "ReportTaskNameFormat", "Generate Reports for {TargetName}"), Arguments);
		const FString ReportScriptPath = LocalizationConfigurationScript::GetReportScriptPath(*TargetSettings);
		LocalizationConfigurationScript::GenerateReportScript(*TargetSettings).Write(ReportScriptPath);
		Tasks.Add(LocalizationCommandletExecution::FTask(ReportTaskName, ReportScriptPath));
	}

	return LocalizationCommandletExecution::Execute(ParentWindow, NSLOCTEXT("LocalizationDashboard", "GenerateReportsForAllTargetsWindowTitle", "Generate Reports for All Targets"), Tasks);
}

bool LocalizationCommandletTasks::GenerateReportsForTarget(const TSharedRef<SWindow>& ParentWindow, FLocalizationTargetSettings& TargetSettings)
{
	TArray<LocalizationCommandletExecution::FTask> Tasks;

	const FString ReportScriptPath = LocalizationConfigurationScript::GetReportScriptPath(TargetSettings);
	LocalizationConfigurationScript::GenerateReportScript(TargetSettings).Write(ReportScriptPath);
	Tasks.Add(LocalizationCommandletExecution::FTask(NSLOCTEXT("LocalizationDashboard", "ReportTaskName", "Generate Reports"), ReportScriptPath));

	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("TargetName"), FText::FromString(TargetSettings.Name));

	const FText WindowTitle = FText::Format(NSLOCTEXT("LocalizationDashboard", "GenerateReportsForTargetWindowTitle", "Generate Reports for Target {TargetName}"), Arguments);
	return LocalizationCommandletExecution::Execute(ParentWindow, WindowTitle, Tasks);
}