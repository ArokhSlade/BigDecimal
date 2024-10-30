#ifndef G_MATH_UTILITY_H
#define G_MATH_UTILITY_H

#include "G_Essentials.h"
#include "G_Miscellany_Utility.h" //provides:ori_x, same_other
#include <cmath>
#include "G_Math_Intrinsics_Component.h"
#include "G_Float_Basics.h"
#include "G_ErrorEstimation.h"
#include <string> //to_string(v2)
#include <iostream> //ostream& operator<<

//#define Pi32 3.14159265359f
constexpr const real32 Pi32 = 3.14159265359f;
#define ROOT2_32 1.41421358f
#define ROOT2_REC_32 0.7071067691f


#define MINIMUM(X,Y) ((X<Y)?(X):(Y))
#define MAXIMUM(X,Y) ((X>Y)?(X):(Y))



inline int32 Abs(int32 X) {
    return X<0 ? -X : X;
}



inline int32 Log2I(int32 X) {
    HardAssert(X > 0);
    return BitScanReverse32(X);
}

inline i32 Log10I(i32 X) {
    HardAssert(X > 0);
    if (X == 1) return 0;
    i32 Result = 0;
    while (X /= 10) {
        ++Result;
    }
    return Result;
}


int32  BubbleSortAsc(float *Values, unsigned int nValues, unsigned int *SortedIndices, bool32 OnlyTrue);

typedef real32 angle32;

inline angle32 ReverseAngle(angle32 Angle)
{
    return Angle <=0.f ? Angle + Pi32 : Angle - Pi32;
}

//note 2022 01 17: current version distinguishes -0 and 0. is this  what we want?
/**
 * Distance in radians from A to B.
 * e.g. Diff(0,pi) = pi, Diff(-1,1) = 2,
 *      Diff(-pi,pi) = 2pi, Diff(pi,-pi) = 0
**/
inline angle32 CCW_Diff(angle32 A, angle32 B)
{
    angle32 Result;
    if (IsNegative(A) == IsNegative(B))
    {
        if (B >= A)
        {
            Result = B-A;
        }
        else
        {
            Result = 2.f * Pi32 - (A-B);
        }
    }
    else
    {
        if (IsNegative(A))
        {
            Result = B-A;
        }
        else
        {
            Result = B-A + 2*Pi32;
        }
    }
    return Result;
}

inline uint32 Minimum(uint32 X, uint32 Y)
{
	return (X < Y ? X : Y);
}

inline uint32 Maximum(uint32 X, uint32 Y)
{
	return (Y < X ? X : Y);
}


inline real32 Minimum(real32 X, real32 Y)
{
	return (X < Y ? X : Y);
}

inline real32 Maximum(real32 X, real32 Y)
{
	return (Y < X ? X : Y);
}
//TODO: move to intrinsics?
inline real32 Square(real32 x) {
	return x*x;
}



inline bool32 EpsilonEquals(real32 A, real32 B, real32 Eps)
{
    return Abs(A-B) <= Eps;
}

inline real32 GetEps(real32 X, real32 Y, int32 K, real32 FallbackExp = -150.f);

inline bool32 TruncEquals(real32 A, real32 B, int32 TruncN) {
    real32 Eps = GetEps(A,B,TruncN);
    return EpsilonEquals(A,B,Eps);
}

inline bool32 EpsilonLessThan ( real32 A, real32 B, real32 Eps)
{
    return A + Eps < B;
}

inline bool32 EpsilonLessOrEquals(real32 A, real32 B, real32 Eps)
{
    return EpsilonEquals(A,B,Eps) || EpsilonLessThan(A,B,Eps);
}

inline bool32 EpsilonGreaterThan (real32 A, real32 B, real32 Eps)
{
    return EpsilonLessThan(B, A, Eps);
}

inline bool32 EpsilonGreaterOrEquals ( real32 A, real32 B, real32 Eps)
{
    return EpsilonLessOrEquals(B, A, Eps);
}


