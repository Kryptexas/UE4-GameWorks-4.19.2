// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 *
 * This file contains the rendering functions used in the stats code
 */

#include "EnginePrivate.h"

#if STATS

#include "StatsData.h"


#define STAT_FONT			GEngine->GetSmallFont()
#define FONT_HEIGHT			12
// The default small font (which the stat rendering code was written around) is
// actually 16 in height... So we will use this to calculate the scale factor
// of the actual font being used to render.
#define IDEAL_FONT_HEIGHT	16

// This is the number of W characters to leave spacing for in the stat name column (32 chars)
static const FString STAT_COLUMN_WIDTH = FString::ChrN( 32, 'W' );

// This is the number of W characters to leave spacing for in the other stat columns (8 chars)
static const FString TIME_COLUMN_WIDTH = FString::ChrN( 8, 'W' );

struct FStatRenderGlobals
{
	/**
	 * Rendering offset for first column from stat label
	 */
	int32 AfterNameColumnOffset;
	/**
	 * Rendering offsets for additional columns
	 */
	int32 InterColumnOffset;
	/**
	 * Color for rendering stats
	 */
	FLinearColor StatColor;
	/**
	 * Color to use when rendering headings
	 */
	FLinearColor HeadingColor;
	/**
	 * Color to use when rendering group names
	 */
	FLinearColor GroupColor;
	/**
	 * Color used as the background for every other stat item to make it easier to read across lines 
	 */
	FLinearColor BackgroundColor;
	/** The scale to apply to the font used for rendering stats. */
	float StatFontScale;


	FStatRenderGlobals(void) :
		AfterNameColumnOffset(0),
		InterColumnOffset(0),
		StatColor(0.f,1.f,0.f),
		HeadingColor(1.f,0.2f,0.f),
		GroupColor(FLinearColor::White),
		BackgroundColor(0.05f, 0.05f, 0.05f, 0.90f),	// dark gray mostly occluding the background
		StatFontScale(0.0f)
	{
	}

	/**
	 * Initializes stat render globals.
	 *
	 * @param bForce - forces to initialize globals again, used after font has been changed
	 *
	 */
	void Initialize( bool bForce = false )
	{
		static bool bIsInitialized = false;
		if( !bIsInitialized || bForce )
		{
			// Get the size of the spaces, since we can't render the width calculation strings.
			int32 StatColumnSpaceSizeY;
			int32 TimeColumnSpaceSizeY;

			// Determine where the first column goes.
			StringSize(STAT_FONT, AfterNameColumnOffset, StatColumnSpaceSizeY, *STAT_COLUMN_WIDTH);

			// Determine the width of subsequent columns.
			StringSize(STAT_FONT, InterColumnOffset, TimeColumnSpaceSizeY, *TIME_COLUMN_WIDTH);

			bIsInitialized = true;
		}
	}
};

static FStatRenderGlobals StatGlobals;



static void RightJustify(FCanvas* Canvas, int32 X, int32 Y, TCHAR const* Text, FLinearColor const& Color)
{
	int32 StatColumnSpaceSizeX, StatColumnSpaceSizeY;
	StringSize(STAT_FONT, StatColumnSpaceSizeX, StatColumnSpaceSizeY, Text);

	Canvas->DrawShadowedString(X + StatGlobals.InterColumnOffset - StatColumnSpaceSizeX,Y,Text,STAT_FONT,Color);
}


// shorten a name for stats display
static FString ShortenName(TCHAR const* LongName)
{
	FString Result(LongName);
	const int32 Limit = 50;
	if (Result.Len() > Limit)
	{
		Result = FString(TEXT("...")) + Result.Right(Limit);
	}
	return Result;
}

/**
 *
 * @param Item the stat to render
 * @param Canvas the render interface to draw with
 * @param X the X location to start drawing at
 * @param Y the Y location to start drawing at
 * @param Indent Indentation of this cycles, used when rendering hierarchy
 * @param bStackStat If false, this is a non-stack cycle counter, don't render the call count column
 */
