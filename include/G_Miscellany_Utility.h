#ifndef G_MISCELLANY_UTILITY_H
#define G_MISCELLANY_UTILITY_H

#include "G_Essentials.h"
#include "..\lib\STB sprintf\stb_sprintf.h"
#include <cstdio>
#include <cstdlib> //std::strtof
#include <iostream> //std::ostream for array<T> :: operator<<()


#define SWAP(A,B, TYPE) do{TYPE t = A;\
A = B;\
B = t; \
}while(0);

enum same_other
{
    SAME, OTHER
};

enum ori_x
{
    LEFT,
    RIGHT
};

enum success_failure
{
    SUCCESS = 0,
    FAILURE = -1,
};

struct status_count
{
    success_failure Status;
    uint32 Count;
};

typedef status_count write_result;

/** direction of rotation: clockwise, counterclockwise */
enum cw_ccw
{
    CW,
    CCW
};

void Swap(float *a, float *b);

const uint32 BIT_SCAN_NO_HIT = 0xFFFF'FFFF;
uint32 BitScanReverse32(uint32);

template <typename uN>
uN BitScanReverse(uN Value) {
    int32 Position = sizeof(uN)*8 - 1;
    uN Mask= 0x1ull << Position;
    while (!(Mask&Value) && Mask != 0) {
        Mask >>= 1;
        --Position;
    }
    return Mask != 0 ? Position : BIT_SCAN_NO_HIT;
}

template <typename uN>
uN BitScanReverse2(uN Value) {
    int32 Position = sizeof(uN)*8 - 1;
    uN Mask= 0x1ull << Position;
    while (!(Mask&Value) && Mask != 0) {
        Mask >>= 1;
        --Position;
    }
    return Mask != 0 ? Position : BIT_SCAN_NO_HIT;
}

u64 BitScanReverseN(u64 Value, i8 n_bytes);

uint32 BitScan32(uint32);

/** returns index of highest set bit, i.e. between 31 and 0. returns 0xffff'ffff if no bit is set. */
template <typename uN>
uint32 BitScan(uN Value) {
    int32 Position = 0;
    uN Mask = 0x1;
    while (!(Mask&Value) && Mask != 0) {
        Mask <<= 1;
        ++Position;
    }
    return Mask != 0 ? Position : BIT_SCAN_NO_HIT;
}


inline void Toggle(bool32& BVal) {
    BVal = !BVal;
}


/**REQUIRED: 0 <= CurIdx < ExclusiveUpperBound */
void IncrementCyclically(memory_index& CurrentIndex, memory_index ExclusiveUpperBound)
;
void IncrementCyclically(memory_index& CurIdx, memory_index ExclusiveUpperBound
                         , memory_index Amount);
/**REQUIRED: 0 <= CurIdx < ExclusiveUpperBound */
memory_index CyclicSuccessor(memory_index CurrentIndex, memory_index ExclusiveUpperBound)
;
/**REQUIRED: 0 <= CurIdx < ExclusiveUpperBound */
memory_index CyclicPredecessor(memory_index CurIdx, memory_index ExclusiveUpperBound)
;
/**REQUIRED: 0 <= CurIdx < ExclusiveUpperBound */
void DecrementCyclically(memory_index& CurIdx, memory_index ExclusiveUpperBound)
;
void DecrementCyclically(memory_index& CurIdx, memory_index ExclusiveUpperBound
                         , memory_index Amount);
/** REQUIRED: Start and End: >= 0, < ExclusiveUpperBound */
memory_index CyclicDistance(memory_index Start, memory_index End, memory_index ExclusiveUpperBound)
;

template<typename T>
struct cyclic_iterator
{
    T *Data;
    memory_index NumElms;
    memory_index CurIdx;

    cyclic_iterator(T *Data, memory_index NumElms)
    : Data(Data), NumElms(NumElms), CurIdx(0) {}

