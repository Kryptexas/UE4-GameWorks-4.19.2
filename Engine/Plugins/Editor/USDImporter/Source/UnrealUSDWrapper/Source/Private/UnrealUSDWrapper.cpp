// UnrealUSDWrapper.cpp : Defines the exported functions for the DLL application.
//

#include "UnrealUSDWrapper.h"
#include "pxr/usd/usd/usdFileFormat.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/stage.h"
#if PXR_VERSION < 075   // this file seems to have been removed in recent versions
    #include "pxr/usd/usd/treeIterator.h"
#endif // PXR_VERSION < 075
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/references.h"
#include "pxr/usd/usdGeom/modelAPI.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/xformCommonAPI.h"
#include "pxr/usd/usdGeom/faceSetAPI.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/setenv.h"
#include "pxr/usd/ar/defaultResolver.h"
#include "pxr/usd/ar/defineResolver.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/usd/usd/variantSets.h"
#include "pxr/usd/usd/debugCodes.h"

using std::vector;
using std::string;

#ifdef _WINDOWS
#pragma warning(disable:4267)
#define PXR_NAMESPACE
#define USDWRAPPER_USE_XFORMACHE	1
#else
#define PXR_NAMESPACE		pxr
#define USDWRAPPER_USE_XFORMACHE	0
#endif

#if USDWRAPPER_USE_XFORMACHE
static UsdGeomXformCache XFormCache;
#endif // USDWRAPPER_USE_XFORMACHE

void Log(const char* Format, ...)
{
	static char Buffer[2048];

	va_list args;

	va_start(args, Format);

#if _WINDOWS
	vsprintf_s(Buffer, Format, args);

	OutputDebugString(Buffer);
#else
	vsprintf(Buffer, Format, args);
	printf("%s", Buffer);
#endif

	va_end(args);
}

class USDHelpers
{

public:
	static void LogPrimTree(const PXR_NAMESPACE::UsdPrim& Root)
	{
		LogPrimTreeHelper("", Root);
	}

private:
	static void LogPrimTreeHelper(const string& Concat, const PXR_NAMESPACE::UsdPrim& Prim)
	{
		string TypeName = Prim.GetTypeName().GetString();
		bool bIsModel = Prim.IsModel();
		bool bIsAbstract = Prim.IsAbstract();
		bool bIsGroup = Prim.IsGroup();
		bool bIsInstance = Prim.IsInstance();
		bool bIsActive = Prim.IsActive();
		bool bInMaster = Prim.IsInMaster();
		bool bIsMaster = Prim.IsMaster();
		Log(string(Concat + "Prim: [%s] %s Model:%d Abstract:%d Group:%d Instance:%d Active:%d InMaster:%d IsMaster:%d\n").c_str(),
			TypeName.c_str(), Prim.GetName().GetText(), bIsModel, bIsAbstract, bIsGroup, bIsInstance, bIsActive, bInMaster, bIsMaster);
		{
			/*UsdMetadataValueMap Metadata = Prim.GetAllMetadata();
			if(Metadata.size())
			{
				Log(string(Concat+"\tMetaData:\n").c_str());
				for(auto KeyValue : Metadata)
				{
					Log(string(Concat+"\t\t[%s] %s\n").c_str(), KeyValue.second.GetTypeName().c_str(), KeyValue.first.GetText());
				}
			}*/

			/*vector<UsdAttribute> Attributes = Prim.GetAttributes();
			if(Attributes.size())
			{
				Log(string(Concat+"\tAttributes:\n").c_str());
				for(const UsdAttribute& Attribute : Attributes)
				{
					if (Attribute.IsAuthored())
					{
						Log(string(Concat + "\t\t[%s] %s %s\n").c_str(), Attribute.GetTypeName().GetAsToken().GetText(), Attribute.GetBaseName().GetText(), Attribute.GetDisplayName().c_str());
					}
				}
			}*/

			/*if (Prim.HasVariantSets())
			{
				Log(string(Concat + "\tVariant Sets:\n").c_str());
				UsdVariantSets VariantSets = Prim.GetVariantSets();
				vector<string> SetNames = VariantSets.GetNames();
				for (const string& SetName : SetNames)
				{
					Log(string(Concat + "\t\t%s:\n").c_str(), SetName.c_str());

					UsdVariantSet Set = Prim.GetVariantSet(SetName);

					vector<string> VariantNames = Set.GetVariantNames();
					for (const string& VariantName : VariantNames)
					{
						char ActiveChar = ' ';
						if (Set.GetVariantSelection() == VariantName)
						{
							ActiveChar = '*';
						}
						Log(string(Concat + "\t\t\t%s%c\n").c_str(), VariantName.c_str(), ActiveChar);
					}
				}
			}*/
		}


		for(const PXR_NAMESPACE::UsdPrim& Child : Prim.GetChildren())
		{
			LogPrimTreeHelper(Concat+"\t", Child);
		}

		//Log("\n");
	}
};

