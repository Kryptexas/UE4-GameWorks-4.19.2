// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	TextureEditorToolkit.cpp: Implements the FTextureEditorToolkit class.
=============================================================================*/

#include "TextureEditorPrivatePCH.h"

#include "Factories.h"


#define LOCTEXT_NAMESPACE "FTextureEditorToolkit"

DEFINE_LOG_CATEGORY_STATIC(LogTextureEditor, Log, All);

#define MIPLEVEL_MIN 0
#define MIPLEVEL_MAX 15
#define EXPOSURE_MIN -10
#define EXPOSURE_MAX 10


const FName FTextureEditorToolkit::ViewportTabId(TEXT("TextureEditor_Viewport"));
const FName FTextureEditorToolkit::PropertiesTabId(TEXT("TextureEditor_Properties"));


/* FTextureEditorToolkit structors
 *****************************************************************************/

FTextureEditorToolkit::~FTextureEditorToolkit( )
{
	FAssetEditorToolkit::OnPreReimport().RemoveAll(this);
	FAssetEditorToolkit::OnPostReimport().RemoveAll(this);

	GEditor->UnregisterForUndo(this);
}


/* FAssetEditorToolkit interface
 *****************************************************************************/

void FTextureEditorToolkit::RegisterTabSpawners( const TSharedRef<class FTabManager>& TabManager )
{
	FAssetEditorToolkit::RegisterTabSpawners(TabManager);

	const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();

	TabManager->RegisterTabSpawner( ViewportTabId, FOnSpawnTab::CreateSP(this, &FTextureEditorToolkit::SpawnTab_Viewport) )
		.SetDisplayName( LOCTEXT("ViewportTab", "Viewport") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );
	
	TabManager->RegisterTabSpawner( PropertiesTabId, FOnSpawnTab::CreateSP(this, &FTextureEditorToolkit::SpawnTab_Properties) )
		.SetDisplayName( LOCTEXT("PropertiesTab", "Details") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );
}


void FTextureEditorToolkit::UnregisterTabSpawners( const TSharedRef<class FTabManager>& TabManager )
{
	FAssetEditorToolkit::UnregisterTabSpawners(TabManager);

	TabManager->UnregisterTabSpawner( ViewportTabId );
	TabManager->UnregisterTabSpawner( PropertiesTabId );
}


void FTextureEditorToolkit::InitTextureEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UObject* ObjectToEdit )
{
	FAssetEditorToolkit::OnPreReimport().AddRaw(this, &FTextureEditorToolkit::OnPreReimport);
	FAssetEditorToolkit::OnPostReimport().AddRaw(this, &FTextureEditorToolkit::OnPostReimport);

	Texture = CastChecked<UTexture>(ObjectToEdit);

	// Support undo/redo
	Texture->SetFlags(RF_Transactional);
	GEditor->RegisterForUndo(this);

	// initialize view options
	bIsRedChannel = true;
	bIsGreenChannel = true;
	bIsBlueChannel = true;
	bIsAlphaChannel = false;
	bIsSaturation = false;

	PreviewEffectiveTextureWidth = 0;
	PreviewEffectiveTextureHeight = 0;
	SpecifiedMipLevel = 0;

	SavedCompressionSetting = false;

	const UTextureEditorSettings& Settings = *GetDefault<UTextureEditorSettings>();
	Zoom = Settings.FillViewport ? 0.0f : 1.0f;

	// Register our commands. This will only register them if not previously registered
	FTextureEditorCommands::Register();

	BindCommands();

	CreateInternalWidgets();

	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_TextureEditor_Layout_v3")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
				->SetOrientation(Orient_Horizontal)
				->Split
				(
					FTabManager::NewSplitter()
						->SetOrientation(Orient_Vertical)
						->SetSizeCoefficient(0.66f)
						->Split
						(
							FTabManager::NewStack()
								->AddTab(GetToolbarTabId(), ETabState::OpenedTab)
								->SetHideTabWell(true)
								->SetSizeCoefficient(0.1f)
								
						)
						->Split
						(
							FTabManager::NewStack()
								->AddTab(ViewportTabId, ETabState::OpenedTab)
								->SetHideTabWell(true)
								->SetSizeCoefficient(0.9f)
						)
				)
				->Split
				(
					FTabManager::NewStack()
						->AddTab(PropertiesTabId, ETabState::OpenedTab)
						->SetSizeCoefficient(0.33f)
				)
		);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, TextureEditorAppIdentifier, StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, ObjectToEdit);
	
	ITextureEditorModule* TextureEditorModule = &FModuleManager::LoadModuleChecked<ITextureEditorModule>("TextureEditor");
	AddMenuExtender(TextureEditorModule->GetMenuExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));

	ExtendToolBar();
	RegenerateMenusAndToolbars();

	// @todo toolkit world centric editing
	/*if(IsWorldCentricAssetEditor())
	{
		SpawnToolkitTab(GetToolbarTabId(), FString(), EToolkitTabSpot::ToolBar);
		SpawnToolkitTab(ViewportTabId, FString(), EToolkitTabSpot::Viewport);
		SpawnToolkitTab(PropertiesTabId, FString(), EToolkitTabSpot::Details);
	}*/
}


