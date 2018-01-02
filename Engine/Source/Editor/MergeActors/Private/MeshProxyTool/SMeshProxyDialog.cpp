// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MeshProxyTool/SMeshProxyDialog.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "Engine/MeshMerging.h"
#include "Engine/Selection.h"
#include "MeshProxyTool/MeshProxyTool.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Styling/SlateTypes.h"
#include "SlateOptMacros.h"
#include "UObject/UnrealType.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"


#define LOCTEXT_NAMESPACE "SMeshProxyDialog"

SMeshProxyDialog::SMeshProxyDialog()
{
	bRefreshListView = false;
}

SMeshProxyDialog::~SMeshProxyDialog()
{
	// Remove all delegates
	USelection::SelectionChangedEvent.RemoveAll(this);
	USelection::SelectObjectEvent.RemoveAll(this);
	FEditorDelegates::MapChange.RemoveAll(this);
	FEditorDelegates::NewCurrentLevel.RemoveAll(this);
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void  SMeshProxyDialog::Construct(const FArguments& InArgs, FMeshProxyTool* InTool)
{
	checkf(InTool != nullptr, TEXT("Invalid owner tool supplied"));
	Tool = InTool;

	UpdateSelectedStaticMeshComponents();
	CreateSettingsView();

	// Create widget layout
	this->ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 10, 0, 0)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SVerticalBox)
					// Static mesh component selection
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("MergeStaticMeshComponentsLabel", "Mesh Components to be incorporated in the merge:"))
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
					[
						SAssignNew(ComponentSelectionControl.ComponentsListView, SListView<TSharedPtr<FMergeComponentData>>)
						.ListItemsSource(&ComponentSelectionControl.SelectedComponents)
						.OnGenerateRow(this, &SMeshProxyDialog::MakeComponentListItemWidget)
						.ToolTipText(LOCTEXT("SelectedComponentsListBoxToolTip", "The selected mesh components will be incorporated into the merged mesh"))
					]
				]
			]

			+ SVerticalBox::Slot()
			.Padding(0, 10, 0, 0)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)
				// Static mesh component selection
				+ SVerticalBox::Slot()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					[
						SettingsView->AsShared()
					]
				]
			]
		]



		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor::Yellow)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			.Visibility_Lambda([this]()->EVisibility { return this->GetContentEnabledState() ? EVisibility::Collapsed : EVisibility::Visible; })
			[
				SNew(STextBlock)
				.Text(LOCTEXT("DeleteUndo", "Insufficient mesh components found for merging."))
			]
			]
		];


	// Selection change
	USelection::SelectionChangedEvent.AddRaw(this, &SMeshProxyDialog::OnLevelSelectionChanged);
	USelection::SelectObjectEvent.AddRaw(this, &SMeshProxyDialog::OnLevelSelectionChanged);
	FEditorDelegates::MapChange.AddSP(this, &SMeshProxyDialog::OnMapChange);
	FEditorDelegates::NewCurrentLevel.AddSP(this, &SMeshProxyDialog::OnNewCurrentLevel);

	ProxySettings = UMeshProxySettingsObject::Get();
	SettingsView->SetObject(ProxySettings);
}

void  SMeshProxyDialog::OnMapChange(uint32 MapFlags)
{
	Reset();
}

void  SMeshProxyDialog::OnNewCurrentLevel()
{
	Reset();
}

void  SMeshProxyDialog::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	// Check if we need to update selected components and the listbox
	if (bRefreshListView == true)
	{
		ComponentSelectionControl.UpdateSelectedCompnentsAndListBox();

		bRefreshListView = false;
	}
}


void SMeshProxyDialog::Reset()
{
	bRefreshListView = true;
}


bool SMeshProxyDialog::GetContentEnabledState() const
{
	return (GetNumSelectedMeshComponents() >= 1); // Only enabled if a mesh is selected
}

void SMeshProxyDialog::UpdateSelectedStaticMeshComponents()
{

	ComponentSelectionControl.UpdateSelectedStaticMeshComponents();
}