    /**REQUIRED: 0 <= StartIdx < NumElms */
    cyclic_iterator(T *Data, memory_index NumElms, memory_index StartIdx)
    : Data(Data), NumElms(NumElms), CurIdx(StartIdx) {}

    cyclic_iterator& operator++()
    {
        IncrementCyclically(CurIdx, NumElms);
        return *this;
    }

    cyclic_iterator operator++(int)
    {
        cyclic_iterator Old = *this;
        ++*this;
        return Old;
    }

    cyclic_iterator& operator--()
    {
        DecrementCyclically(CurIdx, NumElms);
        return *this;
    }

    cyclic_iterator operator--(int)
    {
        cyclic_iterator Old = *this;
        --*this;
        return Old;
    }

    T Front() const
    {
        return Data[CurIdx];
    }



    T Pred() const
    {
        return Data[CyclicPredecessor(CurIdx,NumElms)];
    }

    T Succ() const
    {
        return Data[CyclicSuccessor(CurIdx,NumElms)];
    }

};

template<typename T>
bool32 BuffersOverlap(T *A, memory_index LenA, T *B, memory_index LenB)
{
    return A==B || (A < B && A+LenA >B) || (B<A && B+LenB > A);
}



success_failure MapNDigitsUIntToString(uint32 Number, char *DestStringBuffer, uint32 NumDigits);
success_failure MapNDigitsUIntToStringWithZeroPadding(uint32 Number, char *DestStringBuffer
                                                      , uint32 NumDigits, uint32 TargetWidth);


inline void Memset32(uint32 *Start, uint32 Value, memory_index Size)
{
    uint32 *p = Start;
    while(Size--)
    {
        *p++ = Value;
    }
    return;
}

inline void MemClear(uint8 *Start, memory_index Size)
{
    while (Size--)
    {
        *Start++ = 0;
    }
    return;
}

#define ZeroArray(Array) do{\
  memory_index ElmSize = sizeof(*Array);\
  memory_index ArraySize = ArrayCount(Array) * ElmSize;\
  memory_index WordCount = ArraySize/sizeof(uint32);\
  memory_index Remainder = ArraySize%sizeof(uint32);\
  Memset32((uint32 *)Array, 0, WordCount);\
  MemClear((uint8*)Array+ArraySize-Remainder,Remainder);\
}while(0);

#define ZeroArrayPtr(Array,Count) do{\
  memory_index ElmSize = sizeof(*Array);\
  memory_index ArraySize = Count * ElmSize;\
  memory_index WordCount = ArraySize/sizeof(uint32);\
  memory_index Remainder = ArraySize%sizeof(uint32);\
  Memset32((uint32 *)Array, 0, WordCount);\
  MemClear((uint8*)Array+ArraySize-Remainder,Remainder);\
}while(0);

#define ResetArray(Array,type) do{\
  for (type *Ptr = Array; Ptr < Array+ArrayCount(Array) ; ++Ptr)\
  {\
    *Ptr={};\
  }\
}while(0);

#define ResetArrayPtr(Array,Count,type) do{\
  for (type *Ptr = Array; Ptr < Array+Count ; ++Ptr)\
  {\
    *Ptr={};\
  }\
}while(0);

template <typename T>
size_t FindInArray(T *Array, size_t ArrayLength, T Value)
{
    size_t i = 0;
    for (T *P =Array; P < Array+ArrayLength ; P++)
    {
        if (*P == Value)
        {
            return i;
        }
        i++;
    }
    return -1;
}

uint32 MemCopy32(uint32 *Src, uint32 *Dst, uint32 Len);
uint32 MemCopy8(uint8 *Src, uint8 *Dst, uint32 Len);

#define CopyArray(Src,Dst) MemCopy8((uint8*)Src,(uint8*)Dst,ArrayCount(Src)*sizeof(Src))



template <typename t>
void ReverseArray(t* Ts, uint32 NumT)
{
    t Temp{};
    for (uint32 i = 0 ; i < NumT/2 ; ++i)
    {
        Temp = Ts[i];
        Ts[i] = Ts[NumT-1-i];
        Ts[NumT-1-i] = Temp;
    }
    return;
}

