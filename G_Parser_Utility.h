#ifndef G_PARSER_UTILITY_H
#define G_PARSER_UTILITY_H

#include "G_Essentials.h"
#include "G_MemoryManagement_Service.h"
#include "G_Math_Utility.h"
#include "G_PlatformGame_2_Module.h"

namespace parse {
    static constexpr flags32 DEFAULT = 0;
    static constexpr flags32 OPTIONAL = 1<<0;
    static constexpr flags32 COMMA = 1<<1;
}

enum class parse_status {
    WIN, FAIL
};

/**\note default value evaluates to `true`, because:
   \n failing to parse is the exceptional case
   \n a failed parse cannot turn into a succesful one
   \n but a thereto-succesful parse could fail at any point
 */
struct parse_info {
    parse_status Status = parse_status::WIN;
    const char *After;

    operator bool32() {
        return Status == parse_status::WIN;
    }
};

parse_info ParseInt32(const char *Begin, int32 *Buf);
parse_info ParseIntegralPart(const char *Begin, real32 *Buf);



//TODO(ArokhSlade##2024 08 13): where does this belong?
struct arena_allocator {
    //TODO(##2023 12 08): re-enable the const. disabled it because it caused the default operator= to be ill-formed and thus deleted,
    //                    which in turn caused big_int's operator= to be deleted also.
    memory_arena */*const*/ Arena;
    void *Allocate(memory_index Size) {
        HardAssert(Arena != nullptr);
        return PushSize_(Arena, Size);
    }
};

template <typename t_allocator>
struct big_int;

/**\note Allocator needs to be given a value, everything else can be left to default initialization.
   \brief chunks are stored in ascending order, i.e. Data.Value == least significant digits.
       \n most significant digits are at ::GetChunk(Length-1).Value
 */
template <typename t_allocator>
struct big_int {
    static t_allocator AllocatorForTemporaries;
    static big_int<t_allocator> TempAddF;
    static big_int<t_allocator> TempSub;
    static big_int<t_allocator> TempSubF;
    static big_int<t_allocator> TempMul0;
    static big_int<t_allocator> TempMul1;
    static big_int<t_allocator> TempMulCarries;
    static big_int<t_allocator> TempOne;
    static big_int<t_allocator> TempDivA;
    static big_int<t_allocator> TempDivB;
    static big_int<t_allocator> TempDiv0;
    static big_int<t_allocator> TempDivF;
    static big_int<t_allocator> TempDivInteger;
    static big_int<t_allocator> TempDivFraction;
    static big_int<t_allocator> TempPow10;
    static big_int<t_allocator> TempTen;
    static big_int<t_allocator> TempDigit;
    static big_int<t_allocator> TempToFloat;
    static big_int<t_allocator> TempFrac;

    static bool32 WasInitialized;

    t_allocator Allocator;
    int32 Length = 1;

    bool32 IsNegative = false;
    int32 Exponent = 0;

    uint32 ChunkCount = 1;
    struct chunk_list {
        uint32 Value;
        chunk_list *Next,
                   *Prev;
    } Data = {}; //TODO(##2023 11 19): call it Chunks or ChunkList maybe? or Segments? Pieces, Parts, Spans, ...???
    //TODO(##2024 06 15): replace chunk list with something more performant and dynamic like dynamic arrays or a container template

    //TODO(##2023 11 24): Naming: `Last` is ambiguous. What if someone wants the one at index Length-1? this->Last refers to the last allocated block in the list, not the last used block in the number.
    chunk_list *Last = &Data;

//    big_int FromString(const char *Str) {
//        big_int Result;
//
//        return Result;
//    }

    enum class status {
        INVALID = 0,
        VALID = 1
    };

    auto GetChunk(uint32 Idx) -> chunk_list*;
    auto ExpandCapacity() -> chunk_list*;
    auto ExtendLength() -> chunk_list*;

    //TODO(##2023 11 24): push might allocate, so I'd prefer it to return a status (enable error handling). I'd love to chain calls though: X.Push(1).Push(2)...
    //explore: is it viable to return a wrapper (template) who can act like the thing it wraps? does exist overload for `operator.()` ? -> no. only operator* and operator->
    //there's ways to do this in Dlang however... (opDispatch, alias this)
    //NOTE(##2024 06 15): Push superceded by Append? -> Not in its current form
    auto Push(uint32) -> big_int&;

    enum class temp_id{
        Temp_ParseF32,
    };

    static auto InitializeContext (t_allocator *Allocator) -> void;
    static auto GetTemp(temp_id Temp) -> big_int<t_allocator>&;
    auto GetLeastSignificantExponent() -> int32;
    auto Normalize() -> void;
    auto Trim() -> void;
    auto IsNormalized() -> bool;
    auto Append(big_int& Other) -> void;
    auto Set(uint32 Val,  bool32 IsNegative=0, int32 Exponent=0 ) -> big_int&;
    auto Set(uint32 *ValArray, uint32 ValCount, bool32 IsNegative=false, int32 Exponent=0) -> big_int&;
    auto Set(big_int<t_allocator>& Other) -> big_int&;
    auto SetFloat(real32 Val) -> void;
    auto AddU (big_int& B) -> big_int::status;
    auto Add (big_int& B) -> big_int::status;
    auto Sub (big_int& B) -> big_int::status;
    auto ShiftLeft(uint32 ShiftAmount) -> big_int&;
    auto ShiftRight(uint32 ShiftAmount) -> big_int&;
    auto Mul (big_int& B) -> big_int::status;
    auto Div (big_int& B, uint32 MinFracPrecision=32) -> void;
    auto TruncateTrailingZeroBits() -> void;
    auto TruncateTrailingZeroChunks() -> void;
    auto TruncateLeadingZeroes() -> void;
    auto GetBit(int32 N) -> bool;
    auto RoundToNSignificantBits(int32 N) -> void;
    auto AddF (big_int& B) -> void;
    auto SubF (big_int& B) -> void;
    auto MulF (big_int& B) -> void;
    auto DivF (big_int& B, uint32 MinFracPrecision=32) -> void;
    auto Neg () -> void;
    auto Zero() -> void;
    auto CopyTo(big_int *Dest) -> big_int<t_allocator>::status;
    auto ToFloat () -> float;
    auto ToFloatPrecision () -> void;
    auto to_string () -> std::string;

    big_int(t_allocator AnAllocator, int32 AValue)
    : Allocator{AnAllocator}, IsNegative{AValue < 0},
      Data{.Value{AValue < 0 ? -AValue : AValue}}
      {}
    big_int(t_allocator AnAllocator, uint32 AValue)
    : Allocator{AnAllocator}, IsNegative{false},
      Data{.Value{AValue}}
      {}


    big_int(t_allocator AnAllocator)
    : Allocator{AnAllocator}, IsNegative{false},
      Data{.Value{0}}
      {}

    template <typename t_allocator_arg>
    big_int(t_allocator_arg Arg, int32 AValue)
    : big_int{t_allocator{Arg}, AValue} {}

    template <typename t_allocator_arg>
    big_int(t_allocator_arg Arg, uint32 AValue)
    : big_int{t_allocator{Arg}, AValue} {}

    template <typename t_allocator_arg>
    big_int(t_allocator_arg Arg)
    : big_int{t_allocator{Arg}} {}


    big_int(t_allocator Allocator, big_int& Other)
    : Allocator{Allocator}, IsNegative{Other.IsNegative}, Exponent{Other.Exponent}{
        this->Data.Value = Other.Data.Value;
        chunk_list *Ours = &this->Data;
        chunk_list *Theirs = &Other.Data;
        while (this->Length < Other.Length) {
            Ours = this->ExtendLength();
            Theirs = Theirs->Next;
            Ours->Value = Theirs->Value;
        }
    }



    private:
    big_int() {}
    big_int(const big_int& Other) {}
};

template <typename t_allocator>
auto Str(big_int<t_allocator>& A, memory_arena *TempArena) -> char* {

    char Sign = A.IsNegative ? '-' : '+';
    typename big_int<t_allocator>::chunk_list *CurChunk = A.GetChunk(A.Length-1);
    int32 UnusedChunks = A.ChunkCount - A.Length;
    char HexDigits[11] = "";

    int32 UnusedChunksFieldSize = 2 + ( UnusedChunks > 1 ? UnusedChunks : 1);
    int32 BufSize = 1+A.Length*9+2+UnusedChunksFieldSize+1;
    char *Buf = PushArray(TempArena, BufSize, char);

    char *BufPos=Buf;
    *BufPos++ = Sign;

    for (int32 Idx = 0 ; Idx < A.Length; ++Idx, CurChunk = CurChunk->Prev) {
        stbsp_sprintf(BufPos, "%.8x ", CurChunk->Value);
        BufPos +=9;
    }

    stbsp_sprintf(BufPos, "[%.*d]", UnusedChunks, UnusedChunksFieldSize);
    BufPos += UnusedChunksFieldSize;

    *BufPos = '\0';

    return Buf;
}


template <typename t_allocator>
bool32 IsNormalizedZero(big_int<t_allocator>& A) {
    return A.Length == 1 && A.Data.Value == 0x0 && A.Exponent == 0;
}

