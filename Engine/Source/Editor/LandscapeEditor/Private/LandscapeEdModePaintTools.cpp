// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LandscapeEditorPrivatePCH.h"
#include "ObjectTools.h"
#include "LandscapeEdMode.h"
#include "ScopedTransaction.h"
#include "EngineTerrainClasses.h"
#include "EngineFoliageClasses.h"
#include "Landscape/LandscapeEdit.h"
#include "Landscape/LandscapeRender.h"
#include "Landscape/LandscapeDataAccess.h"
#include "Landscape/LandscapeSplineProxies.h"
#include "LandscapeEditorModule.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "LandscapeEdModeTools.h"


const int32 FNoiseParameter::Permutations[256] = 
{
	151,160,137,91,90,15,
	131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
	190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
	88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
	77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
	102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
	135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
	5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
	223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
	129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
	251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
	49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
	138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};



// 
// FLandscapeToolPaintBase
//


template<class TToolTarget, class TStrokeClass>
class FLandscapeToolPaintBase : public FLandscapeToolBase<TStrokeClass>
{
public:
	FLandscapeToolPaintBase(FEdModeLandscape* InEdMode)
		:	FLandscapeToolBase<TStrokeClass>(InEdMode)
	{}

	virtual bool IsValidForTarget(const FLandscapeToolTarget& Target)
	{
		return Target.TargetType == TToolTarget::TargetType;
	}
};


template<class ToolTarget>
class FLandscapeToolStrokePaintBase
{
public:
	FLandscapeToolStrokePaintBase(FEdModeLandscape* InEdMode, const FLandscapeToolTarget& InTarget)
	:	LandscapeInfo(InTarget.LandscapeInfo.Get())
	,	Cache(InTarget)
	{
	}

	virtual void Apply(FLevelEditorViewportClient* ViewportClient, FLandscapeBrush* Brush, const ULandscapeEditorObject* UISettings, const TArray<FLandscapeToolMousePosition>& MousePositions) = 0;
protected:
	typename ToolTarget::CacheClass Cache;
	class ULandscapeInfo* LandscapeInfo;
};



// 
// FLandscapeToolPaint
//
#define DATA_AT(Array, X, Y) ((Array)[(X-X1) + (Y-Y1)*(1+X2-X1)])


template<class ToolTarget>
class FLandscapeToolStrokePaint : public FLandscapeToolStrokePaintBase<ToolTarget>
{
	TMap<FIntPoint, float> TotalInfluenceMap;	// amount of time and weight the brush has spent on each vertex.
public:
	FLandscapeToolStrokePaint(FEdModeLandscape* InEdMode, const FLandscapeToolTarget& InTarget)
	:	FLandscapeToolStrokePaintBase<ToolTarget>(InEdMode, InTarget)
	{}

