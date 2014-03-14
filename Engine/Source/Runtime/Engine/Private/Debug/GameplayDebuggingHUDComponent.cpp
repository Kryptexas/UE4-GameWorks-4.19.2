// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	HUD.cpp: Heads up Display related functionality
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineUserInterfaceClasses.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogHUD, Log, All);


AGameplayDebuggingHUDComponent::AGameplayDebuggingHUDComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, bDrawToScreen(true)
{
}

FString AGameplayDebuggingHUDComponent::GenerateAllData()
{
	bDrawToScreen = false;
	HugeOutputString.Empty();
	PrintAllData();
	bDrawToScreen = true;

	FString ReturnString = HugeOutputString;
	HugeOutputString.Empty();
	return ReturnString;
}

void AGameplayDebuggingHUDComponent::PostRender()
{
	Super::PostRender();
	PrintAllData();
}

void AGameplayDebuggingHUDComponent::PrintAllData()
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
			if (DebuggingComponent)
			{
				if (NewTarget == MyPC->GetPawn())
				{
					if (DebuggingComponent->IsViewActive(EAIDebugDrawDataView::NavMesh) && bDrawToScreen)
					{
						DrawNavMeshSnapshot(MyPC, DebuggingComponent);
					}
				}
				else if (NewTarget->PlayerState == NULL || NewTarget->PlayerState->bIsABot)
				{
					DrawDebugComponentData(MyPC, DebuggingComponent);
				}
			}
		}
	}
#endif
}

void AGameplayDebuggingHUDComponent::DrawDebugComponentData(APlayerController* MyPC, class UGameplayDebuggingComponent *DebugComponent)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	APawn* Pawn = Cast<APawn>(DebugComponent->GetOwner());
	const FVector ScreenLoc = ProjectLocation(DefaultContext, Pawn->GetActorLocation() + FVector(0.f, 0.f, Pawn->GetSimpleCollisionHalfHeight()));

	OverHeadContext = FPrintContext(GEngine->GetSmallFont(), Canvas, ScreenLoc.X, ScreenLoc.Y);

	if (bDrawToScreen && (DebugComponent->IsViewActive(EAIDebugDrawDataView::OverHead) || DebugComponent->IsViewActive(EAIDebugDrawDataView::EditorDebugAIFlag)))
		DrawOverHeadInformation(MyPC, DebugComponent);

	if (DebugComponent->bIsSelectedForDebugging && DebugComponent->ShowExtendedInformatiomCounter > 0)
	{
		if (bDrawToScreen && !DebugComponent->IsViewActive(EAIDebugDrawDataView::EditorDebugAIFlag) && DefaultContext.Canvas != NULL)
		{
			TArray<FDebugCategoryView> Categories;
			DebugComponent->GetKeyboardDesc(Categories);

			UFont* OldFont = DefaultContext.Font;
			DefaultContext.Font = GEngine->GetMediumFont();

			TArray<float> CategoriesWidth;
			CategoriesWidth.AddZeroed(Categories.Num());
			float TotalWidth = 0.0f, MaxHeight = 0.0f;

			FString HeaderDesc(TEXT("Tap [\"] to close, use Numpad to toggle categories"));
			float HeaderWidth = 0.0f;
			CalulateStringSize(DefaultContext, DefaultContext.Font, HeaderDesc, HeaderWidth, MaxHeight);

			for (int32 i = 0; i < Categories.Num(); i++)
			{
				Categories[i].Desc = FString::Printf(TEXT("%d:%s "), i, *Categories[i].Desc);

				float StrHeight = 0.0f;
				CalulateStringSize(DefaultContext, DefaultContext.Font, Categories[i].Desc, CategoriesWidth[i], StrHeight);
				TotalWidth += CategoriesWidth[i];
				MaxHeight = FMath::Max(MaxHeight, StrHeight);
			}

			TotalWidth = FMath::Max(TotalWidth, HeaderWidth);

			FCanvasTileItem TileItem( FVector2D(10, 10), GWhiteTexture, FVector2D( TotalWidth+20, MaxHeight+20 ), FColor(0,0,0,20) );
			TileItem.BlendMode = SE_BLEND_Translucent;
			DrawItem(DefaultContext, TileItem, 0, 0);

			PrintString(DefaultContext, FColorList::LightBlue, HeaderDesc, 2, 2);

			APawn* PCPawn = MyPC->GetPawn();
			UGameplayDebuggingComponent* PlayerDebugComp = PCPawn ? PCPawn->GetDebugComponent(false) : NULL;

			if (PlayerDebugComp)
			{
				float XPos = 20.0f;
				for (int32 i = 0; i < Categories.Num(); i++)
				{
					const bool bIsActive = (Categories[i].View == EAIDebugDrawDataView::NavMesh) ?
						PlayerDebugComp->IsViewActive(Categories[i].View) :
						DebugComponent->IsViewActive(Categories[i].View);

					PrintString(DefaultContext, bIsActive ? FColorList::Green : FColor::White, Categories[i].Desc, XPos, 20);
					XPos += CategoriesWidth[i];
				}
			}

			DefaultContext.Font = OldFont;
		}

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

void AGameplayDebuggingHUDComponent::DrawPath(APlayerController* MyPC, class UGameplayDebuggingComponent *DebugComponent)
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

void AGameplayDebuggingHUDComponent::DrawOverHeadInformation(APlayerController* PC, class UGameplayDebuggingComponent *DebugComponent)
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

void AGameplayDebuggingHUDComponent::DrawBasicData(APlayerController* PC, class UGameplayDebuggingComponent *DebugComponent)
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

void AGameplayDebuggingHUDComponent::DrawBehaviorTreeData(APlayerController* PC, class UGameplayDebuggingComponent *DebugComponent)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	PrintString(DefaultContext, TEXT("\n{green}BEHAVIOR TREE\n"));
	PrintString(DefaultContext, FString::Printf(TEXT("Brain Component: {yellow}%s\n"), *DebugComponent->BrainComponentName));
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
}