bool FTextureEditorToolkit::IsCubeTexture( ) const
{
	return (Texture->IsA(UTextureCube::StaticClass()) || Texture->IsA(UTextureRenderTargetCube::StaticClass()));
}


/* ITextureEditorToolkit interface
 *****************************************************************************/

void FTextureEditorToolkit::CalculateTextureDimensions( uint32& Width, uint32& Height ) const
{
	uint32 ImportedWidth = Texture->Source.GetSizeX();
	uint32 ImportedHeight = Texture->Source.GetSizeY();

	// if Original Width and Height are 0, use the saved current width and height
	if ((ImportedWidth == 0) && (ImportedHeight == 0))
	{
		ImportedWidth = Texture->GetSurfaceWidth();
		ImportedHeight = Texture->GetSurfaceHeight();
	}

	Width = ImportedWidth;
	Height = ImportedHeight;

	// catch if the Width and Height are still zero for some reason
	if ((Width == 0) || (Height == 0))
	{
		Width = 0;
		Height= 0;

		return;
	}

	// See if we need to uniformly scale it to fit in viewport
	// Cap the size to effective dimensions
	uint32 ViewportW = TextureViewport->GetViewport()->GetSizeXY().X;
	uint32 ViewportH = TextureViewport->GetViewport()->GetSizeXY().Y;
	uint32 MaxWidth; 
	uint32 MaxHeight;

	const UTextureEditorSettings& Settings = *GetDefault<UTextureEditorSettings>();

	if (Settings.FillViewport)
	{
		// Subtract off the viewport space devoted to padding (2 * PreviewPadding)
		// so that the texture is padded on all sides
		MaxWidth = ViewportW;
		MaxHeight = ViewportH;

		if (IsCubeTexture())
		{
			// Cubes are displayed 2:1. 2x width if the source exists and is not an unwrapped image.
			const bool bMultipleSourceImages = Texture->Source.GetNumSlices() > 1;
			const bool bNoSourceImage = Texture->Source.GetNumSlices() == 0;
			Width *= (bNoSourceImage || bMultipleSourceImages) ? 2 : 1;
		}

		// First, scale up based on the size of the viewport
		if (MaxWidth > MaxHeight)
		{
			Height = Height * MaxWidth / Width;
			Width = MaxWidth;
		}
		else
		{
			Width = Width * MaxHeight / Height;
			Height = MaxHeight;
		}

		// then, scale again if our width and height is impacted by the scaling
		if (Width > MaxWidth)
		{
			Height = Height * MaxWidth / Width;
			Width = MaxWidth;
		}
		if (Height > MaxHeight)
		{
			Width = Width * MaxHeight / Height;
			Height = MaxHeight;
		}
	}
	else
	{
		Width = PreviewEffectiveTextureWidth * Zoom;
		Height = PreviewEffectiveTextureHeight * Zoom;
	}
}