template <typename t_allocator>
bool32 IsZero(big_int<t_allocator>& A) {
    for ( typename big_int<t_allocator>::chunk_list *Cur = &A.Data,
          *OnePastLast=A.GetChunk(A.Length-1)->Next ; Cur != OnePastLast ; Cur = Cur->Next ) {
        if (Cur->Value != 0) return false;
    }
    return true;
}


//
template <typename t_allocator>
bool32 CheckZeroAndNormalize(big_int<t_allocator>& A) {
    typename big_int<t_allocator>::chunk_list *Cur = &A.Data;
    bool32 IsZero = true;
    for ( int i = 0 ; i < A.Length ; ++i, Cur = Cur->Next ) {
        IsZero &= Cur->Value == 0;
    }
    if (IsZero) {
        A.Zero();
    }
    return A.Length == 1 && A.Data.Value == 0x0;
}



template <typename t_allocator>
bool32 big_int<t_allocator>::WasInitialized {false};

template <typename t_allocator>
big_int<t_allocator> big_int<t_allocator>::TempAddF{};

template <typename t_allocator>
big_int<t_allocator> big_int<t_allocator>::TempSub {};

template <typename t_allocator>
big_int<t_allocator> big_int<t_allocator>::TempSubF {};

template <typename t_allocator>
big_int<t_allocator> big_int<t_allocator>::TempMul0 {};

template <typename t_allocator>
big_int<t_allocator> big_int<t_allocator>::TempMul1 {};

template <typename t_allocator>
big_int<t_allocator> big_int<t_allocator>::TempMulCarries {};

template <typename t_allocator>
big_int<t_allocator> big_int<t_allocator>::TempOne {};

template <typename t_allocator>
big_int<t_allocator> big_int<t_allocator>::TempDivA{};

template <typename t_allocator>
big_int<t_allocator> big_int<t_allocator>::TempDivB{};

template <typename t_allocator>
big_int<t_allocator> big_int<t_allocator>::TempDiv0{};

template <typename t_allocator>
big_int<t_allocator> big_int<t_allocator>::TempDivF{};

template <typename t_allocator>
big_int<t_allocator> big_int<t_allocator>::TempDivInteger{};

template <typename t_allocator>
big_int<t_allocator> big_int<t_allocator>::TempDivFraction{};

template <typename t_allocator>
big_int<t_allocator> big_int<t_allocator>::TempPow10{};

template <typename t_allocator>
big_int<t_allocator> big_int<t_allocator>::TempTen{};

template <typename t_allocator>
big_int<t_allocator> big_int<t_allocator>::TempDigit{};

template <typename t_allocator>
big_int<t_allocator> big_int<t_allocator>::TempToFloat{};

template <typename t_allocator>
big_int<t_allocator> big_int<t_allocator>::TempFrac{};

template <typename t_allocator>
t_allocator big_int<t_allocator>::AllocatorForTemporaries;


/**
 * \note will return nullptr if given index exceeds capacity
 * \note may return a chunk beyond `Length`, i.e. that's currently not in use,
 * i.e. that currently doesn't contain a part of the number's current value
 */
template <typename t_allocator>
auto big_int<t_allocator>::GetChunk(uint32 Idx) -> chunk_list * {
    chunk_list *Result = nullptr;
    if (Idx < ChunkCount) {
        Result = &Data;
        while (Idx-- > 0) {
            Result = Result ->Next;
        }
    }
    return Result;
}

/**
 * \brief Allocates another chunk
 */
template <typename t_allocator>
//TODO(## 2023 11 23): support List nodes that are multiple segments long?
auto big_int<t_allocator>::ExpandCapacity() -> chunk_list* {
    chunk_list *NewChunk = (chunk_list*)Allocator.Allocate(sizeof(chunk_list));
    HardAssert(NewChunk != nullptr);
    *NewChunk = {.Value = 0, .Next = nullptr, .Prev = nullptr};
    Last->Next = NewChunk;
    NewChunk->Prev = Last;
    Last = NewChunk;
    ++ChunkCount;

    return NewChunk;
}

/**
 * \brief increase length, allocate when appropriate, return pointer to Last chunk (index == Length-1) in list.
 *     \n NOTE: Sets the new chunk's value to Zero.
 */
template <typename t_allocator>
auto big_int<t_allocator>::ExtendLength() -> chunk_list* {
    HardAssert(Length <= ChunkCount);
    ++Length;
    chunk_list *Result =  GetChunk(Length-1);
    if (Result == nullptr) {
        Result = ExpandCapacity();
    }
    Result->Value = 0;

    return Result;
}

/** Unisgned Add, pretend both numbers are positive integers, ignore exponents */
template <typename t_allocator>
//TODO(## 2023 11 18) : test case where allocator returns nullptr
auto big_int<t_allocator>::AddU (big_int& B) -> big_int::status {

    status Result = status::VALID;
    big_int& A = *this;
    uint32 LongerLength =  A.Length >= B.Length ? A.Length : B.Length;
    uint32 ShorterLength =  A.Length <= B.Length ? A.Length : B.Length;
    chunk_list *SegA = &A.Data, *SegB = &B.Data;


    uint32 OldCarry = 0;
    //TODO(##2023 11 23): if (A.Length < B.Length) {/*allocate and append B.Length-A.Length segments}
    for ( uint32 Idx = 0
        ; Idx < LongerLength
        ; ++Idx,
          SegA = SegA ? SegA->Next : nullptr,
          SegB = SegB ? SegB->Next : nullptr ) {
        if ( Idx >= A.Length)  {
//NOTE(## 2023 11 18): commented out because current error handling style is to HardAssert(/*NoErrorsOrCrash*/)  (happens in AppendAndClear(/))
//                    if (!NewChunk) {
//                        this->IsValid = false;
//                        return status::INVALID;
//                    }
            SegA = A.ExtendLength();
        }
        uint32 SegBValue = Idx < B.Length ? SegB->Value : 0U;
        bool32 NewCarry = MAX_UINT32 - SegBValue - OldCarry < SegA->Value;
        SegA->Value += SegBValue + OldCarry; //wrap-around is fine
        OldCarry = NewCarry;
    }
    if (OldCarry) {
        SegA = ExtendLength();
        SegA->Value = OldCarry;
    }

    return Result;
}

template <typename t_allocator>
//TODO(## 2023 11 18) : test case where allocator returns nullptr
auto big_int<t_allocator>::Add (big_int& B) -> big_int::status {

    status Result = status::VALID;
    big_int& A = *this;
    if (A.IsNegative == B.IsNegative) {
        return AddU(B);
    } else if (A.IsNegative) {
        A.Neg();
        Result = A.Sub(B);
        A.Neg();
    } else { /* (B.IsNegative) */
        B.Neg();
        Result = A.Sub(B);
        B.Neg();
    }
    return Result;
}

template <typename t_allocator>
auto big_int<t_allocator>::TruncateTrailingZeroBits() -> void {
    if (IsZero(*this)) { Length = 1; return; }
    int32 TruncCount = 0;
    int32 FirstOne = 0;
    bool32 BitFound = false;
    chunk_list *Current = &this->Data;
    for (int32 Block = 0 ; Block < Length; ++Block, Current = Current -> Next) {
        FirstOne = BitScan32(Current->Value);
        if (FirstOne != BIT_SCAN_NO_HIT) {
            TruncCount += FirstOne;
            BitFound = true;
            break;
        }
        TruncCount += 32;
    }
    ShiftRight(TruncCount);
    return;
}


template <typename t_allocator>
auto big_int<t_allocator>::TruncateTrailingZeroChunks() -> void {
    int32 TruncCount = 0;
    int32 FirstOne = 0;
    bool32 BitFound = false;
    chunk_list *Current = &this->Data;
    for (int32 Block = 0 ; Block < Length; ++Block, Current = Current -> Next) {
        FirstOne = BitScan32(Current->Value);
        if (FirstOne != BIT_SCAN_NO_HIT) {
            BitFound = true;
            break;
        }
        TruncCount += 32;
    }
    ShiftRight(TruncCount);
    return;
}


template <typename t_allocator>
auto big_int<t_allocator>::TruncateLeadingZeroes() -> void {
    for (chunk_list *Cur{ GetChunk(Length-1) } ; Cur != &Data ; Cur = Cur->Prev )
    {
        if (Cur->Value == 0x0) --Length;
        else break;
    };
    HardAssert (Length >= 1);
}

/** \brief Gets rid of leading zero chunks and trailing zeroes, exponent doesn't change, e.g. 10100 E=2 becomes 101 E=2, NOT 101 E=4.
    \n that's because the bits are interpreted as 1.0100E2 */
template <typename t_allocator>
auto big_int<t_allocator>::Normalize() -> void {
    TruncateLeadingZeroes();
    TruncateTrailingZeroBits();
}


template <typename t_allocator>
auto big_int<t_allocator>::Trim() -> void {
    TruncateLeadingZeroes();
    TruncateTrailingZeroChunks();
}

template <typename t_allocator>
auto big_int<t_allocator>::IsNormalized() -> bool {
    if (IsNormalizedZero(*this)) return true;
    if ((Data.Value&0x1) == 0 || GetChunk(Length-1)->Value == 0x0) return false;
    return true;
}