inline bool32 EpsilonEquals_2(real32 A, real32 B, real32 ErrA, real32 ErrB)
{
    return Abs(A-B) <= EstimateArithmeticError(Subtraction, A-B,A,B,ErrA,ErrB);
}

inline bool32 EpsilonLessThan_2 (real32 A, real32 B, real32 ErrA, real32 ErrB)
{
    return (A < B) && (!EpsilonEquals_2(A,B,ErrA,ErrB));
}

inline bool32 EpsilonLessOrEquals_2(real32 A, real32 B, real32 ErrA, real32 ErrB)
{
    return (EpsilonEquals_2(A,B,ErrA,ErrB)) || (A<B);
}

inline bool32 EpsilonGreaterThan_2 (real32 A, real32 B, real32 ErrA, real32 ErrB)
{
    return EpsilonLessThan_2(B, A, ErrB, ErrA);
}

inline bool32 EpsilonGreaterOrEquals_2 (real32 A, real32 B, real32 ErrA, real32 ErrB)
{
    return EpsilonLessOrEquals_2(B, A, ErrB, ErrA);
}

inline bool32 EpsilonEqualAngle (angle32 A, angle32 B, real32 Eps)
{
    return EpsilonEquals(A,B,Eps)
        || (EpsilonEquals(A,0.f,Eps) && EpsilonEquals(B,2*Pi32,Eps))
        || (EpsilonEquals(A,2*Pi32,Eps) && EpsilonEquals(B,0.f,Eps));
}

inline bool32 EpsilonEqualAngle_2 (angle32 A, angle32 B, real32 ErrA, real32 ErrB)
{
    return EpsilonEquals_2(A,B,ErrA,ErrB)
        || (EpsilonEquals(A,0.f,ErrA) && EpsilonEquals_2(B,2*Pi32,ErrB,half_ulp(2*Pi32)))
        || (EpsilonEquals_2(A,2*Pi32,ErrA,half_ulp(2*Pi32)) && EpsilonEquals(B,0.f,ErrB));
}

inline bool32 EpsilonLessThanAngle (angle32 A, angle32 B, real32 Eps)
{
    return !(EpsilonEqualAngle(A,B,Eps))
        && (EpsilonEqualAngle(A,2*Pi32,Eps)
            ? EpsilonLessThan(0.f,B,Eps)
            : EpsilonLessThan(A,B,Eps) );
}

inline bool32 EpsilonLessEqualAngle (angle32 A, angle32 B, real32 Eps)
{
    return EpsilonEqualAngle(A,B,Eps)
        || (EpsilonEqualAngle(A,2*Pi32,Eps)
            ? EpsilonLessThan(0.f,B,Eps)
            : EpsilonLessThan(A,B,Eps) );
}

inline bool32 EpsilonLessThanAngle_2 (angle32 A, angle32 B, real32 ErrA, real32 ErrB)
{
    return !(EpsilonEqualAngle_2(A,B,ErrA,ErrB))
        && (EpsilonEqualAngle_2(A,2*Pi32,ErrA,half_ulp(2*Pi32))
            ? EpsilonLessThan_2(0.f,B,ErrA,ErrB)
            : EpsilonLessThan_2(A,B,ErrA,ErrB) );
}

inline bool32 EpsilonLessEqualAngle_2 (angle32 A, angle32 B, real32 ErrA, real32 ErrB)
{
    return EpsilonEqualAngle_2(A,B,ErrA,ErrB)
        || (EpsilonEqualAngle_2(A,2*Pi32,ErrA,half_ulp(2*Pi32))
            ? EpsilonLessThan_2(0.,B,ErrA,ErrB)
            : EpsilonLessThan_2(A,B,ErrA,ErrB) );
}


