// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "../../Classes/Debug/ReporterBase.h"
#include "../../Classes/Debug/ReporterGraph.h"

UReporterGraph::UReporterGraph(const class FPostConstructInitializeProperties& PCIP) :
	Super(PCIP)
{
	AxesColor = FLinearColor::Yellow;

	AxisStyle = EGraphAxisStyle::Grid;
	DataStyle = EGraphDataStyle::Filled;
	LegendPosition = ELegendPosition::Outside;
	NumXNotches = 10;
	NumYNotches = 10;
	
	BackgroundColor = FColor(0, 0, 0, 0);
}

void UReporterGraph::SetGraphScreenSize(float MinX, float MaxX, float MinY, float MaxY)
{
	FVector2D Min(MinX, MinY);
	FVector2D Max(MaxX, MaxY);

	SetGraphScreenSize(Min, Max);
}

void UReporterGraph::SetGraphScreenSize(const FVector2D& Min, const FVector2D& Max)
{
	GraphScreenSize.Min = Min;
	GraphScreenSize.Max = Max;
}

void UReporterGraph::Draw(UCanvas* Canvas)
{
	if(!bVisible)
	{
		return;
	}

	DrawBackground(Canvas);

	DrawLegend(Canvas);

	switch(DataStyle)
	{
		case EGraphDataStyle::Lines:
		{
			// order doesn't *really* matter, as they're lines
			DrawAxes(Canvas);
			DrawData(Canvas);
		} break;

		case EGraphDataStyle::Filled:
		{
			// draw data first and overlay axes
			DrawData(Canvas);
			DrawAxes(Canvas);
		} break;
	}

	DrawThresholds(Canvas);
}

void UReporterGraph::SetAxesMinMax(float MinX, float MaxX, float MinY, float MaxY)
{
	FVector2D Min(MinX, MinY);
	FVector2D Max(MaxX, MaxY);

	SetAxesMinMax(Min, Max);
}

void UReporterGraph::SetAxesMinMax(const FVector2D& Min, const FVector2D& Max)
{
	GraphMinMaxData.Min = Min;
	GraphMinMaxData.Max = Max;
}

void UReporterGraph::SetNumGraphLines(int32 NumDataLines)
{
	CurrentData.Empty();

	FGraphLine NewDataLine;

	for(int32 i = 0; i < NumDataLines; i++)
	{
		CurrentData.Add(NewDataLine);
	}

	LegendWidth = MIN_flt;
}

FGraphLine* UReporterGraph::GetGraphLine(int32 LineIndex)
{
	if(LineIndex < 0 || LineIndex > CurrentData.Num())
	{
		return NULL;
	}

	return &CurrentData[LineIndex];
}

void UReporterGraph::SetNumThresholds(int32 NumThresholds)
{
	Thresholds.Empty();

	FGraphThreshold NewThreshold;

	for(int32 i = 0; i < NumThresholds; i++)
	{
		Thresholds.Add(NewThreshold);
	}

	LegendWidth = MIN_flt;
}

FGraphThreshold* UReporterGraph::GetThreshold(int32 ThresholdIndex)
{
	if(ThresholdIndex < 0 || ThresholdIndex > Thresholds.Num())
	{
		return NULL;
	}

	return &Thresholds[ThresholdIndex];
}

void UReporterGraph::SetBackgroundColor(FColor Color)
{
	BackgroundColor = Color;
}

void UReporterGraph::SetLegendPosition(ELegendPosition::Type Position)
{
	LegendPosition = Position;
}

void UReporterGraph::DrawBackground(UCanvas* Canvas)
{
	FVector2D Min = FVector2D(GraphScreenSize.Min.X * Canvas->SizeX, GraphScreenSize.Min.Y * Canvas->SizeY);
	FVector2D Max = FVector2D(GraphScreenSize.Max.X * Canvas->SizeX, GraphScreenSize.Max.Y * Canvas->SizeY);

	FCanvasTileItem TileItem(Min, GWhiteTexture, Max-Min, BackgroundColor);
	TileItem.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(TileItem, Min);
}

