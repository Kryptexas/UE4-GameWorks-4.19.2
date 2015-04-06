/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.

#include "GuEPAFacet.h"
#include "GuEPA.h"
#include "PsAllocator.h"
#include "GuVecSphere.h"
#include "GuVecBox.h"
#include "GuVecCapsule.h"
#include "GuVecConvexHull.h"
#include "GuGJKSimplex.h"


namespace physx
{
namespace Gu
{


	PxGJKStatus epaPenetration(const ConvexV& a, const ConvexV& b, EPASupportMapPair* pair, PxU8* PX_RESTRICT aInd, PxU8* PX_RESTRICT bInd, PxU8 _size, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth, const bool takeCoreShape)
	{
		using namespace Ps::aos;
		const BoolV bTrue = BTTTT();
		const Vec3V zeroV = V3Zero();

		PxU32 size=_size;
		const FloatV minMargin = FMin(a.getMinMargin(), b.getMinMargin());

		//ML: eps2 is the square value of an epsilon value which applied in the termination condition for two shapes overlap. EPA will terminate based on sq(v) < eps2 and indicate that two shapes are overlap. 
		//we calculate the eps2 based on 10% of the minimum margin of two shapes
		const FloatV eps = FMul(minMargin, FLoad(0.1f));
		const FloatV eps2 = FMul(eps, eps);

		Vec3V A[4];
		Vec3V B[4];
		Vec3V Q[4];
		Vec3V v(zeroV);
		Vec3V closA(zeroV), closB(zeroV);
		BoolV bNotTerminated  = bTrue;
		BoolV bCon = bTrue;
		FloatV sDist = FMax();
		FloatV minDist= sDist;
  
		if(_size != 0)
		{
			//ML: we constructed a simplex based on the gjk simplex indices
			for(PxU32 i=0; i<_size; ++i)
			{
				pair->doWarmStartSupport(aInd[i], bInd[i], A[i], B[i], Q[i]);
			}

			//ML: if any of the gjk shape is shrunk shape, we need to recreate the simplex for the original shape to work on; otherwise, we can just skip this process
			//and go to the epa code
			if(!takeCoreShape)
			{
				const PxU32 sSize = PxU32(_size-1);
				v = GJKCPairDoSimplex(Q, A, B, Q[sSize], A[sSize],  B[sSize], size, closA, closB);
				sDist = V3Dot(v, v);	
				bNotTerminated = FIsGrtr(sDist, eps2);
			}
			else
			{
				bNotTerminated = BFFFF();
			}
		}
		else
		{
			const Vec3V _initialSearchDir = pair->getDir();
			v = V3Sel(FIsGrtr(V3Dot(_initialSearchDir, _initialSearchDir), FZero()), _initialSearchDir, V3UnitX());
			v = V3UnitX();
		}

	
		while(BAllEq(bNotTerminated, bTrue))
		{
			minDist = sDist;
			//calculate the support point
			pair->doSupport(v, A[size], B[size], Q[size]);
		
			const PxU32 index = size++;
			
			v = GJKCPairDoSimplex(Q, A, B, Q[index], A[index], B[index], size, closA, closB);
			sDist = V3Dot(v, v);
			bCon = FIsGrtr(minDist, sDist);
			bNotTerminated = BAnd(FIsGrtr(sDist, eps2), bCon);
		}

		EPA epa;

		return epa.PenetrationDepth(a, b, pair, Q, A, B, (PxI32)size, contactA, contactB, normal, penetrationDepth, takeCoreShape);
	}

	//PX_EPA_FORCE_INLINE Ps::aos::BoolV Facet::isValid2(const PxU32 i0, const PxU32 i1, const PxU32 i2, const Ps::aos::Vec3V* PX_RESTRICT aBuf, const Ps::aos::Vec3V* PX_RESTRICT bBuf, 
	//	const Ps::aos::FloatVArg lower, const Ps::aos::FloatVArg upper, PxI32& b1)
	//{
	//	using namespace Ps::aos;
	//	const FloatV zero = FNeg(FEps());

	//	const Vec3V pa0(aBuf[i0]);
	//	const Vec3V pa1(aBuf[i1]);
	//	const Vec3V pa2(aBuf[i2]);

	//	const Vec3V pb0(bBuf[i0]);
	//	const Vec3V pb1(bBuf[i1]);
	//	const Vec3V pb2(bBuf[i2]);

	//	const Vec3V p0 = V3Sub(pa0, pb0);
	//	const Vec3V p1 = V3Sub(pa1, pb1);
	//	const Vec3V p2 = V3Sub(pa2, pb2);

	//	const Vec3V v1 = V3Sub(p1, p0);
	//	const Vec3V v2 = V3Sub(p2, p0);

	//	const Vec3V pNor = V3Cross(v1, v2);
	//	const FloatV pDist =  V3Dot(pNor, p0);
	//	const FloatV v1dv1 = V3Dot(v1, v1);
	//	const FloatV v1dv2 = V3Dot(v1, v2);
	//	const FloatV v2dv2 = V3Dot(v2, v2);
	//	
	//	//m_planeN = pNor;
	//	PxF32_From_FloatV(pDist, &m_planeD);
	//	PxVec3_From_Vec3V(pNor, m_planeN);