//TODO(##2024 07 03):this code is broken. especially cases x=0 (150 pointless iterations, returns -150) and x=power of 2 (returns wrong result)
//NOTE(Gerald): isn't this equivalent to floor(log(x))?
//TODO: proper function name
//WARNING: Don't call if x==0. x==0 should return -infinity or throw an exception.
//assertion for now.
//BUG(ArokhSlade##2024 09 26):wrong result for x=2
inline int32 GetBinaryExponentReal32(real32 x)
{
    Assert(x != 0);
    real32 AbsX = x < 0.f ? -x : x;

    int32 e = 0;
    real32 temp = 1.f;
    if (AbsX > 1.f)
    {
        while (temp < AbsX)
        {
            temp *= 2;
            e += 1;
        }
        return e-1;
    }
    else
    {
        while (temp > AbsX)
        {
            temp /= 2.f;
            e -= 1;
        }
        return e;
    }
}

//TODO: input is 0, sub-normal, inf, NaN
inline real32 GetMachineEpsilonReal32(real32 x)
{
    return std::pow(2.f, -23.f + GetBinaryExponentReal32(x));
}

///define eps relative to magintude of X and some carefully chosen K number of least significant bits
inline real32 GetEps(real32 X, int32 K)
{
    return std::pow(2.f, (real32)GetBinaryExponentReal32(X)-23.f+(real32)K);
}


/////pick eps based on the number with the greater magnitude
//inline real32 GetEps(real32 X, real32 Y, int32 K)
//{
//    real32 MaxExp = Maximum((real32)GetBinaryExponentReal32(X), (real32)GetBinaryExponentReal32(Y) );
//    return std::pow(2.f, MaxExp -23.f+(real32)K);
//}


/** \brief: Eps for 0 is not defined, so let client decide with fallback.
 *  the default value leads to an Epsilon of 0.
 **/
inline real32 GetEpsOrFallback(real32 X, int32 K, real32 FallbackExp = -150.f) {
    return (X == 0.f) ? std::pow(2.f, FallbackExp) : GetEps(X, K);
}

///pick eps based on the number with the greater magnitude
inline real32 GetEps(real32 X, real32 Y, int32 K, real32 FallbackExp /*= -150.f*/)
{
    return Maximum(GetEpsOrFallback(X, K, FallbackExp), GetEpsOrFallback(Y, K, FallbackExp));
}

///// \brief: Eps for 0 is not defined, so let client decide with fallback
//inline real32 GetEpsForPairOrFallback(real32 X, real32 Y, real32 K, real32 FallbackExp = -23.f) {
//    return Maximum(GetEpsOrFallback(X, FallbackExp), GetEpsOrFallback(Y, FallbackExp));
//}




inline int32 GetExponentOfLeastSignificantSetBitReal32(real32 F)
{
    Assert(F != 0);
    int32 Result = GetBinaryExponentReal32(F);
    uint32 FBits = *(uint32 *)&F;
    bit_scan_result BSR = FindLeastSignificantSetBit(FBits);
    int32 Offset = BSR.Found && BSR.Index < 23 ? BSR.Index : 23;
    Result += -23 + Offset;
    return Result;
}


/**
    returns angle-distance between two angles, in a "normalized" fashion,
    such that: A is considered to be >= B,
    and the result is always between >=0 and <2pi
    Angles are interpreted  in rad.
    examples: A = 1.5*Pi B = .5*Pi Result = 1*Pi
              A = .5*Pi B = 1.5*Pi Result = 1*Pi
              A = -5.*Pi B = -2000*Pi Result = 1.5*Pi

    cases: 2Pi > A > B
*/
inline real32 MeasureAngleFromTo(real32 A, real32 B)
{
    while (B<A)
        B+=2*Pi32;

    real32 Result = B-A;
    while (Result >= 2*Pi32)
        Result -= 2*Pi32;
    Assert(Result >= 0.f && Result < 2.f * Pi32);
    return Result;
}



inline double LinearInterpolate(real32 Value0, real32 Value1, real32 Ratio)
{
    double Result;
    if (Ratio <= 0) Result = Value0;
    else if (Ratio >= 1) Result = Value1;
    else Result = Value0 + Ratio * (Value1 - Value0);
    return Result;
}

////////////////////////////////////////////////////////////
//v2 - 2D Vectors
////////////////////////////////////////////////////////////