TSharedRef<ITableRow> SMeshProxyDialog::MakeComponentListItemWidget(TSharedPtr<FMergeComponentData> ComponentData, const TSharedRef<STableViewBase>& OwnerTable)
{

	return ComponentSelectionControl.MakeComponentListItemWidget(ComponentData, OwnerTable);
}


void SMeshProxyDialog::CreateSettingsView()
{
	// Create a property view
	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = true;
	DetailsViewArgs.bLockable = true;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::ComponentsAndActorsUseNameArea;
	DetailsViewArgs.bCustomNameAreaLocation = false;
	DetailsViewArgs.bCustomFilterAreaLocation = true;
	DetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;


	// Tiny hack to hide this setting, since we have no way / value to go off to 
	struct Local
	{
		/** Delegate to show all properties */
		static bool IsPropertyVisible(const FPropertyAndParent& PropertyAndParent, bool bInShouldShowNonEditable)
		{
			return (PropertyAndParent.Property.GetFName() != GET_MEMBER_NAME_CHECKED(FMaterialProxySettings, GutterSpace));
		}
	};

	SettingsView = EditModule.CreateDetailView(DetailsViewArgs);
	SettingsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateStatic(&Local::IsPropertyVisible, true));
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SMeshProxyDialog::OnLevelSelectionChanged(UObject* Obj)
{
	Reset();
}


