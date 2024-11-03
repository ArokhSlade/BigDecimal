#ifndef G_BIG_DECIMAL_UTILITY_H
#define G_BIG_DECIMAL_UTILITY_H

#include "G_Essentials.h"
#include "G_Miscellany_Utility.h"
#include "G_Math_Utility.h" //Sign(), Exponent32(), Mantissa32(), all inline defined in header, no need to link library
#include "G_MemoryManagement_Service.h"

#include <string>
#include <limits> //NOTE(ArokhSlade##2024 09 21): FUllMulN,
#include <type_traits> //enable_if, is_integral
#include <concepts> //unsigned_integral

typedef u64 Big_Dec_Chunk;
const Big_Dec_Chunk MAX_SEG_VAL = std::numeric_limits<Big_Dec_Chunk>::max(); //TODO(ArokhSlade##2024 09 22): put this into Big_Decimal's namespace
const i32 Big_Dec_Chunk_Width = sizeof(Big_Dec_Chunk) * 8; ////TODO(ArokhSlade##2024 09 22): put this into Big_Decimal's namespace

template<typename T>
struct Big_Decimal;

template<typename T_Alloc>
auto to_std_string(Big_Decimal<T_Alloc>& A) -> std::string;

template <typename T_Chunk_Bits_Alloc, typename T_Char_Alloc>
auto to_chars(Big_Decimal<T_Chunk_Bits_Alloc>& A, T_Char_Alloc& char_alloc) -> char *;


/**\note  allocator needs to be given a value, everything else can be left to default initialization.
   \brief Big_Decimal can be used to represent integers and floats.
       \n Functions like AddFractional and FromString will return values in a float-like format.
       \n "float-like format" means that the value .IsNormalizedFractional():
       \n the bit sequence starts with 1 and ends with 1 and is to be interpreted as 1.xyz...1x2^Exponent
       \n Functions like AddIntegerSigned and the Shift functions and ParseInteger treat the bit sequence at face value
       \n i.e. as unsigned integer (the sign is stored in a separate bool)
       \n they ignore the exponent and may return denormalized values.
       \n values are internally represented as a linked list of unsigned integer values.
       \n the initial element of the list is stored directly in the struct, the rest is allocated dynamically.
       \n chunks are stored in ascending order, i.e. Data.Value == least significant digits.
       \n most significant digits are at ::GetChunk(Length-1).Value
 */
template <typename T_Alloc = std::allocator<Big_Dec_Chunk>>
struct Big_Decimal {

    using Chunk_Bits = Big_Dec_Chunk;

    enum class Special_Constants{
        BELONGS_TO_CONTEXT
    };

    static constexpr flags32 COPY_DIGITS     = 0x1 << 0;
    static constexpr flags32 COPY_SIGN       = 0x1 << 1;
    static constexpr flags32 COPY_EXPONENT   = 0x1 << 2;
    static constexpr flags32 COPY_EVERYTHING = COPY_DIGITS | COPY_SIGN | COPY_EXPONENT;


    static constexpr flags32 ZERO_DIGITS_ONLY= 0x0;
    static constexpr flags32 ZERO_DIGITS     = 0x1 << 0;
    static constexpr flags32 ZERO_SIGN       = 0x1 << 1;
    static constexpr flags32 ZERO_EXPONENT   = 0x1 << 2;
    static constexpr flags32 ZERO_EVERYTHING = ZERO_DIGITS | ZERO_SIGN | ZERO_EXPONENT;

    static Big_Decimal<T_Alloc> temp_add_fractional;
    static Big_Decimal<T_Alloc> temp_sub_int_unsign;
    static Big_Decimal<T_Alloc> temp_sub_frac;
    static Big_Decimal<T_Alloc> temp_mul_int_0;
    static Big_Decimal<T_Alloc> temp_mul_int_1;
    static Big_Decimal<T_Alloc> temp_mul_int_carries;
    static Big_Decimal<T_Alloc> temp_div_int_a;
    static Big_Decimal<T_Alloc> temp_div_int_b;
    static Big_Decimal<T_Alloc> temp_div_int_0;
    static Big_Decimal<T_Alloc> temp_div_frac;
    static Big_Decimal<T_Alloc> temp_div_frac_int_part;
    static Big_Decimal<T_Alloc> temp_div_frac_frac_part;
    static Big_Decimal<T_Alloc> temp_pow_10;
    static Big_Decimal<T_Alloc> temp_one;
    static Big_Decimal<T_Alloc> temp_ten;
    static Big_Decimal<T_Alloc> temp_digit;
    static Big_Decimal<T_Alloc> temp_to_float;
    static Big_Decimal<T_Alloc> temp_parse_int;
    static Big_Decimal<T_Alloc> temp_parse_frac;
    static Big_Decimal<T_Alloc> temp_from_string;

    static constexpr i32 TEMPORARIES_COUNT = 20;

    static Big_Decimal<T_Alloc> *s_all_temporaries_ptrs[TEMPORARIES_COUNT];

    struct Chunk_List;
    using Chunk_Alloc = std::allocator_traits<T_Alloc>::template rebind_alloc<Chunk_List>;
    using Link = OneLink<Big_Decimal*>;
    using Link_Alloc = std::allocator_traits<T_Alloc>::template rebind_alloc<Link>;

    //Assigned through InitializeContext()
    static T_Alloc s_ctx_alloc; //NOTE(ArokhSlade##2024 09 04): needed for FromFloat (default arg lvalue reference)
    static Chunk_Alloc s_chunk_alloc;
    static Link_Alloc s_link_alloc;

    static Link *s_ctx_links;

    static i32 s_ctx_count;
    static bool s_is_context_initialized;


    Chunk_Alloc m_chunk_alloc;
    bool is_alive;
    bool was_divided_by_zero=false;

    using Chunk_Alloc_Traits = std::allocator_traits<Chunk_Alloc>;

    i32 Length = 1;

    bool IsNegative = false;
    i32 Exponent = 0;

    u32 m_chunks_capacity = 1;
    struct Chunk_List {
        Chunk_Bits Value;
        Chunk_List *Next,
                 *Prev;
    } Data = {};
    //TODO(##2024 06 15): replace chunk list with something more performant and dynamic like dynamic arrays or a container template

    //TODO(##2023 11 24): Naming: `Last` is ambiguous. What if someone wants the one at index Length-1? this->Last refers to the last allocated block in the list, not the last used block in the number.
    Chunk_List *Last = &Data;

    static auto InitializeContext (const T_Alloc& ctx_alloc = T_Alloc()) -> void;

    static void CloseContext(bool ignore_dangling_users=false) {
        HardAssert(ignore_dangling_users || s_ctx_count == TEMPORARIES_COUNT); //NOTE(ArokhSlade##2024 08 28):should not be called otherwise

        //TODO(ArokhSlade##2024 08 28): delete all context variables.
        for (Big_Decimal *temp_variable : s_all_temporaries_ptrs) {
            temp_variable->Release();
        }

        for( Link *next = nullptr, *link = s_ctx_links ; link ; link = next ) {
            next = link->next;
            link->value->Release();
        }

        s_ctx_alloc.~T_Alloc();
        s_chunk_alloc.~Chunk_Alloc();
        s_link_alloc.~Link_Alloc();

        s_is_context_initialized = false;

        return;
    }

    auto GetChunk(u32 Idx) -> Chunk_List*;

    auto ExpandCapacity() -> Chunk_List*;
    auto ExtendLength() -> Chunk_List*;

    auto Normalize() -> void;

    auto CopyTo(Big_Decimal *Dst, flags32 Flags = COPY_EVERYTHING)-> void;

    static auto FromString(char *Str, Big_Decimal *Dst = nullptr) -> bool;

    template <std::integral T_Src=u32>
    auto Set(T_Src Val,  bool IsNegative=false, i32 Exponent=0) -> Big_Decimal&;
    template <std::integral T_Slot=u32>
    auto Set(T_Slot *ValArray, u32 ValCount, bool IsNegative=false, i32 Exponent=0) -> Big_Decimal&;

    auto SetFloat(real32 Val) -> void;
    auto SetDouble(f64 Val) -> void;

    auto LessThanIntegerSigned(Big_Decimal<T_Alloc>& B) -> bool;
    auto LessThanIntegerUnsigned(Big_Decimal<T_Alloc>& B) ->bool;
    auto EqualsInteger(Big_Decimal<T_Alloc> const& B) -> bool;
    auto GreaterEqualsInteger(Big_Decimal<T_Alloc>& B) -> bool;
    auto EqualsFractional(Big_Decimal<T_Alloc> const& B) -> bool;
    auto EqualBits(Big_Decimal<T_Alloc> const& B) -> bool;


    auto ShiftLeft(u32 ShiftAmount) -> Big_Decimal&;
    auto ShiftRight(u32 ShiftAmount) -> Big_Decimal&;

    auto AddIntegerUnsigned (Big_Decimal& B) -> void;
    auto AddIntegerSigned (Big_Decimal& B) -> void;

    auto SubIntegerUnsignedPositive (Big_Decimal& B)-> void;
    auto SubIntegerUnsigned (Big_Decimal& B) -> void;
    auto SubIntegerSigned (Big_Decimal& B) -> void;

    auto MulInteger(Big_Decimal& B) -> void;
    auto DivInteger (Big_Decimal& B, u32 MinFracPrecision=32) -> void;

    auto AddFractional (Big_Decimal& B) -> void;
    auto SubFractional (Big_Decimal& B) -> void;
    auto MulFractional(Big_Decimal& B) -> void;
    auto DivFractional (Big_Decimal& B, u32 MinFracPrecision=32) -> void;

    auto RoundToNSignificantBits(i32 N) -> void;

    explicit operator std::string();


    auto ToFloat () -> f32;
    auto ToDouble () -> f64;



    /* Helper and Convenience Functions */
    template <typename T_Slot>
    auto CopyBitsTo(T_Slot *bits_array, i32 slot_count) -> void;


    auto SetBits64(u64 value) -> void;

    template<typename T_Slot>
    auto SetBitsArray(T_Slot *vals, u32 count) -> void;

    auto IsZero() -> bool;
    auto Neg() -> void;
    auto Zero(flags32 what = ZERO_DIGITS_ONLY) -> void;
    auto GetMSB() -> i32;
    auto GetBit(i32 N) -> bool;
    auto CountBits() -> i32;
    auto GetLeastSignificantExponent() -> i32;
    auto GetHead() -> Chunk_List*;
    auto TruncateTrailingZeroBits() -> void;
    auto TruncateLeadingZeroChunks() -> void;
    auto IsNormalizedFractional() -> bool;
    auto IsNormalizedInteger() -> bool;
    auto UpdateLength() -> void;

    static auto ParseInteger(char *Src, Big_Decimal *Dst = nullptr) -> bool;
    static bool ParseFraction(char *FracStr, Big_Decimal *Dst = nullptr);

    bool is_context_variable() {
        return m_chunk_alloc == s_chunk_alloc;
    }

    void Add_context_link() {
        //prepend to list
        Link *new_link = std::allocator_traits<Link_Alloc>::allocate(s_link_alloc, 1);
        new_link->next = s_ctx_links;
        new_link->value = this;
        s_ctx_links = new_link;

        ++s_ctx_count;
        return;
    }