static int32 RenderCycle( const FComplexStatMessage& Item, class FCanvas* Canvas, int32 X, int32 Y, const int32 Indent, const bool bStackStat )
{
	FColor Color = StatGlobals.StatColor;	

	const FString CounterName = Item.GetDescription();
	check(Item.NameAndInfo.GetFlag(EStatMetaFlags::IsCycle));

	const bool bIsInitialized = Item.NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_int64;

	const int32 IndentWidth = Indent*8;

	if( bIsInitialized )
	{
		float InMs = FPlatformTime::ToMilliseconds(Item.GetValue_Duration(EComplexStatField::IncAve));
		// Color will be determined by the average value of history
		// If show inclusive and and show exclusive is on, then it will choose color based on inclusive average
		GEngine->GetStatValueColoration(CounterName, InMs, Color);

		const float MaxMeter = 33.3f; // the time of a "full bar" in ms
		const int32 MeterWidth = StatGlobals.AfterNameColumnOffset;

		int32 BarWidth = int32((InMs / MaxMeter) * MeterWidth);
		if (BarWidth > 2)
		{
			if (BarWidth > MeterWidth ) 
			{
				BarWidth = MeterWidth;
			}

			FCanvasBoxItem BoxItem( FVector2D(X + MeterWidth - BarWidth, Y + .4f * FONT_HEIGHT), FVector2D(BarWidth, 0.2f * FONT_HEIGHT) );
			BoxItem.SetColor( FLinearColor::Red );
			BoxItem.Draw( Canvas );		
		}
	}




	Canvas->DrawShadowedString(X+IndentWidth,Y,*ShortenName(*CounterName),STAT_FONT,Color);

	int32 CurrX = X + StatGlobals.AfterNameColumnOffset;
	// Now append the call count
	if( bStackStat )
	{
		if (Item.NameAndInfo.GetFlag(EStatMetaFlags::IsPackedCCAndDuration) && bIsInitialized)
		{
			RightJustify(Canvas,CurrX,Y,*FString::Printf(TEXT("%u"), Item.GetValue_CallCount(EComplexStatField::IncAve)),Color);
		}
		CurrX += StatGlobals.InterColumnOffset;
	}

	// Add the two inclusive columns if asked
	if( bIsInitialized )
	{
		RightJustify(Canvas,CurrX,Y,*FString::Printf(TEXT("%1.2f ms"),FPlatformTime::ToMilliseconds(Item.GetValue_Duration(EComplexStatField::IncAve))),Color); 
	}
	CurrX += StatGlobals.InterColumnOffset;

	if( bIsInitialized )
	{
		RightJustify(Canvas,CurrX,Y,*FString::Printf(TEXT("%1.2f ms"),FPlatformTime::ToMilliseconds(Item.GetValue_Duration(EComplexStatField::IncMax))),Color);
		
	}
	CurrX += StatGlobals.InterColumnOffset;

	if( bStackStat )
	{
		// And the exclusive if asked
		if( bIsInitialized )
		{
			RightJustify(Canvas,CurrX,Y,*FString::Printf(TEXT("%1.2f ms"),FPlatformTime::ToMilliseconds(Item.GetValue_Duration(EComplexStatField::ExcAve))),Color);
		}
		CurrX += StatGlobals.InterColumnOffset;

		if( bIsInitialized )
		{
			RightJustify(Canvas,CurrX,Y,*FString::Printf(TEXT("%1.2f ms"),FPlatformTime::ToMilliseconds(Item.GetValue_Duration(EComplexStatField::ExcMax))),Color);
		}
		CurrX += StatGlobals.InterColumnOffset;
	}
	return FONT_HEIGHT;
}

/**
 * Renders the headings for grouped rendering
 *
 * @param RI the render interface to draw with
 * @param X the X location to start rendering at
 * @param Y the Y location to start rendering at
 *
 * @return the height of headings rendered
 */