template <typename t_allocator>
auto big_int<t_allocator>::CopyTo(big_int *Dest) -> big_int<t_allocator>::status {
    //TODO(## 2023 11 23): replace assert with other error handling?
    HardAssert(Dest != nullptr);
    big_int<t_allocator>::status Result = Dest ? status::VALID : status::INVALID;

    if (Result == status::VALID) {
        Dest->IsNegative = this->IsNegative;
        Dest->Exponent = this->Exponent;
        chunk_list *Ours = &Data, *Theirs = &Dest->Data;
        while (Dest->Length < Length) Dest->ExtendLength();
        Dest->Length = Length; //TODO(##2024 07 13):redundant, ExtendLength() takes care of that?
        for ( uint32 Idx = 0 ; Idx < Length ; ++Idx, Ours = Ours->Next, Theirs=Theirs->Next ) {
            Theirs->Value = Ours->Value;
        }
    }

    return Result;
}

template <typename t_allocator>
struct abs_big_int {
    big_int<t_allocator> *Int;
};

template <typename t_allocator>
abs_big_int<t_allocator> Abs(big_int<t_allocator> *A) {
    return abs_big_int<t_allocator>{A};
}


template <typename t_allocator>
bool32 LessThan(big_int<t_allocator> *A, big_int<t_allocator> *B) {
    bool32 Result = false;
    if (A->IsNegative != B->IsNegative) {
        Result = A->IsNegative;
    } else {
        Result = LessThan<t_allocator>(Abs<t_allocator>(A), Abs<t_allocator>(B)) && !A->IsNegative;
    }
    return Result;
}


template <typename t_allocator>
bool32 LessThan(abs_big_int<t_allocator> const& AbsA, abs_big_int<t_allocator> const& AbsB) {

    bool32 Result = false;
    uint32 Length = AbsA.Int->Length;
    if (AbsA.Int->Length == AbsB.Int->Length) {
        for ( typename big_int<t_allocator>::chunk_list
                *A = AbsA.Int->GetChunk(Length-1),
                *B = AbsB.Int->GetChunk(Length-1)
            ; Length > 0
            ; A = A->Prev, B = B->Prev, --Length) {
            if (A->Value == B->Value) {
                continue;
            }
            Result = A->Value < B->Value;
            break;
        }
    } else {
        Result = AbsA.Int->Length < AbsB.Int->Length;
    }

    return Result;
}

template <typename t_allocator>
bool32 Equals(big_int<t_allocator> const& A, big_int<t_allocator> const& B) {
    bool32 Result = A.IsNegative == B.IsNegative && A.Length == B.Length && A.Exponent == B.Exponent;

    uint32 Idx = 0;

    for ( typename big_int<t_allocator>::chunk_list const*ChunkA = &A.Data,
                                                         *ChunkB = &B.Data
        ; Idx < A.Length
          && ( Result &= ChunkA->Value == ChunkB->Value )
        ; ++Idx,
          ChunkA = ChunkA->Next,
          ChunkB = ChunkB->Next ) {
        //all work done inside header
        continue;
    }

    return Result;
}

//TODO(##2024 06 08) param Allocator not used
template <typename t_allocator>
bool32 GreaterEquals(big_int<t_allocator> & A, big_int<t_allocator> & B) {
    return LessThan( Abs<t_allocator>( &B ), Abs<t_allocator>( &A ) ) || Equals( A,B );
}


template <typename t_allocator>
auto big_int<t_allocator>::Sub (big_int& B)-> big_int::status {

    status Result = status::VALID;
    big_int& A = *this;
    uint32 OriginalLength = A.Length;
    uint32 ShorterLength = A.Length <= B.Length ? A.Length : B.Length;
    uint32 LongerLength =  A.Length >= B.Length ? A.Length : B.Length;

    uint32 LeadingZeroBlocks = 0;
    if (!A.IsNegative && !B.IsNegative) {
        chunk_list *ChunkA = &A.Data;
        chunk_list *ChunkB = &B.Data;
        uint32 OldCarry = 0;
        uint32 NewCarry = 0;
        if (!LessThan<t_allocator>(Abs<t_allocator>(&A),Abs<t_allocator>(&B))) {
            for (uint32 Idx = 0 ; Idx < ShorterLength ; ++Idx) {
                OldCarry = NewCarry;
                NewCarry = (ChunkA->Value < ChunkB->Value) || ((ChunkA->Value == ChunkB->Value) && OldCarry ); //this handles cases like A==B==0xffffffff with Carry==1
                ChunkA->Value -= ChunkB->Value+OldCarry;
                if (ChunkA->Value == 0) {
                    LeadingZeroBlocks++;
                } else {
                    LeadingZeroBlocks = 0;
                }
                ChunkA = ChunkA->Next;
                ChunkB = ChunkB->Next;
            }
            if (NewCarry) {
                HardAssert (A.Length > B.Length);
                ChunkA->Value -= NewCarry;
            }
            Length -= LeadingZeroBlocks;
            HardAssert(Length >= 0);
            if (Length == 0) {
                Length = 1;
            }
        } else {
            A.CopyTo(&TempSub);
            B.CopyTo(&A);
            A.Sub(TempSub);
            A.Exponent = TempSub.Exponent;
            A.Neg();
        }
    } else if (A.IsNegative && B.IsNegative ) {
        A.Neg();
        B.Neg();
        A.Sub(B);
        B.Neg();
        A.Neg();

    } else if (A.IsNegative) {
        A.Neg();
        Result = A.Add(B);
        A.Neg();
    } else { /* (B.IsNegative) */
        B.Neg();
        Result =  A.Add(B);
        B.Neg();
    }

    TruncateLeadingZeroes();

    return Result;
}


template <typename t_allocator>
auto big_int<t_allocator>::ShiftLeft(uint32 ShiftAmount) -> big_int& {
    constexpr uint32 BitWidth = 32;
    uint32 UsedChunks = Length;
    chunk_list *Head = GetChunk(UsedChunks-1);

    if (IsZero(*this)) {
        return *this;
    }

    uint32 OldTopIdx = BitScanReverse32(Head->Value);
    HardAssert( OldTopIdx >= 0 && OldTopIdx < BitWidth );
    uint32 HeadZeros = BitWidth - 1 - OldTopIdx ;
    uint32 AvailableChunks = ChunkCount - UsedChunks;
    uint32 NeededDigits = ShiftAmount;
    uint32 AvailableDigits = AvailableChunks * BitWidth + HeadZeros;
    uint32 NeededChunks = NeededDigits < HeadZeros ? 0 :
                            ( (NeededDigits - HeadZeros) / BitWidth )
                          + ( (NeededDigits - HeadZeros) % BitWidth != 0 ? 1 : 0 );
    uint32 NewChunks = NeededChunks <= AvailableChunks ? 0 : NeededChunks - AvailableChunks;
    for (uint32 ChunksAdded =  0 ; ChunksAdded < NewChunks; ++ChunksAdded) {
        ExpandCapacity();
    }
    uint32 OldHeadIdx = UsedChunks-1;
    uint32 NewHeadIdx = OldHeadIdx + NeededChunks;

    chunk_list *DstChunk = GetChunk(NewHeadIdx);
    chunk_list *SrcChunk = GetChunk(OldHeadIdx);
    uint32 NewTopIdx = ShiftAmount <= HeadZeros ? OldTopIdx + ShiftAmount : (ShiftAmount - HeadZeros - 1)%BitWidth ;

    if ( NewTopIdx > OldTopIdx ) {
        uint32 Offset = BitWidth - ShiftAmount;
        for ( uint32 i = 0 ; i < UsedChunks - 1 ; ++i ) {
            HardAssert(SrcChunk->Prev != nullptr);
            DstChunk->Value = SrcChunk->Value << ShiftAmount | SrcChunk->Prev->Value >> Offset;
            DstChunk = DstChunk->Prev;
            SrcChunk = SrcChunk->Prev;
        }
        DstChunk->Value = SrcChunk->Value << ShiftAmount;
        DstChunk = DstChunk->Prev;
        //if we needed no extra chunks, then dest chunk will be nullptr
        HardAssert(NeededChunks == 0 || DstChunk != nullptr);
        for ( uint32 i = 0 ; i < NeededChunks ; ++i ) {
            DstChunk->Value = 0;
            DstChunk = DstChunk->Prev;
        }
        HardAssert(DstChunk == nullptr);


    } else if ( NewTopIdx < OldTopIdx ) {
        uint32 Offset = OldTopIdx - NewTopIdx;
        HardAssert (DstChunk != SrcChunk );
        DstChunk->Value = SrcChunk->Value >> Offset;
        DstChunk = DstChunk->Prev;

        for (uint32 i = 0; i < UsedChunks-1 ; ++i) {
            HardAssert(SrcChunk->Prev != nullptr);
            DstChunk->Value = SrcChunk->Value << ShiftAmount | SrcChunk->Prev->Value >> Offset;
            DstChunk = DstChunk->Prev;
            SrcChunk = SrcChunk->Prev;
        }

        HardAssert( SrcChunk != nullptr && SrcChunk->Prev == nullptr );
        HardAssert( DstChunk != nullptr);
        DstChunk->Value = SrcChunk->Value << ShiftAmount;
        DstChunk = DstChunk->Prev;

        //if we needed more than one chunk, then DstChunk is not nullptr
        HardAssert (NeededChunks > 1 || DstChunk==nullptr);
        uint32 TrailingZeroChunks = NeededChunks-1;
        for (uint32 i = 0 ; i < TrailingZeroChunks ; ++i) {
            HardAssert (DstChunk != nullptr);
            DstChunk->Value = 0;
            DstChunk = DstChunk->Prev;
        }
        HardAssert(DstChunk == nullptr);
    } else /* if ( NewTopIdx == OldTopIdx ) */ {
        for (uint32 i = 0 ; i < UsedChunks ; ++i) {
            DstChunk->Value = SrcChunk->Value;
            DstChunk = DstChunk->Prev;
            SrcChunk = SrcChunk->Prev;
        }
        HardAssert( NeededChunks > 0 || (DstChunk == SrcChunk && SrcChunk == nullptr) );
        for (uint32 i = 0 ; i < NeededChunks ; ++i) {
            DstChunk->Value = 0;
            DstChunk = DstChunk->Prev;
        }
        HardAssert(DstChunk == nullptr);
    }
    Length = UsedChunks+NeededChunks;
    HardAssert( Length <= ChunkCount);

    return *this;
}