// Dll entrypoint
void InitWrapper()
{
}

struct FPrimAndData
{
	PXR_NAMESPACE::UsdPrim Prim;
	class FUsdPrim* PrimData;

	FPrimAndData(const PXR_NAMESPACE::UsdPrim& InPrim)
		: Prim(InPrim)
		, PrimData(nullptr)
	{}
};

class FUsdPrim : public IUsdPrim
{
public:
	FUsdPrim(const PXR_NAMESPACE::UsdPrim& InPrim)
		: Prim(InPrim)
		, GeomData(nullptr)
	{
		PrimName = Prim.GetName().GetString();
		PrimPath = Prim.GetPath().GetString();

		static const PXR_NAMESPACE::TfToken AssetPathToken = PXR_NAMESPACE::TfToken(UnrealIdentifiers::AssetPath);
		PXR_NAMESPACE::UsdAttribute UnrealAssetPathAttr = Prim.GetAttribute(AssetPathToken);
		if (UnrealAssetPathAttr.HasValue())
		{
			UnrealAssetPathAttr.Get(&UnrealAssetPath);
		}

		for (const PXR_NAMESPACE::UsdPrim& Child : Prim.GetChildren())
		{
			Children.push_back(FPrimAndData(Child));
		}
	}

	~FUsdPrim()
	{
		if (GeomData)
		{
			delete GeomData;
		}

		for (FPrimAndData& Child : Children)
		{
			if (Child.PrimData)
			{
				delete Child.PrimData;
			}
		}		

		for (FPrimAndData& Child : VariantData)
		{
			if (Child.PrimData)
			{
				delete Child.PrimData;
			}
		}

		Children.clear();

		VariantData.clear();
		
	}

	virtual const char* GetPrimName() const override
	{
		return PrimName.c_str();
	}

	virtual const char* GetPrimPath() const override
	{
		return PrimPath.c_str();
	}


	virtual bool IsGroup() const override
	{
		return Prim.IsGroup();
	}

	virtual bool HasTransform() const override
	{
		return PXR_NAMESPACE::UsdGeomXformable(Prim);
	}

	virtual FUsdMatrixData GetLocalToWorldTransform() const override
	{
		return GetLocalToWorldTransform(PXR_NAMESPACE::UsdTimeCode::Default().GetValue());
	}

#if !USDWRAPPER_USE_XFORMACHE
	static PXR_NAMESPACE::GfMatrix4d GetLocalToWorldTransform(const PXR_NAMESPACE::UsdPrim& Prim, double Time, const PXR_NAMESPACE::SdfPath& AbsoluteRootPath)
	{
		PXR_NAMESPACE::SdfPath PrimPath = Prim.GetPath();
		if (!Prim || PrimPath == AbsoluteRootPath)
		{
			return PXR_NAMESPACE::GfMatrix4d(1);
		}

		PXR_NAMESPACE::GfMatrix4d AccumulatedTransform(1.);
		bool bResetsXFormStack = false;
		PXR_NAMESPACE::UsdGeomXformable XFormable(Prim);
		// silently ignoring errors
		XFormable.GetLocalTransformation(&AccumulatedTransform, &bResetsXFormStack, Time);

		if (!bResetsXFormStack)
		{
			AccumulatedTransform = AccumulatedTransform * GetLocalToWorldTransform(Prim.GetParent(), Time, AbsoluteRootPath);
		}

		return AccumulatedTransform;
	}
#endif // !USDWRAPPER_USE_XFORMACHE
	