	//	const FloatV p0dv1 = V3Dot(p0, v1); //V3Dot(V3Sub(p0, origin), v1);
	//	const FloatV p0dv2 = V3Dot(p0, v2);

	//	const FloatV det = FNegMulSub(v1dv2, v1dv2, FMul(v1dv1, v2dv2));//FSub( FMul(v1dv1, v2dv2), FMul(v1dv2, v1dv2) ); // non-negative
	//	const FloatV recip = FRecip(det);

	//	const FloatV lambda1 = FNegMulSub(p0dv1, v2dv2, FMul(p0dv2, v1dv2));
	//	const FloatV lambda2 = FNegMulSub(p0dv2, v1dv1, FMul(p0dv1, v1dv2));
	//	
	//	//calculate the closest point and sqdist
	//	const Vec3V a = V3ScaleAdd(v1, lambda1, V3Scale(v2, lambda2));
	//	const Vec3V closest = V3ScaleAdd(a, recip, p0);
	//	const FloatV dist = V3Dot(closest,closest);

	//	PxF32_From_FloatV(dist, PX_FPTR(&m_UDist));
	//	PxVec3_From_Vec3V(closest, m_closest);
	//	//PxF32_From_FloatV(dist, &m_dist);

	//	b1= FAllGrtrOrEq(det, zero);
	//	
	//	const FloatV sumLambda = FAdd(lambda1, lambda2);
	//	const BoolV b2 = BAnd(FIsGrtr(lambda1,zero), BAnd(FIsGrtr(lambda2, zero), FIsGrtr(det, sumLambda)));

	//	//m_closest=closest;
	//	PxF32_From_FloatV(FMul(lambda1, recip), &m_lambda1);
	//	PxF32_From_FloatV(FMul(lambda2, recip), &m_lambda2);
	//	//PxF32_From_FloatV(recip, &m_recipDet);

	//	
	//	return BAnd(b2, BAnd(FIsGrtrOrEq(dist, lower), FIsGrtrOrEq(upper, dist)));
	//	
	//}

	PX_EPA_FORCE_INLINE Ps::aos::BoolV Facet::isValid2(const PxU32 i0, const PxU32 i1, const PxU32 i2, const Ps::aos::Vec3V* PX_RESTRICT aBuf, const Ps::aos::Vec3V* PX_RESTRICT bBuf, 
		const Ps::aos::FloatVArg lower, const Ps::aos::FloatVArg upper, PxI32& b1)
	{
		using namespace Ps::aos;
		const FloatV zero = FNeg(FEps());

		const Vec3V pa0(aBuf[i0]);
		const Vec3V pa1(aBuf[i1]);
		const Vec3V pa2(aBuf[i2]);

		const Vec3V pb0(bBuf[i0]);
		const Vec3V pb1(bBuf[i1]);
		const Vec3V pb2(bBuf[i2]);

		const Vec3V p0 = V3Sub(pa0, pb0);
		const Vec3V p1 = V3Sub(pa1, pb1);
		const Vec3V p2 = V3Sub(pa2, pb2);

		const Vec3V v1 = V3Sub(p1, p0);
		const Vec3V v2 = V3Sub(p2, p0);

		const Vec3V pNor = V3Cross(v1, v2);

		/*const FloatV pDist =  V3Dot(pNor, p0);
		const FloatV v1dv1 = V3Dot(v1, v1);
		const FloatV v1dv2 = V3Dot(v1, v2);
		const FloatV v2dv2 = V3Dot(v2, v2);*/
		const Vec4V results = V3Dot4(pNor, p0, v1, v1, v1, v2, v2, v2);
		const FloatV pDist = V4GetX(results);
		const FloatV v1dv1 = V4GetY(results);
		const FloatV v1dv2 = V4GetZ(results);
		const FloatV v2dv2 = V4GetW(results);

		//m_planeN = pNor;
		FStore(pDist, &m_planeD);
		V3StoreU(pNor, m_planeN);

		const FloatV p0dv1 = V3Dot(p0, v1); //V3Dot(V3Sub(p0, origin), v1);
		const FloatV p0dv2 = V3Dot(p0, v2);

		const FloatV det = FNegMulSub(v1dv2, v1dv2, FMul(v1dv1, v2dv2));//FSub( FMul(v1dv1, v2dv2), FMul(v1dv2, v1dv2) ); // non-negative
		const FloatV recip = FRecip(det);

		const FloatV lambda1 = FNegMulSub(p0dv1, v2dv2, FMul(p0dv2, v1dv2));
		const FloatV lambda2 = FNegMulSub(p0dv2, v1dv1, FMul(p0dv1, v1dv2));
		
		//calculate the closest point and sqdist
		const Vec3V a = V3ScaleAdd(v1, lambda1, V3Scale(v2, lambda2));
		const Vec3V closest = V3ScaleAdd(a, recip, p0);
		const FloatV dist = V3Dot(closest,closest);

		FStore(dist, PX_FPTR(&m_UDist));
		V3StoreU(closest, m_closest);
		//PxF32_From_FloatV(dist, &m_dist);

		b1= (PxI32)FAllGrtrOrEq(det, zero);
		
		const FloatV sumLambda = FAdd(lambda1, lambda2);
		const BoolV b2 = BAnd(FIsGrtr(lambda1,zero), BAnd(FIsGrtr(lambda2, zero), FIsGrtr(det, sumLambda)));

		//m_closest=closest;
		FStore(FMul(lambda1, recip), &m_lambda1);
		FStore(FMul(lambda2, recip), &m_lambda2);
		//PxF32_From_FloatV(recip, &m_recipDet);

		
		return BAnd(b2, BAnd(FIsGrtrOrEq(dist, lower), FIsGrtrOrEq(upper, dist)));
		
	}


