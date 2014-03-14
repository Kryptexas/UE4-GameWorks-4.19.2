// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Core.h"
#include "ModuleInterface.h"
#include "ModuleManager.h"
#include "Engine.h"
#include "TargetPlatform.h"
#include "PhysXFormats.h"

checkAtCompileTime(WITH_PHYSX, no_point_in_compiling_physx_cooker_if_we_dont_have_physx);

static FName NAME_PhysXPC(TEXT("PhysXPC"));
static FName NAME_PhysXXboxOne(TEXT("PhysXXboxOne"));
static FName NAME_PhysXPS4(TEXT("PhysXPS4"));

/**
 * FPhysXFormats. Cooks physics data.
**/
class FPhysXFormats : public IPhysXFormat
{
	enum
	{
		/** Version for PhysX format, this becomes part of the DDC key. */
		UE_PHYSX_PC_VER = 0,
	};

	PxCooking* PhysXCooking;

	/**
	 * Validates format name and returns its PhysX enum value.
	 *
	 * @param InFormatName PhysX format name.
	 * @param OutFormat PhysX enum
	 * @return true if InFormatName is a valid and supported PhysX format
	 */
	bool GetPhysXFormat(FName InFormatName, PxPlatform::Enum& OutFormat) const
	{
		if ((InFormatName == NAME_PhysXPC) || (InFormatName == NAME_PhysXXboxOne) || (InFormatName == NAME_PhysXPS4))
		{
			OutFormat = PxPlatform::ePC;
		}
		else
		{
			return false;
		}
		return true;
	}

	/**
	 * Valudates PhysX format name.
	 */
	bool CheckPhysXFormat(FName InFormatName) const
	{
		PxPlatform::Enum PhysXFormat = PxPlatform::ePC;
		return GetPhysXFormat(InFormatName, PhysXFormat);
	}

public:

	FPhysXFormats( PxCooking* InCooking )
		: PhysXCooking( InCooking )
	{}

	virtual bool AllowParallelBuild() const
	{
		return false;
	}

	virtual uint16 GetVersion(FName Format) const OVERRIDE
	{
		check(CheckPhysXFormat(Format));
		return UE_PHYSX_PC_VER;
	}


	virtual void GetSupportedFormats(TArray<FName>& OutFormats) const
	{
		OutFormats.Add(NAME_PhysXPC);
		OutFormats.Add(NAME_PhysXXboxOne);
		OutFormats.Add(NAME_PhysXPS4);
	}

	virtual bool CookConvex(FName Format, const TArray<FVector>& SrcBuffer, TArray<uint8>& OutBuffer) const OVERRIDE
	{
#if WITH_PHYSX
		PxPlatform::Enum PhysXFormat = PxPlatform::ePC;
		bool bIsPhysXFormatValid = GetPhysXFormat(Format, PhysXFormat);
		check(bIsPhysXFormatValid);

		PxConvexMeshDesc PConvexMeshDesc;
		PConvexMeshDesc.points.data = SrcBuffer.GetData();
		PConvexMeshDesc.points.count = SrcBuffer.Num();
		PConvexMeshDesc.points.stride = sizeof(FVector);
		PConvexMeshDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

		// Set up cooking
		const PxCookingParams& Params = PhysXCooking->getParams();
		PxCookingParams NewParams = Params;
		NewParams.targetPlatform = PhysXFormat;
		PhysXCooking->setParams(NewParams);

		// Cook the convex mesh to a temp buffer
		TArray<uint8> CookedMeshBuffer;
		FPhysXOutputStream Buffer(&CookedMeshBuffer);
		bool Result = PhysXCooking->cookConvexMesh(PConvexMeshDesc, Buffer);
		
		if( Result && CookedMeshBuffer.Num() > 0 )
		{
			// Append the cooked data into cooked buffer
			OutBuffer.Append( CookedMeshBuffer );
			return true;
		}
#endif		// WITH_PHYSX
		return false;
	}

	virtual bool CookTriMesh(FName Format, const TArray<FVector>& SrcVertices, const TArray<FTriIndices>& SrcIndices, const TArray<uint16>& SrcMaterialIndices, const bool FlipNormals, TArray<uint8>& OutBuffer) const OVERRIDE
	{
#if WITH_PHYSX
		PxPlatform::Enum PhysXFormat = PxPlatform::ePC;
		bool bIsPhysXFormatValid = GetPhysXFormat(Format, PhysXFormat);
		check(bIsPhysXFormatValid);

		PxTriangleMeshDesc PTriMeshDesc;
		PTriMeshDesc.points.data = SrcVertices.GetData();
		PTriMeshDesc.points.count = SrcVertices.Num();
		PTriMeshDesc.points.stride = sizeof(FVector);
		PTriMeshDesc.triangles.data = SrcIndices.GetData();
		PTriMeshDesc.triangles.count = SrcIndices.Num();
		PTriMeshDesc.triangles.stride = sizeof(FTriIndices);
		PTriMeshDesc.materialIndices.data = SrcMaterialIndices.GetData();
		PTriMeshDesc.materialIndices.stride = sizeof(PxMaterialTableIndex);
		PTriMeshDesc.flags = FlipNormals ? PxMeshFlag::eFLIPNORMALS : (PxMeshFlags)0;

		// Set up cooking
		const PxCookingParams& Params = PhysXCooking->getParams();
		PxCookingParams NewParams = Params;
		NewParams.targetPlatform = PhysXFormat;
		PhysXCooking->setParams(NewParams);

		// Cook TriMesh Data
		FPhysXOutputStream Buffer(&OutBuffer);
		bool Result = PhysXCooking->cookTriangleMesh(PTriMeshDesc, Buffer);
		return Result;
#else
		return false;
#endif		// WITH_PHYSX
	}
};


/**
 * Module for PhysX cooking
 */
static IPhysXFormat* Singleton = NULL;

class FPhysXPlatformModule : public IPhysXFormatModule
{
public:
	FPhysXPlatformModule()
	{

	}

	virtual ~FPhysXPlatformModule()
	{
		ShutdownPhysXCooking();

		delete Singleton;
		Singleton = NULL;
	}
	virtual IPhysXFormat* GetPhysXFormat()
	{
		if (!Singleton)
		{
			InitPhysXCooking();
			Singleton = new FPhysXFormats(GPhysXCooking);
		}
		return Singleton;
	}

private:

	/**
	 *	Load the required modules for PhysX
	 */
	void InitPhysXCooking()
	{
		// Make sure PhysX libs are loaded
		LoadPhysXModules();
	}

	void ShutdownPhysXCooking()
	{
		// Ideally PhysX cooking should be initialized in InitPhysXCooking and released here
		// but Apex is still being setup in the engine and it also requires PhysX cooking singleton.
	}
};

IMPLEMENT_MODULE( FPhysXPlatformModule, PhysXFormats );