template <typename t_allocator>
auto big_int<t_allocator>::ShiftRight(uint32 ShiftAmount) -> big_int& {
    if (ShiftAmount == 0) {
        return *this;
    };

    if (IsZero(*this)) {
        //TODO(##2024 07 13): why does this function care about normalizing zeroes?
//        Normalize();
        return *this;
    }
    chunk_list *LastChunk = GetChunk(Length-1);
    uint32 MSB_Idx = BitScanReverse32(LastChunk->Value);
    HardAssert (MSB_Idx != BIT_SCAN_NO_HIT);
    int LeadingZeroes = 31-static_cast<int>(MSB_Idx);
    //int LeadingBits  = MSB_Idx+1;
    int BitLength = Length*32 - LeadingZeroes;
    if (ShiftAmount >= BitLength) {
        this->Zero();
        return *this;
    }

    int ShiftOffset = ShiftAmount % 32;
    int ShiftChunks = ShiftAmount / 32;

    int FullBlocksToWrite = (BitLength - ShiftAmount) / 32;
    int TotalBlocksToWrite = FullBlocksToWrite + ( (BitLength - ShiftAmount) % 32 ? 1 : 0 );
    HardAssert(TotalBlocksToWrite - FullBlocksToWrite == 0 || TotalBlocksToWrite - FullBlocksToWrite == 1);

    chunk_list *SrcChunk = GetChunk(ShiftChunks);
    chunk_list *TgtChunk = &Data;

    uint32 RightOffset = ShiftOffset;
    uint32 LeftOffset = 32 - RightOffset;
    for ( int i = 0 ; i < FullBlocksToWrite ; ++i ) {
        uint32 RightPortion = SrcChunk->Value >> RightOffset;
        HardAssert(LeftOffset == 32 || SrcChunk->Next);
        uint32 LeftPortion = LeftOffset == 32 ? 0x0 : SrcChunk->Next->Value << LeftOffset;
        TgtChunk->Value = RightPortion | LeftPortion;
        SrcChunk = SrcChunk->Next;
        TgtChunk = TgtChunk->Next;
    }

    if (TotalBlocksToWrite > FullBlocksToWrite) {
        if (SrcChunk == LastChunk) {
            TgtChunk->Value = 0x0 | SrcChunk->Value>>RightOffset;
        } else if (LastChunk == SrcChunk->Next) {
            TgtChunk->Value = (SrcChunk->Value>>RightOffset) | (SrcChunk->Next->Value<<LeftOffset);
        } else {
            InvalidCodePath("invalid case?");
        }
    }
    this->Length = TotalBlocksToWrite;


    return *this;

}


void FullMul32(uint32 A, uint32 B, uint32 C[2]);


template <typename t_allocator>
auto big_int<t_allocator>::Mul (big_int& B)-> big_int::status {

    big_int::status Result = big_int::status::VALID;
    big_int& A = *this;

	TempMulCarries.Zero();
	TempMul0.Zero();
	uint32 Carry = 0;
	chunk_list *ChunkA = &A.Data;
	for (uint32 IdxA = 0 ; IdxA < A.Length ; ++IdxA, ChunkA = ChunkA->Next) {
        HardAssert(ChunkA != nullptr);
        chunk_list *ChunkB = &B.Data;

	    TempMul1.Zero();
	    chunk_list *ChunkTemp3 = &TempMul1.Data;
        uint32 TempMulCarriesActualLength = 1;
        TempMulCarries.Zero();
        chunk_list *ChunkTempMulCarries = &TempMulCarries.Data;
        while (TempMulCarries.Length <= IdxA) {
            TempMulCarries.ExtendLength();
            ChunkTempMulCarries = ChunkTempMulCarries->Next;
        }
	    for (uint32 IdxB = 0 ; IdxB < B.Length ; ++IdxB, ChunkB = ChunkB->Next) {
            HardAssert(ChunkB != nullptr);
            uint32 ChunkProd[2] = {};
            FullMul32(ChunkA->Value, ChunkB->Value, ChunkProd);
            while (TempMul1.Length-1 < IdxA + IdxB) {
                TempMul1.ExtendLength();
            }
            ChunkTemp3 = TempMul1.GetChunk(IdxA+IdxB);
            ChunkTemp3->Value = ChunkProd[0];

            TempMulCarries.ExtendLength();
            ChunkTempMulCarries = ChunkTempMulCarries->Next;
            HardAssert(ChunkTempMulCarries != nullptr);
            ChunkTempMulCarries->Value = ChunkProd[1];
            TempMulCarriesActualLength = ChunkProd[1] != 0 ? TempMulCarries.Length : TempMulCarriesActualLength;
	    }
	    HardAssert(TempMul1.Length >= IdxA + B.Length);
	    HardAssert(TempMulCarries.Length >= TempMul1.Length);
	    ChunkTempMulCarries = &TempMulCarries.Data;
	    for (uint32 CheckIdx = 0; CheckIdx <= IdxA ; CheckIdx++) {
            HardAssert(ChunkTempMulCarries != nullptr);
	        ChunkTempMulCarries->Value == 0;
	        ChunkTempMulCarries = ChunkTempMulCarries->Next;
	    }
	    TempMulCarries.Length = TempMulCarriesActualLength;
	    TempMul1.Add(TempMulCarries);
	    TempMul0.Add(TempMul1);
	}

	TempMul0.IsNegative = A.IsNegative ^ B.IsNegative;
	A.Set(TempMul0);
    Result = big_int::status::VALID;

    return Result;
}


//BUG(##2024 06 21): this whole function is messed up. what if there are trailing bits? error for fractionals, but valid for integers. what if there are leading zeroes? could happen after subtraction...
template <typename t_allocator>
int CountBits(big_int<t_allocator>& A) {
//    HardAssert(A.IsNormalized()); //NOTE(##2024 06 17):this function is also used in Div which works with integers i.e. denormalized representation
    if (IsNormalizedZero(A)) {
        return 1;
    }
    int NDigits= (A.Length-1) * 32;
    typename big_int<t_allocator>::chunk_list *LastChunk = A.GetChunk(A.Length-1);
    if (LastChunk->Value != 0) {
        NDigits += BitScanReverse32(LastChunk->Value) + 1;
    } else {
        //NOTE(##2024 06 12): situation after Multiplication with zero or the like, where multiple chunks remain.
        NDigits = 1;
    }
    HardAssert(NDigits) >= 1;
    return NDigits;
}

template <typename t_allocator>
int CountBits_(big_int<t_allocator>& A) {
//    HardAssert(A.IsNormalized()); //NOTE(##2024 06 17):this function is also used in Div which works with integers i.e. denormalized representation
    if (IsZero(A)) {
        return 1;
    }

    typename big_int<t_allocator>::chunk_list *Cur= A.GetChunk(A.Length-1);
    int32 LeadingZeroChunks = 0;
    for (int i = 0 ; i < A.Length ; ++i) {
        if (Cur->Value == 0x0) {
            Cur = Cur->Prev;
            LeadingZeroChunks++;
            continue;
        }
    }
    int32 FullChunks = A.Length - LeadingZeroChunks - 1;
    HardAssert(FullChunks >= 0);
    int32 NDigits = FullChunks * 32 + BitScanReverse32(Cur->Value) + 1;
    HardAssert(NDigits) >= 1;
    return NDigits;
}