inline real32 ParseReal32(char *Text, char **End=nullptr)
{
  return std::strtof(Text,End);
}

inline uint32 ParseUInt32(char *Text, char **End=nullptr, uint32 Base=10)
{
  return std::strtoul(Text,End,Base);
}

/** SET the N-th bit in an array of 32bit-blocks
 *  e.g. N = 0 means bit 0 of block 0. N=32 means bit 0 of block 1 ...
 *
**/
inline void SetBit32(uint32 *Array, memory_index ArrayLength, uint32 N) {
    memory_index BlockIdx = N / 32;
    HardAssert(BlockIdx < ArrayLength);
    uint32 BitIdx = N % 32;
    Array[BlockIdx] |= 1 << (BitIdx);
}

/** READ the N-th bit from an array of 32bit-blocks
 *  e.g. N = 0 means bit 0 of block 0. N=32 means bit 0 of block 1 ...
 *
*/
inline bool32 GetBit32(uint32 *Array, memory_index ArrayLength, uint32 N) {
    memory_index BlockIdx = N / 32;
    HardAssert(BlockIdx < ArrayLength);
    uint32 BitIdx = N % 32;
    bool32 Result = Array[BlockIdx] & 1 << BitIdx;
    return Result;
}


template <class T>
struct array{
    T *Ptr;
    int32 Cnt;
    int32 Cap;
};


template <class T>
inline array<T> Array(T *Ptr_, int32 Cnt_) {
    return array<T>{Ptr_, Cnt_, Cnt_};
}

template <class T>
b32 operator==(array<T>& A, array<T>& B) {
    if (A.Cnt != B.Cnt) return false;

    for (int i=0 ; i < A.Cnt ; ++i) {
        if (A.Ptr[i] != B.Ptr[i]) return false;
    }
    return true;
}

using std::ostream;
template <class T>
ostream& operator<<(ostream& os, const array<T>& A) {
    os << "{";
    i32 i=0;
    if (A.Cnt>0) os << A.Ptr[i++];
    while (i < A.Cnt) {
        os << ", " << A.Ptr[i++];
    }
    os << "}";
    return os;
}


template<class T>
struct RemoveReference{typedef T type;};
template<class T>
struct RemoveReference<T&>{typedef T type;};
template<class T>
struct RemoveReference<T&&>{typedef T type;};

#define AsArray(X) (array<RemoveReference<decltype((X)[0])>::type>{(&X[0]), ArrayCount(X), ArrayCount(X)})

inline auto Contains (auto *Array_, int32 n, auto x) -> bool32 {
    for (int32 i = 0 ; i < n ; ++i) {
        if (Array_[i]==x) return true;
    }
    return false;
}

#define ArrayContains(Array,x) Contains(Array,ArrayCount(Array),x)

template <class T>
inline auto Contains (array<T> Array, T X) -> bool32 {
    return Contains(Array.Ptr, Array.Cnt, X);
}

/** \brief Where are we and what do we want to skip?*/
template<class T>
inline T* SkipAny(T *Where, int32 WhereCount, T* What, int32 WhatCount) {
    for (int32 i = 0 ; WhereCount==0 || i < WhereCount ; ++i) {
        if (Contains(What, WhatCount,*Where)) ++Where;
        else break;
    }
    return Where;
}

template<>
inline char* SkipAny(char *Where, int32 WhereCount, char* What, int32 WhatCount) {
    for (int32 i = 0 ; *Where!='\0' && (WhereCount==0 || i < WhereCount) ; ++i) {
        if (Contains(What, WhatCount, *Where)) ++Where;
        else break;
    }
    return Where;
}

template<>
inline const char* SkipAny(const char *Where, int32 WhereCount, const char* What, int32 WhatCount) {
    for (int32 i = 0 ; *Where!='\0' && (WhereCount==0 || i < WhereCount) ; ++i) {
        if (Contains(What, WhatCount, *Where)) ++Where;
        else break;
    }
    return Where;
}