	virtual void Apply(FLevelEditorViewportClient* ViewportClient, FLandscapeBrush* Brush, const ULandscapeEditorObject* UISettings, const TArray<FLandscapeToolMousePosition>& MousePositions) OVERRIDE
	{
		// Get list of verts to update
		TMap<FIntPoint, float> BrushInfo;
		int32 X1, Y1, X2, Y2;
		if( !Brush->ApplyBrush(MousePositions, BrushInfo, X1, Y1, X2, Y2) || MousePositions.Num()==0 )
		{
			return;
		}
		
		// Tablet pressure
		float Pressure = ViewportClient->Viewport->IsPenActive() ? ViewportClient->Viewport->GetTabletPressure() : 1.f;

		// expand the area by one vertex in each direction to ensure normals are calculated correctly
		X1 -= 1;
		Y1 -= 1;
		X2 += 1;
		Y2 += 1;

		this->Cache.CacheData(X1,Y1,X2,Y2);

		// Invert when holding Shift
		bool bInvert = MousePositions[MousePositions.Num()-1].bShiftDown;
		//UE_LOG(LogLandscape, Log, TEXT("bInvert = %d"), bInvert);
		bool bUseClayBrush = UISettings->bUseClayBrush && ToolTarget::TargetType == ELandscapeToolTargetType::Heightmap;
		bool bUseWeightTargetValue = UISettings->bUseWeightTargetValue && ToolTarget::TargetType == ELandscapeToolTargetType::Weightmap;

		// The data we'll be writing to
		TArray<typename ToolTarget::CacheClass::DataType> Data;
		this->Cache.GetCachedData(X1,Y1,X2,Y2,Data);

		// The source data we use for editing. 
		// For heightmaps we use a cached snapshot, for weightmaps we use the live data.
		TArray<typename ToolTarget::CacheClass::DataType>* SourceDataArrayPtr = &Data;
		TArray<typename ToolTarget::CacheClass::DataType> OriginalData;

		if( ToolTarget::TargetType == ELandscapeToolTargetType::Heightmap )
		{
			// Heightmaps use the original data rather than the data edited during the stroke.
			this->Cache.GetOriginalData(X1,Y1,X2,Y2,OriginalData);
			SourceDataArrayPtr = &OriginalData;
		}
		else
		if( !bUseWeightTargetValue )
		{
			// When painting weights (and not using target value mode), we use a source value that tends more
			// to the current value as we paint over the same region multiple times.
			this->Cache.GetOriginalData(X1,Y1,X2,Y2,OriginalData);
			SourceDataArrayPtr = &OriginalData;

			for( int32 Y=Y1;Y<Y2;Y++ )
			{
				for( int32 X=X1;X<X2;X++ )
				{
					float VertexInfluence = TotalInfluenceMap.FindRef(ALandscape::MakeKey(X,Y));

					typename ToolTarget::CacheClass::DataType& CurrentValue = DATA_AT(Data,X,Y);
					typename ToolTarget::CacheClass::DataType& SourceValue = DATA_AT(OriginalData,X,Y);

					SourceValue = FMath::Lerp<typename ToolTarget::CacheClass::DataType>( SourceValue, CurrentValue, FMath::Min<float>(VertexInfluence * 0.05f, 1.f) );
				}
			}
		}

		FMatrix ToWorld = ToolTarget::ToWorldMatrix(this->LandscapeInfo);
		FMatrix FromWorld = ToolTarget::FromWorldMatrix(this->LandscapeInfo);

		// Adjust strength based on brush size and drawscale, so strength 1 = one hemisphere
		float AdjustedStrength = ToolTarget::StrengthMultiplier(this->LandscapeInfo, UISettings->BrushRadius);
		typename ToolTarget::CacheClass::DataType DestValue = ToolTarget::CacheClass::ClampValue(255.f * UISettings->WeightTargetValue);

		FPlane BrushPlane;
		TArray<FVector> Normals;

		if( bUseClayBrush )
		{
			// Calculate normals for brush verts in data space
			Normals.Empty(OriginalData.Num());
			Normals.AddZeroed(OriginalData.Num());

			for( int32 Y=Y1;Y<Y2;Y++ )
			{
				for( int32 X=X1;X<X2;X++ )
				{
					FVector Vert00 = ToWorld.TransformPosition( FVector((float)X+0.f,(float)Y+0.f,DATA_AT(OriginalData,X+0,Y+0)) );
					FVector Vert01 = ToWorld.TransformPosition( FVector((float)X+0.f,(float)Y+1.f,DATA_AT(OriginalData,X+0,Y+1)) );
					FVector Vert10 = ToWorld.TransformPosition( FVector((float)X+1.f,(float)Y+0.f,DATA_AT(OriginalData,X+1,Y+0)) );
					FVector Vert11 = ToWorld.TransformPosition( FVector((float)X+1.f,(float)Y+1.f,DATA_AT(OriginalData,X+1,Y+1)) );

					FVector FaceNormal1 = ((Vert00-Vert10) ^ (Vert10-Vert11)).SafeNormal();
					FVector FaceNormal2 = ((Vert11-Vert01) ^ (Vert01-Vert00)).SafeNormal(); 

					// contribute to the vertex normals.
					DATA_AT(Normals,X+1,Y+0) += FaceNormal1;
					DATA_AT(Normals,X+0,Y+1) += FaceNormal2;
					DATA_AT(Normals,X+0,Y+0) += FaceNormal1 + FaceNormal2;
					DATA_AT(Normals,X+1,Y+1) += FaceNormal1 + FaceNormal2;
				}
			}
			for( int32 Y=Y1;Y<=Y2;Y++ )
			{
				for( int32 X=X1;X<=X2;X++ )
				{
					DATA_AT(Normals,X,Y) = DATA_AT(Normals,X,Y).SafeNormal();
				}
			}

			// Find brush centroid location
			FVector AveragePoint(0.f,0.f,0.f);
			FVector AverageNormal(0.f,0.f,0.f);
			float TotalWeight = 0.f;
			for( auto It = BrushInfo.CreateIterator(); It; ++It )
			{
				int32 X, Y;
				ALandscape::UnpackKey(It.Key(), X, Y);
				float Weight = It.Value();

				AveragePoint += FVector( (float)X * Weight, (float)Y * Weight, (float)DATA_AT(OriginalData,FMath::Floor(X),FMath::Floor(Y)) * Weight );

				FVector SampleNormal = DATA_AT(Normals,X,Y);
				AverageNormal += SampleNormal * Weight;

				TotalWeight += Weight;
			}

			if( TotalWeight > 0.f )
			{
				AveragePoint /= TotalWeight;
				AverageNormal = AverageNormal.SafeNormal();
			}

			// Convert to world space
			FVector AverageLocation = ToWorld.TransformPosition( AveragePoint );
			FVector StrengthVector = ToWorld.TransformVector(FVector(0,0,UISettings->ToolStrength * Pressure * AdjustedStrength));

			// Brush pushes out in the normal direction
			FVector OffsetVector = AverageNormal * StrengthVector.Z;
			if( bInvert )
			{
				OffsetVector *= -1;
			}

			// World space brush plane
			BrushPlane = FPlane( AverageLocation + OffsetVector, AverageNormal );
		}

		// Apply the brush	
		for( auto It = BrushInfo.CreateIterator(); It; ++It )
		{
			int32 X, Y;
			ALandscape::UnpackKey(It.Key(), X, Y);

			// Update influence map
			float VertexInfluence = TotalInfluenceMap.FindRef(It.Key());
			TotalInfluenceMap.Add( It.Key(), VertexInfluence + It.Value() );

			float PaintAmount = It.Value() * UISettings->ToolStrength * Pressure * AdjustedStrength;
			typename ToolTarget::CacheClass::DataType& CurrentValue = DATA_AT(Data,X,Y);
			const typename ToolTarget::CacheClass::DataType& SourceValue = DATA_AT(*SourceDataArrayPtr,X,Y);

			if( bUseWeightTargetValue )
			{
				if( bInvert )
				{
					CurrentValue = FMath::Lerp( CurrentValue, DestValue, PaintAmount / AdjustedStrength );
				}
				else
				{
					CurrentValue = FMath::Lerp( CurrentValue, DestValue, PaintAmount / AdjustedStrength );
				}
			}
			else
			if( bUseClayBrush )
			{
				// Brush application starts from original world location at start of stroke
				FVector WorldLoc = ToWorld.TransformPosition(FVector(X,Y,SourceValue));

				// Calculate new location on the brush plane
				WorldLoc.Z = (BrushPlane.W - BrushPlane.X*WorldLoc.X - BrushPlane.Y*WorldLoc.Y) / BrushPlane.Z;

				// Painted amount lerps based on brush falloff.
				float PaintValue = FMath::Lerp<float>( (float)SourceValue, FromWorld.TransformPosition(WorldLoc).Z, It.Value() );

				if( bInvert )
				{
					CurrentValue = ToolTarget::CacheClass::ClampValue( FMath::Min<int32>(FMath::Round(PaintValue), CurrentValue) );
				}
				else
				{
					CurrentValue = ToolTarget::CacheClass::ClampValue( FMath::Max<int32>(FMath::Round(PaintValue), CurrentValue) );
				}
			}
			else
			{
				if( bInvert )
				{
					CurrentValue = ToolTarget::CacheClass::ClampValue( FMath::Min<int32>(SourceValue - FMath::Round(PaintAmount), CurrentValue) );
				}
				else
				{
					CurrentValue = ToolTarget::CacheClass::ClampValue( FMath::Max<int32>(SourceValue + FMath::Round(PaintAmount), CurrentValue) );
				}
			}
		}

		this->Cache.SetCachedData(X1,Y1,X2,Y2,Data, UISettings->PaintingRestriction);
		this->Cache.Flush();
	}
#undef DATA_AT
};