static int32 RenderGroupedHeadings(class FCanvas* Canvas,int X,int32 Y,const bool bIsHierarchy)
{
	// The heading looks like:
	// Stat [32chars]    CallCount [8chars]    IncAvg [8chars]    IncMax [8chars]    ExcAvg [8chars]    ExcMax [8chars]

	static const TCHAR* CaptionFlat = TEXT("Cycle counters (flat)");
	static const TCHAR* CaptionHier = TEXT("Cycle counters (hierarchy)");

	Canvas->DrawShadowedString(X,Y,bIsHierarchy?CaptionHier:CaptionFlat,STAT_FONT,StatGlobals.HeadingColor);

	int32 CurrX = X + StatGlobals.AfterNameColumnOffset;
	RightJustify(Canvas,CurrX,Y,TEXT("CallCount"),StatGlobals.HeadingColor);
	CurrX += StatGlobals.InterColumnOffset;

	RightJustify(Canvas,CurrX,Y,TEXT("InclusiveAvg"),StatGlobals.HeadingColor);
	CurrX += StatGlobals.InterColumnOffset;
	RightJustify(Canvas,CurrX,Y,TEXT("InclusiveMax"),StatGlobals.HeadingColor);
	CurrX += StatGlobals.InterColumnOffset;

	RightJustify(Canvas,CurrX,Y,TEXT("ExclusiveAvg"),StatGlobals.HeadingColor);
	CurrX += StatGlobals.InterColumnOffset;
	RightJustify(Canvas,CurrX,Y,TEXT("ExclusiveMax"),StatGlobals.HeadingColor);
	CurrX += StatGlobals.InterColumnOffset;

	return FONT_HEIGHT + (FONT_HEIGHT / 3);
}

/**
 * Renders the counter headings for grouped rendering
 *
 * @param RI the render interface to draw with
 * @param X the X location to start rendering at
 * @param Y the Y location to start rendering at
 *
 * @return the height of headings rendered
 */
static int32 RenderCounterHeadings(class FCanvas* Canvas,int32 X,int32 Y)
{
	// The heading looks like:
	// Stat [32chars]    Value [8chars]    Average [8chars]

	Canvas->DrawShadowedString(X,Y,TEXT("Counters"),STAT_FONT,StatGlobals.HeadingColor);

	// Determine where the first column goes
	int32 CurrX = X + StatGlobals.AfterNameColumnOffset;

	// Draw the average column label.
	RightJustify(Canvas,CurrX,Y,TEXT("Average"),StatGlobals.HeadingColor);
	CurrX += StatGlobals.InterColumnOffset;

	// Draw the max column label.
	RightJustify(Canvas,CurrX,Y,TEXT("Max"),StatGlobals.HeadingColor);
	return FONT_HEIGHT + (FONT_HEIGHT / 3);
}

/**
 * Renders the memory headings for grouped rendering
 *
 * @param RI the render interface to draw with
 * @param X the X location to start rendering at
 * @param Y the Y location to start rendering at
 *
 * @return the height of headings rendered
 */
static int32 RenderMemoryHeadings(class FCanvas* Canvas,int32 X,int32 Y)
{
	// The heading looks like:
	// Stat [32chars]    MemUsed [8chars]    PhysMem [8chars]

	// get the size of the spaces, since we can't render the width calculation strings
	int32 StatColumnSpaceSizeX, StatColumnSpaceSizeY;
	int32 TimeColumnSpaceSizeX, TimeColumnSpaceSizeY;
	StringSize(STAT_FONT, StatColumnSpaceSizeX, StatColumnSpaceSizeY, *STAT_COLUMN_WIDTH);
	StringSize(STAT_FONT, TimeColumnSpaceSizeX, TimeColumnSpaceSizeY, *TIME_COLUMN_WIDTH);

	Canvas->DrawShadowedString(X,Y,TEXT("Memory Counters"),STAT_FONT,StatGlobals.HeadingColor);

	// Determine where the first column goes
	int32 CurrX = X + StatGlobals.AfterNameColumnOffset;
	RightJustify(Canvas,CurrX,Y,TEXT("MemUsedAvg"),StatGlobals.HeadingColor);

	CurrX += StatGlobals.InterColumnOffset;
	RightJustify(Canvas,CurrX,Y,TEXT("MemUsedAvg%"),StatGlobals.HeadingColor);

	CurrX += StatGlobals.InterColumnOffset;
	RightJustify(Canvas,CurrX,Y,TEXT("MemUsedMax"),StatGlobals.HeadingColor);

	CurrX += StatGlobals.InterColumnOffset;
	RightJustify(Canvas,CurrX,Y,TEXT("MemUsedMax%"),StatGlobals.HeadingColor);

	CurrX += StatGlobals.InterColumnOffset;
	RightJustify(Canvas,CurrX,Y,TEXT("MemPool"),StatGlobals.HeadingColor);

	CurrX += StatGlobals.InterColumnOffset;
	RightJustify(Canvas,CurrX,Y,TEXT("Pool Capacity"),StatGlobals.HeadingColor);

	return FONT_HEIGHT + (FONT_HEIGHT / 3);
}