template <typename t_allocator>
auto Div (big_int<t_allocator>& A, big_int<t_allocator>& B, big_int<t_allocator>& ResultInteger, big_int<t_allocator>& ResultFraction, t_allocator Allocator, uint32 MinFracPrecision=32) -> void{

    ResultInteger.Set(0);
    ResultFraction.Set(0);

    int DigitCountB = CountBits(B);
    bool IntegerPartDone = false;
    bool NonZeroInteger = false;

    big_int<t_allocator>& A_ = big_int<t_allocator>::TempDivA;
    big_int<t_allocator>& B_ = big_int<t_allocator>::TempDivB;
    big_int<t_allocator>& One_ = big_int<t_allocator>::TempOne;
    big_int<t_allocator>& Temp_ = big_int<t_allocator>::TempDiv0;
    A.CopyTo(&A_);
    B.CopyTo(&B_);
    A_.IsNegative = B_.IsNegative = false;


    //Compute Integer Part

    int ExplicitSignificantFractionBits = 0 ;

    if ( GreaterEquals(A_, B_) ) {
        NonZeroInteger = true;
        for ( ; !IsZero(A_) && !IntegerPartDone ; ) {
            int DigitCountA = CountBits(A_);
            int Diff = DigitCountA - DigitCountB;

            if (Diff > 0) {
                B_.ShiftLeft(Diff);
                if (LessThan<t_allocator>(&A_,&B_)){
                    B_.ShiftRight(1);
                    Diff -= 1;
                }
                Temp_.Set(1);
                Temp_.ShiftLeft(Diff);
                ResultInteger.Add(Temp_);
                A_.Sub(B_);
                B.CopyTo(&B_);
                B_.IsNegative = false;
            }
            else if (Diff == 0) {
                if ( GreaterEquals(A_, B_) ) {
                    A_.Sub(B_);
                    ResultInteger.Add(One_);
                } else {
                    IntegerPartDone = true;
                }
            } else {
                IntegerPartDone = true; //A.#Digits < B.#NumDigits => A/B = 0.something
            }
        }
        if (IsZero(A_)) {
            return;
        }
    }
    else IntegerPartDone = true;

    ResultInteger.Exponent = CountBits ( ResultInteger ) - 1;

    int DigitCountA = CountBits(A_);
    int RevDiff = DigitCountB-DigitCountA;
    A_.ShiftLeft(RevDiff);
    if (LessThan(&A_,&B_)) {
        A_.ShiftLeft(1);
        RevDiff += 1;
    }
    //TODO(##2024 06 17): what if we have result = x.0...0y?
    ResultFraction.Exponent = -RevDiff;;
    ResultFraction.Add(One_);
    ExplicitSignificantFractionBits += 1;
    A_.Sub(B_);
    //implicit first 1 was found.

    HardAssert (IntegerPartDone );



    //Compute Fraction Part
    for ( ; ExplicitSignificantFractionBits < MinFracPrecision && !IsZero(A_) ; ) {
        int DigitCountA = CountBits(A_);
        int RevDiff = DigitCountB-DigitCountA;
        HardAssert(RevDiff >= 0);
        if (RevDiff == 0) {
            HardAssert(LessThan(&A_,&B_));

            A_.ShiftLeft(1);
            ExplicitSignificantFractionBits += 1;
            ResultFraction.ShiftLeft(1);
            ResultFraction.Add(One_);
            A_.Sub(B_);
        } else if (RevDiff > 0) {
            A_.ShiftLeft(RevDiff);
            if (LessThan(&A_,&B_)) {
                A_.ShiftLeft(1);
                RevDiff += 1;
            }
            ExplicitSignificantFractionBits += RevDiff;
            ResultFraction.ShiftLeft(RevDiff);
            ResultFraction.Add(One_);
            A_.Sub(B_);
        }
    }

    ResultInteger.IsNegative = A.IsNegative && !B.IsNegative || !A.IsNegative && B.IsNegative;


    return;
}

//TODO(##2024 06 15): may not need this at all
template <typename t_allocator>
auto Combine (big_int<t_allocator>& Integer, big_int<t_allocator>& Fraction, big_int<t_allocator>& Result, t_allocator Allocator) -> big_int<t_allocator>& {
    HardAssert(IsNormalizedZero(Fraction) || Fraction.Exponent < 0 );
    HardAssert(Integer.Exponent >=0);

    Integer.CopyTo(&Result);
    Result.IsNegative = false;
    HardAssert(!Fraction.IsNegative);
    Result.AddF(Fraction);
    Result.IsNegative = Integer.IsNegative;

    return Result;
}


template <typename t_allocator>
auto GetMSB(big_int<t_allocator>&A) -> int32 {
    //NOTE(##2024 06 23): don't check for IsNormalized because this function is called in ParseIntegral, where denormalized values are used.
    if (IsZero(A)) return 0;
    typename big_int<t_allocator>::chunk_list *Cur = A.GetChunk(A.Length-1);
    int32 Result = 0;
    for (int i= 1; i< A.Length ; ++i, Cur = Cur->Prev) {
        if (Cur->Value != 0x0) {
            Result = BitScanReverse32(Cur->Value) + (A.Length-i)*32;
            break;
        }
    }
    if (Result == 0) {
        HardAssert(Cur == &A.Data);
        Result = A.Data.Value == 0x0 ? 0 : BitScanReverse32(A.Data.Value);
    }
    HardAssert(Result >= 0);
    return Result;
}


/** Get Bit at Idx N, 0-based, starting from top.
 *  e.g. N=0 yields MSB.
 **/
template<typename t_allocator>
auto big_int<t_allocator>::GetBit(int32 N) -> bool {
    HardAssert(0 <= N);

    chunk_list*Cur = GetChunk(Length-1);
    while (Cur->Value == 0x0 && Cur != &Data) Cur = Cur->Prev;
    int32 TopIdx = Cur->Value == 0 ? 0 : BitScanReverse32(Cur->Value);
    int32 TopBits = TopIdx+1;

    int32 BitsLeft = N;
    uint32 Mask = 0x1 << TopIdx;
    if (TopBits < BitsLeft) {
        BitsLeft -= TopBits;
        Cur = Cur->Prev;
        for ( int32 StepBits = 32 ; StepBits < BitsLeft; Cur = Cur->Prev ){
            HardAssert(Cur != nullptr);
            BitsLeft-=StepBits;
        }
        Mask = 0x8000'0000;
    }

    Mask >>= BitsLeft;
    HardAssert(N < CountBits_(*this) || Mask == 0x0);
    return ((Cur->Value & Mask) != 0);
}

template<typename t_allocator>
auto big_int<t_allocator>::RoundToNSignificantBits(int32 N) -> void {
    HardAssert(this->IsNormalized());
    int32 BitCount = CountBits(*this);
    if (BitCount <= N) return;

    int32 LSB = N==0 ? 0 : GetBit(N-1);
    int32 Guard = GetBit(N);
    //we know: this->IsNormalized() => LSB == 1 => ("BitCount>N" => "round or sticky bit is set")
    int32 RoundSticky = BitCount >  N + 1 ;

    ShiftRight(BitCount-N);

    bool32 RoundUp = Guard && (RoundSticky || LSB); //NOTE(Arokh##2024 07 14): round-to-even
    if (RoundUp) {
        static big_int<t_allocator> Temp{Allocator, Guard};
        Temp.Set(Guard);
        AddU(Temp);
    }

    int32 NewBitCount = CountBits(*this);
    HardAssert(NewBitCount == N || NewBitCount == N+1);
    if (/* !IsZero(*this)  &&*/ NewBitCount == N+1) { //TODO(##2024 07 13):CountBits returns 1 for zero. that's why we check for IsZero here. review!
        Normalize();
        ++Exponent;
    }
    Normalize();

    return;
}

template <typename t_allocator>
auto big_int<t_allocator>::Div (big_int& B, uint32 MinFracPrecision) -> void {
    ::Div(*this, B, TempDivInteger, TempDivFraction, this->Allocator, MinFracPrecision);
    Combine(TempDivInteger, TempDivFraction, *this, this->Allocator);
    return;
}

template <typename t_allocator>
auto big_int<t_allocator>::GetLeastSignificantExponent() -> int32 {
    HardAssert(IsNormalized());
    return (Exponent-CountBits(*this))+1;
}



template <typename t_allocator>
auto big_int<t_allocator>::AddF (big_int& B) -> void {
    big_int& A = *this;
    HardAssert(A.IsNormalized());
    HardAssert(B.IsNormalized());
    int A_LSE = A.GetLeastSignificantExponent();
    int B_LSE = B.GetLeastSignificantExponent();
    int Diff = A_LSE - B_LSE;
    big_int& B_ = TempAddF;
    B.CopyTo(&B_);
    if (Diff > 0 ) {
        A.ShiftLeft(Diff);
    }
    if (Diff < 0) {
        B_.ShiftLeft(-Diff);
    }
    int OldMSB = GetMSB(A);
    bool WasZero = IsZero(A);
    A.Add(B_);
    int NewMSB = GetMSB(A);
    A.Exponent = WasZero ? B_.Exponent : A.Exponent+(NewMSB-OldMSB);
    Normalize();

    return;
}

template <typename t_allocator>
auto big_int<t_allocator>::SubF (big_int& B)-> void {
    HardAssert(IsNormalized());
    big_int& A = *this;
    int A_LSE = A.GetLeastSignificantExponent();
    int B_LSE = B.GetLeastSignificantExponent();
    int Diff = A_LSE - B_LSE;
    big_int& B_ = big_int<t_allocator>::TempSubF;
    B.CopyTo(&B_);
    if (Diff > 0 ) {
        A.ShiftLeft(Diff);
    }
    if (Diff < 0) {
        B_.ShiftLeft(-Diff);
    }
    int OldMSB = BitScanReverse32(A.GetChunk(A.Length-1)->Value) + (A.Length-1)*32;
    A.Sub(B_);
    int NewMSB = BitScanReverse32(A.GetChunk(A.Length-1)->Value) + (A.Length-1)*32;
    A.Exponent += (NewMSB-OldMSB);
    Normalize();

    return;
}