union v2 {
    struct {
        real32 X, Y;
    };
    real32 E[2];
    struct {
        real32 Slope, Intercept;
    };
         /*
	v2() =default;
	v2(float ArgX, float ArgY): X{ArgX}, Y{ArgY} {}
	v2(int ArgX, int ArgY) : X {static_cast<float>(ArgX)} ,
	                             Y {static_cast<float>(ArgY)}
    {} */

};

inline v2 V2(real32 X, real32 Y) {
	return{.X=X,.Y=Y };
}

inline void ParseV2(char *Text, char **End, v2 *Result)
{
  char *Cur = Text;
  ++Cur;
  Result->X = ParseReal32(Cur,&Cur);
  ++Cur;
  Result->Y = ParseReal32(Cur,End);
  (*End)++;
}

inline int32 Str(char *Buf, v2 V)
{
  int32 Cnt = stbsp_sprintf( Buf, "(%f,%f)", V.X,V.Y);
//  int32 Cnt = std::sprintf( Buf, "(%f,%f)", V.X,V.Y);
  return Cnt;
}

///hook up to std::to_string
inline std::string to_string(v2 const& V) { return "(" + std::to_string(V.X) + "," + std::to_string(V.Y) + ")"; }
inline std::ostream& operator<<(std::ostream& OS, v2 V)
{
    return OS << to_string(V);
}

/**
Expected format: (X,Y)
where X and Y are real32
*/
inline std::istream& operator>>(std::istream& IS, v2& V)
{
    char Discardable;
    IS >> Discardable >> V.X >> Discardable >> V.Y >> Discardable;

    return IS;
}

inline v2 Abs(v2 A)
{
    return V2(Abs(A.X), Abs(A.Y));
}

inline v2 operator+(v2 A, v2 B) {
	v2 C;
	C.X = A.X + B.X;
	C.Y = A.Y + B.Y;
	return C;
}

inline v2 operator-(v2 A, v2 B) {
	v2 C;
	C.X = A.X - B.X;
	C.Y = A.Y - B.Y;
	return C;
}

inline v2 operator-(v2 A) {
	v2 V;
	V.X = -A.X;
	V.Y = -A.Y;
	return V;
}

inline v2 operator*(real32 a, v2 B) {
	v2 C;
	C.X = a * B.X;
	C.Y = a * B.Y;
	return C;
}

inline v2 operator*(v2 B, real32 a) {
	return a * B;
}

inline v2 &operator*=(v2 &B, real32 a) {
	B = a * B;
	return B;
}

inline v2 &operator+=(v2 &A, v2 B) {
	A = A + B;
	return A;
}

inline v2 &operator-=(v2 &A, v2 B) {
	A = A - B;
	return A;
}

inline v2 operator/(v2 V, real32 a) {
	return (1 / a) * V;
}

inline v2 &operator/=(v2 &V, real32 a) {
	V = V / a;
	return V;
}

inline bool32 operator==(const v2& V, const v2& U){
	return V.X == U.X && V.Y == U.Y;
}

inline bool32 operator!=(const v2& V, const v2& U){
	return V.X != U.X || V.Y != U.Y;
}

inline real32 Inner(v2 A, v2 B) {
	return A.X*B.X + A.Y*B.Y;
}


inline real32 GetInnerEps(v2 A, v2 B, uint32 LowBitIdx)
{
  real32 XX = A.X*B.X;
  real32 YY = A.Y*B.Y;
  real32 Eps = GetEps(XX,YY,LowBitIdx);
  return Eps;
}

inline real32 EpsInner (v2 A, v2 B, uint32 LowBitIdx)
{
  real32 XX = A.X*B.X;
  real32 YY = A.Y*B.Y;
  real32 Eps = GetEps(XX,YY,LowBitIdx);
  real32 InnerProd = XX+YY;
  real32 Result = EpsilonEquals(InnerProd,0.f,Eps) ? 0.f : InnerProd;
  return Result;
}