ESimpleElementBlendMode FTextureEditorToolkit::GetColourChannelBlendMode( ) const
{
	if (Texture && (Texture->CompressionSettings == TC_Grayscale || Texture->CompressionSettings == TC_Alpha)) 
	{
		return SE_BLEND_Opaque;
	}

	// Add the red, green, blue, alpha and desaturation flags to the enum to identify the chosen filters
	uint32 Result = (uint32)SE_BLEND_RGBA_MASK_START;
	Result += bIsRedChannel ? (1 << 0) : 0;
	Result += bIsGreenChannel ? (1 << 1) : 0;
	Result += bIsBlueChannel ? (1 << 2) : 0;
	Result += bIsAlphaChannel ? (1 << 3) : 0;
	
	// If we only have one color channel active, enable color desaturation by default
	const int32 NumColorChannelsActive = (bIsRedChannel ? 1 : 0) + (bIsGreenChannel ? 1 : 0) + (bIsBlueChannel ? 1 : 0);
	const bool bIsSaturationLocal = bIsSaturation ? true : (NumColorChannelsActive==1);
	Result += bIsSaturationLocal ? (1 << 4) : 0;

	return (ESimpleElementBlendMode)Result;
}


void FTextureEditorToolkit::PopulateQuickInfo( )
{
	UTexture2D* Texture2D = Cast<UTexture2D>(Texture);
	UTextureRenderTarget2D* Texture2DRT = Cast<UTextureRenderTarget2D>(Texture);
	UTextureRenderTargetCube* TextureCubeRT = Cast<UTextureRenderTargetCube>(Texture);
	UTextureCube* TextureCube = Cast<UTextureCube>(Texture);

	uint32 ImportedWidth = Texture->Source.GetSizeX();
	uint32 ImportedHeight = Texture->Source.GetSizeY();
	uint32 Width, Height;

	Texture->CachedCombinedLODBias = GSystemSettings.TextureLODSettings.CalculateLODBias(Texture, !GetUseSpecifiedMip());

	// If Original Width and Height are 0, use the saved current width and height
	if (ImportedWidth == 0 && ImportedHeight == 0)
	{
		ImportedWidth = Texture->GetSurfaceWidth();
		ImportedHeight = Texture->GetSurfaceHeight();
	}

	const int32 MipLevel = FMath::Max( GetMipLevel(), Texture->CachedCombinedLODBias );
	CalculateEffectiveTextureDimensions(MipLevel, PreviewEffectiveTextureWidth, PreviewEffectiveTextureHeight);
	uint32 MaxInGameWidth = PreviewEffectiveTextureWidth;
	uint32 MaxInGameHeight = PreviewEffectiveTextureHeight;

	// Cubes are previewed as unwrapped 2D textures.
	// These have 2x the width of a cube face.
	PreviewEffectiveTextureWidth *= IsCubeTexture() ? 2 : 1;

	CalculateTextureDimensions(Width, Height);

	ImportedText->SetText( FText::Format( NSLOCTEXT("TextureEditor", "QuickInfo_Imported", "Imported: {0}x{1}"), FText::AsNumber( ImportedWidth ), FText::AsNumber( ImportedHeight ) ) );
	CurrentText->SetText(  FText::Format( NSLOCTEXT("TextureEditor", "QuickInfo_Current", "Current: {0}x{1}"), FText::AsNumber( FMath::Max((uint32)1, ImportedWidth >> MipLevel) ), FText::AsNumber( FMath::Max((uint32)1, ImportedHeight >> MipLevel)) ) );
	MaxInGameText->SetText(FText::Format( NSLOCTEXT("TextureEditor", "QuickInfo_MaxInGame", "Max In-Game: {0}x{1}"), FText::AsNumber( MaxInGameWidth ), FText::AsNumber( MaxInGameHeight )));
	MethodText->SetText(   FText::Format( NSLOCTEXT("TextureEditor", "QuickInfo_Method", "Method: {0}"), Texture->NeverStream ? NSLOCTEXT("TextureEditor", "QuickInfo_MethodNotStreamed", "Not Streamed") : NSLOCTEXT("TextureEditor", "QuickInfo_MethodStreamed", "Streamed")));
	LODBiasText->SetText(  FText::Format( NSLOCTEXT("TextureEditor", "QuickInfo_LODBias", "Combined LOD Bias: {0}"), FText::AsNumber( Texture->CachedCombinedLODBias )));
	
	int32 TextureFormatIndex = PF_MAX;
	
	if (Texture2D)
	{
		TextureFormatIndex = Texture2D->GetPixelFormat();
	}
	else if (TextureCube)
	{
		TextureFormatIndex = TextureCube->GetPixelFormat();
	}
	else if (Texture2DRT)
	{
		TextureFormatIndex = Texture2DRT->GetFormat();
	}

	if (TextureFormatIndex != PF_MAX)
	{
		FormatText->SetText(FText::Format( NSLOCTEXT("TextureEditor", "QuickInfo_Format", "Format: {0}"), FText::FromString( GPixelFormats[TextureFormatIndex].Name )));
	}
}