    Link *Find_predecessor(Big_Decimal *them) {
        HardAssert(them);
        HardAssert(s_ctx_links);
        if (s_ctx_links->value == them) { return nullptr; }
        Link *predecessor = s_ctx_links;
        while (predecessor->next && predecessor->next->value != them) {
            predecessor = predecessor->next;
        }
        return predecessor;
    }

    void Remove_successor_or_head(Link *them) {
        if (!them) {
            HardAssert(s_ctx_links);
            Link *new_head = s_ctx_links->next;
            std::allocator_traits<Link_Alloc>::deallocate(s_link_alloc, s_ctx_links, sizeof(Link));
            s_ctx_links = new_head;
            return;
        }

        if (them->next) {
            Link *new_successor = them->next->next;
            std::allocator_traits<Link_Alloc>::deallocate(s_link_alloc, them->next, sizeof(Link));
            them->next = new_successor;
        }
        return;
    }

    void Remove_context_link() {
        HardAssert(is_context_variable());

        //find in list and remove
        Link *our_preceding_link = Find_predecessor(this);
        //NOTE(ArokhSlade##2024 08 29): every context variable has a link.
        HardAssert(s_ctx_links->value == this || our_preceding_link && our_preceding_link->next->value == this);
        Remove_successor_or_head(our_preceding_link);

        --s_ctx_count;
        return;
    }

    //TODO(ArokhSlade##2024 09 29): not needed anymore? delete
    Chunk_Alloc& Get_chunk_alloc() {
        HardAssert(s_is_context_initialized);
        return s_chunk_alloc;
    }

    static T_Alloc& Get_ctx_alloc() {
        HardAssert(s_is_context_initialized);
        return s_ctx_alloc;
    }


    //TODO(ArokhSlade##2024 09 29): redundant? why not ambiguous? is it ever called?
    Big_Decimal(T_Alloc& allocator)
    : m_chunk_alloc{Chunk_Alloc{allocator}}, IsNegative{false},Data{.Value{0}},
      is_alive{true}
    {
        HardAssert(s_is_context_initialized);
        if (is_context_variable()) Add_context_link();
    }


    template <std::integral T_Src = u32>
    Big_Decimal(const T_Alloc& allocator, T_Src Value_=0, bool IsNegative_=false, i32 Exponent_=0)
    : m_chunk_alloc{Chunk_Alloc{allocator}}, IsNegative{IsNegative_},
      Exponent{Exponent_}, is_alive{true}
    {
        HardAssert(s_is_context_initialized);
        if (is_context_variable()) Add_context_link();
        SetBits64(Value_);
    }

    Big_Decimal(const T_Alloc& allocator, f32 Value_)
    : Big_Decimal(allocator)
    {
        SetFloat(Value_);
    }

    Big_Decimal(const T_Alloc& allocator, f64 Value_)
    : Big_Decimal(allocator)
    {
        SetDouble(Value_);
    }

    template <std::integral T_Src = u32>
    Big_Decimal(const T_Alloc& allocator, T_Src *Values, u32 Count, bool IsNegative_=false, i32 Exponent_=0)
    : Big_Decimal{allocator, 0, IsNegative_, Exponent_}
    {
        SetBitsArray<T_Src>(Values, Count);
        HardAssert(IsNormalizedInteger());
    }

    template <std::integral T_Src = u32>
    Big_Decimal(T_Src Value_=0, bool IsNegative_=false, i32 Exponent_=0)
    : Big_Decimal{Get_ctx_alloc(), Value_, IsNegative_, Exponent_}
    {}

    Big_Decimal(f32 Value_)
    : Big_Decimal{Get_ctx_alloc(), Value_}
    {}

    Big_Decimal(f64 Value_)
    : Big_Decimal{Get_ctx_alloc(), Value_}
    {}

    template<std::integral T_Src=u32>
    Big_Decimal(T_Src *Values, u32 ValuesCount, bool IsNegative_=false, i32 Exponent_=0)
    : Big_Decimal{Get_ctx_alloc(),Values, ValuesCount, IsNegative_, Exponent_}
    {}



    Big_Decimal(T_Alloc& allocator, Big_Decimal& Other)
    : m_chunk_alloc{Chunk_Alloc{allocator}}, IsNegative{Other.IsNegative},
      Exponent{Other.Exponent}, is_alive{true}
    {
        HardAssert(s_is_context_initialized);
        Other.CopyTo(this, COPY_DIGITS);
        if (is_context_variable()) Add_context_link();
    }



    Big_Decimal(Big_Decimal& other)
    : m_chunk_alloc{other.m_chunk_alloc}, is_alive{other.is_alive}
    {
        HardAssert(is_alive);
        other.CopyTo(this, COPY_EVERYTHING);
        Add_context_link();
    }
    Big_Decimal(const Big_Decimal& other) = delete;

    void Release() {
        HardAssert(is_alive);
        is_alive = false;

        //delete all chunks through allocator
        Chunk_List *chunk = Last;
        Chunk_List *prev;
        i32 CountDeleted = 0;
        i32 old_capacity = m_chunks_capacity;

        HardAssert(Last->Next == nullptr);
        HardAssert(Data.Prev == nullptr);

        while (chunk != &Data) {
            HardAssert(chunk);
            prev = chunk->Prev;
            std::allocator_traits<Chunk_Alloc>::deallocate(m_chunk_alloc, chunk, sizeof(Chunk_List));
            chunk = prev;
            chunk->Next = nullptr;
            --m_chunks_capacity;
            ++CountDeleted;
        }

        HardAssert(CountDeleted+1 == old_capacity);
        HardAssert(m_chunks_capacity == 1);
        HardAssert(Data.Next == nullptr);
        this->Zero(ZERO_EVERYTHING);

        Length = 1;
        Last = &Data;

        if (is_context_variable()) {
            Remove_context_link();
        }

        m_chunk_alloc.~Chunk_Alloc();

        return;
    }

    ~Big_Decimal() {
        if (!is_alive) return;

        Release();
    }

    /* issues:
     - shallow or deep copy?
     - internal pointers (Last)
     - other with other allocator
     - other with other allocator type
     - copy other's allocator or only their values with our current allocator?
    */
    Big_Decimal& operator=(const Big_Decimal& other){
        return *this;
    }

    private:

    //NOTE(ArokhSlade##2024 08 28): this is more or less a work-around
    //because some constructor WILL be called at start-up for statics, which I don't actually want
    //so call this special constructor that does nothing
    //then later, when InitializeContext() is called,
    //these variables get re-assigned, setting their allocator and incrementing the context variables counter
    Big_Decimal(Special_Constants must_belong_to_context, u32 Value=0,
               bool IsNegative_=false, i32 Exponent_=0)
    : m_chunk_alloc{}, Data{.Value{Value}},
      IsNegative{IsNegative_}, Exponent{Exponent_},
      is_alive{false} //NOTE()ArokhSlade##2024 08 29):allocator for static temporaries not known at startup
    {
        HardAssert(must_belong_to_context == Special_Constants::BELONGS_TO_CONTEXT);
    }

    Big_Decimal(Special_Constants must_belong_to_context, Chunk_Alloc& chunk_alloc, u32 Value=0,
               bool IsNegative_=false, i32 Exponent_=0)
    : m_chunk_alloc{chunk_alloc}, Data{.Value{Value}},
      IsNegative{IsNegative_}, Exponent{Exponent_},
      is_alive{true} //NOTE(ArokhSlade##2024 08 29): allocator for static temporaries not known at startup
    {
        HardAssert(must_belong_to_context == Special_Constants::BELONGS_TO_CONTEXT);
        Add_context_link();
    }

    void Initialize(Chunk_Alloc& chunk_alloc) {
        m_chunk_alloc = chunk_alloc;
        is_alive = true;
        Add_context_link();
    }
};


template <typename T_Alloc>
auto Str(Big_Decimal<T_Alloc>& A, memory_arena *TempArena) -> char* {

    char Sign = A.IsNegative ? '-' : '+';
    typename Big_Decimal<T_Alloc>::Chunk_List *CurChunk = A.GetHead();
    i32 UnusedChunks = A.m_chunks_capacity - A.Length;
    char HexDigits[11] = "";

    i32 NumSize = UnusedChunks > 0 ? Log10I(UnusedChunks ) : 1;
    i32 UnusedChunksFieldSize = 2 + NumSize + 1;

    i32 BufSize = 1+A.Length*9+2+UnusedChunksFieldSize+1;
    char *Buf = PushArray(TempArena, BufSize, char);

    char *BufPos=Buf;
    *BufPos++ = Sign;

    for (i32 Idx = 0 ; Idx < A.Length; ++Idx, CurChunk = CurChunk->Prev) {
        stbsp_sprintf(BufPos, "%.8x ", CurChunk->Value);
        BufPos +=9;
    }

    stbsp_sprintf(BufPos, "[%.*d]", NumSize, UnusedChunks);
    BufPos += UnusedChunksFieldSize;

    *BufPos = '\0';

    return Buf;
}




template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::ParseInteger(char *Src, Big_Decimal *Dst) -> bool {

    if (!Dst) Dst = &Big_Decimal<T_Alloc>::temp_parse_int;

    Dst->Zero(ZERO_EVERYTHING);

    if (Src == nullptr || !IsNum(*Src)) return false;


    char *LastDigit = Src;
    while (IsNum(LastDigit[1])) {
        ++LastDigit;
    }

    Big_Decimal<T_Alloc>& Digit_ = Big_Decimal<T_Alloc>::temp_digit;
    Big_Decimal<T_Alloc>& Pow10_ = Big_Decimal<T_Alloc>::temp_pow_10;
    Big_Decimal<T_Alloc>& Ten_ = Big_Decimal<T_Alloc>::temp_ten;

    Pow10_.Set(1);
    Ten_.Set(10);

    auto ToDigit = [](char c){return static_cast<int>(c-'0');};

    for (char *DigitP = LastDigit ; DigitP >= Src ; --DigitP) {
        int D = ToDigit(*DigitP);
        Digit_.Set(D);
        Digit_.MulInteger(Pow10_);
        Dst->AddIntegerSigned(Digit_);
        Pow10_.MulInteger(Ten_);
    }

    Dst->Exponent = Dst->GetMSB();

    return true;
}


template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::ParseFraction(char *FracStr, Big_Decimal<T_Alloc> *Dst) -> bool{

    HardAssert(!!FracStr);

    if (!Dst) Dst = &Big_Decimal<T_Alloc>::temp_parse_frac;

    Dst->Zero(ZERO_EVERYTHING);

    Big_Decimal<T_Alloc>& digit = Big_Decimal<T_Alloc>::temp_digit;
    Big_Decimal<T_Alloc>& pow10 = Big_Decimal<T_Alloc>::temp_pow_10;
    Big_Decimal<T_Alloc>& ten = Big_Decimal<T_Alloc>::temp_ten;

    i32 StrLen = 0;
    for (char *Cur=FracStr; IsNum(*Cur) ; ++Cur) {
        ++StrLen;
    }
    if (StrLen == 0) return false;

    pow10.Set(1,0,0);
    ten.Set(10,0,Log2I(10));
    ten.Normalize();
    digit.Zero(ZERO_EVERYTHING);

    for (int32 i = 0 ; i < StrLen ; ++i) {
        pow10.DivFractional(ten,128);

        int32 Digit = static_cast<i32>(FracStr[i]-'0');
        int32 DigitExp = Digit == 0 ? 0 : Log2I(Digit);
        digit.Set(Digit,false,DigitExp);
        digit.Normalize();

        digit.MulFractional(pow10);

        Dst->AddFractional(digit);
    }

    HardAssert(Dst->IsNormalizedFractional());

    return true;
}