void SThirdPartyMeshProxyDialog::Construct(const FArguments& InArgs, FThirdPartyMeshProxyTool* InTool)
{
	Tool = InTool;
	check(Tool != nullptr);

	CuttingPlaneOptions.Add(MakeShareable(new FString(TEXT("+X"))));
	CuttingPlaneOptions.Add(MakeShareable(new FString(TEXT("+Y"))));
	CuttingPlaneOptions.Add(MakeShareable(new FString(TEXT("+Z"))));
	CuttingPlaneOptions.Add(MakeShareable(new FString(TEXT("-X"))));
	CuttingPlaneOptions.Add(MakeShareable(new FString(TEXT("-Y"))));
	CuttingPlaneOptions.Add(MakeShareable(new FString(TEXT("-Z"))));

	TextureResolutionOptions.Add(MakeShareable(new FString(TEXT("64"))));
	TextureResolutionOptions.Add(MakeShareable(new FString(TEXT("128"))));
	TextureResolutionOptions.Add(MakeShareable(new FString(TEXT("256"))));
	TextureResolutionOptions.Add(MakeShareable(new FString(TEXT("512"))));
	TextureResolutionOptions.Add(MakeShareable(new FString(TEXT("1024"))));
	TextureResolutionOptions.Add(MakeShareable(new FString(TEXT("2048"))));

	CreateLayout();
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void  SThirdPartyMeshProxyDialog::CreateLayout()
{
	int32 TextureResEntryIndex = FindTextureResolutionEntryIndex(Tool->ProxySettings.MaterialSettings.TextureSize.X);
	int32 LightMapResEntryIndex = FindTextureResolutionEntryIndex(Tool->ProxySettings.LightMapResolution);
	TextureResEntryIndex = FMath::Max(TextureResEntryIndex, 0);
	LightMapResEntryIndex = FMath::Max(LightMapResEntryIndex, 0);
		
	this->ChildSlot
	[
		SNew(SVerticalBox)
			
		+SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(10)
		[
			// Simplygon logo
			SNew(SImage)
			.Image(FEditorStyle::GetBrush("MeshProxy.SimplygonLogo"))
		]
			
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				// Proxy options
				SNew(SVerticalBox)

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.FillWidth(0.5f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("OnScreenSizeLabel", "On Screen Size (pixels)"))
						.Font(FEditorStyle::GetFontStyle("StandardDialog.SmallFont"))
						.ToolTipText(GetPropertyToolTipText(GET_MEMBER_NAME_CHECKED(FMeshProxySettings, ScreenSize)))
					]
					+ SHorizontalBox::Slot()
					.FillWidth(0.5f)
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					[
						SNew(SBox)
						.HAlign(HAlign_Fill)
						.MinDesiredWidth(100.0f)
						.MaxDesiredWidth(100.0f)
						[
							SNew(SNumericEntryBox<int32>)
							.Font(FEditorStyle::GetFontStyle("StandardDialog.SmallFont"))
							.MinValue(40)
							.MaxValue(1200)
							.MinSliderValue(40)
							.MaxSliderValue(1200)
							.AllowSpin(true)
							.Value(this, &SThirdPartyMeshProxyDialog::GetScreenSize)
							.OnValueChanged(this, &SThirdPartyMeshProxyDialog::ScreenSizeChanged)
						]
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.FillWidth(0.5f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("MergeDistanceLabel", "Merge Distance (pixels)"))
						.Font(FEditorStyle::GetFontStyle("StandardDialog.SmallFont"))
						.ToolTipText(GetPropertyToolTipText(GET_MEMBER_NAME_CHECKED(FMeshProxySettings, MergeDistance)))
					]
					+ SHorizontalBox::Slot()
					.FillWidth(0.5f)
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					[
						SNew(SBox)
						.HAlign(HAlign_Fill)
						.MinDesiredWidth(100.0f)
						.MaxDesiredWidth(100.0f)
						[
							SNew(SNumericEntryBox<int32>)
							.Font(FEditorStyle::GetFontStyle("StandardDialog.SmallFont"))
							.MinValue(0)
							.MaxValue(300)
							.MinSliderValue(0)
							.MaxSliderValue(300)
							.AllowSpin(true)
							.Value(this, &SThirdPartyMeshProxyDialog::GetMergeDistance)
							.OnValueChanged(this, &SThirdPartyMeshProxyDialog::MergeDistanceChanged)
						]
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.FillWidth(0.5f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("TextureResolutionLabel", "Texture Resolution"))
						.Font(FEditorStyle::GetFontStyle("StandardDialog.SmallFont"))
					]
					+ SHorizontalBox::Slot()
					.FillWidth(0.5f)
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					[
						SNew(STextComboBox)
						.Font(FEditorStyle::GetFontStyle("StandardDialog.SmallFont"))
						.OptionsSource(&TextureResolutionOptions)
						.InitiallySelectedItem(TextureResolutionOptions[TextureResEntryIndex])
						.OnSelectionChanged(this, &SThirdPartyMeshProxyDialog::SetTextureResolution)
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.FillWidth(0.5f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("LightMapResolutionLabel", "LightMap Resolution"))
						.Font(FEditorStyle::GetFontStyle("StandardDialog.SmallFont"))
						.ToolTipText(GetPropertyToolTipText(GET_MEMBER_NAME_CHECKED(FMeshProxySettings, LightMapResolution)))
					]
					+ SHorizontalBox::Slot()
					.FillWidth(0.5f)
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					[
						SNew(STextComboBox)
						.Font(FEditorStyle::GetFontStyle("StandardDialog.SmallFont"))
						.OptionsSource(&TextureResolutionOptions)
						.InitiallySelectedItem(TextureResolutionOptions[LightMapResEntryIndex])
						.OnSelectionChanged(this, &SThirdPartyMeshProxyDialog::SetLightMapResolution)
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.FillWidth(0.5f)
					.VAlign(VAlign_Center)
					.Padding(0.0, 0.0, 3.0, 0.0)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("HardAngleLabel", "Hard Edge Angle"))
						.Font(FEditorStyle::GetFontStyle("StandardDialog.SmallFont"))
						.ToolTipText(GetPropertyToolTipText(GET_MEMBER_NAME_CHECKED(FMeshProxySettings, HardAngleThreshold)))
					]
					+SHorizontalBox::Slot()
					.FillWidth(0.5f)
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					[
						SNew(SBox)
						.HAlign(HAlign_Fill)
						.MinDesiredWidth(100.0f)
						.MaxDesiredWidth(100.0f)
						[
							SNew(SNumericEntryBox<float>)
							.Font(FEditorStyle::GetFontStyle("StandardDialog.SmallFont"))
							.MinValue(0.f)
							.MaxValue(180.f)
							.MinSliderValue(0.f)
							.MaxSliderValue(180.f)
							.AllowSpin(true)
							.Value(this, &SThirdPartyMeshProxyDialog::GetHardAngleThreshold)
							.OnValueChanged(this, &SThirdPartyMeshProxyDialog::HardAngleThresholdChanged)
							.IsEnabled(this, &SThirdPartyMeshProxyDialog::HardAngleThresholdEnabled)
						]
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SCheckBox)
					.Type(ESlateCheckBoxType::CheckBox)
					.IsChecked(this, &SThirdPartyMeshProxyDialog::GetRecalculateNormals)
					.OnCheckStateChanged(this, &SThirdPartyMeshProxyDialog::SetRecalculateNormals)
					.Content()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("RecalcNormalsLabel", "Recalculate Normals"))
						.Font(FEditorStyle::GetFontStyle("StandardDialog.SmallFont"))
						.ToolTipText(GetPropertyToolTipText(GET_MEMBER_NAME_CHECKED(FMeshProxySettings, bRecalculateNormals)))
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SCheckBox)
					.Type(ESlateCheckBoxType::CheckBox)
					.IsChecked(this, &SThirdPartyMeshProxyDialog::GetExportNormalMap)
					.OnCheckStateChanged(this, &SThirdPartyMeshProxyDialog::SetExportNormalMap)
					.Content()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ExportNormalMapLabel", "Export Normal Map"))
						.Font(FEditorStyle::GetFontStyle("StandardDialog.SmallFont"))
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SCheckBox)
					.Type(ESlateCheckBoxType::CheckBox)
					.IsChecked(this, &SThirdPartyMeshProxyDialog::GetExportMetallicMap)
					.OnCheckStateChanged(this, &SThirdPartyMeshProxyDialog::SetExportMetallicMap)
					.Content()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ExportMetallicMapLabel", "Export Metallic Map"))
						.Font(FEditorStyle::GetFontStyle("StandardDialog.SmallFont"))
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SCheckBox)
					.Type(ESlateCheckBoxType::CheckBox)
					.IsChecked(this, &SThirdPartyMeshProxyDialog::GetExportRoughnessMap)
					.OnCheckStateChanged(this, &SThirdPartyMeshProxyDialog::SetExportRoughnessMap)
					.Content()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ExportRoughnessMapLabel", "Export Roughness Map"))
						.Font(FEditorStyle::GetFontStyle("StandardDialog.SmallFont"))
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SCheckBox)
					.Type(ESlateCheckBoxType::CheckBox)
					.IsChecked(this, &SThirdPartyMeshProxyDialog::GetExportSpecularMap)
					.OnCheckStateChanged(this, &SThirdPartyMeshProxyDialog::SetExportSpecularMap)
					.Content()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ExportSpecularMapLabel", "Export Specular Map"))
						.Font(FEditorStyle::GetFontStyle("StandardDialog.SmallFont"))
					]
				]
			]
		]
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