void AGameplayDebuggingHUDComponent::DrawEQSData(APlayerController* PC, class UGameplayDebuggingComponent *DebugComponent)
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

void AGameplayDebuggingHUDComponent::DrawPerception(APlayerController* PC, class UGameplayDebuggingComponent *DebugComponent)
{
}

void AGameplayDebuggingHUDComponent::DrawNavMeshSnapshot(APlayerController* PC, class UGameplayDebuggingComponent *DebugComponent)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (DebugComponent && DebugComponent->NavmeshRepData.Num())
	{
		float TimeLeft = 0.0f;
		if (PC && PC->GetDebuggingController())
		{
			TimeLeft = GetWorldTimerManager().GetTimerRemaining(PC->GetDebuggingController(), &UGameplayDebuggingControllerComponent::UpdateNavMeshTimer);
		}

		FString NextUpdateDesc;
		if (TimeLeft > 0.0f)
		{
			NextUpdateDesc = FString::Printf(TEXT(", next update: {yellow}%.1fs"), TimeLeft);
		}
	
		PrintString(DefaultContext, FString::Printf(TEXT("\n\n{green}Showing NavMesh (%.1fkB)%s\n"),
			DebugComponent->NavmeshRepData.Num() / 1024.0f, *NextUpdateDesc));
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
}

//////////////////////////////////////////////////////////////////////////
void AGameplayDebuggingHUDComponent::PrintString(FPrintContext& Context, const FString& InString )
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

void AGameplayDebuggingHUDComponent::PrintString(FPrintContext& Context, const FColor& InColor, const FString& InString )
{
	PrintString( Context, FString::Printf(TEXT("{%s}%s"), *InColor.ToString(), *InString));
}

void AGameplayDebuggingHUDComponent::PrintString(FPrintContext& Context, const FColor& InColor, const FString& InString, float X, float Y )
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


void AGameplayDebuggingHUDComponent::CalulateStringSize(const AGameplayDebuggingHUDComponent::FPrintContext& DefaultContext, UFont* Font, const FString& InString, float& OutX, float& OutY )
{
	OutX = OutY = 0;
	if (DefaultContext.Canvas != NULL)
	{
		DefaultContext.Canvas->StrLen(Font != NULL ? Font : DefaultContext.Font, InString, OutX, OutY);
	}
}

FVector AGameplayDebuggingHUDComponent::ProjectLocation(const AGameplayDebuggingHUDComponent::FPrintContext& Context, const FVector& Location)
{
	if (Context.Canvas != NULL)
	{
		return Context.Canvas->Project(Location);
	}

	return FVector();
}

void AGameplayDebuggingHUDComponent::DrawItem(const AGameplayDebuggingHUDComponent::FPrintContext& DefaultContext, class FCanvasItem& Item, float X, float Y )
{
	if(DefaultContext.Canvas)
	{
		DefaultContext.Canvas->DrawItem( Item, FVector2D( X, Y ) );
	}
}

void AGameplayDebuggingHUDComponent::DrawIcon(const AGameplayDebuggingHUDComponent::FPrintContext& DefaultContext, const FColor& InColor, const FCanvasIcon& Icon, float X, float Y, float Scale)
{
	if(DefaultContext.Canvas)
	{
		DefaultContext.Canvas->SetDrawColor(InColor);
		DefaultContext.Canvas->DrawIcon(Icon, X, Y, Scale);
	}
}