	virtual FUsdMatrixData GetLocalToWorldTransform(double Time) const override
	{
#if USDWRAPPER_USE_XFORMACHE
		XFormCache.SetTime(Time);
		PXR_NAMESPACE::GfMatrix4d LocalToWorld = XFormCache.GetLocalToWorldTransform(Prim);
#else
		PXR_NAMESPACE::UsdGeomXformable XFormable(Prim);
		PXR_NAMESPACE::SdfPath WorldPath = PXR_NAMESPACE::SdfPath::AbsoluteRootPath();
		PXR_NAMESPACE::GfMatrix4d LocalToWorld;
		if (!Prim.GetPath().HasPrefix(WorldPath))
		{
			LocalToWorld = PXR_NAMESPACE::GfMatrix4d(1);
		}
		else
		{
			LocalToWorld = GetLocalToWorldTransform(Prim, Time, WorldPath);
		}
#endif

		FUsdMatrixData Ret;
		memcpy(Ret.Data, LocalToWorld.GetArray(), sizeof(double) * 16);

		return Ret;

	}

	virtual FUsdMatrixData GetLocalToParentTransform() const override
	{
		return GetLocalToParentTransform(PXR_NAMESPACE::UsdTimeCode::Default().GetValue());
	}

	virtual FUsdMatrixData GetLocalToParentTransform(double Time) const override
	{
		bool bResetsXFormStack = false;
#if USDWRAPPER_USE_XFORMACHE
		XFormCache.SetTime(Time);
		PXR_NAMESPACE::GfMatrix4d LocalToParent = XFormCache.GetLocalTransformation(Prim, &bResetsXFormStack);
#else
		PXR_NAMESPACE::UsdGeomXformable XFormable(Prim);
		PXR_NAMESPACE::GfMatrix4d LocalToParent;
		// silently ignoring errors
		XFormable.GetLocalTransformation(&LocalToParent, &bResetsXFormStack, Time);
#endif
		FUsdMatrixData Ret;
		memcpy(Ret.Data, LocalToParent.GetArray(), sizeof(double) * 16);
		
		return Ret;
	}

	virtual int GetNumChildren() const override
	{
		return Children.size();
	}

	virtual IUsdPrim* GetChild(int ChildIndex) override
	{
		FPrimAndData& PrimAndData = Children[ChildIndex];
		if (!PrimAndData.PrimData)
		{
			PrimAndData.PrimData = new FUsdPrim(PrimAndData.Prim);
		}

		return PrimAndData.PrimData;
	}

	virtual const char* GetUnrealAssetPath() const override
	{
		return UnrealAssetPath.length() > 0 ? UnrealAssetPath.c_str() : nullptr;
	}

	bool HasGeometryData() const override
	{
		PXR_NAMESPACE::UsdGeomMesh Mesh(Prim);

		return Mesh;
	}

	virtual const FUsdGeomData* GetGeometryData() override
	{
		return GetGeometryData(PXR_NAMESPACE::UsdTimeCode::Default().GetValue());
	}