	Facet* EPA::addFacet(const PxU32 i0, const PxU32 i1, const PxU32 i2, const Ps::aos::FloatVArg lower2, const Ps::aos::FloatVArg upper2)
	{
		using namespace Ps::aos;
		const BoolV bTrue = BTTTT();
		PX_ASSERT(i0 != i1 && i0 != i2 && i1 != i2);
		if(facetManager.getNumFacets() < MaxFacets)
		{
			PxU8 facetId = facetManager.getNewID();
			Ps::prefetchLine(&facetBuf[facetId], 128);

			Facet * PX_RESTRICT facet = PX_PLACEMENT_NEW(&facetBuf[facetId],Facet(i0, i1, i2));
			facet->m_FacetId = facetId;
			PxI32 b1;
			/*const BoolV b2 = facet->isValid(i0, i1, i2, aBuf, bBuf, b1);
			const BoolV con = BAnd(b2, BAnd(FIsGrtrOrEq(facet->m_dist, lower2), FIsGrtrOrEq(upper2, facet->m_dist)));*/

			const BoolV con = facet->isValid2(i0, i1, i2, aBuf, bBuf, lower2, upper2, b1);

			if(BAllEq(con, bTrue))
			{
				heap.insert(facet);
				facet->m_inHeap = true;
			}
			else
			{
				facet->m_inHeap = false;
			}
			return b1!=0 ? facet: NULL;
		}
		return NULL;

	}


	void Facet::silhouette(const PxU32 _index, const Ps::aos::Vec3VArg w, EdgeBuffer& edgeBuffer, EPAFacetManager& manager) 
	{
		using namespace Ps::aos;
		Edge stack[MaxFacets];
		PxI32 size = 1;
		Facet* next_facet = this;
		PxI32 next_index = (PxI32)_index;
		while(size--)
		{
			Facet* const PX_RESTRICT f = next_facet;
			const PxU32 index = (PxU32)next_index;
			PX_ASSERT(f->Valid());
			if(f->m_obsolete)
			{ 
				const Edge& next_e = stack[ PxMax(size-1,0) ];
				next_facet = next_e.m_facet;
				next_index = (PxI32)next_e.m_index;
			}
			else
			{
				const FloatV vw = V3Dot(f->getClosest(), w);
				//if (V3Dot(f->m_closest, w) < f->m_dist) //the facet is not visible from w
				if(FAllGrtr(f->getDist(), vw))
				{
					edgeBuffer.Insert(f, index);
					const Edge& next_e = stack[ PxMax(size-1,0) ];
					next_facet = next_e.m_facet;
					next_index = (PxI32)next_e.m_index;
				} 
				else 
				{
					f->m_obsolete = true; // Facet is visible from w
					const PxU32 next(incMod3(index));
					const PxU32 next2(incMod3(next));
					stack[size].m_facet = f->m_adjFacets[next2];
					stack[size].m_index = PxU32(f->m_adjEdges[next2]);
					size += 2;
					PX_ASSERT(size <= MaxFacets);
					next_facet = f->m_adjFacets[next];
					next_index = f->m_adjEdges[next];
					if(!f->m_inHeap)
					{
						manager.deferFreeID(f->m_FacetId);
					}
				}
			}
		}
	}

	void Facet::silhouette(const Ps::aos::Vec3VArg w, EdgeBuffer& edgeBuffer, EPAFacetManager& manager)
	{
		m_obsolete = true;
		for(PxU32 a = 0; a < 3; ++a)
		{
			m_adjFacets[a]->silhouette((PxU32)m_adjEdges[a], w, edgeBuffer, manager);
		}
	}



	//bool EPA::expandSegment(const ConvexV&, const ConvexV&, EPASupportMapPair* pair)
	//{
	//	using namespace Ps::aos;
	//	const FloatV zero = FZero();
	//	const FloatV _max = FMax();

	//	const FloatV sinHalfTheta0 = FLoad(0.70710678119f);//sin((45), rotate 90 degree

