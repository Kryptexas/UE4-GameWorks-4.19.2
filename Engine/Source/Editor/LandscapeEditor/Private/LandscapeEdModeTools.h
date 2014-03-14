// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

//
//	FNoiseParameter - Perlin noise
//
struct FNoiseParameter
{
	float	Base,
		NoiseScale,
		NoiseAmount;

	// Constructors.

	FNoiseParameter() {}
	FNoiseParameter(float InBase,float InScale,float InAmount):
	Base(InBase),
		NoiseScale(InScale),
		NoiseAmount(InAmount)
	{}

	// Sample
	float Sample(int32 X,int32 Y) const
	{
		float	Noise = 0.0f;
		X = FMath::Abs(X);
		Y = FMath::Abs(Y);

		if(NoiseScale > DELTA)
		{
			for(uint32 Octave = 0;Octave < 4;Octave++)
			{
				float	OctaveShift = 1 << Octave;
				float	OctaveScale = OctaveShift / NoiseScale;
				Noise += PerlinNoise2D(X * OctaveScale,Y * OctaveScale) / OctaveShift;
			}
		}

		return Base + Noise * NoiseAmount;
	}

	// TestGreater - Returns 1 if TestValue is greater than the parameter.
	bool TestGreater(int32 X,int32 Y,float TestValue) const
	{
		float	ParameterValue = Base;

		if(NoiseScale > DELTA)
		{
			for(uint32 Octave = 0;Octave < 4;Octave++)
			{
				float	OctaveShift = 1 << Octave;
				float	OctaveAmplitude = NoiseAmount / OctaveShift;

				// Attempt to avoid calculating noise if the test value is outside of the noise amplitude.

				if(TestValue > ParameterValue + OctaveAmplitude)
					return 1;
				else if(TestValue < ParameterValue - OctaveAmplitude)
					return 0;
				else
				{
					float	OctaveScale = OctaveShift / NoiseScale;
					ParameterValue += PerlinNoise2D(X * OctaveScale,Y * OctaveScale) * OctaveAmplitude;
				}
			}
		}

		return TestValue >= ParameterValue;
	}

	// TestLess
	bool TestLess(int32 X,int32 Y,float TestValue) const { return !TestGreater(X,Y,TestValue); }

private:
	static const int32 Permutations[256];

	bool operator==(const FNoiseParameter& SrcNoise)
	{
		if ((Base == SrcNoise.Base) &&
			(NoiseScale == SrcNoise.NoiseScale) &&
			(NoiseAmount == SrcNoise.NoiseAmount))
		{
			return true;
		}

		return false;
	}

	void operator=(const FNoiseParameter& SrcNoise)
	{
		Base = SrcNoise.Base;
		NoiseScale = SrcNoise.NoiseScale;
		NoiseAmount = SrcNoise.NoiseAmount;
	}


	float Fade(float T) const
	{
		return T * T * T * (T * (T * 6 - 15) + 10);
	}


	float Grad(int32 Hash,float X,float Y) const
	{
		int32		H = Hash & 15;
		float	U = H < 8 || H == 12 || H == 13 ? X : Y,
			V = H < 4 || H == 12 || H == 13 ? Y : 0;
		return ((H & 1) == 0 ? U : -U) + ((H & 2) == 0 ? V : -V);
	}

	float PerlinNoise2D(float X,float Y) const
	{
		int32		TruncX = FMath::Trunc(X),
			TruncY = FMath::Trunc(Y),
			IntX = TruncX & 255,
			IntY = TruncY & 255;
		float	FracX = X - TruncX,
			FracY = Y - TruncY;

		float	U = Fade(FracX),
			V = Fade(FracY);

		int32	A = Permutations[IntX] + IntY,
			AA = Permutations[A & 255],
			AB = Permutations[(A + 1) & 255],
			B = Permutations[(IntX + 1) & 255] + IntY,
			BA = Permutations[B & 255],
			BB = Permutations[(B + 1) & 255];

		return	FMath::Lerp(	FMath::Lerp(	Grad(Permutations[AA],			FracX,	FracY	),
			Grad(Permutations[BA],			FracX-1,FracY	),	U),
			FMath::Lerp(	Grad(Permutations[AB],			FracX,	FracY-1	),
			Grad(Permutations[BB],			FracX-1,FracY-1	),	U),	V);
	}
};



#if WITH_KISSFFT
#include "tools/kiss_fftnd.h" // Kiss FFT for Real component...
#endif