template<class ToolTarget>
class FLandscapeToolPaint : public FLandscapeToolPaintBase<ToolTarget, FLandscapeToolStrokePaint<ToolTarget> >
{
public:
	FLandscapeToolPaint(class FEdModeLandscape* InEdMode)
	:	FLandscapeToolPaintBase<ToolTarget, FLandscapeToolStrokePaint<ToolTarget> >(InEdMode)
	{}

	virtual const TCHAR* GetToolName() OVERRIDE { return TEXT("Paint"); }
	virtual FText GetDisplayName() OVERRIDE { return NSLOCTEXT("UnrealEd", "LandscapeMode_Paint", "Paint"); };

};

// 
// FLandscapeToolSmooth
//

template<class ToolTarget>
class FLandscapeToolStrokeSmooth : public FLandscapeToolStrokePaintBase<ToolTarget>
{
public:
	FLandscapeToolStrokeSmooth(FEdModeLandscape* InEdMode, const FLandscapeToolTarget& InTarget)
	:	FLandscapeToolStrokePaintBase<ToolTarget>(InEdMode, InTarget)
	{}

	virtual void Apply(FLevelEditorViewportClient* ViewportClient, FLandscapeBrush* Brush, const ULandscapeEditorObject* UISettings, const TArray<FLandscapeToolMousePosition>& MousePositions) OVERRIDE
	{
		if (!this->LandscapeInfo) return;

		// Get list of verts to update
		TMap<FIntPoint, float> BrushInfo;
		int32 X1, Y1, X2, Y2;
		if (!Brush->ApplyBrush(MousePositions, BrushInfo, X1, Y1, X2, Y2))
		{
			return;
		}

		// Tablet pressure
		float Pressure = ViewportClient->Viewport->IsPenActive() ? ViewportClient->Viewport->GetTabletPressure() : 1.f;

		// expand the area by one vertex in each direction to ensure normals are calculated correctly
		X1 -= 1;
		Y1 -= 1;
		X2 += 1;
		Y2 += 1;

		this->Cache.CacheData(X1,Y1,X2,Y2);

		TArray<typename ToolTarget::CacheClass::DataType> Data;
		this->Cache.GetCachedData(X1,Y1,X2,Y2,Data);

		// Apply the brush
		if (UISettings->bDetailSmooth)
		{
			LowPassFilter<typename ToolTarget::CacheClass::DataType>(X1, Y1, X2, Y2, BrushInfo, Data, UISettings->DetailScale, UISettings->ToolStrength * Pressure);
		}
		else
		{
			// Filter size is SmoothFilterKernelScale * BrushSize, But avoids overflow value or under 1
			int32 HalfFilterSize = FMath::Clamp<int32>(UISettings->SmoothFilterKernelScale * FMath::Max(Y2-Y1+1, X2-X1+1) * 0.25f, 1, 127);

			for( auto It = BrushInfo.CreateIterator(); It; ++It )
			{
				int32 X, Y;
				ALandscape::UnpackKey(It.Key(), X, Y);

				if( It.Value() > 0.f )
				{
					int32 FilterValue = 0;
					int32 FilterSamplingNumber = 0;

					for( int32 y = Y - HalfFilterSize; y <= Y + HalfFilterSize; y++ )
					{
						int32 YY = FMath::Clamp<int32>(y, Y1, Y2);
						for( int32 x = X - HalfFilterSize; x <= X + HalfFilterSize; x++ )
						{
							int32 XX = FMath::Clamp<int32>(x, X1, X2);
							if (BrushInfo.Find(FIntPoint(XX, YY)))
							{
								FilterValue += Data[(XX-X1) + (YY-Y1)*(1+X2-X1)];
								FilterSamplingNumber++;
							}
						}
					}

					FilterValue /= FilterSamplingNumber;

					int32 HeightDataIndex = (X-X1) + (Y-Y1)*(1+X2-X1);
					Data[HeightDataIndex] = FMath::Lerp( Data[HeightDataIndex], (typename ToolTarget::CacheClass::DataType)FilterValue, It.Value() * UISettings->ToolStrength * Pressure );
				}	
			}
		}

		this->Cache.SetCachedData(X1,Y1,X2,Y2,Data, UISettings->PaintingRestriction);
		this->Cache.Flush();
	}

};