	//	const Vec3V q0 = V3Sub(aBuf[0], bBuf[0]);
	//	const Vec3V q1 = V3Sub(aBuf[1], bBuf[1]);

	//	const Vec3V dir = V3Normalize(V3Sub(q1, q0));
	//
	//	const FloatV sDir = V3Dot(dir, dir);
	//	const Vec3V temp2 = V3Splat(sDir);
	//	const Vec3V t1 = V3Normalize(V3Cross(temp2, dir));
	//	const Vec3V aux1 = V3Cross(dir, t1);

	//	Vec3V p1;
	//	pair->doSupport(aux1, aBuf[2], bBuf[2], p1);
	//	
	//	const Vec3V imag0 = V3Scale(dir, sinHalfTheta0);
	//	const QuatV qua0 = V4SetW(Vec4V_From_Vec3V(imag0), sinHalfTheta0);
	//	
	//	const Vec3V aux2 = QuatRotate(qua0, aux1);//aux1 * qua0;
	//	Vec3V p2;
	//	pair->doSupport(aux2, aBuf[3], bBuf[3], p2);
	//	
	//	const Vec3V aux3 = QuatRotate(qua0, aux2);//(aux2 * qua0);
	//
	//	Vec3V p3;
	//	pair->doSupport(aux3, aBuf[4], bBuf[4], p3);

	//	const Vec3V aux4 = QuatRotate(qua0, aux3);
	//	
	//	Vec3V p4;
	//	pair->doSupport(aux4, aBuf[5], bBuf[5], p4);

	//	Facet * PX_RESTRICT f0 = addFacet(2, 0, 5, zero, _max);
	//	Facet * PX_RESTRICT f1 = addFacet(3, 0, 2, zero, _max);
	//	Facet * PX_RESTRICT f2 = addFacet(4, 0, 3, zero, _max);
	//	Facet * PX_RESTRICT f3 = addFacet(5, 0, 4, zero, _max);
	//	Facet * PX_RESTRICT f4 = addFacet(2, 1, 3, zero, _max);
	//	Facet * PX_RESTRICT f5 = addFacet(3, 1, 4, zero, _max);
	//	Facet * PX_RESTRICT f6 = addFacet(4, 1, 5, zero, _max);
	//	Facet * PX_RESTRICT f7 = addFacet(5, 1, 2, zero, _max);

	//	if( (f0== NULL) | (f1 == NULL)| (f2 == NULL) | (f3 == NULL) | (f4 == NULL) | (f5 == NULL ) | (f6 == NULL) | (f7 == NULL) | heap.isEmpty())
	//	{
	//		return false;
	//	}

	//	f0->link(0, f1, 1);
	//	f0->link(1, f3, 0);
	//	f0->link(2, f7, 2);
	//	f1->link(0, f2, 1);
	//	f1->link(2, f4, 2);
	//	f2->link(0, f3, 1);
	//	f2->link(2, f5, 2);
	//	f3->link(2, f6, 2);
	//	f4->link(0, f7, 1);
	//	f4->link(1, f5, 0);
	//	f5->link(1, f6, 0);
	//	f6->link(1, f7, 0);

	//	//num_verts = 6;

	//	return true;
	//}

	bool EPA::expandSegment(const ConvexV& a, const ConvexV& b, EPASupportMapPair* pair, PxI32& numVerts)
	{
		using namespace Ps::aos;
		//const FloatV zero = FZero();
		//const FloatV _max = FMax();

		const FloatV sinTheta0 = FLoad(0.8660254037844386f);//sin(120), rotate 120 degree
		const FloatV cosTheta0 = FLoad(-0.5f);//cos(120)
		
	
		const Vec3V q3 = V3Sub(aBuf[0], bBuf[0]);
		const Vec3V q4 = V3Sub(aBuf[1], bBuf[1]);

		const Vec3V dir = V3Normalize(V3Sub(q3, q4));
	
		const FloatV sDir = V3Dot(dir, dir);
		const Vec3V temp2 = V3Splat(sDir);
		const Vec3V t1 = V3Normalize(V3Cross(temp2, dir));
		const Vec3V aux1 = V3Cross(dir, t1);

		Vec3V q0;
   		pair->doSupport(aux1, aBuf[0], bBuf[0], q0);
		
		const Vec3V imag0 = V3Scale(dir, sinTheta0);
		const QuatV qua0 = V4SetW(Vec4V_From_Vec3V(imag0), cosTheta0);
		
		const Vec3V aux2 = V3Normalize(QuatRotate(qua0, aux1));//aux1 * qua0;
		Vec3V q1;
		pair->doSupport(aux2, aBuf[1], bBuf[1], q1);
		
		const Vec3V aux3 = V3Normalize(QuatRotate(qua0, aux2));//(aux2 * qua0);
	
		Vec3V q2;
		pair->doSupport(aux3, aBuf[2], bBuf[2], q2);

		return expandTriangle(a, b, pair, numVerts);
	}


