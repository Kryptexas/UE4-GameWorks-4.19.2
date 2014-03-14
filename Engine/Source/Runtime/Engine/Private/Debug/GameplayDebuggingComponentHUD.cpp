// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	HUD.cpp: Heads up Display related functionality
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineUserInterfaceClasses.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogHUD, Log, All);


AGameplayDebuggingComponentHUD::AGameplayDebuggingComponentHUD(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, bDrawToScreen(true)
{
}

FString AGameplayDebuggingComponentHUD::GenerateAllData()
{
	bDrawToScreen = false;
	HugeOutputString.Empty();
	PrintAllData();
	bDrawToScreen = true;

	FString ReturnString = HugeOutputString;
	HugeOutputString.Empty();
	return ReturnString;
}

void AGameplayDebuggingComponentHUD::PostRender()
{
	Super::PostRender();
	PrintAllData();
}

void AGameplayDebuggingComponentHUD::PrintAllData()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	DefaultContext = FPrintContext(GEngine->GetSmallFont(), Canvas, 20, 20);
	DefaultContext.FontRenderInfo.bEnableShadow = true;

	if (DefaultContext.Canvas != NULL)
	{
		float XL, YL;
		CalulateStringSize(DefaultContext, DefaultContext.Font, TEXT("AI Debug ENABLED"), XL, YL);
		PrintString(DefaultContext, FColorList::White, TEXT("AI Debug ENABLED"), DefaultContext.Canvas->ClipX/2.0f - XL/2.0f, 0);
	}

	APlayerController* const MyPC = Cast<APlayerController>(PlayerOwner);
	if (MyPC)
	{
		UWorld* World = MyPC->GetWorld();
		for (FConstPawnIterator Iterator = World->GetPawnIterator(); Iterator; ++Iterator )
		{
			APawn* NewTarget = *Iterator;
			UGameplayDebuggingComponent* DebuggingComponent = NewTarget->GetDebugComponent(false);
			if ( DebuggingComponent == NULL || NewTarget == MyPC->GetPawn() || (NewTarget->PlayerState && !NewTarget->PlayerState->bIsABot))
				continue;

			DrawDebugComponentData(MyPC, DebuggingComponent);
		}

		if (APawn* MyPawn = MyPC->GetPawn())
		{
			if (UGameplayDebuggingComponent *MyComp = MyPawn->GetDebugComponent(false))
			{
				if (MyComp->IsViewActive(EAIDebugDrawDataView::NavMesh) && bDrawToScreen)
				{
					DrawNavMeshSnapshot(MyPC, MyComp);
				}
			}
		}
	}
#endif
}

void AGameplayDebuggingComponentHUD::DrawDebugComponentData(APlayerController* MyPC, class UGameplayDebuggingComponent *DebugComponent)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	APawn* Pawn = Cast<APawn>(DebugComponent->GetOwner());
	const FVector ScreenLoc = ProjectLocation(DefaultContext, Pawn->GetActorLocation() + FVector(0.f, 0.f, Pawn->GetSimpleCollisionHalfHeight()));

	OverHeadContext = FPrintContext(GEngine->GetSmallFont(), Canvas, ScreenLoc.X, ScreenLoc.Y);

	if (bDrawToScreen && (DebugComponent->IsViewActive(EAIDebugDrawDataView::OverHead) || DebugComponent->IsViewActive(EAIDebugDrawDataView::EditorDebugAIFlag)))
		DrawOverHeadInformation(MyPC, DebugComponent);

	if (bDrawToScreen && !DebugComponent->IsViewActive(EAIDebugDrawDataView::EditorDebugAIFlag) && DefaultContext.Canvas != NULL)
	{
		float XL = 0.f, YL = 0.f;
		const FString Keys = DebugComponent->GetKeyboardDesc();
		UFont* OldFont = DefaultContext.Font;
		DefaultContext.Font = GEngine->GetMediumFont();
		CalulateStringSize(DefaultContext, DefaultContext.Font, Keys, XL, YL);
		FCanvasTileItem TileItem( FVector2D(10, 10), GWhiteTexture, FVector2D( XL+20, YL+20 ), FColor(0,0,0,20) );
		TileItem.BlendMode = SE_BLEND_Translucent;
		DrawItem(DefaultContext, TileItem, 0, 0);
		PrintString(DefaultContext, FColor::White, Keys, 20, 20);
		DefaultContext.Font = OldFont;
	}

	if (DebugComponent->bIsSelectedForDebugging && DebugComponent->ShowExtendedInformatiomCounter > 0)
	{
		if (!bDrawToScreen || DebugComponent->IsViewActive(EAIDebugDrawDataView::Basic))
		{
			DrawBasicData(MyPC, DebugComponent);
		}

		if (!bDrawToScreen || DebugComponent->IsViewActive(EAIDebugDrawDataView::BehaviorTree))
		{
			DrawBehaviorTreeData(MyPC, DebugComponent);
		}

		if (!bDrawToScreen || DebugComponent->IsViewActive(EAIDebugDrawDataView::EQS))
		{
			DrawEQSData(MyPC, DebugComponent);
		}

		if (!bDrawToScreen || DebugComponent->IsViewActive(EAIDebugDrawDataView::Perception))
		{
			DrawPerception(MyPC, DebugComponent);
		}

		DrawGameSpecyficView(MyPC, DebugComponent);
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
}