template<typename DataType>
inline void LowPassFilter(int32 X1, int32 Y1, int32 X2, int32 Y2, TMap<FIntPoint, float>& BrushInfo, TArray<DataType>& Data, const float DetailScale, const float ApplyRatio = 1.f)
{
#if WITH_KISSFFT
	// Low-pass filter
	int32 FFTWidth = X2-X1-1;
	int32 FFTHeight = Y2-Y1-1;

	const int NDims = 2;
	const int32 Dims[NDims] = {FFTHeight-FFTHeight%2, FFTWidth-FFTWidth%2};
	kiss_fftnd_cfg stf = kiss_fftnd_alloc(Dims, NDims, 0, NULL, NULL),
		sti = kiss_fftnd_alloc(Dims, NDims, 1, NULL, NULL);

	kiss_fft_cpx *buf = (kiss_fft_cpx *)KISS_FFT_MALLOC(sizeof(kiss_fft_cpx) * Dims[0] * Dims[1]);
	kiss_fft_cpx *out = (kiss_fft_cpx *)KISS_FFT_MALLOC(sizeof(kiss_fft_cpx) * Dims[0] * Dims[1]);

	for (int X = X1+1; X <= X2-1-FFTWidth%2; X++)
	{
		for (int Y = Y1+1; Y <= Y2-1-FFTHeight%2; Y++)
		{
			buf[(X-X1-1) + (Y-Y1-1)*(Dims[1])].r = Data[(X-X1) + (Y-Y1)*(1+X2-X1)];
			buf[(X-X1-1) + (Y-Y1-1)*(Dims[1])].i = 0;
		}
	}

	// Forward FFT
	kiss_fftnd(stf, buf, out);

	int32 CenterPos[2] = {Dims[0]>>1, Dims[1]>>1};
	for (int Y = 0; Y < Dims[0]; Y++)
	{
		float DistFromCenter = 0.f;
		for (int X = 0; X < Dims[1]; X++)
		{
			if (Y < CenterPos[0])
			{
				if (X < CenterPos[1])
				{
					// 1
					DistFromCenter = X*X + Y*Y;
				}
				else
				{
					// 2
					DistFromCenter = (X-Dims[1])*(X-Dims[1]) + Y*Y;
				}
			}
			else
			{
				if (X < CenterPos[1])
				{
					// 3
					DistFromCenter = X*X + (Y-Dims[0])*(Y-Dims[0]);
				}
				else
				{
					// 4
					DistFromCenter = (X-Dims[1])*(X-Dims[1]) + (Y-Dims[0])*(Y-Dims[0]);
				}
			}
			// High frequency removal
			float Ratio = 1.f - DetailScale;
			float Dist = FMath::Min<float>((Dims[0]*Ratio)*(Dims[0]*Ratio), (Dims[1]*Ratio)*(Dims[1]*Ratio));
			float Filter = 1.0 / (1.0 + DistFromCenter/Dist);
			out[X+Y*Dims[1]].r *= Filter;
			out[X+Y*Dims[1]].i *= Filter;
		}
	}

	// Inverse FFT
	kiss_fftnd(sti, out, buf);

	float Scale = Dims[0] * Dims[1];
	for( auto It = BrushInfo.CreateConstIterator(); It; ++It )
	{
		int32 X, Y;
		ALandscape::UnpackKey(It.Key(), X, Y);

		if (It.Value() > 0.f)
		{
			Data[(X-X1) + (Y-Y1)*(1+X2-X1)] = FMath::Lerp((float)Data[(X-X1) + (Y-Y1)*(1+X2-X1)], buf[(X-X1-1) + (Y-Y1-1)*(Dims[1])].r / Scale, It.Value() * ApplyRatio);
			//buf[(X-X1-1) + (Y-Y1-1)*(Dims[1])].r / Scale;
		}
	}

	// Free FFT allocation
	KISS_FFT_FREE(stf);
	KISS_FFT_FREE(sti);
	KISS_FFT_FREE(buf);
	KISS_FFT_FREE(out);
#endif
}



//
// FLandscapeHeightCache
//
template<class Accessor, typename AccessorType>
struct TLandscapeEditCache
{
	TLandscapeEditCache(Accessor& InDataAccess)
		:	DataAccess(InDataAccess)
		,	Valid(false)
	{
	}

	void CacheData( int32 X1, int32 Y1, int32 X2, int32 Y2 )
	{
		if( !Valid )
		{
			if (Accessor::bUseInterp)
			{
				ValidX1 = CachedX1 = X1;
				ValidY1 = CachedY1 = Y1;
				ValidX2 = CachedX2 = X2;
				ValidY2 = CachedY2 = Y2;

				DataAccess.GetData( ValidX1, ValidY1, ValidX2, ValidY2, CachedData );
				if (!ensureMsgf(ValidX1 <= ValidX2 && ValidY1 <= ValidY2, TEXT("Invalid cache area: X(%d-%d), Y(%d-%d) from region X(%d-%d), Y(%d-%d)"), ValidX1, ValidX2, ValidY1, ValidY2, X1, X2, Y1, Y2))
				{
					Valid = false;
					return;
				}
			}
			else
			{
				CachedX1 = X1;
				CachedY1 = Y1;
				CachedX2 = X2;
				CachedY2 = Y2;

				DataAccess.GetDataFast( CachedX1, CachedY1, CachedX2, CachedY2, CachedData );
			}

			OriginalData = CachedData;

			Valid = true;
		}
		else
		{
			// Extend the cache area if needed
			if( X1 < CachedX1 )
			{
				if (Accessor::bUseInterp)
				{
					int32 x1 = X1;
					int32 x2 = ValidX1;
					int32 y1 = FMath::Min<int32>(Y1,CachedY1);
					int32 y2 = FMath::Max<int32>(Y2,CachedY2);

					DataAccess.GetData( x1, y1, x2, y2, CachedData );
					ValidX1 = FMath::Min<int32>(x1,ValidX1);
				}
				else
				{
					DataAccess.GetDataFast( X1, CachedY1, CachedX1-1, CachedY2, CachedData );
				}

				CacheOriginalData( X1, CachedY1, CachedX1-1, CachedY2 );
				CachedX1 = X1;
			}

			if( X2 > CachedX2 )
			{
				if (Accessor::bUseInterp)
				{
					int32 x1 = ValidX2;
					int32 x2 = X2;
					int32 y1 = FMath::Min<int32>(Y1,CachedY1);
					int32 y2 = FMath::Max<int32>(Y2,CachedY2);

					DataAccess.GetData( x1, y1, x2, y2, CachedData );
					ValidX2 = FMath::Max<int32>(x2,ValidX2);
				}
				else
				{
					DataAccess.GetDataFast( CachedX2+1, CachedY1, X2, CachedY2, CachedData );
				}
				CacheOriginalData( CachedX2+1, CachedY1, X2, CachedY2 );			
				CachedX2 = X2;
			}			

			if( Y1 < CachedY1 )
			{
				if (Accessor::bUseInterp)
				{
					int32 x1 = CachedX1;
					int32 x2 = CachedX2;
					int32 y1 = Y1;
					int32 y2 = ValidY1;

					DataAccess.GetData( x1, y1, x2, y2, CachedData );
					ValidY1 = FMath::Min<int32>(y1,ValidY1);
				}
				else
				{
					DataAccess.GetDataFast( CachedX1, Y1, CachedX2, CachedY1-1, CachedData );
				}
				CacheOriginalData( CachedX1, Y1, CachedX2, CachedY1-1 );			
				CachedY1 = Y1;
			}

			if( Y2 > CachedY2 )
			{
				if (Accessor::bUseInterp)
				{
					int32 x1 = CachedX1;
					int32 x2 = CachedX2;
					int32 y1 = ValidY2;
					int32 y2 = Y2;

					DataAccess.GetData( x1, y1, x2, y2, CachedData );
					ValidY2 = FMath::Max<int32>(y2,ValidY2);
				}
				else
				{
					DataAccess.GetDataFast( CachedX1, CachedY2+1, CachedX2, Y2, CachedData );
				}

				CacheOriginalData( CachedX1, CachedY2+1, CachedX2, Y2 );			
				CachedY2 = Y2;
			}
		}
	}