inline real32 Det(v2 A, v2 B)
{
	return A.X*B.Y-A.Y*B.X;
}


inline v2 EstimateArithmeticError(ArithmeticOperation BinOp
                           , v2 Result, v2 Operand1, v2 Operand2, v2 Er_Opr1, v2 Er_Opr2)
{
    switch (BinOp)
    {
    case Multiplication:
    case Division:
        Assert(false);
        return V2(NAN,NAN);
    break;
    default:
        return v2{{ EstimateArithmeticError(BinOp, Result.X, Operand1.X, Operand2.X, Er_Opr1.X
                                            , Er_Opr2.X)
                 , EstimateArithmeticError(BinOp, Result.Y, Operand1.Y, Operand2.Y, Er_Opr1.Y
                                           , Er_Opr2.Y)}};
    }
}

inline v2 Epsilon_Inner(v2 A, v2 B, v2 ErrA, v2 ErrB){
    real32 Prod0 = A.X * B.X;
    real32 Err_Prod0 = EstimateArithmeticError(Multiplication, Prod0, A.X, B.X, ErrA.X, ErrB.X);
    real32 Prod1 = A.Y * B.Y;
    real32 Err_Prod1 = EstimateArithmeticError(Multiplication, Prod1, A.Y, B.Y, ErrA.Y, ErrB.Y);
    real32 Result = Prod0 + Prod1;
    real32 Err_Result = EstimateArithmeticError(Addition, Result, Prod0, Prod1, Err_Prod0, Err_Prod1);
    return V2(Result, Err_Result);
}

inline real32 LengthSquared(v2 A) {
	return Square(A.X) + Square(A.Y);
}

inline real32 LengthOf(v2 A) {
	return SquareRoot(LengthSquared(A));
}


inline int32 FindInArray(v2 V, v2 *Ar, int32 ArSiz)
{
    for (int32 ArIdx = 0; ArIdx < ArSiz ; ArIdx++)
    {
        if (Ar[ArIdx]==V)
            return ArIdx;
    }
    return -1;
}

inline bool32 ExistInArray(v2 V, v2 *Ar, int32 ArSiz)
{
    return FindInArray(V, Ar, ArSiz) >= 0;
}

inline bool32 ArePairwiseDistinct(v2 *Points, int32 NumPoints)
{
    for (v2 *CurP = Points; CurP < Points + NumPoints ; CurP++)
    {
        for(v2 *CmpP = Points; CmpP < Points + NumPoints ;CmpP++)
        {
            if (CurP == CmpP)
                continue;
            if (*CurP == *CmpP)
                return false;
        }
    }
    return true;
}



////////////////////////////////////////////////////////////
//Calculus - Linear Functions and Linear Spaces
////////////////////////////////////////////////////////////

/*meant to be used on linearly dependent vectors!
return value: 1 == same direction, -1 == opposite direction, 0 == at least one vector is zero*/
//int32 CompareDirection(v2 A, v2 AA);
//returns Errorcode on failure; 0 on success.
int ComputeIntersection(real32 Slope1, real32 Intercept1, real32 Slope2, real32 Intercept2
                        , v2 *Result, v2 *Result2);
int ComputeIntersection(real32 Slope1, real32 Intercept1, real32 Slope2, real32 Intercept2
                        , real32 Errors_Slope_Icept_1and2[4]
                        ,v2 *Result, v2 *Result2, v2 *IntersectionPointError);

int ComputeIntersection(v2 A1, v2 A2, v2 B1, v2 B2, v2 *Result, v2 *Result2);

/**
*Intercept = P0.Y- *Slope*P0.X;
*Intercept2 = P1.Y- *Slope*P1.X;

ReturnVal = -1; //Vertical
ReturnVal = -2; //Error: Invalid Input: Points are equal.
ReturnVal = -3; //Valid Result. Intercepts are not equal.
int32 ReturnVal = 0; //Regular result. Intercepts are equal.
*/
int32 LinearFunction(v2 P0, v2 P1, real32 *Slope, real32 *Intercept, real32 *Intercept2);