	bool EPA::addInitialFacet4()	
	{
		using namespace Ps::aos;
		const FloatV zero = FZero();
		const FloatV _max = FMax();

		Facet * PX_RESTRICT f0 = addFacet(0, 1, 2, zero, _max);
		Facet * PX_RESTRICT f1 = addFacet(0, 3, 1, zero, _max);
		Facet * PX_RESTRICT f2 = addFacet(0, 2, 3, zero, _max);
		Facet * PX_RESTRICT f3 = addFacet(1, 3, 2, zero, _max);

		if((f0==NULL) || (f1==NULL) || (f2==NULL) || (f3==NULL) || heap.isEmpty())
			return false;
	
		f0->link(0, f1, 2);
		f0->link(1, f3, 2);
		f0->link(2, f2, 0);
		f1->link(0, f2, 2);
		f1->link(1, f3, 0);
		f2->link(1, f3, 1);

		return true;
	}



	bool EPA::addInitialFacet5()
	{
		using namespace Ps::aos;
		const FloatV zero = FZero();
		const FloatV _max = FMax();

		Facet * PX_RESTRICT f0 = addFacet(0, 3, 2, zero, _max);
		Facet * PX_RESTRICT f1 = addFacet(1, 3, 0, zero, _max);
		Facet * PX_RESTRICT f2 = addFacet(2, 3, 1, zero, _max);
		Facet * PX_RESTRICT f3 = addFacet(2, 4, 0, zero, _max);
		Facet * PX_RESTRICT f4 = addFacet(0, 4, 1, zero, _max);
		Facet * PX_RESTRICT f5 = addFacet(1, 4, 2, zero, _max);

		if((f0==NULL) || (f1==NULL) || (f2==NULL) || (f3==NULL) || (f4==NULL) || (f5==NULL) || heap.isEmpty())
			return false;
		
		f0->link(0, f1, 1);
		f0->link(1, f2, 0);
		f0->link(2, f3, 2);
		f1->link(0, f2, 1);
		f1->link(2, f4, 2);
		f2->link(2, f5, 2);
		f3->link(0, f5, 1);
		f3->link(1, f4, 0);
		f4->link(1, f5, 0);

		return true;
	}

	//static void  printVerts(const Ps::aos::Vec3VArg q0, const Ps::aos::Vec3VArg q1, const Ps::aos::Vec3VArg q2, const Ps::aos::Vec3VArg q3)
	//{
	//	using namespace Ps::aos;
	//	PxVec3 t0, t1, t2, t3;
	//	PxVec3_From_Vec3V(q0, t0);
	//	PxVec3_From_Vec3V(q1, t1);
	//	PxVec3_From_Vec3V(q2, t2);
	//	PxVec3_From_Vec3V(q3, t3);
	//	printf("PxVec3 q0(%f, %f, %f);\n", t0.x, t0.y, t0.z);
	//	printf("PxVec3 q1(%f, %f, %f);\n", t1.x, t1.y, t1.z);
	//	printf("PxVec3 q2(%f, %f, %f);\n", t2.x, t2.y, t2.z);
	//	printf("PxVec3 q3(%f, %f, %f);\n", t3.x, t3.y, t3.z);
	//}

