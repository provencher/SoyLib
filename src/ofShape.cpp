#include "ofShape.h"
#include <ofxDelaunay.h>


bool ofShapeBox3::IsOutside(const vec2f& Pos) const
{
	if ( Pos.x < mMin.x )	return true;
	if ( Pos.y < mMin.y )	return true;

	if ( Pos.x > mMax.x )	return true;
	if ( Pos.y > mMax.y )	return true;

	return false;
}




TIntersection ofShape::GetIntersection(const ofShapeCircle2& a,const ofShapeCircle2& b)
{
	if ( !a.IsValid() || !b.IsValid() )
		return TIntersection(false);

	TIntersection Intersection;
	auto& Delta = Intersection.mDelta;

	Intersection.mDelta = b.mPosition - a.mPosition;
	Intersection.mDistanceSq = Delta.lengthSquared();
	float Distance = Delta.length();

	float RadTotal = a.mRadius + b.mRadius;

	Intersection.mIsValid = ( Intersection.mDistanceSq <= RadTotal*RadTotal );
	if ( !Intersection.mIsValid )
		return Intersection;

	//	work out hit points
	Intersection.mMidIntersection = a.mPosition + ( Delta*0.5f);
	Intersection.mCollisionPointA = a.mPosition + ( Delta.normalized() * a.mRadius );
	Intersection.mCollisionPointB = b.mPosition + ( Delta.normalized() * -b.mRadius );
	
	return Intersection;
}



TIntersection ofShape::GetIntersection(const ofShapePolygon2& a,const ofShapeCircle2& b)
{
	if ( !a.IsValid() || !b.IsValid() )
		return TIntersection(false);

	ofLine2 ab( a.mTriangle[0], a.mTriangle[1] );
	ofLine2 bc( a.mTriangle[1], a.mTriangle[2] );
	ofLine2 ca( a.mTriangle[2], a.mTriangle[0] );
	
	//	see if circle intersects edge
	//	get circle -> triangle edge nearest point
	vec2f abnear = ab.GetNearestPoint( b.GetCenter() ) - b.GetCenter();
	vec2f bcnear = bc.GetNearestPoint( b.GetCenter() ) - b.GetCenter();
	vec2f canear = ca.GetNearestPoint( b.GetCenter() ) - b.GetCenter();
	float abneardistsq = abnear.lengthSquared();
	float bcneardistsq = bcnear.lengthSquared();
	float caneardistsq = canear.lengthSquared();
	float RadSq = b.mRadius * b.mRadius;

	//	find nearest valid edge intersection
	vec2f* pnear = NULL;
	float* pneardistsq = NULL;
	if ( abneardistsq < bcneardistsq && abneardistsq < caneardistsq && abneardistsq <= RadSq )
	{
		pnear = &abnear;
		pneardistsq = &abneardistsq;
	}
	else if ( bcneardistsq < caneardistsq && bcneardistsq <= RadSq )
	{
		pnear = &bcnear;
		pneardistsq = &bcneardistsq;
	}
	else if ( caneardistsq <= RadSq )
	{
		pnear = &canear;
		pneardistsq = &caneardistsq;
	}

	//	got nearest intersection
	if ( pnear )
	{
		TIntersection Intersection(true);
		Intersection.mDelta = pnear->normalized();
		Intersection.mCollisionPointA = *pnear + b.GetCenter();	//	triangle
		Intersection.mCollisionPointB = b.GetCenter() + (Intersection.mDelta*b.mRadius);	//	circle
		Intersection.mDistanceSq = *pneardistsq;
		Intersection.mMidIntersection = ( b.GetCenter() + a.GetCenter() ) * 0.5f;
		return Intersection;
	}

	//	see if the circle is wholly inside the triangle
	if ( !a.IsInside( b.GetCenter() ) )
		return TIntersection(false);

	TIntersection Intersection(true);
	Intersection.mCollisionPointA = a.GetCenter();	//	triangle
	Intersection.mCollisionPointB = b.GetCenter();	//	circle
	Intersection.mDelta = Intersection.mCollisionPointB - Intersection.mCollisionPointA;
	Intersection.mDistanceSq = Intersection.mDelta.lengthSquared();
	Intersection.mMidIntersection = ( Intersection.mCollisionPointA + Intersection.mCollisionPointB ) * 0.5f;
	return Intersection;

}