/* IToolkit interface
 *****************************************************************************/

FText FTextureEditorToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "Texture Editor");
}


FString FTextureEditorToolkit::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "Texture ").ToString();
}


TSharedRef<SDockTab> FTextureEditorToolkit::SpawnTab_Viewport(const FSpawnTabArgs& Args)
{
	check( Args.GetTabId() == ViewportTabId );

	return SNew(SDockTab)
		.Label(LOCTEXT("TextureViewportTitle", "Viewport"))
		[
			TextureViewport.ToSharedRef()
		];
}


TSharedRef<SDockTab> FTextureEditorToolkit::SpawnTab_Properties(const FSpawnTabArgs& Args)
{
	check( Args.GetTabId() == PropertiesTabId );

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("TextureEditor.Tabs.Properties"))
		.Label(LOCTEXT("TexturePropertiesTitle", "Details"))
		[
			TextureProperties.ToSharedRef()
		];

	PopulateQuickInfo();

	return SpawnedTab;
}


/* FGCObject interface
 *****************************************************************************/

void FTextureEditorToolkit::AddReferencedObjects( FReferenceCollector& Collector )
{
	Collector.AddReferencedObject(Texture);
	TextureViewport->AddReferencedObjects(Collector);
}


/* FTextureEditorToolkit static functions
 *****************************************************************************/

void FTextureEditorToolkit::CreateInternalWidgets()
{
	TextureViewport = SNew(STextureEditorViewport, SharedThis(this));

	TextureProperties = SNew(SVerticalBox)

	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		[
			SNew(SBorder)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
						.FillWidth(0.5f)
						[
							SNew(SVerticalBox)

							+ SVerticalBox::Slot()
								.AutoHeight()
								.VAlign(VAlign_Center)
								.Padding(4.0f)
								[
									SAssignNew(ImportedText, STextBlock)
								]

							+ SVerticalBox::Slot()
								.AutoHeight()
								.VAlign(VAlign_Center)
								.Padding(4.0f)
								[
									SAssignNew(CurrentText, STextBlock)
								]

							+ SVerticalBox::Slot()
								.AutoHeight()
								.VAlign(VAlign_Center)
								.Padding(4.0f)
								[
									SAssignNew(MaxInGameText, STextBlock)
								]
						]

					+ SHorizontalBox::Slot()
						.FillWidth(0.5f)
						[
							SNew(SVerticalBox)

							+ SVerticalBox::Slot()
								.AutoHeight()
								.VAlign(VAlign_Center)
								.Padding(4.0f)
								[
									SAssignNew(MethodText, STextBlock)
								]

							+ SVerticalBox::Slot()
								.AutoHeight()
								.VAlign(VAlign_Center)
								.Padding(4.0f)
								[
									SAssignNew(FormatText, STextBlock)
								]

							+ SVerticalBox::Slot()
								.AutoHeight()
								.VAlign(VAlign_Center)
								.Padding(4.0f)
								[
									SAssignNew(LODBiasText, STextBlock)
								]
						]
				]
		]

	+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(2.0f)
		[
			SNew(SBorder)
				.Padding(4.0f)
				[
					BuildTexturePropertiesWidget()
				]
		];
}