#define INTERSECTION_RESULT_PARALLEL_APART -1
#define INTERSECTION_RESULT_COLLINEAR -2
#define INTERSECTION_RESULT_VALID 0
#define INTERSECTION_RESULT_AMBIGUOUS -3
#define INTERSECTION_RESULT_INVALID -5
int ComputeIntersection(real32 Slope1, real32 Intercept1, real32 Slope2, real32 Intercept2
                        , v2 *Result);
int ComputeIntersection(v2 A1, v2 A2, v2 B1, v2 B2, v2 *Result);
int Debug_ComputeIntersection(real32 Slope1, real32 Intercept1, real32 Slope2, real32 Intercept2
                              , v2 *Result, v2 *IntersectionPointError);
int Debug_ComputeIntersection(v2 A1, v2 A2, v2 B1, v2 B2, v2 *Result, v2 *IntersectionPointError);
//int32 ClampPointToLineY(v2 *P, real32 Slope, real32 Intercept);
//int32 ClampPointToNormalSideOfEdge(v2 *P, v2 E0, v2 E1);


inline bool AreHorizontal(v2 A, v2 B, v2 C)
{
    return ((A.Y == B.Y) && (B.Y==C.Y));
}

inline bool AreVertical(v2 A, v2 B, v2 C)
{
    return ((A.X == B.X) && (B.X==C.X));
}


//returns Errorcode on failure; 0 on success.
//int ComputeIntersection_old(v2 A1, v2 A2, v2 B1, v2 B2, v2 *Result);

typedef v2 point2d;
typedef v2 linfn;
linfn LinFn(point2d A, point2d B, int *Error);

union ls2d{//linear space 2d
	struct
	{
		v2 Pos, Dir;
	};
	struct
	{
		v2 Position, Direction;
	};
	struct
	{
		v2 P0, P1;
	};
	v2 E[2];
	v2 P[2];


};

//ls2d LinSpace2D(v2 Pos, v2 Dir);
//ls2d PlumbOf(ls2d Line, bool32 Clockwise = true);
v2 OrthogonalOf(v2 V, bool32 Clockwise = true);
inline v2 GetNormal(v2 Vec) {return OrthogonalOf(Vec, false);};
void Normalize(v2 *u);
v2 Normalize(v2 u);
v2 ProjectOnto(v2 v, v2 u);
//TODO handle u==0
//real32 LengthRatio(v2 v, v2 u);
//real32 SignedLengthRatio(v2 v, v2 u);
//real32 SignedProjectionRatio(v2 v, v2 u);

typedef ls2d edge;

inline edge operator+(edge E, v2 V)
{
    return edge{.P0=E.P0+V,.P1=E.P1+V};
}

inline edge operator+(v2 V, edge E)
{
    return edge{.P0=E.P0+V,.P1=E.P1+V};
}

inline edge operator+=(edge& E, v2 V)
{
  return E = E+V;
}

struct edge_and_angle
{
    edge E;
    angle32 Rad;
};

inline bool32 operator==(edge lhs, edge rhs)
{
    return lhs.P0 == rhs.P0 && lhs.P1 == rhs.P1;
}

inline v2 GetDirection(edge E)
{
    return E.P1-E.P0;
}

inline edge ReverseEdge(const edge& E)
{
    return edge{.P0=E.P1,.P1=E.P0};
}

inline angle32 GetAngle(edge E)
{
    v2 D = GetDirection(E);
    return ATan2(D.Y,D.X);
}


inline real32 LengthOf(edge E)
{
    return LengthOf(E.P1 - E.P0);
}

inline real32 LengthSquared(edge E)
{
    return LengthSquared(E.P1 - E.P0);
}

/**
* computes angle between vectors from a to b in clockwise direction
*/
inline real32 MeasureAngleFromTo(v2 DirectionA, v2 DirectionB)
{
    real32 AngleA = ATan2(DirectionA.Y, DirectionA.X);
    real32 AngleB = ATan2(DirectionB.Y, DirectionB.X);
    real32 AngleFromAToB = MeasureAngleFromTo(AngleA, AngleB);
    return AngleFromAToB;
}