template<class ToolTarget>
class FLandscapeToolSmooth : public FLandscapeToolPaintBase<ToolTarget, FLandscapeToolStrokeSmooth<ToolTarget> >
{
public:
	FLandscapeToolSmooth(class FEdModeLandscape* InEdMode)
	:	FLandscapeToolPaintBase<ToolTarget, FLandscapeToolStrokeSmooth<ToolTarget> >(InEdMode)
	{}

	virtual const TCHAR* GetToolName() OVERRIDE { return TEXT("Smooth"); }
	virtual FText GetDisplayName() OVERRIDE { return NSLOCTEXT("UnrealEd", "LandscapeMode_Smooth", "Smooth"); };

};

//
// FLandscapeToolFlatten
//
template<class ToolTarget>
class FLandscapeToolStrokeFlatten : public FLandscapeToolStrokePaintBase<ToolTarget>
{
	bool bInitializedFlattenHeight;
	FVector FlattenNormal;
	float FlattenPlaneDist;
	typename ToolTarget::CacheClass::DataType FlattenHeight;

public:
	FLandscapeToolStrokeFlatten(FEdModeLandscape* InEdMode, const FLandscapeToolTarget& InTarget)
	:	FLandscapeToolStrokePaintBase<ToolTarget>(InEdMode, InTarget)
	,	bInitializedFlattenHeight(false)
	{
		if (InEdMode->UISettings->bUseFlattenTarget && InTarget.TargetType == ELandscapeToolTargetType::Heightmap)
		{
			FTransform LocalToWorld = InTarget.LandscapeInfo->GetLandscapeProxy()->ActorToWorld();
			float Height = InEdMode->UISettings->FlattenTarget / LocalToWorld.GetScale3D().Z - LocalToWorld.GetTranslation().Z;
			FlattenHeight = LandscapeDataAccess::GetTexHeight(Height);
			bInitializedFlattenHeight = true;
		}
	}