	bool EPA::expand(const Ps::aos::Vec3VArg q0, const Ps::aos::Vec3VArg q1, const Ps::aos::Vec3VArg q2, EPASupportMapPair* pair,  PxI32& numVerts)
	{
		using namespace Ps::aos;
		const FloatV zero = FZero();
		const FloatV eps = FEps();
		const BoolV bTrue = BTTTT();
		const Vec3V v0 = V3Sub(q1, q0);
		const Vec3V v1 = V3Sub(q2, q0);
		const Vec3V n = V3Normalize(V3Cross(v0,v1));
		const Vec3V nn = V3Neg(n);

		Vec3V a3, b3,q3;
		pair->doSupport(n, a3, b3, q3);
		//printVerts(q0, q1, q2, q3);
		const FloatV d = V3Dot(n, q0);

		//origin distance to the plane
		const FloatV dist0 = FNeg(d);//V3Dot(n, V3Sub(origin, q0));

		const FloatV dist3 = FSub(V3Dot(n, q3), d);
		//check to see whether the p3 is at the same side of origin
		const FloatV temp3 = FMul(dist3, dist0);
		const BoolV con3 = BAnd(FIsGrtrOrEq(FAbs(dist3), eps), FIsGrtr(temp3, zero));
		if(BAllEq(con3, bTrue))
		//if(FAllGrtrOrEq(FAbs(dist3), eps))
		{
			if(originInTetrahedron(q0, q1, q2, q3))
			{
				PX_ASSERT(FAllGrtr(temp3, zero) != 0);
				aBuf[3] = a3;
				bBuf[3] = b3;
				if(addInitialFacet4())
				{
					
					numVerts = 4;
					return true;
				}
				else
					return false;
			}
		}
		
		Vec3V a4, b4, q4;
		pair->doSupport(nn, a4, b4, q4);
		//printVerts(q0, q1, q2, q4);
		const FloatV dist4 = FSub(V3Dot(n, q4), d);
		//check to see whether the p4 is at the same side of origin
		const FloatV temp4 = FMul(dist4, dist0);
		const BoolV con4 = BAnd(FIsGrtrOrEq(FAbs(dist4), eps), FIsGrtr(temp4, zero));
		if(BAllEq(con4, bTrue))
		//if(FAllGrtrOrEq(FAbs(dist4), eps))
		{
			if(originInTetrahedron(q0, q1, q2, q4))
			{
				PX_ASSERT(FAllGrtr(temp4, zero) != 0);
				aBuf[3] = a4;
				bBuf[3] = b4;
				if(addInitialFacet4())
				{	
					numVerts = 4;
					return true;
				}
				else
					return false;
			}
			return false;
		}
		return false;
	}

	
	bool EPA::expandTriangle(const ConvexV&, const ConvexV&, EPASupportMapPair* pair, PxI32& numVerts)
	{

		using namespace Ps::aos;

		const FloatV zero=FZero();
		const FloatV eps = FLoad(1e-4f);
		const BoolV bTrue = BTTTT();

		const Vec3V a0 = aBuf[0];
		const Vec3V b0 = bBuf[0];
		const Vec3V a1 = aBuf[1];
		const Vec3V b1 = bBuf[1];
		const Vec3V a2 = aBuf[2];
		const Vec3V b2 = bBuf[2];

		const Vec3V q0 = V3Sub(a0, b0);
		const Vec3V q1 = V3Sub(a1, b1);
		const Vec3V q2 = V3Sub(a2, b2);
		
		const Vec3V v1 = V3Sub(q1, q0);
		const Vec3V v2 = V3Sub(q2, q0);
		const Vec3V vv = V3Normalize(V3Cross(v1,v2));
		const Vec3V nvv = V3Neg(vv);

		const FloatV d = V3Dot(vv, q0);
		const FloatV dist0 = FNeg(d);//the distance from origin to the triangle


		Vec3V a3, b3, q3;
		pair->doSupport(vv, a3, b3, q3);
		const FloatV dist3 = FSub(V3Dot(vv, q3), d);
		const FloatV temp3 = FMul(dist3, dist0);
		const BoolV con3 = BAnd(FIsGrtrOrEq(FAbs(dist3), eps), FIsGrtrOrEq(temp3, zero));
		//ML: if origin and the new support point are in the same side of the triangle plane,
		//we can construct the tetrahedron on based on the new support point
		if(BAllEq(con3, bTrue))
		{
			if(originInTetrahedron(q0, q1, q2, q3))
			{
				aBuf[3] = a3;
				bBuf[3] = b3;
				if(addInitialFacet4())
				{
					numVerts = 4;
					return true;
				}
				return false;//fail in linking
			}
			//try triangle q0, q1, q3
			aBuf[2] = a3;
			bBuf[2] = b3;
			if(expand(q0, q1, q3, pair, numVerts))
				return true;

			aBuf[1] = a2;
			bBuf[1] = b2;
			//triangle q0, q2, q3
			if(expand(q0, q2, q3, pair, numVerts))
				return true;

			//triangle q1, q2, q3
			aBuf[0]=a1;
			bBuf[0]=b1;
			if(expand(q1, q2, q3, pair, numVerts))
				return true;
		}

		Vec3V a4, b4, q4;
		pair->doSupport(nvv, a4, b4, q4);
		const FloatV dist4 = FSub(V3Dot(vv, q4), d);
		const FloatV temp4 = FMul(dist4, dist0);

		const BoolV con4 = BAnd(FIsGrtrOrEq(FAbs(dist4), eps), FIsGrtrOrEq(temp4, zero));
		//ML: if origin and the new support point are in the same side of the triangle plane,
		//we can construct the tetrahedron on based on the new support point
		if(BAllEq(con4, bTrue))
		{
			//reset the buffer
			aBuf[0] = a0; aBuf[1] = a1; aBuf[2]= a2;
			bBuf[0] = b0; bBuf[1] = b1; bBuf[2]= b2;

			if(originInTetrahedron(q0, q1, q2, q4))
			{
				aBuf[3] = a4;
				bBuf[3] = b4;
				if(addInitialFacet4())
				{
					numVerts = 4;
					return true;
				}
				return false;
			}

			//triangle q0, q1, q4
			aBuf[2] = a4;
			bBuf[2] = b4;
			if(expand(q0, q1, q4, pair, numVerts))
				return true;

			//triangle q0, q2, q4
			aBuf[1] = a2;
			bBuf[1] = b2;
			if(expand(q0, q2, q4, pair, numVerts))
				return true;

			//triangle q1, q2, q4
			aBuf[0] = a1;
			bBuf[0] = b1;
			if(expand(q1, q2, q4, pair, numVerts))
				return true;

		}

		//ML: if origin are on the triangle plane, we can construct a 5 point polytope which contain the origin inside
		if(FAllEq(dist0, zero))
		{
			aBuf[0] = a0; aBuf[1] = a1; aBuf[2]= a2;
			bBuf[0] = b0; bBuf[1] = b1; bBuf[2]= b2;

			//origin is inside the triangle
			//Vec3V a3, b3, q3;
			//pair->doSupport(vv, a3, b3, q3);
			aBuf[3] = a3;
			bBuf[3] = b3;

			//Vec3V a4, b4, q4;
			//pair->doSupport(nvv, a4, b4, q4);
			aBuf[4] = a4;
			bBuf[4] = b4;

			if(addInitialFacet5())
			{
				numVerts = 5;
				return true;
			}
		}
		return false;
	}

