// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "ConfigEditorPropertyDetails.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Editor/PropertyEditor/Public/PropertyPath.h"
#include "Editor/PropertyEditor/Public/IPropertyTable.h"
#include "Editor/PropertyEditor/Public/IPropertyTableColumn.h"
#include "Editor/PropertyEditor/Public/IPropertyTableCustomColumn.h"

#include "IConfigEditorModule.h"
#include "ConfigPropertyColumn.h"
#include "ConfigPropertyConfigFileStateColumn.h"
#include "ConfigPropertyHelper.h"


#define LOCTEXT_NAMESPACE "ConfigPropertyHelperDetails"

////////////////////////////////////////////////
// FConfigPropertyHelperDetails

void FConfigPropertyHelperDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TSharedPtr<IPropertyHandle> PropertyHandle = DetailBuilder.GetProperty("EditProperty");
	DetailBuilder.HideProperty(PropertyHandle);

	UObject* PropValue;
	PropertyHandle->GetValue(PropValue);

	// Keep a record of the UProperty we are looking to update
	Property = CastChecked<UProperty>(PropValue);

	// Get access to all of the config files where this property is configurable.
	ConfigFilesHandle = DetailBuilder.GetProperty("ConfigFilePropertyObjects");
	FSimpleDelegate OnConfigFileListChangedDelegate = FSimpleDelegate::CreateSP(this, &FConfigPropertyHelperDetails::OnConfigFileListChanged);
	ConfigFilesHandle->SetOnPropertyValueChanged(OnConfigFileListChangedDelegate);
	DetailBuilder.HideProperty(ConfigFilesHandle);

	// Add the properties to a property table so we can edit these.
	IDetailCategoryBuilder& A = DetailBuilder.EditCategory("ConfigHierarchy");
	A.AddCustomRow(LOCTEXT("ConfigHierarchy", "ConfigHierarchy"))
	[
		// Create a property table with the values.
		ConstructPropertyTable(DetailBuilder)
	];

	// Listen for changes to the properties, we handle these by updating the ini file associated.
	FCoreUObjectDelegates::OnObjectPropertyChanged.AddSP(this, &FConfigPropertyHelperDetails::OnPropertyValueChanged);
}


void FConfigPropertyHelperDetails::OnPropertyValueChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent)
{
	UClass* OwnerClass = Property->GetOwnerClass();
	if (Object->IsA(OwnerClass))
	{
		const FString* FileName = ConfigFileAndPropertySourcePairings.FindKey(Object);
		if (FileName != nullptr)
		{
			Object->UpdateDefaultConfigFile(*FileName);
		}
	}
}


// TPMB - This didnt work... why?
void FConfigPropertyHelperDetails::OnConfigFileListChanged()
{
	PropertyTable->RequestRefresh();
}


void FConfigPropertyHelperDetails::AddEditablePropertyForConfig(IDetailLayoutBuilder& DetailBuilder, const UPropertyConfigFileDisplayRow* ConfigFileProperty)
{
	AssociatedConfigFileAndObjectPairings.Add(ConfigFileProperty->ConfigFileName, (UObject*)ConfigFileProperty);

	// Add the properties to a property table so we can edit these.
	IDetailCategoryBuilder& A = DetailBuilder.EditCategory("TestCat");

	// Create an instance of the owner object so we can dictate values for our property on a per object basis.
	UObject* CDO = Property->GetOwnerClass()->GetDefaultObject<UObject>();
	UObject* ConfigEntryObject = StaticDuplicateObject(CDO, GetTransientPackage(), *(ConfigFileProperty->ConfigFileName + TEXT("_cdoDupe")));
	ConfigEntryObject->AddToRoot();
	ConfigEntryObject->LoadConfig(Property->GetOwnerClass(), *ConfigFileProperty->ConfigFileName, UE4::LCPF_None, Property);

	// Add a reference for future saving.
	ConfigFileAndPropertySourcePairings.Add(ConfigFileProperty->ConfigFileName, (UObject*)ConfigEntryObject);

	// We need to add a property row for each config file entry. 
	// This allows us to have an editable widget for each config file.
	TArray<UObject*> ConfigPropertyDisplayObjects;
	ConfigPropertyDisplayObjects.Add(ConfigEntryObject);
	if (IDetailPropertyRow* ExternalRow = A.AddExternalProperty(ConfigPropertyDisplayObjects, Property->GetFName()))
	{
		TSharedPtr<SWidget> NameWidget;
		TSharedPtr<SWidget> ValueWidget;
		ExternalRow->GetDefaultWidgets(NameWidget, ValueWidget);

		// Register the Value widget and config file pairing with the config editor.
		// The config editor needs this to determine what a cell presenter shows.
		IConfigEditorModule& ConfigEditor = FModuleManager::Get().LoadModuleChecked<IConfigEditorModule>("ConfigEditor");
		ConfigEditor.AddExternalPropertyValueWidgetAndConfigPairing(ConfigFileProperty->ConfigFileName, ValueWidget);
		
		// now hide the property so it is not added to the property display view
		ExternalRow->Visibility(EVisibility::Hidden);
	}
}