	virtual void Apply(FLevelEditorViewportClient* ViewportClient, FLandscapeBrush* Brush, const ULandscapeEditorObject* UISettings, const TArray<FLandscapeToolMousePosition>& MousePositions) OVERRIDE
	{
		if (!this->LandscapeInfo) return;

		if( !bInitializedFlattenHeight || UISettings->bPickValuePerApply)
		{
			bInitializedFlattenHeight = false;
			float FlattenX = MousePositions[0].PositionX;
			float FlattenY = MousePositions[0].PositionY;
			int32 FlattenHeightX = FMath::Floor(FlattenX);
			int32 FlattenHeightY = FMath::Floor(FlattenY);

			this->Cache.CacheData(FlattenHeightX,FlattenHeightY,FlattenHeightX+1,FlattenHeightY+1);
			float HeightValue = this->Cache.GetValue(FlattenX, FlattenY);
			FlattenHeight = HeightValue;

			if (UISettings->bUseSlopeFlatten)
			{
				FlattenNormal = this->Cache.GetNormal(FlattenHeightX, FlattenHeightY);
				FlattenPlaneDist = -(FlattenNormal | FVector(FlattenX, FlattenY, HeightValue) );
			}

			bInitializedFlattenHeight = true;
		}


		// Get list of verts to update
		TMap<FIntPoint, float> BrushInfo;
		int32 X1, Y1, X2, Y2;
		if (!Brush->ApplyBrush(MousePositions, BrushInfo, X1, Y1, X2, Y2))
		{
			return;
		}

		// Tablet pressure
		float Pressure = ViewportClient->Viewport->IsPenActive() ? ViewportClient->Viewport->GetTabletPressure() : 1.f;

		// expand the area by one vertex in each direction to ensure normals are calculated correctly
		X1 -= 1;
		Y1 -= 1;
		X2 += 1;
		Y2 += 1;

		this->Cache.CacheData(X1,Y1,X2,Y2);

		TArray<typename ToolTarget::CacheClass::DataType> HeightData;
		this->Cache.GetCachedData(X1,Y1,X2,Y2,HeightData);

		// For Add or Sub Flatten Mode
		// Apply Ratio...
		TMap<int32, float> RatioInfo;
		int32 MaxDelta = INT_MIN;
		int32 MinDelta = INT_MAX;

		// Apply the brush
		for( auto It = BrushInfo.CreateIterator(); It; ++It )
		{
			int32 X, Y;
			ALandscape::UnpackKey(It.Key(), X, Y);

			if( It.Value() > 0.f )
			{
				int32 HeightDataIndex = (X-X1) + (Y-Y1)*(1+X2-X1);

				// Conserve stiff
				if (!UISettings->bUseSlopeFlatten)
				{
					int32 Delta = HeightData[HeightDataIndex] - FlattenHeight;
					switch(UISettings->FlattenMode)
					{
					case ELandscapeToolNoiseMode::Add:
						if (Delta < 0)
						{
							MinDelta = FMath::Min<int32>(Delta, MinDelta);
							RatioInfo.Add(HeightDataIndex, It.Value() * UISettings->ToolStrength * Pressure * Delta);
						}
						break;
					case ELandscapeToolNoiseMode::Sub:
						if (Delta > 0)
						{
							MaxDelta = FMath::Max<int32>(Delta, MaxDelta);
							RatioInfo.Add(HeightDataIndex, It.Value() * UISettings->ToolStrength * Pressure * Delta);
						}
						break;
					default:
					case ELandscapeToolNoiseMode::Both:
						HeightData[HeightDataIndex] = FMath::Lerp( HeightData[HeightDataIndex], FlattenHeight, It.Value() * UISettings->ToolStrength * Pressure );
						break;
					}
				}
				else
				{
					typename ToolTarget::CacheClass::DataType DestValue = -( FlattenNormal.X * X + FlattenNormal.Y * Y + FlattenPlaneDist ) / FlattenNormal.Z;
					//float PlaneDist = FlattenNormal | FVector(X, Y, HeightData(HeightDataIndex)) + FlattenPlaneDist;
					float PlaneDist = HeightData[HeightDataIndex] - DestValue;
					DestValue = HeightData[HeightDataIndex] - PlaneDist * It.Value() * UISettings->ToolStrength * Pressure;
					switch(UISettings->FlattenMode)
					{
					case ELandscapeToolNoiseMode::Add:
						if (PlaneDist < 0)
						{
							HeightData[HeightDataIndex] = FMath::Lerp( HeightData[HeightDataIndex], DestValue, It.Value() * UISettings->ToolStrength * Pressure );
						}
						break;
					case ELandscapeToolNoiseMode::Sub:
						if (PlaneDist > 0)
						{
							HeightData[HeightDataIndex] = FMath::Lerp( HeightData[HeightDataIndex], DestValue, It.Value() * UISettings->ToolStrength * Pressure );
						}
						break;
					default:
					case ELandscapeToolNoiseMode::Both:
						HeightData[HeightDataIndex] = FMath::Lerp( HeightData[HeightDataIndex], DestValue, It.Value() * UISettings->ToolStrength * Pressure );
						break;
					}
				}
			}
		}

		if (!UISettings->bUseSlopeFlatten)
		{
			for( TMap<int32, float>::TIterator It(RatioInfo); It; ++It )
			{
				switch(UISettings->FlattenMode)
				{
				case ELandscapeToolNoiseMode::Add:
					HeightData[It.Key()] = FMath::Lerp( HeightData[It.Key()], FlattenHeight, It.Value() / (float)MinDelta );
					break;
				case ELandscapeToolNoiseMode::Sub:
					HeightData[It.Key()] = FMath::Lerp( HeightData[It.Key()], FlattenHeight, It.Value() / (float)MaxDelta );
					break;
				default:
					break;
				}
			}
		}

		this->Cache.SetCachedData(X1,Y1,X2,Y2,HeightData, UISettings->PaintingRestriction);
		this->Cache.Flush();
	}
};