	virtual const FUsdGeomData* GetGeometryData(double Time) override
	{
		bool bValid = false;
		PXR_NAMESPACE::UsdGeomMesh Mesh(Prim);
		if (Mesh)
		{
			if (GeomData)
			{
				delete GeomData;
			}

			GeomData = new FUsdGeomData;

			// Faces and points
			{
				PXR_NAMESPACE::UsdAttribute FaceCounts = Mesh.GetFaceVertexCountsAttr();
				if (FaceCounts)
				{
					GeomData->FaceVertexCounts.clear();

					PXR_NAMESPACE::VtArray<int> FaceCountArray;
					FaceCounts.Get(&FaceCountArray, Time);
					GeomData->FaceVertexCounts.assign(FaceCountArray.begin(), FaceCountArray.end());
				}

				PXR_NAMESPACE::UsdAttribute FaceIndices = Mesh.GetFaceVertexIndicesAttr();
				if (FaceIndices)
				{
					GeomData->FaceIndices.clear();

					PXR_NAMESPACE::VtArray<int> FaceIndicesArray;
					FaceIndices.Get(&FaceIndicesArray, Time);
					GeomData->FaceIndices.assign(FaceIndicesArray.begin(), FaceIndicesArray.end());
				}

				PXR_NAMESPACE::UsdAttribute Points = Mesh.GetPointsAttr();
				if (Points)
				{
					GeomData->Points.clear();

					PXR_NAMESPACE::VtArray<PXR_NAMESPACE::GfVec3f> PointsArray;
					Points.Get(&PointsArray, Time);

					// Bug??  Usd returns nothing for UsdTimeCode::Default if there are animated points
					if (PointsArray.size() == 0 && Time == PXR_NAMESPACE::UsdTimeCode::Default())
					{
						// Try to get at time = 0
						Points.Get(&PointsArray, PXR_NAMESPACE::UsdTimeCode(0));
					}
					
					GeomData->Points.resize(PointsArray.size());
					memcpy(&GeomData->Points[0], &PointsArray[0], PointsArray.size() * sizeof(PXR_NAMESPACE::GfVec3f));
				}

				PXR_NAMESPACE::UsdAttribute Normals = Mesh.GetNormalsAttr();
				if (Normals)
				{
					GeomData->Normals.clear();

					PXR_NAMESPACE::VtArray<PXR_NAMESPACE::GfVec3f> NormalsArray;
					Normals.Get(&NormalsArray, Time);

					GeomData->Normals.resize(NormalsArray.size());
					memcpy(&GeomData->Normals[0], &NormalsArray[0], NormalsArray.size() * sizeof(PXR_NAMESPACE::GfVec3f));
				} 

				PXR_NAMESPACE::UsdGeomPrimvar DisplayColorPrimVar = Mesh.GetDisplayColorPrimvar();
				if (DisplayColorPrimVar)
				{
					GeomData->VertexColors.clear();

					PXR_NAMESPACE::TfToken Interpolation = DisplayColorPrimVar.GetInterpolation();

					if (Interpolation == PXR_NAMESPACE::UsdGeomTokens->faceVarying || Interpolation == PXR_NAMESPACE::UsdGeomTokens->uniform)
					{
						GeomData->VertexColorInterpMethod = Interpolation == PXR_NAMESPACE::UsdGeomTokens->faceVarying ? EUsdInterpolationMethod::FaceVarying : EUsdInterpolationMethod::Uniform;
						PXR_NAMESPACE::VtArray<PXR_NAMESPACE::GfVec3f> DisplayColorArray;
						DisplayColorPrimVar.ComputeFlattened(&DisplayColorArray, Time);
		
						GeomData->VertexColors.resize(DisplayColorArray.size());
						memcpy(&GeomData->VertexColors[0], &DisplayColorArray[0], DisplayColorArray.size() * sizeof(PXR_NAMESPACE::GfVec3f));
					}
					else if (Interpolation == PXR_NAMESPACE::UsdGeomTokens->vertex)
					{
						GeomData->VertexColorInterpMethod = EUsdInterpolationMethod::Vertex;
						PXR_NAMESPACE::VtIntArray VertexColorIndices;
						PXR_NAMESPACE::VtArray<PXR_NAMESPACE::GfVec3f> DisplayColorArray;
						DisplayColorPrimVar.GetIndices(&VertexColorIndices, Time);
						DisplayColorPrimVar.Get(&DisplayColorArray, Time);

						if (VertexColorIndices.size() == GeomData->Points.size())
						{
							GeomData->VertexColors.resize(VertexColorIndices.size());
							for (int PointIdx = 0; PointIdx < GeomData->Points.size(); ++PointIdx)
							{
								PXR_NAMESPACE::GfVec3f Color = DisplayColorArray[VertexColorIndices[PointIdx]];
								GeomData->VertexColors[PointIdx] = FUsdVectorData(Color[0], Color[1], Color[2]);
							}
						}
						else
						{
							// Assume mapping is identical
							GeomData->VertexColors.resize(DisplayColorArray.size());
							memcpy(&GeomData->VertexColors[0], &DisplayColorArray[0], DisplayColorArray.size() * sizeof(PXR_NAMESPACE::GfVec3f));
						}
					}
					else if (Interpolation == PXR_NAMESPACE::UsdGeomTokens->constant)
					{
						PXR_NAMESPACE::VtArray<PXR_NAMESPACE::GfVec3f> DisplayColorArray;
						DisplayColorPrimVar.Get(&DisplayColorArray, Time);

						if(DisplayColorArray.size() > 0)
						{
							PXR_NAMESPACE::GfVec3f Color = DisplayColorArray[0];
							GeomData->VertexColors.push_back(FUsdVectorData(Color[0], Color[1], Color[2]));
						}
					}
				}

				GeomData->Orientation = EUsdGeomOrientation::RightHanded;
				PXR_NAMESPACE::UsdAttribute Orientation = Mesh.GetOrientationAttr();
				if(Orientation)
				{ 
					static PXR_NAMESPACE::TfToken RightHanded("rightHanded");
					static PXR_NAMESPACE::TfToken LeftHanded("leftHanded");

					PXR_NAMESPACE::TfToken OrientationValue;
					Orientation.Get(&OrientationValue, Time);

					GeomData->Orientation = OrientationValue == RightHanded ? EUsdGeomOrientation::RightHanded : EUsdGeomOrientation::LeftHanded;
				}

			}

			// UVs
			{
				static PXR_NAMESPACE::TfToken UVSetName("primvars:st");

				PXR_NAMESPACE::UsdGeomPrimvar STPrimvar = Mesh.GetPrimvar(UVSetName);

				int UVIndex = GeomData->NumUVs;
				if(STPrimvar)
				{
					// We are creating one UV set
					++GeomData->NumUVs;

					GeomData->UVs[UVIndex].Coords.clear();

					if (STPrimvar.GetInterpolation() == PXR_NAMESPACE::UsdGeomTokens->faceVarying)
					{
						GeomData->UVs[UVIndex].UVInterpMethod = EUsdInterpolationMethod::FaceVarying;
						PXR_NAMESPACE::VtVec2fArray UVs;
						STPrimvar.ComputeFlattened(&UVs, Time);
						if (UVs.size() == GeomData->FaceIndices.size())
						{
							GeomData->UVs[UVIndex].Coords.resize(UVs.size());
							memcpy(&GeomData->UVs[UVIndex].Coords[0], &UVs[0], UVs.size() * sizeof(PXR_NAMESPACE::GfVec2f));
						}
					}
					else if (STPrimvar.GetInterpolation() == PXR_NAMESPACE::UsdGeomTokens->vertex)
					{
						GeomData->UVs[UVIndex].UVInterpMethod = EUsdInterpolationMethod::Vertex;
						PXR_NAMESPACE::VtIntArray UVIndices;
						PXR_NAMESPACE::VtVec2fArray UVs;
						STPrimvar.GetIndices(&UVIndices, Time);
						STPrimvar.Get(&UVs, Time);

						if (UVIndices.size() == GeomData->Points.size())
						{
							GeomData->UVs[UVIndex].Coords.resize(UVIndices.size());
							for (int PointIdx = 0; PointIdx < GeomData->Points.size(); ++PointIdx)
							{
								PXR_NAMESPACE::GfVec2f UV = UVs[UVIndices[PointIdx]];
								GeomData->UVs[UVIndex].Coords[PointIdx] = FUsdVector2Data(UV[0], UV[1]);

							}
						}
					}
				}
			}

			// Material mappings
			// @todo time not supported yet
			if(GeomData->FaceMaterialIndices.size() == 0)
			{
				vector<PXR_NAMESPACE::UsdGeomFaceSetAPI> FaceSets = PXR_NAMESPACE::UsdGeomFaceSetAPI::GetFaceSets(Prim);

				GeomData->FaceMaterialIndices.resize(GeomData->FaceVertexCounts.size());
				memset(&GeomData->FaceMaterialIndices[0], 0, sizeof(int)*GeomData->FaceMaterialIndices.size());
			
				// Figure out a zero based mateiral index for each face.  The mapping is FaceMaterialIndices[FaceIndex] = MaterialIndex;
				// This is done by walking the face sets and for each face set getting the number number of unique groups of faces in the set
				// Each one of these groups represents a material index for that face set.  If there are multiple face sets the material index is offset by the face set index
				// Once the groups of faces are determined, walk the indices for the total number of faces in each group.  Each element in the face indices array represents a single global face index
				// Assign the current material index to it

				// @todo USD/Unreal.  This is probably wrong for multiple face sets.  They don't make a ton of sense for unreal as there can only be one "set" of materials at once and there is no construct in the engine for material sets
			
				//GeomData->MaterialNames.resize(FaceSets)
				for (int FaceSetIdx = 0; FaceSetIdx < FaceSets.size(); ++FaceSetIdx)
				{
					const PXR_NAMESPACE::UsdGeomFaceSetAPI& FaceSet = FaceSets[FaceSetIdx];
					
					PXR_NAMESPACE::SdfPathVector BindingTargets;
					FaceSet.GetBindingTargets(&BindingTargets);

					
					PXR_NAMESPACE::UsdStageWeakPtr Stage = Prim.GetStage();
					for(const PXR_NAMESPACE::SdfPath& Path : BindingTargets)
					{
						// load each material at the material path; 
						PXR_NAMESPACE::UsdPrim MaterialPrim = Stage->Load(Path);

						// Default to using the prim path name as the path for this material in Unreal
						std::string MaterialName = MaterialPrim.GetName().GetString();

						// See if the material has an "unrealAssetPath" attribute.  This should be the full name of the material
						static const PXR_NAMESPACE::TfToken AssetPathToken = PXR_NAMESPACE::TfToken(UnrealIdentifiers::AssetPath);
						PXR_NAMESPACE::UsdAttribute UnrealAssetPathAttr = MaterialPrim.GetAttribute(AssetPathToken);
						if (UnrealAssetPathAttr.HasValue())
						{
							UnrealAssetPathAttr.Get(&MaterialName);
						}

						GeomData->MaterialNames.push_back(MaterialName);
					}
					// Faces must be mutually exclusive
					if (FaceSet.GetIsPartition())
					{
						// Get the list of faces in the face set.  The size of this list determines the number of materials in this set
						PXR_NAMESPACE::VtIntArray FaceCounts;
						FaceSet.GetFaceCounts(&FaceCounts, Time);

						// Get the list of global face indices mapped in this set
						PXR_NAMESPACE::VtIntArray FaceIndices;
						FaceSet.GetFaceIndices(&FaceIndices, Time);

						// How far we are into the face indices list
						int Offset = 0;

						// Walk each face group in the set
						for (int FaceCountIdx = 0; FaceCountIdx < FaceCounts.size(); ++FaceCountIdx)
						{
							int MaterialIdx = FaceSetIdx * FaceSets.size() + FaceCountIdx;


							// Number of faces with the material index
							int FaceCount = FaceCounts[FaceCountIdx];

							// Walk each face and map it to the computed material index
							for (int FaceNum = 0; FaceNum < FaceCount; ++FaceNum)
							{
								int Face = FaceIndices[FaceNum + Offset];
								GeomData->FaceMaterialIndices[Face] = MaterialIdx;
							}
							Offset += FaceCount;
						}
					}
				}
			}

			// SubD
			{
				GeomData->SubdivisionScheme = EUsdSubdivisionScheme::CatmullClark;
				PXR_NAMESPACE::UsdAttribute SubDivScheme = Mesh.GetSubdivisionSchemeAttr();
				if (SubDivScheme)
				{
					static const PXR_NAMESPACE::TfToken CatmullClark("catmullClark");
					static const PXR_NAMESPACE::TfToken Loop("loop");
					static const PXR_NAMESPACE::TfToken Bilinear("bilinear");
					static const PXR_NAMESPACE::TfToken None("none");
					PXR_NAMESPACE::TfToken SchemeName;
					SubDivScheme.Get(&SchemeName, Time);

					if (SchemeName == CatmullClark)
					{
						GeomData->SubdivisionScheme = EUsdSubdivisionScheme::CatmullClark;
					}
					else if (SchemeName == Loop)
					{
						GeomData->SubdivisionScheme = EUsdSubdivisionScheme::Loop;
					}
					else if (SchemeName == Bilinear)
					{
						GeomData->SubdivisionScheme = EUsdSubdivisionScheme::Bilinear;
					}
					else if (SchemeName == None)
					{
						GeomData->SubdivisionScheme = EUsdSubdivisionScheme::None;
					}

				}
				PXR_NAMESPACE::UsdAttribute CreaseIndices = Mesh.GetCreaseIndicesAttr();
				if (CreaseIndices)
				{
					GeomData->CreaseIndices.clear();

					PXR_NAMESPACE::VtIntArray CreaseIndicesArray;
					CreaseIndices.Get(&CreaseIndicesArray, Time);
					GeomData->CreaseIndices.assign(CreaseIndicesArray.begin(), CreaseIndicesArray.end());
				}

				PXR_NAMESPACE::UsdAttribute CreaseLengths = Mesh.GetCreaseLengthsAttr();
				if (CreaseLengths)
				{
					GeomData->CreaseLengths.clear();

					PXR_NAMESPACE::VtIntArray CreaseLengthsArray;
					CreaseLengths.Get(&CreaseLengthsArray);
					GeomData->CreaseLengths.assign(CreaseLengthsArray.begin(), CreaseLengthsArray.end());
				}

				PXR_NAMESPACE::UsdAttribute CreaseSharpnesses = Mesh.GetCreaseSharpnessesAttr();
				if (CreaseSharpnesses)
				{
					GeomData->CreaseSharpnesses.clear();

					PXR_NAMESPACE::VtFloatArray CreaseSharpnessesArray;
					CreaseSharpnesses.Get(&CreaseSharpnessesArray);
					GeomData->CreaseSharpnesses.assign(CreaseSharpnessesArray.begin(), CreaseSharpnessesArray.end());
				}

				PXR_NAMESPACE::UsdAttribute CornerCreaseIndices = Mesh.GetCornerIndicesAttr();
				if (CornerCreaseIndices)
				{
					GeomData->CornerCreaseIndices.clear();

					PXR_NAMESPACE::VtIntArray CornerCreaseIndicesArray;
					CornerCreaseIndices.Get(&CornerCreaseIndicesArray);
					GeomData->CornerCreaseIndices.assign(CornerCreaseIndicesArray.begin(), CornerCreaseIndicesArray.end());
				}
				
				PXR_NAMESPACE::UsdAttribute CornerSharpnesses = Mesh.GetCornerSharpnessesAttr();
				if (CornerSharpnesses)
				{
					GeomData->CornerSharpnesses.clear();

					PXR_NAMESPACE::VtFloatArray CornerSharpnessesArray;
					CornerSharpnesses.Get(&CornerSharpnessesArray);
					GeomData->CornerSharpnesses.assign(CornerSharpnessesArray.begin(), CornerSharpnessesArray.end());
				}

			}

		}

		return GeomData;
	}