bool ofShapePolygon2::IsInside(const vec2f& Pos) const
{
	auto& v1 = mTriangle[0];
	auto& v2 = mTriangle[1];
	auto& v3 = mTriangle[2];

	bool absign = ((v2.y - v1.y)*(Pos.x - v1.x) - (v2.x - v1.x)*(Pos.y - v1.y)) >= 0;
	bool bcsign = ((v3.y - v2.y)*(Pos.x - v2.x) - (v3.x - v2.x)*(Pos.y - v2.y)) >= 0;
	bool casign = ((v1.y - v3.y)*(Pos.x - v3.x) - (v1.x - v3.x)*(Pos.x - v3.x)) >= 0;

	//	all same side (doesn't matter about winding then)
	return (absign == bcsign && absign == casign);
}


TIntersection ofShape::GetIntersection(const ofShapePolygon2& a,const ofShapePolygon2& b)
{
	if ( !a.IsValid() || !b.IsValid() )
		return TIntersection(false);

	//	test line intersections
	for ( int i=0;	i<a.mTriangle.GetSize();	i++ )
	{
		ofLine2 aedge( a.mTriangle[i], a.mTriangle[(i+1)%a.mTriangle.GetSize()] );
		for ( int j=0;	j<b.mTriangle.GetSize();	j++ )
		{
			ofLine2 bedge( b.mTriangle[j], b.mTriangle[(j+1)%b.mTriangle.GetSize()] );

			vec2f CrossPos;
			if ( !aedge.GetIntersection( bedge, CrossPos ) )
				continue;

		}
	}
	
	//	check in case one is wholly inside the other,
	//	we just need to check any point so use first
	vec2f acenter = b.GetCenter();
	vec2f bcenter = b.GetCenter();
	if ( a.IsInside( bcenter ) || b.IsInside( acenter ) )
	{
		TIntersection Intersection(true);
		Intersection.mCollisionPointA = acenter;	//	triangle
		Intersection.mCollisionPointB = bcenter;	//	circle
		Intersection.mDelta = Intersection.mCollisionPointB - Intersection.mCollisionPointA;
		Intersection.mDistanceSq = Intersection.mDelta.lengthSquared();
		Intersection.mMidIntersection = ( Intersection.mCollisionPointA + Intersection.mCollisionPointB ) * 0.5f;
		return Intersection;
	}

	return TIntersection(false);
}


ofShapeCircle2 ofShapePolygon2::GetBounds() const
{
	ofShapeCircle2 Circle;
	Circle.mPosition = GetCenter();

	//	find longest point from center
	for ( int i=0;	i<mTriangle.GetSize();	i++ )
	{
		vec2f Delta = mTriangle[i] - Circle.mPosition;
		Circle.mRadius = ofMax( Circle.mRadius, Delta.length() );
	}
	return Circle;
}
	
bool ofShapeCircle2::IsInside(const vec2f& Point) const
{
	vec2f Delta = Point - mPosition;
	float DeltaLenSq = Delta.lengthSquared();
	return ( DeltaLenSq <= mRadius*mRadius );
}


bool ofShapeCircle2::IsInside(const ofShapeCircle2& Shape) const
{
	vec2f Delta = Shape.mPosition - mPosition;
	float DeltaLenSq = Delta.lengthSquared();
	float Rad = mRadius + Shape.mRadius;
	return ( DeltaLenSq <= Rad*Rad );
}


void ofShapeCircle2::Accumulate(const ofShapeCircle2& that)
{
	//	make no change
	if ( !that.IsValid() )
		return;

	//	just use argument
	if ( !IsValid() )
	{
		*this = that;
		return;
	}

	//	get furthest point on both spheres from each other
	vec2f DirToSphere( that.GetCenter() - this->GetCenter() );
	float LengthSq = DirToSphere.lengthSquared();
	
	//	overlapping, so just get a new larger radius
	if ( LengthSq < ofNearZero )
	{
		mRadius = ofMax( that.mRadius, this->mRadius );
		return;
	}

	//	normalise dir
	DirToSphere.normalize();

	//	get the furthest point away on the sphere
	vec2f FurthestPointOnSphere = that.GetCenter() + ( DirToSphere * that.mRadius );

	//	if its already inside this sphere then we dont need to change anything
	if ( IsInside( FurthestPointOnSphere ) )
		return;

	//	get furthest point away on the existing sphere
	vec2f FurthestPointOnThis = this->GetCenter() - ( DirToSphere * this->mRadius );

	//	new sphere center is midpoint between furthest points
	mPosition = (FurthestPointOnSphere + FurthestPointOnThis) * 0.5f;

	//	new radius is half length from furthest point to furthest point
	mRadius = (FurthestPointOnSphere - FurthestPointOnThis).length() * 0.5f;
}