TSharedRef<SWidget> FTextureEditorToolkit::BuildTexturePropertiesWidget()
{
	FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	TexturePropertiesWidget = PropertyModule.CreateDetailView(Args);
	TexturePropertiesWidget->SetObject( Texture );

	return TexturePropertiesWidget.ToSharedRef();
}


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FTextureEditorToolkit::ExtendToolBar()
{
	struct Local
	{
		static void FillToolbar(FToolBarBuilder& ToolbarBuilder, const TSharedRef< FUICommandList > ToolkitCommands, TSharedRef<SWidget> LODControl)
		{
			ToolbarBuilder.BeginSection("TextureMisc");
			{
				ToolbarBuilder.AddToolBarButton(FTextureEditorCommands::Get().CompressNow);
				ToolbarBuilder.AddToolBarButton(FTextureEditorCommands::Get().Reimport);
			}
			ToolbarBuilder.EndSection();
	
			ToolbarBuilder.BeginSection("TextureMipAndExposure");
			{
				ToolbarBuilder.AddWidget(LODControl);
			}
			ToolbarBuilder.EndSection();
		}
	};
	
	TSharedRef<SWidget> LODControl =
		SNew(SBox)
		.WidthOverride(240.0f)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.MaxWidth(240.0f)
			.Padding(0.0f,0.0f,0.0f,0.0f)
			.VAlign(VAlign_Center)
			[
				// Mip and exposure controls
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.Padding(4.0f,0.0f,4.0f,0.0f)
				.AutoWidth()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.AutoWidth()
					[
						SNew( SCheckBox )
						.IsChecked( this, &FTextureEditorToolkit::OnGetUseSpecifiedMip )
						.IsEnabled( this, &FTextureEditorToolkit::OnGetUseSpecifiedMipEnabled )
						.OnCheckStateChanged( this, &FTextureEditorToolkit::OnUseSpecifiedMipChanged )
					]
				]
				+SHorizontalBox::Slot()
					.Padding(4.0f,0.0f,4.0f,0.0f)
					.FillWidth(1)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.Padding(0.0f,0.0,4.0,0.0)
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(NSLOCTEXT("TextureEditor", "MipLevel", "Mip Level: "))
						]
						+SHorizontalBox::Slot()
							.VAlign(VAlign_Center)
							.FillWidth(1.0f)
							[
								SNew(SNumericEntryBox<int32>)
								.AllowSpin(true)
								.MinSliderValue(MIPLEVEL_MIN)
								.MaxSliderValue(this, &FTextureEditorToolkit::GetMaxMipLevel)
								.Value(this, &FTextureEditorToolkit::GetWidgetMipLevel)
								.OnValueChanged(this, &FTextureEditorToolkit::OnMipLevelChanged)
								.IsEnabled(this, &FTextureEditorToolkit::GetUseSpecifiedMip)
							]
						+SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(2.0f)
							[
								SNew(SButton)
								.Text(NSLOCTEXT("TextureEditor", "MipMinus", "-"))
								.OnClicked(this, &FTextureEditorToolkit::OnMipLevelDown)
								.IsEnabled(this, &FTextureEditorToolkit::GetUseSpecifiedMip)
							]
						+SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(2.0f)
							[
								SNew(SButton)
								.Text(NSLOCTEXT("TextureEditor", "MipPlus", "+"))
								.OnClicked(this, &FTextureEditorToolkit::OnMipLevelUp)
								.IsEnabled(this, &FTextureEditorToolkit::GetUseSpecifiedMip)
							]
					]
			]
		];

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateStatic( &Local::FillToolbar, GetToolkitCommands(), LODControl )
	);
	
	AddToolbarExtender(ToolbarExtender);

	ITextureEditorModule* TextureEditorModule = &FModuleManager::LoadModuleChecked<ITextureEditorModule>( "TextureEditor" );
	AddToolbarExtender(TextureEditorModule->GetToolBarExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


void FTextureEditorToolkit::BindCommands()
{
	const FTextureEditorCommands& Commands = FTextureEditorCommands::Get();

	ToolkitCommands->MapAction(
		Commands.RedChannel,
		FExecuteAction::CreateSP(this, &FTextureEditorToolkit::OnRedChannel),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FTextureEditorToolkit::IsRedChannelChecked));

	ToolkitCommands->MapAction(
		Commands.GreenChannel,
		FExecuteAction::CreateSP(this, &FTextureEditorToolkit::OnGreenChannel),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FTextureEditorToolkit::IsGreenChannelChecked));

	ToolkitCommands->MapAction(
		Commands.BlueChannel,
		FExecuteAction::CreateSP(this, &FTextureEditorToolkit::OnBlueChannel),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FTextureEditorToolkit::IsBlueChannelChecked));

	ToolkitCommands->MapAction(
		Commands.AlphaChannel,
		FExecuteAction::CreateSP(this, &FTextureEditorToolkit::OnAlphaChannel),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FTextureEditorToolkit::IsAlphaChannelChecked));

	ToolkitCommands->MapAction(
		Commands.Saturation,
		FExecuteAction::CreateSP(this, &FTextureEditorToolkit::OnSaturation),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FTextureEditorToolkit::IsSaturationChecked));

	ToolkitCommands->MapAction(
		Commands.FillViewport,
		FExecuteAction::CreateSP(this, &FTextureEditorToolkit::OnFillViewport),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FTextureEditorToolkit::IsFillViewportChecked));

	ToolkitCommands->MapAction(
		Commands.CheckeredBackground,
		FExecuteAction::CreateSP(this, &FTextureEditorToolkit::HandleBackgroundActionExecute, TextureEditorBackground_Checkered),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FTextureEditorToolkit::HandleBackgroundActionIsChecked, TextureEditorBackground_Checkered));

	ToolkitCommands->MapAction(
		Commands.CheckeredBackgroundFill,
		FExecuteAction::CreateSP(this, &FTextureEditorToolkit::HandleBackgroundActionExecute, TextureEditorBackground_CheckeredFill),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FTextureEditorToolkit::HandleBackgroundActionIsChecked, TextureEditorBackground_CheckeredFill));

	ToolkitCommands->MapAction(
		Commands.SolidBackground,
		FExecuteAction::CreateSP(this, &FTextureEditorToolkit::HandleBackgroundActionExecute, TextureEditorBackground_SolidColor),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FTextureEditorToolkit::HandleBackgroundActionIsChecked, TextureEditorBackground_SolidColor));

	ToolkitCommands->MapAction(
		Commands.TextureBorder,
		FExecuteAction::CreateSP(this, &FTextureEditorToolkit::OnShowBorder),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FTextureEditorToolkit::IsShowBorderChecked));

	ToolkitCommands->MapAction(
		Commands.CompressNow,
		FExecuteAction::CreateSP(this, &FTextureEditorToolkit::OnCompressNow),
		FCanExecuteAction::CreateSP(this, &FTextureEditorToolkit::OnCompressNowEnabled));

	ToolkitCommands->MapAction(
		Commands.Reimport,
		FExecuteAction::CreateSP(this, &FTextureEditorToolkit::OnReimport));

	ToolkitCommands->MapAction(
		Commands.Settings,
		FExecuteAction::CreateSP(this, &FTextureEditorToolkit::HandleSettingsActionExecute));
}