	virtual int GetNumLODs() const override
	{
		// 0 indicates no variant or no lods in variant. 
		int NumLODs = 0;
		if (Prim.HasVariantSets())
		{
			PXR_NAMESPACE::UsdVariantSet LODVariantSet = Prim.GetVariantSet(UnrealIdentifiers::LOD);
			if(LODVariantSet.IsValid())
			{
				vector<string> VariantNames = LODVariantSet.GetVariantNames();
				NumLODs = VariantNames.size();
			}
		}

		return NumLODs;
	}

	virtual IUsdPrim* GetLODChild(int LODIndex) override
	{
		if (Prim.HasVariantSets())
		{
			PXR_NAMESPACE::UsdVariantSet LODVariantSet = Prim.GetVariantSet(UnrealIdentifiers::LOD);
			if (LODVariantSet.IsValid())
			{
				char LODName[10];
#if _WINDOWS
				sprintf_s(LODName, "LOD%d", LODIndex);
#else
				sprintf(LODName, "LOD%d", LODIndex);
#endif
				LODVariantSet.SetVariantSelection(string(LODName));

				PXR_NAMESPACE::UsdPrim LODChild = Prim.GetChild(PXR_NAMESPACE::TfToken(LODName));
				if (LODChild)
				{
					
					auto Result = std::find_if(
						std::begin(VariantData),
						std::end(VariantData), 
						[&LODChild](const FPrimAndData& Elem)
						{
							return Elem.Prim == LODChild;
						});

					if (Result != std::end(VariantData))
					{
						FUsdPrim* LODChildData = new FUsdPrim(LODChild);
						FPrimAndData LODData(LODChild);
						LODData.PrimData = LODChildData;

						VariantData.push_back(LODData);
						return LODData.PrimData;
					}
					else
					{
						return Result->PrimData;
					}
			
				}
			}
		}

		return nullptr;
	}
private:
	PXR_NAMESPACE::UsdPrim Prim;
	vector<FPrimAndData> Children;
	vector<FPrimAndData> VariantData;
	string PrimName;
	string PrimPath;
	string UnrealAssetPath;
	FUsdGeomData* GeomData;
};