	static void calculateContactInformation(const Ps::aos::Vec3VArg _pa, const Ps::aos::Vec3VArg _pb, const ConvexV& a, const ConvexV& b, Ps::aos::Vec3V& pa, Ps::aos::Vec3V& pb, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penDepth, const bool takeCoreShape)
	{
		using namespace Ps::aos;

		FloatV zero = FZero();
		if(takeCoreShape)
		{
			pa = _pa;
			pb = _pb;
			const Vec3V v = V3Sub(_pb, _pa);
			const FloatV length = V3Length(v);
			normal = V3ScaleInv(v, length);
			penDepth = FNeg(length);

		}
		else
		{
			const BoolV aQuadratic = a.isMarginEqRadius();
			const BoolV bQuadratic = b.isMarginEqRadius();

			const Vec3V v = V3Sub(_pb, _pa);
			const FloatV length = V3Length(v);
			const Vec3V n = V3ScaleInv(v, length);
			const FloatV marginA = FSel(aQuadratic, a.getMargin(), zero);
			const FloatV marginB = FSel(bQuadratic, b.getMargin(), zero);
			const FloatV sumMargin = FAdd(marginA, marginB);
			pa =V3NegScaleSub(n, marginA, _pa);
			pb =V3ScaleAdd(n, marginB, _pb);
			penDepth = FNeg(FAdd(length, sumMargin));
			normal = n;
		}
	}

