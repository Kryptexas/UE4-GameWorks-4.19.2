// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MaterialEditorModule.h"
#include "Modules/ModuleManager.h"
#include "IMaterialEditor.h"
#include "MaterialEditor.h"
#include "MaterialEditorUtilities.h"
#include "MaterialInstanceEditor.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialFunctionInstance.h"
#include "Settings/EditorExperimentalSettings.h"
#include "ContentBrowserModule.h"
#include "EditorStyleSet.h"
#include "ContentBrowserDelegates.h"
#include "IPluginManager.h"
#include "IIntroTutorials.h"
#include "SDockTab.h"
#include "EditorTutorial.h"
#include "SlateApplication.h"
#include "SImage.h"
#include "Misc/EngineBuildSettings.h"

const FName MaterialEditorAppIdentifier = FName(TEXT("MaterialEditorApp"));
const FName MaterialInstanceEditorAppIdentifier = FName(TEXT("MaterialInstanceEditorApp"));

class SNewSubstanceMenuEntry : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SNewSubstanceMenuEntry)
		: _Width(32)
		, _Height(32)
	{}
		SLATE_ARGUMENT(uint32, Width)
		SLATE_ARGUMENT(uint32, Height)
		SLATE_ARGUMENT(const FSlateBrush*, Icon)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		const FLinearColor AssetColor = FColor(255, 128, 0);
		ChildSlot
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(4, 0, 0, 0)
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SBox)
					.WidthOverride(InArgs._Width + 4)
					.HeightOverride(InArgs._Height + 4)
					[
						SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("AssetThumbnail.AssetBackground"))
						.BorderBackgroundColor(AssetColor.CopyWithNewOpacity(0.3f))
						.Padding(2.0f)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						[
							SNew(SImage)
							.Image(InArgs._Icon)
						]
					]
				]
				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Bottom)
				[
					SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("WhiteBrush"))
					.BorderBackgroundColor(AssetColor)
					.Padding(FMargin(0, FMath::Max(FMath::CeilToFloat(InArgs._Width*0.025f), 3.0f), 0, 0))
				]
			]

			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.Padding(4, 0, 4, 0)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.Padding(0, 0, 0, 1)
				.AutoHeight()
				[
					SNew(STextBlock)
					.Font(FEditorStyle::GetFontStyle("LevelViewportContextMenu.AssetLabel.Text.Font"))
					.Text(NSLOCTEXT("MaterialEditor", "GetContentText_Substance", "Substance"))
				]
			]
		];
	}
};



/**
 * Material editor module
 */
class FMaterialEditorModule : public IMaterialEditorModule
{
public:
	/** Constructor, set up console commands and variables **/
	FMaterialEditorModule()
	{
	}

	/**
	 * Called right after the module DLL has been loaded and the module object has been created
	 */
	virtual void StartupModule() override
	{
		MenuExtensibilityManager = MakeShareable(new FExtensibilityManager);
		ToolBarExtensibilityManager = MakeShareable(new FExtensibilityManager);

		if(!FEngineBuildSettings::IsInternalBuild() && !FEngineBuildSettings::IsSourceDistribution())
		{
			TSharedPtr<IPlugin> SubstancePlugin = IPluginManager::Get().FindPlugin(TEXT("Substance"));
			if (!SubstancePlugin.IsValid() )
			{
				// Extend content browser menu
				{
					FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
					ContentBrowserModule.GetAllAssetContextMenuExtenders().Add(FContentBrowserMenuExtender_SelectedPaths::CreateRaw(this, &FMaterialEditorModule::ExtendContentBrowserAssetContextMenu));
					ContentBrowserAssetExtenderDelegateHandle = ContentBrowserModule.GetAllAssetContextMenuExtenders().Last().GetHandle();
				}

				// Extend material editor toolbar
				{

					TSharedRef<FExtender> ToolbarExtender = MakeShareable(new FExtender);
					ToolbarExtender->AddToolBarExtension("Graph", EExtensionHook::After, nullptr,
						FToolBarExtensionDelegate::CreateLambda([this](FToolBarBuilder& ToolbarBuilder)
					{
						ToolbarBuilder.BeginSection("Substance");
						ToolbarBuilder.AddToolBarButton(
							FUIAction(FExecuteAction::CreateRaw(this, &FMaterialEditorModule::OpenSubstancePluginGetter)),
							NAME_None,
							NSLOCTEXT("MaterialEditor", "GetContentText_SubstanceToolbar", "Substance"),
							NSLOCTEXT("MaterialEditor", "GetContentText_Substance", "Add Substance textures to the project"),
							FSlateIcon(FEditorStyle::GetStyleSetName(), "MaterialEditor.AddSubstanceSpecialToolbar")
						);
						ToolbarBuilder.EndSection();
					})
					);

					ToolBarExtensibilityManager->AddExtender(ToolbarExtender);
				}
			}
		}

	}
	
	TSharedRef<FExtender> ExtendContentBrowserAssetContextMenu(const TArray<FString>& SelectedPaths)
	{
		TSharedRef<FExtender> Extender = MakeShared<FExtender>();
		Extender->AddMenuExtension(
			"ContentBrowserNewBasicAsset",
			EExtensionHook::After,
			TSharedPtr<FUICommandList>(),
			FMenuExtensionDelegate::CreateRaw(this, &FMaterialEditorModule::ContentBrowserExtenderFunc, SelectedPaths)
		);
		return Extender;
	}