	AccessorType* GetValueRef(int32 LandscapeX, int32 LandscapeY)
	{
		return CachedData.Find(ALandscape::MakeKey(LandscapeX,LandscapeY));
	}

	float GetValue(float LandscapeX, float LandscapeY)
	{
		int32 X = FMath::Floor(LandscapeX);
		int32 Y = FMath::Floor(LandscapeY);
		AccessorType* P00 = CachedData.Find(ALandscape::MakeKey(X, Y));
		AccessorType* P10 = CachedData.Find(ALandscape::MakeKey(X+1, Y));
		AccessorType* P01 = CachedData.Find(ALandscape::MakeKey(X, Y+1));
		AccessorType* P11 = CachedData.Find(ALandscape::MakeKey(X+1, Y+1));

		// Search for nearest value if missing data
		float V00 = P00 ? *P00 : (P10 ? *P10 : (P01 ? *P01 : (P11 ? *P11 : 0.f) ));
		float V10 = P10 ? *P10 : (P00 ? *P00 : (P11 ? *P11 : (P01 ? *P01 : 0.f) ));
		float V01 = P01 ? *P01 : (P00 ? *P00 : (P11 ? *P11 : (P10 ? *P10 : 0.f) ));
		float V11 = P11 ? *P11 : (P10 ? *P10 : (P01 ? *P01 : (P00 ? *P00 : 0.f) ));

		return FMath::Lerp(
			FMath::Lerp(V00, V10, LandscapeX - X),
			FMath::Lerp(V01, V11, LandscapeX - X),
			LandscapeY - Y);
	}

	FVector GetNormal(int32 X, int32 Y)
	{
		AccessorType* P00 = CachedData.Find(ALandscape::MakeKey(X, Y));
		AccessorType* P10 = CachedData.Find(ALandscape::MakeKey(X+1, Y));
		AccessorType* P01 = CachedData.Find(ALandscape::MakeKey(X, Y+1));
		AccessorType* P11 = CachedData.Find(ALandscape::MakeKey(X+1, Y+1));

		// Search for nearest value if missing data
		float V00 = P00 ? *P00 : (P10 ? *P10 : (P01 ? *P01 : (P11 ? *P11 : 0.f) ));
		float V10 = P10 ? *P10 : (P00 ? *P00 : (P11 ? *P11 : (P01 ? *P01 : 0.f) ));
		float V01 = P01 ? *P01 : (P00 ? *P00 : (P11 ? *P11 : (P10 ? *P10 : 0.f) ));
		float V11 = P11 ? *P11 : (P10 ? *P10 : (P01 ? *P01 : (P00 ? *P00 : 0.f) ));

		FVector Vert00 = FVector(0.f,0.f, V00);
		FVector Vert01 = FVector(0.f,1.f, V01);
		FVector Vert10 = FVector(1.f,0.f, V10);
		FVector Vert11 = FVector(1.f,1.f, V11);

		FVector FaceNormal1 = ((Vert00-Vert10) ^ (Vert10-Vert11)).SafeNormal();
		FVector FaceNormal2 = ((Vert11-Vert01) ^ (Vert01-Vert00)).SafeNormal();
		return (FaceNormal1 + FaceNormal2).SafeNormal();
	}

	void SetValue(int32 LandscapeX, int32 LandscapeY, AccessorType Value)
	{
		CachedData.Add(ALandscape::MakeKey(LandscapeX,LandscapeY), Value);
	}

	void GetCachedData(int32 X1, int32 Y1, int32 X2, int32 Y2, TArray<AccessorType>& OutData)
	{
		int32 NumSamples = (1+X2-X1)*(1+Y2-Y1);
		OutData.Empty(NumSamples);
		OutData.AddUninitialized(NumSamples);

		for( int32 Y=Y1;Y<=Y2;Y++ )
		{
			for( int32 X=X1;X<=X2;X++ )
			{
				AccessorType* Ptr = GetValueRef(X,Y);
				if( Ptr )
				{
					OutData[(X-X1) + (Y-Y1)*(1+X2-X1)] = *Ptr;
				}
			}
		}
	}

	void SetCachedData(int32 X1, int32 Y1, int32 X2, int32 Y2, TArray<AccessorType>& Data, ELandscapeLayerPaintingRestriction::Type PaintingRestriction = ELandscapeLayerPaintingRestriction::None)
	{
		// Update cache
		for( int32 Y=Y1;Y<=Y2;Y++ )
		{
			for( int32 X=X1;X<=X2;X++ )
			{
				SetValue( X, Y, Data[(X-X1) + (Y-Y1)*(1+X2-X1)] );
			}
		}

		// Update real data
		DataAccess.SetData( X1, Y1, X2, Y2, Data.GetTypedData(), PaintingRestriction );
	}