void FTextureEditorToolkit::CalculateEffectiveTextureDimensions(int32 LODBias, uint32& EffectiveTextureWidth, uint32& EffectiveTextureHeight)
{
	//Calculate in-game max resolution and store in EffectiveTextureWidth, EffectiveTextureHeight
	GSystemSettings.TextureLODSettings.ComputeInGameMaxResolution(LODBias, *Texture, (uint32 &)EffectiveTextureWidth, (uint32 &)EffectiveTextureHeight);
}


void FTextureEditorToolkit::OnRedChannel()
{
	bIsRedChannel = !bIsRedChannel;
}


bool FTextureEditorToolkit::IsRedChannelChecked() const
{
	return bIsRedChannel;
}


void FTextureEditorToolkit::OnGreenChannel()
{
	bIsGreenChannel = !bIsGreenChannel;
}


bool FTextureEditorToolkit::IsGreenChannelChecked() const
{
	return bIsGreenChannel;
}


void FTextureEditorToolkit::OnBlueChannel()
{
	bIsBlueChannel = !bIsBlueChannel;
}


bool FTextureEditorToolkit::IsBlueChannelChecked() const
{
	return bIsBlueChannel;
}


void FTextureEditorToolkit::OnAlphaChannel()
{
	bIsAlphaChannel = !bIsAlphaChannel;
}