template <typename t_allocator>
auto big_int<t_allocator>::MulF (big_int& B)-> void {
    HardAssert(IsNormalized()) && B.IsNormalized();
    big_int& A = *this;
    int Exponent_ = A.Exponent + B.Exponent - GetMSB(A) - GetMSB(B);
    A.Mul(B);
    Normalize();
	A.Exponent = IsNormalizedZero(A) ? 0 : Exponent_ + GetMSB(A);

    return;
}

template <typename t_allocator>
auto big_int<t_allocator>::DivF (big_int& B, uint32 MinFracPrecision) -> void {

    HardAssert(IsNormalized() && B.IsNormalized() && !IsNormalizedZero(B));
    big_int& A = *this;
    int A_LSE = A.GetLeastSignificantExponent();
    int B_LSE = B.GetLeastSignificantExponent();
    int Diff = A_LSE - B_LSE;

    big_int& B_ = big_int<t_allocator>::TempDivF;
    B.CopyTo(&B_);
    if (Diff > 0 ) {
        A.ShiftLeft(Diff);
    }
    if (Diff < 0) {
        B_.ShiftLeft(-Diff);
    }
    A.Div( B_, MinFracPrecision );
    Normalize();

    return;
}


template <typename t_allocator>
auto big_int<t_allocator>::Neg () -> void {
    this->IsNegative = !this->IsNegative;
}


/** \brief zeroes the data and contracts length to 1 chunk. leaves sign and exponent unchanged. */
template <typename t_allocator>
auto big_int<t_allocator>::Zero() -> void {
    chunk_list *Current = &Data;
    for (uint32 Idx = 0 ; Idx < ChunkCount ; ++Idx, Current = Current->Next) {
        Current->Value = 0;
    }
    Length = 1;
}

template <typename t_allocator>
auto big_int<t_allocator>::Push(uint32 ValueToAppend) -> big_int& {
    //TODO(## 2023 11 24): error handling. currently relies on Assert within ExpandCapacity()...
    //see also method declaration inside class definition

//    typedef typename big_int<t_allocator>::chunk_list chunk_list_t;
    using chunk_list_t = typename big_int<t_allocator>::chunk_list;
    //TODO(##2023 12 04): consider using ExtendLength only if Length < ChunkCount, and avoiding list traversal in the common case where Length == ChunkCount, where we can take advantage of `Last` pointer
    chunk_list_t *TheOne = ExtendLength();
    TheOne->Value = ValueToAppend;
    return *this;
}

template <typename t_allocator>
auto big_int<t_allocator>::Set(uint32 Value, bool32 IsNegative, int32 Exponent)  -> big_int<t_allocator>& {
    Length = 1;
    Data.Value = Value;
    this->IsNegative = IsNegative;
    this->Exponent = Exponent;
    return *this;
}

template <typename t_allocator>
auto big_int<t_allocator>::Set(uint32 *ValArray, uint32 ValCount, bool32 IsNegative/* =false */, int32 Exponent /* =0 */) -> big_int& {
    while (ChunkCount < ValCount) {
        ExpandCapacity();
    }
    auto GetLength = [=]() -> uint32 {
        uint32 ActualLength = 1;
        for (uint32 i = 0 ; i < ValCount ; ++i) {
            if (ValArray[i] != 0) {
                ActualLength = i+1;
            }
        }
        return ActualLength;
    };
    Length = GetLength();
    chunk_list *Current = &Data;
    uint32 *SrcVal = ValArray;
    for ( uint32 Idx = 0
        ; Idx < ValCount
        ; ++Idx, ++SrcVal, Current = Current->Next ) {
        Current->Value = *SrcVal;
    }
    this->IsNegative = IsNegative;
    this->Exponent = Exponent;

    return *this;
}

template <typename t_allocator>
auto big_int<t_allocator>::Set(big_int<t_allocator>& Other) -> big_int& {
    IsNegative = Other.IsNegative;

    Exponent = Other.Exponent;

    chunk_list *ChunkOther = &Other.Data;
    chunk_list *ChunkThis = &Data;
    while (Length < Other.Length) {
        ExtendLength();
    }
    for (uint32 Idx = 0 ; Idx < Other.Length ; ++Idx) {
        ChunkThis->Value = ChunkOther->Value;
        ChunkThis = ChunkThis->Next;
        ChunkOther = ChunkOther->Next;
    }
    return *this;
}


//TODO(##2024 07 04):Support NAN (e=128, mantissa != 0)
template <typename t_allocator>
auto big_int<t_allocator>::SetFloat(real32 Val) -> void {
    this->Zero();
    this->IsNegative = Sign(Val);
    this->Exponent = Exponent32(Val);
    bool32 Denormalized = Exponent == -127;
    uint32 Mantissa = Mantissa32(Val);
    if (Denormalized && Mantissa != 0) {
        Exponent -= 22-BitScanReverse32(Mantissa);
    }
    this->Data.Value = Denormalized ? Mantissa : 1<<23 | Mantissa;
    this->Normalize();
}


template <typename t_allocator>
auto big_int<t_allocator>::to_string() -> std::string {
    std::string Result;

    Result += IsNegative ? '-' : '+';
    chunk_list *Cur = GetChunk(Length-1);
    int32 UnusedChunks = ChunkCount - Length;
    char HexDigits[11] = "";

//    int32 UnusedChunksFieldSize = 2 + ( UnusedChunks > 1 ? UnusedChunks : 1);
//    int32 BufSize = 1+Length*9+2+UnusedChunksFieldSize+1;

    for (int32 Idx = 0 ; Idx < Length; ++Idx, Cur = Cur->Prev) {
        stbsp_sprintf(HexDigits, "%.8x ", Cur->Value);
        Result += HexDigits;
    }

    char ExpBuf[16] = "";

    stbsp_sprintf(ExpBuf, "E%d", Exponent);
    Result+=ExpBuf;

    Result += "[";
    Result += std::to_string(UnusedChunks);
    Result += ']';

    return Result;
}


template std::string big_int<arena_allocator>::to_string(); //explicit instantiation so it's available in debugger


template <typename t_allocator>
auto big_int<t_allocator>::GetTemp(temp_id TempId) -> big_int<t_allocator>& {
    static big_int<t_allocator> Temp_ParseF32 { AllocatorForTemporaries };

    switch (TempId) {
    break; case temp_id::Temp_ParseF32: return Temp_ParseF32;
    break; default : InvalidCodePath("Temporary not found\n");
    }
    static big_int<t_allocator> Invalid;
    return Invalid;
}

template <typename t_allocator>
auto big_int<t_allocator>::InitializeContext (t_allocator *Allocator) -> void {
    big_int<t_allocator>::AllocatorForTemporaries = *Allocator;
    big_int<t_allocator>::TempAddF.Allocator = *Allocator;
    big_int<t_allocator>::TempSub.Allocator = *Allocator;
    big_int<t_allocator>::TempSubF.Allocator = *Allocator;
    big_int<t_allocator>::TempMul0.Allocator = *Allocator;
    big_int<t_allocator>::TempMul1.Allocator = *Allocator;
    big_int<t_allocator>::TempMulCarries.Allocator = *Allocator;
    big_int<t_allocator>::TempOne.Allocator = *Allocator;
    big_int<t_allocator>::TempDivA.Allocator = *Allocator;
    big_int<t_allocator>::TempDivB.Allocator = *Allocator;
    big_int<t_allocator>::TempDiv0.Allocator = *Allocator;
    big_int<t_allocator>::TempDivF.Allocator = *Allocator;
    big_int<t_allocator>::TempDivInteger.Allocator = *Allocator;
    big_int<t_allocator>::TempDivFraction.Allocator = *Allocator;
    big_int<t_allocator>::TempPow10.Allocator = *Allocator;
    big_int<t_allocator>::TempTen.Allocator = *Allocator;
    big_int<t_allocator>::TempDigit.Allocator = *Allocator;
    big_int<t_allocator>::TempToFloat.Allocator = *Allocator;
    big_int<t_allocator>::TempFrac.Allocator = *Allocator;
    TempOne.Set(1,0,0);
    TempTen.Set(10);
    TempPow10.Set(1,0,0);
    TempFrac.Set(1,0,0);
    big_int<t_allocator>::WasInitialized = true;
}

template <typename t_allocator>
std::ostream& operator<<(std::ostream& Out, big_int<t_allocator>& BigInt) {
    return Out << BigInt.to_string();
}


/**
 * Pre-Condition: Pointers are valid
 * Pre-Condition: Src actually points to a numerical digit
 */
