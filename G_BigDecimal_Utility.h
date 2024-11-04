#ifndef G_BIG_DECIMAL_UTILITY_H
#define G_BIG_DECIMAL_UTILITY_H

#include "G_Essentials.h"
#include "G_Miscellany_Utility.h"
#include "G_Math_Utility.h" //Sign(), Exponent32(), Mantissa32(), all inline defined in header, no need to link library
#include "G_MemoryManagement_Service.h"

#include <string>
#include <limits> //NOTE(ArokhSlade##2024 09 21): used for MAX_CHUNK_VAL
#include <type_traits> //enable_if, is_integral
#include <concepts> //unsigned_integral

typedef u64 ChunkBits;
const ChunkBits MAX_CHUNK_VAL = std::numeric_limits<ChunkBits>::max(); //TODO(ArokhSlade##2024 09 22): put this into BigDecimal's namespace
const i32 CHUNK_WIDTH = sizeof(ChunkBits) * 8; ////TODO(ArokhSlade##2024 09 22): put this into BigDecimal's namespace

template<typename T>
struct BigDecimal;

template <typename T_ChunkBitsAlloc, typename T_CharAlloc>
auto to_chars(BigDecimal<T_ChunkBitsAlloc>& A, T_CharAlloc& char_alloc) -> char *;


/**\note  allocator needs to be given a value, everything else can be left to default initialization.
   \brief BigDecimal can be used to represent integers and floats.
       \n Functions like add_fractional and from_string will return values in a float-like format.
       \n "float-like format" means that the value fulfills .is_normalized_fractional():
       \n the bit sequence starts with 1 and ends with 1 and is to be interpreted as 1.xyz...1x2^Exponent
       \n Functions like add_integer_signed and the Shift functions and parse_integer treat the bit sequence at face value
       \n i.e. as unsigned integer (the sign is stored in a separate bool)
       \n they ignore the exponent and may return values with trailing zeros,
       \n i.e. their results may not fulfill .is_normalized_fractional()
       \n values are internally represented as a linked list of unsigned integer values.
       \n the initial element of the list is stored directly in the struct, the rest is allocated dynamically.
       \n chunks are stored in ascending order, i.e. data.value == least significant bits.
       \n most significant bits are at ::get_chunk(length-1).value
 */
template <typename T_Alloc = std::allocator<ChunkBits>>
struct BigDecimal {

    enum class SpecialConstants{
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

    static BigDecimal<T_Alloc> temp_add_fractional;
    static BigDecimal<T_Alloc> temp_sub_int_unsign;
    static BigDecimal<T_Alloc> temp_sub_frac;
    static BigDecimal<T_Alloc> temp_mul_int_0;
    static BigDecimal<T_Alloc> temp_mul_int_1;
    static BigDecimal<T_Alloc> temp_mul_int_carries;
    static BigDecimal<T_Alloc> temp_div_int_a;
    static BigDecimal<T_Alloc> temp_div_int_b;
    static BigDecimal<T_Alloc> temp_div_int_0;
    static BigDecimal<T_Alloc> temp_div_frac;
    static BigDecimal<T_Alloc> temp_div_frac_int_part;
    static BigDecimal<T_Alloc> temp_div_frac_frac_part;
    static BigDecimal<T_Alloc> temp_pow_10;
    static BigDecimal<T_Alloc> temp_one;
    static BigDecimal<T_Alloc> temp_ten;
    static BigDecimal<T_Alloc> temp_digit;
    static BigDecimal<T_Alloc> temp_to_float;
    static BigDecimal<T_Alloc> temp_parse_int;
    static BigDecimal<T_Alloc> temp_parse_frac;
    static BigDecimal<T_Alloc> temp_from_string;

    static constexpr i32 TEMPORARIES_COUNT = 20;

    static BigDecimal<T_Alloc> *s_all_temporaries_ptrs[TEMPORARIES_COUNT];

    struct ChunkList;
    using ChunkAlloc = std::allocator_traits<T_Alloc>::template rebind_alloc<ChunkList>;
    using Link = OneLink<BigDecimal*>;
    using LinkAlloc = std::allocator_traits<T_Alloc>::template rebind_alloc<Link>;

    //Assigned through initialize_context()
    static T_Alloc s_ctx_alloc; //NOTE(ArokhSlade##2024 09 04): needed for FromFloat (default arg lvalue reference)
    static ChunkAlloc s_chunk_alloc;
    static LinkAlloc s_link_alloc;

    static Link *s_ctx_links;

    static i32 s_ctx_count;
    static bool s_is_context_initialized;


    ChunkAlloc m_chunk_alloc;
    bool is_alive;
    bool was_divided_by_zero=false;

    using ChunkAllocTraits = std::allocator_traits<ChunkAlloc>;

    i32 length = 1;

    bool IsNegative = false;
    i32 Exponent = 0;

    u32 m_chunks_capacity = 1;
    struct ChunkList {
        ChunkBits value;
        ChunkList *next,
                 *prev;
    } data = {};
    //TODO(##2024 06 15): replace chunk list with something more performant and dynamic like dynamic arrays or a container template

    ChunkList *sentinel = &data;

    static auto initialize_context (const T_Alloc& ctx_alloc = T_Alloc()) -> void;

    static void close_context(bool ignore_dangling_users=false) {
        HardAssert(ignore_dangling_users || s_ctx_count == TEMPORARIES_COUNT); //NOTE(ArokhSlade##2024 08 28):should not be called otherwise

        //TODO(ArokhSlade##2024 08 28): delete all context variables.
        for (BigDecimal *temp_variable : s_all_temporaries_ptrs) {
            temp_variable->release();
        }

        for( Link *next = nullptr, *link = s_ctx_links ; link ; link = next ) {
            next = link->next;
            link->value->release();
        }

        s_ctx_alloc.~T_Alloc();
        s_chunk_alloc.~ChunkAlloc();
        s_link_alloc.~LinkAlloc();

        s_is_context_initialized = false;

        return;
    }

    auto get_chunk(u32 Idx) -> ChunkList*;

    auto expand_capacity() -> ChunkList*;
    auto extend_length() -> ChunkList*;

    auto normalize() -> void;

    auto copy_to(BigDecimal *Dst, flags32 Flags = COPY_EVERYTHING)-> void;

    static auto from_string(char *Str, BigDecimal *Dst = nullptr) -> bool;

    template <std::integral T_Src=u32>
    auto set(T_Src Val,  bool IsNegative=false, i32 Exponent=0) -> BigDecimal&;
    template <std::integral T_Slot=u32>
    auto set(T_Slot *ValArray, u32 ValCount, bool IsNegative=false, i32 Exponent=0) -> BigDecimal&;

    auto set_float(real32 Val) -> void;
    auto set_double(f64 Val) -> void;

    auto less_than_integer_signed(BigDecimal<T_Alloc>& B) -> bool;
    auto less_than_integer_unsigned(BigDecimal<T_Alloc>& B) ->bool;
    auto equals_integer(BigDecimal<T_Alloc> const& B) -> bool;
    auto greater_equals_integer(BigDecimal<T_Alloc>& B) -> bool;
    auto equals_fractional(BigDecimal<T_Alloc> const& B) -> bool;
    auto equal_bits(BigDecimal<T_Alloc> const& B) -> bool;


    auto shift_left(u32 ShiftAmount) -> BigDecimal&;
    auto shift_right(u32 ShiftAmount) -> BigDecimal&;

    auto add_integer_unsigned (BigDecimal& B) -> void;
    auto add_integer_signed (BigDecimal& B) -> void;

    auto sub_integer_unsigned_positive (BigDecimal& B)-> void;
    auto sub_integer_unsigned (BigDecimal& B) -> void;
    auto sub_integer_signed (BigDecimal& B) -> void;

    auto mul_integer(BigDecimal& B) -> void;
    auto div_integer (BigDecimal& B, u32 MinFracPrecision=32) -> void;

    auto add_fractional (BigDecimal& B) -> void;
    auto sub_fractional (BigDecimal& B) -> void;
    auto mul_fractional(BigDecimal& B) -> void;
    auto div_fractional (BigDecimal& B, u32 MinFracPrecision=32) -> void;

    auto round_to_n_significant_bits(i32 N) -> void;

    explicit operator std::string();


    auto to_float () -> f32;
    auto to_double () -> f64;



    /* Helper and Convenience Functions */
    template <typename T_Slot>
    auto copy_bits_to(T_Slot *bits_array, i32 slot_count) -> void;


    auto set_bits_64(u64 value) -> void;

    template<typename T_Slot>
    auto set_bits_array(T_Slot *vals, u32 count) -> void;

    auto is_zero() -> bool;
    auto neg() -> void;
    auto zero(flags32 what = ZERO_DIGITS_ONLY) -> void;
    auto get_msb() -> i32;
    auto get_bit(i32 N) -> bool;
    auto count_bits() -> i32;
    auto get_least_significant_exponent() -> i32;
    auto get_head() -> ChunkList*;
    auto truncate_trailing_zero_bits() -> void;
    auto truncate_leading_zero_chunks() -> void;
    auto is_normalized_fractional() -> bool;
    auto is_normalized_integer() -> bool;
    auto UpdateLength() -> void;

    static auto parse_integer(char *Src, BigDecimal *Dst = nullptr) -> bool;
    static bool parse_fraction(char *FracStr, BigDecimal *Dst = nullptr);

    bool is_context_variable() {
        return m_chunk_alloc == s_chunk_alloc;
    }

    void add_context_link() {
        //prepend to list
        Link *new_link = std::allocator_traits<LinkAlloc>::allocate(s_link_alloc, 1);
        new_link->next = s_ctx_links;
        new_link->value = this;
        s_ctx_links = new_link;

        ++s_ctx_count;
        return;
    }


    Link *find_predecessor(BigDecimal *them) {
        HardAssert(them);
        HardAssert(s_ctx_links);
        if (s_ctx_links->value == them) { return nullptr; }
        Link *predecessor = s_ctx_links;
        while (predecessor->next && predecessor->next->value != them) {
            predecessor = predecessor->next;
        }
        return predecessor;
    }