bool FTextureEditorToolkit::OnAlphaChannelCanExecute() const
{
	const UTexture2D* Texture2D = Cast<UTexture2D>(Texture);

	if (Texture2D == NULL)
	{
		return false;
	}

	return Texture2D->HasAlphaChannel();
}


bool FTextureEditorToolkit::IsAlphaChannelChecked() const
{
	return bIsAlphaChannel;
}


void FTextureEditorToolkit::OnSaturation()
{
	bIsSaturation = !bIsSaturation;
}


bool FTextureEditorToolkit::IsSaturationChecked() const
{
	return bIsSaturation;
}


void FTextureEditorToolkit::OnFillViewport()
{
	UTextureEditorSettings* Settings = Cast<UTextureEditorSettings>(UTextureEditorSettings::StaticClass()->GetDefaultObject());
	Settings->FillViewport = !Settings->FillViewport;
}


bool FTextureEditorToolkit::IsFillViewportChecked() const
{
	const UTextureEditorSettings& Settings = *GetDefault<UTextureEditorSettings>();

	return Settings.FillViewport;
}


void FTextureEditorToolkit::HandleBackgroundActionExecute( ETextureEditorBackgrounds Background )
{
	UTextureEditorSettings* Settings = Cast<UTextureEditorSettings>(UTextureEditorSettings::StaticClass()->GetDefaultObject());
	Settings->Background = Background;
}


bool FTextureEditorToolkit::HandleBackgroundActionIsChecked( ETextureEditorBackgrounds Background )
{
	const UTextureEditorSettings& Settings = *GetDefault<UTextureEditorSettings>();

	return (Background == Settings.Background);
}


void FTextureEditorToolkit::OnShowBorder()
{
	UTextureEditorSettings* Settings = Cast<UTextureEditorSettings>(UTextureEditorSettings::StaticClass()->GetDefaultObject());
	Settings->TextureBorderEnabled = !Settings->TextureBorderEnabled;
}


bool FTextureEditorToolkit::IsShowBorderChecked() const
{
	const UTextureEditorSettings& Settings = *GetDefault<UTextureEditorSettings>();

	return Settings.TextureBorderEnabled;
}


void FTextureEditorToolkit::OnPreReimport(UObject* InObject)
{
	// Ignore if this is regarding a different object
	if (InObject != Texture)
	{
		return;
	}

	// Prevent the texture from being compressed immediately, so the user can see the results
	SavedCompressionSetting = Texture->DeferCompression;
	Texture->DeferCompression = true;

	// Reduce the year to make sure the texture will always be reloaded even if it hasn't changed on disk
	FDateTime TimeStamp;

	if (FDateTime::Parse(Texture->SourceFileTimestamp, TimeStamp))
	{
		TimeStamp -= FTimespan(365, 0, 0, 0);
	}
	else
	{
		TimeStamp = FDateTime::MinValue();
	}

	Texture->SourceFileTimestamp = TimeStamp.ToString();
}


void FTextureEditorToolkit::OnPostReimport(UObject* InObject, bool bSuccess)
{
	// Ignore if this is regarding a different object
	if (InObject != Texture)
	{
		return;
	}

	if (!bSuccess)
	{
		// Failed, restore the compression flag
		Texture->DeferCompression = SavedCompressionSetting;
	}
}


void FTextureEditorToolkit::OnCompressNow()
{
	GWarn->BeginSlowTask( NSLOCTEXT("TextureEditor", "CompressNow", "Compressing 1 Textures that have Defer Compression set"), true);

	if (Texture->DeferCompression)
	{
		// Turn off defer compression and compress the texture
		Texture->DeferCompression = false;
		Texture->Source.Compress();
		Texture->PostEditChange();
		PopulateQuickInfo();
	}

	GWarn->EndSlowTask();
}


bool FTextureEditorToolkit::OnCompressNowEnabled() const
{
	return Texture->DeferCompression != 0;
}