void AGameplayDebuggingComponentHUD::DrawPath(APlayerController* MyPC, class UGameplayDebuggingComponent *DebugComponent)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	static const FColor Grey(100,100,100);
	const int32 NumPathVerts = DebugComponent->PathPoints.Num();
	if (MyPC)
	{
		UWorld* World = MyPC->GetWorld();

		for (int32 VertIdx=0; VertIdx < NumPathVerts-1; ++VertIdx)
		{
			FVector const VertLoc = DebugComponent->PathPoints[VertIdx] + NavigationDebugDrawing::PathOffeset;
			DrawDebugSolidBox(World, VertLoc, NavigationDebugDrawing::PathNodeBoxExtent, Grey, false);

			// draw line to next loc
			FVector const NextVertLoc = DebugComponent->PathPoints[VertIdx+1] + NavigationDebugDrawing::PathOffeset;
			DrawDebugLine(World, VertLoc, NextVertLoc, Grey, false
				, -1.f, 0
				, NavigationDebugDrawing::PathLineThickness);
		}

		// draw last vert
		if (NumPathVerts > 0)
		{
			DrawDebugBox(World, DebugComponent->PathPoints[NumPathVerts-1] + NavigationDebugDrawing::PathOffeset, FVector(15.f), Grey, false);
		}
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
}

void AGameplayDebuggingComponentHUD::DrawOverHeadInformation(APlayerController* PC, class UGameplayDebuggingComponent *DebugComponent)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	APawn* MyPawn = Cast<APawn>(DebugComponent->GetOwner());
	const FVector ScreenLoc = OverHeadContext.Canvas->Project(MyPawn->GetActorLocation() + FVector(0.f, 0.f, MyPawn->GetSimpleCollisionHalfHeight()));
	static const FVector2D FontScale(1.f, 1.f);
	UFont* Font = GEngine->GetSmallFont();

	float TextXL = 0.f;
	float YL = 0.f;
	CalulateStringSize(OverHeadContext, OverHeadContext.Font, DebugComponent->PawnName, TextXL, YL);

	bool bDrawFullOverHead = DebugComponent->bIsSelectedForDebugging;
	if (bDrawFullOverHead)
	{
		OverHeadContext.DefaultX -= (0.5f*TextXL*FontScale.X);
		OverHeadContext.DefaultY -= (1.2f*YL*FontScale.Y);
		OverHeadContext.CursorX = OverHeadContext.DefaultX;
		OverHeadContext.CursorY = OverHeadContext.DefaultY;
	}

	if (DebugComponent->DebugIcon.Len() > 0 )
	{
		UTexture2D* RegularIcon = (UTexture2D*)StaticLoadObject(UTexture2D::StaticClass(), NULL, *DebugComponent->DebugIcon, NULL, LOAD_None, NULL);
		if (RegularIcon)
		{
			FCanvasIcon Icon = UCanvas::MakeIcon(RegularIcon);
			if (Icon.Texture)
			{
				const float DesiredIconSize = bDrawFullOverHead ? 32.f : 16.f;
				DrawIcon(OverHeadContext, FColor::White, Icon, OverHeadContext.CursorX - DesiredIconSize, OverHeadContext.CursorY, DesiredIconSize / Icon.Texture->GetSurfaceWidth());
			}
		}
	}
	if (bDrawFullOverHead)
	{
		OverHeadContext.FontRenderInfo.bEnableShadow = bDrawFullOverHead;
		PrintString(OverHeadContext, bDrawFullOverHead ? FColor::White : FColor(255,255,255,128), FString::Printf(TEXT("%s\n"), *DebugComponent->PawnName));
		OverHeadContext.FontRenderInfo.bEnableShadow = false;
	}

	if (DebugComponent->ActiveViews & (1 << EAIDebugDrawDataView::EditorDebugAIFlag))
	{
		PrintString(OverHeadContext, FString::Printf(TEXT("{red}%s\n"), *DebugComponent->PathErrorString));
		DrawPath(PC, DebugComponent);
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
}