	// Get the original data before we made any changes with the SetCachedData interface.
	void GetOriginalData(int32 X1, int32 Y1, int32 X2, int32 Y2, TArray<AccessorType>& OutOriginalData)
	{
		int32 NumSamples = (1+X2-X1)*(1+Y2-Y1);
		OutOriginalData.Empty(NumSamples);
		OutOriginalData.AddUninitialized(NumSamples);

		for( int32 Y=Y1;Y<=Y2;Y++ )
		{
			for( int32 X=X1;X<=X2;X++ )
			{
				AccessorType* Ptr = OriginalData.Find(ALandscape::MakeKey(X,Y));
				if( Ptr )
				{
					OutOriginalData[(X-X1) + (Y-Y1)*(1+X2-X1)] = *Ptr;
				}
			}
		}
	}

	void Flush()
	{
		DataAccess.Flush();
	}

protected:
	Accessor& DataAccess;
private:
	void CacheOriginalData(int32 X1, int32 Y1, int32 X2, int32 Y2 )
	{
		for( int32 Y=Y1;Y<=Y2;Y++ )
		{
			for( int32 X=X1;X<=X2;X++ )
			{
				FIntPoint Key = ALandscape::MakeKey(X,Y);
				AccessorType* Ptr = CachedData.Find(Key);
				if( Ptr )
				{
					check( OriginalData.Find(Key) == NULL );
					OriginalData.Add(Key, *Ptr);
				}
			}
		}
	}

	TMap<FIntPoint, AccessorType> CachedData;
	TMap<FIntPoint, AccessorType> OriginalData;

	bool Valid;

	int32 CachedX1;
	int32 CachedY1;
	int32 CachedX2;
	int32 CachedY2;

	// To store valid region....
	int32 ValidX1, ValidX2, ValidY1, ValidY2;
};

//
// FHeightmapAccessor
//
template<bool bInUseInterp>
struct FHeightmapAccessor
{
	enum { bUseInterp = bInUseInterp };
	FHeightmapAccessor( ULandscapeInfo* InLandscapeInfo )
	{
		LandscapeInfo = InLandscapeInfo;
		LandscapeEdit = new FLandscapeEditDataInterface(InLandscapeInfo);
	}

	// accessors
	void GetData(int32& X1, int32& Y1, int32& X2, int32& Y2, TMap<FIntPoint, uint16>& Data)
	{
		LandscapeEdit->GetHeightData( X1, Y1, X2, Y2, Data);
	}

	void GetDataFast(int32 X1, int32 Y1, int32 X2, int32 Y2, TMap<FIntPoint, uint16>& Data)
	{
		LandscapeEdit->GetHeightDataFast( X1, Y1, X2, Y2, Data);
	}

	void SetData(int32 X1, int32 Y1, int32 X2, int32 Y2, const uint16* Data, ELandscapeLayerPaintingRestriction::Type PaintingRestriction = ELandscapeLayerPaintingRestriction::None)
	{
		TSet<ULandscapeComponent*> Components;
		if (LandscapeInfo && LandscapeEdit->GetComponentsInRegion(X1, Y1, X2, Y2, &Components))
		{
			// Update data
			ChangedComponents.Append(Components);

			for (TSet<ULandscapeComponent*>::TConstIterator It(Components); It; ++It)
			{
				(*It)->InvalidateLightingCache();
			}

			// Notify foliage to move any attached instances
			AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActor(LandscapeInfo->GetLandscapeProxy()->GetWorld(), false);
			if( IFA )
			{
				// Calculate landscape local-space bounding box of old data, to look for foliage instances.
				TArray<ULandscapeHeightfieldCollisionComponent*> CollisionComponents;
				CollisionComponents.Empty(Components.Num());
				TArray<FBox> PreUpdateLocalBoxes;
				PreUpdateLocalBoxes.Empty(Components.Num());

				int32 Index=0;
				for(TSet<ULandscapeComponent*>::TConstIterator It(Components);It;++It)
				{
					CollisionComponents.Add( (*It)->CollisionComponent.Get() );
					PreUpdateLocalBoxes.Add( FBox( FVector((float)X1, (float)Y1, (*It)->CachedLocalBox.Min.Z), FVector((float)X2, (float)Y2, (*It)->CachedLocalBox.Max.Z) ) );
					Index++;
				}

				// Update landscape.
				LandscapeEdit->SetHeightData( X1, Y1, X2, Y2, Data, 0, true);

				// Snap foliage for each component.
				for( Index=0; Index<CollisionComponents.Num();Index++ )
				{
					IFA->SnapInstancesForLandscape( CollisionComponents[Index], PreUpdateLocalBoxes[Index].TransformBy(LandscapeInfo->GetLandscapeProxy()->LandscapeActorToWorld().ToMatrixWithScale()).ExpandBy(1.f) );
				}
			}
			else
			{
				// No foliage, just update landscape.
				LandscapeEdit->SetHeightData( X1, Y1, X2, Y2, Data, 0, true);
			}
		}
		else
		{
			ChangedComponents.Empty();
		}
	}

	void Flush()
	{
		LandscapeEdit->Flush();
	}