class FUsdStage : public IUsdStage
{
public:
	FUsdStage(PXR_NAMESPACE::UsdStageRefPtr& InStage)
		: Stage(InStage)
		, RootPrim(nullptr)
	{
	}

	~FUsdStage()
	{
		if (RootPrim)
		{
			delete RootPrim;
		}
	}

	EUsdUpAxis GetUpAxis() const override
	{
		// note USD does not support X up
		return UsdGeomGetStageUpAxis(Stage) == PXR_NAMESPACE::UsdGeomTokens->y ? EUsdUpAxis::YAxis : EUsdUpAxis::ZAxis;
	}

	IUsdPrim* GetRootPrim() override
	{
		if (Stage && !RootPrim)
		{
			RootPrim = new FUsdPrim(Stage->GetPseudoRoot());
		}

		return RootPrim;
	}

	virtual bool HasAuthoredTimeCodeRange() const override
	{
		return Stage->HasAuthoredTimeCodeRange();
	}

	virtual double GetStartTimeCode() const override
	{
		return Stage->GetStartTimeCode();
	}

	virtual double GetEndTimeCode() const override
	{
		return Stage->GetEndTimeCode();
	}

	virtual double GetFramesPerSecond() const override
	{
		return Stage->GetFramesPerSecond();
	}