static FString GetMemoryString(double Value)
{
	if (Value > 1024.0 * 1024.0 * 1024.0)
	{
		return FString::Printf(TEXT("%.2f GB"), float(Value / (1024.0 * 1024.0 * 1024.0)));
	}
	if (Value > 1024.0 * 1024.0)
	{
		return FString::Printf(TEXT("%.2f MB"), float(Value / (1024.0 * 1024.0)));
	}
	if (Value > 1024.0)
	{
		return FString::Printf(TEXT("%.2f KB"), float(Value / (1024.0)));
	}
	return FString::Printf(TEXT("%.2f B"), float(Value));
}

static int32 RenderMemoryCounter(const FGameThreadHudData& ViewData, const FComplexStatMessage& All,class FCanvas* Canvas,int32 X,int32 Y)
{
	FPlatformMemory::EMemoryCounterRegion Region = FPlatformMemory::EMemoryCounterRegion(All.NameAndInfo.GetField<EMemoryRegion>());
	const bool bDisplayAll = All.NameAndInfo.GetFlag(EStatMetaFlags::ShouldClearEveryFrame);
	
	const float MemUsed = All.GetValue_double(EComplexStatField::IncAve);
	const float MaxMemUsed = All.GetValue_double(EComplexStatField::IncMax);

	// Draw the label
	Canvas->DrawShadowedString(X,Y,*ShortenName(*All.GetDescription()),STAT_FONT,StatGlobals.StatColor);
	int32 CurrX = X + StatGlobals.AfterNameColumnOffset;

	if( bDisplayAll )
	{
		// Now append the value of the stat
		RightJustify(Canvas,CurrX,Y,*GetMemoryString(MemUsed),StatGlobals.StatColor);
		CurrX += StatGlobals.InterColumnOffset;
		if (ViewData.PoolCapacity.Contains(Region))
		{
			RightJustify(Canvas,CurrX,Y,*FString::Printf(TEXT("%.0f%%"), float(100.0 * MemUsed / double(ViewData.PoolCapacity[Region]))),StatGlobals.StatColor);
		}
		CurrX += StatGlobals.InterColumnOffset;
	}
	else
	{
		CurrX += StatGlobals.InterColumnOffset;
	CurrX += StatGlobals.InterColumnOffset;
	}
	
	// Now append the max value of the stat
	RightJustify(Canvas,CurrX,Y,*GetMemoryString(MaxMemUsed),StatGlobals.StatColor);
	CurrX += StatGlobals.InterColumnOffset;
	if (ViewData.PoolCapacity.Contains(Region))
	{
		RightJustify(Canvas,CurrX,Y,*FString::Printf(TEXT("%.0f%%"), float(100.0 * MaxMemUsed / double(ViewData.PoolCapacity[Region]))),StatGlobals.StatColor);
	}
	CurrX += StatGlobals.InterColumnOffset;
	if (ViewData.PoolAbbreviation.Contains(Region))
	{
		RightJustify(Canvas,CurrX,Y,*ViewData.PoolAbbreviation[Region],StatGlobals.StatColor);
	}
	CurrX += StatGlobals.InterColumnOffset;
	if (ViewData.PoolCapacity.Contains(Region))
	{
		RightJustify(Canvas,CurrX,Y,*GetMemoryString(double(ViewData.PoolCapacity[Region])),StatGlobals.StatColor);
	}
	CurrX += StatGlobals.InterColumnOffset;

	return FONT_HEIGHT;
}