	virtual ~FHeightmapAccessor()
	{
		delete LandscapeEdit;
		LandscapeEdit = NULL;

		// Update the bounds and navmesh for the components we edited
		for(TSet<ULandscapeComponent*>::TConstIterator It(ChangedComponents);It;++It)
		{
			(*It)->UpdateCachedBounds();
			(*It)->UpdateComponentToWorld();

			ULandscapeHeightfieldCollisionComponent* NavCollisionComp = (*It)->CollisionComponent.Get();
			if (NavCollisionComp)
			{
				UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(*It);
				if (NavSys)
				{
					NavSys->UpdateNavOctree(NavCollisionComp);
				}
			}
		}
	}

private:
	ULandscapeInfo* LandscapeInfo;
	FLandscapeEditDataInterface* LandscapeEdit;
	TSet<ULandscapeComponent*> ChangedComponents;
};

struct FLandscapeHeightCache : public TLandscapeEditCache<FHeightmapAccessor<true>,uint16>
{
	typedef uint16 DataType;
	static uint16 ClampValue( int32 Value ) { return FMath::Clamp(Value, 0, LandscapeDataAccess::MaxValue); }

	FHeightmapAccessor<true> HeightmapAccessor;

	FLandscapeHeightCache(const FLandscapeToolTarget& InTarget)
		:	HeightmapAccessor(InTarget.LandscapeInfo.Get())
		,	TLandscapeEditCache(HeightmapAccessor)
	{
	}
};

//
// FXYOffsetmapAccessor
//
template<bool bInUseInterp>
struct FXYOffsetmapAccessor
{
	enum { bUseInterp = bInUseInterp };
	FXYOffsetmapAccessor( ULandscapeInfo* InLandscapeInfo )
	{
		LandscapeInfo = InLandscapeInfo;
		LandscapeEdit = new FLandscapeEditDataInterface(InLandscapeInfo);
	}

	// accessors
	void GetData(int32& X1, int32& Y1, int32& X2, int32& Y2, TMap<FIntPoint, FVector>& Data)
	{
		LandscapeEdit->GetXYOffsetData(X1, Y1, X2, Y2, Data);

		TMap<FIntPoint, uint16> NewHeights;
		LandscapeEdit->GetHeightData( X1, Y1, X2, Y2, NewHeights);
		for (int32 Y = Y1; Y <= Y2; ++Y)
		{
			for (int32 X = X1; X <= X2; ++X)
			{
				FVector* Value = Data.Find( ALandscape::MakeKey(X, Y) );
				if (Value)
				{
					Value->Z = ( (float)NewHeights.FindRef( ALandscape::MakeKey(X, Y) ) - 32768.f) * LANDSCAPE_ZSCALE;
				}
			}
		}
	}

	void GetDataFast(int32 X1, int32 Y1, int32 X2, int32 Y2, TMap<FIntPoint, FVector>& Data)
	{
		LandscapeEdit->GetXYOffsetData(X1, Y1, X2, Y2, Data);

		TMap<FIntPoint, uint16> NewHeights;
		LandscapeEdit->GetHeightDataFast( X1, Y1, X2, Y2, NewHeights);
		for (int32 Y = Y1; Y <= Y2; ++Y)
		{
			for (int32 X = X1; X <= X2; ++X)
			{
				FVector* Value = Data.Find( ALandscape::MakeKey(X, Y) );
				if (Value)
				{
					Value->Z = ( (float)NewHeights.FindRef( ALandscape::MakeKey(X, Y) ) - 32768.f) * LANDSCAPE_ZSCALE;
				}
			}
		}
	}

	void SetData(int32 X1, int32 Y1, int32 X2, int32 Y2, const FVector* Data, ELandscapeLayerPaintingRestriction::Type PaintingRestriction = ELandscapeLayerPaintingRestriction::None)
	{
		TSet<ULandscapeComponent*> Components;
		if (LandscapeInfo && LandscapeEdit->GetComponentsInRegion(X1, Y1, X2, Y2, &Components))
		{
			// Update data
			ChangedComponents.Append(Components);

			// Convert Height to uint16
			TArray<uint16> NewHeights;
			NewHeights.AddZeroed( (Y2-Y1+1) * (X2-X1+1) );
			for (int32 Y = Y1; Y <= Y2; ++Y)
			{
				for (int32 X = X1; X <= X2; ++X)
				{
					NewHeights[X - X1 + (Y - Y1) * (X2-X1+1) ] = FMath::Clamp<uint16>(Data[(X - X1 + (Y - Y1) * (X2-X1+1) )].Z * LANDSCAPE_INV_ZSCALE + 32768.f, 0, 65535);
				}
			}

			// Notify foliage to move any attached instances
			AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActor(LandscapeInfo->GetLandscapeProxy()->GetWorld(), false);
			if( IFA )
			{
				// Calculate landscape local-space bounding box of old data, to look for foliage instances.
				TArray<ULandscapeHeightfieldCollisionComponent*> CollisionComponents;
				CollisionComponents.Empty(Components.Num());
				TArray<FBox> PreUpdateLocalBoxes;
				PreUpdateLocalBoxes.Empty(Components.Num());

				int32 Index=0;
				for(TSet<ULandscapeComponent*>::TConstIterator It(Components);It;++It)
				{
					CollisionComponents.Add( (*It)->CollisionComponent.Get() );
					PreUpdateLocalBoxes.Add( FBox( FVector((float)X1, (float)Y1, (*It)->CachedLocalBox.Min.Z), FVector((float)X2, (float)Y2, (*It)->CachedLocalBox.Max.Z) ) );
					Index++;
				}

				// Update landscape.
				LandscapeEdit->SetXYOffsetData( X1, Y1, X2, Y2, Data, 0 ); // XY Offset always need to be update before the height update
				LandscapeEdit->SetHeightData( X1, Y1, X2, Y2, NewHeights.GetTypedData(), 0, true);

				// Snap foliage for each component.
				for( Index=0; Index<CollisionComponents.Num();Index++ )
				{
					IFA->SnapInstancesForLandscape( CollisionComponents[Index], PreUpdateLocalBoxes[Index].TransformBy(LandscapeInfo->GetLandscapeProxy()->LandscapeActorToWorld().ToMatrixWithScale()).ExpandBy(1.f) );
				}
			}
			else
			{
				// No foliage, just update landscape.
				LandscapeEdit->SetXYOffsetData( X1, Y1, X2, Y2, Data, 0 ); // XY Offset always need to be update before the height update
				LandscapeEdit->SetHeightData( X1, Y1, X2, Y2, NewHeights.GetTypedData(), 0, true);
			}
		}
		else
		{
			ChangedComponents.Empty();
		}
	}

