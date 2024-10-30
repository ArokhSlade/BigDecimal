#ifndef G_ESSENTIALS_H
#define G_ESSENTIALS_H

#define internal_function static
#define local_persist static
#define global_variable static

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t i8;
typedef int16_t i6;
typedef int32_t i32;
typedef int64_t i64;
typedef int32 b32;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef uint8_t h8;
typedef uint16_t h16;
typedef uint32_t h32;
typedef uint64_t h64;

typedef size_t memory_index;

typedef float real32;
typedef double real64;

typedef float f32;
typedef double f64;


constexpr uint32 MAX_UINT32 = 0xFFFF'FFFF;
constexpr int32 MAX_INT32 = 0x7FFFFFFF;
constexpr int32 MIN_INT32 = 0xFFFFFFFF;
constexpr int32 INT32_OVERFLOW = MIN_INT32;  //TODO(##2024 07 03): wrong and useless (converting float to int does not guarantee these values, the behavior is undeined, in fact)
constexpr int32 INT32_UNDERFLOW = MAX_INT32; //TODO(##2024 07 03): wrong and useless

#ifdef __cplusplus
}
#endif // __cplusplus

/*2023 03 16: how to make this work just like arrays/pointers, i.e. the expression: &MyNArray[10] would return &Data[10] rather than address of a local variable
template <typename T>
struct n_array
{
    T *Data;
    uint32 *Count;

    T operator[](memory_index N)
    {
        return Data[N];
    }
};
*/

//simply to have a funciton name to set a breakpoint to for debugging.
inline int32 AssertionFailed()
{
    return 0;
}

inline int32 Assert(bool Condition)
{
    //NOTE:to activate assertions put Breakpoint here
    if (!Condition)
    {
        return AssertionFailed();
    }
    return 1;
}

inline int32 HardAssert(bool Condition)
{
    if (!Condition)
    {
        return *(int32 *)0x0;
    }
    return 1;
}

#define InvalidCodePath(Message) HardAssert(false)

inline uint32 SafeTruncateUInt64(uint64 Value)
{
    // TODO(casey): Defines for maximum values
    Assert(Value <= 0xFFFFFFFF);
    uint32 Result = (uint32)Value;
    return(Result);
}

inline uint32 SafeTruncateMemoryIndex(memory_index Value) {
    Assert(Value <= 0xFFFF'FFFF);
    uint32 Result = (uint32)Value;
    return Result ;
}


inline void Memset8(uint8 *Start, uint8 Value, memory_index Size)
{
    uint8 *p = Start;
    while(Size--)
    {
        *p++ = Value;
    }
    return;
}


#endif //G_ESSENTIALS_H