void ofShapePath2::PushTail(const vec2f& Pos)
{
	//	empty path? initialise it
	if ( mPath.IsEmpty() )
		mPath.PushBack( Pos );

	//	get length of last segment...
	bool ReplaceTail = false;
	if ( mPath.GetSize() >= 2 )
	{
		vec2f TailSegment = mPath[mPath.GetSize()-2] - mPath[mPath.GetSize()-1];
		if ( TailSegment.lengthSquared() < mMinSegmentLength*mMinSegmentLength )
			ReplaceTail = true;
	}

	if ( ReplaceTail )
	{
		mPath.GetBack() = Pos;
	}
	else
	{
		mPath.PushBack( Pos );
	}		
}

float ofShapePath2::GetLength() const
{
	float LengthSq = 0.f;

	for ( int i=0;	i<mPath.GetSize()-1;	i++ )
	{
		auto& Start = mPath[i];
		auto& End = mPath[i+1];
		float DeltaLengthSq = (End-Start).lengthSquared();
		LengthSq += DeltaLengthSq;
	}

	return sqrtf( LengthSq );
}

SoyPair<vec2f,vec2f> ofShapePath2::GetPosition(float PathPos,float NextPos) const
{
	SoyPair<vec2f,vec2f> Positions;
	if ( mPath.IsEmpty() )
		return Positions;

	//	check incase reverse
	bool Swap = false;
	if ( PathPos > NextPos )
	{
		ofSwap( PathPos, NextPos );
		Swap = true;
	}

	//	walk along path to find points
	float Walk = 0.f;
	vec2f& PathPosPoint = Positions.mFirst;
	vec2f& NextPosPoint = Positions.mSecond;

	for ( int i=0;	i<mPath.GetSize()-1;	i++ )
	{
		vec2f Segment = mPath[i+1] - mPath[i];
		float SegmentLength = Segment.length();

		//	not at first point yet, step over
		if ( Walk + SegmentLength < PathPos )
		{
			Walk += SegmentLength;
			continue;
		}

		//	gone past last point
		if ( Walk > NextPos )
			break;

		//	first point is in this segment...
		if ( Walk <= PathPos && PathPos < Walk+SegmentLength )
		{
			float PathPosTime = ofGetMathTime( PathPos, Walk, Walk+SegmentLength );
			PathPosPoint = mPath[i] + (Segment * PathPosTime);
		}

		//	second point is in this segment...
		if ( Walk <= NextPos && NextPos < Walk+SegmentLength )
		{
			float NextPosTime = ofGetMathTime( NextPos, Walk, Walk+SegmentLength );
			NextPosPoint = mPath[i] + (Segment * NextPosTime);
		}

		//	move along
		Walk += SegmentLength;
	}

	//	in case we never reached the desired points...
	if ( PathPos >= Walk )
		PathPosPoint = mPath.GetBack();
	if ( NextPos >= Walk )
		NextPosPoint = mPath.GetBack();

	if ( Swap )
		ofSwap( PathPosPoint, NextPosPoint );

	return Positions;
}

vec2f ofShapePath2::GetPosition(float PathPos) const
{
	SoyPair<vec2f,vec2f> PosA = GetPosition( PathPos, PathPos );
	return PosA.mFirst;
}



vec2f ofShapePath2::GetDelta(float PathPos,float NextPos) const
{
	SoyPair<vec2f,vec2f> Positions = GetPosition( PathPos, NextPos );
	return (Positions.mSecond - Positions.mFirst);
}


vec2f ofShapePath2::GetTailNormal(float TailDistance) const
{
	//	get the last delta (normalised)
	if ( mPath.GetSize() < 2 )
		return vec2f( 0, 1 );

	//	make a list of points back to the specified distance...
	//	todo: turn into spline and get a more clever normal..
	Array<vec2f> Tail;
	for ( int i=mPath.GetSize()-1;	(TailDistance>0.f) && (i>=0);	i-- )
	{
		Tail.PushBack( mPath[i] );

		//	see if we need to cut off
		if ( i == 0 )
			continue;
		
		vec2f Delta = mPath[i-1] - mPath[i];
		float DeltaLen = Delta.length();
		if ( DeltaLen < TailDistance )
		{
			TailDistance -= DeltaLen;
			continue;
		}

		//	break half way through this delta.
		float Time = ofGetMathTime( 0, TailDistance-DeltaLen, TailDistance );
		Delta *= 1.f - Time;
		Tail.PushBack( mPath[i] + Delta );
		break;
	}

	//	unexpected...
	assert( Tail.GetSize() >= 2 );
	if ( Tail.GetSize() < 2 )
	{
		Tail.PushBack( mPath[mPath.GetSize()-2] );
	}

	//	get this tail normal
	vec2f Delta = Tail[0] - Tail.GetBack();
	return Delta.normalized();
}