template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::FromString(char *DecStr, Big_Decimal<T_Alloc> *Dst) -> bool {
    using BigDec = Big_Decimal<T_Alloc>;

    if (DecStr == nullptr) return false;

    if (!Dst) Dst = &BigDec::temp_from_string;

    b32 Neg = 0;
    if (IsSign(DecStr[0])) {
        Neg = DecStr[0] == '-';
        DecStr+=1;
    }

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

    if (!ParseInteger(DecStr, Dst)) return false;
    Dst->Normalize();

    if (FoundPoint) {
        char *FractionStart = DecStr+PointPos+1;
        BigDec *Frac = &temp_parse_frac;
        if (!ParseFraction(DecStr+PointPos+1, Frac)) return false;
        Dst->AddFractional(*Frac);
    }

    Dst->IsNegative = Neg;

    HardAssert(Dst->IsNormalizedFractional());

    return bool(Dst);
}


/**
 *  \brief  A.Length == 1 && A.Data.Value == 0x0
 *  \n      Exponent does not matter. 0^0 is still zero
**/
template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::IsZero() -> bool {
    return Length == 1 && Data.Value == 0x0;
}

//TODO(ArokhSlade##2024 08 27): why does this not work?
//template <typename T_Alloc>
//using is_ctx_var = Big_Decimal<T_Alloc>::BELONGS_TO_CONTEXT;

//template <typename T_Alloc>
//constexpr auto is_ctx_var = Big_Decimal<T_Alloc>::BELONGS_TO_CONTEXT;