void AGameplayDebuggingComponentHUD::DrawBasicData(APlayerController* PC, class UGameplayDebuggingComponent *DebugComponent)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	PrintString(DefaultContext, TEXT("\n{R=0,G=255,B=0,A=255}BASIC DATA\n"));
	PrintString(DefaultContext, FString::Printf(TEXT("Pawn Name: {yellow}%s, {white}Pawn Class: {yellow}%s\n"), *DebugComponent->PawnName, *DebugComponent->PawnClass));
	PrintString(DefaultContext, FString::Printf(TEXT("Controller Name: {yellow}%s\n"), *DebugComponent->ControllerName));
	if (DebugComponent->PathErrorString.Len() > 0)
	{
		PrintString(DefaultContext, FString::Printf(TEXT("{red}%s\n"), *DebugComponent->PathErrorString));
	}

	if (DebugComponent->bIsSelectedForDebugging)
	{
		DrawPath(PC, DebugComponent);
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
}

void AGameplayDebuggingComponentHUD::DrawBehaviorTreeData(APlayerController* PC, class UGameplayDebuggingComponent *DebugComponent)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	PrintString(DefaultContext, TEXT("\n{green}BEHAVIOR TREE\n"));
	PrintString(DefaultContext, FString::Printf(TEXT("Brain Component: {yellow}%s\n"), *DebugComponent->BrainComponentName));
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
}

void AGameplayDebuggingComponentHUD::DrawEQSData(APlayerController* PC, class UGameplayDebuggingComponent *DebugComponent)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	PrintString(DefaultContext, TEXT("\n{green}EQS\n"));
	if (DebugComponent->EQSQueryInstance.IsValid())
	{
		PrintString(DefaultContext, FString::Printf(TEXT("{yellow}Timestamp: {white}%.3f (%.2fs ago)\n")
			, DebugComponent->EQSTimestamp, PC->GetWorld()->GetTimeSeconds() - DebugComponent->EQSTimestamp
			));
		PrintString(DefaultContext, FString::Printf(TEXT("{yellow}Query Name: {white}%s\n")
			, *DebugComponent->EQSQueryInstance->QueryName
			));
		PrintString(DefaultContext, FString::Printf(TEXT("{yellow}Query ID: {white}%d\n")
			, DebugComponent->EQSQueryInstance->QueryID
			));
	}
	
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
}

void AGameplayDebuggingComponentHUD::DrawPerception(APlayerController* PC, class UGameplayDebuggingComponent *DebugComponent)
{

}