/**
Computes Angle between Directions from A towards B in Clockwise Direction
e.g. if A=={{0,0},{1,0}} and B=={{2,2},{2,3} then Angle = .5*Pi
*/
inline real32 MeasureAngleFromTo(edge A, edge B)
{
    v2 DirectionA = A.P1 - A.P0;
    v2 DirectionB = B.P1 - B.P0;
    return MeasureAngleFromTo(DirectionA, DirectionB);
}


typedef edge line_segment;
//bool32 LineSegmentContainsPoint(line_segment LS, v2 Point);
v2 CartesianToAngleRadius(v2 Cartesian);
v2 AngleRadiusToCartesian(v2 AngleRadius);




////////////////////////////////////////////////////////////
//v3 - 3D Vectors
////////////////////////////////////////////////////////////

typedef struct v3 {
	union {
		struct {
			real32 X, Y, Z;
		};
		real32 E[3];
	};
} v3;

inline v3 V3(real32 X, real32 Y, real32 Z) {
	return { .X=X,.Y=Y,.Z=Z };
}

inline v3 operator+(v3 A, v3 B) {
	v3 C;
	C.X = A.X + B.X;
	C.Y = A.Y + B.Y;
	C.Z = A.Z + B.Z;
	return C;
}

inline v3 operator-(v3 A, v3 B) {
	v3 C;
	C.X = A.X - B.X;
	C.Y = A.Y - B.Y;
	C.Z = A.Z - B.Z;
	return C;
}

inline v3 operator-(v3 A) {
	v3 V;
	V.X = -A.X;
	V.Y = -A.Y;
	V.Z = -A.Z;
	return V;
}

inline v3 operator*(real32 a, v3 B) {
	v3 C;
	C.X = a * B.X;
	C.Y = a * B.Y;
	C.Z = a * B.Z;
	return C;
}

inline v3 operator*(v3 B, real32 a) {
	return a * B;
}

inline v3 &operator*=(v3 &B, real32 a) {
	B = a * B;
	return B;
}

inline v3 &operator+=(v3 &A, v3 B) {
	A = A + B;
	return A;
}

inline v3 &operator-=(v3 &A, v3 B) {
	A = A - B;
	return A;
}

inline v3 operator/(v3 V, real32 a) {
	return (1 / a) * V;
}

inline v3 &operator/=(v3 &V, real32 a) {
	V = V / a;
	return V;
}


inline bool32 operator==(const v3& V, const v3& U){
	return V.X == U.X && V.Y == U.Y;
}

inline bool32 operator!=(const v3& V, const v3& U){
	return V.X != U.X || V.Y != U.Y;
}

inline real32 Inner(v3 A, v3 B) {
	return A.X*B.X + A.Y*B.Y + A.Z*B.Z;
}

inline real32 LengthSquared(v3 A) {
	return Square(A.X) + Square(A.Y) + Square(A.Z);
}

inline real32 LengthOf(v3 A) {
	return SquareRoot(LengthSquared(A));
}

void Normalize(v3 *u);
v3 Normalize(v3 u);

memory_index GetIndexOfClosestEdgeCCW(edge_and_angle& TargetE, edge *OtherEdges, angle32 *OtherAngles
                                             , memory_index NumEdges);

memory_index GetIndexOfClosestEdge(edge_and_angle& TargetE, edge *OtherEdges, angle32 *OtherAngles
                                             , memory_index NumEdges
                                             , ori_x FromSide, same_other Orientation);

memory_index GetIndexOfClosestEdgeFromCyclicInterval(edge_and_angle& TargetE, edge *OtherEdges
                                                     , angle32 *OtherAngles
                                                    , memory_index NumEdges
                                                    , ori_x FromSide, same_other Orientation
                                                    , memory_index StartIdx
                                                    , memory_index NumEdgesToCompare);


#endif // G_MATH_UTILITY_H