	void Flush()
	{
		LandscapeEdit->Flush();
	}

	virtual ~FXYOffsetmapAccessor()
	{
		delete LandscapeEdit;
		LandscapeEdit = NULL;

		// Update the bounds for the components we edited
		for(TSet<ULandscapeComponent*>::TConstIterator It(ChangedComponents);It;++It)
		{
			(*It)->UpdateCachedBounds();
			(*It)->UpdateComponentToWorld();
		}
	}

private:
	ULandscapeInfo* LandscapeInfo;
	FLandscapeEditDataInterface* LandscapeEdit;
	TSet<ULandscapeComponent*> ChangedComponents;
};

struct FLandscapeXYOffsetCache : public TLandscapeEditCache<FXYOffsetmapAccessor<true>,FVector>
{
	typedef FVector DataType;

	FXYOffsetmapAccessor<true> XYOffsetmapAccessor;

	FLandscapeXYOffsetCache(const FLandscapeToolTarget& InTarget)
		:	XYOffsetmapAccessor(InTarget.LandscapeInfo.Get())
		,	TLandscapeEditCache(XYOffsetmapAccessor)
	{
	}
};

//
// FAlphamapAccessor
//
template<bool bInUseInterp, bool bInUseTotalNormalize>
struct FAlphamapAccessor
{
	enum { bUseInterp = bInUseInterp };
	enum { bUseTotalNormalize = bInUseTotalNormalize };
	FAlphamapAccessor( ULandscapeInfo* InLandscapeInfo, ULandscapeLayerInfoObject* InLayerInfo )
		:	LandscapeInfo(InLandscapeInfo)
		,	LandscapeEdit(InLandscapeInfo)
		,	LayerInfo(InLayerInfo)
		,	bBlendWeight(true)
	{
		// should be no Layer change during FAlphamapAccessor lifetime...
		if (InLandscapeInfo && InLayerInfo)
		{
			if (LayerInfo == ALandscapeProxy::DataLayer)
			{
				bBlendWeight = false;
			}
			else
			{
				bBlendWeight = !LayerInfo->bNoWeightBlend;
			}
		}
	}

	~FAlphamapAccessor()
	{
		// Recreate collision for modified components to update the physical materials
		for (auto It = ModifiedComponents.CreateConstIterator(); It; ++It)
		{
			ULandscapeHeightfieldCollisionComponent* CollisionComponent = (*It)->CollisionComponent.Get();
			if( CollisionComponent )
			{
				CollisionComponent->RecreateCollision(false);
			}
		}
	}

	void GetData(int32& X1, int32& Y1, int32& X2, int32& Y2, TMap<FIntPoint, uint8>& Data)
	{
		LandscapeEdit.GetWeightData(LayerInfo, X1, Y1, X2, Y2, Data);
	}

	void GetDataFast(int32 X1, int32 Y1, int32 X2, int32 Y2, TMap<FIntPoint, uint8>& Data)
	{
		LandscapeEdit.GetWeightDataFast(LayerInfo, X1, Y1, X2, Y2, Data);
	}

	void SetData(int32 X1, int32 Y1, int32 X2, int32 Y2, const uint8* Data, ELandscapeLayerPaintingRestriction::Type PaintingRestriction)
	{
		TSet<ULandscapeComponent*> Components;
		if (LandscapeEdit.GetComponentsInRegion(X1, Y1, X2, Y2, &Components))
		{
			LandscapeEdit.SetAlphaData(LayerInfo, X1, Y1, X2, Y2, Data, 0, PaintingRestriction, bBlendWeight, bUseTotalNormalize);
			ModifiedComponents.Append(Components);
		}
	}

	void Flush()
	{
		LandscapeEdit.Flush();
	}

private:
	ULandscapeInfo* LandscapeInfo;
	FLandscapeEditDataInterface LandscapeEdit;
	TSet<ULandscapeComponent*> ModifiedComponents;
	ULandscapeLayerInfoObject* LayerInfo;
	bool bBlendWeight;
};

struct FLandscapeAlphaCache : public TLandscapeEditCache<FAlphamapAccessor<true, false>,uint8>
{
	typedef uint8 DataType;
	static uint8 ClampValue( int32 Value ) { return FMath::Clamp(Value, 0, 255); }

	FAlphamapAccessor<true, false> AlphamapAccessor;

	FLandscapeAlphaCache(const FLandscapeToolTarget& InTarget)
		:	AlphamapAccessor(InTarget.LandscapeInfo.Get(), InTarget.LayerInfo.Get())
		,	TLandscapeEditCache(AlphamapAccessor)
	{
	}
};

struct FLandscapeVisCache : public TLandscapeEditCache<FAlphamapAccessor<false, false>,uint8>
{
	typedef uint8 DataType;
	static uint8 ClampValue( int32 Value ) { return FMath::Clamp(Value, 0, 255); }

	FAlphamapAccessor<false, false> AlphamapAccessor;

	FLandscapeVisCache(const FLandscapeToolTarget& InTarget)
		:	AlphamapAccessor(InTarget.LandscapeInfo.Get(), ALandscapeProxy::DataLayer)
		,	TLandscapeEditCache(AlphamapAccessor)
	{
	}
};