template<class ToolTarget>
class FLandscapeToolFlatten : public FLandscapeToolPaintBase<ToolTarget, FLandscapeToolStrokeFlatten<ToolTarget>>
{
protected:
	FVector LastMousePosition;
	UStaticMesh* PlaneMesh;
	UStaticMeshComponent* MeshComponent;

public:
	FLandscapeToolFlatten(class FEdModeLandscape* InEdMode)
	:	FLandscapeToolPaintBase<ToolTarget, FLandscapeToolStrokeFlatten<ToolTarget> >(InEdMode)
	,	LastMousePosition(FVector::ZeroVector)
	,	PlaneMesh(LoadObject<UStaticMesh>(NULL, TEXT("/Engine/EditorLandscapeResources/FlattenPlaneMesh.FlattenPlaneMesh")))
	,	MeshComponent(NULL)
	{
		check(PlaneMesh);
	}

	virtual const TCHAR* GetToolName() OVERRIDE { return TEXT("Flatten"); }
	virtual FText GetDisplayName() OVERRIDE { return NSLOCTEXT("UnrealEd", "LandscapeMode_Flatten", "Flatten"); };

	virtual bool MouseMove(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y) OVERRIDE
	{
		bool bResult = FLandscapeToolPaintBase<ToolTarget, FLandscapeToolStrokeFlatten<ToolTarget>>::MouseMove(ViewportClient, Viewport, x, y);

		if (ViewportClient->IsLevelEditorClient() && MeshComponent != NULL &&
			this->EdMode->UISettings->bUseFlattenTarget && this->EdMode->CurrentToolTarget.TargetType == ELandscapeToolTargetType::Heightmap)
		{
			this->EdMode->LandscapeMouseTrace((FLevelEditorViewportClient*)ViewportClient, x, y, LastMousePosition);

			const FTransform LocalToWorld = this->EdMode->CurrentToolTarget.LandscapeInfo->GetLandscapeProxy()->ActorToWorld();
			FVector Origin;
			Origin.X = FMath::Round(LastMousePosition.X);
			Origin.Y = FMath::Round(LastMousePosition.Y);
			Origin.Z = (FMath::Round((this->EdMode->UISettings->FlattenTarget / LocalToWorld.GetScale3D().Z - LocalToWorld.GetTranslation().Z) * LANDSCAPE_INV_ZSCALE) - 0.1f) * LANDSCAPE_ZSCALE;
			MeshComponent->SetRelativeLocation(Origin, false);
		}

		return bResult;
	}