template <typename T_Alloc>
Big_Decimal<T_Alloc> Big_Decimal<T_Alloc>::temp_add_fractional {Big_Decimal<T_Alloc>::Special_Constants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
Big_Decimal<T_Alloc> Big_Decimal<T_Alloc>::temp_sub_int_unsign {Big_Decimal<T_Alloc>::Special_Constants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
Big_Decimal<T_Alloc> Big_Decimal<T_Alloc>::temp_sub_frac{Big_Decimal<T_Alloc>::Special_Constants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
Big_Decimal<T_Alloc> Big_Decimal<T_Alloc>::temp_mul_int_0 {Big_Decimal<T_Alloc>::Special_Constants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
Big_Decimal<T_Alloc> Big_Decimal<T_Alloc>::temp_mul_int_1 {Big_Decimal<T_Alloc>::Special_Constants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
Big_Decimal<T_Alloc> Big_Decimal<T_Alloc>::temp_mul_int_carries {Big_Decimal<T_Alloc>::Special_Constants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
Big_Decimal<T_Alloc> Big_Decimal<T_Alloc>::temp_one {Big_Decimal<T_Alloc>::Special_Constants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
Big_Decimal<T_Alloc> Big_Decimal<T_Alloc>::temp_div_int_a{Big_Decimal<T_Alloc>::Special_Constants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
Big_Decimal<T_Alloc> Big_Decimal<T_Alloc>::temp_div_int_b{Big_Decimal<T_Alloc>::Special_Constants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
Big_Decimal<T_Alloc> Big_Decimal<T_Alloc>::temp_div_int_0{Big_Decimal<T_Alloc>::Special_Constants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
Big_Decimal<T_Alloc> Big_Decimal<T_Alloc>::temp_div_frac{Big_Decimal<T_Alloc>::Special_Constants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
Big_Decimal<T_Alloc> Big_Decimal<T_Alloc>::temp_div_frac_int_part{Big_Decimal<T_Alloc>::Special_Constants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
Big_Decimal<T_Alloc> Big_Decimal<T_Alloc>::temp_div_frac_frac_part{Big_Decimal<T_Alloc>::Special_Constants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
Big_Decimal<T_Alloc> Big_Decimal<T_Alloc>::temp_pow_10{Big_Decimal<T_Alloc>::Special_Constants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
Big_Decimal<T_Alloc> Big_Decimal<T_Alloc>::temp_ten{Big_Decimal<T_Alloc>::Special_Constants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
Big_Decimal<T_Alloc> Big_Decimal<T_Alloc>::temp_digit{Big_Decimal<T_Alloc>::Special_Constants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
Big_Decimal<T_Alloc> Big_Decimal<T_Alloc>::temp_to_float{Big_Decimal<T_Alloc>::Special_Constants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
Big_Decimal<T_Alloc> Big_Decimal<T_Alloc>::temp_parse_int{Big_Decimal<T_Alloc>::Special_Constants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
Big_Decimal<T_Alloc> Big_Decimal<T_Alloc>::temp_parse_frac{Big_Decimal<T_Alloc>::Special_Constants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
Big_Decimal<T_Alloc> Big_Decimal<T_Alloc>::temp_from_string{Big_Decimal<T_Alloc>::Special_Constants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
Big_Decimal<T_Alloc> *Big_Decimal<T_Alloc>::s_all_temporaries_ptrs[TEMPORARIES_COUNT] = {
        &temp_add_fractional, &temp_sub_int_unsign, &temp_sub_frac, &temp_mul_int_0, &temp_mul_int_1,
        &temp_mul_int_carries, &temp_div_int_a, &temp_div_int_b, &temp_div_int_0, &temp_div_frac,
        &temp_div_frac_int_part, &temp_div_frac_frac_part, &temp_pow_10, &temp_one, &temp_ten,
        &temp_digit, &temp_to_float, &temp_parse_int, &temp_parse_frac, &temp_from_string
    };


template <typename T_Alloc>
bool Big_Decimal<T_Alloc>::s_is_context_initialized {false};

template <typename T_Alloc>
i32 Big_Decimal<T_Alloc>::s_ctx_count{0};

template <typename T_Alloc>
T_Alloc Big_Decimal<T_Alloc>::s_ctx_alloc{};

template <typename T_Alloc>
Big_Decimal<T_Alloc>::Chunk_Alloc Big_Decimal<T_Alloc>::s_chunk_alloc{};

template <typename T_Alloc>
Big_Decimal<T_Alloc>::Link_Alloc Big_Decimal<T_Alloc>::s_link_alloc{Big_Decimal<T_Alloc>::s_chunk_alloc};

template <typename T_Alloc>
Big_Decimal<T_Alloc>::Link *Big_Decimal<T_Alloc>::s_ctx_links = nullptr;




/**
 * \note will return nullptr if given index exceeds capacity
 * \note may return a chunk beyond `Length`, i.e. that's currently not in use,
 * i.e. that currently doesn't contain a part of the number's current value
 */
template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::GetChunk(u32 Idx) -> Chunk_List * {
    Chunk_List *Result = nullptr;
    if (Idx < m_chunks_capacity) {
        Result = &Data;
        while (Idx-- > 0) {
            Result = Result ->Next;
        }
    }
    return Result;
}

template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::GetHead() -> Chunk_List* {
    return GetChunk(Length-1);
}

/**
 * \brief Allocates another chunk
 */
template <typename T_Alloc>
//TODO(## 2023 11 23): support List nodes that are multiple chunks long?
auto Big_Decimal<T_Alloc>::ExpandCapacity() -> Chunk_List* {
    Chunk_List *NewChunk = (Chunk_List*)Chunk_Alloc_Traits::allocate(m_chunk_alloc, 1);
    HardAssert(NewChunk != nullptr);
    *NewChunk = {.Value = 0, .Next = nullptr, .Prev = nullptr};
    Last->Next = NewChunk;
    NewChunk->Prev = Last;
    Last = NewChunk;
    ++m_chunks_capacity;

    return NewChunk;
}

/**
 * \brief increase length, allocate when appropriate, return pointer to Last chunk (index == Length-1) in list.
 *     \n NOTE: Sets the new chunk's value to Zero.
 */
template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::ExtendLength() -> Chunk_List* {
    HardAssert(Length <= m_chunks_capacity);
    ++Length;
    Chunk_List *Result = GetHead();
    if (Result == nullptr) {
        Result = ExpandCapacity();
    }
    Result->Value = 0;

    return Result;
}

/** \brief Unisgned Add, pretend both numbers are positive integers, ignore exponents

 */
template <typename T_Alloc>
//TODO(## 2023 11 18) : test case where allocator returns nullptr
auto Big_Decimal<T_Alloc>::AddIntegerUnsigned (Big_Decimal& B) -> void {

    HardAssert(this->IsNormalizedInteger());

    Big_Decimal& A = *this;

    A.UpdateLength();
    B.UpdateLength();


    u32 LongerLength =  A.Length >= B.Length ? A.Length : B.Length;
    u32 ShorterLength =  A.Length <= B.Length ? A.Length : B.Length;
    Chunk_List *ChunkA = &A.Data, *ChunkB = &B.Data;

    Big_Dec_Chunk OldCarry = 0;
    //TODO(##2023 11 23): if (A.Length < B.Length) {/*allocate and append B.Length-A.Length chunks}
    for ( u32 Idx = 0
        ; Idx < LongerLength
        ; ++Idx,
          ChunkA = ChunkA ? ChunkA->Next : nullptr,
          ChunkB = ChunkB ? ChunkB->Next : nullptr ) {

        if ( Idx >= A.Length)  {
            ChunkA = A.ExtendLength();
        }

        Big_Dec_Chunk ChunkBValue = Idx < B.Length ? ChunkB->Value : 0U;
        bool NewCarry = MAX_SEG_VAL - ChunkBValue - OldCarry < ChunkA->Value;
        ChunkA->Value += ChunkBValue + OldCarry; //wrap-around is fine
        OldCarry = NewCarry;
    }

    if (OldCarry) {
        ChunkA = ExtendLength();
        ChunkA->Value = OldCarry;
    }

    return;

    HardAssert(this->IsNormalizedInteger());
}

template <typename T_Alloc>
//TODO(## 2023 11 18) : test case where allocator returns nullptr
auto Big_Decimal<T_Alloc>::AddIntegerSigned (Big_Decimal& B) -> void {

    HardAssert(this->IsNormalizedInteger());

    Big_Decimal& A = *this;
    if (A.IsNegative == B.IsNegative) {
        AddIntegerUnsigned(B);
    } else if (A.IsNegative) {
        A.Neg();
        A.SubIntegerSigned(B);
        A.Neg();
    } else { /* (B.IsNegative) */
        B.Neg();
        A.SubIntegerSigned(B);
        B.Neg();
    }

    HardAssert(this->IsNormalizedInteger());

    return ;
}

template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::TruncateTrailingZeroBits() -> void {
    if (this->IsZero()) { Length = 1; return; }
    i32 TruncCount = 0;
    i32 FirstOne = 0;
    bool BitFound = false;
    Chunk_List *Current = &this->Data;
    for (i32 Block = 0 ; Block < Length; ++Block, Current = Current -> Next) {
        FirstOne = BitScan<Big_Dec_Chunk>(Current->Value);
        if (FirstOne != BIT_SCAN_NO_HIT) {
            TruncCount += FirstOne;
            BitFound = true;
            break;
        }
        TruncCount += Big_Dec_Chunk_Width;
    }
    ShiftRight(TruncCount);
    return;
}


template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::TruncateLeadingZeroChunks() -> void {
    for (Chunk_List *Cur{ GetHead() } ; Cur != &Data ; Cur = Cur->Prev )
    {
        if (Cur->Value == 0x0) --Length;
        else break;
    };
    HardAssert (Length >= 1);
}

/** \brief Gets rid of leading zero chunks and trailing zeros, exponent doesn't change, e.g. 10100 E=2 becomes 101 E=2, NOT 101 E=4.
 *  \n that's because the bits are interpreted as 1.0100E2
 **/
template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::Normalize() -> void {
    TruncateLeadingZeroChunks();
    TruncateTrailingZeroBits();
}


template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::IsNormalizedFractional() -> bool {
    if (this->IsZero()) return true;
    bool has_trailing_zeros = (Data.Value&0x1) == 0;
    bool has_leading_zero_chunks = GetHead()->Value == 0x0;
    if ( has_trailing_zeros || has_leading_zero_chunks) return false;
    return true;
}

template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::IsNormalizedInteger() -> bool {
    return this->IsZero() || GetHead()->Value != 0x0;
}


/** \brief  if length is too long (i.e. includes leading zero chunks)
 *          this function fixes it.
 *          new length can never be greater than old length.
 *  \note   does not go beyond the old length, because there might be garbage values there.
**/
template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::UpdateLength() -> void {
    Chunk_List *chunk = &Data;
    i32 chunks_visited = 1;
    i32 new_length = 1;
    while (chunks_visited < Length && (chunk = chunk->Next)) {
        ++chunks_visited;

        if (chunk->Value != 0x0) {
            new_length = chunks_visited;
        }
    }
    this->Length = new_length;
    return;
}

template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::CopyTo(Big_Decimal *Dst, flags32 Flags) -> void {
    HardAssert(Dst != nullptr);

    if (IsSet(Flags, COPY_SIGN)) Dst->IsNegative = this->IsNegative;
    if (IsSet(Flags, COPY_EXPONENT)) Dst->Exponent = this->Exponent;


    if (IsSet(Flags, COPY_DIGITS)) {

        Chunk_List *Ours = &Data, *Theirs = &Dst->Data;

        while (Dst->Length < Length) Dst->ExtendLength();

        Dst->Length = Length; //if Dst->Length > this->Length

        for ( u32 Idx = 0 ; Idx < Length ; ++Idx ) {

            Theirs->Value = Ours->Value;

            Ours = Ours->Next;
            Theirs = Theirs->Next;
        }
    }

    return;
}



/**
 *  \brief tells whether Abs(A) < Abs(B)
**/
template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::LessThanIntegerUnsigned(Big_Decimal<T_Alloc>& B) ->bool{

    Big_Decimal<T_Alloc>& A = *this;
    bool Result = false;
    u32 Length = A.Length;
    if (A.Length == B.Length) {
        for ( typename Big_Decimal<T_Alloc>::Chunk_List
                *A_chunk = A.GetHead(),
                *B_chunk = B.GetHead()
            ; Length > 0
            ; A_chunk = A_chunk->Prev, B_chunk = B_chunk->Prev, --Length) {
            if (A_chunk->Value == B_chunk->Value) {
                continue;
            }
            Result = A_chunk->Value < B_chunk->Value;
            break;
        }
    } else {
        Result = A.Length < B.Length;
    }

    return Result;
}


template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::LessThanIntegerSigned(Big_Decimal<T_Alloc>& B) -> bool{
    Big_Decimal<T_Alloc>& A = *this;
    bool Result = false;
    if (A.IsNegative != B.IsNegative) {
        Result = A.IsNegative;
    } else {
        Result = !A.IsNegative && A.LessThanIntegerUnsigned(B)
               || A.IsNegative && B.LessThanIntegerUnsigned(A);
    }
    return Result;
}


template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::EqualBits(Big_Decimal<T_Alloc> const& B) -> bool {

    Big_Decimal<T_Alloc>& A = *this;
    bool all_equal = true;
    all_equal &= A.Length == B.Length;

    using p_chunk = typename Big_Decimal<T_Alloc>::Chunk_List const *;
    p_chunk chunk_A = &A.Data, chunk_B = &B.Data;

    for ( u32 idx = 0 ; idx < A.Length && all_equal ; ++idx ) {

        all_equal &= chunk_A->Value == chunk_B->Value;

        chunk_A = chunk_A->Next;
        chunk_B = chunk_B->Next;
    }

    return all_equal;
}

template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::EqualsInteger(Big_Decimal<T_Alloc> const& B) -> bool {

    Big_Decimal<T_Alloc>& A = *this;
    bool all_equal = true;
    all_equal &= A.IsNegative == B.IsNegative;
    all_equal &= A.EqualBits(B);

    return all_equal;
}

template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::EqualsFractional(Big_Decimal<T_Alloc> const& B) -> bool {

    Big_Decimal<T_Alloc>& A = *this;
    bool all_equal = true;
    all_equal &= A.EqualsInteger(B);
    all_equal &= A.Exponent == B.Exponent;

    return all_equal;
}


//TODO(##2024 06 08) param Allocator not used
template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::GreaterEqualsInteger(Big_Decimal<T_Alloc>& B) -> bool {
    Big_Decimal<T_Alloc>& A = *this;
    return B.LessThanIntegerSigned( A ) || A.EqualsInteger( B );
}

/**
\brief  subtraction algorithm for the special case where both operands are positive and minuend >= subtrahend
 */
template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::SubIntegerUnsignedPositive (Big_Decimal& B)-> void {

    HardAssert(this->IsNormalizedInteger());

    Big_Decimal& A = *this;

    A.UpdateLength();
    B.UpdateLength();

    HardAssert(A.GreaterEqualsInteger(B));

    Chunk_List *ChunkA = &A.Data;
    Chunk_List *ChunkB = &B.Data;
    u32 LeadingZeroBlocks = 0;
    u32 iterations = [](Big_Decimal<T_Alloc>& X){
        Chunk_List *cur = &X.Data;
        i32 chunks_visited = 1;
        i32 significant_chunks = 1;
        while ((cur = cur->Next) && chunks_visited < X.Length) {
            ++chunks_visited;
            if (cur->Value != 0x0) {
                significant_chunks = chunks_visited;
            }
        }
        return significant_chunks;
    }(B);

    HardAssert(B.Length == iterations);

    Big_Dec_Chunk Carry = 0;

    for (u32 Idx = 0 ; Idx < iterations; ++Idx) {
        if (ChunkB->Value == MAX_SEG_VAL && Carry) {
            Carry = 1;
        } else if (ChunkA->Value >= ChunkB->Value + Carry) {
            ChunkA->Value -= ChunkB->Value + Carry;
            Carry = 0;
        } else {
            ChunkA->Value += MAX_SEG_VAL - (ChunkB->Value + Carry) + 1;
            Carry = 1;
        }
        ChunkA = ChunkA->Next;
        ChunkB = ChunkB->Next;
    }

    if (Carry) {
        HardAssert (A.Length > B.Length);
        ChunkA->Value -= Carry;
        if (ChunkA->Value == 0x0 && A.GetHead() == ChunkA) {
            A.UpdateLength();
        }
    }

    this->TruncateLeadingZeroChunks();

    HardAssert(this->IsNormalizedInteger());

    return;
}


/**
\brief  subtraction algorithm for the special case where both operands are positive,
        but the difference may cross zero, i.e. subtrahend may be greater than minuend
 */
template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::SubIntegerUnsigned (Big_Decimal& B)-> void {

    HardAssert(this->IsNormalizedInteger());

    Big_Decimal& A = *this;

    bool crosses_zero = false;
    if (A.LessThanIntegerSigned(B)) {
        A.CopyTo(&temp_sub_int_unsign);
        B.CopyTo(&A, COPY_DIGITS | COPY_SIGN); //SubInteger does not care about exponent, but callers might, so we keep it intact
        crosses_zero = true;
    }
    Big_Decimal& B_ = crosses_zero ? temp_sub_int_unsign : B;

    A.SubIntegerUnsignedPositive(B_);

    if (crosses_zero) { A.Neg(); }

    HardAssert(this->IsNormalizedInteger());

    return;
}


/**
\brief  subtraction algorithm that will treat both operands as integers,
        i.e. will ignore exponents and just take the bits at face value.
        will respect signs, however.
*/
template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::SubIntegerSigned (Big_Decimal& B)-> void {

    HardAssert(this->IsNormalizedInteger());

    Big_Decimal& A = *this;

    if (!A.IsNegative && !B.IsNegative) {

        A.SubIntegerUnsigned(B);

    } else if (A.IsNegative && B.IsNegative ) {

        A.Neg();
        B.Neg();
        A.SubIntegerUnsigned(B);
        B.Neg();
        A.Neg();

    } else if (A.IsNegative) {

        A.Neg();
        A.AddIntegerUnsigned(B);
        A.Neg();

    } else { /* (B.IsNegative) */

        B.Neg();
        A.AddIntegerUnsigned(B);
        B.Neg();
    }

    TruncateLeadingZeroChunks();

    HardAssert(this->IsNormalizedInteger());

    return;
}

/**
 *  \brief  BIT-shift. simply shifts the bits, interpreted as ... sequence of bits, i.e. as unsigned integer.
 *  \n      exponent does not matter. decimal point does not matter.
 *  \note   PRE-condition: this->IsNormalizedInteger()
 *  \note   POST-condition: this->IsNormalizedInteger()
 *  \note   naturally, the result is not guaranted to be a noramlized Fractional, i.e. may have trailing zeros
**/
template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::ShiftLeft(u32 ShiftAmount) -> Big_Decimal& {

    HardAssert(this->IsNormalizedInteger());

    if (this->IsZero()) {
        return *this;
    }

    Chunk_List *Head = GetHead();

    HardAssert(Head->Value != 0x0); //TODO(ArokhSlade##2024 08 20): support denormalized numbers?

    constexpr u32 BitWidth = sizeof(Big_Dec_Chunk) * 8;
    u32 OldTopIdx = BitScanReverse<u64>(Head->Value);
    HardAssert(OldTopIdx != BIT_SCAN_NO_HIT);
    u32 HeadZeros = BitWidth - 1 - OldTopIdx;
    i32 Overflow = ShiftAmount - HeadZeros;

    u32 NeededChunks = 0;
    if (ShiftAmount > HeadZeros ) {
        NeededChunks = Overflow / BitWidth; //TODO(ArokhSlade#2024 10 04):replace with DivCeil()
        if (Overflow % BitWidth != 0) { NeededChunks++; }
    }

    for (u32 FreeChunks = m_chunks_capacity - Length; NeededChunks > FreeChunks; ++FreeChunks) {
        ExpandCapacity();
    }
    //TODO(ArokhSlade##2024 10 04):can probably just replace this with DstChunk = ExtendLength(NeededChunks)
    Chunk_List *DstChunk = GetChunk(Length-1 + NeededChunks);
    Chunk_List *SrcChunk = Head;

    u32 Offset = ShiftAmount % BitWidth;
    u32 NewTopIdx = (OldTopIdx + Offset) % BitWidth;

    if (NewTopIdx < OldTopIdx) {
        DstChunk->Value = SrcChunk->Value >> BitWidth-Offset; //NOTE(ArokhSlade##2024 10 06): inside this `if`, Offset cannot be zero, and amount shifted by is guaranteed to be >= 0, < width
        DstChunk = DstChunk->Prev;
    }

    for (i32 i = 0 ; i < Length-1 ; ++i, DstChunk = DstChunk->Prev, SrcChunk = SrcChunk->Prev) {

        Big_Dec_Chunk Left = SrcChunk->Value << Offset;
        Big_Dec_Chunk Right = SafeShiftRight(SrcChunk->Prev->Value, BitWidth-Offset); //NOTE(ArokhSlade##2024 08 20): regular shift would cause UB when Offset == 0

        DstChunk->Value = Left | Right;
    }

    HardAssert(SrcChunk == &Data);
    DstChunk->Value = SrcChunk->Value << Offset;

    for (DstChunk = DstChunk->Prev ; DstChunk ; DstChunk = DstChunk->Prev) {
        DstChunk->Value = 0x0;
    }

    Length += NeededChunks;
    HardAssert(Length <= m_chunks_capacity);

    HardAssert(this->IsNormalizedInteger());

    return *this;
}

/**
 *  \brief  BIT-shift. simply shifts the bits, interpreted as ... sequence of bits, i.e. as unsigned integer.
 *  \n      exponent does not matter. decimal point does not matter.
 *  \note   PRE-condition: this->IsNormalizedInteger()
 *  \note   POS-condition: this->IsNormalizedInteger()
 *  \note   naturally, the result is not guaranted to be a noramlized Fractional, i.e. may have trailing zeros
**/
template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::ShiftRight(u32 ShiftAmount) -> Big_Decimal& {

    HardAssert(this->IsNormalizedInteger());

    if (ShiftAmount == 0) { return *this; }
    if (this->IsZero()) { return *this; } //NOTE(ArokhSlade##2024 08 19): this normalizes Zero without calling Normalize() (no infinite loop)

    i32 BitCount = CountBits();
    if (ShiftAmount >= BitCount) { this->Zero(ZERO_DIGITS); return *this; }

    u32 BitWidth = Big_Dec_Chunk_Width;
    u32 Offset = ShiftAmount % BitWidth;
    i32 ChunksShifted = ShiftAmount / BitWidth;

    Chunk_List *Src = GetChunk(ChunksShifted);
    Chunk_List *Dst = &Data;
    i32 IterCount = 0;

    for (i32 i_Src = ChunksShifted  ; i_Src < Length-1 ; ++i_Src, ++IterCount) {
        Big_Dec_Chunk Right = Src->Value >> Offset;
        Big_Dec_Chunk Left  = Offset == 0 ? 0x0 : Src->Next->Value << BitWidth-Offset; //NOTE(ArokhSlade##2024 08 20): x>>y UB for y >= BitWidth. according to standard.
        Dst->Value = Left | Right;

        Src = Src->Next;
        Dst = Dst->Next;
    }

    Dst->Value = Src->Value >> Offset;
    if (Dst->Value) IterCount++;

    Length = IterCount;

    HardAssert(this->IsNormalizedInteger());

    return *this;
}

template <typename uN>
void FullMulN(uN A, uN B, uN C[2]);

/**
\brief  multiply, treat operands as integers, i.e. ignore exponents.
*/
template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::MulInteger (Big_Decimal& B)-> void {

    HardAssert(this->IsNormalizedInteger());

    Big_Decimal& A = *this;
    Big_Decimal& carries = temp_mul_int_carries;
    Big_Decimal& result = temp_mul_int_0;
    Big_Decimal& row_result = temp_mul_int_1;

//    if (A.IsZero() || B.IsZero()) {
//        this->Zero(); //TODO(ArokhSlade##2024 10 19): sign!
//        return;
//    }

	carries.Zero();
	result.Zero();

	Chunk_List *chunk_A = &A.Data;
	for (u32 IdxA = 0 ; IdxA < A.Length ; ++IdxA, chunk_A = chunk_A->Next) {
        HardAssert(chunk_A != nullptr);
        Chunk_List *chunk_B = &B.Data;

	    row_result.Zero();
	    Chunk_List *chunk_row_result = &row_result.Data;
        u32 carriesActualLength = 1;
        carries.Zero();
//        if (chunk_A->Value != 0x0) {
            Chunk_List *chunk_carries = &carries.Data;
            while (carries.Length <= IdxA) {
                carries.ExtendLength();
                chunk_carries = chunk_carries->Next;
            }
            for (u32 IdxB = 0 ; IdxB < B.Length ; ++IdxB, chunk_B = chunk_B->Next) {
                HardAssert(chunk_B != nullptr);
                Big_Dec_Chunk ChunkProd[2] = {};
                FullMulN<Big_Dec_Chunk>(chunk_A->Value, chunk_B->Value, ChunkProd);
                while (row_result.Length-1 < IdxA + IdxB) {
                    row_result.ExtendLength();
                }
                chunk_row_result = row_result.GetChunk(IdxA+IdxB);
                chunk_row_result->Value = ChunkProd[0];

                carries.ExtendLength();
                chunk_carries = chunk_carries->Next;
                HardAssert(chunk_carries != nullptr);
                chunk_carries->Value = ChunkProd[1];
                carriesActualLength = ChunkProd[1] != 0 ? carries.Length : carriesActualLength;
            }
            HardAssert(row_result.Length >= IdxA + B.Length);
            HardAssert(carries.Length >= row_result.Length);
            chunk_carries = &carries.Data;
            for (u32 CheckIdx = 0; CheckIdx <= IdxA ; CheckIdx++) {
                HardAssert(chunk_carries != nullptr);
                chunk_carries->Value == 0; //TODO(ArokhSlade##2024 09 23): no-op. intent? assert, i think. because there can't be carry values for the chunks below-or-equal IdxA
                chunk_carries = chunk_carries->Next;
            }
            carries.Length = carriesActualLength;

            row_result.UpdateLength();

            HardAssert(row_result.IsNormalizedInteger());
            HardAssert(carries.IsNormalizedInteger());
            HardAssert(result.IsNormalizedInteger());

            row_result.AddIntegerSigned(carries);
            result.AddIntegerSigned(row_result);
//        }
	}

	result.IsNegative = A.IsNegative != B.IsNegative;
	result.CopyTo(&A);

	HardAssert(this->IsNormalizedInteger());

    return;
}


#if OLD_GetMSB
/**
 *  \brief computes most significant bit in terms of internal bit sequence.
 *  \return index of leading 1 (lsb 0 numbering) for non-zero values, 0 otherwise.
 *  \note works also if this->IsNormalizedFractional() == false
 */
template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::GetMSB() -> i32 {

    if (IsZero()) return 0;
    typename Big_Decimal<T_Alloc>::Chunk_List *Cur = GetHead();
    i32 Result = 0;
    for (int i = 0 ; i < Length ; ++i, Cur = Cur->Prev) {
        if (Cur->Value != 0x0) {
            Result = BitScanReverse<Big_Dec_Chunk>(Cur->Value) + (Length-i-1) * Big_Dec_Chunk_Width;
            break;
        }
    }
    return Result;
}
#else
/**
 *  \brief computes most significant bit in terms of internal bit sequence.
 *  \return index of leading 1 (lsb 0 numbering) for non-zero values, 0 otherwise.
 *  \note   Big Decimal must have no leading zero chunks
 */
template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::GetMSB() -> i32 {

    HardAssert(IsNormalizedInteger());

    if (IsZero()) return 0;
    typename Big_Decimal<T_Alloc>::Chunk_List *Cur = GetHead();
    i32 Result = 0;
    Result = BitScanReverse<Big_Dec_Chunk>(Cur->Value) + (Length-1) * Big_Dec_Chunk_Width;
    return Result;

}
#endif



#if OLD_GetBit
/** Get Bit at Idx N, 0-based, starting from top.
 *  e.g. N=0 yields MSB.
 **/
template<typename T_Alloc>
auto Big_Decimal<T_Alloc>::GetBit(i32 N) -> bool {
    HardAssert(0 <= N);

    Chunk_List*Cur = GetHead();
    while (Cur->Value == 0x0 && Cur != &Data) Cur = Cur->Prev;
    i32 TopIdx = Cur->Value == 0 ? 0 : BitScanReverse<Big_Dec_Chunk>(Cur->Value);
    i32 TopBits = TopIdx+1;

    i32 BitsLeft = N;
    u32 Mask = 0x1 << TopIdx;
    if (TopBits < BitsLeft) {
        BitsLeft -= TopBits;
        Cur = Cur->Prev;
        for ( i32 StepBits = Big_Dec_Chunk_Width ; StepBits < BitsLeft ; Cur = Cur->Prev ){
            HardAssert(Cur != nullptr);
            BitsLeft-=StepBits;
        }
        Mask = 0x1 << (Big_Dec_Chunk_Width-1);
    }

    Mask >>= BitsLeft;
//    HardAssert(N < CountBitsButSkipLeadingZeroChunks(*this) || Mask == 0x0);
    HardAssert(N < this->CountBits() || Mask == 0x0);
    return ((Cur->Value & Mask) != 0);
}
#else
/** \brief  Get Bit at Index N (MSb 0 bit numbering),
 *  \n      i.e. 0-based, starting from top.
 *  \n      e.g. N=0 yields MSB.
 *  \note   N must be within bounds.
 *  \note   Big Decimal must have no leading zero chunks
 **/
template<typename T_Alloc>
auto Big_Decimal<T_Alloc>::GetBit(i32 high_idx) -> bool {
    HardAssert(0 <= high_idx);
    HardAssert(IsNormalizedInteger());

    i32 low_idx = GetMSB() - high_idx;

    HardAssert(low_idx >= 0);

    i32 chunk_idx = low_idx / Big_Dec_Chunk_Width;
    i32 bit_idx = low_idx % Big_Dec_Chunk_Width;

    Chunk_List* chunk = GetChunk(chunk_idx);
    bool result = chunk->Value & (Big_Dec_Chunk(1) << bit_idx);
    return result;
}
#endif


template<typename T_Alloc>
auto Big_Decimal<T_Alloc>::CountBits() -> i32 {
    return GetMSB() + 1;
}

/**
\brief performs round-to-even with guard bit, round bit and sticky bits (any 1 after the round bit counts as sticky bit set)
 */
template<typename T_Alloc>
auto Big_Decimal<T_Alloc>::RoundToNSignificantBits(i32 N) -> void {
    HardAssert(this->IsNormalizedFractional());
    i32 BitCount = this->CountBits();
    if (BitCount <= N) return;

    i32 LSB = N==0 ? 0 : GetBit(N-1);
    i32 Guard = GetBit(N);
    // we know: this->IsNormalizedFractional(), therefor LSB == 1
    // for explanation sake, assume bits are numbered front to back, i.e. LSB at [0]
    // we will round to N bits, i.e. indices [0..N-1]
    // bit[N] = Guard bit
    // bit[N+1] = Round bit
    // Sticky bit is set if any bit thereafter is set to true
    // thus, if we have N+3 or more bits, sticky bit is set.
    // Round OR Sticky bit is set, if we have N+2 or more bits.
    i32 RoundSticky = BitCount >  N + 1 ;

    ShiftRight(BitCount-N);

    //GRS:100 = midway, round if lsb is set, i.e. "uneven"
    //all other cases are determined:
    //GRS:0xx = round down
    //Guard bit set followed by a 1 anywhere later ("round or sticky") = round up
    bool RoundUp = Guard && (RoundSticky || LSB); //NOTE(Arokh##2024 07 14): round-to-even
    if (RoundUp) {
        AddIntegerUnsigned(temp_one);
    }

    i32 NewBitCount = this->CountBits();
    HardAssert(NewBitCount == N || NewBitCount == N+1);
    if (/* !this->IsZero()  &&*/ NewBitCount == N+1) { //TODO(##2024 07 13):CountBits returns 1 for zero. that's why we check for IsZero here. review!
        Normalize();
        ++Exponent;
    }
    Normalize();

    return;
}


template <typename T_Alloc>
auto DivInteger(Big_Decimal<T_Alloc>& A, Big_Decimal<T_Alloc>& B, Big_Decimal<T_Alloc>& ResultInteger, Big_Decimal<T_Alloc>& ResultFraction, u32 MinFracPrecision=32) -> bool{

    HardAssert(A.IsNormalizedInteger());
    HardAssert(B.IsNormalizedInteger());

    bool was_div_by_zero = B.IsZero();
    if (ResultInteger.was_divided_by_zero = was_div_by_zero) {
        ResultInteger.Zero();
        return was_div_by_zero;
    }

    using T_Big_Decimal = Big_Decimal<T_Alloc>;
    ResultInteger.Set(0);
    ResultFraction.Set(0);

    int DigitCountB = B.CountBits();
    bool IntegerPartDone = false;
    bool NonZeroInteger = false;

    T_Big_Decimal& A_    = T_Big_Decimal::temp_div_int_a;
    T_Big_Decimal& B_    = T_Big_Decimal::temp_div_int_b;
    T_Big_Decimal& One_  = T_Big_Decimal::temp_one;
    T_Big_Decimal& Temp_ = T_Big_Decimal::temp_div_int_0;
    A.CopyTo(&A_);
    B.CopyTo(&B_);
    A_.IsNegative = B_.IsNegative = false;


    //Compute Integer Part

    if ( A_.GreaterEqualsInteger(B_) ) {
        NonZeroInteger = true;
        for ( ; !A_.IsZero() && !IntegerPartDone ; ) {
            int DigitCountA = A_.CountBits();
            int Diff = DigitCountA - DigitCountB;

            if (Diff > 0) {
                B_.ShiftLeft(Diff);
                if (A_.LessThanIntegerSigned(B_)){
                    B_.ShiftRight(1);
                    Diff -= 1;
                }
                Temp_.Set(1);
                Temp_.ShiftLeft(Diff);
                ResultInteger.AddIntegerSigned(Temp_);
                A_.SubIntegerSigned(B_);
                B.CopyTo(&B_, T_Big_Decimal::COPY_DIGITS | T_Big_Decimal::COPY_EXPONENT); //Don't copy Sign
            }
            else if (Diff == 0) {
                if ( A_.GreaterEqualsInteger(B_) ) {
                    A_.SubIntegerSigned(B_);
                    ResultInteger.AddIntegerSigned(One_);
                } else {
                    IntegerPartDone = true;
                }
            } else {
                IntegerPartDone = true; //A.#Digits < B.#NumDigits => A/B = 0.something
            }
        }
        if (A_.IsZero()) {
            return was_div_by_zero;
        }
    }
    else { IntegerPartDone = true; }

    ResultInteger.Exponent = ResultInteger.CountBits() - 1;

    i32 DigitCountA = A_.CountBits();
    i32 RevDiff = DigitCountB-DigitCountA;
    A_.ShiftLeft(RevDiff);
    if (A_.LessThanIntegerSigned(B_)) {
        A_.ShiftLeft(1);
        RevDiff += 1;
    }

    ResultFraction.Set(0x1, false, -RevDiff);
    i32 ExplicitSignificantFractionBits = 1 ;
    A_.SubIntegerSigned(B_);

    //Compute Fraction Part
    for ( ; ExplicitSignificantFractionBits < MinFracPrecision && !A_.IsZero() ; ) {
        int DigitCountA = A_.CountBits();
        int RevDiff = DigitCountB-DigitCountA;
        HardAssert(RevDiff >= 0);
        if (RevDiff == 0) {
            HardAssert(A_.LessThanIntegerSigned(B_));

            A_.ShiftLeft(1);
            ExplicitSignificantFractionBits += 1;
            ResultFraction.ShiftLeft(1);
            ResultFraction.AddIntegerSigned(One_);
            A_.SubIntegerSigned(B_);
        } else if (RevDiff > 0) {
            A_.ShiftLeft(RevDiff);
            if (A_.LessThanIntegerSigned(B_)) {
                A_.ShiftLeft(1);
                RevDiff += 1;
            }
            ExplicitSignificantFractionBits += RevDiff;
            ResultFraction.ShiftLeft(RevDiff);
            ResultFraction.AddIntegerSigned(One_);
            A_.SubIntegerSigned(B_);
        }
    }

    ResultInteger.IsNegative = A.IsNegative && !B.IsNegative || !A.IsNegative && B.IsNegative;

    return was_div_by_zero;
}

/**
\brief  integer division, treat operands as integers, i.e. ignore their exponents.
 */
template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::DivInteger (Big_Decimal& B, u32 MinFracPrecision) -> void {

    HardAssert(this->IsNormalizedInteger());

    ::DivInteger(*this, B, temp_div_frac_int_part, temp_div_frac_frac_part, MinFracPrecision);

    HardAssert(temp_div_frac_frac_part.IsZero() || temp_div_frac_frac_part.Exponent < 0 );
    HardAssert(temp_div_frac_int_part.IsZero() || temp_div_frac_int_part.Exponent >=0);

    temp_div_frac_int_part.CopyTo(this, Big_Decimal::COPY_DIGITS | Big_Decimal::COPY_EXPONENT);

    this->IsNegative = false;

    HardAssert(!temp_div_frac_frac_part.IsNegative);
    this->AddFractional(temp_div_frac_frac_part);

    this->IsNegative = temp_div_frac_int_part.IsNegative;

    HardAssert(this->IsNormalizedInteger());

    return;
}

template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::GetLeastSignificantExponent() -> i32 {
    HardAssert(IsNormalizedFractional());
    return (Exponent-this->CountBits())+1;
}


/**
\brief addition algorithm that treats operands as fractionals.
 */
template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::AddFractional (Big_Decimal& B) -> void {
    Big_Decimal& A = *this;
    HardAssert(A.IsNormalizedFractional());
    HardAssert(B.IsNormalizedFractional());
    int A_LSE = A.GetLeastSignificantExponent();
    int B_LSE = B.GetLeastSignificantExponent();
    int Diff = A_LSE - B_LSE;
    Big_Decimal& B_ = temp_add_fractional;
    B.CopyTo(&B_);
    if (Diff > 0 ) {
        A.ShiftLeft(Diff);
    }
    if (Diff < 0) {
        B_.ShiftLeft(-Diff);
    }
    int OldMSB = A.GetMSB();
    bool WasZero = A.IsZero();
    A.AddIntegerSigned(B_);
    int NewMSB = A.GetMSB();
    A.Exponent = WasZero ? B_.Exponent : A.Exponent+(NewMSB-OldMSB);
    Normalize();

    return;
}


/**
\brief subtraction algorithm that treats operands as fractionals.
 */
template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::SubFractional (Big_Decimal& B)-> void {
    HardAssert(IsNormalizedFractional());
    Big_Decimal& A = *this;
    int A_LSE = A.GetLeastSignificantExponent();
    int B_LSE = B.GetLeastSignificantExponent();
    int Diff = A_LSE - B_LSE;
    Big_Decimal& B_ = Big_Decimal<T_Alloc>::temp_sub_frac;
    B.CopyTo(&B_);
    if (Diff > 0 ) {
        A.ShiftLeft(Diff);
    }
    if (Diff < 0) {
        B_.ShiftLeft(-Diff);
    }
    int OldMSB = GetMSB();
    A.SubIntegerSigned(B_);
    int NewMSB = GetMSB();
    A.Exponent += (NewMSB-OldMSB);
    Normalize();

    return;
}


/**
\brief multiply algorithm that treats operands as fractionals.
 */
template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::MulFractional(Big_Decimal& B) -> void {
    HardAssert(IsNormalizedFractional());
    HardAssert(B.IsNormalizedFractional());
    Big_Decimal& A = *this;
    int Exponent_ = A.Exponent + B.Exponent - A.GetMSB() - B.GetMSB();
    A.MulInteger(B);
    Normalize();
	A.Exponent = Exponent_ + A.GetMSB();

    return;
}


/**
\brief division algorithm that treats operands as fractionals.
 */
template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::DivFractional (Big_Decimal& B, u32 MinFracPrecision) -> void {

    Big_Decimal& A = *this;
    HardAssert(A.IsNormalizedFractional());
    HardAssert(B.IsNormalizedFractional());

    if (B.IsZero()) {
        A.was_divided_by_zero = true;
        return;
    }

    int A_LSE = A.GetLeastSignificantExponent();
    int B_LSE = B.GetLeastSignificantExponent();
    int Diff = A_LSE - B_LSE;

    Big_Decimal& B_ = Big_Decimal<T_Alloc>::temp_div_frac;
    B.CopyTo(&B_);
    if (Diff > 0 ) {
        A.ShiftLeft(Diff);
    }
    if (Diff < 0) {
        B_.ShiftLeft(-Diff);
    }
    A.DivInteger( B_, MinFracPrecision );
    Normalize();

    return;
}


template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::Neg () -> void {
    this->IsNegative = !this->IsNegative;
}


/** \brief zeros the data and contracts length to 1 chunk.
 *         leaves sign and exponent unchanged by default,
 *         but this can be controlled with flags
 *  \arg   what : leave empty for default (ZERO_DIGITS_ONLY), or add ZERO_SIGN | ZERO_EXPONENT as needed
 *  \note  why this function exists:
 *         1: it's presumeably cheaper than Set(0) (used often in hot loops of multiply alrogithm for example)
 *         2: easier say A.Zero() than A.Set(0,A.IsNegative,A.Exponent)
 *            and more explicit and clear to say A.Zero(ZERO_EVERYTHING) than A.Set(0)*/
template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::Zero(flags32 what) -> void {
        Data.Value = 0x0;
        Length = 1;
    if (IsSet(what, ZERO_SIGN))
        IsNegative = false;
    if (IsSet(what, ZERO_EXPONENT))
        Exponent = 0;
}

template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::SetBits64(u64 value) -> void {
    i32 n_src_bits = BitScanReverse<u64>(value) + 1;
    if (n_src_bits == 0) n_src_bits = 1;
    i32 n_src_bytes = DivCeil(n_src_bits, 8);
    i32 n_chunk_bytes = sizeof(Big_Dec_Chunk);

    Length = 1;
    Chunk_List *chunk = &Data;

    i32 n_grab_bytes = n_src_bytes <= n_chunk_bytes ? n_src_bytes : n_chunk_bytes;
    i32 n_grab_bits = n_grab_bytes * 8;
    i32 n_bytes_remaining = n_src_bytes - n_grab_bytes;

    i32 offset = 0;
    u64 mask = GetMaskBottomN<u64>(n_grab_bits);
    chunk->Value = value & mask;

    for ( ; n_bytes_remaining > 0 ; n_bytes_remaining -= n_grab_bytes ) {
        offset += n_grab_bits;
        if (n_bytes_remaining < n_grab_bytes) {
            n_grab_bytes = n_bytes_remaining;
            i32 n_grab_bits = n_grab_bytes * 8;
        }
        mask = GetMaskBottomN<u64>(n_grab_bits);
        mask <<= offset;
        chunk = ExtendLength();
        HardAssert(offset < 64 && offset >=0);
        chunk->Value = (value & mask) >> offset;
    }

    HardAssert(this->IsNormalizedInteger());
    return;
}

/**
 * \brief   copy bits from array of unsigned integral type.
 *          automatically marshall from array slot type to chunk type.
 *          i.e. spread wider types onto multiple chunks, jam multiple slots of shorter type into single chunk.
 *          don't copy leading zeroes if they would create a leading zero chunk.
 */
template <typename T_Alloc>
template<typename T_Slot>
auto Big_Decimal<T_Alloc>::SetBitsArray(T_Slot *vals, u32 count) -> void {

    using T_Chunk = Big_Dec_Chunk;

    i32 slot_bytes = sizeof(T_Slot);
    i32 chunk_bytes= sizeof(T_Chunk);
    i32 chunks_per_slot = slot_bytes / chunk_bytes;
    i32 slots_per_chunk = chunk_bytes/ slot_bytes;

    T_Slot *last = vals + count - 1;
    for (i32 i = 0 ; i < count-1 ; ++i) {
        if (*last == 0) {
            --last;
        }
    }
    i32 slots_total = last - vals + 1;

    T_Slot top = vals[slots_total-1];
    i32 top_bits = BitScanReverse<T_Slot>(top)+1;
    if (top_bits == 0) top_bits = 1;
    i32 top_bytes = DivCeil(top_bits, 8);

    i32 bytes_total = slot_bytes * (slots_total-1) + top_bytes;

    i32 chunks_total = (bytes_total + chunk_bytes - 1) / chunk_bytes;

    Length = 1;
    while (Length < chunks_total) {
        ExtendLength();
    }

    Chunk_List *chunk = &Data;
    for (i32 i = 0; i < Length ; ++i) {
        chunk->Value = 0;
        chunk = chunk->Next;
    }

    chunk = &Data;
    T_Slot *slot = vals;
    i32 chunk_bits = chunk_bytes * 8;
    if (chunks_per_slot) {
        u32 offset = 0;

        T_Chunk base_mask = GetMaskBottomN<T_Chunk>(chunk_bits);

        T_Slot mask = base_mask;
        for (i32 slot_idx = 0 ; slot_idx < slots_total-1 ; ++slot_idx){
            offset = 0;
            mask = base_mask;
            for(i32 chunk_idx = 0 ; chunk_idx < chunks_per_slot ; ++chunk_idx) {
                T_Slot chunk_val = *slot & mask;
                chunk_val >>= offset;
                chunk->Value = chunk_val;

                mask <<= chunk_bits;
                offset += chunk_bits;
                chunk = chunk->Next;
            }
            ++slot;
        }
        offset = 0;
        mask = base_mask;
        i32 top_chunks = DivCeil(top_bytes, chunk_bytes);
        for(i32 chunk_idx = 0 ; chunk_idx < top_chunks ; ++chunk_idx) {
            T_Slot chunk_val = *slot & mask;
            chunk_val >>= offset;
            chunk->Value = chunk_val;

            mask <<= chunk_bits;
            offset += chunk_bits;
            chunk = chunk->Next;
        }
    } else {
        i32 slot_bits = slot_bytes * 8;
        T_Chunk slot_val = 0x0;
        T_Chunk chunk_val = 0x0;
        i32 offset = 0;
        for (i32 chunk_idx = 0 ; chunk_idx < Length-1 ; ++chunk_idx) {
            slot_val = 0x0;
            chunk_val = 0x0;
            offset = 0;
            for (i32 slot_idx = 0 ; slot_idx < slots_per_chunk ; ++slot_idx) {
                slot_val = *slot;
                slot_val <<= offset;
                chunk_val |= slot_val;

                offset += slot_bits;
                ++slot;
            }
            chunk->Value = chunk_val;
            chunk = chunk->Next;
        }
        i32 slots_left = slots_total - slots_per_chunk * (Length-1);
        chunk_val = 0x0;
        offset = 0;
        for (i32 slot_idx = 0 ; slot_idx < slots_left ; ++slot_idx) {
            slot_val = *slot;
            slot_val <<= offset;
            chunk_val |= slot_val;

            offset += slot_bits;
            ++slot;
        }
        chunk->Value = chunk_val;
        chunk = chunk->Next;
    }

    HardAssert(this->IsNormalizedInteger());
};



/**
 *  \note   does not automatically normalize. e.g. you can set value to 2 and exponent to 0, which wouldn't be a valid decimal. some functions want this to represent / interpret as integers.
**/
template <typename T_Alloc>
template <std::integral T_Src>
auto Big_Decimal<T_Alloc>::Set(T_Src Value, bool IsNegative, i32 Exponent)  -> Big_Decimal<T_Alloc>& {
    this->SetBits64(Value);
    this->IsNegative = IsNegative;
    this->Exponent = Exponent;

    HardAssert(IsNormalizedInteger());
    return *this;
}


/**
 *  \note   does not automatically normalize. e.g. you can set value to 2 and exponent to 0, which wouldn't be a valid decimal. some functions want this to represent / interpret as integers.
**/
template <typename T_Alloc>
template <std::integral T_Slot>
auto Big_Decimal<T_Alloc>::Set(T_Slot *ValArray, u32 ValCount, bool IsNegative/* =false */, i32 Exponent /* =0 */) -> Big_Decimal& {
    SetBitsArray(ValArray, ValCount);
    this->IsNegative = IsNegative;
    this->Exponent = Exponent;

    HardAssert(IsNormalizedInteger());

    return *this;
}


//TODO(##2024 07 04):Support NAN (e=128, mantissa != 0)
template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::SetFloat(real32 Val) -> void {
    this->Zero();
    this->IsNegative = Sign(Val);
    this->Exponent = Exponent32(Val);
    bool Denormalized = IsDenormalized(Val);
    u32 Mantissa = Mantissa32(Val, !Denormalized);
    i32 mantissa_bitcount = BitScanReverse32(Mantissa) + 1;
    if (Denormalized && Mantissa != 0) {
        Exponent -= 22-BitScanReverse32(Mantissa);
    }

    u32 chunk_width = Big_Dec_Chunk_Width;
    SetBits64(Mantissa);
    this->Normalize(); //NOTE(ArokhSlade##2024 10 18): truncate trailing zeros
}

//TODO(##2024 07 04):Support NAN (e=128, mantissa != 0)
template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::SetDouble(f64 Val) -> void {

    this->Zero();
    this->IsNegative = Sign(Val);
    this->Exponent = Exponent64(Val);
    bool Denormalized = IsDenormalized(Val);
    u64 Mantissa = Mantissa64(Val, !Denormalized);
    i32 mantissa_bitcount = BitScanReverse<u64>(Mantissa) + 1;
    if (Denormalized && Mantissa != 0) {
        Exponent -= DOUBLE_PRECISION-2-BitScanReverse<u64>(Mantissa);
    }
    SetBits64(Mantissa);
    this->Normalize();
}

template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::InitializeContext (const T_Alloc& ctx_alloc) -> void {
    using BigDec = Big_Decimal<T_Alloc>;

    HardAssert(!s_is_context_initialized);
    BigDec::s_ctx_alloc = ctx_alloc;
    BigDec::s_chunk_alloc = Chunk_Alloc{ctx_alloc};
    BigDec::s_link_alloc = Link_Alloc{BigDec::s_chunk_alloc};

    for (i32 i = 0 ;  i < TEMPORARIES_COUNT ; ++i) {
//        new(s_all_temporaries_ptrs[i]) BigDec{Special_Constants::BELONGS_TO_CONTEXT, s_chunk_alloc};
        s_all_temporaries_ptrs[i]->Initialize(s_chunk_alloc);
    }

    temp_one.Set(1);
    temp_ten.Set(10);
    temp_pow_10.Set(1);
    temp_parse_frac.Set(1);
    Big_Decimal<T_Alloc>::s_is_context_initialized = true;
}


/**
\brief  copy the bits (without sign or exponent) to an array of unsigned integral type.
        will automatically marshall properly, i.e. if multiple chunks fit into one array slot
        then it will put as many in there as possible. likewise, if one array slot can't hold
        a full chunk, then the chunk gets split and spread over multiple array slots.
        array slots are filled in the same order as the chunks in the list, i.e. low chunks to high chunks.
 */
template <typename T_Alloc>
template <typename T_Slot>
auto Big_Decimal<T_Alloc>::CopyBitsTo(T_Slot *slots, i32 slot_count) -> void {

    i32 bit_count = CountBits();
    i32 bits_per_slot = sizeof(T_Slot) * 8;
    HardAssert(bit_count <= bits_per_slot * slot_count);
    i32 bits_per_chunk = Big_Dec_Chunk_Width;
    i32 chunks_per_slot = bits_per_slot / bits_per_chunk;
    i32 slots_per_chunk = bits_per_chunk / bits_per_slot;

    Chunk_List *chunk = &Data;
    T_Slot *slot = slots;
    if (chunks_per_slot) {
        i32 chunk_idx = 0;
        i32 offset = 0;
        for (  ; chunk_idx  + chunks_per_slot <= Length ; chunk_idx  += chunks_per_slot ) {
            for ( i32 slot_idx = 0 ; slot_idx < chunks_per_slot ; ++slot_idx ) {
                *slot |= (T_Slot)chunk->Value << offset;
                offset += bits_per_chunk;
                chunk = chunk->Next;
            }
            offset = 0;
            ++slot;
        }
        //handle remainder
        for ( ; chunk_idx < Length ; ++chunk_idx ) {
            *slot |= (T_Slot)chunk->Value << offset;
            offset += bits_per_chunk;
            chunk = chunk->Next;

        }
    } else { //chunks are bigger than slots
        i32 chunk_idx = 0 ;
        Big_Dec_Chunk chunk_mask = 0x0;
        i32 total_slot_idx = 0;
        for ( ; chunk_idx < Length ; ++chunk_idx ) {
            chunk_mask = GetMaskBottomN<Big_Dec_Chunk>(bits_per_chunk);
            HardAssert(total_slot_idx < slot_count);
            i32 offset = 0;
            for ( i32 slot_idx = 0 ; slot_idx < slots_per_chunk ; ++slot_idx) {
                Big_Dec_Chunk piece = chunk->Value & chunk_mask;
                piece >>= offset; //NOTE(ArokhSlade ## 2024 10 06): offset < bit width b/c chunks_per_slot==0
                *slot = piece;
                chunk_mask <<= bits_per_slot;
                ++slot;
                if (++total_slot_idx == slot_count) {
                    break;
                }
                offset += bits_per_slot;
            }
            chunk = chunk->Next;
        }
    }

    return;
}

//TODO(##2024 07 04): are there faster ways to compute the mantissa than calling Round()?
template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::ToFloat() -> f32 {

    if (this->IsZero()) {
        return 0.f;
    }

    this->CopyTo(&Big_Decimal::temp_to_float);
    bool Sign = temp_to_float.IsNegative;

    int MantissaBitCount = 24;

    i32 ResultExponent = temp_to_float.Exponent;

    if (ResultExponent < -150) {
        return Sign ? -0.f : 0.f;
    }

    if (ResultExponent < -126) {
        MantissaBitCount = 150+ResultExponent;
    }

    temp_to_float.RoundToNSignificantBits(MantissaBitCount); //Exponent may change

    ResultExponent = temp_to_float.Exponent;

    if (temp_to_float.IsZero()) {
        return Sign ? -0.f : 0.f;
    }
    if (ResultExponent > 127)
    {
        return Sign ? -Inf32 : Inf32;
    }

    bool Denormalized = ResultExponent < -126;

    if (Denormalized) {
        MantissaBitCount = 150+ResultExponent; //new Exponent => new MSB for Denormalized
    }
    HardAssert(MantissaBitCount >= 1 && MantissaBitCount <= 24);

    u32 MantissaMask = MantissaBitCount == 24 ? GetMaskBottomN<u32>(23) : //Normalized: 1st Bit implicit
                                    GetMaskBottomN<u32>(MantissaBitCount); //Denormalized: all Bits explicit




    i32 chunks_required = (FLOAT_PRECISION + Big_Dec_Chunk_Width - 1) / Big_Dec_Chunk_Width;
    HardAssert(temp_to_float.Length <= chunks_required);

    i32 BitCount = temp_to_float.CountBits(); //NOTE(ArokhSlade##2024 09 25): after rounding, value is normalized, i.e. trailing zeroes are chopped off.
    HardAssert (MantissaBitCount >= BitCount);
    i32 ShiftAmount = MantissaBitCount - BitCount; //NOTE(ArokhSlade##2024 09 25): reintroduce trailing zeroes

    u32 Mantissa = 0;
    temp_to_float.ShiftLeft(ShiftAmount);
    temp_to_float.CopyBitsTo(&Mantissa, 1);
    Mantissa &= MantissaMask;

    // IEEE-754 float32 format
    //sign|exponent|        mantissa
    //   _|________|__________,__________,___
    //   1|<---8-->|<---------23------------>

    u32 Result = Sign << 31;
    u32 Bias = 127;
    ResultExponent = Denormalized || this->IsZero() ? 0 : ResultExponent + Bias;
    Result |= ResultExponent << 23;
    Result |= Mantissa;

    return *reinterpret_cast<float *>(&Result);
}


//TODO(##2024 10 18): (copied from ToFloat) are there faster ways to compute the mantissa than calling Round()?
template <typename T_Alloc>
auto Big_Decimal<T_Alloc>::ToDouble() -> f64 {

    if (this->IsZero()) {
        return 0.f;
    }

    this->CopyTo(&Big_Decimal::temp_to_float);
    bool Sign = temp_to_float.IsNegative;

    int MantissaBitCount = DOUBLE_PRECISION;

    i32 ResultExponent = temp_to_float.Exponent;

    if (ResultExponent < MinExponent64) {
        return Sign ? -0.f : 0.f;
    }

    if (ResultExponent < -(FloatBias64-1)) {
        MantissaBitCount = (-MinExponent64)+ResultExponent;
    }

    temp_to_float.RoundToNSignificantBits(MantissaBitCount); //Exponent may change

    ResultExponent = temp_to_float.Exponent;

    if (ResultExponent < (MinExponent64+1) /*|| temp_to_float.IsZero()*/) { //TODO(##2024 07 13):check IsZero?
        return Sign ? -0.f : 0.f;
    }
    if (ResultExponent > FloatBias64)
    {
        return Sign ? -Inf64 : Inf64;
    }

    bool Denormalized = ResultExponent < -(FloatBias64-1); //TODO(ArokhSlade##2024 10 18): simplify with <=

    if (Denormalized) {
        MantissaBitCount = (-MinExponent64)+ResultExponent; //new Exponent => new MSB for Denormalized
    }
    HardAssert(MantissaBitCount >= 1 && MantissaBitCount <= DOUBLE_PRECISION);
    //TODO(ArokhSlade##2024 09 30): use the new MaskBottomNBits function
    u64 MantissaMask = MantissaBitCount == DOUBLE_PRECISION ? GetMaskBottomN<u64>(DOUBLE_PRECISION-1) : //Normalized: 1st Bit implicit
                                    GetMaskBottomN<u64>(MantissaBitCount); //Denormalized: all Bits explicit




    i32 chunks_required = (DOUBLE_PRECISION + Big_Dec_Chunk_Width - 1) / Big_Dec_Chunk_Width;
    HardAssert(temp_to_float.Length <= chunks_required);

    i32 BitCount = temp_to_float.CountBits(); //NOTE(ArokhSlade##2024 09 25): after rounding, value is normalized, i.e. trailing zeroes are chopped off.
    HardAssert (MantissaBitCount >= BitCount);
    i32 ShiftAmount = MantissaBitCount - BitCount; //NOTE(ArokhSlade##2024 09 25): reintroduce trailing zeroes

    u64 Mantissa = 0;

    temp_to_float.ShiftLeft(ShiftAmount);
    temp_to_float.CopyBitsTo(&Mantissa, 1);
    Mantissa &= MantissaMask;

    // IEEE-754 float64 format
    //sign|  exponent  |                      mantissa
    //   _|_______,____|____,________,________,________,________,________,________
    //   1|<----11---->|<------------------------52------------------------------>

    u64 ResultBits = Sign << 63;
    u64 Bias = FloatBias64;
    ResultExponent = Denormalized || this->IsZero() ? 0 : ResultExponent + Bias;
    ResultBits = ResultExponent;
    ResultBits <<= DOUBLE_PRECISION-1;
    ResultBits |= Mantissa;
    f64 Result = *reinterpret_cast<f64 *>(&ResultBits);
    return Result;
}

typedef Big_Decimal<std::allocator<Big_Dec_Chunk>> Big_Dec_Std;




template <typename T_Alloc>
Big_Decimal<T_Alloc>::operator std::string(){
    std::string Result;

    Result += IsNegative ? '-' : '+';
    Chunk_List *Cur = GetHead();
    i32 UnusedChunks = m_chunks_capacity - Length;
    constexpr i32 width = sizeof(Big_Dec_Chunk) * 2;
    char HexDigits[width+1] = "";

//    i32 UnusedChunksFieldSize = 2 + ( UnusedChunks > 1 ? UnusedChunks : 1);
//    i32 BufSize = 1+Length*9+2+UnusedChunksFieldSize+1;

    for (i32 Idx = 0 ; Idx < Length; ++Idx, Cur = Cur->Prev) {
        stbsp_sprintf(HexDigits, "%.*llx ", width, Cur->Value);
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

//template Big_Decimal<Arena_Allocator<Big_Dec_Chunk>>::operator std::string(); //explicit instantiation so it's available in debugger


template <typename T_Alloc>
std::ostream& operator<<(std::ostream& Out, Big_Decimal<T_Alloc>& big_decimal) {
    return Out << std::string(big_decimal);
}

template <typename T_Chunk_Bits_Alloc, typename T_Char_Alloc>
auto to_chars(Big_Decimal<T_Chunk_Bits_Alloc>& A, T_Char_Alloc& string_alloc) -> char* {
    char Sign = A.IsNegative ? '-' : '+';
    typename Big_Decimal<T_Chunk_Bits_Alloc>::Chunk_List *CurChunk = A.GetHead();
    i32 UnusedChunks = A.m_chunks_capacity - A.Length;
    char HexDigits[11] = "";

    i32 NumSize = UnusedChunks > 0 ? Log10I(UnusedChunks ) : 1;
    i32 UnusedChunksFieldSize = 2 + NumSize + 1;

    i32 BufSize = 1+A.Length*9+2+UnusedChunksFieldSize+1;
    char *Buf = std::allocator_traits<T_Char_Alloc>::allocate(string_alloc, BufSize);

    char *BufPos=Buf;
    *BufPos++ = Sign;

    for (i32 Idx = 0 ; Idx < A.Length; ++Idx, CurChunk = CurChunk->Prev) {
        stbsp_sprintf(BufPos, "%.8x ", CurChunk->Value);
        BufPos +=9;
    }

    stbsp_sprintf(BufPos, "[%.*d]", NumSize, UnusedChunks);
    BufPos += UnusedChunksFieldSize;

    *BufPos = '\0';

    return Buf;
}

#if OLD_FullMulN
template <typename uN>
void FullMulN(uN A, uN B, uN C[2]) {
    i32 HalfSize = 4 * sizeof(uN);
    uN HalfMask = (uN)-1 >> HalfSize;
    uN LoA = A & HalfMask ;
    uN HiA = A >> HalfSize;
    uN LoB = B & HalfMask ;
    uN HiB = B >> HalfSize;
    uN HiC = HiA*HiB;
    uN MidC1 = HiA * LoB;
    uN MidC2 = LoA * HiB;
    uN LoC = LoA * LoB;

    uN MAX = std::numeric_limits<uN>::max();

    auto ComputeAdditionOverflow = [MAX](uN A_, uN B_) -> uN {
        uN Result = 0;
        uN Capacity = MAX - A_;
        Result = Capacity < B_ ? B_-(Capacity+1) : 0;
        return Result;
    };

    uN Carry_ = ComputeAdditionOverflow(LoC, MidC1 << HalfSize);
    C[0] = LoC + (MidC1<<HalfSize);
    uN Carry_2 = ComputeAdditionOverflow(C[0], MidC2 << HalfSize);

    C[0] += (MidC2<<HalfSize);
    HardAssert(ComputeAdditionOverflow(Carry_, Carry_2) == 0);

    C[1] = Carry_ + Carry_2;
    HardAssert(ComputeAdditionOverflow(C[1],MidC1>>HalfSize) == 0);

    C[1]+=MidC1>>HalfSize;
    HardAssert(ComputeAdditionOverflow(C[1],MidC2>>HalfSize) == 0);

    C[1]+=MidC2>>HalfSize;
    HardAssert(ComputeAdditionOverflow(C[1],HiC<<HalfSize) == 0);

    C[1] += HiC;
}
#else
template <typename uN>
void FullMulN(uN A, uN B, uN C[2]) {

    i32 HalfSize = 4 * sizeof(uN);
    uN HalfMask = (uN)-1 >> HalfSize;

    uN a0 = A & HalfMask ;
    uN a1 = A >> HalfSize;
    uN b0 = B & HalfMask ;
    uN b1 = B >> HalfSize;

    uN a0b0 = a0 * b0;
    uN a0b1 = a0 * b1;
    uN a1b0 = a1 * b0;
    uN a1b1 = a1 * b1;

    auto add = [](uN a, uN b, uN* carry) -> uN {
        uN c = a+b;
        *carry += static_cast<uN>(c<a || c<b);
        return c;
    };

    uN c32, c10;

    c32 = 0;
    c10 = a0b0;

    c10 = add( c10, a0b1 << HalfSize, &c32 );
    c10 = add( c10, a1b0 << HalfSize, &c32 );

    c32 += a1b1;
    c32 += a0b1 >> HalfSize;
    c32 += a1b0 >> HalfSize;

    C[0] = c10;
    C[1] = c32;
}
#endif

#endif //G_BIG_DECIMAL_UTILITY_H
