// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEditorStyle.h"

#include "SlateApplication.h"
#include "EditorStyleSet.h"
#include "Slate/SlateGameResources.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"

TSharedPtr< FSlateStyleSet > FNiagaraEditorStyle::NiagaraEditorStyleInstance = NULL;

void FNiagaraEditorStyle::Initialize()
{
	if (!NiagaraEditorStyleInstance.IsValid())
	{
		NiagaraEditorStyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*NiagaraEditorStyleInstance);
	}
}

void FNiagaraEditorStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*NiagaraEditorStyleInstance);
	ensure(NiagaraEditorStyleInstance.IsUnique());
	NiagaraEditorStyleInstance.Reset();
}

FName FNiagaraEditorStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("NiagaraEditorStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )
#define TTF_CORE_FONT( RelativePath, ... ) FSlateFontInfo( FPaths::EngineContentDir()  / "Slate" / RelativePath + TEXT(".ttf"), __VA_ARGS__ )
#define BOX_CORE_BRUSH( RelativePath, ... ) FSlateBoxBrush( FPaths::EngineContentDir() / "Editor/Slate" / RelativePath + TEXT(".png"), __VA_ARGS__ )

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);

TSharedRef< FSlateStyleSet > FNiagaraEditorStyle::Create()
{
	const FTextBlockStyle NormalText = FEditorStyle::GetWidgetStyle<FTextBlockStyle>("NormalText");
	const FEditableTextBoxStyle NormalEditableTextBox = FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox");
	const FSpinBoxStyle NormalSpinBox = FEditorStyle::GetWidgetStyle<FSpinBoxStyle>("SpinBox");

	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("NiagaraEditorStyle"));
	Style->SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate/Niagara"));

	FTextBlockStyle CategoryText = FTextBlockStyle(NormalText)
		.SetFont(TTF_CORE_FONT("Fonts/Roboto-Regular", 10))
		.SetShadowOffset(FVector2D(0, 1))
		.SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.9f));

	Style->Set("NiagaraEditor.CategoryText", CategoryText);

	FTextBlockStyle HeadingText = FTextBlockStyle(NormalText)
		.SetFont(TTF_CORE_FONT("Fonts/Roboto-Regular", 14))
		.SetShadowOffset(FVector2D(0, 1))
		.SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.9f));

	Style->Set("NiagaraEditor.HeadingText", HeadingText);

	FEditableTextBoxStyle HeadingEditableTextBox = FEditableTextBoxStyle(NormalEditableTextBox)
		.SetFont(TTF_CORE_FONT("Fonts/Roboto-Regular", 14));

	Style->Set("NiagaraEditor.HeadingEditableTextBox", HeadingEditableTextBox);

	Style->Set("NiagaraEditor.HeadingInlineEditableText", FInlineEditableTextBlockStyle()
		.SetTextStyle(HeadingText)
		.SetEditableTextBoxStyle(HeadingEditableTextBox));

	FSlateFontInfo ParameterFont = TTF_CORE_FONT("Fonts/Roboto-Regular", 8);

	Style->Set("NiagaraEditor.ParameterFont", ParameterFont);

	FTextBlockStyle ParameterText = FTextBlockStyle(NormalText)
		.SetFont(ParameterFont);

	Style->Set("NiagaraEditor.ParameterText", ParameterText);

	FEditableTextBoxStyle ParameterEditableTextBox = FEditableTextBoxStyle(NormalEditableTextBox)
		.SetFont(ParameterFont);

	Style->Set("NiagaraEditor.ParameterEditableTextBox", ParameterEditableTextBox);

	Style->Set("NiagaraEditor.ParameterInlineEditableText", FInlineEditableTextBlockStyle()
		.SetTextStyle(ParameterText)
		.SetEditableTextBoxStyle(ParameterEditableTextBox));

	FSpinBoxStyle ParameterSpinBox = FSpinBoxStyle(NormalSpinBox)
		.SetTextPadding(1);

	Style->Set("NiagaraEditor.ParameterSpinbox", ParameterSpinBox);

	Style->Set("NiagaraEditor.Compile", new IMAGE_BRUSH("Icons/icon_compile_40x", Icon40x40));
	Style->Set("NiagaraEditor.Compile.Small", new IMAGE_BRUSH("Icons/icon_compile_40x", Icon20x20));

	Style->Set("Niagara.CompileStatus.Unknown", new IMAGE_BRUSH("Icons/CompileStatus_Working", Icon40x40));
	Style->Set("Niagara.CompileStatus.Error",   new IMAGE_BRUSH("Icons/CompileStatus_Fail", Icon40x40));
	Style->Set("Niagara.CompileStatus.Good",    new IMAGE_BRUSH("Icons/CompileStatus_Good", Icon40x40));
	Style->Set("Niagara.CompileStatus.Warning", new IMAGE_BRUSH("Icons/CompileStatus_Warning", Icon40x40));
	Style->Set("Niagara.Asset.ReimportAsset.Needed", new IMAGE_BRUSH("Icons/icon_Reimport_Needed_40x", Icon40x40));
	Style->Set("Niagara.Asset.ReimportAsset.Default", new IMAGE_BRUSH("Icons/icon_Reimport_40x", Icon40x40));
	
	Style->Set("NiagaraEditor.MaterialWarningBorder", new BOX_CORE_BRUSH("Common/GroupBorderLight", FMargin(4.0f / 16.0f)));

	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT
#undef TTF_CORE_FONT
#undef BOX_CORE_BRUSH

void FNiagaraEditorStyle::ReloadTextures()
{
	FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
}

const ISlateStyle& FNiagaraEditorStyle::Get()
{
	return *NiagaraEditorStyleInstance;
}