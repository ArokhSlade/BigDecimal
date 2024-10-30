#ifndef __G_ERROR_ESTIMATION_H
#define __G_ERROR_ESTIMATION_H

#include "G_Essentials.h"

//#include <csignal>
#include <cfenv> //FE_INEXACT
enum ArithmeticOperation
{
    Addition, Subtraction, Multiplication, Division
};

//DELETEME 2023 11 16 this is neither used nor needed?
#if 0
enum class fp_exc
{
    DIVBYZERO = FE_DIVBYZERO
    ,INEXACT = FE_INEXACT
    ,INVALID = FE_INVALID
    ,OVERFLOW = FE_OVERFLOW
    ,UNDERFLOW = FE_UNDERFLOW
    ,ALL_EXCEPT = FE_ALL_EXCEPT
};
#endif

real32 half_ulp(real32 x);
real32 get_rounding_error(ArithmeticOperation Op, real32 A, real32 B);
real32 EstimateArithmeticError(ArithmeticOperation BinOp, real32 Result, real32 Operand1, real32 Operand2
                               , real32 Er_Opr1, real32 Er_Opr2);



static const real32 mantissa_width = 24.f;

///G_Math_Utility.h needs to know about get_rounding_error(..) and ArithmeticOperation{..}
#include "G_Math_Utility.h"

inline real32 float_exponent(real32 x)
{
    return floor(log2f(Abs(x)));
}

inline real32 half_ulp(real32 x)
{
    if (x == 0)
        return 0;
    real32 AbsX = Abs(x);
    real32 Log2X = log2f(AbsX);
    real32 Exp = Log2X-24;
    real32 FExp = floor(Exp);
    real32 Result = std::pow(2,FExp);
    return Result;
}

inline void reset_fpexc()
{
    feclearexcept(FE_ALL_EXCEPT);
}



inline bool32 signs_not_equal_both_nonzero(real32 a,real32 b)
{
    return (a < 0 && b > 0) || (a > 0 && b < 0);
}

inline bool32  signs_equal_both_nonzero(real32 a,real32 b)
{
    return (a > 0 && b > 0) || (a < 0 && b < 0);
}

inline real32 get_rounding_error(ArithmeticOperation Op, real32 A, real32 B)
{
    reset_fpexc();
    real32 Result=0.f;
    switch (Op)
    {
    case Addition:
        {
            Result = A+B;
        }
        break;
    case Subtraction:
        {
            Result = A-B;
        }
        break;
    case Multiplication:
        {
            Result = A*B;
        }
        break;
    case Division:
        {
            Result = A/B;
        }
        break;
    }
    real32 RoundingError = fetestexcept(FE_INEXACT)
                            ? half_ulp(Result) : 0.f;
    return RoundingError;
}
#endif // __G_ERROR_ESTIMATION_H