int32 SThirdPartyMeshProxyDialog::FindTextureResolutionEntryIndex(int32 InResolution) const
{
	FString ResolutionStr = TTypeToString<int32>::ToString(InResolution);
	
	int32 Result = TextureResolutionOptions.IndexOfByPredicate([&](const TSharedPtr<FString>& Entry)
	{
		return (ResolutionStr == *Entry);
	});

	return Result;
}

FText SThirdPartyMeshProxyDialog::GetPropertyToolTipText(const FName& PropertyName) const
{
	UProperty* Property = FMeshProxySettings::StaticStruct()->FindPropertyByName(PropertyName);
	if (Property)
	{
		return Property->GetToolTipText();
	}
	
	return FText::GetEmpty();
}

//Screen size
TOptional<int32> SThirdPartyMeshProxyDialog::GetScreenSize() const
{
	return Tool->ProxySettings.ScreenSize;
}

void SThirdPartyMeshProxyDialog::ScreenSizeChanged(int32 NewValue)
{
	Tool->ProxySettings.ScreenSize = NewValue;
}

//Recalculate normals
ECheckBoxState SThirdPartyMeshProxyDialog::GetRecalculateNormals() const
{
	return Tool->ProxySettings.bRecalculateNormals ? ECheckBoxState::Checked: ECheckBoxState::Unchecked;
}