void AGameplayDebuggingComponentHUD::DrawNavMeshSnapshot(APlayerController* PC, class UGameplayDebuggingComponent *DebugComponent)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	PrintString(DefaultContext, TEXT("\n{green}NavMesh snapshot\n"));
	if (DebugComponent != NULL && DebugComponent->AllNavMeshAreas[0].IndexBuffer.Num() > 0)
	{
		PrintString(DefaultContext, TEXT("\n{green}GOT {red}NAVMESH {blue}DATA\n"));
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
}

//////////////////////////////////////////////////////////////////////////
void AGameplayDebuggingComponentHUD::PrintString(FPrintContext& Context, const FString& InString )
{
	class FParserHelper
	{
		enum TokenType
		{
			OpenTag,
			CloseTag,
			NewLine,
			EndOfString,
			RegularChar,
		};

		enum Tag
		{
			DefinedColor,
			OtherColor,
			ErrorTag,
		};

		uint8 ReadToken()
		{
			uint8 OutToken = RegularChar;
			TCHAR ch = Index < DataString.Len() ? DataString[Index] : '\0';
			switch(ch)
			{
			case '\0':
				OutToken = EndOfString;
				break;
			case '{':
				OutToken = OpenTag;
				Index++;
				break;
			case '}':
				OutToken = CloseTag;
				Index++;
				break;
			case '\n':
				OutToken = NewLine;
				Index++;
				break;
			default:
				OutToken = RegularChar;
				break;
			}
			return OutToken;
		}

		uint32 ParseTag( FString& OutData )
		{
			FString TagString;
			int32 OryginalIndex = Index;
			uint8 token = ReadToken();
			while (token != EndOfString && token != CloseTag)
			{
				if (token == RegularChar)
				{
					TagString.AppendChar(DataString[Index++]);
				}
				token = ReadToken();
			}

			int OutTag = ErrorTag;

			if (token != CloseTag)
			{
				Index = OryginalIndex;
				OutData = FString::Printf( TEXT("{%s"), *TagString);
				OutData.AppendChar(DataString[Index-1]);
				return OutTag;
			}

			if (GColorList.IsValidColorName(*TagString.ToLower()))
			{
				OutTag = DefinedColor;
				OutData = TagString;
			}
			else
			{
				FColor Color;
				if (Color.InitFromString(TagString))
				{
					OutTag = OtherColor;
					OutData = TagString;
				}
				else
				{
					OutTag = ErrorTag;
					OutData = FString::Printf( TEXT("{%s"), *TagString);
					OutData.AppendChar(DataString[Index-1]);
				}
			}
			//Index++;
			return OutTag;
		}

		struct StringNode
		{
			FString String;
			FColor Color;
			bool bNewLine;
			StringNode() : Color(FColor::White), bNewLine(false) {}
		};
		int32 Index;
		FString DataString;
	public:
		TArray<StringNode> Strings;

		void ParseString(const FString& StringToParse)
		{
			Index = 0;
			DataString = StringToParse;
			Strings.Add(StringNode());
			if (Index >= DataString.Len())
				return;

			uint8 Token = ReadToken();
			while (Token != EndOfString)
			{
				switch (Token)
				{
				case RegularChar:
					Strings[Strings.Num()-1].String.AppendChar( DataString[Index++] );
					break;
				case NewLine:
					Strings.Add(StringNode());
					Strings[Strings.Num()-1].bNewLine = true;
					Strings[Strings.Num()-1].Color = Strings[Strings.Num()-2].Color;
					break;
				case EndOfString:
					break;
				case OpenTag:
					{
						FString OutData;
						switch (ParseTag(OutData))
						{
						case DefinedColor:
							{
								int32 i = Strings.Add(StringNode());
								Strings[i].Color = GColorList.GetFColorByName(*OutData.ToLower());
							}
							break;
						case OtherColor:
							{
								FColor NewColor; 
								if (NewColor.InitFromString( OutData ))
								{
									int32 i = Strings.Add(StringNode());
									Strings[i].Color = NewColor;
									break;
								}
							}
						default:
							Strings[Strings.Num()-1].String += OutData;
							break;
						}
					}
					break;
				}
				Token = ReadToken();
			}
		}
	};

	FParserHelper Helper;
	Helper.ParseString( InString );

	float YMovement = 0;
	float XL = 0.f, YL = 0.f;
	CalulateStringSize(Context, Context.Font, TEXT("X"), XL, YL);

	for (int32 Index=0; Index < Helper.Strings.Num(); ++Index)
	{
		if (Index > 0 && Helper.Strings[Index].bNewLine)
		{
			if (Context.Canvas != NULL && YMovement + Context.CursorY > Context.Canvas->ClipY)
			{
				Context.DefaultX += Context.Canvas->ClipX / 2;
				Context.CursorX = Context.DefaultX;
				Context.CursorY = Context.DefaultY;
				YMovement = 0;
			}
			YMovement += YL;
			Context.CursorX = Context.DefaultX;
			if (!bDrawToScreen)
				HugeOutputString += TEXT("\n");
		}
		const FString Str = Helper.Strings[Index].String;
		if (!bDrawToScreen)
		{
			HugeOutputString += Str;
		}

		if (Str.Len() > 0 && Context.Canvas != NULL)
		{
			float SizeX, SizeY;
			CalulateStringSize(Context, Context.Font, Str, SizeX, SizeY);

			FCanvasTextItem TextItem( FVector2D::ZeroVector, FText::FromString(Str), Context.Font, FLinearColor::White );
			if (Context.FontRenderInfo.bEnableShadow)
			{
				TextItem.EnableShadow( FColor::Black, FVector2D( 1, 1 ) );
			}

			TextItem.SetColor(Helper.Strings[Index].Color);
			DrawItem( Context, TextItem, Context.CursorX, YMovement + Context.CursorY );
			Context.CursorX += SizeX;
		}
	}

	Context.CursorY += YMovement;
}