	virtual void EnterTool()
	{
		FLandscapeToolPaintBase<ToolTarget, FLandscapeToolStrokeFlatten<ToolTarget>>::EnterTool();

		ALandscapeProxy* LandscapeProxy = this->EdMode->CurrentToolTarget.LandscapeInfo->GetLandscapeProxy();
		MeshComponent = ConstructObject<UStaticMeshComponent>(UStaticMeshComponent::StaticClass(), LandscapeProxy, NAME_None, RF_Transient);
		MeshComponent->StaticMesh = PlaneMesh;
		MeshComponent->AttachTo(LandscapeProxy->GetRootComponent());
		MeshComponent->RegisterComponent();
	}

	virtual void ExitTool()
	{
		MeshComponent->DetachFromParent();
		MeshComponent->DestroyComponent();
	}
};

// 
// FLandscapeToolNoise
//
template<class ToolTarget>
class FLandscapeToolStrokeNoise : public FLandscapeToolStrokePaintBase<ToolTarget>
{
public:
	FLandscapeToolStrokeNoise(FEdModeLandscape* InEdMode, const FLandscapeToolTarget& InTarget)
	:	FLandscapeToolStrokePaintBase<ToolTarget>(InEdMode, InTarget)
	{}

	virtual void Apply(FLevelEditorViewportClient* ViewportClient, FLandscapeBrush* Brush, const ULandscapeEditorObject* UISettings, const TArray<FLandscapeToolMousePosition>& MousePositions) OVERRIDE
	{
		if (!this->LandscapeInfo) return;

		// Get list of verts to update
		TMap<FIntPoint, float> BrushInfo;
		int32 X1, Y1, X2, Y2;
		if (!Brush->ApplyBrush(MousePositions, BrushInfo, X1, Y1, X2, Y2))
		{
			return;
		}

		// Tablet pressure
		float Pressure = ViewportClient->Viewport->IsPenActive() ? ViewportClient->Viewport->GetTabletPressure() : 1.f;

		// expand the area by one vertex in each direction to ensure normals are calculated correctly
		X1 -= 1;
		Y1 -= 1;
		X2 += 1;
		Y2 += 1;

		this->Cache.CacheData(X1,Y1,X2,Y2);
		TArray<typename ToolTarget::CacheClass::DataType> Data;
		this->Cache.GetCachedData(X1,Y1,X2,Y2,Data);

		float BrushSizeAdjust = 1.0f;
		if (ToolTarget::TargetType != ELandscapeToolTargetType::Weightmap && UISettings->BrushRadius < UISettings->MaximumValueRadius)
		{
			BrushSizeAdjust = UISettings->BrushRadius / UISettings->MaximumValueRadius;
		}

		bool bUseWeightTargetValue = UISettings->bUseWeightTargetValue && ToolTarget::TargetType == ELandscapeToolTargetType::Weightmap;

		// Apply the brush
		for( auto It = BrushInfo.CreateIterator(); It; ++It )
		{
			int32 X, Y;
			ALandscape::UnpackKey(It.Key(), X, Y);

			if( It.Value() > 0.f )
			{
				float OriginalValue = Data[(X-X1) + (Y-Y1)*(1+X2-X1)];
				if (bUseWeightTargetValue)
				{
					FNoiseParameter NoiseParam(0, UISettings->NoiseScale, 255.f / 2.f);
					float DestValue = ELandscapeToolNoiseMode::Conversion(ELandscapeToolNoiseMode::Add, NoiseParam.NoiseAmount, NoiseParam.Sample(X, Y)) * UISettings->WeightTargetValue;
					switch (UISettings->NoiseMode)
					{
					case ELandscapeToolNoiseMode::Add:
						if (OriginalValue >= DestValue)
						{
							continue;
						}
						break;
					case ELandscapeToolNoiseMode::Sub:
						DestValue += (1.f - UISettings->WeightTargetValue) * NoiseParam.NoiseAmount;
						if (OriginalValue <= DestValue)
						{
							continue;
						}
						break;
					}
					Data[(X-X1) + (Y-Y1)*(1+X2-X1)] = ToolTarget::CacheClass::ClampValue( FMath::Round(FMath::Lerp( OriginalValue, DestValue, It.Value() * UISettings->ToolStrength * Pressure)) );
				}
				else
				{
					float TotalStrength = It.Value() * UISettings->ToolStrength * Pressure * ToolTarget::StrengthMultiplier(this->LandscapeInfo, UISettings->BrushRadius);
					FNoiseParameter NoiseParam(0, UISettings->NoiseScale,  TotalStrength * BrushSizeAdjust);
					float PaintAmount = ELandscapeToolNoiseMode::Conversion(UISettings->NoiseMode, NoiseParam.NoiseAmount, NoiseParam.Sample(X, Y));
					Data[(X-X1) + (Y-Y1)*(1+X2-X1)] = ToolTarget::CacheClass::ClampValue(OriginalValue + PaintAmount);
				}
			}	
		}

		this->Cache.SetCachedData(X1,Y1,X2,Y2,Data, UISettings->PaintingRestriction);
		this->Cache.Flush();
	}
};