void FTextureEditorToolkit::OnReimport()
{
	FReimportManager::Instance()->Reimport(Texture, /*bAskForNewFileIfMissing=*/true);
}


void FTextureEditorToolkit::HandleSettingsActionExecute()
{
	FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer("Editor", "ContentEditors", "TextureEditor");
}


TOptional<int32> FTextureEditorToolkit::GetWidgetMipLevel() const
{
	return SpecifiedMipLevel;
}


void FTextureEditorToolkit::OnMipLevelChanged(int32 NewMipLevel)
{
	SpecifiedMipLevel = FMath::Clamp<int32>( NewMipLevel, MIPLEVEL_MIN, GetMaxMipLevel().Get(MIPLEVEL_MAX) );
}


FReply FTextureEditorToolkit::OnMipLevelDown()
{
	if (SpecifiedMipLevel > MIPLEVEL_MIN)
	{
		--SpecifiedMipLevel;
	}

	return FReply::Handled();
}


FReply FTextureEditorToolkit::OnMipLevelUp()
{
	if (SpecifiedMipLevel < GetMaxMipLevel().Get(MIPLEVEL_MAX))
	{
		++SpecifiedMipLevel;
	}

	return FReply::Handled();
}


TOptional<int32> FTextureEditorToolkit::GetMaxMipLevel()const
{
	const UTexture2D* Texture2D = Cast<UTexture2D>(Texture);
	const UTextureCube* TextureCube = Cast<UTextureCube>(Texture);
	const UTextureRenderTargetCube* RTTextureCube = Cast<UTextureRenderTargetCube>(Texture);
	const UTextureRenderTarget2D* RTTexture2D = Cast<UTextureRenderTarget2D>(Texture);
	
	if (Texture2D)
	{
		return Texture2D->GetNumMips() - 1;
	}
	
	if (TextureCube)
	{
		return TextureCube->GetNumMips() - 1;
	}
	
	if (RTTextureCube)
	{
		return RTTextureCube->GetNumMips() - 1;
	}
	
	if (RTTexture2D)
	{
		return RTTexture2D->GetNumMips() - 1;
	}

	return MIPLEVEL_MAX;
}


bool FTextureEditorToolkit::GetUseSpecifiedMip() const
{
	if( GetMaxMipLevel().Get(MIPLEVEL_MAX) > 0.0f )
	{
		if( OnGetUseSpecifiedMipEnabled() )
		{
			return bUseSpecifiedMipLevel;
		}

		// by default this is on
		return true; 
	}

	// disable the widgets if we have no mips
	return false;
}


double FTextureEditorToolkit::GetZoom() const
{
	const UTextureEditorSettings& Settings = *GetDefault<UTextureEditorSettings>();

	if (Settings.FillViewport)
	{
		return 0.0;
	}

	return Zoom;
}


void FTextureEditorToolkit::SetZoom( double ZoomValue )
{
	UTextureEditorSettings* Settings = Cast<UTextureEditorSettings>(UTextureEditorSettings::StaticClass()->GetDefaultObject());

	Zoom = FMath::Clamp(ZoomValue, 0.0, MaxZoom);
	Settings->FillViewport = FMath::IsNearlyZero(Zoom);
}


void FTextureEditorToolkit::ZoomIn()
{
	SetZoom(Zoom + ZoomStep);
}


void FTextureEditorToolkit::ZoomOut()
{
	SetZoom(Zoom - ZoomStep);
}


ESlateCheckBoxState::Type FTextureEditorToolkit::OnGetUseSpecifiedMip() const
{
	return GetUseSpecifiedMip() ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}


bool FTextureEditorToolkit::OnGetUseSpecifiedMipEnabled() const
{
	UTextureCube* TextureCube = Cast<UTextureCube>(Texture);

	if( GetMaxMipLevel().Get(MIPLEVEL_MAX) <= 0.0f || TextureCube )
	{
		return false;
	}

	return true;
}


void FTextureEditorToolkit::OnUseSpecifiedMipChanged(ESlateCheckBoxState::Type InNewState)
{
	bUseSpecifiedMipLevel = InNewState == ESlateCheckBoxState::Checked;
}


#undef LOCTEXT_NAMESPACE