	PxGJKStatus EPA::PenetrationDepth(const ConvexV& a, const ConvexV& b, EPASupportMapPair* pair, const Ps::aos::Vec3V* PX_RESTRICT /*Q*/, const Ps::aos::Vec3V* PX_RESTRICT A, const Ps::aos::Vec3V* PX_RESTRICT B, const PxI32 size, Ps::aos::Vec3V& pa, Ps::aos::Vec3V& pb, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penDepth, const bool takeCoreShape)
	{
	
		using namespace Ps::aos;   

		Ps::prefetchLine(&facetBuf[0]);
		Ps::prefetchLine(&facetBuf[0], 128);

		const FloatV zero = FZero();
	
		const FloatV _max = FMax();
	
		aBuf[0]=A[0]; aBuf[1]=A[1]; aBuf[2]=A[2]; aBuf[3]=A[3];
		bBuf[0]=B[0]; bBuf[1]=B[1]; bBuf[2]=B[2]; bBuf[3]=B[3];

		PxI32 numVertsLocal = 0;

		heap.makeEmpty();

		//if the simplex isn't a tetrahedron, we need to construct one before we can expand it
		switch (size) 
		{
		case 1:
			{
				// Touching contact. Yes, we have a collision,
				// but no penetration, will still have a contact point, but we treat it as nonintersect because it don't have any penetration
				return EPA_FAIL;
			}
		case 2: 
			{
				// We have a line segment inside the Minkowski sum containing the
				// origin. Blow it up by adding three additional support points.
				if(!expandSegment(a, b, pair, numVertsLocal))
					return EPA_FAIL;
				break;
			}
		case 3: 
			{
				// We have a triangle inside the Minkowski sum containing
				// the origin. First blow it up.
				if(!expandTriangle(a, b, pair, numVertsLocal))
					return EPA_FAIL;
				
				break;
				
			}
		case 4:
			{
				Facet * PX_RESTRICT f0 = addFacet(0, 1, 2, zero, _max);
				Facet * PX_RESTRICT f1 = addFacet(0, 3, 1, zero, _max);
				Facet * PX_RESTRICT f2 = addFacet(0, 2, 3, zero, _max);
				Facet * PX_RESTRICT f3 = addFacet(1, 3, 2, zero, _max);

				if((f0==NULL) || (f1==NULL) || (f2==NULL) || (f3==NULL) || heap.isEmpty())
					return EPA_FAIL;

			
				f0->link(0, f1, 2);
				f0->link(1, f3, 2);
				f0->link(2, f2, 0);
				f1->link(0, f2, 2);
				f1->link(1, f3, 0);
				f2->link(1, f3, 1);
				numVertsLocal = 4;

				break;
			}
		}

		const BoolV bTrue = BTTTT();
		FloatV upper_bound2(_max);
		FloatV inflated_upper_bound2(_max);
		const FloatV eps = FLoad(0.0001f);
		const FloatV sqEps = FMul(eps, eps);
		const FloatV eps2 = FLoad(1e-6);


		Facet* PX_RESTRICT facet = NULL;
		
		do 
		{
		
			facetManager.processDeferredIds();
			facet = heap.deleteTop(); //get the shortest distance triangle of origin from the list
			facet->m_inHeap = false;
			Vec3V tempa, tempb, q;

			if (!facet->isObsolete()) 
			{
				Ps::prefetchLine(edgeBuffer.m_pEdges);
				Ps::prefetchLine(edgeBuffer.m_pEdges,128);
				Ps::prefetchLine(edgeBuffer.m_pEdges,256);

				const FloatV fSqDist = facet->getDist();
				
				const Vec3V closest = facet->getClosest();
		
				pair->doSupport(V3Neg(closest), tempa, tempb, q);
				
				Ps::prefetchLine(&aBuf[numVertsLocal],128);
				Ps::prefetchLine(&bBuf[numVertsLocal],128);

				//if the support point and closest point close enough, found the contact point, break
				const FloatV dist = V3Dot(q, closest); 

				const FloatV sqDist = FMul(dist, dist);

				upper_bound2 = FMin(upper_bound2, FDiv(FMax(eps2, sqDist), fSqDist));

				const FloatV planeDist = FSub(V3Dot(facet->getPlaneNormal(), q), facet->getPlaneDist());

				const BoolV con0 = FIsGrtrOrEq(eps2, FAbs(FSub(fSqDist, upper_bound2)));
				const BoolV con1 = FIsGrtrOrEq(eps2, FAbs(planeDist));

				if(BAllEq(BOr(con0, con1), bTrue))
				{
					Vec3V _pa, _pb;
					facet->getClosestPoint(aBuf, bBuf, _pa, _pb);
					calculateContactInformation(_pa, _pb, a, b, pa, pb, normal, penDepth, takeCoreShape);
					return EPA_CONTACT;
				} 

		
				aBuf[numVertsLocal]=tempa;
				bBuf[numVertsLocal]=tempb;

				const PxU32 index =PxU32(numVertsLocal++);

				inflated_upper_bound2 = FAdd(upper_bound2, sqEps);
				const FloatV inflated_lower_bound2 = FSub(fSqDist, sqEps);


				// Compute the silhouette cast by the new vertex
				// Note that the new vertex is on the positive side
				// of the current facet, so the current facet is will
				// not be in the convex hull. Start local search
				// from this facet.

				edgeBuffer.MakeEmpty();

				facet->silhouette(q, edgeBuffer, facetManager);

				if(edgeBuffer.IsEmpty()) 
					return EPA_FAIL;

				Edge* PX_RESTRICT edge=edgeBuffer.Get(0);

				Facet *firstFacet = addFacet(edge->getTarget(), edge->getSource(),index, inflated_lower_bound2, inflated_upper_bound2);
				if(firstFacet == NULL)
				{
					Vec3V _pa, _pb;
					facet->getClosestPoint(aBuf, bBuf, _pa, _pb);
					calculateContactInformation(_pa, _pb, a, b, pa, pb, normal, penDepth, takeCoreShape);
					return EPA_DEGENERATE;
				}
				
			
				firstFacet->link(0, edge->getFacet(), edge->getIndex());
				Facet * PX_RESTRICT lastFacet = firstFacet;

				PxU32 bufferSize=edgeBuffer.Size();

				bool degenerate = false;

				for(PxU32 i=1; i<bufferSize; ++i)
				{
					edge=edgeBuffer.Get(i);
					Facet* PX_RESTRICT newFacet = addFacet(edge->getTarget(), edge->getSource(),index, inflated_lower_bound2, inflated_upper_bound2);
					if(newFacet == NULL)
					{
						degenerate = true;
						break;
					}
					else
					{
						const bool b0 = newFacet->link(0, edge->getFacet(), edge->getIndex());
						const bool b1 = newFacet->link(2, lastFacet, 1);
						if((!b0) | (!b1))
						{
							degenerate = true;
							break;
						}
						lastFacet = newFacet;
					}
				}

				if(degenerate)
				{
					Vec3V _pa, _pb;
					facet->getClosestPoint(aBuf, bBuf, _pa, _pb);
					calculateContactInformation(_pa, _pb, a, b, pa, pb, normal, penDepth, takeCoreShape);
					return EPA_DEGENERATE;
				}

				firstFacet->link(2, lastFacet, 1);
			}
			facetManager.freeID(facet->m_FacetId);
		}
		while((heap.heapSize > 0) && (FAllGrtrOrEq(upper_bound2, heap.getTop()->getDist()) & (numVertsLocal != MaxSupportPoints)));

		Vec3V _pa, _pb;
		facet->getClosestPoint(aBuf, bBuf, _pa, _pb);
		calculateContactInformation(_pa, _pb, a, b, pa, pb, normal, penDepth, takeCoreShape);
		
		return EPA_DEGENERATE;
	}
}

}