TIntersection ofShape::GetIntersection(const ofShapeCapsule2& a,const ofShapeCircle2& b)
{
	if ( !a.IsValid() || !b.IsValid() )
		return TIntersection(false);

	//	turn the capsule into a circle at the nearest point
	vec2f NearestPointOnA = a.mLine.GetNearestPoint( b.mPosition );
	ofShapeCircle2 aCircle( NearestPointOnA, a.mRadius );

	return GetIntersection( aCircle, b );
}

TIntersection ofShape::GetIntersection(const ofShapeCapsule2& a,const ofShapeCapsule2& b)
{
	if ( !a.IsValid() || !b.IsValid() )
		return TIntersection(false);

	//	find the nearest points on the two lines (doesn't matter if there's an intersection or not)
	vec2f NearestPointOnA, NearestPointOnB;
	a.mLine.GetNearestPoints( b.mLine, NearestPointOnA, NearestPointOnB );

	//	turn the capsule into a circle at the nearest points
	ofShapeCircle2 aCircle( NearestPointOnA, a.mRadius );
	ofShapeCircle2 bCircle( NearestPointOnB, b.mRadius );

	return GetIntersection( aCircle, bCircle );
}

TIntersection ofShape::GetIntersection(const ofShapeCapsule2& a,const ofShapePolygon2& b)
{
	if ( !a.IsValid() || !b.IsValid() )
		return TIntersection(false);

	//	todo!
	return TIntersection(false);
}


float sign(const vec2f& p1, const vec2f& p2, const vec2f& p3)
{
  return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}

bool ofShapeTriangle2::IsInside(const vec2f& Point) const
{
	bool b1 = sign( Point, mTriangle[0], mTriangle[1] ) < 0.0f;
	bool b2 = sign( Point, mTriangle[1], mTriangle[2] ) < 0.0f;
	bool b3 = sign( Point, mTriangle[2], mTriangle[0] ) < 0.0f;

	return ((b1 == b2) && (b2 == b3));
}

float ofShapeTriangle2::GetArea() const
{
	ofPolyline PolyA;
	PolyA.addVertex( mTriangle[2] );
	PolyA.addVertex( mTriangle[1] );
	PolyA.addVertex( mTriangle[0] );
	float AreaA = PolyA.getArea();
	ofPolyline PolyB;
	PolyB.addVertex( mTriangle[0] );
	PolyB.addVertex( mTriangle[1] );
	PolyB.addVertex( mTriangle[2] );
	float AreaB = PolyB.getArea();
	return AreaA;
}


template<class POINTARRAY>
void TesselatePoints(ArrayBridge<ofShapeTriangle2>& Triangles,const POINTARRAY& Points,int PointCount)
{
	ofxDelaunay Mesh;
	for ( int i=0;	i<PointCount;	i++ )
	{
		Mesh.addPoint( Points[i] );
	}
	Mesh.triangulate();

	//	extract triangles
	auto& Vertexes = Mesh.triangleMesh.getVertices();
	auto& TriangleIndexes = Mesh.triangleMesh.getIndices();
	for ( int i=0;	i<TriangleIndexes.size();	i+=3 )
	{
		auto& v0 = Vertexes[TriangleIndexes[i+0]];
		auto& v1 = Vertexes[TriangleIndexes[i+1]];
		auto& v2 = Vertexes[TriangleIndexes[i+2]];
		auto& Triangle = Triangles.PushBack();
		Triangle.mTriangle[0] = v0.xy();
		Triangle.mTriangle[1] = v1.xy();
		Triangle.mTriangle[2] = v2.xy();
	}
	
}

void Tesselate(ArrayBridge<ofShapeTriangle2>& Triangles,const ArrayBridge<vec2f>& Polygon)
{
	TesselatePoints( Triangles, Polygon, Polygon.GetSize() );
}

void Tesselate(ArrayBridge<ofShapeTriangle2>& Triangles,const ofPolyline& Polygon)
{
	TesselatePoints( Triangles, Polygon, Polygon.size() );

}