static int32 RenderCounter(const FGameThreadHudData& ViewData, const FComplexStatMessage& All,class FCanvas* Canvas,int32 X,int32 Y)
{
	// If this is a cycle, render it as a cycle. This is a special case for manually set cycle counters.
	const bool bIsCycle = All.NameAndInfo.GetFlag(EStatMetaFlags::IsCycle);
	if( bIsCycle )
	{
		return RenderCycle( All, Canvas, X, Y, 0, false );
	}

	const bool bDisplayAll = All.NameAndInfo.GetFlag(EStatMetaFlags::ShouldClearEveryFrame);

	// Draw the label
	Canvas->DrawShadowedString(X,Y,*ShortenName(*All.GetDescription()),STAT_FONT,StatGlobals.StatColor);
	int32 CurrX = X + StatGlobals.AfterNameColumnOffset;

	if( bDisplayAll )
	{
		// Append the average.
		if (All.NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_double)
		{
			RightJustify(Canvas,CurrX,Y,*FString::Printf(TEXT("%.2f"), All.GetValue_double(EComplexStatField::IncAve)),StatGlobals.StatColor);
		}
		else if (All.NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_int64)
		{
			RightJustify(Canvas,CurrX,Y,*FString::Printf(TEXT("%lld"), All.GetValue_int64(EComplexStatField::IncAve)),StatGlobals.StatColor);
		}
	}
	CurrX += StatGlobals.InterColumnOffset;

	// Append the maximum.
	if (All.NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_double)
	{
		RightJustify(Canvas,CurrX,Y,*FString::Printf(TEXT("%.2f"), All.GetValue_double(EComplexStatField::IncMax)),StatGlobals.StatColor);
	}
	else if (All.NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_int64)
	{
		RightJustify(Canvas,CurrX,Y,*FString::Printf(TEXT("%lld"), All.GetValue_int64(EComplexStatField::IncMax)),StatGlobals.StatColor);
	}
	return FONT_HEIGHT;
}

void RenderHierCycles( FCanvas* Canvas, int32 X, int32& Y, const FHudGroup& HudGroup )
{
	UTexture2D* BackgroundTexture = UCanvas::StaticClass()->GetDefaultObject<UCanvas>()->GradientTexture0;

	// Render all cycle counters.
	for( int32 RowIndex = 0; RowIndex < HudGroup.HierAggregate.Num(); ++RowIndex )
	{
		const FComplexStatMessage& ComplexStat = HudGroup.HierAggregate[RowIndex];
		const int32 Indent = HudGroup.Indentation[RowIndex];

		if(BackgroundTexture != nullptr)
		{
			Canvas->DrawTile( X, Y + 2, StatGlobals.AfterNameColumnOffset + StatGlobals.InterColumnOffset * 6, FONT_HEIGHT,
				0, 0, 1, 1,
				StatGlobals.BackgroundColor, BackgroundTexture->Resource, true );
		}

		Y += RenderCycle( ComplexStat, Canvas, X, Y, Indent, true );
	}
}

template< typename T >
void RenderArrayOfStats( FCanvas* Canvas, int32 X, int32& Y, const TArray<FComplexStatMessage>& Aggregates, const FGameThreadHudData& ViewData, const T& FunctionToCall )
{
	UTexture2D* BackgroundTexture = UCanvas::StaticClass()->GetDefaultObject<UCanvas>()->GradientTexture0;

	// Render all counters.
	for( int32 RowIndex = 0; RowIndex < Aggregates.Num(); ++RowIndex )
	{
		const FComplexStatMessage& ComplexStat = Aggregates[RowIndex];

		if(BackgroundTexture != nullptr)
		{
			Canvas->DrawTile( X, Y + 2, StatGlobals.AfterNameColumnOffset + StatGlobals.InterColumnOffset * 6, FONT_HEIGHT,
				0, 0, 1, 1,
				StatGlobals.BackgroundColor, BackgroundTexture->Resource, true );
		}

		Y += FunctionToCall( ViewData, ComplexStat, Canvas, X, Y );
	}
}