//template <class T>
//inline T* SkipAny(T *Where, int32 WhereCount, array<T> What) {
//    return SkipAny(Where, WhereCount, What.Ptr, What.Cnt);
//}

template <class T>
inline T* SkipAny(array<T> Where, array<T> What) {
    return SkipAny(Where.Ptr, Where.Cnt, What.Ptr, What.Cnt);
}

template <typename T>
inline void MemCopy(T *Src, T *Dst, i32 Size) {
    HardAssert(!!Src && !! Dst && Size>=0);
    char *Cur = Src;
    for (i32 i = 0 ; i < Size ; ++i) {
        *Dst++ = *Cur++;
    }
    return;
}

typedef uint32 flags32;


inline bool32 IsSet(flags32 Flags, uint32 BitVal) {
    HardAssert(BitVal%2 == 0 || BitVal==1);
    bool32 Result = Flags & BitVal;
    return Result;
}
inline void Unset(flags32& Flags, uint32 BitVal) {
    HardAssert(BitVal%2 == 0 || BitVal==1);
    Flags &= ~BitVal;
}

inline void Set(flags32& Flags, uint32 BitVal) {
    HardAssert(1 <= BitVal && BitVal <= 31);
    Flags |= BitVal;
}

inline void ToggleFlag(flags32& Flags, uint32 BitVal) {
    HardAssert(BitVal%2 == 0 || BitVal==1);
    uint32 Mask = Flags & ~BitVal;
    Flags = Mask | ((Flags & BitVal) ^ BitVal);
}

template<typename T>
struct OneLink;

template<typename T>
std::ostream& operator<<(std::ostream& os, OneLink<T> *link);
template<typename T>
std::ostream& operator<<(std::ostream& os, OneLink<T>& link);

template <typename T>
struct OneLink{
    T value;
    OneLink<T> *next = nullptr;

    friend std::ostream& operator<< <>(std::ostream& os, OneLink<T>& link);
    friend std::ostream& operator<< <>(std::ostream& os, OneLink<T> *link);

    i32 length() {
        if (!this) return 0;
        i32 count = 1 ;
        OneLink *cur = this;
        while (cur = cur->next) {
            ++count;
        }
        return count;
    }
};


//TODO(ArokhSlade##2024 09 01): if you to implement this properly, knock myself out!
template <typename T>
struct OneList{
    OneLink<T> *Head;

//    friend std::ostream& operator<<(std::ostream& os, OneList<T>&);
};

enum class Copy_Style {
    SHALLOW,
    DEEP
};

template<std::integral uN>
uN GetMaskBottomN(i32 n_bits) {
    u32 width = sizeof(uN) * 8;
    HardAssert(n_bits >= 0 && n_bits <= width);
    uN result = 0;
    if (n_bits > 0) {
        result = ~0ull;
        result >>= width - n_bits;
    }
    return result;
}

template<std::integral T_Int>
inline T_Int DivCeil(T_Int dend, T_Int dsor){
    T_Int result = (dend + dsor - 1) / dsor;
    return result;
}

template<std::integral T_Int>
inline T_Int SafeShiftRight(T_Int value, i8 shift_amount) {
    i32 width = sizeof(T_Int) * 8;
    if (shift_amount <= 0 ) return value;
    if (shift_amount >= width) return 0;
    return value >> shift_amount;
}

template<std::integral T_Int>
inline T_Int SafeShiftLeft(T_Int value, i8 shift_amount) {
    i32 width = sizeof(T_Int) * 8;
    if (shift_amount <= 0 ) return value;
    if (shift_amount >= width) return 0;
    return value << shift_amount;
}

template <typename T>
struct TypeDetector;

#include "G_StringProcessing.h";
#include "G_MiscStdExtensions_Utility.h"
#endif // G_MISCELLANY_UTILITY_H