	void ContentBrowserExtenderFunc(FMenuBuilder& MenuBuilder, const TArray<FString> SelectedPaths)
	{
		MenuBuilder.AddMenuEntry(
			FUIAction(FExecuteAction::CreateRaw(this, &FMaterialEditorModule::OpenSubstancePluginGetter)),
			SNew(SNewSubstanceMenuEntry).Icon(FEditorStyle::GetBrush("MaterialEditor.AddSubstanceSpecialMenu"))
		);
	}

	void OpenSubstancePluginGetter()
	{
		TSharedPtr<SWindow> Window = nullptr;
		TSharedPtr<SWidget> Widget = FSlateApplication::Get().GetKeyboardFocusedWidget();
		if (Widget.IsValid())
		{
			Window = FSlateApplication::Get().FindWidgetWindow(Widget.ToSharedRef());
		}

		IIntroTutorials::Get().LaunchTutorial(TEXT("/Engine/Tutorial/SubEditors/GettingSubstance"), Window);
	}

	/**
	 * Called before the module is unloaded, right before the module object is destroyed.
	 */
	virtual void ShutdownModule() override
	{
		MenuExtensibilityManager.Reset();
		ToolBarExtensibilityManager.Reset();

		if (FModuleManager::Get().IsModuleLoaded(TEXT("ContentBrowser")))
		{
			FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
			ContentBrowserModule.GetAllAssetContextMenuExtenders().RemoveAll([this](const auto& Delegate) { return Delegate.GetHandle() == ContentBrowserAssetExtenderDelegateHandle; });
		}
	}

	/**
	 * Creates a new material editor, either for a material or a material function
	 */
	virtual TSharedRef<IMaterialEditor> CreateMaterialEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UMaterial* Material) override
	{
		TSharedRef<FMaterialEditor> NewMaterialEditor(new FMaterialEditor());
		NewMaterialEditor->InitEditorForMaterial(Material);
		OnMaterialEditorOpened().Broadcast(NewMaterialEditor);
		NewMaterialEditor->InitMaterialEditor(Mode, InitToolkitHost, Material);
		return NewMaterialEditor;
	}

	virtual TSharedRef<IMaterialEditor> CreateMaterialEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UMaterialFunction* MaterialFunction) override
	{
		TSharedRef<FMaterialEditor> NewMaterialEditor(new FMaterialEditor());
		NewMaterialEditor->InitEditorForMaterialFunction(MaterialFunction);
		OnMaterialFunctionEditorOpened().Broadcast(NewMaterialEditor);
		NewMaterialEditor->InitMaterialEditor(Mode, InitToolkitHost, MaterialFunction);
		return NewMaterialEditor;
	}

	virtual TSharedRef<IMaterialEditor> CreateMaterialInstanceEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UMaterialInstance* MaterialInstance) override
	{
		TSharedRef<FMaterialInstanceEditor> NewMaterialInstanceEditor(new FMaterialInstanceEditor());
		NewMaterialInstanceEditor->InitEditorForMaterial(MaterialInstance);
		OnMaterialInstanceEditorOpened().Broadcast(NewMaterialInstanceEditor);
		NewMaterialInstanceEditor->InitMaterialInstanceEditor(Mode, InitToolkitHost, MaterialInstance);
		return NewMaterialInstanceEditor;
	}

	virtual TSharedRef<IMaterialEditor> CreateMaterialInstanceEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UMaterialFunctionInstance* MaterialFunction) override
	{
		TSharedRef<FMaterialInstanceEditor> NewMaterialInstanceEditor(new FMaterialInstanceEditor());
		NewMaterialInstanceEditor->InitEditorForMaterialFunction(MaterialFunction);
		OnMaterialInstanceEditorOpened().Broadcast(NewMaterialInstanceEditor);
		NewMaterialInstanceEditor->InitMaterialInstanceEditor(Mode, InitToolkitHost, MaterialFunction);
		return NewMaterialInstanceEditor;
	}
	
	virtual void GetVisibleMaterialParameters(const class UMaterial* Material, class UMaterialInstance* MaterialInstance, TArray<FMaterialParameterInfo>& VisibleExpressions) override
	{
		FMaterialEditorUtilities::GetVisibleMaterialParameters(Material, MaterialInstance, VisibleExpressions);
	}

	virtual bool MaterialLayersEnabled()
	{
		static auto* CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.SupportMaterialLayers"));
		return CVar && CVar->GetValueOnAnyThread() == 1;
	};

	/** Gets the extensibility managers for outside entities to extend material editor's menus and toolbars */
	virtual TSharedPtr<FExtensibilityManager> GetMenuExtensibilityManager() override { return MenuExtensibilityManager; }
	virtual TSharedPtr<FExtensibilityManager> GetToolBarExtensibilityManager() override { return ToolBarExtensibilityManager; }

private:
	TSharedPtr<FExtensibilityManager> MenuExtensibilityManager;
	TSharedPtr<FExtensibilityManager> ToolBarExtensibilityManager;
	FDelegateHandle ContentBrowserAssetExtenderDelegateHandle;
};

IMPLEMENT_MODULE( FMaterialEditorModule, MaterialEditor );