void UReporterGraph::DrawLegend(UCanvas* Canvas)
{
	FVector2D CurrentTextPos = GraphScreenSize.Max;
	CurrentTextPos.X = GraphScreenSize.Min.X;
	UFont* Font = GEngine->GetSmallFont();
	if(LegendWidth < KINDA_SMALL_NUMBER)
	{
		if (LegendPosition == ELegendPosition::Outside)
		{
			int32 DummyY, CurrentX;
			for (int32 i = 0; i < CurrentData.Num(); i++)
			{
				StringSize(Font, CurrentX, DummyY, *CurrentData[i].LineName);

				if (CurrentX > LegendWidth)
				{
					LegendWidth = CurrentX;
				}
			}
		}
		else
		{
			int32 DummyY, CurrentX;
			StringSize(Font, CurrentX, DummyY, TEXT("M"));
			LegendWidth = -CurrentX;
		}
	}
	
	for(int32 i = 0; i < CurrentData.Num(); i++)
	{
		FVector2D ScreenPos = ToScreenSpace(CurrentTextPos, Canvas);
		Canvas->Canvas->DrawShadowedString(ScreenPos.X  - LegendWidth, ScreenPos.Y, *CurrentData[i].LineName, Font, CurrentData[i].Color);

		CurrentTextPos.Y -= (GraphScreenSize.Max.Y - GraphScreenSize.Min.Y) / CurrentData.Num();
	}

	
}

void UReporterGraph::DrawAxes(UCanvas* Canvas)
{
	FVector2D Min = GraphScreenSize.Min;
	
	FVector2D XMax = GraphScreenSize.Min;
	XMax.X = GraphScreenSize.Max.X;
	
	FVector2D YMax = GraphScreenSize.Min;
	YMax.Y = GraphScreenSize.Max.Y;

	// Draw the X axis
	DrawAxis(Canvas, Min, XMax, NumXNotches, false);

	// Draw the Y axis
	DrawAxis(Canvas, Min, YMax, NumYNotches, true);
}

void UReporterGraph::DrawAxis(UCanvas* Canvas, FVector2D Start, FVector2D End, float NumNotches, bool bIsVerticalAxis)
{
	// Draw the axis line
	DrawLine(Canvas, Start, End, AxesColor);

	// Divide each axis up into the desired notches
	float NotchDelta = (Start - End).Size() / (float)NumNotches;
	FVector2D NotchDataDelta = (GraphMinMaxData.Max - GraphMinMaxData.Min) / NumNotches;
	FVector2D NotchLocation = Start;
	FVector2D NotchLength(0,0);

	FLinearColor NotchColor = AxesColor;

	// if we should just draw notches
	switch(AxisStyle)
	{
		case EGraphAxisStyle::Lines:
		{
			NumNotches = 1;
			FVector2D NotchDataDelta = (GraphMinMaxData.Max - GraphMinMaxData.Min) / NumNotches;
			NotchDelta = (Start - End).Size();
			if(bIsVerticalAxis)
			{
				NotchLength.X = -(End.Y - Start.Y) * 0.05f;
				NotchLocation.X += NotchLength.X * -0.5f;
			}
			else
			{
				NotchLength.Y = -(End.X - Start.X) * 0.05f;
				NotchLocation.Y += NotchLength.Y * -0.5f;
			}
		} break;

		case EGraphAxisStyle::Notches:
		{
			if(bIsVerticalAxis)
			{
				NotchLength.X = -(End.Y - Start.Y) * 0.05f;
				NotchLocation.X += NotchLength.X * -0.5f;
			}
			else
			{
				NotchLength.Y = -(End.X - Start.X) * 0.05f;
				NotchLocation.Y += NotchLength.Y * -0.5f;
			}
		} break;

		case EGraphAxisStyle::Grid:
		{
			NotchColor *= 0.125f;

			if(bIsVerticalAxis)
			{
				NotchLength.X = End.Y - Start.Y;
			}
			else
			{
				NotchLength.Y = End.X - Start.X;
			}
		} break;
	}
	
	UFont* Font = GEngine->GetSmallFont();
	for(int i = 0; i < NumNotches; i++)
	{
		if(bIsVerticalAxis)
		{
			NotchLocation.Y += NotchDelta;
		}
		else
		{
			NotchLocation.X += NotchDelta;
		}

		FString NotchValue = FString::Printf(TEXT("%1.0f"), (bIsVerticalAxis ? GraphMinMaxData.Min.Y + (NotchDataDelta.Y * (i + 1)) : GraphMinMaxData.Min.X + (NotchDataDelta.X * (i + 1))));
		
		int32 StringSizeX, StringSizeY;
		StringSize(Font, StringSizeX, StringSizeY, *NotchValue );

		FVector2D ScreenPos = ToScreenSpace(NotchLocation, Canvas);

		if(bIsVerticalAxis)
		{
			Canvas->Canvas->DrawShadowedString( ScreenPos.X + NotchLength.X, ScreenPos.Y - StringSizeY * 0.5f, *NotchValue , Font, AxesColor);
		}
		else
		{
			Canvas->Canvas->DrawShadowedString( ScreenPos.X - StringSizeX * 0.5f, ScreenPos.Y + (AxisStyle == EGraphAxisStyle::Grid ? 0 : -NotchLength.Y * Canvas->SizeY), *NotchValue , Font, AxesColor);
		}
		
		DrawLine(Canvas, NotchLocation, NotchLocation + NotchLength, NotchColor);
	}
}