TSharedRef<SWidget> FConfigPropertyHelperDetails::ConstructPropertyTable(IDetailLayoutBuilder& DetailBuilder)
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	PropertyTable = PropertyEditorModule.CreatePropertyTable();
	PropertyTable->SetSelectionMode(ESelectionMode::None);
	PropertyTable->SetIsUserAllowedToChangeRoot(false);
	PropertyTable->SetShowObjectName(false);

	RepopulatePropertyTable(DetailBuilder);

	TArray< TSharedRef<class IPropertyTableCustomColumn>> CustomColumns;
	TSharedRef<FConfigPropertyCustomColumn> EditPropertyColumn = MakeShareable(new FConfigPropertyCustomColumn());
	EditPropertyColumn->EditProperty = Property; // FindFieldChecked<UProperty>(UPropertyConfigFileDisplay::StaticClass(), TEXT("EditProperty"));
	CustomColumns.Add(EditPropertyColumn);

	//TSharedRef<FConfigPropertyConfigFileStateCustomColumn> ConfigFileStateColumn = MakeShareable(new FConfigPropertyConfigFileStateCustomColumn());
	//ConfigFileStateColumn->SupportedProperty = FindFieldChecked<UProperty>(UPropertyConfigFileDisplay::StaticClass(), TEXT("FileState"));
	//CustomColumns.Add(ConfigFileStateColumn);

	return PropertyEditorModule.CreatePropertyTableWidget(PropertyTable.ToSharedRef(), CustomColumns);
}


void FConfigPropertyHelperDetails::RepopulatePropertyTable(IDetailLayoutBuilder& DetailBuilder)
{
	// Clear out any previous entries from the table.
	AssociatedConfigFileAndObjectPairings.Empty();

	// Add an entry for each config so the value can be set in each of the config files independently.
	uint32 ConfigCount = 0;
	TSharedPtr<IPropertyHandleArray> ConfigFilesArrayHandle = ConfigFilesHandle->AsArray();
	ConfigFilesArrayHandle->GetNumElements(ConfigCount);

	// For each config file, add the capacity to edit this property.
	for (uint32 Index = 0; Index < ConfigCount; Index++)
	{
		FString ConfigFile;
		TSharedRef<IPropertyHandle> ConfigFileElementHandle = ConfigFilesArrayHandle->GetElement(Index);

		UObject* ConfigFileSinglePropertyHelperObj = nullptr;
		ensure(ConfigFileElementHandle->GetValue(ConfigFileSinglePropertyHelperObj) == FPropertyAccess::Success);

		UPropertyConfigFileDisplayRow* ConfigFileSinglePropertyHelper = CastChecked<UPropertyConfigFileDisplayRow>(ConfigFileSinglePropertyHelperObj);

		AddEditablePropertyForConfig(DetailBuilder, ConfigFileSinglePropertyHelper);
	}

	// We need a row for each config file
	TArray<UObject*> ConfigPropertyDisplayObjects;
	AssociatedConfigFileAndObjectPairings.GenerateValueArray(ConfigPropertyDisplayObjects);
	PropertyTable->SetObjects(ConfigPropertyDisplayObjects);

	// We need a column for each property in our Helper class.
	for (UProperty* NextProperty = UPropertyConfigFileDisplayRow::StaticClass()->PropertyLink; NextProperty; NextProperty = NextProperty->PropertyLinkNext)
	{
		PropertyTable->AddColumn((TWeakObjectPtr<UProperty>)NextProperty);
	}

	// Ensure the columns cannot be removed.
	TArray<TSharedRef<IPropertyTableColumn>> Columns = PropertyTable->GetColumns();
	for (TSharedRef<IPropertyTableColumn> Column : Columns)
	{
		Column->SetFrozen(true);
	}

	// Create the 'Config File' vs 'Property' table
	PropertyTable->RequestRefresh();
}


#undef LOCTEXT_NAMESPACE