    void remove_successor_or_head(Link *them) {
        if (!them) {
            HardAssert(s_ctx_links);
            Link *new_head = s_ctx_links->next;
            std::allocator_traits<LinkAlloc>::deallocate(s_link_alloc, s_ctx_links, sizeof(Link));
            s_ctx_links = new_head;
            return;
        }

        if (them->next) {
            Link *new_successor = them->next->next;
            std::allocator_traits<LinkAlloc>::deallocate(s_link_alloc, them->next, sizeof(Link));
            them->next = new_successor;
        }
        return;
    }

    void remove_context_link() {
        HardAssert(is_context_variable());

        //find in list and remove
        Link *our_preceding_link = find_predecessor(this);
        //NOTE(ArokhSlade##2024 08 29): every context variable has a link.
        HardAssert(s_ctx_links->value == this || our_preceding_link && our_preceding_link->next->value == this);
        remove_successor_or_head(our_preceding_link);

        --s_ctx_count;
        return;
    }

    //TODO(ArokhSlade##2024 09 29): not needed anymore? delete
    ChunkAlloc& get_chunk_alloc() {
        HardAssert(s_is_context_initialized);
        return s_chunk_alloc;
    }

    static T_Alloc& get_ctx_alloc() {
        HardAssert(s_is_context_initialized);
        return s_ctx_alloc;
    }


    //TODO(ArokhSlade##2024 09 29): redundant? why not ambiguous? is it ever called?
    BigDecimal(T_Alloc& allocator)
    : m_chunk_alloc{ChunkAlloc{allocator}}, IsNegative{false},data{.value{0}},
      is_alive{true}
    {
        HardAssert(s_is_context_initialized);
        if (is_context_variable()) add_context_link();
    }


    template <std::integral T_Src = u32>
    BigDecimal(const T_Alloc& allocator, T_Src Value_=0, bool IsNegative_=false, i32 Exponent_=0)
    : m_chunk_alloc{ChunkAlloc{allocator}}, IsNegative{IsNegative_},
      Exponent{Exponent_}, is_alive{true}
    {
        HardAssert(s_is_context_initialized);
        if (is_context_variable()) add_context_link();
        set_bits_64(Value_);
    }

    BigDecimal(const T_Alloc& allocator, f32 Value_)
    : BigDecimal(allocator)
    {
        set_float(Value_);
    }

    BigDecimal(const T_Alloc& allocator, f64 Value_)
    : BigDecimal(allocator)
    {
        set_double(Value_);
    }

    template <std::integral T_Src = u32>
    BigDecimal(const T_Alloc& allocator, T_Src *Values, u32 Count, bool IsNegative_=false, i32 Exponent_=0)
    : BigDecimal{allocator, 0, IsNegative_, Exponent_}
    {
        set_bits_array<T_Src>(Values, Count);
        HardAssert(is_normalized_integer());
    }

    template <std::integral T_Src = u32>
    BigDecimal(T_Src Value_=0, bool IsNegative_=false, i32 Exponent_=0)
    : BigDecimal{get_ctx_alloc(), Value_, IsNegative_, Exponent_}
    {}

    BigDecimal(f32 Value_)
    : BigDecimal{get_ctx_alloc(), Value_}
    {}

    BigDecimal(f64 Value_)
    : BigDecimal{get_ctx_alloc(), Value_}
    {}

    template<std::integral T_Src=u32>
    BigDecimal(T_Src *Values, u32 ValuesCount, bool IsNegative_=false, i32 Exponent_=0)
    : BigDecimal{get_ctx_alloc(),Values, ValuesCount, IsNegative_, Exponent_}
    {}



    BigDecimal(T_Alloc& allocator, BigDecimal& Other)
    : m_chunk_alloc{ChunkAlloc{allocator}}, IsNegative{Other.IsNegative},
      Exponent{Other.Exponent}, is_alive{true}
    {
        HardAssert(s_is_context_initialized);
        Other.copy_to(this, COPY_DIGITS);
        if (is_context_variable()) add_context_link();
    }



    BigDecimal(BigDecimal& other)
    : m_chunk_alloc{other.m_chunk_alloc}, is_alive{other.is_alive}
    {
        HardAssert(is_alive);
        other.copy_to(this, COPY_EVERYTHING);
        add_context_link();
    }
    BigDecimal(const BigDecimal& other) = delete;

    void release() {
        HardAssert(is_alive);
        is_alive = false;

        //delete all chunks through allocator
        ChunkList *chunk = sentinel;
        ChunkList *prev;
        i32 CountDeleted = 0;
        i32 old_capacity = m_chunks_capacity;

        HardAssert(sentinel->next == nullptr);
        HardAssert(data.prev == nullptr);

        while (chunk != &data) {
            HardAssert(chunk);
            prev = chunk->prev;
            std::allocator_traits<ChunkAlloc>::deallocate(m_chunk_alloc, chunk, sizeof(ChunkList));
            chunk = prev;
            chunk->next = nullptr;
            --m_chunks_capacity;
            ++CountDeleted;
        }

        HardAssert(CountDeleted+1 == old_capacity);
        HardAssert(m_chunks_capacity == 1);
        HardAssert(data.next == nullptr);
        this->zero(ZERO_EVERYTHING);

        length = 1;
        sentinel = &data;

        if (is_context_variable()) {
            remove_context_link();
        }

        m_chunk_alloc.~ChunkAlloc();

        return;
    }

    ~BigDecimal() {
        if (!is_alive) return;

        release();
    }

    /* issues:
     - shallow or deep copy?
     - internal pointers (sentinel)
     - other with other allocator
     - other with other allocator type
     - copy other's allocator or only their values with our current allocator?
    */
    BigDecimal& operator=(const BigDecimal& other){
        return *this;
    }

    private:

    //NOTE(ArokhSlade##2024 08 28): this is more or less a work-around
    //because some constructor WILL be called at start-up for statics, which I don't actually want
    //so call this special constructor that does nothing
    //then later, when initialize_context() is called,
    //these variables get re-assigned, setting their allocator and incrementing the context variables counter
    BigDecimal(SpecialConstants must_belong_to_context, u32 value=0,
               bool IsNegative_=false, i32 Exponent_=0)
    : m_chunk_alloc{}, data{.value{value}},
      IsNegative{IsNegative_}, Exponent{Exponent_},
      is_alive{false} //NOTE()ArokhSlade##2024 08 29):allocator for static temporaries not known at startup
    {
        HardAssert(must_belong_to_context == SpecialConstants::BELONGS_TO_CONTEXT);
    }

    BigDecimal(SpecialConstants must_belong_to_context, ChunkAlloc& chunk_alloc, u32 value=0,
               bool IsNegative_=false, i32 Exponent_=0)
    : m_chunk_alloc{chunk_alloc}, data{.value{value}},
      IsNegative{IsNegative_}, Exponent{Exponent_},
      is_alive{true} //NOTE(ArokhSlade##2024 08 29): allocator for static temporaries not known at startup
    {
        HardAssert(must_belong_to_context == SpecialConstants::BELONGS_TO_CONTEXT);
        add_context_link();
    }