//
// FFullWeightmapAccessor
//
template<bool bInUseInterp>
struct FFullWeightmapAccessor
{
	enum { bUseInterp = bInUseInterp };
	FFullWeightmapAccessor( ULandscapeInfo* InLandscapeInfo)
		:	LandscapeEdit(InLandscapeInfo)
	{
	}
	void GetData(int32& X1, int32& Y1, int32& X2, int32& Y2, TMap<FIntPoint, TArray<uint8>>& Data)
	{
		// Do not Support for interpolation....
		check(false && TEXT("Do not support interpolation for FullWeightmapAccessor for now"));
	}

	void GetDataFast(int32 X1, int32 Y1, int32 X2, int32 Y2, TMap<FIntPoint, TArray<uint8>>& Data)
	{
		DirtyLayerInfos.Empty();
		LandscapeEdit.GetWeightDataFast(NULL, X1, Y1, X2, Y2, Data);
	}

	void SetData(int32 X1, int32 Y1, int32 X2, int32 Y2, const uint8* Data, ELandscapeLayerPaintingRestriction::Type PaintingRestriction)
	{
		if (LandscapeEdit.GetComponentsInRegion(X1, Y1, X2, Y2))
		{
			LandscapeEdit.SetAlphaData(DirtyLayerInfos, X1, Y1, X2, Y2, Data, 0, PaintingRestriction);
		}
		DirtyLayerInfos.Empty();
	}

	void Flush()
	{
		LandscapeEdit.Flush();
	}

	TSet<ULandscapeLayerInfoObject*> DirtyLayerInfos;
private:
	FLandscapeEditDataInterface LandscapeEdit;
};

struct FLandscapeFullWeightCache : public TLandscapeEditCache<FFullWeightmapAccessor<false>,TArray<uint8>>
{
	typedef TArray<uint8> DataType;

	FFullWeightmapAccessor<false> WeightmapAccessor;

	FLandscapeFullWeightCache(const FLandscapeToolTarget& InTarget)
		:	WeightmapAccessor(InTarget.LandscapeInfo.Get())
		,	TLandscapeEditCache(WeightmapAccessor)
	{
	}

	// Only for all weight case... the accessor type should be TArray<uint8>
	void GetCachedData(int32 X1, int32 Y1, int32 X2, int32 Y2, TArray<uint8>& OutData, int32 ArraySize)
	{
		int32 NumSamples = (1+X2-X1)*(1+Y2-Y1) * ArraySize;
		OutData.Empty(NumSamples);
		OutData.AddUninitialized(NumSamples);

		for( int32 Y=Y1;Y<=Y2;Y++ )
		{
			for( int32 X=X1;X<=X2;X++ )
			{
				TArray<uint8>* Ptr = GetValueRef(X,Y);
				if( Ptr )
				{
					for (int32 Z = 0; Z < ArraySize; Z++)
					{
						OutData[( (X-X1) + (Y-Y1)*(1+X2-X1)) * ArraySize + Z] = (*Ptr)[Z];
					}
				}
			}
		}
	}

	// Only for all weight case... the accessor type should be TArray<uint8>
	void SetCachedData(int32 X1, int32 Y1, int32 X2, int32 Y2, TArray<uint8>& Data, int32 ArraySize, ELandscapeLayerPaintingRestriction::Type PaintingRestriction)
	{
		// Update cache
		for( int32 Y=Y1;Y<=Y2;Y++ )
		{
			for( int32 X=X1;X<=X2;X++ )
			{
				TArray<uint8> Value;
				Value.Empty(ArraySize);
				Value.AddUninitialized(ArraySize);
				for ( int32 Z=0; Z < ArraySize; Z++)
				{
					Value[Z] = Data[ ((X-X1) + (Y-Y1)*(1+X2-X1)) * ArraySize + Z];
				}
				SetValue( X, Y, Value );
			}
		}

		// Update real data
		DataAccess.SetData(X1, Y1, X2, Y2, Data.GetTypedData(), PaintingRestriction);
	}

	void AddDirtyLayer(ULandscapeLayerInfoObject* LayerInfo)
	{
		WeightmapAccessor.DirtyLayerInfos.Add(LayerInfo);
	}
};

// 
// FDatamapAccessor
//
template<bool bInUseInterp>
struct FDatamapAccessor
{
	enum { bUseInterp = bInUseInterp };
	FDatamapAccessor( ULandscapeInfo* InLandscapeInfo )
		:	LandscapeEdit(InLandscapeInfo)
	{
	}

	void GetData(int32& X1, int32& Y1, int32& X2, int32& Y2, TMap<FIntPoint, uint8>& Data)
	{
		LandscapeEdit.GetSelectData(X1, Y1, X2, Y2, Data);
	}

	void GetDataFast(const int32 X1, const int32 Y1, const int32 X2, const int32 Y2, TMap<FIntPoint, uint8>& Data)
	{
		LandscapeEdit.GetSelectData(X1, Y1, X2, Y2, Data);
	}

	void SetData(int32 X1, int32 Y1, int32 X2, int32 Y2, const uint8* Data, ELandscapeLayerPaintingRestriction::Type PaintingRestriction = ELandscapeLayerPaintingRestriction::None )
	{
		if (LandscapeEdit.GetComponentsInRegion(X1, Y1, X2, Y2))
		{
			LandscapeEdit.SetSelectData(X1, Y1, X2, Y2, Data, 0);
		}
	}

	void Flush()
	{
		LandscapeEdit.Flush();
	}

private:
	FLandscapeEditDataInterface LandscapeEdit;
};

struct FLandscapeDataCache : public TLandscapeEditCache<FDatamapAccessor<false>,uint8>
{
	typedef uint8 DataType;
	static uint8 ClampValue( int32 Value ) { return FMath::Clamp(Value, 0, 255); }