template <typename arena_allocator>
char * ParseIntegral(char *Src, big_int<arena_allocator> *Dst) {
    if (Src == nullptr) return nullptr;
    char *After=Src;


    char *LSD = Src; //Least Significant Digit
    while (IsNum(*LSD)) {
        ++LSD;
    }

    After = LSD;

    --LSD;

    Dst->Set(0);

    auto ToDigit = [](char c){return static_cast<int>(c-'0');};
    big_int<arena_allocator>& Digit_ = big_int<arena_allocator>::TempDigit;
    big_int<arena_allocator>& Pow10_= big_int<arena_allocator>::TempPow10;
    big_int<arena_allocator>& Ten_ = big_int<arena_allocator>::TempTen;

    Pow10_.Set(1);
    Ten_.Set(10,0,0);
    for (char *DigitP = LSD ; DigitP >= Src ; --DigitP) {
        int D = ToDigit(*DigitP);
        Digit_.Set(D);
        Digit_.Mul(Pow10_);
        Dst->Add(Digit_);
        Pow10_.Mul(Ten_);
    }

    HardAssert(Dst->Exponent == 0);
    Dst->Exponent = GetMSB(*Dst);

    return After;
}

//WIP 2024 06 01
template <typename arena_allocator>
char* ParseFraction(char *FracStr, big_int<arena_allocator> *Result) {

    if (FracStr == nullptr) return nullptr;

    Result->Set(0,0,0);
    char *After = FracStr;

    big_int<arena_allocator>& Digit_ = big_int<arena_allocator>::TempDigit;
    big_int<arena_allocator>& Pow10_= big_int<arena_allocator>::TempPow10;
    big_int<arena_allocator>& Ten_ = big_int<arena_allocator>::TempTen;

    int StrLen = StringLength(FracStr);
    for (int32 i = 0 ; i < StrLen ; ++i) {
        if (!IsNum(FracStr[i])) {
            StrLen = i;
            break;
        }
    }
    After = FracStr + StrLen;

    Pow10_.Set(1,0,0);
    Ten_.Set(10,0,Log2I(10));
    Ten_.Normalize();
    Digit_.Zero();

    for (int32 i = 0 ; i < StrLen ; ++i) {
        Pow10_.DivF(Ten_,128);
        int32 Digit = static_cast<int>(FracStr[i]-'0');
        int32 DigitExp = Digit == 0 ? 0 : Log2I(Digit);
        Digit_.Set(Digit,0,DigitExp);
        Digit_.Normalize();

        Digit_.MulF(Pow10_);

        Result->AddF(Digit_);
    }

    return After;
}


//WIP 2024 06 01
template <typename t_allocator>
char *ParseDecimal(char *DecStr, big_int<t_allocator>& Result) {

    if (DecStr == nullptr) return nullptr;

    b32 Neg = 0;
    if (IsSign(DecStr[0])) {
        Neg = DecStr[0] == '-';
        DecStr+=1;
    }

    char *After = DecStr;
    int StrLen = StringLength(DecStr);
    int PointPos = StrLen;
    bool FoundPoint = false;

    for (int i = 0 ; i < StrLen ; ++i) {
        if (!IsNum(DecStr[i])) {
            if (!FoundPoint && DecStr[i] == '.') {
                PointPos = i;
                FoundPoint = true;
            } else {
                StrLen = i;
                break;
            }
        }
    }

    After = ParseIntegral(DecStr, &Result);
    Result.Normalize();
    if (PointPos >= StrLen) return After;

    char *FractionStart = DecStr+PointPos+1;
    big_int<arena_allocator>& Frac = big_int<arena_allocator>::TempFrac;
    After = ParseFraction(DecStr+PointPos+1, &Frac);
    Result.AddF(Frac);
    Result.IsNegative = Neg;

    return After;
}


//TODO(##2024 07 04): are there faster ways to compute the mantissa than calling Round()?
template <typename t_allocator>
auto big_int<t_allocator>::ToFloat() -> float {

    this->CopyTo(&big_int::TempToFloat);
    bool32 Sign = TempToFloat.IsNegative;

    int MantissaBitCount = 24;

    int32 ResultExponent = TempToFloat.Exponent;

    if (ResultExponent < -150) {
        return Sign ? -0.f : 0.f;
    }

    if (ResultExponent < -126) {
        MantissaBitCount = 150+ResultExponent;
    }

    TempToFloat.RoundToNSignificantBits(MantissaBitCount); //Exponent may change

    ResultExponent = TempToFloat.Exponent;

    if (ResultExponent < -149 /*|| IsZero(TempToFloat)*/) { //TODO(##2024 07 13):check IsZero?
        return Sign ? -0.f : 0.f;
    }
    if (ResultExponent > 127)
    {
        return Sign ? -Inf : Inf;
    }

    bool32 Denormalized = ResultExponent < -126;

    if (Denormalized) {
        MantissaBitCount = 150+ResultExponent; //new Exponent => new MSB for Denormalized
    }
    HardAssert(MantissaBitCount >= 1 && MantissaBitCount <= 24);

    uint32 Mantissa = MantissaBitCount == 24 ? (1u << 23) - 1u : //Normalized: 1st Bit implicit
                                    (1u << MantissaBitCount) - 1u; //Denormalized: all Bits explicit





    HardAssert(TempToFloat.Length == 1);
    int32 BitCount = CountBits(TempToFloat);
    HardAssert (MantissaBitCount >= BitCount);
    int32 ShiftAmount = MantissaBitCount - BitCount;
    Mantissa &= TempToFloat.Data.Value << ShiftAmount;

    // IEEE-754 float32 format
    //sign|exponent|        mantissa
    //   _|________|__________,__________,___
    //   1|<---8-->|<---------23------------>

    uint32 Result = Sign << 31;
    uint32 Bias = 127;
    ResultExponent = Denormalized ? 0 : ResultExponent + Bias;
    Result |= ResultExponent << 23;
    Result |= Mantissa;
    return *reinterpret_cast<float *>(&Result);
}


template <typename t_allocator>
auto big_int<t_allocator>::ToFloatPrecision() -> void {

    bool32 Sign = this->IsNegative;

    int MantissaBitCount = 24;

    if (Exponent < -150) {
        this->Zero();
        return;
    }

    if (Exponent < -126) {
        MantissaBitCount = 150+Exponent;
    }

    RoundToNSignificantBits(MantissaBitCount); //Exponent may change

    if (Exponent < -149) {
        this->Zero();
    }
    if (Exponent > 127)
    {
        Set(1,Sign,128);
    }
    if (Exponent < -126) {
        MantissaBitCount = 150+Exponent; //new Exponent => new MSB for Denormalized
    }
//    HardAssert(MantissaBitCount >= 1 && MantissaBitCount <= 24);
//
//    uint32 Mantissa = MantissaBitCount == 24 ? (1u << 23) - 1u                //Normalized: 1st Bit implicit
//                                             : (1u << MantissaBitCount) - 1u; //Denormalized: all Bits explicit

    HardAssert(Length == 1);
//    int32 BitCount = CountBits(TempToFloat);
//    HardAssert (MantissaBitCount >= BitCount);
//    int32 ShiftAmount = MantissaBitCount - BitCount;
//    Mantissa &= Data.Value << ShiftAmount;

    return;
}


char *SkipSpace(char *);


//NOTE(##2024 07 12):delete this shim asap, temporary fix to make old functions compile i probably no longer use. such as ReadWord
inline char *SkipSpace(char *Beg, int32 BufSize){
    return SkipSpace(Beg);
}


inline bool32 Is1stIdChar(char C){
    return C == '_' || C >= 'a' && C<='z' || C>='A' && C<='Z';
}                                                                           inline bool32 Is2ndIdChar(char C){
    return C == '_' || C >= 'a' && C<= 'z' || C>='A' && C<='Z' || C>='0' && C<='9';
}


/** \brief returns advanced ptr on success, nullptr or original pointer on failure, depending on whether OPTIONAL Flags is set*/
inline char *MatchChar(char *Text, char Char, flags32 Flags=parse::DEFAULT) {

    if (Text == nullptr) return nullptr;
    char *Backup = Text;
    Text = SkipSpace(Text);
    if (*Text == Char) {
        return Text+1;
    }
    if (IsSet(Flags, parse::OPTIONAL)) {
        return Backup;
    }
    return nullptr;
}

/** \brief matches UTF-8 '§' sign : code point (hex): c2 a7
 *  \n returns advanced ptr on success, nullptr or original pointer on failure, depending on whether OPTIONAL Flags is set*/
inline char *MatchUTF8ParagraphSign(char *Text, flags32 Flags=parse::DEFAULT) {

    if (Text == nullptr) return nullptr;
    char *Backup = Text;
    Text = SkipSpace(Text);
    if ( Text[0] == (char)0xc2 && Text[1] == (char)0xa7 ) {
        return Text+2;
    }
    if (IsSet(Flags, parse::OPTIONAL)) {
        return Backup;
    }
    return nullptr;
}

/** \brief returns advanced ptr on success, nullptr or original pointer on failure, depending on whether OPTIONAL Flags is set*/
inline char *MatchUTF8Char(char *Text, u8 Chars[4], flags32 Flags=parse::DEFAULT) {
    InvalidCodePath("NotImplemented");
//    if (Text == nullptr) return nullptr;
//    char *Backup = Text;
//    Text = SkipSpace(Text);
//    if (*Text == Char) {
//        return Text+1;
//    }
//    if (IsSet(Flags, parse::OPTIONAL)) {
//        return Backup;
//    }
//    return nullptr;
}