void UReporterGraph::DrawThresholds(UCanvas* Canvas)
{
	UFont* Font = GEngine->GetSmallFont();
	for(int32 i = 0; i < Thresholds.Num(); i++)
	{
		if(Thresholds[i].Threshold < GraphMinMaxData.Max.Y)
		{
			FVector2D ThresholdStart(0, Thresholds[i].Threshold);
			ThresholdStart = DataToNormalized(ThresholdStart);

			FVector2D ThresholdEnd = ThresholdStart;
			ThresholdEnd.X = GraphScreenSize.Max.X;
		
			DrawLine(Canvas, ThresholdStart, ThresholdEnd, Thresholds[i].Color, EReporterLineStyle::Dash);
			FVector2D TextPos = ToScreenSpace(ThresholdEnd, Canvas);
			Canvas->Canvas->DrawShadowedString( TextPos.X, TextPos.Y, *Thresholds[i].ThresholdName , Font, Thresholds[i].Color);
		}
		else
		{
			break;
		}
	}
}

void UReporterGraph::DrawData(UCanvas* Canvas)
{
	FVector2D Start, End;
	
	for(int32 i = 0; i < CurrentData.Num(); i++)
	{
		for(int32 j = 1; j < CurrentData[i].Data.Num(); j++)
		{
			Start = CurrentData[i].Data[j - 1];
			End = CurrentData[i].Data[j];

			Start = DataToNormalized(Start);
			End = DataToNormalized(End);

			if(DataStyle == EGraphDataStyle::Lines)
			{
				DrawLine(Canvas, Start, End, CurrentData[i].Color);
			}
			else
			{
				FVector2D Position0, Position1, Position2;
				
				// draw the top triangle of the quad
				Position0.X = Start.X;
				Position0.Y = (GraphMinMaxData.Min.Y * (GraphScreenSize.Max.Y - GraphScreenSize.Min.Y)) + GraphScreenSize.Min.Y;
				Position1 = End;
				Position2 = Start;
				DrawTriangle(Canvas, Position0, Position1, Position2, CurrentData[i].Color);

				
				// draw the second triangle of the quad
				Position0.X = Start.X;
				Position0.Y = (GraphMinMaxData.Min.Y * (GraphScreenSize.Max.Y - GraphScreenSize.Min.Y)) + GraphScreenSize.Min.Y;
				Position1.X = End.X;
				Position1.Y = (GraphMinMaxData.Min.Y * (GraphScreenSize.Max.Y - GraphScreenSize.Min.Y)) + GraphScreenSize.Min.Y;
				Position2 = End;

				DrawTriangle(Canvas, Position0, Position1, Position2, CurrentData[i].Color);
			}
		}
	}
}

FVector2D UReporterGraph::ToScreenSpace(const FVector2D& InVector, UCanvas* Canvas)
{
	FVector2D OutVector = InVector;
	OutVector.X *= Canvas->SizeX;
	OutVector.Y = Canvas->SizeY - (OutVector.Y * Canvas->SizeY);

	return OutVector;
}

FVector2D UReporterGraph::DataToNormalized(const FVector2D& InVector)
{
	FVector2D OutVector = InVector;
	OutVector.X = FMath::Clamp((OutVector.X - GraphMinMaxData.Min.X) / (GraphMinMaxData.Max.X - GraphMinMaxData.Min.X), 0.0f, 1.0f);
	OutVector.Y = FMath::Clamp((OutVector.Y - GraphMinMaxData.Min.Y) / (GraphMinMaxData.Max.Y - GraphMinMaxData.Min.Y), 0.0f, 1.0f);

	OutVector.X = (OutVector.X * (GraphScreenSize.Max.X - GraphScreenSize.Min.X)) + GraphScreenSize.Min.X;
	OutVector.Y = (OutVector.Y * (GraphScreenSize.Max.Y - GraphScreenSize.Min.Y)) + GraphScreenSize.Min.Y;
	
	return OutVector;
}