void AGameplayDebuggingComponentHUD::PrintString(FPrintContext& Context, const FColor& InColor, const FString& InString )
{
	PrintString( Context, FString::Printf(TEXT("{%s}%s"), *InColor.ToString(), *InString));
}

void AGameplayDebuggingComponentHUD::PrintString(FPrintContext& Context, const FColor& InColor, const FString& InString, float X, float Y )
{
	const float OldX = Context.CursorX, OldY = Context.CursorY;
	const float OldDX = Context.DefaultX, OldDY = Context.DefaultY;

	Context.DefaultX = Context.CursorX = X;
	Context.DefaultY = Context.CursorY = Y;
	PrintString(Context, InColor, InString);

	Context.CursorX = OldX;
	Context.CursorY = OldY;
	Context.DefaultX = OldDX;
	Context.DefaultY = OldDY;
}


void AGameplayDebuggingComponentHUD::CalulateStringSize(const AGameplayDebuggingComponentHUD::FPrintContext& DefaultContext, UFont* Font, const FString& InString, float& OutX, float& OutY )
{
	OutX = OutY = 0;
	if (DefaultContext.Canvas != NULL)
	{
		DefaultContext.Canvas->StrLen(Font != NULL ? Font : DefaultContext.Font, InString, OutX, OutY);
	}
}

FVector AGameplayDebuggingComponentHUD::ProjectLocation(const AGameplayDebuggingComponentHUD::FPrintContext& Context, const FVector& Location)
{
	if (Context.Canvas != NULL)
	{
		return Context.Canvas->Project(Location);
	}

	return FVector();
}

void AGameplayDebuggingComponentHUD::DrawItem(const AGameplayDebuggingComponentHUD::FPrintContext& DefaultContext, class FCanvasItem& Item, float X, float Y )
{
	if(DefaultContext.Canvas)
	{
		DefaultContext.Canvas->DrawItem( Item, FVector2D( X, Y ) );
	}
}

void AGameplayDebuggingComponentHUD::DrawIcon(const AGameplayDebuggingComponentHUD::FPrintContext& DefaultContext, const FColor& InColor, const FCanvasIcon& Icon, float X, float Y, float Scale)
{
	if(DefaultContext.Canvas)
	{
		DefaultContext.Canvas->SetDrawColor(InColor);
		DefaultContext.Canvas->DrawIcon(Icon, X, Y, Scale);
	}
}