    void initialize(ChunkAlloc& chunk_alloc) {
        m_chunk_alloc = chunk_alloc;
        is_alive = true;
        add_context_link();
    }
};


template <typename T_Alloc>
auto Str(BigDecimal<T_Alloc>& A, memory_arena *TempArena) -> char* {

    char Sign = A.IsNegative ? '-' : '+';
    typename BigDecimal<T_Alloc>::ChunkList *CurChunk = A.get_head();
    i32 UnusedChunks = A.m_chunks_capacity - A.length;
    char HexDigits[11] = "";

    i32 NumSize = UnusedChunks > 0 ? Log10I(UnusedChunks ) : 1;
    i32 UnusedChunksFieldSize = 2 + NumSize + 1;

    i32 BufSize = 1+A.length*9+2+UnusedChunksFieldSize+1;
    char *Buf = PushArray(TempArena, BufSize, char);

    char *BufPos=Buf;
    *BufPos++ = Sign;

    for (i32 Idx = 0 ; Idx < A.length; ++Idx, CurChunk = CurChunk->prev) {
        stbsp_sprintf(BufPos, "%.8x ", CurChunk->value);
        BufPos +=9;
    }

    stbsp_sprintf(BufPos, "[%.*d]", NumSize, UnusedChunks);
    BufPos += UnusedChunksFieldSize;

    *BufPos = '\0';

    return Buf;
}




template <typename T_Alloc>
auto BigDecimal<T_Alloc>::parse_integer(char *Src, BigDecimal *Dst) -> bool {

    if (!Dst) Dst = &BigDecimal<T_Alloc>::temp_parse_int;

    Dst->zero(ZERO_EVERYTHING);

    if (Src == nullptr || !IsNum(*Src)) return false;


    char *LastDigit = Src;
    while (IsNum(LastDigit[1])) {
        ++LastDigit;
    }

    BigDecimal<T_Alloc>& Digit_ = BigDecimal<T_Alloc>::temp_digit;
    BigDecimal<T_Alloc>& Pow10_ = BigDecimal<T_Alloc>::temp_pow_10;
    BigDecimal<T_Alloc>& Ten_ = BigDecimal<T_Alloc>::temp_ten;

    Pow10_.set(1);
    Ten_.set(10);

    auto ToDigit = [](char c){return static_cast<int>(c-'0');};

    for (char *DigitP = LastDigit ; DigitP >= Src ; --DigitP) {
        int D = ToDigit(*DigitP);
        Digit_.set(D);
        Digit_.mul_integer(Pow10_);
        Dst->add_integer_signed(Digit_);
        Pow10_.mul_integer(Ten_);
    }

    Dst->Exponent = Dst->get_msb();

    return true;
}


template <typename T_Alloc>
auto BigDecimal<T_Alloc>::parse_fraction(char *FracStr, BigDecimal<T_Alloc> *Dst) -> bool{

    HardAssert(!!FracStr);

    if (!Dst) Dst = &BigDecimal<T_Alloc>::temp_parse_frac;

    Dst->zero(ZERO_EVERYTHING);

    BigDecimal<T_Alloc>& digit = BigDecimal<T_Alloc>::temp_digit;
    BigDecimal<T_Alloc>& pow10 = BigDecimal<T_Alloc>::temp_pow_10;
    BigDecimal<T_Alloc>& ten = BigDecimal<T_Alloc>::temp_ten;

    i32 StrLen = 0;
    for (char *Cur=FracStr; IsNum(*Cur) ; ++Cur) {
        ++StrLen;
    }
    if (StrLen == 0) return false;

    pow10.set(1,0,0);
    ten.set(10,0,Log2I(10));
    ten.normalize();
    digit.zero(ZERO_EVERYTHING);

    for (int32 i = 0 ; i < StrLen ; ++i) {
        pow10.div_fractional(ten,128);

        int32 Digit = static_cast<i32>(FracStr[i]-'0');
        int32 DigitExp = Digit == 0 ? 0 : Log2I(Digit);
        digit.set(Digit,false,DigitExp);
        digit.normalize();

        digit.mul_fractional(pow10);

        Dst->add_fractional(digit);
    }

    HardAssert(Dst->is_normalized_fractional());

    return true;
}


template <typename T_Alloc>
auto BigDecimal<T_Alloc>::from_string(char *DecStr, BigDecimal<T_Alloc> *Dst) -> bool {
    using BigDec = BigDecimal<T_Alloc>;

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

    if (!parse_integer(DecStr, Dst)) return false;
    Dst->normalize();

    if (FoundPoint) {
        char *FractionStart = DecStr+PointPos+1;
        BigDec *Frac = &temp_parse_frac;
        if (!parse_fraction(DecStr+PointPos+1, Frac)) return false;
        Dst->add_fractional(*Frac);
    }

    Dst->IsNegative = Neg;

    HardAssert(Dst->is_normalized_fractional());

    return bool(Dst);
}


/**
 *  \brief  A.length == 1 && A.data.value == 0x0
 *  \n      Exponent does not matter. 0^0 is still zero
**/
template <typename T_Alloc>
auto BigDecimal<T_Alloc>::is_zero() -> bool {
    return length == 1 && data.value == 0x0;
}

//TODO(ArokhSlade##2024 08 27): why does this not work?
//template <typename T_Alloc>
//using is_ctx_var = BigDecimal<T_Alloc>::BELONGS_TO_CONTEXT;

//template <typename T_Alloc>
//constexpr auto is_ctx_var = BigDecimal<T_Alloc>::BELONGS_TO_CONTEXT;

template <typename T_Alloc>
BigDecimal<T_Alloc> BigDecimal<T_Alloc>::temp_add_fractional {BigDecimal<T_Alloc>::SpecialConstants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
BigDecimal<T_Alloc> BigDecimal<T_Alloc>::temp_sub_int_unsign {BigDecimal<T_Alloc>::SpecialConstants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
BigDecimal<T_Alloc> BigDecimal<T_Alloc>::temp_sub_frac{BigDecimal<T_Alloc>::SpecialConstants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
BigDecimal<T_Alloc> BigDecimal<T_Alloc>::temp_mul_int_0 {BigDecimal<T_Alloc>::SpecialConstants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
BigDecimal<T_Alloc> BigDecimal<T_Alloc>::temp_mul_int_1 {BigDecimal<T_Alloc>::SpecialConstants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
BigDecimal<T_Alloc> BigDecimal<T_Alloc>::temp_mul_int_carries {BigDecimal<T_Alloc>::SpecialConstants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
BigDecimal<T_Alloc> BigDecimal<T_Alloc>::temp_one {BigDecimal<T_Alloc>::SpecialConstants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
BigDecimal<T_Alloc> BigDecimal<T_Alloc>::temp_div_int_a{BigDecimal<T_Alloc>::SpecialConstants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
BigDecimal<T_Alloc> BigDecimal<T_Alloc>::temp_div_int_b{BigDecimal<T_Alloc>::SpecialConstants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
BigDecimal<T_Alloc> BigDecimal<T_Alloc>::temp_div_int_0{BigDecimal<T_Alloc>::SpecialConstants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
BigDecimal<T_Alloc> BigDecimal<T_Alloc>::temp_div_frac{BigDecimal<T_Alloc>::SpecialConstants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
BigDecimal<T_Alloc> BigDecimal<T_Alloc>::temp_div_frac_int_part{BigDecimal<T_Alloc>::SpecialConstants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
BigDecimal<T_Alloc> BigDecimal<T_Alloc>::temp_div_frac_frac_part{BigDecimal<T_Alloc>::SpecialConstants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
BigDecimal<T_Alloc> BigDecimal<T_Alloc>::temp_pow_10{BigDecimal<T_Alloc>::SpecialConstants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
BigDecimal<T_Alloc> BigDecimal<T_Alloc>::temp_ten{BigDecimal<T_Alloc>::SpecialConstants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
BigDecimal<T_Alloc> BigDecimal<T_Alloc>::temp_digit{BigDecimal<T_Alloc>::SpecialConstants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
BigDecimal<T_Alloc> BigDecimal<T_Alloc>::temp_to_float{BigDecimal<T_Alloc>::SpecialConstants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
BigDecimal<T_Alloc> BigDecimal<T_Alloc>::temp_parse_int{BigDecimal<T_Alloc>::SpecialConstants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
BigDecimal<T_Alloc> BigDecimal<T_Alloc>::temp_parse_frac{BigDecimal<T_Alloc>::SpecialConstants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
BigDecimal<T_Alloc> BigDecimal<T_Alloc>::temp_from_string{BigDecimal<T_Alloc>::SpecialConstants::BELONGS_TO_CONTEXT};

template <typename T_Alloc>
BigDecimal<T_Alloc> *BigDecimal<T_Alloc>::s_all_temporaries_ptrs[TEMPORARIES_COUNT] = {
        &temp_add_fractional, &temp_sub_int_unsign, &temp_sub_frac, &temp_mul_int_0, &temp_mul_int_1,
        &temp_mul_int_carries, &temp_div_int_a, &temp_div_int_b, &temp_div_int_0, &temp_div_frac,
        &temp_div_frac_int_part, &temp_div_frac_frac_part, &temp_pow_10, &temp_one, &temp_ten,
        &temp_digit, &temp_to_float, &temp_parse_int, &temp_parse_frac, &temp_from_string
    };


template <typename T_Alloc>
bool BigDecimal<T_Alloc>::s_is_context_initialized {false};

template <typename T_Alloc>
i32 BigDecimal<T_Alloc>::s_ctx_count{0};

template <typename T_Alloc>
T_Alloc BigDecimal<T_Alloc>::s_ctx_alloc{};

template <typename T_Alloc>
BigDecimal<T_Alloc>::ChunkAlloc BigDecimal<T_Alloc>::s_chunk_alloc{};

template <typename T_Alloc>
BigDecimal<T_Alloc>::LinkAlloc BigDecimal<T_Alloc>::s_link_alloc{BigDecimal<T_Alloc>::s_chunk_alloc};

template <typename T_Alloc>
BigDecimal<T_Alloc>::Link *BigDecimal<T_Alloc>::s_ctx_links = nullptr;




/**
 * \note will return nullptr if given index exceeds capacity
 * \note may return a chunk beyond `length`, i.e. that's currently not in use,
 * i.e. that currently doesn't contain a part of the number's current value
 */
template <typename T_Alloc>
auto BigDecimal<T_Alloc>::get_chunk(u32 Idx) -> ChunkList * {
    ChunkList *Result = nullptr;
    if (Idx < m_chunks_capacity) {
        Result = &data;
        while (Idx-- > 0) {
            Result = Result ->next;
        }
    }
    return Result;
}

template <typename T_Alloc>
auto BigDecimal<T_Alloc>::get_head() -> ChunkList* {
    return get_chunk(length-1);
}

/**
 * \brief Allocates another chunk
 */
template <typename T_Alloc>
//TODO(## 2023 11 23): support List nodes that are multiple chunks long?
auto BigDecimal<T_Alloc>::expand_capacity() -> ChunkList* {
    ChunkList *NewChunk = (ChunkList*)ChunkAllocTraits::allocate(m_chunk_alloc, 1);
    HardAssert(NewChunk != nullptr);
    *NewChunk = {.value = 0, .next = nullptr, .prev = nullptr};
    sentinel->next = NewChunk;
    NewChunk->prev = sentinel;
    sentinel = NewChunk;
    ++m_chunks_capacity;

    return NewChunk;
}

/**
 * \brief increase length, allocate when appropriate, return pointer to last chunk (index == length-1) in list.
 *     \n NOTE: Sets the new chunk's value to Zero.
 */
template <typename T_Alloc>
auto BigDecimal<T_Alloc>::extend_length() -> ChunkList* {
    HardAssert(length <= m_chunks_capacity);
    ++length;
    ChunkList *Result = get_head();
    if (Result == nullptr) {
        Result = expand_capacity();
    }
    Result->value = 0;

    return Result;
}

/** \brief Unisgned Add, pretend both numbers are positive integers, ignore exponents

 */
template <typename T_Alloc>
//TODO(## 2023 11 18) : test case where allocator returns nullptr
auto BigDecimal<T_Alloc>::add_integer_unsigned (BigDecimal& B) -> void {

    HardAssert(this->is_normalized_integer());

    BigDecimal& A = *this;

    A.UpdateLength();
    B.UpdateLength();


    u32 LongerLength =  A.length >= B.length ? A.length : B.length;
    u32 ShorterLength =  A.length <= B.length ? A.length : B.length;
    ChunkList *ChunkA = &A.data, *ChunkB = &B.data;

    ChunkBits OldCarry = 0;
    //TODO(##2023 11 23): if (A.length < B.length) {/*allocate and append B.length-A.length chunks}
    for ( u32 Idx = 0
        ; Idx < LongerLength
        ; ++Idx,
          ChunkA = ChunkA ? ChunkA->next : nullptr,
          ChunkB = ChunkB ? ChunkB->next : nullptr ) {

        if ( Idx >= A.length)  {
            ChunkA = A.extend_length();
        }

        ChunkBits ChunkBValue = Idx < B.length ? ChunkB->value : 0U;
        bool NewCarry = MAX_CHUNK_VAL - ChunkBValue - OldCarry < ChunkA->value;
        ChunkA->value += ChunkBValue + OldCarry; //wrap-around is fine
        OldCarry = NewCarry;
    }

    if (OldCarry) {
        ChunkA = extend_length();
        ChunkA->value = OldCarry;
    }

    return;

    HardAssert(this->is_normalized_integer());
}

template <typename T_Alloc>
//TODO(## 2023 11 18) : test case where allocator returns nullptr
auto BigDecimal<T_Alloc>::add_integer_signed (BigDecimal& B) -> void {

    HardAssert(this->is_normalized_integer());

    BigDecimal& A = *this;
    if (A.IsNegative == B.IsNegative) {
        add_integer_unsigned(B);
    } else if (A.IsNegative) {
        A.neg();
        A.sub_integer_signed(B);
        A.neg();
    } else { /* (B.IsNegative) */
        B.neg();
        A.sub_integer_signed(B);
        B.neg();
    }

    HardAssert(this->is_normalized_integer());

    return ;
}

template <typename T_Alloc>
auto BigDecimal<T_Alloc>::truncate_trailing_zero_bits() -> void {
    if (this->is_zero()) { length = 1; return; }
    i32 TruncCount = 0;
    i32 FirstOne = 0;
    bool BitFound = false;
    ChunkList *Current = &this->data;
    for (i32 Block = 0 ; Block < length; ++Block, Current = Current -> next) {
        FirstOne = BitScan<ChunkBits>(Current->value);
        if (FirstOne != BIT_SCAN_NO_HIT) {
            TruncCount += FirstOne;
            BitFound = true;
            break;
        }
        TruncCount += CHUNK_WIDTH;
    }
    shift_right(TruncCount);
    return;
}


template <typename T_Alloc>
auto BigDecimal<T_Alloc>::truncate_leading_zero_chunks() -> void {
    for (ChunkList *Cur{ get_head() } ; Cur != &data ; Cur = Cur->prev )
    {
        if (Cur->value == 0x0) --length;
        else break;
    };
    HardAssert (length >= 1);
}

/** \brief Gets rid of leading zero chunks and trailing zeros, exponent doesn't change, e.g. 10100 E=2 becomes 101 E=2, NOT 101 E=4.
 *  \n that's because the bits are interpreted as 1.0100E2
 **/
template <typename T_Alloc>
auto BigDecimal<T_Alloc>::normalize() -> void {
    truncate_leading_zero_chunks();
    truncate_trailing_zero_bits();
}


template <typename T_Alloc>
auto BigDecimal<T_Alloc>::is_normalized_fractional() -> bool {
    if (this->is_zero()) return true;
    bool has_trailing_zeros = (data.value&0x1) == 0;
    bool has_leading_zero_chunks = get_head()->value == 0x0;
    if ( has_trailing_zeros || has_leading_zero_chunks) return false;
    return true;
}

template <typename T_Alloc>
auto BigDecimal<T_Alloc>::is_normalized_integer() -> bool {
    return this->is_zero() || get_head()->value != 0x0;
}


/** \brief  if length is too long (i.e. includes leading zero chunks)
 *          this function fixes it.
 *          new length can never be greater than old length.
 *  \note   does not go beyond the old length, because there might be garbage values there.
**/
template <typename T_Alloc>
auto BigDecimal<T_Alloc>::UpdateLength() -> void {
    ChunkList *chunk = &data;
    i32 chunks_visited = 1;
    i32 new_length = 1;
    while (chunks_visited < length && (chunk = chunk->next)) {
        ++chunks_visited;

        if (chunk->value != 0x0) {
            new_length = chunks_visited;
        }
    }
    this->length = new_length;
    return;
}

template <typename T_Alloc>
auto BigDecimal<T_Alloc>::copy_to(BigDecimal *Dst, flags32 Flags) -> void {
    HardAssert(Dst != nullptr);

    if (IsSet(Flags, COPY_SIGN)) Dst->IsNegative = this->IsNegative;
    if (IsSet(Flags, COPY_EXPONENT)) Dst->Exponent = this->Exponent;


    if (IsSet(Flags, COPY_DIGITS)) {

        ChunkList *Ours = &data, *Theirs = &Dst->data;

        while (Dst->length < length) Dst->extend_length();

        Dst->length = length; //if Dst->length > this->length

        for ( u32 Idx = 0 ; Idx < length ; ++Idx ) {

            Theirs->value = Ours->value;

            Ours = Ours->next;
            Theirs = Theirs->next;
        }
    }

    return;
}



/**
 *  \brief tells whether Abs(A) < Abs(B)
**/
template <typename T_Alloc>
auto BigDecimal<T_Alloc>::less_than_integer_unsigned(BigDecimal<T_Alloc>& B) ->bool{

    BigDecimal<T_Alloc>& A = *this;
    bool Result = false;
    u32 length = A.length;
    if (A.length == B.length) {
        for ( typename BigDecimal<T_Alloc>::ChunkList
                *A_chunk = A.get_head(),
                *B_chunk = B.get_head()
            ; length > 0
            ; A_chunk = A_chunk->prev, B_chunk = B_chunk->prev, --length) {
            if (A_chunk->value == B_chunk->value) {
                continue;
            }
            Result = A_chunk->value < B_chunk->value;
            break;
        }
    } else {
        Result = A.length < B.length;
    }

    return Result;
}


template <typename T_Alloc>
auto BigDecimal<T_Alloc>::less_than_integer_signed(BigDecimal<T_Alloc>& B) -> bool{
    BigDecimal<T_Alloc>& A = *this;
    bool Result = false;
    if (A.IsNegative != B.IsNegative) {
        Result = A.IsNegative;
    } else {
        Result = !A.IsNegative && A.less_than_integer_unsigned(B)
               || A.IsNegative && B.less_than_integer_unsigned(A);
    }
    return Result;
}


template <typename T_Alloc>
auto BigDecimal<T_Alloc>::equal_bits(BigDecimal<T_Alloc> const& B) -> bool {

    BigDecimal<T_Alloc>& A = *this;
    bool all_equal = true;
    all_equal &= A.length == B.length;

    using p_chunk = typename BigDecimal<T_Alloc>::ChunkList const *;
    p_chunk chunk_A = &A.data, chunk_B = &B.data;

    for ( u32 idx = 0 ; idx < A.length && all_equal ; ++idx ) {

        all_equal &= chunk_A->value == chunk_B->value;

        chunk_A = chunk_A->next;
        chunk_B = chunk_B->next;
    }

    return all_equal;
}

template <typename T_Alloc>
auto BigDecimal<T_Alloc>::equals_integer(BigDecimal<T_Alloc> const& B) -> bool {

    BigDecimal<T_Alloc>& A = *this;
    bool all_equal = true;
    all_equal &= A.IsNegative == B.IsNegative;
    all_equal &= A.equal_bits(B);

    return all_equal;
}

template <typename T_Alloc>
auto BigDecimal<T_Alloc>::equals_fractional(BigDecimal<T_Alloc> const& B) -> bool {

    BigDecimal<T_Alloc>& A = *this;
    bool all_equal = true;
    all_equal &= A.equals_integer(B);
    all_equal &= A.Exponent == B.Exponent;

    return all_equal;
}


//TODO(##2024 06 08) param Allocator not used
template <typename T_Alloc>
auto BigDecimal<T_Alloc>::greater_equals_integer(BigDecimal<T_Alloc>& B) -> bool {
    BigDecimal<T_Alloc>& A = *this;
    return B.less_than_integer_signed( A ) || A.equals_integer( B );
}

/**
\brief  subtraction algorithm for the special case where both operands are positive and minuend >= subtrahend
 */
template <typename T_Alloc>
auto BigDecimal<T_Alloc>::sub_integer_unsigned_positive (BigDecimal& B)-> void {

    HardAssert(this->is_normalized_integer());

    BigDecimal& A = *this;

    A.UpdateLength();
    B.UpdateLength();

    HardAssert(A.greater_equals_integer(B));

    ChunkList *ChunkA = &A.data;
    ChunkList *ChunkB = &B.data;
    u32 LeadingZeroBlocks = 0;
    u32 iterations = [](BigDecimal<T_Alloc>& X){
        ChunkList *cur = &X.data;
        i32 chunks_visited = 1;
        i32 significant_chunks = 1;
        while ((cur = cur->next) && chunks_visited < X.length) {
            ++chunks_visited;
            if (cur->value != 0x0) {
                significant_chunks = chunks_visited;
            }
        }
        return significant_chunks;
    }(B);

    HardAssert(B.length == iterations);

    ChunkBits Carry = 0;

    for (u32 Idx = 0 ; Idx < iterations; ++Idx) {
        if (ChunkB->value == MAX_CHUNK_VAL && Carry) {
            Carry = 1;
        } else if (ChunkA->value >= ChunkB->value + Carry) {
            ChunkA->value -= ChunkB->value + Carry;
            Carry = 0;
        } else {
            ChunkA->value += MAX_CHUNK_VAL - (ChunkB->value + Carry) + 1;
            Carry = 1;
        }
        ChunkA = ChunkA->next;
        ChunkB = ChunkB->next;
    }

    if (Carry) {
        HardAssert (A.length > B.length);
        ChunkA->value -= Carry;
        if (ChunkA->value == 0x0 && A.get_head() == ChunkA) {
            A.UpdateLength();
        }
    }

    this->truncate_leading_zero_chunks();

    HardAssert(this->is_normalized_integer());

    return;
}


/**
\brief  subtraction algorithm for the special case where both operands are positive,
        but the difference may cross zero, i.e. subtrahend may be greater than minuend
 */
template <typename T_Alloc>
auto BigDecimal<T_Alloc>::sub_integer_unsigned (BigDecimal& B)-> void {

    HardAssert(this->is_normalized_integer());

    BigDecimal& A = *this;

    bool crosses_zero = false;
    if (A.less_than_integer_signed(B)) {
        A.copy_to(&temp_sub_int_unsign);
        B.copy_to(&A, COPY_DIGITS | COPY_SIGN); //SubInteger does not care about exponent, but callers might, so we keep it intact
        crosses_zero = true;
    }
    BigDecimal& B_ = crosses_zero ? temp_sub_int_unsign : B;

    A.sub_integer_unsigned_positive(B_);

    if (crosses_zero) { A.neg(); }

    HardAssert(this->is_normalized_integer());

    return;
}


/**
\brief  subtraction algorithm that will treat both operands as integers,
        i.e. will ignore exponents and just take the bits at face value.
        will respect signs, however.
*/
template <typename T_Alloc>
auto BigDecimal<T_Alloc>::sub_integer_signed (BigDecimal& B)-> void {

    HardAssert(this->is_normalized_integer());

    BigDecimal& A = *this;

    if (!A.IsNegative && !B.IsNegative) {

        A.sub_integer_unsigned(B);

    } else if (A.IsNegative && B.IsNegative ) {

        A.neg();
        B.neg();
        A.sub_integer_unsigned(B);
        B.neg();
        A.neg();

    } else if (A.IsNegative) {

        A.neg();
        A.add_integer_unsigned(B);
        A.neg();

    } else { /* (B.IsNegative) */

        B.neg();
        A.add_integer_unsigned(B);
        B.neg();
    }

    truncate_leading_zero_chunks();

    HardAssert(this->is_normalized_integer());

    return;
}

/**
 *  \brief  BIT-shift. simply shifts the bits, interpreted as ... sequence of bits, i.e. as unsigned integer.
 *  \n      exponent does not matter. decimal point does not matter.
 *  \note   PRE-condition: this->is_normalized_integer()
 *  \note   POST-condition: this->is_normalized_integer()
 *  \note   naturally, the result is not guaranted to be a noramlized Fractional, i.e. may have trailing zeros
**/
template <typename T_Alloc>
auto BigDecimal<T_Alloc>::shift_left(u32 ShiftAmount) -> BigDecimal& {

    HardAssert(this->is_normalized_integer());

    if (this->is_zero()) {
        return *this;
    }

    ChunkList *Head = get_head();

    HardAssert(Head->value != 0x0); //TODO(ArokhSlade##2024 08 20): support denormalized numbers?

    constexpr u32 BitWidth = sizeof(ChunkBits) * 8;
    u32 OldTopIdx = BitScanReverse<u64>(Head->value);
    HardAssert(OldTopIdx != BIT_SCAN_NO_HIT);
    u32 HeadZeros = BitWidth - 1 - OldTopIdx;
    i32 Overflow = ShiftAmount - HeadZeros;

    u32 NeededChunks = 0;
    if (ShiftAmount > HeadZeros ) {
        NeededChunks = Overflow / BitWidth; //TODO(ArokhSlade#2024 10 04):replace with DivCeil()
        if (Overflow % BitWidth != 0) { NeededChunks++; }
    }

    for (u32 FreeChunks = m_chunks_capacity - length; NeededChunks > FreeChunks; ++FreeChunks) {
        expand_capacity();
    }
    //TODO(ArokhSlade##2024 10 04):can probably just replace this with DstChunk = extend_length(NeededChunks)
    ChunkList *DstChunk = get_chunk(length-1 + NeededChunks);
    ChunkList *SrcChunk = Head;

    u32 Offset = ShiftAmount % BitWidth;
    u32 NewTopIdx = (OldTopIdx + Offset) % BitWidth;

    if (NewTopIdx < OldTopIdx) {
        DstChunk->value = SrcChunk->value >> BitWidth-Offset; //NOTE(ArokhSlade##2024 10 06): inside this `if`, Offset cannot be zero, and amount shifted by is guaranteed to be >= 0, < width
        DstChunk = DstChunk->prev;
    }

    for (i32 i = 0 ; i < length-1 ; ++i, DstChunk = DstChunk->prev, SrcChunk = SrcChunk->prev) {

        ChunkBits Left = SrcChunk->value << Offset;
        ChunkBits Right = SafeShiftRight(SrcChunk->prev->value, BitWidth-Offset); //NOTE(ArokhSlade##2024 08 20): regular shift would cause UB when Offset == 0

        DstChunk->value = Left | Right;
    }

    HardAssert(SrcChunk == &data);
    DstChunk->value = SrcChunk->value << Offset;

    for (DstChunk = DstChunk->prev ; DstChunk ; DstChunk = DstChunk->prev) {
        DstChunk->value = 0x0;
    }

    length += NeededChunks;
    HardAssert(length <= m_chunks_capacity);

    HardAssert(this->is_normalized_integer());

    return *this;
}

/**
 *  \brief  BIT-shift. simply shifts the bits, interpreted as ... sequence of bits, i.e. as unsigned integer.
 *  \n      exponent does not matter. decimal point does not matter.
 *  \note   PRE-condition: this->is_normalized_integer()
 *  \note   POS-condition: this->is_normalized_integer()
 *  \note   naturally, the result is not guaranted to be a noramlized Fractional, i.e. may have trailing zeros
**/
template <typename T_Alloc>
auto BigDecimal<T_Alloc>::shift_right(u32 ShiftAmount) -> BigDecimal& {

    HardAssert(this->is_normalized_integer());

    if (ShiftAmount == 0) { return *this; }
    if (this->is_zero()) { return *this; } //NOTE(ArokhSlade##2024 08 19): this normalizes Zero without calling normalize() (no infinite loop)

    i32 BitCount = count_bits();
    if (ShiftAmount >= BitCount) { this->zero(ZERO_DIGITS); return *this; }

    u32 BitWidth = CHUNK_WIDTH;
    u32 Offset = ShiftAmount % BitWidth;
    i32 ChunksShifted = ShiftAmount / BitWidth;

    ChunkList *Src = get_chunk(ChunksShifted);
    ChunkList *Dst = &data;
    i32 IterCount = 0;

    for (i32 i_Src = ChunksShifted  ; i_Src < length-1 ; ++i_Src, ++IterCount) {
        ChunkBits Right = Src->value >> Offset;
        ChunkBits Left  = Offset == 0 ? 0x0 : Src->next->value << BitWidth-Offset; //NOTE(ArokhSlade##2024 08 20): x>>y UB for y >= BitWidth. according to standard.
        Dst->value = Left | Right;

        Src = Src->next;
        Dst = Dst->next;
    }

    Dst->value = Src->value >> Offset;
    if (Dst->value) IterCount++;

    length = IterCount;

    HardAssert(this->is_normalized_integer());

    return *this;
}

template <typename uN>
void FullMulN(uN A, uN B, uN C[2]);

/**
\brief  multiply, treat operands as integers, i.e. ignore exponents.
*/
template <typename T_Alloc>
auto BigDecimal<T_Alloc>::mul_integer (BigDecimal& B)-> void {

    HardAssert(this->is_normalized_integer());

    BigDecimal& A = *this;
    BigDecimal& carries = temp_mul_int_carries;
    BigDecimal& result = temp_mul_int_0;
    BigDecimal& row_result = temp_mul_int_1;

//    if (A.is_zero() || B.is_zero()) {
//        this->zero(); //TODO(ArokhSlade##2024 10 19): sign!
//        return;
//    }

	carries.zero();
	result.zero();

	ChunkList *chunk_A = &A.data;
	for (u32 IdxA = 0 ; IdxA < A.length ; ++IdxA, chunk_A = chunk_A->next) {
        HardAssert(chunk_A != nullptr);
        ChunkList *chunk_B = &B.data;

	    row_result.zero();
	    ChunkList *chunk_row_result = &row_result.data;
        u32 carriesActualLength = 1;
        carries.zero();
//        if (chunk_A->value != 0x0) {
            ChunkList *chunk_carries = &carries.data;
            while (carries.length <= IdxA) {
                carries.extend_length();
                chunk_carries = chunk_carries->next;
            }
            for (u32 IdxB = 0 ; IdxB < B.length ; ++IdxB, chunk_B = chunk_B->next) {
                HardAssert(chunk_B != nullptr);
                ChunkBits ChunkProd[2] = {};
                FullMulN<ChunkBits>(chunk_A->value, chunk_B->value, ChunkProd);
                while (row_result.length-1 < IdxA + IdxB) {
                    row_result.extend_length();
                }
                chunk_row_result = row_result.get_chunk(IdxA+IdxB);
                chunk_row_result->value = ChunkProd[0];

                carries.extend_length();
                chunk_carries = chunk_carries->next;
                HardAssert(chunk_carries != nullptr);
                chunk_carries->value = ChunkProd[1];
                carriesActualLength = ChunkProd[1] != 0 ? carries.length : carriesActualLength;
            }
            HardAssert(row_result.length >= IdxA + B.length);
            HardAssert(carries.length >= row_result.length);
            chunk_carries = &carries.data;
            for (u32 CheckIdx = 0; CheckIdx <= IdxA ; CheckIdx++) {
                HardAssert(chunk_carries != nullptr);
                chunk_carries->value == 0; //TODO(ArokhSlade##2024 09 23): no-op. intent? assert, i think. because there can't be carry values for the chunks below-or-equal IdxA
                chunk_carries = chunk_carries->next;
            }
            carries.length = carriesActualLength;

            row_result.UpdateLength();

            HardAssert(row_result.is_normalized_integer());
            HardAssert(carries.is_normalized_integer());
            HardAssert(result.is_normalized_integer());

            row_result.add_integer_signed(carries);
            result.add_integer_signed(row_result);
//        }
	}

	result.IsNegative = A.IsNegative != B.IsNegative;
	result.copy_to(&A);

	HardAssert(this->is_normalized_integer());

    return;
}


/**
 *  \brief computes most significant bit in terms of internal bit sequence.
 *  \return index of leading 1 (lsb 0 numbering) for non-zero values, 0 otherwise.
 *  \note   Big Decimal must have no leading zero chunks
 */
template <typename T_Alloc>
auto BigDecimal<T_Alloc>::get_msb() -> i32 {

    HardAssert(is_normalized_integer());

    if (is_zero()) return 0;
    typename BigDecimal<T_Alloc>::ChunkList *Cur = get_head();
    i32 Result = 0;
    Result = BitScanReverse<ChunkBits>(Cur->value) + (length-1) * CHUNK_WIDTH;
    return Result;

}


/** \brief  Get Bit at Index N (MSb 0 bit numbering),
 *  \n      i.e. 0-based, starting from top.
 *  \n      e.g. N=0 yields MSB.
 *  \note   N must be within bounds.
 *  \note   Big Decimal must have no leading zero chunks
 **/
template<typename T_Alloc>
auto BigDecimal<T_Alloc>::get_bit(i32 high_idx) -> bool {
    HardAssert(0 <= high_idx);
    HardAssert(is_normalized_integer());

    i32 low_idx = get_msb() - high_idx;

    HardAssert(low_idx >= 0);

    i32 chunk_idx = low_idx / CHUNK_WIDTH;
    i32 bit_idx = low_idx % CHUNK_WIDTH;

    ChunkList* chunk = get_chunk(chunk_idx);
    bool result = chunk->value & (ChunkBits(1) << bit_idx);
    return result;
}


template<typename T_Alloc>
auto BigDecimal<T_Alloc>::count_bits() -> i32 {
    return get_msb() + 1;
}

/**
\brief performs round-to-even with guard bit, round bit and sticky bits (any 1 after the round bit counts as sticky bit set)
 */
template<typename T_Alloc>
auto BigDecimal<T_Alloc>::round_to_n_significant_bits(i32 N) -> void {
    HardAssert(this->is_normalized_fractional());
    i32 BitCount = this->count_bits();
    if (BitCount <= N) return;

    i32 LSB = N==0 ? 0 : get_bit(N-1);
    i32 Guard = get_bit(N);
    // we know: this->is_normalized_fractional(), therefor LSB == 1
    // for explanation sake, assume bits are numbered front to back, i.e. LSB at [0]
    // we will round to N bits, i.e. indices [0..N-1]
    // bit[N] = Guard bit
    // bit[N+1] = Round bit
    // Sticky bit is set if any bit thereafter is set to true
    // thus, if we have N+3 or more bits, sticky bit is set.
    // Round OR Sticky bit is set, if we have N+2 or more bits.
    i32 RoundSticky = BitCount >  N + 1 ;

    shift_right(BitCount-N);

    //GRS:100 = midway, round if lsb is set, i.e. "uneven"
    //all other cases are determined:
    //GRS:0xx = round down
    //Guard bit set followed by a 1 anywhere later ("round or sticky") = round up
    bool RoundUp = Guard && (RoundSticky || LSB); //NOTE(Arokh##2024 07 14): round-to-even
    if (RoundUp) {
        add_integer_unsigned(temp_one);
    }

    i32 NewBitCount = this->count_bits();
    HardAssert(NewBitCount == N || NewBitCount == N+1);
    if (/* !this->is_zero()  &&*/ NewBitCount == N+1) { //TODO(##2024 07 13):count_bits returns 1 for zero. that's why we check for is_zero here. review!
        normalize();
        ++Exponent;
    }
    normalize();

    return;
}


template <typename T_Alloc>
auto div_integer(BigDecimal<T_Alloc>& A, BigDecimal<T_Alloc>& B, BigDecimal<T_Alloc>& ResultInteger, BigDecimal<T_Alloc>& ResultFraction, u32 MinFracPrecision=32) -> bool{

    HardAssert(A.is_normalized_integer());
    HardAssert(B.is_normalized_integer());

    bool was_div_by_zero = B.is_zero();
    if (ResultInteger.was_divided_by_zero = was_div_by_zero) {
        ResultInteger.zero();
        return was_div_by_zero;
    }

    using T_Big_Decimal = BigDecimal<T_Alloc>;
    ResultInteger.set(0);
    ResultFraction.set(0);

    int DigitCountB = B.count_bits();
    bool IntegerPartDone = false;
    bool NonZeroInteger = false;

    T_Big_Decimal& A_    = T_Big_Decimal::temp_div_int_a;
    T_Big_Decimal& B_    = T_Big_Decimal::temp_div_int_b;
    T_Big_Decimal& One_  = T_Big_Decimal::temp_one;
    T_Big_Decimal& Temp_ = T_Big_Decimal::temp_div_int_0;
    A.copy_to(&A_);
    B.copy_to(&B_);
    A_.IsNegative = B_.IsNegative = false;


    //Compute Integer Part

    if ( A_.greater_equals_integer(B_) ) {
        NonZeroInteger = true;
        for ( ; !A_.is_zero() && !IntegerPartDone ; ) {
            int DigitCountA = A_.count_bits();
            int Diff = DigitCountA - DigitCountB;

            if (Diff > 0) {
                B_.shift_left(Diff);
                if (A_.less_than_integer_signed(B_)){
                    B_.shift_right(1);
                    Diff -= 1;
                }
                Temp_.set(1);
                Temp_.shift_left(Diff);
                ResultInteger.add_integer_signed(Temp_);
                A_.sub_integer_signed(B_);
                B.copy_to(&B_, T_Big_Decimal::COPY_DIGITS | T_Big_Decimal::COPY_EXPONENT); //Don't copy Sign
            }
            else if (Diff == 0) {
                if ( A_.greater_equals_integer(B_) ) {
                    A_.sub_integer_signed(B_);
                    ResultInteger.add_integer_signed(One_);
                } else {
                    IntegerPartDone = true;
                }
            } else {
                IntegerPartDone = true; //A.#Digits < B.#NumDigits => A/B = 0.something
            }
        }
        if (A_.is_zero()) {
            return was_div_by_zero;
        }
    }
    else { IntegerPartDone = true; }

    ResultInteger.Exponent = ResultInteger.count_bits() - 1;

    i32 DigitCountA = A_.count_bits();
    i32 RevDiff = DigitCountB-DigitCountA;
    A_.shift_left(RevDiff);
    if (A_.less_than_integer_signed(B_)) {
        A_.shift_left(1);
        RevDiff += 1;
    }

    ResultFraction.set(0x1, false, -RevDiff);
    i32 ExplicitSignificantFractionBits = 1 ;
    A_.sub_integer_signed(B_);

    //Compute Fraction Part
    for ( ; ExplicitSignificantFractionBits < MinFracPrecision && !A_.is_zero() ; ) {
        int DigitCountA = A_.count_bits();
        int RevDiff = DigitCountB-DigitCountA;
        HardAssert(RevDiff >= 0);
        if (RevDiff == 0) {
            HardAssert(A_.less_than_integer_signed(B_));

            A_.shift_left(1);
            ExplicitSignificantFractionBits += 1;
            ResultFraction.shift_left(1);
            ResultFraction.add_integer_signed(One_);
            A_.sub_integer_signed(B_);
        } else if (RevDiff > 0) {
            A_.shift_left(RevDiff);
            if (A_.less_than_integer_signed(B_)) {
                A_.shift_left(1);
                RevDiff += 1;
            }
            ExplicitSignificantFractionBits += RevDiff;
            ResultFraction.shift_left(RevDiff);
            ResultFraction.add_integer_signed(One_);
            A_.sub_integer_signed(B_);
        }
    }

    ResultInteger.IsNegative = A.IsNegative && !B.IsNegative || !A.IsNegative && B.IsNegative;

    return was_div_by_zero;
}

/**
\brief  integer division, treat operands as integers, i.e. ignore their exponents.
 */
template <typename T_Alloc>
auto BigDecimal<T_Alloc>::div_integer (BigDecimal& B, u32 MinFracPrecision) -> void {

    HardAssert(this->is_normalized_integer());

    ::div_integer(*this, B, temp_div_frac_int_part, temp_div_frac_frac_part, MinFracPrecision);

    HardAssert(temp_div_frac_frac_part.is_zero() || temp_div_frac_frac_part.Exponent < 0 );
    HardAssert(temp_div_frac_int_part.is_zero() || temp_div_frac_int_part.Exponent >=0);

    temp_div_frac_int_part.copy_to(this, BigDecimal::COPY_DIGITS | BigDecimal::COPY_EXPONENT);

    this->IsNegative = false;

    HardAssert(!temp_div_frac_frac_part.IsNegative);
    this->add_fractional(temp_div_frac_frac_part);

    this->IsNegative = temp_div_frac_int_part.IsNegative;

    HardAssert(this->is_normalized_integer());

    return;
}

template <typename T_Alloc>
auto BigDecimal<T_Alloc>::get_least_significant_exponent() -> i32 {
    HardAssert(is_normalized_fractional());
    return (Exponent-this->count_bits())+1;
}


/**
\brief addition algorithm that treats operands as fractionals.
 */
template <typename T_Alloc>
auto BigDecimal<T_Alloc>::add_fractional (BigDecimal& B) -> void {
    BigDecimal& A = *this;
    HardAssert(A.is_normalized_fractional());
    HardAssert(B.is_normalized_fractional());
    int A_LSE = A.get_least_significant_exponent();
    int B_LSE = B.get_least_significant_exponent();
    int Diff = A_LSE - B_LSE;
    BigDecimal& B_ = temp_add_fractional;
    B.copy_to(&B_);
    if (Diff > 0 ) {
        A.shift_left(Diff);
    }
    if (Diff < 0) {
        B_.shift_left(-Diff);
    }
    int OldMSB = A.get_msb();
    bool WasZero = A.is_zero();
    A.add_integer_signed(B_);
    int NewMSB = A.get_msb();
    A.Exponent = WasZero ? B_.Exponent : A.Exponent+(NewMSB-OldMSB);
    normalize();

    return;
}


/**
\brief subtraction algorithm that treats operands as fractionals.
 */
template <typename T_Alloc>
auto BigDecimal<T_Alloc>::sub_fractional (BigDecimal& B)-> void {
    HardAssert(is_normalized_fractional());
    BigDecimal& A = *this;
    int A_LSE = A.get_least_significant_exponent();
    int B_LSE = B.get_least_significant_exponent();
    int Diff = A_LSE - B_LSE;
    BigDecimal& B_ = BigDecimal<T_Alloc>::temp_sub_frac;
    B.copy_to(&B_);
    if (Diff > 0 ) {
        A.shift_left(Diff);
    }
    if (Diff < 0) {
        B_.shift_left(-Diff);
    }
    int OldMSB = get_msb();
    A.sub_integer_signed(B_);
    int NewMSB = get_msb();
    A.Exponent += (NewMSB-OldMSB);
    normalize();

    return;
}


/**
\brief multiply algorithm that treats operands as fractionals.
 */
template <typename T_Alloc>
auto BigDecimal<T_Alloc>::mul_fractional(BigDecimal& B) -> void {
    HardAssert(is_normalized_fractional());
    HardAssert(B.is_normalized_fractional());
    BigDecimal& A = *this;
    int Exponent_ = A.Exponent + B.Exponent - A.get_msb() - B.get_msb();
    A.mul_integer(B);
    normalize();
	A.Exponent = Exponent_ + A.get_msb();

    return;
}


/**
\brief division algorithm that treats operands as fractionals.
 */
template <typename T_Alloc>
auto BigDecimal<T_Alloc>::div_fractional (BigDecimal& B, u32 MinFracPrecision) -> void {

    BigDecimal& A = *this;
    HardAssert(A.is_normalized_fractional());
    HardAssert(B.is_normalized_fractional());

    if (B.is_zero()) {
        A.was_divided_by_zero = true;
        return;
    }

    int A_LSE = A.get_least_significant_exponent();
    int B_LSE = B.get_least_significant_exponent();
    int Diff = A_LSE - B_LSE;

    BigDecimal& B_ = BigDecimal<T_Alloc>::temp_div_frac;
    B.copy_to(&B_);
    if (Diff > 0 ) {
        A.shift_left(Diff);
    }
    if (Diff < 0) {
        B_.shift_left(-Diff);
    }
    A.div_integer( B_, MinFracPrecision );
    normalize();

    return;
}


template <typename T_Alloc>
auto BigDecimal<T_Alloc>::neg () -> void {
    this->IsNegative = !this->IsNegative;
}


/** \brief zeros the data and contracts length to 1 chunk.
 *         leaves sign and exponent unchanged by default,
 *         but this can be controlled with flags
 *  \arg   what : leave empty for default (ZERO_DIGITS_ONLY), or add ZERO_SIGN | ZERO_EXPONENT as needed
 *  \note  why this function exists:
 *         1: it's presumeably cheaper than set(0) (used often in hot loops of multiply alrogithm for example)
 *         2: easier say A.zero() than A.set(0,A.IsNegative,A.Exponent)
 *            and more explicit and clear to say A.zero(ZERO_EVERYTHING) than A.set(0)*/
template <typename T_Alloc>
auto BigDecimal<T_Alloc>::zero(flags32 what) -> void {
        data.value = 0x0;
        length = 1;
    if (IsSet(what, ZERO_SIGN))
        IsNegative = false;
    if (IsSet(what, ZERO_EXPONENT))
        Exponent = 0;
}

template <typename T_Alloc>
auto BigDecimal<T_Alloc>::set_bits_64(u64 value) -> void {
    i32 n_src_bits = BitScanReverse<u64>(value) + 1;
    if (n_src_bits == 0) n_src_bits = 1;
    i32 n_src_bytes = DivCeil(n_src_bits, 8);
    i32 n_chunk_bytes = sizeof(ChunkBits);

    length = 1;
    ChunkList *chunk = &data;

    i32 n_grab_bytes = n_src_bytes <= n_chunk_bytes ? n_src_bytes : n_chunk_bytes;
    i32 n_grab_bits = n_grab_bytes * 8;
    i32 n_bytes_remaining = n_src_bytes - n_grab_bytes;

    i32 offset = 0;
    u64 mask = GetMaskBottomN<u64>(n_grab_bits);
    chunk->value = value & mask;

    for ( ; n_bytes_remaining > 0 ; n_bytes_remaining -= n_grab_bytes ) {
        offset += n_grab_bits;
        if (n_bytes_remaining < n_grab_bytes) {
            n_grab_bytes = n_bytes_remaining;
            i32 n_grab_bits = n_grab_bytes * 8;
        }
        mask = GetMaskBottomN<u64>(n_grab_bits);
        mask <<= offset;
        chunk = extend_length();
        HardAssert(offset < 64 && offset >=0);
        chunk->value = (value & mask) >> offset;
    }

    HardAssert(this->is_normalized_integer());
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
auto BigDecimal<T_Alloc>::set_bits_array(T_Slot *vals, u32 count) -> void {

    using T_Chunk = ChunkBits;

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

    length = 1;
    while (length < chunks_total) {
        extend_length();
    }

    ChunkList *chunk = &data;
    for (i32 i = 0; i < length ; ++i) {
        chunk->value = 0;
        chunk = chunk->next;
    }

    chunk = &data;
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
                chunk->value = chunk_val;

                mask <<= chunk_bits;
                offset += chunk_bits;
                chunk = chunk->next;
            }
            ++slot;
        }
        offset = 0;
        mask = base_mask;
        i32 top_chunks = DivCeil(top_bytes, chunk_bytes);
        for(i32 chunk_idx = 0 ; chunk_idx < top_chunks ; ++chunk_idx) {
            T_Slot chunk_val = *slot & mask;
            chunk_val >>= offset;
            chunk->value = chunk_val;

            mask <<= chunk_bits;
            offset += chunk_bits;
            chunk = chunk->next;
        }
    } else {
        i32 slot_bits = slot_bytes * 8;
        T_Chunk slot_val = 0x0;
        T_Chunk chunk_val = 0x0;
        i32 offset = 0;
        for (i32 chunk_idx = 0 ; chunk_idx < length-1 ; ++chunk_idx) {
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
            chunk->value = chunk_val;
            chunk = chunk->next;
        }
        i32 slots_left = slots_total - slots_per_chunk * (length-1);
        chunk_val = 0x0;
        offset = 0;
        for (i32 slot_idx = 0 ; slot_idx < slots_left ; ++slot_idx) {
            slot_val = *slot;
            slot_val <<= offset;
            chunk_val |= slot_val;

            offset += slot_bits;
            ++slot;
        }
        chunk->value = chunk_val;
        chunk = chunk->next;
    }

    HardAssert(this->is_normalized_integer());
};



/**
 *  \note   does not automatically normalize. e.g. you can set value to 2 and exponent to 0, which wouldn't be a valid decimal. some functions want this to represent / interpret as integers.
**/
template <typename T_Alloc>
template <std::integral T_Src>
auto BigDecimal<T_Alloc>::set(T_Src Value, bool IsNegative, i32 Exponent)  -> BigDecimal<T_Alloc>& {
    this->set_bits_64(Value);
    this->IsNegative = IsNegative;
    this->Exponent = Exponent;

    HardAssert(is_normalized_integer());
    return *this;
}


/**
 *  \note   does not automatically normalize. e.g. you can set value to 2 and exponent to 0, which wouldn't be a valid decimal. some functions want this to represent / interpret as integers.
**/
template <typename T_Alloc>
template <std::integral T_Slot>
auto BigDecimal<T_Alloc>::set(T_Slot *ValArray, u32 ValCount, bool IsNegative/* =false */, i32 Exponent /* =0 */) -> BigDecimal& {
    set_bits_array(ValArray, ValCount);
    this->IsNegative = IsNegative;
    this->Exponent = Exponent;

    HardAssert(is_normalized_integer());

    return *this;
}


//TODO(##2024 07 04):Support NAN (e=128, mantissa != 0)
template <typename T_Alloc>
auto BigDecimal<T_Alloc>::set_float(real32 Val) -> void {
    this->zero();
    this->IsNegative = Sign(Val);
    this->Exponent = Exponent32(Val);
    bool Denormalized = IsDenormalized(Val);
    u32 Mantissa = Mantissa32(Val, !Denormalized);
    i32 mantissa_bitcount = BitScanReverse32(Mantissa) + 1;
    if (Denormalized && Mantissa != 0) {
        Exponent -= 22-BitScanReverse32(Mantissa);
    }

    u32 chunk_width = CHUNK_WIDTH;
    set_bits_64(Mantissa);
    this->normalize(); //NOTE(ArokhSlade##2024 10 18): truncate trailing zeros
}

//TODO(##2024 07 04):Support NAN (e=128, mantissa != 0)
template <typename T_Alloc>
auto BigDecimal<T_Alloc>::set_double(f64 Val) -> void {

    this->zero();
    this->IsNegative = Sign(Val);
    this->Exponent = Exponent64(Val);
    bool Denormalized = IsDenormalized(Val);
    u64 Mantissa = Mantissa64(Val, !Denormalized);
    i32 mantissa_bitcount = BitScanReverse<u64>(Mantissa) + 1;
    if (Denormalized && Mantissa != 0) {
        Exponent -= DOUBLE_PRECISION-2-BitScanReverse<u64>(Mantissa);
    }
    set_bits_64(Mantissa);
    this->normalize();
}

template <typename T_Alloc>
auto BigDecimal<T_Alloc>::initialize_context (const T_Alloc& ctx_alloc) -> void {
    using BigDec = BigDecimal<T_Alloc>;

    HardAssert(!s_is_context_initialized);
    BigDec::s_ctx_alloc = ctx_alloc;
    BigDec::s_chunk_alloc = ChunkAlloc{ctx_alloc};
    BigDec::s_link_alloc = LinkAlloc{BigDec::s_chunk_alloc};

    for (i32 i = 0 ;  i < TEMPORARIES_COUNT ; ++i) {
//        new(s_all_temporaries_ptrs[i]) BigDec{SpecialConstants::BELONGS_TO_CONTEXT, s_chunk_alloc};
        s_all_temporaries_ptrs[i]->initialize(s_chunk_alloc);
    }

    temp_one.set(1);
    temp_ten.set(10);
    temp_pow_10.set(1);
    temp_parse_frac.set(1);
    BigDecimal<T_Alloc>::s_is_context_initialized = true;
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
auto BigDecimal<T_Alloc>::copy_bits_to(T_Slot *slots, i32 slot_count) -> void {

    i32 bit_count = count_bits();
    i32 bits_per_slot = sizeof(T_Slot) * 8;
    HardAssert(bit_count <= bits_per_slot * slot_count);
    i32 bits_per_chunk = CHUNK_WIDTH;
    i32 chunks_per_slot = bits_per_slot / bits_per_chunk;
    i32 slots_per_chunk = bits_per_chunk / bits_per_slot;

    ChunkList *chunk = &data;
    T_Slot *slot = slots;
    if (chunks_per_slot) {
        i32 chunk_idx = 0;
        i32 offset = 0;
        for (  ; chunk_idx  + chunks_per_slot <= length ; chunk_idx  += chunks_per_slot ) {
            for ( i32 slot_idx = 0 ; slot_idx < chunks_per_slot ; ++slot_idx ) {
                *slot |= (T_Slot)chunk->value << offset;
                offset += bits_per_chunk;
                chunk = chunk->next;
            }
            offset = 0;
            ++slot;
        }
        //handle remainder
        for ( ; chunk_idx < length ; ++chunk_idx ) {
            *slot |= (T_Slot)chunk->value << offset;
            offset += bits_per_chunk;
            chunk = chunk->next;

        }
    } else { //chunks are bigger than slots
        i32 chunk_idx = 0 ;
        ChunkBits chunk_mask = 0x0;
        i32 total_slot_idx = 0;
        for ( ; chunk_idx < length ; ++chunk_idx ) {
            chunk_mask = GetMaskBottomN<ChunkBits>(bits_per_chunk);
            HardAssert(total_slot_idx < slot_count);
            i32 offset = 0;
            for ( i32 slot_idx = 0 ; slot_idx < slots_per_chunk ; ++slot_idx) {
                ChunkBits piece = chunk->value & chunk_mask;
                piece >>= offset; //NOTE(ArokhSlade ## 2024 10 06): offset < bit width b/c chunks_per_slot==0
                *slot = piece;
                chunk_mask <<= bits_per_slot;
                ++slot;
                if (++total_slot_idx == slot_count) {
                    break;
                }
                offset += bits_per_slot;
            }
            chunk = chunk->next;
        }
    }

    return;
}

//TODO(##2024 07 04): are there faster ways to compute the mantissa than calling Round()?
template <typename T_Alloc>
auto BigDecimal<T_Alloc>::to_float() -> f32 {

    if (this->is_zero()) {
        return 0.f;
    }

    this->copy_to(&BigDecimal::temp_to_float);
    bool Sign = temp_to_float.IsNegative;

    int MantissaBitCount = 24;

    i32 ResultExponent = temp_to_float.Exponent;

    if (ResultExponent < -150) {
        return Sign ? -0.f : 0.f;
    }

    if (ResultExponent < -126) {
        MantissaBitCount = 150+ResultExponent;
    }

    temp_to_float.round_to_n_significant_bits(MantissaBitCount); //Exponent may change

    ResultExponent = temp_to_float.Exponent;

    if (temp_to_float.is_zero()) {
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




    i32 chunks_required = (FLOAT_PRECISION + CHUNK_WIDTH - 1) / CHUNK_WIDTH;
    HardAssert(temp_to_float.length <= chunks_required);

    i32 BitCount = temp_to_float.count_bits(); //NOTE(ArokhSlade##2024 09 25): after rounding, value is normalized, i.e. trailing zeroes are chopped off.
    HardAssert (MantissaBitCount >= BitCount);
    i32 ShiftAmount = MantissaBitCount - BitCount; //NOTE(ArokhSlade##2024 09 25): reintroduce trailing zeroes

    u32 Mantissa = 0;
    temp_to_float.shift_left(ShiftAmount);
    temp_to_float.copy_bits_to(&Mantissa, 1);
    Mantissa &= MantissaMask;

    // IEEE-754 float32 format
    //sign|exponent|        mantissa
    //   _|________|__________,__________,___
    //   1|<---8-->|<---------23------------>

    u32 Result = Sign << 31;
    u32 Bias = 127;
    ResultExponent = Denormalized || this->is_zero() ? 0 : ResultExponent + Bias;
    Result |= ResultExponent << 23;
    Result |= Mantissa;

    return *reinterpret_cast<float *>(&Result);
}


//TODO(##2024 10 18): (copied from to_float) are there faster ways to compute the mantissa than calling Round()?
template <typename T_Alloc>
auto BigDecimal<T_Alloc>::to_double() -> f64 {

    if (this->is_zero()) {
        return 0.f;
    }

    this->copy_to(&BigDecimal::temp_to_float);
    bool Sign = temp_to_float.IsNegative;

    int MantissaBitCount = DOUBLE_PRECISION;

    i32 ResultExponent = temp_to_float.Exponent;

    if (ResultExponent < MinExponent64) {
        return Sign ? -0.f : 0.f;
    }

    if (ResultExponent < -(FloatBias64-1)) {
        MantissaBitCount = (-MinExponent64)+ResultExponent;
    }

    temp_to_float.round_to_n_significant_bits(MantissaBitCount); //Exponent may change

    ResultExponent = temp_to_float.Exponent;

    if (ResultExponent < (MinExponent64+1) /*|| temp_to_float.is_zero()*/) { //TODO(##2024 07 13):check is_zero?
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




    i32 chunks_required = (DOUBLE_PRECISION + CHUNK_WIDTH - 1) / CHUNK_WIDTH;
    HardAssert(temp_to_float.length <= chunks_required);

    i32 BitCount = temp_to_float.count_bits(); //NOTE(ArokhSlade##2024 09 25): after rounding, value is normalized, i.e. trailing zeroes are chopped off.
    HardAssert (MantissaBitCount >= BitCount);
    i32 ShiftAmount = MantissaBitCount - BitCount; //NOTE(ArokhSlade##2024 09 25): reintroduce trailing zeroes

    u64 Mantissa = 0;

    temp_to_float.shift_left(ShiftAmount);
    temp_to_float.copy_bits_to(&Mantissa, 1);
    Mantissa &= MantissaMask;

    // IEEE-754 float64 format
    //sign|  exponent  |                      mantissa
    //   _|_______,____|____,________,________,________,________,________,________
    //   1|<----11---->|<------------------------52------------------------------>

    u64 ResultBits = Sign << 63;
    u64 Bias = FloatBias64;
    ResultExponent = Denormalized || this->is_zero() ? 0 : ResultExponent + Bias;
    ResultBits = ResultExponent;
    ResultBits <<= DOUBLE_PRECISION-1;
    ResultBits |= Mantissa;
    f64 Result = *reinterpret_cast<f64 *>(&ResultBits);
    return Result;
}

typedef BigDecimal<std::allocator<ChunkBits>> Big_Dec_Std;




template <typename T_Alloc>
BigDecimal<T_Alloc>::operator std::string(){
    std::string Result;

    Result += IsNegative ? '-' : '+';
    ChunkList *Cur = get_head();
    i32 UnusedChunks = m_chunks_capacity - length;
    constexpr i32 width = sizeof(ChunkBits) * 2;
    char HexDigits[width+1] = "";

//    i32 UnusedChunksFieldSize = 2 + ( UnusedChunks > 1 ? UnusedChunks : 1);
//    i32 BufSize = 1+length*9+2+UnusedChunksFieldSize+1;

    for (i32 Idx = 0 ; Idx < length; ++Idx, Cur = Cur->prev) {
        stbsp_sprintf(HexDigits, "%.*llx ", width, Cur->value);
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

//template BigDecimal<Arena_Allocator<ChunkBits>>::operator std::string(); //explicit instantiation so it's available in debugger


template <typename T_Alloc>
std::ostream& operator<<(std::ostream& Out, BigDecimal<T_Alloc>& big_decimal) {
    return Out << std::string(big_decimal);
}

template <typename T_ChunkBitsAlloc, typename T_CharAlloc>
auto to_chars(BigDecimal<T_ChunkBitsAlloc>& A, T_CharAlloc& string_alloc) -> char* {
    char Sign = A.IsNegative ? '-' : '+';
    typename BigDecimal<T_ChunkBitsAlloc>::ChunkList *CurChunk = A.get_head();
    i32 UnusedChunks = A.m_chunks_capacity - A.length;
    char HexDigits[11] = "";

    i32 NumSize = UnusedChunks > 0 ? Log10I(UnusedChunks ) : 1;
    i32 UnusedChunksFieldSize = 2 + NumSize + 1;

    i32 BufSize = 1+A.length*9+2+UnusedChunksFieldSize+1;
    char *Buf = std::allocator_traits<T_CharAlloc>::allocate(string_alloc, BufSize);

    char *BufPos=Buf;
    *BufPos++ = Sign;

    for (i32 Idx = 0 ; Idx < A.length; ++Idx, CurChunk = CurChunk->prev) {
        stbsp_sprintf(BufPos, "%.8x ", CurChunk->value);
        BufPos +=9;
    }

    stbsp_sprintf(BufPos, "[%.*d]", NumSize, UnusedChunks);
    BufPos += UnusedChunksFieldSize;

    *BufPos = '\0';

    return Buf;
}


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

#endif //G_BIG_DECIMAL_UTILITY_H