char *MatchPrefix(char *Text, char *Prefix);

char* MatchIdentifier(char *Text, char *Identifier, flags32 Flags=parse::DEFAULT);

char *SkipSeparator(char *Text);

inline char *SkipSeparatorUnlessTerminator(char *Text, char Terminator=']') {
    if (!Text) return nullptr;
    if (*Text == Terminator) return Text;
    return SkipSeparator(Text);
}



/**
 *  Grammar (WIP)
 *   X? means x is optional, i.e. one or zero times
 *   X|Y means X or Y
 *   words starting with capital letter are rules
 *   all other words and characters are meant verbatim
 *
 *  Values ::= Value , Values | Value , | Value | Value Values
 *  Array ::= Identifier '[' Values ']' | '[' Values ']'
 *  Value ::= BracedList | LiteralValue
 *  LiteralValue ::= FloatValue | IntegerValue | Hex32Value | Identifier
 *  Identifier ::= 1stIdChar 2ndIdChars
 *  1stIdChar ::= Lower|Upper|_
 *  2ndIdChar ::= Num|Lower|Upper|_
 *  Lower ::= a|b|c|...|z
 *  Upper ::= A|B|C|...|Z
 *  FloatValue ::= Sign? Num+ . Num* | Sign? . Num+
 *  IntegerValue
 *  Hex32Value ::= (0x|0X) HexDigit{1,8}
 *  Sign ::= +|-
 *  Nums ::= Num Nums | Num
 *  Num ::= 0|...|9
 *  -------------------
 *  proposals:
 *  BracedList ::= Identifer { Value } | { Value } | Identifier {} | {}
 *  BracedList<Identifier> ::= Identifer { Value } | { Value } | Identifier {} | {}
 *  BracketList ::= [ Values ]
 *  Array ::= BracedList<array>
 *  v2 ::= BracedList<v2>
 **/


//entity_type ParseEntityType(char *&Begin, char *End);

char *ToString(entity_type ET);

char *ParseEntityType(char *Text, entity_type *Result);
char *ParseInteger(char *Text, i32 *Buf);
char *ParseF32(char *Text, real32 *Result);
char *ParseV2(char *Text, v2 *Result);
char *ParseHex32(char *Text, u32 *Result);
char *ParseShape2D(char *Text, shape_2d *Result, memory_arena *Arena);
char *ParseEntity(char *Text, entity *Entity, memory_arena *Arena=nullptr);

template <class T>
char* ParseValue(char *Text, T *Result, memory_arena *Arena=nullptr) {
    static_assert(false, "not_implemented");
    return nullptr;
}

template <>
char* ParseValue(char *Text, entity_type *Result, memory_arena *Arena);
template <>
char* ParseValue(char *Text, i32 *Result, memory_arena *Arena);
template <>
char* ParseValue(char *Text, f32 *Result, memory_arena *Arena);
template <>
char* ParseValue(char *Text, h32 *Result, memory_arena *Arena);
template <>
char* ParseValue(char *Text, v2 *Result, memory_arena *Arena);
template <>
char* ParseValue(char *Text, shape_2d *Result, memory_arena *Arena);



template<class T>
struct parser_traits {
    static const bool NeedsAllocator = false;
    static const bool IsPrimitive = false;
    static char TypeName[];
};

//
//#define SET_PARSER_TRAITS_DECL(type)\
//template<>\
//struct parser_traits<type>;
//
//SET_PARSER_TRAITS_DECL(f32);
//SET_PARSER_TRAITS_DECL(i32);
//SET_PARSER_TRAITS_DECL(u32);
//SET_PARSER_TRAITS_DECL(entity_type);
//SET_PARSER_TRAITS_DECL(char);
//SET_PARSER_TRAITS_DECL(v2);
//SET_PARSER_TRAITS_DECL(shape_2d);
//SET_PARSER_TRAITS_DECL(attack_frame_);
//SET_PARSER_TRAITS_DECL(hit_box);

#define SET_PARSER_TRAITS_DEF(type, bNeedsAllocator, bIsPrimitive)\
template<>\
struct parser_traits<type> {\
    static const bool NeedsAllocator = bNeedsAllocator;\
    static const bool IsPrimitive = bIsPrimitive;\
    static char TypeName[128] ;\
};


SET_PARSER_TRAITS_DEF(f32,false,true)
SET_PARSER_TRAITS_DEF(i32,false,true)
SET_PARSER_TRAITS_DEF(u32,false,true)
SET_PARSER_TRAITS_DEF(entity_type,false,true)
SET_PARSER_TRAITS_DEF(char,false,true)
SET_PARSER_TRAITS_DEF(v2,false,false)
SET_PARSER_TRAITS_DEF(shape_2d,true,false)
SET_PARSER_TRAITS_DEF(attack_frame_,true,false)
SET_PARSER_TRAITS_DEF(hit_box,true,false)

//
//#define SET_TRAIT__TYPE_NAME_DECL(type) \
//extern char parser_traits<type>::TypeName[128];
//SET_TRAIT__TYPE_NAME_DECL(f32)
//SET_TRAIT__TYPE_NAME_DECL(i32)
//SET_TRAIT__TYPE_NAME_DECL(h32)
//SET_TRAIT__TYPE_NAME_DECL(entity_type)
//SET_TRAIT__TYPE_NAME_DECL(v2)
//SET_TRAIT__TYPE_NAME_DECL(shape_2d)
//SET_TRAIT__TYPE_NAME_DECL(attack_frame_)
//SET_TRAIT__TYPE_NAME_DECL(hit_box)

#define TEMPLATE_PARSE_VALUE_DECL(type) \
template<>\
char *ParseValue(char *Text, type *Result, memory_arena *Arena);
TEMPLATE_PARSE_VALUE_DECL(f32)
TEMPLATE_PARSE_VALUE_DECL(i32)
TEMPLATE_PARSE_VALUE_DECL(h32)
TEMPLATE_PARSE_VALUE_DECL(entity_type)
TEMPLATE_PARSE_VALUE_DECL(v2)
TEMPLATE_PARSE_VALUE_DECL(shape_2d)
TEMPLATE_PARSE_VALUE_DECL(attack_frame_)
TEMPLATE_PARSE_VALUE_DECL(hit_box)

i32 CountElements(char *Text, b32 ElementTypeIsPrimitive, char *TypeName);

inline char *MatchLabel(char *Text, char *Identifier);


template<typename T>
char *ParseField(char *Text, char *Label, flags32 Flags, T *Result, memory_arena *Arena=nullptr) {

    HardAssert(Label && *Label!='\0');      //NOTE(ArokhSlade##2024 08 04): ALLOW EMPTY LABEL?

    char *Backup = Text;
    if (!(Text = MatchLabel(Text, Label))) { Text = Backup; }

    Backup = Text;
    if (!(Text = ParseValue<T>(Text, Result, Arena))) {
        if (!IsSet(Flags,parse::OPTIONAL)) {
            return nullptr;
        } else {
            // NOTE(ArokhSlade ##2024 08 03): '§' stands for "default"
//            if (!(Text = MatchChar(Backup, '§'))
            if (!(Text = MatchUTF8ParagraphSign(Backup))
            && (!(Text = MatchIdentifier(Backup, "default", parse::DEFAULT)))) {
                return nullptr;
            }
        }
    }
    Text = SkipSeparatorUnlessTerminator(Text, '}');

    return Text;
}


template<typename T>
char *ParseArrayField(char *Text, char *Label, flags32 Flags, i32 *ResultCnt, T **ResultPtr, memory_arena *Arena) {

    HardAssert(Label && *Label!='\0');      //NOTE(ArokhSlade##2024 08 06): ALLOW EMPTY LABEL?

    char *Backup = Text;
    if (!(Text = MatchLabel(Text, Label))) { Text = Backup; }

    Backup = Text;
    array<T> Result;
    if (!(Text = ParseArray<T>(Text, &Result, Arena))) { //TODO(ArokhSlade##2024 08 06): default value for [] empty arrays or nullptr??
        return nullptr;
    }
    Text = SkipSeparatorUnlessTerminator(Text, '}');

    *ResultCnt = Result.Cnt;
    *ResultPtr = Result.Ptr;

    return Text;
}


/**
 *  \brief will overwrite Result to point to newly allocated array
 *  \param Result (out parameter)
**/
template<class T>
char *ParseArray(char *Text, array<T> *Result, memory_arena *Arena) {

    Text = MatchIdentifier(Text, const_cast<char*>(parser_traits<T>::TypeName), parse::OPTIONAL);

    Text = MatchChar(Text, '[');

    Result->Cnt = CountElements(Text, parser_traits<T>::IsPrimitive, const_cast<char*>(parser_traits<T>::TypeName));

    Result->Ptr = PushArray(Arena, Result->Cnt, T);


    char *Backup = Text;
    for ( int i = 0; i < Result->Cnt && Text ; ++i ) {
        Text = ParseValue<T>(Text, &Result->Ptr[i]);
        Text = SkipSeparatorUnlessTerminator(Text);
    }

    Text = MatchChar(Text, ']');
    //TODO(ArokhSlade##2024 08 04): if (!Text) Undo Allocation
    return Text;
}


#endif //G_PARSER_UTILITY_H