static int32 RenderFlatCycle( const FGameThreadHudData& ViewData, const FComplexStatMessage& Item, class FCanvas* Canvas, int32 X, int32 Y )
{
	return RenderCycle( Item, Canvas, X, Y, 0, true );
}

/**
 * Renders stats using groups
 *
 * @param ViewData data from the stats thread
 * @param RI the render interface to draw with
 * @param X the X location to start rendering at
 * @param Y the Y location to start rendering at
 */
static void RenderGroupedWithHierarchy(const FGameThreadHudData& ViewData, class FCanvas* Canvas,int32 X,int32& Y)
{
	// Grab texture for rendering text background.
	UTexture2D* BackgroundTexture = UCanvas::StaticClass()->GetDefaultObject<UCanvas>()->DefaultTexture;

	// Render all groups.
	for( int32 GroupIndex = 0; GroupIndex < ViewData.HudGroups.Num(); ++GroupIndex )
	{
		const FHudGroup& HudGroup = ViewData.HudGroups[GroupIndex];

		// Render header.
		const FString& GroupDesc = ViewData.GroupDescriptions[GroupIndex];
		const FName& GroupName = ViewData.GroupNames[GroupIndex];
		const FString GroupLongName = FString::Printf( TEXT("%s [%s]"), *GroupDesc, *GroupName.GetPlainNameString() );
		Canvas->DrawShadowedString( X, Y, *GroupLongName, STAT_FONT, StatGlobals.GroupColor );
		Y += FONT_HEIGHT;

		const bool bHasHierarchy = !!HudGroup.HierAggregate.Num();
		const bool bHasFlat = !!HudGroup.FlatAggregate.Num();

		if (bHasHierarchy || bHasFlat)
		{
		// Render grouped headings.
		Y += RenderGroupedHeadings( Canvas, X, Y, bHasHierarchy );
		}

		// Render hierarchy.
		if( bHasHierarchy )
		{
			RenderHierCycles( Canvas, X, Y, HudGroup );
			Y += FONT_HEIGHT;
		}

		// Render flat.
		if( bHasFlat )
		{
			RenderArrayOfStats(Canvas,X,Y,HudGroup.FlatAggregate, ViewData, RenderFlatCycle);
			Y += FONT_HEIGHT;
		}

		// Render memory counters.
		if( HudGroup.MemoryAggregate.Num() )
		{
			Y += RenderMemoryHeadings(Canvas,X,Y);
			RenderArrayOfStats(Canvas,X,Y,HudGroup.MemoryAggregate, ViewData, RenderMemoryCounter);
			Y += FONT_HEIGHT;
		}

		// Render remaining counters.
		if( HudGroup.CountersAggregate.Num() )
		{
			Y += RenderCounterHeadings(Canvas,X,Y);
			RenderArrayOfStats(Canvas,X,Y,HudGroup.CountersAggregate, ViewData, RenderCounter);
			Y += FONT_HEIGHT;
		}
	}
}

/**
 * Renders the stats data
 *
 * @param RI the render interface to draw with
 * @param X the X location to start rendering at
 * @param Y the Y location to start rendering at
 */
void RenderStats(class FCanvas* Canvas,int32 X,int32 Y)
{
	FGameThreadHudData* ViewData = FHUDGroupGameThreadRenderer::Get().Latest;
	if (!ViewData)
	{
		return;
	}

	if (FMath::IsNearlyZero(StatGlobals.StatFontScale))
	{
		StatGlobals.StatFontScale = 1.0f;
		int32 StatFontHeight = STAT_FONT->GetMaxCharHeight();
		float CheckFontHeight = float(IDEAL_FONT_HEIGHT);
		if (StatFontHeight > CheckFontHeight)
		{
			StatGlobals.StatFontScale = CheckFontHeight / StatFontHeight;
		}
	}
 	
	float SavedFontScale = STAT_FONT->GetFontScalingFactor();
 	STAT_FONT->SetFontScalingFactor(StatGlobals.StatFontScale);

	StatGlobals.Initialize();
	RenderGroupedWithHierarchy(*ViewData, Canvas,X,Y);

	STAT_FONT->SetFontScalingFactor(SavedFontScale);
}

#endif