template<class ToolTarget>
class FLandscapeToolNoise : public FLandscapeToolPaintBase<ToolTarget, FLandscapeToolStrokeNoise<ToolTarget> >
{
public:
	FLandscapeToolNoise(class FEdModeLandscape* InEdMode)
	:	FLandscapeToolPaintBase<ToolTarget, FLandscapeToolStrokeNoise<ToolTarget> >(InEdMode)
	{}

	virtual const TCHAR* GetToolName() OVERRIDE { return TEXT("Noise"); }
	virtual FText GetDisplayName() OVERRIDE { return NSLOCTEXT("UnrealEd", "LandscapeMode_Noise", "Noise"); };
};


//
// Toolset initialization
//
void FEdModeLandscape::IntializeToolSet_Paint()
{
	FLandscapeToolSet* ToolSet_Sculpt = new(LandscapeToolSets) FLandscapeToolSet(TEXT("ToolSet_Sculpt"));
	ToolSet_Sculpt->AddTool(new FLandscapeToolPaint<FHeightmapToolTarget>(this));

	ToolSet_Sculpt->ValidBrushes.Add("BrushSet_Circle");
	ToolSet_Sculpt->ValidBrushes.Add("BrushSet_Alpha");
	ToolSet_Sculpt->ValidBrushes.Add("BrushSet_Pattern");
	ToolSet_Sculpt->ValidBrushes.Add("BrushSet_Component");

	FLandscapeToolSet* ToolSet_Paint = new(LandscapeToolSets) FLandscapeToolSet(TEXT("ToolSet_Paint"));
	ToolSet_Paint->AddTool(new FLandscapeToolPaint<FWeightmapToolTarget>(this));

	ToolSet_Paint->ValidBrushes.Add("BrushSet_Circle");
	ToolSet_Paint->ValidBrushes.Add("BrushSet_Alpha");
	ToolSet_Paint->ValidBrushes.Add("BrushSet_Pattern");
	ToolSet_Paint->ValidBrushes.Add("BrushSet_Component");
}

void FEdModeLandscape::IntializeToolSet_Smooth()
{
	FLandscapeToolSet* ToolSet_Smooth = new(LandscapeToolSets) FLandscapeToolSet(TEXT("ToolSet_Smooth"));
	ToolSet_Smooth->AddTool(new FLandscapeToolSmooth<FHeightmapToolTarget>(this));
	ToolSet_Smooth->AddTool(new FLandscapeToolSmooth<FWeightmapToolTarget>(this));

	ToolSet_Smooth->ValidBrushes.Add("BrushSet_Circle");
	ToolSet_Smooth->ValidBrushes.Add("BrushSet_Alpha");
	ToolSet_Smooth->ValidBrushes.Add("BrushSet_Pattern");
}

void FEdModeLandscape::IntializeToolSet_Flatten()
{
	FLandscapeToolSet* ToolSet_Flatten = new(LandscapeToolSets) FLandscapeToolSet(TEXT("ToolSet_Flatten"));
	ToolSet_Flatten->AddTool(new FLandscapeToolFlatten<FHeightmapToolTarget>(this));
	ToolSet_Flatten->AddTool(new FLandscapeToolFlatten<FWeightmapToolTarget>(this));

	ToolSet_Flatten->ValidBrushes.Add("BrushSet_Circle");
	ToolSet_Flatten->ValidBrushes.Add("BrushSet_Alpha");
	ToolSet_Flatten->ValidBrushes.Add("BrushSet_Pattern");
}

void FEdModeLandscape::IntializeToolSet_Noise()
{
	FLandscapeToolSet* ToolSet_Noise = new(LandscapeToolSets) FLandscapeToolSet(TEXT("ToolSet_Noise"));
	ToolSet_Noise->AddTool(new FLandscapeToolNoise<FHeightmapToolTarget>(this));
	ToolSet_Noise->AddTool(new FLandscapeToolNoise<FWeightmapToolTarget>(this));

	ToolSet_Noise->ValidBrushes.Add("BrushSet_Circle");
	ToolSet_Noise->ValidBrushes.Add("BrushSet_Alpha");
	ToolSet_Noise->ValidBrushes.Add("BrushSet_Pattern");
}