	virtual double GetTimeCodesPerSecond() const override
	{
		return Stage->GetTimeCodesPerSecond();
	}

private:
	PXR_NAMESPACE::UsdStageRefPtr Stage;
	FUsdPrim* RootPrim;
};

bool UnrealUSDWrapper::bInitialized = false;
FUsdStage* UnrealUSDWrapper::CurrentStage = nullptr;



void UnrealUSDWrapper::Initialize(const char* PathToModule)
{
	bInitialized = true;

	string PluginPath = PathToModule;
#ifdef _WINDOWS
	PluginPath += "/Resources/UsdResources/plugins";
#elif defined(__linux__)
	PluginPath += "/Resources/UsdResources/Linux/plugins";
#endif

	// Needed to find plugins in non-standard places
	PXR_NAMESPACE::PlugRegistry::GetInstance().RegisterPlugins(PluginPath);
}

IUsdStage* UnrealUSDWrapper::ImportUSDFile(const char* Path, const char* Filename)
{
	if (!bInitialized)
	{
		return nullptr;
	}

	bool bImportedSuccessfully = false;

	// Clean up any old data
	CleanUp();

	PXR_NAMESPACE::TfErrorMark ErrorMark;

	string PathAndFilename = string(Path) + string(Filename);

	bool bIsSupported = PXR_NAMESPACE::UsdStage::IsSupportedFile(PathAndFilename);

	PXR_NAMESPACE::UsdStageRefPtr Stage = PXR_NAMESPACE::UsdStage::Open(PathAndFilename);

	if(Stage)
	{
		CurrentStage = new FUsdStage(Stage);


		//USDHelpers::LogPrimTree(Stage->GetPseudoRoot());
	}


	if (!ErrorMark.IsClean())
	{
		PXR_NAMESPACE::TfErrorMark::Iterator i;
		for (i = ErrorMark.GetBegin(); i != ErrorMark.GetEnd(); ++i)
		{
			Log("%s %s\n", i->GetErrorCodeAsString().c_str(), i->GetCommentary().c_str());
		}
	}

	return CurrentStage;
}

void UnrealUSDWrapper::CleanUp()
{
	if (CurrentStage)
	{
#if USDWRAPPER_USE_XFORMACHE
		XFormCache.Clear();
#endif // USDWRAPPER_USE_XFORMACHE

		delete CurrentStage;
		CurrentStage = nullptr;
	}
}

double UnrealUSDWrapper::GetDefaultTimeCode()
{
	return PXR_NAMESPACE::UsdTimeCode::Default().GetValue();
}