void SThirdPartyMeshProxyDialog::SetRecalculateNormals(ECheckBoxState NewValue)
{
	Tool->ProxySettings.bRecalculateNormals = (NewValue == ECheckBoxState::Checked);
}

//Hard Angle Threshold
bool SThirdPartyMeshProxyDialog::HardAngleThresholdEnabled() const
{
	if(Tool->ProxySettings.bRecalculateNormals)
	{
		return true;
	}

	return false;
}

TOptional<float> SThirdPartyMeshProxyDialog::GetHardAngleThreshold() const
{
	return Tool->ProxySettings.HardAngleThreshold;
}

void SThirdPartyMeshProxyDialog::HardAngleThresholdChanged(float NewValue)
{
	Tool->ProxySettings.HardAngleThreshold = NewValue;
}

//Merge Distance
TOptional<int32> SThirdPartyMeshProxyDialog::GetMergeDistance() const
{
	return Tool->ProxySettings.MergeDistance;
}

void SThirdPartyMeshProxyDialog::MergeDistanceChanged(int32 NewValue)
{
	Tool->ProxySettings.MergeDistance = NewValue;
}

//Texture Resolution
void SThirdPartyMeshProxyDialog::SetTextureResolution(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	int32 Resolution = 512;
	TTypeFromString<int32>::FromString(Resolution, **NewSelection);
	FIntPoint TextureSize(Resolution, Resolution);
	
	Tool->ProxySettings.MaterialSettings.TextureSize = TextureSize;
}

void SThirdPartyMeshProxyDialog::SetLightMapResolution(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	int32 Resolution = 256;
	TTypeFromString<int32>::FromString(Resolution, **NewSelection);
		
	Tool->ProxySettings.LightMapResolution = Resolution;
}

ECheckBoxState SThirdPartyMeshProxyDialog::GetExportNormalMap() const
{
	return Tool->ProxySettings.MaterialSettings.bNormalMap ? ECheckBoxState::Checked :  ECheckBoxState::Unchecked;
}

void SThirdPartyMeshProxyDialog::SetExportNormalMap(ECheckBoxState NewValue)
{
	Tool->ProxySettings.MaterialSettings.bNormalMap = (NewValue == ECheckBoxState::Checked);
}

ECheckBoxState SThirdPartyMeshProxyDialog::GetExportMetallicMap() const
{
	return Tool->ProxySettings.MaterialSettings.bMetallicMap ? ECheckBoxState::Checked :  ECheckBoxState::Unchecked;
}

void SThirdPartyMeshProxyDialog::SetExportMetallicMap(ECheckBoxState NewValue)
{
	Tool->ProxySettings.MaterialSettings.bMetallicMap = (NewValue == ECheckBoxState::Checked);
}

ECheckBoxState SThirdPartyMeshProxyDialog::GetExportRoughnessMap() const
{
	return Tool->ProxySettings.MaterialSettings.bRoughnessMap ? ECheckBoxState::Checked :  ECheckBoxState::Unchecked;
}

void SThirdPartyMeshProxyDialog::SetExportRoughnessMap(ECheckBoxState NewValue)
{
	Tool->ProxySettings.MaterialSettings.bRoughnessMap = (NewValue == ECheckBoxState::Checked);
}

ECheckBoxState SThirdPartyMeshProxyDialog::GetExportSpecularMap() const
{
	return Tool->ProxySettings.MaterialSettings.bSpecularMap ? ECheckBoxState::Checked :  ECheckBoxState::Unchecked;
}

void SThirdPartyMeshProxyDialog::SetExportSpecularMap(ECheckBoxState NewValue)
{
	Tool->ProxySettings.MaterialSettings.bSpecularMap = (NewValue == ECheckBoxState::Checked);
}


#undef LOCTEXT_NAMESPACE