	FDatamapAccessor<false> DataAccessor;

	FLandscapeDataCache(const FLandscapeToolTarget& InTarget)
		:	DataAccessor(InTarget.LandscapeInfo.Get())
		,	TLandscapeEditCache(DataAccessor)
	{
	}
};


//
// Tool targets
//
struct FHeightmapToolTarget
{
	typedef FLandscapeHeightCache CacheClass;
	static const ELandscapeToolTargetType::Type TargetType = ELandscapeToolTargetType::Heightmap;

	static float StrengthMultiplier(ULandscapeInfo* LandscapeInfo, float BrushRadius)
	{
		if (LandscapeInfo)
		{
			// Adjust strength based on brush size and drawscale, so strength 1 = one hemisphere
			return BrushRadius * LANDSCAPE_INV_ZSCALE / (LandscapeInfo->DrawScale.Z);
		}
		return 5.f * LANDSCAPE_INV_ZSCALE;
	}

	static FMatrix ToWorldMatrix(ULandscapeInfo* LandscapeInfo)
	{
		FMatrix Result = FTranslationMatrix(FVector(0,0,-32768.f));
		Result *= FScaleMatrix( FVector(1.f,1.f,LANDSCAPE_ZSCALE) * LandscapeInfo->DrawScale );
		return Result;
	}

	static FMatrix FromWorldMatrix(ULandscapeInfo* LandscapeInfo)
	{
		FMatrix Result = FScaleMatrix( FVector(1.f,1.f,LANDSCAPE_INV_ZSCALE) / (LandscapeInfo->DrawScale) );
		Result *= FTranslationMatrix(FVector(0,0,32768.f));
		return Result;
	}
};


struct FWeightmapToolTarget
{
	typedef FLandscapeAlphaCache CacheClass;
	static const ELandscapeToolTargetType::Type TargetType = ELandscapeToolTargetType::Weightmap;

	static float StrengthMultiplier(ULandscapeInfo* LandscapeInfo, float BrushRadius)
	{
		return 255.f;
	}

	static FMatrix ToWorldMatrix(ULandscapeInfo* LandscapeInfo) { return FMatrix::Identity; }
	static FMatrix FromWorldMatrix(ULandscapeInfo* LandscapeInfo) { return FMatrix::Identity; }
};

template<class TInputType>
class FLandscapeStrokeBase
{
public:
	FLandscapeStrokeBase(TInputType& InTarget) {}
	virtual void Apply(FLevelEditorViewportClient* ViewportClient, FLandscapeBrush* Brush, const ULandscapeEditorObject* UISettings, const TArray<FLandscapeToolMousePosition>& MousePositions) = 0;
};


/*
 * FLandscapeToolPaintBase - base class for painting tools
 *		ToolTarget - the target for the tool (weight or heightmaap)
 *		StrokeClass - the class that implements the behavior for a mouse stroke applying the tool.
 */
template<class TStrokeClass>
class FLandscapeToolBase : public FLandscapeTool
{
public:
	FLandscapeToolBase(FEdModeLandscape* InEdMode)
	:	EdMode(InEdMode)
	,	bToolActive(false)
	,	ToolStroke(NULL)
	{}

	virtual bool BeginTool( FLevelEditorViewportClient* ViewportClient, const FLandscapeToolTarget& InTarget, const FVector& InHitLocation ) OVERRIDE
	{
		if( !ensure(MousePositions.Num()==0) ) 
		{
			MousePositions.Empty();
		}

		bToolActive = true;
		ToolStroke = new TStrokeClass(EdMode, InTarget);

		EdMode->CurrentBrush->BeginStroke(InHitLocation.X, InHitLocation.Y, this);

		new(MousePositions) FLandscapeToolMousePosition(InHitLocation.X, InHitLocation.Y, IsShiftDown(ViewportClient->Viewport));
		ToolStroke->Apply(ViewportClient, EdMode->CurrentBrush, EdMode->UISettings, MousePositions);
		MousePositions.Empty();
		return true;
	}

	virtual void Tick(FLevelEditorViewportClient* ViewportClient,float DeltaTime) OVERRIDE 
	{
		if( bToolActive && MousePositions.Num() )
		{
			ToolStroke->Apply(ViewportClient, EdMode->CurrentBrush, EdMode->UISettings, MousePositions);
			MousePositions.Empty();
		}
	}

	virtual void EndTool(FLevelEditorViewportClient* ViewportClient) OVERRIDE 
	{
		if( bToolActive && MousePositions.Num() )
		{
			ToolStroke->Apply(ViewportClient, EdMode->CurrentBrush, EdMode->UISettings, MousePositions);
			MousePositions.Empty();
		}

		delete ToolStroke;
		ToolStroke = NULL;
		bToolActive = false;
		EdMode->CurrentBrush->EndStroke();
	}

	virtual bool MouseMove( FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y ) OVERRIDE 
	{
		FVector HitLocation;
		if( EdMode->LandscapeMouseTrace(ViewportClient, x, y, HitLocation)  )
		{
			if( EdMode->CurrentBrush )
			{
				// Inform the brush of the current location, to update the cursor
				EdMode->CurrentBrush->MouseMove(HitLocation.X, HitLocation.Y);
			}

			if( bToolActive )
			{
				// Save the mouse position
				new(MousePositions) FLandscapeToolMousePosition(HitLocation.X, HitLocation.Y, IsShiftDown(ViewportClient->Viewport));
			}
		}

		return true;
	}	

protected:
	TArray<FLandscapeToolMousePosition> MousePositions;
	class FEdModeLandscape* EdMode;
	bool bToolActive;
	TStrokeClass* ToolStroke;
};
