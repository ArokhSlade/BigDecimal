#include "G_Essentials.h"
#include "G_Miscellany_Utility.h"
#include "G_MemoryManagement_Service.h"
#include "G_BigDecimal_Utility.h"
#include "G_Math_Utility.h" //FLOAT_PRECISION

#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdio.h>


#include "Memoryapi.h" //VirtuaAlloc()
#undef OPTION
#undef BitScanReverse //NOTE(ArokhSlade##2024 10 13): avoid the windows library macro that renames our function calls to _BitScanReverse
inline FUN_DELETER(virtual_free_decommit) {
    VirtualFree(memory,size,MEM_DECOMMIT);
}

#define ACTIVATE_ALL_TESTS 1
//TODO(ArokhSlade##2024 08 13): this is copy-pasta from UnitTest_G_PlatformGame_2_Module.cpp. extract!
typedef u32 success_bits;
struct test_result {
    //TODO(##2024 05 04): this limit is arbitrary. if you run out of tests cases, increase it.
    static const u32 MaxTestWords = 8;
    u32 TestCount;
    i32 FailCount;
    success_bits SuccessBits[MaxTestWords];

    //TODO(## 2023 11 19): maybe "CheckAndAppend" ?
    void Append(bool Condition) {
        if (Condition) {
            SetBit32(SuccessBits, MaxTestWords, TestCount);
        }
        ++TestCount;
        FailCount += !Condition;
    }

    void PrintResults() {
        for (u32 TestIdx = 0 ; TestIdx < TestCount ; ++TestIdx ) {
            bool PassedCurrent = GetBit32(SuccessBits, MaxTestWords, TestIdx);
            std::cout << "#" << TestIdx << "\t: " << (PassedCurrent ? "OK" : "ERROR") << '\n';
        }
    }
};


//TODO(ArokhSlade##2024 08 13): this is copy-pasta from UnitTest_G_PlatformGame_2_Module.cpp. extract!
std::ostream& operator<<(std::ostream& Out, test_result& Tests) {

    Out << "Tests Passed: " << (Tests.TestCount-Tests.FailCount) << "/" << Tests.TestCount;
    return Out;
}

//TODO(ArokhSlade##2024 08 13): this is copy-pasta from UnitTest_G_PlatformGame_2_Module.cpp. extract!
//TODO(ArokhSlade##2024 08 26): look into maybe using the BigDecimal's own allocator for this?
template <typename T_Alloc>
auto Print(BigDecimal<T_Alloc>& A, memory_arena *StringArena) -> void {
    memory_arena *TempArena = PushArena(StringArena);
    std::cout << Str(A, TempArena) << "\n";
    PopArena(TempArena);
}


//TODO(ArokhSlade##2024 08 13): this is copy-pasta from UnitTest_G_PlatformGame_2_Module.cpp. extract!
//TODO(ArokhSlade##2024 08 18): implement only_errors for all tests -> ideally make it a unit test facility
i32 Test_big_decimal(bool only_errors = false) {
    using std::cout;
    using std::string;
    i32 FailCount { 0 };

    cout << __func__ << '\n';

    test_result Tests {};

    memory_index ArenaSize = 256*32768*sizeof(u32);
    uint8 *ArenaBase = (uint8*)VirtualAlloc(0, ArenaSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    memory_index StringArenaSize = 4096 * sizeof(char);
    uint8 *StringArenaBase = (uint8*)VirtualAlloc(0, StringArenaSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    memory_arena StringArena;
    InitializeArena(&StringArena, StringArenaSize, StringArenaBase);

    using ArenaChunkAlloc = ArenaAlloc<ChunkBits>;
    using BigDec_Arena = BigDecimal<ArenaChunkAlloc>;
    ArenaChunkAlloc virtual_alloc_arena_alloc_1 { ArenaSize, ArenaBase, virtual_free_decommit};

    BigDec_Arena::initialize_context ( virtual_alloc_arena_alloc_1);

    BigDec_Arena A { virtual_alloc_arena_alloc_1, 9};
    BigDec_Arena OldA { virtual_alloc_arena_alloc_1, A};
    BigDec_Arena B { virtual_alloc_arena_alloc_1, 10};

    BigDec_Arena Expected ( virtual_alloc_arena_alloc_1, 0);

    auto CheckA = [&](){
        Tests.Append(A.equals_integer(Expected)); };

    auto PrintTestOutput = [&Tests](BigDec_Arena& Before, BigDec_Arena& After, BigDec_Arena& Wanted,
                                    char *BeforeName, char *AfterName, char*WantedName, char *TestName){
        cout << "Test #" << Tests.TestCount;
        if (TestName)   cout << " : " << TestName;
                        cout << "\n";
        if (BeforeName) cout << BeforeName << " = " << string(Before) << "\n" ;
        if (AfterName)  cout << AfterName << " = " << string(After) << "\n" ;
        if (WantedName) cout << WantedName << " = " << string(Wanted) << "\n" ;
    };
    char AName[] = "       A";
    char OldAName[] = "   Old A";
    char ExpectedName[] = "Expected";
    auto PrintTestA = [&](char *TestName){PrintTestOutput(OldA, A, Expected, OldAName, AName, ExpectedName, TestName);};

    std::cout << std::boolalpha;

#if 1
    Tests.Append(A.length >= 1 && A.get_chunk(0)->value == i32{9});
    Print(A,&StringArena);

    Tests.Append(B.length >= 1 && B.get_chunk(0)->value == i32{10});
    Print(B,&StringArena);

    B.add_integer_signed(A);
    Tests.Append(B.length == 1 && B.get_chunk(0)->value == 19U);
    Print(B,&StringArena);

    A.neg();
    Tests.Append(A.length == 1 && A.get_chunk(0)->value == 9U && A.is_negative);
    Print(A,&StringArena);

    {
        A.set(0x1, false, 0);
        B.set(0x2, true, 1);
        Print(A, &StringArena);
        Print(B, &StringArena);
        bool is_abs_a_less_than_abs_b = A.less_than_integer_unsigned(B);
        bool is_abs_b_less_than_abs_a = B.less_than_integer_unsigned(A);
        Tests.Append(true == is_abs_a_less_than_abs_b);
        Tests.Append(false == is_abs_b_less_than_abs_a);
        std::cout << "Abs( " << string(A) << " ) < Abs( " << string(B) << " ) == " << is_abs_a_less_than_abs_b << "\n";
        std::cout << "Abs( " << string(B) << " ) < Abs( " << string(A) << " ) == " << is_abs_b_less_than_abs_a << "\n";
    }

    {
        B.neg();
        Print(B,&StringArena);
        bool is_a_less_than_b = A.less_than_integer_signed(B);
        bool is_b_less_than_a = B.less_than_integer_signed(A);
        Tests.Append(true == is_a_less_than_b);
        Tests.Append(false == is_b_less_than_a);
        std::cout << string(A) << " < " << string(B) << " == " << is_a_less_than_b << "\n";
        std::cout << string(B) << " < " << string(A) << " == " << is_b_less_than_a << "\n";
    }
    {

        A.set(0x1, true, 0);
        B.set(0x2, true, 0);
        // -1 < -2 == false?
        bool is_a_less_than_b = A.less_than_integer_signed(B);
        bool is_b_less_than_a = B.less_than_integer_signed(A);
        Tests.Append(false == is_a_less_than_b);
        Tests.Append(true == is_b_less_than_a);
        std::cout << string(A) << " < " << string(B) << " == " << is_a_less_than_b << "\n";
        std::cout << string(B) << " < " << string(A) << " == " << is_b_less_than_a << "\n";
    }

    {

        A.set(0x2, true, 0);
        B.set(0x1, true, 0);
        // -2 >= -1 == false?
        bool is_a_greater_equal_b = A.greater_equals_integer(B);
        bool is_b_greater_equal_a = B.greater_equals_integer(A);
        std::cout << "Tests #" << Tests.TestCount << " and #" << (Tests.TestCount+1) << " - Greaterequal\n";
        Tests.Append(!is_a_greater_equal_b);
        Tests.Append(is_b_greater_equal_a);
        std::cout << string(A) << " < " << string(B) << " == " << is_a_greater_equal_b << "\n";
        std::cout << string(B) << " < " << string(A) << " == " << is_b_greater_equal_a << "\n";
    }

    //trying to get non-existent chunks will return a common fallback
    Tests.Append(B.get_chunk(1) == B.get_chunk(12345) && B.get_chunk(1) == nullptr);
    Print(B,&StringArena);



    //add_integer_signed(+,+) 1,1 -> 2 blocks
    A.set(MAX_UINT32, false, 0);
    B.set(0x1, false, 0) ;
    A.add_integer_signed(B);
    {
        // 0xFFFFFFFF + 1 = 0x1'0000'0000
        u32 ExpectedValues[] = {0,1};
        Expected.set(ExpectedValues, ArrayCount(ExpectedValues));
        CheckA();
    }


    //add_integer_signed(+,+) 2,1 -> 2 blocks
    A.add_integer_signed(B);
    {
        // 0x1'0000'0000 + 1 = 0x1'0000'0001
        u32 ExpectedValues[] = {1,1};
        Expected.set(ExpectedValues, ArrayCount(ExpectedValues));
        CheckA();
    }

    //add_integer_signed(+,+) 1,2 -> 2 blocks
    B.add_integer_signed(A);
    {
        // 1 + 0x1'0000'0001 = 0x1'0000'0002
        u32 EVals[] = {2,1};
        Expected.set(EVals,ArrayCount(EVals));
        Tests.Append(B.equals_integer(Expected));
    }


    {
        //add_integer_signed(+,+) 1,1 -> 2 blocks
        u32 AVals[] = {0xFFFFFFFE};
        A.set(AVals, 1);
        B.set(0x4);
        u32 EVals[] = {2,1};
        Expected.set(EVals,ArrayCount(EVals));
        A.add_integer_signed(B);
        CheckA();
    }

    //add_integer_signed(+,-)
    A.set(0);
    B.set(1);
    B.neg();
    A.add_integer_signed(B);
    {
        // 0 + -1 = -1
        u32 EVals[] = {1};
        Expected.set(EVals,ArrayCount(EVals),true);
        Tests.Append(A.equals_integer(Expected));
        Tests.Append(B.equals_integer(Expected));
    }



    //add_integer_signed(-,+)
    A.set(1,true);
    B.set(2);
    A.add_integer_signed(B);
    Expected.set(1);
    Tests.Append(A.equals_integer( Expected));

    //add_integer_signed(-,-)
    A.set(2,true);
    B.set(2,true);
    A.add_integer_signed(B);
    Expected.set(4,true);
    Tests.Append(A.equals_integer(Expected));


    {
    //add_integer_signed(-,-) length 1->2
        u32 Vals[] = {0xFFFFFFFF};
        A.set(Vals, 1);

        A.set(1,true);
        B.set(Vals,1);
        B.neg();
        A.add_integer_signed(B);
        u32 EVals[] = {0,1};
        Expected.set(EVals, ArrayCount(EVals), true);
//        Expected.neg();
        CheckA();
    }


    //sub_integer_signed(+,+) >0 -> >0
    A.set(123,0,Log2I(123));
    B.set(23,0,Log2I(23));
    A.sub_integer_signed(B);
    {
        // 123-23 = 100
        u32 ExpectedValues[] = {100};
        Expected.set(ExpectedValues, ArrayCount(ExpectedValues));
        CheckA();
    }

    //sub_integer_signed(+,+) >0 to <0
    A.set(0);
    B.set(1);
    A.sub_integer_signed(B);
    {
        // 0 - 1 = -1
        u32 EVals[] = {1};
        Expected.set(EVals, ArrayCount(EVals), true);
        CheckA();
    }

    {
        //sub_integer_signed(+,-)
        cout << "sub_integer_signed(+,-), Test# " << Tests.TestCount << "...\n";
        A.set(1);
        B.set(1,true);
        Expected.set(2);
        A.sub_integer_signed(B);
        CheckA();
    }

    {
        //Sub (x,x) == 0 for long chains
        cout << "Test# " << Tests.TestCount << " : A.sub_integer_signed(B) == 0 (where A==B) for long chains ...\n";
        u32 ValuesA[] { 0x0000'0000, 0x0000'0000, 0x0000'0000, 0x8000'0000 };
        A.set(ValuesA,ArrayCount(ValuesA));
        B.set(ValuesA,ArrayCount(ValuesA));
        A.sub_integer_signed(B);
        Expected.set(0);
        CheckA();

        cout << "Test# " << Tests.TestCount << " : A.sub_integer_signed(A) == 0 for long chains ...\n";
        A.set(ValuesA,ArrayCount(ValuesA));
        A.sub_integer_signed(A);
        Tests.Append(A.is_zero());

        cout << "Test# " << Tests.TestCount << " : Sub for long chains changes the length and uses carries ...\n";
        A.set(ValuesA,ArrayCount(ValuesA));
        u32 ValuesB[] { 0x0000'0000, 0x8000'0000, 0xFFFF'FFFF, 0x7FFF'FFFF };
        B.set(ValuesB, ArrayCount(ValuesB));
        A.sub_integer_signed(B);
        u32 ValuesExpected[] { 0x0000'0000, 0x8000'0000 };
        Expected.set(ValuesExpected,ArrayCount(ValuesExpected));
        CheckA();
    }

    //sub_integer_signed(-,+) length 1->2
    A.set(2,true);
    u32 Vals[] = {0xFFFFffff};
    B.set(Vals, 1);
    A.sub_integer_signed(B);
    u32 EVals[] = {1,1};
    Expected.set(EVals, ArrayCount(EVals), true);
    Tests.Append(A.equals_integer( Expected));

    //sub_integer_signed(-,-)
    A.set(2,true);
    B.set(1,true);
    A.sub_integer_signed(B);
    Expected.set(1,true);
    CheckA();

    B.set(2,true);
    A.sub_integer_signed(B);
    Expected.set(1);
    CheckA();


    A.set(1);
    A.add_integer_signed(A);
    Expected.set(2);
    CheckA();

    A.set(1);
    A.sub_integer_signed(A);
    Expected.set(0);
    CheckA();

    {
    //#29
    u32 AVals[] = {0,1};
    A.set(AVals, ArrayCount(AVals));
    u32 Vals[] = {0xFFFFFFFF};
    Expected.set(Vals,1);
    B.set(1);
    Expected.add_integer_signed(B);
    CheckA();
    }

    //test that Add won't allocate chunks when there are enough.
    {
        ChunkBits max_unsigned = 0;
        --max_unsigned ;
        auto X = BigDec_Arena( virtual_alloc_arena_alloc_1, max_unsigned);

        bool AllTestsPass = true;
        bool InitialCapacityIsOne = X.length == X.m_chunks_capacity && X.length == 1;
        AllTestsPass &= InitialCapacityIsOne;

        auto Y = BigDec_Arena( virtual_alloc_arena_alloc_1, 1);
        X.add_integer_signed(Y);
        //ensure that allocation has happened
        bool RequiredCapcaityGotAllocated = X.length == X.m_chunks_capacity && X.length == 2;
        AllTestsPass &= RequiredCapcaityGotAllocated;

        X.set(max_unsigned);
        bool SetPreservesCapacity = X.length == 1 && X.m_chunks_capacity == 2;
        AllTestsPass &= SetPreservesCapacity;

        X.add_integer_signed(Y);
        //ensure no memory leak on overflow
        bool OverflowUsesAvailableCapacity = X.length == X.m_chunks_capacity && X.length == 2;
        AllTestsPass &= OverflowUsesAvailableCapacity;

        Tests.Append(AllTestsPass);
    }


    //Mul
    u32 FullProd[2] = {};
//    FullMul32(0xFFFFFFFF,0xFFFFFFFF, FullProd);
    FullMulN<u32>(0xFFFFFFFF,0xFFFFFFFF, FullProd);
    Tests.Append(FullProd[0] == 0x00000001 && FullProd[1] == 0xFFFF'FFFE);

    A.set(1);
    B.set(1,true);
    A.mul_integer(B);
    Expected.set(1,true);
    CheckA();

    A.set(1,true);
    B.set(1);
    A.mul_integer(B);
    Expected.set(1,true);
    CheckA();

    A.set(1,true);
    B.set(1,true);
    A.mul_integer(B);
    Expected.set(1);
    CheckA();

    {
        cout << "Test#" << Tests.TestCount << "\n";
        u32 ValA[] = {4'000'000'000U};
        A.set(ValA,1); //0xee6b2800
        B.set(10);
        A.mul_integer(B); //0x9502f9000 == { 0x502f9000 , 0x9 } == { 1345294336 , 9}
        u32 EVals[] = {1345294336,9};
        Expected.set(EVals, ArrayCount(EVals));
        Tests.Append(A.equals_integer( Expected));
    }

    {
        u32 Vals[] = {0xFFFF'FFFF};
        A.set(Vals,1);
    //    B.set(A); //TODO(##2023 12 01) implement
        B.set(Vals,1);
        A.mul_integer(B);
        u32 EVals[] = {1,0xFFFF'FFFE};
        Expected.set(EVals, ArrayCount(EVals));
        Tests.Append(A.equals_integer(Expected));
        cout << "Mul 0xFFFF'FFFF 0xFFFF'FFFF => {1,0xFFFF'FFFE}  " << '\n';
        cout << std::hex <<  string(Expected) << std::dec << '\n';
    }

    {
        u32 ValA[] = {0x1000'0000};
        A.set(ValA,1);
        B.set(0x1000,0,Log2I(0x1000));
        // 1000'0000'000
        //=100'0000'0000
        Expected.set(0x100'0000'0000ull);
        A.mul_integer(B);
        CheckA();
    }

    {
        u32 ValuesA[] = {0,0,0x1000'0000};
        u32 ValuesB[] = {0,0x1000'0000};
        A.set(ValuesA, ArrayCount(ValuesA));
        B.set(ValuesB, ArrayCount(ValuesB));
        u32 ValuesExp[] = {0,0,0,0,0x100'0000};
        A.mul_integer(B);
        Expected.set(ValuesExp, ArrayCount(ValuesExp));
        CheckA();
    }


    { //TODO(##2023 12 17): verify all bits, not just the last N ones
        /*NOTE(ArokhSlade##2024 09 30): BIN(10^k) ends with 10^k, e.g. 10_dec = 1010_bin, 100_dec = 0110'0100_bin, 1000_dec = something'something'1000_bin, ...
        */
        A.set(1);
        B.set(10,0,3);
        for (u32 Pow = 1 ; Pow < 100 ; ++Pow) {
            A.mul_integer(B);
            cout << string(A) << '\n';
            u32 LeastBitIdx = Pow%CHUNK_WIDTH;
            u32 LeastChunkIdx = Pow/CHUNK_WIDTH;;

            bool LastNBitsGood = A.get_chunk(LeastChunkIdx)->value & (1ull<<LeastBitIdx);
            if (!LastNBitsGood) {
                cout << "ERROR: " << Pow << ", " << Pow << '\n';
            }
            for (u32 BitIdx = 0 ; BitIdx < Pow-1 ; ++BitIdx) {
                u32 LocalBitIdx = BitIdx%CHUNK_WIDTH;;
                u32 ChunkIdx = BitIdx/CHUNK_WIDTH;;
                LastNBitsGood = LastNBitsGood && ~(A.get_chunk(ChunkIdx)->value)&(1ull<<LocalBitIdx);
                if (!LastNBitsGood) {
                    cout << "ERROR: " << Pow << ", " << BitIdx << '\n';
                }
            }
//            cout << "Last N Bits Good: " << (LastNBitsGood ? "Yes" : "No") << '\n';
        }
    }

    //TODO(##2023 12 04): test copy_to(), make sure allocations happen only when necessary, and that length and capacity have correct values in all scenarios

    //tests for shift_left(N)

    {
        //case 1: new leading idx > old leading idx
        u32 ValuesA[] {0xF000F000, 0x0000FFFF};
        A.set(ValuesA, ArrayCount(ValuesA));
        u32 ShiftAmount = 16;
        u32 ValuesExpected[] {0xF000'0000, 0xFFFFF000 };
        Expected.set(ValuesExpected, ArrayCount(ValuesExpected));
        A.shift_left(ShiftAmount);
        CheckA();
    }

    {
        //case 2: new leading idx < old leading idx
        u32 ValuesA[] {0xF000F000, 0x0000FFFF};
        A.set(ValuesA, ArrayCount(ValuesA));
        u32 ShiftAmount = 24;
        u32 ValuesExpected[] {0x0000'0000, 0xFFF0'00F0, 0x0000'00FF };
        Expected.set(ValuesExpected, ArrayCount(ValuesExpected));
        A.shift_left(ShiftAmount);
        CheckA();
    }

    {
        //case 3: new leading idx == old leading idx
        u32 ValuesA[] {0xF000F000, 0x0000FFFF};
        A.set(ValuesA, ArrayCount(ValuesA));
        u32 ShiftAmount = 32;
        u32 ValuesExpected[] {0x0000'0000, 0xF000'F000, 0x0000FFFF };
        Expected.set(ValuesExpected, ArrayCount(ValuesExpected));
        A.shift_left(ShiftAmount);
        CheckA();
    }

    {
        //case 4
        A.set(1);
        A.copy_to(&OldA);
        u32 ShiftAmount = 132;
        u32 ValuesExpected[] {0x0000'0000, 0x0000'0000, 0x0000'0000, 0x0000'0000, 0x10 };
        Expected.set(ValuesExpected, ArrayCount(ValuesExpected));
        A.shift_left(ShiftAmount);
        char TestName[] = "shift_left 1<<132";
        PrintTestA(TestName);
        CheckA();
    }

    {
        //case 5
        cout << "Test#" << Tests.TestCount << " : 1 << [0...255]\n";
        bool TestPassed = true;
        for (i32 i = 0 ; i<256 ; ++i) {
            A.set(1);
            A.shift_left(i);
            i32 MSB = A.get_msb();
            TestPassed &= MSB == i;
            cout << "1<<" << i << " = " << string(A) << "\n";
        }
        Tests.Append(TestPassed);
    }

    {
        //print different shifts
        A.set(1);
        for (u32 i = 0 ; i < 64 ; ++i) {
            cout << string(A) << '\n';
            A.shift_left(i);
        }
        cout << string(A) << '\n';
    }

    {
        //print different shifts
        cout << string(A) << '\n';
        for (u32 i = 0 ; i < 64 ; ++i) {
            A.shift_right(64-1-i);
            cout << string(A) << '\n';
        }
        A.shift_right(1);
        cout << string(A) << '\n';
    }


    //tests for shift_right(N)

    {
        u32 ValuesA[] {0xFFFF'FFFF, 0xFFFF'FFFF, 0xFFFF'FFFF, 0xFFFF'FFFF, };
        A.set(ValuesA, ArrayCount(ValuesA));
        u32 ShiftAmount = 0;
        u32 ValuesExpected[] {0xFFFF'FFFF, 0xFFFF'FFFF, 0xFFFF'FFFF, 0xFFFF'FFFF, };
        Expected.set(ValuesExpected, ArrayCount(ValuesExpected));
        A.shift_right(ShiftAmount);
        CheckA();
    }

    {
        u32 ValuesA[] {0x1};
        A.set(ValuesA, ArrayCount(ValuesA));
        u32 ShiftAmount = 1;
        u32 ValuesExpected[] {0x0};
        Expected.set(ValuesExpected, ArrayCount(ValuesExpected));
        A.shift_right(ShiftAmount);
        CheckA();
    }

    {
        u32 ValuesA[] {0x2};
        A.set(ValuesA, ArrayCount(ValuesA));
        u32 ShiftAmount = 1;
        u32 ValuesExpected[] {0x1};
        Expected.set(ValuesExpected, ArrayCount(ValuesExpected));
        A.shift_right(ShiftAmount);
        CheckA();
    }

    {
        u32 ValuesA[] {0x00000000, 0xFFFFFFFF};
        A.set(ValuesA, ArrayCount(ValuesA));
        u32 ShiftAmount = 32;
        u32 ValuesExpected[] {0xFFFF'FFFF};
        Expected.set(ValuesExpected, ArrayCount(ValuesExpected));
        A.shift_right(ShiftAmount);
        CheckA();
    }

    {
        u32 ValuesA[] {0x8000'8000, 0xFFFF'0000, 0xAAAA'AAAA};
        A.set(ValuesA, ArrayCount(ValuesA));
        u32 ShiftAmount = 48;
        u32 ValuesExpected[] {0xAAAA'FFFF, 0x0000'AAAA };
        Expected.set(ValuesExpected, ArrayCount(ValuesExpected));
        A.shift_right(ShiftAmount);
        CheckA();
    }

    {
        u32 ValuesA[] {0x0000'0000, 0x0000'8000};
        A.set(ValuesA, ArrayCount(ValuesA));
        u32 ShiftAmount = 17;
        u32 ValuesExpected[] {0x4000'0000 };
        Expected.set(ValuesExpected, ArrayCount(ValuesExpected));
        A.shift_right(ShiftAmount);
        CheckA();
    }

    {
        //print different shifts
        u32 ValuesA[] {0x5555'8888, 0xFFFF'6666, 0xAAAA'9999};
        A.set(ValuesA, ArrayCount(ValuesA));
        for (u32 i = 0 ; i <= 96 ; ++i) {
            cout << string(A) << '\n';
            A.shift_right(1);
        }
        cout << string(A) << '\n';
    }
    cout << Tests.TestCount << "\n";
    BigDec_Arena ExpectedInteger ( virtual_alloc_arena_alloc_1, 0);
    BigDec_Arena ExpectedFraction { virtual_alloc_arena_alloc_1, 0};
    BigDec_Arena QuoInt { virtual_alloc_arena_alloc_1, 0};
    BigDec_Arena QuoFrac { virtual_alloc_arena_alloc_1, 0};
    auto CheckQuo = [&](){
        //The Algorithm is optimized to append multiple 0's in a row when possible.
        //we truncate the result if its precision is higher than that of Expected
        int Diff = QuoFrac.count_bits()-ExpectedFraction.count_bits();
        if (Diff < 0) {
            //Quo was not computed to the desired Precision
            Tests.Append(false);
            return;
        }
        QuoFrac.shift_right(Diff);
        Tests.Append(  QuoInt.equals_integer( ExpectedInteger ) &&
                       QuoFrac.equals_integer( ExpectedFraction ) );
    };

    {
        A.set(24);
        B.set(4);
        ExpectedInteger.set(6);
        ExpectedFraction.set(0);
        div_integer<ArenaChunkAlloc>(A, B, QuoInt, QuoFrac);
        CheckQuo();
    }

    {
        A.set(24);
        B.set(24);
        ExpectedInteger.set(1);
        ExpectedFraction.set(0);
        div_integer<ArenaChunkAlloc>(A, B, QuoInt, QuoFrac);
        CheckQuo();
    }

    {
        A.set(1);
        B.set(2);
        ExpectedInteger.set(0);
        ExpectedFraction.set(1);
        ExpectedFraction.exponent = -1;
        div_integer<ArenaChunkAlloc>(A, B, QuoInt, QuoFrac);
        cout << "Test#" << Tests.TestCount << "\n";
        CheckQuo();

    }

    {

        // 1478/2048=0.7216796875 = 0.10'1110'0011 ( BIN 10'1110'0011 = DEC 739 )
        A.set(1478);
        B.set(2046);
        ExpectedInteger.set(0);
        ExpectedFraction.set(739);
        ExpectedFraction.exponent = -1;
        div_integer(A, B, QuoInt, QuoFrac, 10);
        CheckQuo();
    }


    {
        //3456/2123=1.62788506829957602128899907257...
        A.set(3456);
        B.set(2123);
        ExpectedInteger.set(1);
        u32 FracVal[]{0b1010'00001011'11010001'00110110'1001};
        ExpectedFraction.set(FracVal,1);
        ExpectedFraction.exponent = -1;
        div_integer(A, B, QuoInt, QuoFrac, 32);
        CheckQuo();
    }

    {
        //3456/2123=1.62788506829957602128899907257...
        A.set(3456,1);
        B.set(2123,1);
        ExpectedInteger.set(1);     //1010'00001011'11010001'00110110'1001    1111'11100100'11111101
        u32 FractionChunks[2] = {0b00110110'10011111'11100100'11111101, 0b1010'00001011'11010001};
        ExpectedFraction.set(FractionChunks, ArrayCount(FractionChunks));
        ExpectedFraction.exponent = -1;
        div_integer(A, B, QuoInt, QuoFrac, 52);
        CheckQuo();
    }
    {
        //#57
        cout << "Test#" << Tests.TestCount << "\n";
        A.set(10,1);
        B.set(4);
        ExpectedInteger.set(2, 1, 1);
        u32 FractionChunks[1] = {0b1};
        ExpectedFraction.set(FractionChunks, ArrayCount(FractionChunks));
        ExpectedFraction.exponent = -1;
        div_integer(A, B, QuoInt, QuoFrac, 1);
        CheckQuo();
    }

    {
        A.set(10);
        B.set(3,1);
        ExpectedInteger.set(3,1,Log2I(3)); //dec 3.3333333... == bin 11.0101010101...
        u32 FractionChunks[2] = {0b01010101'01010101'01010101'01010101, 0b0101'01010101'01010101};
        ExpectedFraction.set(FractionChunks, ArrayCount(FractionChunks));
        ExpectedFraction.exponent = -2;
        div_integer(A, B, QuoInt, QuoFrac, 52);
        CheckQuo();
    }

    {
        A.set(1);
        B.set(1'000'000);

        div_integer(A, B, QuoInt, QuoFrac, 52);
        cout << "1 millionth (fraction part ): " << QuoFrac << ", " << QuoFrac.exponent << " exponent, " << QuoFrac.count_bits() << " explicit significant fraction bits" << '\n';

        B.set(1'000'000'000);
        div_integer(A, B, QuoInt, QuoFrac, 52);
        cout << "1 billionth (fraction part ): " << QuoFrac << ", " << QuoFrac.exponent<< " exponent, " << QuoFrac.count_bits() << " explicit significant fraction bits" << '\n';

//        B.set(1'000'000'000'000);
//        div_integer(A, B, QuoInt, QuoFrac, 52);
//        cout << "1 trillionth (fraction part ): " << QuoFrac << '\n';
//
//        B.set(1'000'000'000'000'000);
//        div_integer(A, B, QuoInt, QuoFrac, 52);
//        cout << "1 quadrillionth (fraction part ): " << QuoFrac << '\n';
    }


    {
//        test multi-block division
        u32 ValuesA[] {0x8000'0000, 0x0000'0000, 0x0000'0000, 0x8000'0000};
        cout << "Test#" << Tests.TestCount << " : multi block division\n";
        u32 ValuesB[] {0x8000'0000};
        A.set(ValuesA, ArrayCount(ValuesA));
        B.set(ValuesB, ArrayCount(ValuesB));
        u32 ValuesExpected[] {0x0000'0001,0x0000'0000,0x0000'0000,0x0000'0001};
        ExpectedInteger.set(ValuesExpected, ArrayCount(ValuesExpected));
        ExpectedFraction.set(0);
        ExpectedFraction.exponent = 0;

        div_integer(A, B, QuoInt, QuoFrac, 52);
        CheckQuo();
    }

    {
        //Test copy constructor
        u32 ValuesA[] { 0x0000'0000, 0x0FFFF'FFFF };
        A.set(ValuesA, ArrayCount(ValuesA));

        BigDec_Arena A_ { virtual_alloc_arena_alloc_1,A};
        //BigDec_Arena A__ {A}; //disallowed, no shallow copies, deep copy requires an allocator as parameter.

        u32 NewValues[] { 0xFFFF'FFFF, 0x8000'0000 };
        A_.set(NewValues,ArrayCount(NewValues));
        u32 ValuesExpected[] { 0x0000'0000, 0x0FFFF'FFFF };
        Expected.set(ValuesExpected, ArrayCount(ValuesExpected));
        CheckA();
    }

    {
        //test truncate_trailing_zero_bits (from fractional part)

        u32 ValuesA[] { 0xAAAA'0000, 0xAAAA'AAAA };
        A.set(ValuesA, ArrayCount(ValuesA), 0, 47);
        cout << "Test #" << Tests.TestCount << " : Test truncate_trailing_zero_bits : A = " << string(A);
        A.truncate_trailing_zero_bits();
        cout << " -> " << string(A) << "\n";
        u32 ValuesExpected[] { 0x5555'5555, 0x5555 };
        Expected.set(ValuesExpected, ArrayCount(ValuesExpected), 0, 47 );
        Tests.Append(A.equals_fractional(Expected));

        //---------------------------------------------v fractional point here
        u32 ValuesA_1[] { 0x0, 0b1111'1111'1111'1111'0101'1000'0000'0000 };
        A.set( ValuesA_1, ArrayCount(ValuesA_1), 0, 15 );
        cout << "Test #" << Tests.TestCount << " : Test truncate_trailing_zero_bits : A = " << string(A);
        A.truncate_trailing_zero_bits();
        cout << " -> " << string(A) << "\n";
        //----------------------------------------------------------v fractional point here
        u32 ValuesExpected_1[] { 0b00000000'000'11111111'11111111'01011 };
        Expected.set( ValuesExpected_1, ArrayCount(ValuesExpected_1), 0, 15 );
        Tests.Append(A.equals_fractional(Expected));

        //--------------------------------------------------v fractional point here
        u32 ValuesA_2[] { 0x0, 0b1111'1111'1111'1111'0101'1000'0000'0000 };
        A.set( ValuesA_2, ArrayCount(ValuesA_2), 0, 19 );
        cout << "Test #" << Tests.TestCount << " : Test truncate_trailing_zero_bits : A = " << string(A);
        A.truncate_trailing_zero_bits();
        cout << " -> " << string(A) << "\n";
        //---------------------------------------------------------------v fractional point here
        u32 ValuesExpected_2[] { 0b00000000'000'11111111'11111111'0101'1 };
        Expected.set( ValuesExpected_2, ArrayCount(ValuesExpected_2), 0, 19 );
        Tests.Append(A.equals_fractional(Expected));

        //-----------------------------------------------------v fractional point here
        u32 ValuesA_3[] { 0x0, 0b1111'1111'1111'1111'0101'10'00'0000'0000 };
        A.set( ValuesA_3, ArrayCount(ValuesA_3), 0, 21 );
        cout << "Test #" << Tests.TestCount << " : Test truncate_trailing_zero_bits : A = " << string(A);
        A.truncate_trailing_zero_bits();
        cout << " -> " << string(A) << "\n";
        //-------------------------------------------------------------------v fractional point here
        u32 ValuesExpected_3[] { 0b0'0000'0000'0011'1111'1111'1111'1101'011 };
        Expected.set( ValuesExpected_3, ArrayCount(ValuesExpected_3), 0, 21 );
        Tests.Append(A.equals_fractional(Expected));
    }

#endif

    {
        //use BigDecimal to represent arbitrary precision fractional numbers
        A.set(3);
        B.set(2);
        u32 ValuesExpected[] = { 0b1'1 };
        Expected.set(ValuesExpected, ArrayCount(ValuesExpected), 0, 0);

        A.div_integer(B); // A= 3/2
        cout << "Test #" << Tests.TestCount << " : " << string(A) << "\n";
        CheckA();


        ValuesExpected[0] = { 0b11 };
        Expected.set(ValuesExpected, ArrayCount(ValuesExpected), 0, 1);

        A.copy_to(&B); // B= 3/2
        A.add_fractional(A); // A= 6/2 = 3
        cout << "Test #" << Tests.TestCount << " : " << string(A) << "\n"; //#88
        Tests.Append(A.equals_fractional(Expected));



        ValuesExpected[0] = { 0b100'1 };
        Expected.set(ValuesExpected, ArrayCount(ValuesExpected), 0, 2);

        A.add_fractional(B); // A= 3+3/2 = 4.5
        cout << "Test #" << Tests.TestCount << " : " << string(A) << "\n";
        Tests.Append(A.equals_fractional(Expected));

        A.set(81,1); //-81
        B.set(6);
        ValuesExpected[0] = 0b1101'1;
        Expected.set(ValuesExpected, ArrayCount(ValuesExpected), 1, 3);

        A.div_integer(B); // C= -81/6 = -13.5
        cout << "Test #" << Tests.TestCount << " got/exp :\n" << string(A) << "\n" << string(Expected) << "\n";;
        CheckA();

        A.set(3);
        A.exponent = Log2I(3);
        A.normalize();
        B.set(2);
        B.exponent = Log2I(2);
        B.normalize();
        A.div_fractional(B); // A= 3/2
        B.div_fractional(A,33); // B= 4/3
        u32 ValuesExpected_1[] = { 0b01010101'01010101'01010101'01010101, 0b101 };
        Expected.set(ValuesExpected_1, ArrayCount(ValuesExpected_1), 0, 0);
        cout << "Test #" << Tests.TestCount << " : \nB= \t  " << string(B);
        cout << "\nExpected= " << string(Expected) << "\n" ;
        Tests.Append(B.equals_fractional(Expected) && B.count_bits() == 35);

        A.add_fractional(B); // A= 17/6
        ValuesExpected_1[1] = 0b1011;
        ValuesExpected_1[0] = 0b01010101'01010101'01010101'01010101;
        Expected.set(ValuesExpected_1, ArrayCount(ValuesExpected_1), 0, 1);
        cout << "Test #" << Tests.TestCount << " : \nA= \t  ";
        cout << string(A);
        cout << "\nExpected= " << string(Expected) << "\n" ;
        Tests.Append(A.equals_fractional(Expected) && A.count_bits() == 36);

        B.sub_fractional(A); // B= -9/6
        ValuesExpected[0] = 0b1'1;
        Expected.set(ValuesExpected, ArrayCount(ValuesExpected), 1, 0);
        cout << "Test #" << Tests.TestCount << " : \nB= \t  ";
        cout << string(B);
        cout << "\nExpected= " << string(Expected) << "\n" ;
        Tests.Append(B.equals_fractional(Expected));

        cout << "Test #" << Tests.TestCount << " : \nA.round_to_n_significant_bits(33);\n";
        A.round_to_n_significant_bits(33);
        cout << "=" << string(A) << "\n";
        ValuesExpected_1[1] = 0b1;
        ValuesExpected_1[0] = 0b0'1101'0101'0101'0101'0101'0101'0101'011;
        Expected.set(ValuesExpected_1, ArrayCount(ValuesExpected_1), 0, 1);
        cout << "(" << string(Expected) << ")\n";
        Tests.Append(A.equals_fractional(Expected));

        cout << "Test #" << Tests.TestCount << " : \nA= \t  ";
        cout << string(A) << "\n*\t  " << string(B) << "\n=\t  ";
        //17/6 * -9/6 = 2.833333333333 * -1.5
        A.mul_fractional(B); // A= -51/12 == -4.25
        ValuesExpected_1[1] = 0b100;
        ValuesExpected_1[0] = 0b0100'0000'0000'0000'0000'0000'0000'0001; //NOTE(##2024 06 12)not sure if this is what float would produce?
        Expected.set(ValuesExpected_1, ArrayCount(ValuesExpected_1), 1, 2);
        cout << string(A);
        cout << "\nExpected= " << string(Expected) << "\n" ;
        Tests.Append(A.equals_fractional(Expected));

        {
            unsigned long X = 0x0000'B0AA'AAAA'0640;
            unsigned long Y = 0x0000'0000'0000'F83F;
            unsigned long Z = 0x0000'0400'0000'1140;
            double *XPF = (double*)&X;
            double *YPF = (double*)&Y;
            double *ZPF = (double*)&Z;
            double ZF = *XPF * *YPF;
            cout << *XPF << " * " << *YPF << " = " << Z << " ( " << *ZPF << " ) \n";
        }

        double XF = 2.83333333348855376243591308594E0; //0x 4006AAAAAAB00000 ?
        double YF = 1.5;
        double ZFE = 4.25000000023283064365386962891E0; //0x 4011000000040000 ?
        double ZF = XF * YF;
        cout << XF << " * " << YF << " = " << ZF << " ( " << ZFE << " ) \n";

        //---------

        A.set(3,0,1);
        A.normalize();
        B.set(2,0,1);
        B.normalize();
        A.div_fractional(B,200); // A= 3/2
        B.div_fractional(A,200); // B= 4/3

        A.add_fractional(B); // A= 17/6

        cout << "Test #" << Tests.TestCount << "\n";
        cout << "A= " << string(A) << "\n";

        B.sub_fractional(A); // B= -9/6

        cout << "B= " << string(B) << "\n";

        A.round_to_n_significant_bits(192);
        cout << "A.round_to_n_significant_bits(192);\n";
        cout << "A= " << string(A) << "\n";

        A.mul_fractional(B); // A= -51/12 == -4.25
        ValuesExpected[0] = 0b100'01;
        Expected.set(ValuesExpected, ArrayCount(ValuesExpected), 1, 2);
//        ValuesExpected_1[1]= 4;
//        ValuesExpected_1[0]= 0b0011'0101'0101'0101'0101'0101'0101'0101;
//        Expected.set(ValuesExpected_1, ArrayCount(ValuesExpected_1), 1, 32);

        cout << "A*B= " << string(A) << "\n";

        A.round_to_n_significant_bits(53);
        cout << "A.round_to_n_significant_bits(53);\n";
        cout << "A= " << string(A) << "\n";
        cout << "\nExpected= " << string(Expected) << "\n" ;
        Tests.Append(A.equals_fractional(Expected)); //#96

        //---------

        A.set(1);
        B.set(3);
        i32 Precision = 4;
        cout << string(A) << " / " << string(B) << "(Prec=" << Precision << ")\n= ";
        A.div_fractional(B,Precision);
        cout << string(A) << "\n";
        A.set(1);
        Precision = 5;
        cout << string(A) << " / " << string(B) << "(Prec=" << Precision << ")\n= ";
        A.div_fractional(B,Precision);
        cout << string(A) << "\n";
        cout << "\n";

        A.set(1);
        A.div_integer(B,32); //B=3
        A.round_to_n_significant_bits(30);
        ValuesExpected[0] = 0b0'0101'0101'0101'0101'0101'0101'0101'011;
        Expected.set(ValuesExpected, ArrayCount(ValuesExpected), 0, -2);
        Tests.Append(A.equals_fractional(Expected)); //#97
        cout << string(A) << "\n";
        cout << string(Expected) << "\n";
//
//        Tests.Append(C.RoughlyEquals(A,16));
    }

#if 1
    {

        /*
        test normalize()
        A = 00 88 11 00 00 -> A = 88 11
        */
        char TestName[]="normalize()";
        A.set(0);
        A.set(0x8811,false,7);
        A.shift_left(64);
        A.extend_length();
        A.exponent = 7;
        A.copy_to(&OldA);
        A.normalize();
        Expected.set(0x8811);
        Expected.exponent = 7;
        PrintTestA(TestName);
        Tests.Append(A.equals_fractional(Expected));

    }

    {
        cout << "Test#" << Tests.TestCount << "\n";
        A.set(0x10'000'000);
        A.normalize();
        Expected.set(1);
        Expected.exponent = 0; //Normalize doesn't change exponent

        Tests.Append(A.equals_fractional(Expected));

    }


    {
        //2024 07 04: Test to_float
        cout << "\tTest #" << Tests.TestCount << "\n";
        char TestName[] = "Test to_float";
        float Challenge = -0x1.FFFFFEp127;
        A.set_float(Challenge);
        Expected.set(0xFFFFFF,true,127);
        Tests.Append(A.equals_fractional(Expected));
        cout << TestName << "\n";
        cout << std::hexfloat << Challenge << " -> " << "\n";
        cout << string(A) << "\n";
        cout << "(" << string(Expected) << ")\n";

        float Parsed = A.to_float();
        Tests.Append(Parsed==Challenge);
        cout << Parsed << " <- " << "\n";
    }



    {
        cout << "\tTest #" << Tests.TestCount << "\n";
        cout << "Denormalized" << "\n";
        float Challenge = 0x1p-149f;
        A.set_float(Challenge);
        Expected.set(0x1,false, -149);
        Tests.Append(A.equals_fractional(Expected));
        float Parsed = A.to_float();
        Tests.Append(Parsed == Challenge);
        cout << std::hexfloat << Challenge << " -> " << "\n";
        cout << string(A) << "\n";
        cout << "(" << string(Expected) << ")\n";
        cout << Parsed << " <- " << "\n";
    }

    {
        cout << "\tTest #" << Tests.TestCount << "\n";
        cout << "Denormalized" << "\n";
        float Challenge = 0x1.fffffcp-127f;
        A.set_float(Challenge);
        Expected.set(0x7fffff,false, -127);
        Tests.Append(A.equals_fractional(Expected));
        float Parsed = A.to_float();
        Tests.Append(Parsed == Challenge);
        cout << std::hexfloat << Challenge << " -> " << "\n";
        cout << string(A) << "\n";
        cout << "(" << string(Expected) << ")\n";
        cout << Parsed << " <- " << "\n";
    }
#endif

    {
        cout << "\tTest #" << Tests.TestCount << "\n";
        cout << "Infinity" << "\n";
        float Challenge = 0x1p+128f;
        HardAssert (IsInf(Challenge));
        A.set_float(Challenge);
        Expected.set(0x1,false, 128);
        Tests.Append(A.equals_fractional(Expected));
        float Parsed = A.to_float();
        Tests.Append(Parsed == Challenge);
        cout << std::hexfloat << Challenge << " -> " << "\n";
        cout << string(A) << "\n";
        cout << "(" << string(Expected) << ")\n";
        cout << Parsed << " <- " << "\n";
    }

    {
        cout << "\tTest #" << Tests.TestCount << "\n";
        cout << "Zero" << "\n";
        A.set(0x1,false,-150);
        float Parsed = A.to_float();
        Tests.Append(Parsed == 0.f);
        cout << string(A) << " -> " << std::hexfloat << Parsed << "\n";
    }

        cout << "\nTest parsed values where rounding changes exponent.\n";

    {
        cout << "\tTest #" << Tests.TestCount << "\n";
        cout << "for infinity\n";
        A.set(0xFF'FFFF8,true,127);
        A.normalize();
        A.copy_to(&B);
//        B.RoundToFloatPrecision();
        float Challenge = -Inf32;

        float Parsed = B.to_float();
        Tests.Append(Parsed == Challenge);

        cout << string(A) << " -> Round(24)\n";
        cout << string(B) << " -> to_float()\n";
        cout << " " << std::hexfloat << Parsed << "\n";
        cout << "(" << Parsed << ")\n";
    }

    {
        cout << "\tTest #" << Tests.TestCount << "\n";
        cout << "for infinity\n";
        A.set(0xFF'FFFE8,true,127);
        A.normalize();
        A.copy_to(&B);
//        B.RoundToFloatPrecision();
        float Challenge = -0x1.fffffcp+127;

        float Parsed = B.to_float();
        Tests.Append(Parsed == Challenge);

        cout << string(A) << " -> Round(24)\n";
        cout << string(B) << " -> to_float()\n";
        cout << " " << std::hexfloat << Parsed << "\n";
        cout << "(" << Challenge << ")\n";
    }


    {
        cout << "\tTest #" << Tests.TestCount << "\n";
        cout << "for subnormal\n";
        A.set(0b11,true,-149);
        real32 Parsed = A.to_float();
        real32 Challenge = -0x1p-148f;
        Tests.Append(Parsed == Challenge);
        cout << string(A) << "-> to_float()\n";
        cout << std::hexfloat << Parsed << "\n";
        cout << "(" << Challenge << ")\n";
    }

    {
        cout << "\tTest #" << Tests.TestCount << "\n";
        cout << "for regular" << "\n";
        A.set(0x1FFFFFF,true,0);
        real32 Parsed = A.to_float();
        real32 Challenge = -0x1p+1f;
        Tests.Append(Parsed == Challenge);
        cout << string(A) << "-> to_float()\n";
        cout << std::hexfloat << Parsed << "\n";
        cout << "(" << Challenge << ")\n";
    }

    {
        cout << "\tTest #" << Tests.TestCount << "\n";
        cout << "for almost zero:\n";
        A.set(0b11,true,-150);
        real32 Parsed = A.to_float();
        real32 Challenge = -0x1p-149;
        Tests.Append(Parsed == Challenge);
        cout << string(A) << "-> to_float()\n";
        cout << std::hexfloat << Parsed << "\n";
        cout << "(" << Challenge << ")\n";
    }


    //TODO(##2024 07 13): test cases for round to even where Guard,Round,Sticky bits are needed. -> current rounding function may be biased towards infinity in certain cases.
    cout << "\nTest parity of ieee-754 round-to-even behavior, with guard, round and sticky bits\n\n";

    {
        auto Test = [&A,&Tests,only_errors](u64 Val, bool Neg, i32 Exp, f32 Expected){
            bool OK = true;
            A.set(Val,Neg,Exp);
            f32 Result = A.to_float();
            OK &= Result == Expected;
            if (!OK || !only_errors) {
                cout << "Tests #" << Tests.TestCount << "\n";
                cout << string(A) << " -> to_float() ->\n"
                << " " << Result << "\n"
                << "(" << Expected << ")\n";
                cout << (OK ? "OK" : "ERROR") << "\n";
            }
            Tests.Append(OK);
        };

        typedef BigDec_Arena bignum ;
        auto Test2 = [&Tests, only_errors](bignum& A, f32 Expected){
           bool OK = true;
            f32 Result = A.to_float();
            OK &= Result == Expected;
            if (!OK || !only_errors) {
                cout << "Tests #" << Tests.TestCount << "\n";
                cout << string(A) << " -> to_float() ->\n"
                << " " << Result << "\n"
                << "(" << Expected << ")\n";
                cout << (OK ? "OK" : "ERROR") << "\n";
            }
            Tests.Append(OK);
        };


        Test(0x1,0,0,1.f);
        Test(0x1,0,-1,  0x1.p-1f);
        Test(0x1,0,-12, 0x1.p-12f);
        Test(0x1,0,-125,0x1.p-125);
        Test(0x1,0,-126,0x1.p-126);
        Test(0x1,0,-127,0x1.p-127);
        Test(0x1,0,-128,0x1.p-128);
        Test(0x1,0,-129,0x1.p-129);
        Test(0x1,0,-149,0x1.p-149);
        Test(0x1,0,-150,0x1.p-150);
        Test(0x1,0,-151,0x1.p-151);
        Test(0x1,0,-155,0x1.p-155);

        Test(0x0,0,0   ,0.f);
        Test(0x0,1,1   ,0.f);
        Test(0x0,0,-1  ,0.f);
        Test(0x0,0,-128,0.f);
        Test(0x0,0,128 ,0.f);
        Test(0x0,1,-128,0.f);
        Test(0x0,1,128 ,0.f);



        double Tiny;
        float Small;
        cout << std::hexfloat;

        auto IEEE754 = [&Tiny, &Small](double Val){
            Tiny = Val;
            Small = Tiny;
            cout << Tiny << " = " << Small << "\n";
        };

        cout << "1x2^-149 is in float, but 1x2^-150 isn't:\n\n";
        IEEE754(0x1.p-149);
        IEEE754(0x1.p-150);
        Test(0x1,0,-149,0x1.p-149f);
        Test(0x1,0,-150,0.f);

        cout << "\nguard and round bit set => round up\n";
        IEEE754(0x1.8p-150);
        Test(0x3,0,-150,0x1.p-149f);

        cout << "\nguard and sticky bit set => round up\n";
        IEEE754(0x1.4p-150);
        Test(0x5,0,-149,0x1.p-149f);

        cout << "\nguard and (very Small) sticky bit set => round up\n";
        IEEE754(0x1.000001p-150);
        Test(0x1000001,0,-150,0x1.p-149f);

        cout << "\nonly guard bit set => round to even rule applies => bit before guard bit == 0 => round down\n";
        IEEE754(0x1.p-150);
        Test(0x1,0,-150,0.f);

        cout << "\nonly guard bit set => round to even rule applies => bit before guard bit == 1 => round up\n";
        IEEE754(0x1.8p-149);
        Test(0x3,0,-149,0x1.p-148f);

        cout << "\nround-to-even-rule - more tests\n";
        bool OK;

        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 0, GRS==000\n";
        IEEE754(0x1.0000000p0);
        A.set(0x10000000);
        A.normalize();
        Test2(A,Small);


        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 1, GRS==000\n";
        IEEE754(0x1.0000020p0);
        A.set(0x10000020);
        A.normalize();
        Test2(A,Small);

        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 0, GRS==001\n";
        IEEE754(0x1.0000001p0);
        A.set(0x10000001);
        A.normalize();
        Test2(A,Small);


        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 1, GRS==001\n";
        IEEE754(0x1.0000021p0);
        A.set(0x10000021);
        A.normalize();
        Test2(A,Small);


        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 0, GRS==010\n";
        IEEE754(0x1.0000008p0);
        A.set(0x10000008);
        A.normalize();
        Test2(A,Small);


        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 1, GRS==010\n";
        IEEE754(0x1.0000028p0);
        A.set(0x10000028);
        A.normalize();
        Test2(A,Small);


        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 0, GRS==011\n";
        IEEE754(0x1.0000009p0);
        A.set(0x10000009);
        A.normalize();
        Test2(A,Small);


        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 1, GRS==011\n";
        IEEE754(0x1.0000029p0);
        A.set(0x10000029);
        A.normalize();
        Test2(A,Small);


        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 0, GRS==100\n";
        IEEE754(0x1.000001p0);
        A.set(0x1000001);
        A.normalize();
        Test2(A,Small);


        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 1, GRS==100\n";
        IEEE754(0x1.000003p0);
        A.set(0x1000003);
        A.normalize();
        Test2(A,Small);


        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 0, GRS==101\n";
        IEEE754(0x1.0000011p0);
        A.set(0x10000011);
        A.normalize();
        Test2(A,Small);


        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 1, GRS==101\n";
        IEEE754(0x1.0000031p0);
        A.set(0x10000031);
        A.normalize();
        Test2(A,Small);


        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 0, GRS==110\n";
        IEEE754(0x1.0000018p0);
        A.set(0x10000018);
        A.normalize();
        Test2(A,Small);


        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 1, GRS==110\n";
        IEEE754(0x1.0000038p0);
        A.set(0x10000038);
        A.normalize();
        Test2(A,Small);


        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 0, GRS==111\n";
        IEEE754(0x1.0000019p0);
        A.set(0x10000019);
        A.normalize();
        Test2(A,Small);


        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 1, GRS==111\n";
        IEEE754(0x1.0000039p0);
        A.set(0x10000039);
        A.normalize();
        Test2(A,Small);

    }

    {
        A.set_float(3.f);
        B.set_float(2.f);
        A.div_fractional(B,32); // A= 3/2
        B.div_fractional(A,31); // B= 4/3
        u32 ValuesExpected_1[] = { 0b01010101'01010101'01010101'01010101, 0b1 };
        Expected.set(ValuesExpected_1, ArrayCount(ValuesExpected_1), 0, 0);
        cout << "\nTest #" << Tests.TestCount;
        cout << "\nB=4/3 == 0b1.0101...01";
        cout << "\nB= \t  " << string(B);
        cout << "\nExpected= " << string(Expected) << "\n" ;
        Tests.Append(B.equals_fractional(Expected) && B.count_bits() == 33);

        A.add_fractional(B); // A= 17/6
        ValuesExpected_1[1] = 0b10;
        ValuesExpected_1[0] = 0b11010101'01010101'01010101'01010101;
        Expected.set(ValuesExpected_1, ArrayCount(ValuesExpected_1), 0, 1);
        cout << "\nTest #" << Tests.TestCount;
        cout << "\nA == 3/2; A += B;";
        cout << "\n3/2 + 4/3 = 17/6 == 0b10.1101...01";
        cout << "\nA = " << string(A);
        cout << "\nExpected= " << string(Expected) << "\n" ;
        Tests.Append(A.equals_fractional(Expected) && A.count_bits() == 34);

        B.sub_fractional(A); // B= -9/6
        u32 ValuesExpected[] = {0b1'1};
        Expected.set(ValuesExpected, ArrayCount(ValuesExpected), 1, 0);
        cout << "\nTest #" << Tests.TestCount;
        cout << "\n4/3 - 17/6 = -9/6 == -0b1.1";
        cout << "\nB = " << string(B);
        cout << "\nExpected = " << string(Expected) << "\n" ;
        Tests.Append(B.equals_fractional(Expected));

        cout << "\nTest #" << Tests.TestCount << "\n";
        cout << "\nA.round_to_n_significant_bits(33) with A.BitCount == 34";
        cout << "\n=> A bits end with 010101";
        cout << "\n=>              LSB----^^---GuardBit";
        cout << "\n=> LSB == 0, Guard bit == 1, Round bit == Sticky bit == 0\n";
        cout << "\n=> round down (round-to-even)\n";
        A.round_to_n_significant_bits(33);
        cout << "=" << string(A) << "\n";
        Expected.set(0b10'1101'0101'0101'0101'0101'0101'0101'01, 0, 1);
        cout << "(" << string(Expected) << ")\n";
        Tests.Append(A.equals_fractional(Expected));

        double DA,DB;
        DA = 3.; DB = 2.;
        DA /= DB; DB /= DA; DA += DB;
        real32 FA=DA;
        cout << "\nTest #" << Tests.TestCount << "\n";
        cout << "double/float result: " << FA << "\n";
        cout << "BigDecimal result:      " << A.to_float() << "\n";
        Tests.Append(FA == A.to_float());
    }

    BigDec_Arena::close_context(true);

    //TODO(##2023 11 24): improve unit_test interface:
    //Tests.Assert(Condition)
    //Tests.AssertEqual(A,B)
    //Tests.AssertSatisfies(A,Predicate)
    //Tests.AssertAllSatisfy(ListA, Predicate) -> implement in terms of reduce/fold

    cout << "|-> " << Tests << "\n\n";
    Tests.PrintResults();
    return Tests.FailCount;
}

Test_Parsing(bool only_errors = false) {

    using std::cout;
    using std::string;
    using ArenaChunkAlloc = ArenaAlloc<ChunkBits>;
    using BigDec_Arena = BigDecimal<ArenaChunkAlloc>;

    i32 FailCount { 0 };

    std::cout << __func__ << "\n\n";

    test_result Tests {};

    memory_index ArenaSize = 256*32768*sizeof(u32);
    uint8 *ArenaBase = (uint8*)VirtualAlloc(0, ArenaSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    ArenaChunkAlloc  virtual_alloc_arena_alloc_2 { ArenaSize, ArenaBase, virtual_free_decommit};
    BigDec_Arena::initialize_context ( virtual_alloc_arena_alloc_2);

    BigDec_Arena A { virtual_alloc_arena_alloc_2, 9};
    BigDec_Arena OldA { virtual_alloc_arena_alloc_2, A};
    BigDec_Arena B { virtual_alloc_arena_alloc_2, 10};

    BigDec_Arena Expected ( virtual_alloc_arena_alloc_2, 0);

    auto CheckA = [&](){
        Tests.Append(A.equals_integer(Expected)); };

    auto PrintTestOutput = [&Tests](BigDec_Arena& Before, BigDec_Arena& After, BigDec_Arena& Wanted,
                                    char *BeforeName, char *AfterName, char*WantedName, char *TestName){
        cout << "Test #" << Tests.TestCount;
        if (TestName)   cout << " : " << TestName;
                        cout << "\n";
        if (BeforeName) cout << BeforeName << " = " << string(Before) << "\n" ;
        if (AfterName)  cout << AfterName << " = " << string(After) << "\n" ;
        if (WantedName) cout << WantedName << " = " << string(Wanted) << "\n" ;
    };

    char AName[] = "       A";
    char OldAName[] = "   Old A";
    char ExpectedName[] = "Expected";
    auto PrintTestA = [&](char *TestName){PrintTestOutput(OldA, A, Expected, OldAName, AName, ExpectedName, TestName);};

    std::cout << std::boolalpha;

    {

        std::cout << "\nTesting BigDecimal::parse_integer() ... \n";
        std::cout << "\nShould succeed ... \n";

        real32 Buf[1];
        std::cout << "Test # | PASS? | Input | Parsed | Expected " << '\n';

        char Input[][64] = {
            "1",          "0",                    "2",                                    "123.xyz",
            "456",        "70",                   "8",                                    "99999999",
            "1234567890", "12345678900987654321", "999999999999999999999999999999999999", "42MEANING"
        };
        real32 Expected[ArrayCount(Input)] = {
            1.f,           0.f,                    2.f,                                    123.f,
            456.f,         70.f,                   8.f,                                    99999999.f,
            1234567890.f,  12345678900987654321.f, 999999999999999999999999999999999999.f, 42.f
        };

        for (size_t i = 0 ; i < ArrayCount(Input) ; ++i) {
            bool Passed = BigDec_Arena::parse_integer(Input[i]);
            BigDec_Arena::temp_parse_int.normalize();
            Buf[0] = BigDec_Arena::temp_parse_int.to_float();
            Passed &= Buf[0] == Expected[i];

            const char *CheckMsg = Passed ? "OK"  : "ERROR";

            std::cout << "Test # " << Tests.TestCount << "  :  ";
            std::cout << CheckMsg << " : " << Input[i] << " -> " << Buf[0] << " ( " << Expected[i] << " )" << '\n';

            Tests.Append(Passed);
        }
        std::cout << "\n";
    }

    {
        std::cout << "\nTesting BigDecimal::from_string() ... \n";
        std::cout << "\nShould succeed ... \n";

        real32 Buf[1];
        std::cout << "Test # | PASS? | Input | Parsed | Expected " << '\n';

        char Input[][64] = {
            "-1",  "-123",  "-999",  "-0"
        };
        real32 Expected[ArrayCount(Input)] = {
            -1.f, -123.f, -999.f, -0.f
            };

        for (size_t i = 0 ; i < ArrayCount(Input) ; ++i) {
            bool Passed = BigDec_Arena::from_string(Input[i]);
            Buf[0] = BigDec_Arena::temp_from_string.to_float();
            Passed &= Buf[0] == Expected[i];
            const char *CheckMsg = Passed ? "OK"  : "ERROR";

            std::cout << "Test # " << Tests.TestCount << "  :  ";
            std::cout << CheckMsg << " : " << Input[i] << " -> " << Buf[0] << " ( " << Expected[i] << " )" << '\n';

            Tests.Append(Passed);
        }

        std::cout << "\n";
    }

    {
        std::cout << "\nTesting BigDecimal::parse_integer()... \n";
        std::cout << "\nShould fail (expects string to begin with digits) ... \n";

        real32 Buf[1];
        std::cout << "Test # | PASS? (Status) | Input | Parsed " << '\n';

        char Input[][64] = { ".0","+1e10", "-2e10", "abc", "lOS"};
        for (size_t i = 0 ; i < ArrayCount(Input) ; ++i) {

            bool Passed = !BigDec_Arena::parse_integer(Input[i]);
            BigDec_Arena::temp_parse_int.normalize();
            Buf[0] = BigDec_Arena::temp_parse_int.to_float();

            const char *CheckMsg = Passed ? "OK ( failed )"  : "ERROR";

            std::cout << "Test # " << Tests.TestCount << "  :  ";
            std::cout << CheckMsg << " : " << Input[i] << " -> " << Buf[0] << '\n';
            Tests.Append(Passed);
        }
        std::cout << "\n";
    }


    {
        char NumStr[] = "4294967296";
        BigDec_Arena::parse_integer(NumStr, &A);
        u32 ValuesExpected[] {0x0000'0000, 0x0000'0001};
        Expected.set(ValuesExpected, ArrayCount(ValuesExpected),0,32);

        PrintTestOutput(A,A,Expected,nullptr,"A       ","Expected", "Parse 4294967296");
        Tests.Append(A.equals_fractional(Expected));
    }


    {

        char NumStr[] = "999999999999999999";
        BigDec_Arena::parse_integer(NumStr, &A);
        //DE0 B6B3 A763 FFFF
        u32 ValuesExpected[] {0xA763'FFFF, 0x0DE0'B6B3};
        Expected.set(ValuesExpected, ArrayCount(ValuesExpected),0,59);
        PrintTestOutput(A,A,Expected,nullptr,"A       ","Expected", "Parse 999999999999999999");
        Tests.Append(A.equals_fractional(Expected));
    }

    {
        char NumStr[] = "1000000000000000";
        BigDec_Arena::parse_integer(NumStr, &A);
        //3 8D7E A4C6 8000
        u32 ValuesExpected[] {0xA4C6'8000, 0x0003'8D7E};
        Expected.set(ValuesExpected, ArrayCount(ValuesExpected),0,49);
        PrintTestOutput(A,A,Expected,nullptr,"A       ","Expected", "Parse 1000000000000000");
        Tests.Append(A.equals_fractional(Expected));
    }

    {
        cout << "Test #: " << Tests.TestCount << "\n";
        char NumStr[] = "999999999999999999";
        int MaxLen = StringLength(NumStr);
        int DigitCount = MaxLen;
        for (int i = 0 ; i < MaxLen  ; --DigitCount, ++i) {
            char * SubString = ( NumStr+MaxLen-DigitCount );
            BigDec_Arena::parse_integer(SubString, &A);
            //DE0 B6B3 A763 FFFF
            u32 ValuesExpected[][2] = {
                {0xa763ffff, 0xde0b6b3},
                {0x5d89ffff, 0x1634578},
                 {0x6fc0ffff, 0x2386f2},
                  {0xa4c67fff, 0x38d7e},
                   {0x107a3fff, 0x5af3},
                    {0x4e729fff, 0x918},
                     {0xd4a50fff, 0xe8},
                     {0x4876e7ff, 0x17},
                      {0x540be3ff, 0x2},
                      {0x3b9ac9ff, 0x0},
                       {0x5f5e0ff, 0x0},
                        {0x98967f, 0x0},
                         {0xf423f, 0x0},
                         {0x1869f, 0x0},
                          {0x270f, 0x0},
                           {0x3e7, 0x0},
                            {0x63, 0x0},
                             {0x9, 0x0}
            };
            A.exponent = 0; //check only the bits
            Expected.set(ValuesExpected[i], ArrayCount(ValuesExpected[i]));
            char TestName[64] = {'\0'};
            CatStrings("Parse ", SubString, ArrayCount(TestName), TestName);
            PrintTestOutput(A,A,Expected,nullptr,"A       ","Expected", TestName);
            Tests.Append(A.equal_bits(Expected));

        }
    }

    {
        char NumStr[] = "999999999999999999999999999999999999";
        BigDec_Arena::parse_integer(NumStr, &A);
        u32 ValuesExpected[] {0xA763'FFFF, 0x0DE0'B6B3};
        Expected.set(ValuesExpected, ArrayCount(ValuesExpected));
        char TestName[64] = {'\0'};

        cout << "\nEYEBALL TEST: 999999999999999999999999999999999999 = " << string(A) << '\n';
    }

    {
        char NumStr[] = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
        BigDec_Arena::parse_integer(NumStr, &A);
//        uint32 ValuesExpected[] {0xA763'FFFF, 0x0DE0'B6B3};
//        Expected.set(ValuesExpected, ArrayCount(ValuesExpected));
//        CheckA();
        cout << "\nEYEBALL TEST: 1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890 = " << string(A) << '\n';
    }
#if ACTIVATE_ALL_TESTS
    {
        char NumStr[] = "\
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\
123456789012345678901234\
";
        BigDec_Arena::parse_integer(NumStr, &A);
//        uint32 ValuesExpected[] {0xA763'FFFF, 0x0DE0'B6B3};
//        Expected.set(ValuesExpected, ArrayCount(ValuesExpected));
//        CheckA();
        cout << "\nEYEBALL TEST: \
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\
123456789012345678901234 = " << string(A) << '\n';
    }
#endif


    ////////////////////////

    {
        cout << "\n";
        cout << "\tTests #" << Tests.TestCount << " and " << (Tests.TestCount+1) << "\n";
        cout << "Parse 12345.6789012345f and compare to built_in representation\n";
        float Challenge = 12345.6789012345f;
        std::stringstream SS;
        SS << std::fixed << std::setprecision(20) << Challenge;
        std::string S = SS.str();
        char SBuf[64] = "";
        for (char *Src= &S[0], *Dst=SBuf ; *Src != '\0' ; ++Src, ++Dst) {
            *Dst = *Src;
        }
        BigDec_Arena::from_string(SBuf,&A);
        A.round_to_n_significant_bits(24);
        B.set_float(Challenge);
        Tests.Append(A.equals_fractional(B));
        float Parsed = A.to_float();
        Tests.Append(Parsed==Challenge);

        cout << std::fixed << std::setprecision(20) << Challenge << " -> from_string()" << "\n";
        cout << string(A) << "\n";
        cout << Challenge << " -> set_float()" << "\n";
        cout << string(B) << "\n";
        cout << "A.to_float() -> " << "\n";
        cout << Parsed << "\n";
        cout << "\n";
    }


    ////////////////////////


    {
        char DecimalStr[] = "1.0";
        float ExpectedFloat = 1.0f;
        BigDec_Arena::from_string(DecimalStr, &A);
        A.round_to_n_significant_bits(DOUBLE_PRECISION);
        Expected.set(0x1);
        PrintTestOutput(A,A,Expected,nullptr,"A       ","Expected", "Parse 1.0");
        Tests.Append(A.equals_fractional(Expected));

    }

    {
        char DecimalStr[] = "1.1";
        float ExpectedFloat = 1.1f;
        BigDec_Arena::from_string(DecimalStr, &A);
        A.round_to_n_significant_bits(DOUBLE_PRECISION);
        u32 ExpectedBits[] = {0xCCCCCCCD,0b1'0001'1001'1001'1001'100};
        Expected.set(ExpectedBits, ArrayCount(ExpectedBits),0,0);
        PrintTestOutput(A,A,Expected,nullptr,"A       ","Expected", "Parse 1.1");
        Tests.Append(A.equals_fractional(Expected));
    }

    {
        char DecimalStr[] = "1.1";
        double ExpectedDouble = 1.1;
        BigDec_Arena::from_string(DecimalStr, &A);
        A.round_to_n_significant_bits(DOUBLE_PRECISION);
        //1.0001100110011001100110011001100110011001100110011010e0
        u32 ExpectedBits[] = {0b10011001100110011001100110011010,0b100011001100110011001};
        Expected.set(ExpectedBits, ArrayCount(ExpectedBits),0,Log2I(1));
        Expected.normalize();
        PrintTestOutput(A,A,Expected,nullptr,"A       ","Expected", "Parse 1.1");
        Tests.Append(A.equals_fractional(Expected));
    }

    {
        char DecimalStr[] = "1.12";
        double ExpectedDouble = 1.12;
        BigDec_Arena::from_string(DecimalStr, &A);
            A.round_to_n_significant_bits(DOUBLE_PRECISION);
           //100011110101110000101 00011110101110000101000111101100
        u32 ExpectedBits[] = {0b00011110101110000101000111101100,0b100011110101110000101};
        Expected.set(ExpectedBits, ArrayCount(ExpectedBits),0,Log2I(1));
        Expected.normalize();
        PrintTestOutput(A,A,Expected,nullptr,"A       ","Expected", "Parse 1.12");
        Tests.Append(A.equals_fractional(Expected));
    }

    {
        char DecimalStr[] = "12.98";
        double ExpectedDouble = 12.98;
        BigDec_Arena::from_string(DecimalStr, &A);
        A.round_to_n_significant_bits(DOUBLE_PRECISION);
        //11001111101011100001010001111010111000010100011110110
        u32 ExpectedBits[] = {0b01000111101011100001010001111011,0b11001111101011100001};
        Expected.set(ExpectedBits, ArrayCount(ExpectedBits),0,Log2I(12));
        Expected.normalize();
        PrintTestOutput(A,A,Expected,nullptr,"A       ","Expected", "Parse 12.98");
        Tests.Append(A.equals_fractional(Expected));
    }

    {
        char DecimalStr[] = "1234.9876";
        double ExpectedDouble = 1234.9876;
        BigDec_Arena::from_string(DecimalStr, &A);
        A.round_to_n_significant_bits(DOUBLE_PRECISION);
        //1.0011010010111111001101001101011010100001011000011110e10
        u32 ExpectedBits[] = {0b10100110101101010000101100001111,0b10011010010111111001};
        Expected.set(ExpectedBits, ArrayCount(ExpectedBits),0,Log2I(1234));
        Expected.normalize();
        PrintTestOutput(A,A,Expected,nullptr,"A       ","Expected", "Parse 1234.9876");
        Tests.Append(A.equals_fractional(Expected));
    }




    {
        char DecimalStr[] = "123456789.987654321";

        double ExpectedDouble = 123456789.987654321;
        BigDec_Arena::from_string(DecimalStr, &A);
        A.round_to_n_significant_bits(DOUBLE_PRECISION);
        // 11101011011110011010001010111111100110101101110101000e26
        u32 ExpectedBits[] = {0b10001010111111100110101101110101,0b111010110111100110};
        Expected.set(ExpectedBits, ArrayCount(ExpectedBits),0,Log2I(123456789));
        Expected.normalize();
        PrintTestOutput(A,A,Expected,nullptr,"A       ","Expected", "Parse 123456789.987654321");
        Tests.Append(A.equals_fractional(Expected));

    }

    {
        char DecimalStr[] = "-123456789.987654321";

        double ExpectedDouble = -123456789.987654321;
        BigDec_Arena::from_string(DecimalStr, &A);
        A.round_to_n_significant_bits(DOUBLE_PRECISION);
        // 11101011011110011010001010111111100110101101110101000e26
        u32 ExpectedBits[] = {0b10001010111111100110101101110101,0b111010110111100110};
        Expected.set(ExpectedBits, ArrayCount(ExpectedBits),1,Log2I(123456789));
        Expected.normalize();
        PrintTestOutput(A,A,Expected,nullptr,"A       ","Expected", "Parse -123456789.987654321");
        Tests.Append(A.equals_fractional(Expected));

    }

    BigDec_Arena::close_context(true);

    cout << "|-> " << Tests << "\n\n";
    Tests.PrintResults();
    return Tests.FailCount;
}


template <typename T_Big_Dec>
void unit_test_context_variables_count(test_result& Tests, char *test_label, i32 expected_count){
    using std::cout;
    using std::string;
    bool OK = T_Big_Dec::s_ctx_count == expected_count;
    typename  T_Big_Dec::Link *link = T_Big_Dec::s_ctx_links;
    i32 link_cnt = 0;
    for (i32 i = 0 ; i < T_Big_Dec::s_ctx_count ; ++i) {
        if (!link) {
            OK = false;
            break;
        }
        ++link_cnt;
        link = link->next;
    }
    cout << "Test #" << Tests.TestCount << " - " << test_label << "\n";
    cout << "Big Decimal context variables \n";
    cout << "  stated : " << T_Big_Dec::s_ctx_count << "\n";
    cout << " counted : " << link_cnt << "\n";
    cout << "expected : " << expected_count << "\n";
    cout << T_Big_Dec::s_ctx_links << "\n";
    cout << (OK ? "PASSED" : "ERROR") << "\n\n";
    Tests.Append(OK);
}

int Test_new_allocator_interface() {

    using std::cout;
    using std::string;

    using ArenaChunkAlloc = ArenaAlloc<ChunkBits>;
    using BigDec_Arena = BigDecimal<ArenaChunkAlloc>;
    cout << __func__ << "\n\n";

    i32 FailCount { 0 };
    bool OK = false;

    test_result Tests {};

    {
        cout << "Test #" << Tests.TestCount << "\n";
        cout << "- BigDecimal::release(): length == 1, capacity == 1, data.next == 0, sentinel == &data, allocator.meta == nullptr \n";

        Big_Dec_Std::initialize_context();
        ChunkBits Bits[] = {1,2,3};
        Big_Dec_Std A;
        A.set(Bits,3);

        cout << "Before: " << A << "\n";
        A.release();

        OK = A.length == 1;
        OK &= A.m_chunks_capacity == 1;
        OK &= A.data.next == nullptr;
        OK &= A.sentinel == &A.data;

        cout << "After : " << A << "\n";
        cout << ( OK ? "OK" : "ERROR") << "\n";
        Tests.Append(OK);

        Big_Dec_Std::close_context();

    }

    {
        cout << "Test #" << Tests.TestCount << "\n";
        cout << "Initialize Context (arena1) -> Close Context () -> Initialize Context (arena2)\n";

        size_t size = Kilobytes(1);
        u8 * memory = new u8[size]();
        ArenaAlloc<ChunkBits> alloc_1{size, memory, deleter_std};
        memory = new u8[size]();
        ArenaAlloc<ChunkBits> alloc_2{size, memory, deleter_std};

        OK = true;
        OK &= alloc_1.meta->ref_count == 1;

        cout << "before 1st Init Ctx()  : alloc 1 #refs ==  1 ? " << (OK ? "OK" : "ERROR") << "\n";
        BigDec_Arena::initialize_context(alloc_1);
        //NOTE(ArokhSlade##2024 09 20): 1 here, 3 static arenas, one member per static variable
        i32 expected_ref_count = 1 + 3 + BigDec_Arena::s_ctx_count;
        OK &= alloc_1.meta->ref_count == expected_ref_count;
        cout << "after  1st Init Ctx()  : alloc 1 #refs == " << expected_ref_count << " ? " << (OK ? "OK" : "ERROR") << "\n";

        BigDec_Arena::close_context();
        OK &= alloc_1.meta->ref_count == 1;
        cout << "after  1st Close Ctx() : alloc 1 #refs ==  1 ? " << (OK ? "OK" : "ERROR") << "\n";
        OK &= BigDec_Arena::s_ctx_count == 0;
        cout << "                       : alloc 1 #ctx  ==  0 ? " << (OK ? "OK" : "ERROR") << "\n";

        OK &= alloc_2.meta->ref_count == 1;
        cout << "before 2nd Init Ctx()  : alloc 2 #refs ==  1 ? " << (OK ? "OK" : "ERROR") << "\n";
        BigDec_Arena::initialize_context(alloc_2);
        OK &= alloc_1.meta->ref_count == 1;
        cout << "after  2nd InitCtx()   : alloc 1 #refs ==  1 ? " << (OK ? "OK" : "ERROR") << "\n";
        OK &= alloc_2.meta->ref_count == expected_ref_count;
        cout << "after  2nd InitCtx()   : alloc 2 #refs == " << expected_ref_count << " ? " << (OK ? "OK" : "ERROR") << "\n";


        BigDec_Arena::close_context();
        OK &= alloc_2.meta->ref_count == 1;
        cout << "after  2st Close Ctx() : alloc 2 #refs ==  1 ? " << (OK ? "OK" : "ERROR") << "\n";
        OK &= BigDec_Arena::s_ctx_count == 0;
        cout << "                       : alloc 2 #ctx  ==  0 ? " << (OK ? "OK" : "ERROR") << "\n";


        cout << "Final Result: " << ( OK ? "OK" : "ERROR") << "\n";
        Tests.Append(OK);
    }

    {
        cout << "BigDecimal with std::allocator - mandatory call to initialize_context().\n";

        OK = Big_Dec_Std::s_ctx_count == 0;
        OK &= Big_Dec_Std::s_ctx_links == nullptr;
        cout << "Test #" << Tests.TestCount << "\n";
        cout << "Big_Dec_Std : no context variables for init : " << OK << "\n\n";
        Tests.Append(OK);

        Big_Dec_Std::initialize_context();

        OK = Big_Dec_Std::s_ctx_count == Big_Dec_Std::TEMPORARIES_COUNT;
        Big_Dec_Std::Link *link = Big_Dec_Std::s_ctx_links;
        for (i32 i = 0 ; i < Big_Dec_Std::s_ctx_count ; ++i) {
            if (!link) {
                OK = false;
                break;
            }
            link = link->next;
        }
        cout << "Test #" << Tests.TestCount << "\n";
        cout << "Big_Dec_Std : " <<  Big_Dec_Std::TEMPORARIES_COUNT << " context variables for init : " << OK << "\n\n";
        Tests.Append(OK);


        BigDecimal my_num = BigDecimal();

        unit_test_context_variables_count<Big_Dec_Std>(Tests, "user created context variable", Big_Dec_Std::TEMPORARIES_COUNT + 1);

        my_num.extend_length();
        my_num.data.next->value = static_cast<ChunkBits>(0x12345678);

        OK = true;
        OK &= my_num.m_chunks_capacity == 2;
        OK &= my_num.length == 2;
        OK &= my_num.data.next && my_num.data.next->value == static_cast<ChunkBits>(0x12345678);
        cout << "Test #" << Tests.TestCount << "\n";
        cout << "extend_length() -> new chunk allocated: " << (OK ? "OK" : "ERROR") << "\n\n";
        Tests.Append(OK);

        my_num.release();

        unit_test_context_variables_count<Big_Dec_Std>(Tests, "user released context variable", Big_Dec_Std::TEMPORARIES_COUNT);

        Big_Dec_Std::close_context();

        unit_test_context_variables_count<Big_Dec_Std>(Tests, "after closing context", 0);

        Tests.Append(OK);
    }

    {
        cout << "calling style with user-provided allocator param - arena allocator.\n";

        memory_index capacity = Kilobytes(1);
        u8 *memory = new u8[capacity]();
//        Arena_Allocator<ChunkBits> std_new_arena_alloc_1{capacity, memory};

        ArenaChunkAlloc std_new_arena_alloc_1{ capacity, memory, deleter_std};

        unit_test_context_variables_count<BigDec_Arena>(Tests, "Big Decimal with Arena before Init", 0);

        BigDec_Arena::initialize_context(std_new_arena_alloc_1);

        unit_test_context_variables_count<BigDec_Arena>(Tests, "Big Decimal with Arena after Init", BigDec_Arena::TEMPORARIES_COUNT);

        BigDec_Arena my_num{std_new_arena_alloc_1,1,false,0};

        unit_test_context_variables_count<BigDec_Arena>(Tests, "Big Decimal with Arena with 1 user variable in context", BigDec_Arena::TEMPORARIES_COUNT+1);

        my_num.extend_length();
        my_num.data.next->value = static_cast<ChunkBits>(0x12345678);

        bool OK = true;
        OK &= my_num.m_chunks_capacity == 2;
        OK &= my_num.length == 2;
        OK &= my_num.data.next && my_num.data.next->value == static_cast<ChunkBits>(0x12345678);

        cout << "Test #" << Tests.TestCount << "\n";
        cout << "capacity increased after extend_length(): " << (OK ? "OK" : "ERROR") << " ( " << my_num << " )\n\n";

        Tests.Append(OK);
    }

    {
        i32 old_ctx_count = BigDec_Arena::s_ctx_count;
        {
            f32 f1=3.f;
            f32 f2=10.f;

            size_t user_capacity = 512;
            u8 *user_memory = new u8[user_capacity]();
            ArenaAlloc<ChunkBits> std_new_arena_alloc_2{user_capacity, user_memory, deleter_std};

            i32 old_ctx_count = BigDec_Arena::s_ctx_count;
            BigDec_Arena A { std_new_arena_alloc_2, f1};;
            BigDec_Arena B { std_new_arena_alloc_2, f2};;
            A.add_fractional(B);

            BigDec_Arena Xpcd { std_new_arena_alloc_2, 10.f+3.f};;
            Xpcd.normalize();

            unit_test_context_variables_count<BigDec_Arena>(Tests, "Big Decimal (Arena) - 3 user variables called outside of context", old_ctx_count);

            OK = true;
            OK &= A.equals_fractional( Xpcd);
            cout << "Test #" << Tests.TestCount << " - " << f1 << " + " << f2 << " = \n";
            cout << string(A) << " ( " << string(Xpcd) << " ) \n\n";
            Tests.Append(OK);
        }

        {
            unit_test_context_variables_count<BigDec_Arena>(Tests, "Big Decimal (Arena) - new scope, not doing anything", old_ctx_count);

            BigDec_Arena::close_context();
            //TODO: free memory

            unit_test_context_variables_count<BigDec_Arena>(Tests, "Big Decimal (Arena) - same new scope, after closing context", 0);
        }

        {
            constexpr i32 MAX_STRING_LEN = 512;
            constexpr i32 MAX_STRING_COUNT = 128;
            size_t char_buf_size = sizeof(char) * MAX_STRING_LEN * MAX_STRING_COUNT;
            u8 *char_buf = new u8[char_buf_size]();
            ArenaAlloc<char> std_new_arena_alloc_3{char_buf_size, char_buf, deleter_std};

            constexpr i32 BIG_DEC_BUF_SIZE = Kilobytes(1);
            u8 *big_dec_buf = new u8[BIG_DEC_BUF_SIZE]();
            ArenaChunkAlloc std_new_arena_alloc_4{BIG_DEC_BUF_SIZE, big_dec_buf, deleter_std};

            BigDec_Arena::initialize_context(std_new_arena_alloc_4);

            BigDec_Arena my_num { std_new_arena_alloc_4, 0.f};;
            char *my_num_str = to_chars(my_num, std_new_arena_alloc_3);
            cout << my_num_str << "\n";

            char expected[] = "+00000000 [0]";
            i32 len = StringLength(expected);
            OK = true;
            for (i32 i = 0 ; i < len ; ++i) {
                OK &= expected[i] == my_num_str[i];
            }
            OK &= my_num_str[len] == '\0';

            cout << "Test #" << Tests.TestCount << " - AsString() to a different arena allocator\n";
            cout << (OK ? "PASSED" : "ERROR") << "\n\n";
            Tests.Append(OK);
        }

        {
            BigDec_Arena a{1};
            std::stringstream ss;
            ss << a;
            std::string s = ss.str();
            OK = true;
            ss.str("");
            ss << "+";
            for (int i = 0 ; i < sizeof(ChunkBits) * 2 - 1 ; ++i) {
                ss << '0';
            }
            ss << "1 E0[0]";

            std::string expected = ss.str();
            i32 len = expected.size();
            OK &= len == s.size();

            for (i32 i = 0 ; OK && i < len ; ++i) {
                OK &= s[i] == expected[i];
            }
            cout << "Tests #" << Tests.TestCount << " - operator string()\n";
            cout << ( OK ? "PASSED" : "ERROR" ) << "\n";
            cout << "result: " << s << "\n";
            cout << "expect: " << expected << "\n";
            cout << "direct: " << a << "\n";
            Tests.Append(OK);
        }

        {
            BigDec_Arena my_num {0.f};
            unit_test_context_variables_count<BigDec_Arena>(Tests, " - ctor(float) with default arena allocator\n", BigDec_Arena::TEMPORARIES_COUNT + 1);

            Big_Dec_Std::initialize_context();

            Big_Dec_Std my_num_std {1.f};
            unit_test_context_variables_count<Big_Dec_Std>(Tests, " - ctor(float) with default std::allocator\n", Big_Dec_Std::TEMPORARIES_COUNT + 1);
        }

        {
            OK = true;
            std::allocator<ChunkBits> std_alloc{};
            Big_Dec_Std A {std_alloc, 1, false, 0};
            Big_Dec_Std A_implicit {1.f};
            Big_Dec_Std B {std_alloc, 1, false, 0};
            Big_Dec_Std B_copy {std_alloc, A};



            OK &= A.equals_integer(A_implicit);
            OK &= B.equals_integer(A_implicit);
            OK &= B_copy.equals_integer(A_implicit);

            //NOTE(ArokhSlade##2024 09 04): cross-type operations not supported: B_copy is of different type than A_Arena
//            BigDec_Arena A_Arena {0.f};
//            OK &= B_copy.equals_integer(A_Arena);

            cout << "Test# " << Tests.TestCount << " - try out all constructors with std allocator\n";
            cout << ( OK ? "PASSED" : "ERROR" ) << "\n";

            Tests.Append(OK);
        }

        {
            //finished with all the tests
            BigDec_Arena::close_context(true);
            Big_Dec_Std::close_context(true);
        }
    }

    cout << "|-> " << Tests << "\n\n";
    Tests.PrintResults();
    return Tests.FailCount;
}

template<typename T_Slot>
void test_CopyBitsTo(bool& OK, bool& only_errors, test_result& Tests)
{
    using std::cout;
    using std::string;

    using ArenaChunkAlloc = ArenaAlloc<ChunkBits>;
    using BigDec_Arena = BigDecimal<ArenaChunkAlloc>;
    HardAssert(BigDec_Arena::s_is_context_initialized);

    using T_Chunk = ChunkBits;
    constexpr i32 chunk_bits = CHUNK_WIDTH;
    constexpr i32 top_bits = chunk_bits / 2 - 1;
    T_Chunk test_value = GetMaskBottomN<T_Chunk>(top_bits);

    BigDec_Arena in{test_value};

    constexpr i32 slots_per_chunk = sizeof(T_Chunk) / sizeof(T_Slot);
    constexpr i32 chunks_per_slot = sizeof(T_Slot) / sizeof(T_Chunk);
    constexpr i32 slot_bits = sizeof(T_Slot) * 8;
    constexpr i32 slot_count = (top_bits + slot_bits-1) / slot_bits;

    T_Slot slots[slot_count] {};
    in.copy_bits_to<T_Slot>(slots, slot_count);
    T_Slot exp[slot_count] {};
    T_Slot slot_mask = GetMaskBottomN<T_Slot>(slot_bits);
    for (i32 slot_idx = 0 ; slot_idx < slot_count-1 ; ++slot_idx) {
        exp[slot_idx] = slot_mask;
    }
    exp[slot_count-1] = GetMaskBottomN<T_Slot>(top_bits - (slot_count-1)*slot_bits);

    OK = true;
    for (i32 slot_idx = 0 ; slot_idx < slot_count ; ++slot_idx) {
        OK &= slots[slot_idx] == exp[slot_idx];
    }

    if (!only_errors || !OK) {
        cout << "Test #" << Tests.TestCount << "\n";
        cout << "copy bits to slots -> 1 less than half a chunkment's worth of 1's\n";
        cout << "in :  " << in  << "\n";
        cout << "out:  ";
        cout << std::hex << std::setw(sizeof(T_Slot)*2) << std::setfill('0');
        cout << (u64)slots[0];
        for (i32 slot_idx = 1 ; slot_idx < slot_count ; ++slot_idx) {
            cout << ", " << (u64)slots[slot_idx];
        }
        cout << "\n";

        cout << "exp ( ";
        cout << (u64)slots[0];
        for (i32 slot_idx = 1 ; slot_idx < slot_count ; ++slot_idx) {
            cout << ", " << (u64)slots[slot_idx];
        }
        cout << " )\n";

        cout << (OK ? "OK" : "ERROR") << "\n";
    }
    Tests.Append(OK);
    cout << std::dec;
}

template<typename T_Slot>
void test_CopyBitsTo_2(bool& OK, bool& only_errors, test_result& Tests, ChunkBits test_value)
{
    using std::cout;
    using std::string;

    using ArenaChunkAlloc = ArenaAlloc<ChunkBits>;
    using BigDec_Arena = BigDecimal<ArenaChunkAlloc>;
    HardAssert(BigDec_Arena::s_is_context_initialized);

    using T_Chunk = ChunkBits;
    constexpr i32 chunk_bits = CHUNK_WIDTH;
    i32 top_bits = BitScanReverse<ChunkBits>(test_value) + 1;

    BigDec_Arena in{test_value};

    constexpr i32 slots_per_chunk = sizeof(T_Chunk) / sizeof(T_Slot);
    constexpr i32 chunks_per_slot = sizeof(T_Slot) / sizeof(T_Chunk);
    constexpr i32 slot_bits = sizeof(T_Slot) * 8;
    i32 slot_count = (top_bits + slot_bits-1) / slot_bits;

    T_Slot *slots = new T_Slot[slot_count] {};

    in.copy_bits_to<T_Slot>(slots, slot_count);

    T_Slot *exp = new T_Slot[slot_count] {};
    T_Chunk chunk_mask = GetMaskBottomN<T_Chunk>(chunk_bits < slot_bits ? chunk_bits : slot_bits);
    for (i32 slot_idx = 0 ; slot_idx < slot_count ; ++slot_idx) {
        T_Chunk slot_value = chunk_mask & test_value;
        slot_value = SafeShiftRight(slot_value, slot_idx * slot_bits);
        exp[slot_idx] = static_cast<T_Slot>(slot_value);
        chunk_mask <<= slot_bits;
    }

    OK = true;
    for (i32 slot_idx = 0 ; slot_idx < slot_count ; ++slot_idx) {
        OK &= slots[slot_idx] == exp[slot_idx];
    }

    if (!only_errors || !OK) {
        cout << "Test #" << Tests.TestCount << "\n";
        cout << "copy bits to slots -> one value (function parameter)\n";
        cout << "in :  " << in  << "\n";
        cout << "out:  ";
        cout << std::hex << std::setw(sizeof(T_Slot)*2) << std::setfill('0');
        cout << (u64)slots[0];
        for (i32 slot_idx = 1 ; slot_idx < slot_count ; ++slot_idx) {
            cout << ", " << (u64)slots[slot_idx];
        }
        cout << "\n";

        cout << "exp ( ";
        cout << (u64)exp[0];
        for (i32 slot_idx = 1 ; slot_idx < slot_count ; ++slot_idx) {
            cout << ", " << (u64)exp[slot_idx];
        }
        cout << " )\n";

        cout << (OK ? "OK" : "ERROR") << "\n";
    }
    Tests.Append(OK);
    cout << std::dec;
}


template<typename T_Slot>
void test_CopyBitsTo_3(bool& OK, bool& only_errors, test_result& Tests, ChunkBits *chunks, i32 chunks_count)
{
    HardAssert(chunks_count >= 0);

    using std::cout;
    using std::string;

    using ArenaChunkAlloc = ArenaAlloc<ChunkBits>;
    using BigDec_Arena = BigDecimal<ArenaChunkAlloc>;
    HardAssert(BigDec_Arena::s_is_context_initialized);

    using T_Chunk = ChunkBits;
    i32 top_bits = BitScanReverse<ChunkBits>(chunks[chunks_count-1])+1;
    i32 chunk_bits = CHUNK_WIDTH * (chunks_count - 1) + top_bits;

    BigDec_Arena in{chunks, chunks_count};

    constexpr i32 slots_per_chunk = sizeof(T_Chunk) / sizeof(T_Slot);
    constexpr i32 chunks_per_slot = sizeof(T_Slot) / sizeof(T_Chunk);
    constexpr i32 slot_width = sizeof(T_Slot) * 8;
    i32 slot_count = DivCeil(chunk_bits, slot_width);

    T_Slot *slots = new T_Slot[slot_count] {};

    in.copy_bits_to<T_Slot>(slots, slot_count);

    T_Slot *exp = new T_Slot[slot_count] {};

    if (slots_per_chunk) {
        T_Chunk chunk_mask = 0x0;
        i32 total_slot_idx = 0;
        for (i32 chunk_idx = 0 ; chunk_idx < chunks_count ; ++chunk_idx) {
            chunk_mask = GetMaskBottomN<T_Chunk>(slot_width > CHUNK_WIDTH ? CHUNK_WIDTH : slot_width);
            for (i32 slot_idx = 0 ; slot_idx < slots_per_chunk ; ++slot_idx) {
                if (total_slot_idx == slot_count) break;

                exp[total_slot_idx] = chunk_mask & chunks[chunk_idx];
                ++total_slot_idx;
                chunk_mask = SafeShiftLeft(chunk_mask, slot_width);
            }
        }
    } else {
        i32 total_chunk_idx = 0;
        for (i32 slot_idx = 0 ; slot_idx < slot_count ; ++slot_idx) {
            for (i32 chunk_idx = 0 ; chunk_idx < chunks_per_slot ; ++chunk_idx) {
                if (total_chunk_idx == chunks_count) break;
                T_Slot shifted_chunk = chunks[total_chunk_idx];
                shifted_chunk = SafeShiftLeft(shifted_chunk, CHUNK_WIDTH * chunk_idx);
                exp[slot_idx] |= shifted_chunk;
                ++total_chunk_idx;
            }
        }
    }

    OK = true;
    for (i32 slot_idx = 0 ; slot_idx < slot_count ; ++slot_idx) {
        OK &= slots[slot_idx] == exp[slot_idx];
    }

    if (!only_errors || !OK) {
        cout << "Test #" << Tests.TestCount << "\n";
        cout << "copy bits to slots -> multiple chunkments\n";
        cout << "in :  " << in  << "\n";
        cout << "out:  ";
        cout << std::hex;
        cout << std::hex << std::setw(sizeof(T_Slot)*2);
        cout << (u64)slots[0];
        for (i32 slot_idx = 1 ; slot_idx < slot_count ; ++slot_idx) {
            cout << ", " << std::setw(sizeof(T_Slot)*2) << (u64)slots[slot_idx];
        }
        cout << "\n";

        cout << "exp ( ";
        cout << std::hex << std::setw(sizeof(T_Slot)*2);
        cout << (u64)exp[0];
        for (i32 slot_idx = 1 ; slot_idx < slot_count ; ++slot_idx) {
            cout << ", " << std::setw(sizeof(T_Slot)*2) << (u64)exp[slot_idx];
        }
        cout << " )\n";

        cout << (OK ? "OK" : "ERROR") << "\n";
    }
    Tests.Append(OK);
    cout << std::dec;
}


int Test_variable_chunkment_size(bool only_errors=false) {

    using std::cout;
    using std::string;

    using ArenaChunkAlloc = ArenaAlloc<ChunkBits>;
    using BigDec_Arena = BigDecimal<ArenaChunkAlloc>;
    cout << __func__ << "\n\n";

    i32 FailCount { 0 };
    bool OK = false;

    test_result Tests {};


    size_t size = Kilobytes(512);
    u8 * memory = new u8[size]();
    ArenaAlloc<ChunkBits> alloc{size, memory, deleter_std};
    BigDec_Arena::initialize_context(alloc);
    {

        cout << "Test #" << Tests.TestCount << "\n";

        BigDec_Arena A{};
        ChunkBits vals[] = {1,1};
        A.set(vals,2);

        i32 ctx_count = 0;
        ctx_count = BigDec_Arena::s_ctx_links->length();
        cout << ctx_count << "\n";

        BigDec_Arena B{2};

        ctx_count = BigDec_Arena::s_ctx_links->length();
        cout << ctx_count << "\n";

        BigDec_Arena C{A};
        C.mul_integer(B);

        ctx_count = BigDec_Arena::s_ctx_links->length();
        cout << ctx_count << "\n";

        cout << A << "\n";
        cout << B << "\n";
        cout << C << "\n";


        OK = true;
        Tests.Append(OK);
    }

    {
        cout << "Test #" << Tests.TestCount << "\n";
        cout << "Integer Addition" << Tests.TestCount << "\n";

        BigDec_Arena A{0xFF,false,0};
        BigDec_Arena B{0xFF,false,0};
        for (i32 i = 0 ; i < 1000 ; ++i) {
            A.add_integer_signed(B);
            cout << A << "\n";
        }

        OK = true;
        Tests.Append(OK);
    }

    {
        cout << "Test #" << Tests.TestCount << "\n";
        cout << "(eyeball test)" << "\n";

        BigDec_Arena A{0x10,false,0};
        BigDec_Arena B{0x2,false,0};
        for (i32 i = 0 ; i < 16; ++i) {
            A.mul_integer(B);
            cout << A << "\n";
        }
        OK = true;
//        Tests.Append(OK);
    }

    {
        ChunkBits vals[] {0x0d,0xf3,0x10};
        BigDec_Arena A{vals,3};
        BigDec_Arena B{0xd};
        BigDec_Arena C{A};
        C.sub_integer_unsigned_positive(B);
        ChunkBits exp[] {0x0, 0xf3, 0x10};
        BigDec_Arena E{exp,3};

        OK = C.equals_integer(E);

        if (!only_errors || !OK) {
            cout << "Test #" << Tests.TestCount << "\n";
            cout << "10 f3 0d - 0d = 10 f3 00?\n";
            cout << "  " << A << "\n";
            cout << "- " << B << "\n";
            cout << "= " << C << "\n";
            cout << "( " << E << " )\n";
            cout << (OK ? "OK" : "ERROR") << "\n";
        }

        Tests.Append(OK);
    }

    {
        ChunkBits values_A[] = {0x0,0x0,0x0,0x0,0x1};
        ChunkBits values_B[] = {-1,-1,-1,-1,};
        BigDec_Arena A{values_A, 5};
        BigDec_Arena B{values_B, 4};
        BigDec_Arena C{A};


        C.sub_integer_unsigned_positive(B);

        OK = true;
        i32 expected_length = 1;
        OK &= C.length == expected_length;

        if (!only_errors || !OK) {
            cout << "Test #" << Tests.TestCount << "\n";
            cout << " 1 0 0 0 0 - 0 9 9 9 = 1 (correctly updated length = 1) \n";
            cout << A << "\n";
            cout << B << "\n";
            cout << C << "\n";
            cout << "length = " << C.length << " ( Expected: " << expected_length << " )\n";
            cout << (OK ? "OK" : "ERROR") << "\n";
        }

        Tests.Append(OK);
    }

    {
        ChunkBits val_in = ~ChunkBits(0ull);
        BigDec_Arena in{val_in};

        ChunkBits values_exp[] = {ChunkBits(~1ull), 0x1};
        BigDec_Arena exp{values_exp, 2};

        BigDec_Arena out{in};
        i32 shift_amount = 1;

        out.shift_left(shift_amount);

        OK = true;

        OK &= out.equals_integer(exp);

        if (!only_errors || !OK) {
            cout << "Test #" << Tests.TestCount << "\n";
            cout << " ff..ff << 1 = 0001 ff..fe \n";
            cout << in << "\n";
            cout << out << "\n";
            cout << "( " << exp << " )\n";
            cout << (OK ? "OK" : "ERROR") << "\n";
        }

        Tests.Append(OK);
    }


    {
        ChunkBits values_in[] = {1,0,0,0,1};
        BigDec_Arena A{values_in, 5};
        i32 msb_exp = CHUNK_WIDTH * 4;
        i32 msb_out = A.get_msb();

        OK = true;

        OK &= msb_exp == msb_out;

        if (!only_errors || !OK) {
            cout << "Test #" << Tests.TestCount << "\n";
            cout << "001 000 000 000 001 -> Most Significant Bit (MSB)";
            cout << msb_out << "\n";
            cout << "( " << msb_exp << " )\n";
            cout << (OK ? "OK" : "ERROR") << "\n";
        }

        Tests.Append(OK);
    }


    {
        ChunkBits values_in[] = {1,0,0,0,1};
        BigDec_Arena A{values_in, 5};
        i32 wanted_idx = CHUNK_WIDTH * 4;
        bool bit_exp = true;
        bool bit_out = A.get_bit(wanted_idx);

        OK = true;

        OK &= bit_exp == bit_out;

        if (!only_errors || !OK) {
            cout << "Test #" << Tests.TestCount << "\n";
            cout << "001 000 000 000 001 -> get_bit() at last (lowest) index: " << wanted_idx << "\n";
            cout << bit_out << "\n";
            cout << "( " << bit_exp << " )\n";
            cout << (OK ? "OK" : "ERROR") << "\n";
        }

        Tests.Append(OK);
    }


    {
        ChunkBits values_in[] = {0,0,0,0,1};
        BigDec_Arena in{values_in, 5};
        BigDec_Arena out{in};
        ChunkBits values_exp[] = {0,0,0,1,0};
        BigDec_Arena exp{values_exp, 5};
        i32 shift_amount = CHUNK_WIDTH;

        out.shift_right(shift_amount);

        OK = true;
        OK &= out.equals_integer(exp);

        if (!only_errors || !OK) {
            cout << "Test #" << Tests.TestCount << "\n";
            cout << "001 000 000 000 000 -> shift right by " << shift_amount << "\n";
            cout << "  " << in << "\n";
            cout << "  " << out<< "\n";
            cout << "( " << exp << " )\n";
            cout << (OK ? "OK" : "ERROR") << "\n";
        }

        Tests.Append(OK);
    }

    {
        ChunkBits values_in[] = {0,0,0,0,1};
        BigDec_Arena in{values_in, 5};
        BigDec_Arena out{in};
        BigDec_Arena exp{1};
        i32 shift_amount = CHUNK_WIDTH*4;

        out.shift_right(shift_amount);

        OK = true;
        OK &= out.equals_integer(exp);

        if (!only_errors || !OK) {
            cout << "Test #" << Tests.TestCount << "\n";
            cout << "001 000 000 000 000 -> shift right by " << shift_amount << "\n";
            cout << "  " << in << "\n";
            cout << "  " << out<< "\n";
            cout << "( " << exp << " )\n";
            cout << (OK ? "OK" : "ERROR") << "\n";
        }

        Tests.Append(OK);
    }

    {
        u8 out[2] = {};
        u64 out64[2] = {};
        u64 exp[2] = {0x28,0x8};

        FullMulN<u8>(0x9, 0xe8, out);
        for (int i = 0 ; i < 2 ; ++i) {
            out64[i] = (u64)out[i];
        }

        OK = true;
        OK &= out64[0]==exp[0];
        OK &= out64[1]==exp[1];

        if (!only_errors || !OK) {
            cout << "Test #" << Tests.TestCount << "\n";
            cout << "FullMulN<u8>(0x9, 0xe8, out) == lo:0x28,hi:0x8 ?\n";
            cout << "  " << Array<u64>(out64,2) << "\n";
            cout << "( " << Array<u64>(exp,2) << " )\n";
            cout << (OK ? "OK" : "ERROR") << "\n";
        }
        Tests.Append(OK);
    }



    {
        char in[] = "256";
        BigDec_Arena out{};
        u8 exp_val[] = {0,1};
        BigDec_Arena exp{exp_val,2};

        BigDec_Arena::parse_integer(in, &out);

        OK = true;
        OK &= out.equals_integer(exp);

        if (!only_errors || !OK) {
            cout << "Test #" << Tests.TestCount << "\n";
            cout << "parse_integer( " << in << " )\n";
            cout << "  " << out<< "\n";
            cout << "( " << exp << " )\n";
            cout << (OK ? "OK" : "ERROR") << "\n";
        }

        Tests.Append(OK);
    }


    {
        char in[] = "99999999";
        BigDec_Arena out{};
        u8 exp_val[] = {0xFF, 0xE0, 0xF5, 0x5};
        BigDec_Arena exp{exp_val,ArrayCount(exp_val)};

        BigDec_Arena::parse_integer(in, &out);

        OK = true;
        OK &= out.equals_integer(exp);

        if (!only_errors || !OK) {
            cout << "Test #" << Tests.TestCount << "\n";
            cout << "parse_integer( " << in << " )\n";
            cout << "  " << out<< "\n";
            cout << "( " << exp << " )\n";
            cout << (OK ? "OK" : "ERROR") << "\n";
        }
        Tests.Append(OK);
    }

    {
        ChunkBits chunks[] { -1, -1, -1, -1 };
        BigDec_Arena in{chunks, ArrayCount(chunks),false,0};
        bool out = false;
        bool exp = true;
        i32 search_idx = 24;
        out = in.get_bit(search_idx);


        OK = true;
        OK &= out == exp;

        if (!only_errors || !OK) {
            cout << "Test #" << Tests.TestCount << "\n";
            cout << search_idx << "-th bit of " << in << " , counting from front\n";
            cout << "in :  " << in  << "\n";
            cout << "out:  " << out << "\n";
            cout << "exp ( " << exp << " )\n";
            cout << (OK ? "OK" : "ERROR") << "\n";
        }
        Tests.Append(OK);
    }


    {
        test_CopyBitsTo<u8>(OK, only_errors, Tests);
        test_CopyBitsTo<u16>(OK, only_errors, Tests);
        test_CopyBitsTo<u32>(OK, only_errors, Tests);
        test_CopyBitsTo<u64>(OK, only_errors, Tests);
    }

    {
        ChunkBits test_value = SafeShiftLeft(1,CHUNK_WIDTH-1) | 1;
        test_CopyBitsTo_2<u8>(OK, only_errors, Tests , test_value);
        test_CopyBitsTo_2<u16>(OK, only_errors, Tests, test_value);
        test_CopyBitsTo_2<u32>(OK, only_errors, Tests, test_value);
        test_CopyBitsTo_2<u64>(OK, only_errors, Tests, test_value);
    }

    {
        ChunkBits test_chunks[] = {1,1,1,1,1};

        test_CopyBitsTo_3<u8>(OK, only_errors, Tests , test_chunks, ArrayCount(test_chunks));
        test_CopyBitsTo_3<u16>(OK, only_errors, Tests, test_chunks, ArrayCount(test_chunks));
        test_CopyBitsTo_3<u32>(OK, only_errors, Tests, test_chunks, ArrayCount(test_chunks));
        test_CopyBitsTo_3<u64>(OK, only_errors, Tests, test_chunks, ArrayCount(test_chunks));
    }

    {
        using T_Slot = u32;
        ChunkBits chunks[] { -1, -1, -1, -1 };
        constexpr i32 chunk_count = ArrayCount(chunks);
        BigDec_Arena in{chunks, chunk_count, false,0};
        constexpr i32 slots_per_chunk = sizeof(chunks[0]) / sizeof(T_Slot);
        constexpr i32 chunks_per_slot = sizeof(T_Slot) / sizeof(chunks[0]);
        constexpr i32 slot_count = chunks_per_slot ? (chunk_count + chunks_per_slot - 1) / chunks_per_slot : chunk_count * slots_per_chunk;
        T_Slot slots[slot_count] {};

        in.copy_bits_to<T_Slot>(slots, slot_count);

        OK = true;
        for (i32 slot_idx = 0 ; slot_idx < slot_count ; ++slot_idx) {
            OK &= slots[slot_idx] == -1;
        }

        if (!only_errors || !OK) {
            cout << "Test #" << Tests.TestCount << "\n";
            cout << "copy bits to slots -> u32\n";
            cout << "in :  " << in  << "\n";
            cout << "out:  ";
            cout << std::hex << std::setw(8) << std::setfill('0');
            cout << slots[0];
            for (i32 slot_idx = 1 ; slot_idx < slot_count ; ++slot_idx) {
                cout << ", " << slots[slot_idx];
            }
            cout << "\n";

            cout << "exp ( ";
            cout << std::hex << std::setw(8) << std::setfill('0');
            cout << (T_Slot)-1;
            for (i32 slot_idx = 1 ; slot_idx < slot_count ; ++slot_idx) {
                cout << ", " << (T_Slot)-1;
            }
            cout << " )\n";

            cout << (OK ? "OK" : "ERROR") << "\n";
        }
        Tests.Append(OK);
        cout << std::dec;
    }

    {
        using T_Slot = u64;
        ChunkBits chunks[] { ~0ull, ~0ull, ~0ull, ~0ull };
        constexpr i32 chunk_count = ArrayCount(chunks);
        BigDec_Arena in{chunks, chunk_count, false,0};
        constexpr i32 slots_per_chunk = sizeof(chunks[0]) / sizeof(T_Slot);
        constexpr i32 chunks_per_slot = sizeof(T_Slot) / sizeof(chunks[0]);
        i32 slot_count = chunks_per_slot ? DivCeil(chunk_count,chunks_per_slot) : chunk_count * slots_per_chunk;

        T_Slot out[slot_count] {};

        T_Slot exp[slot_count] {};
        for (i32 i_slot = 0 ; i_slot < slot_count-1 ; ++i_slot) {
            exp[i_slot] = ~0ull;
        }
        i32 src_bytes = chunk_count * sizeof(ChunkBits);
        i32 bytes_left = src_bytes - (slot_count-1)*sizeof(T_Slot);
        T_Slot mask = GetMaskBottomN<T_Slot>(8*bytes_left);
        exp[slot_count-1] = mask;

        in.copy_bits_to<T_Slot>(out, slot_count);

        OK = true;
        OK &= (sizeof(u64)*(slot_count - 1)) / (sizeof(ChunkBits) * chunk_count) == 0;

        for (i32 slot_idx = 0 ; slot_idx < slot_count ; ++slot_idx) {
            OK &= out[slot_idx] == exp[slot_idx];
        }

        if (!only_errors || !OK) {
            cout << "Test #" << Tests.TestCount << "\n";
            cout << "copy bits to out -> u64\n";
            cout << "in :  " << in  << "\n";
            cout << "out:  ";
            cout << std::hex << std::setw(16) << std::setfill('0');
            cout << out[0];
            for (i32 slot_idx = 1 ; slot_idx < slot_count ; ++slot_idx) {
                cout << ", " << out[slot_idx];
            }
            cout << "\n";

            cout << "exp ( ";
            cout << std::setw(16) << std::setfill('0');
            cout << exp[0];
            for (i32 slot_idx = 1 ; slot_idx < slot_count ; ++slot_idx) {
                cout << ", " << exp[slot_idx];
            }
            cout << " )\n";

            cout << (OK ? "OK" : "ERROR") << "\n";
        }
        Tests.Append(OK);
        cout << std::dec;
    }


    {
        float in { 0x1.000002p+0};
        BigDec_Arena out{};
        BigDec_Arena exp{0x800001, false, 0};

        out.set_float(in);

        OK = true;
        OK &= out.equals_fractional(exp);


        if (!only_errors || !OK) {
            cout << "Test #" << Tests.TestCount << "\n";
            cout << std::hex;
            cout << "set_float( " << in << " )\n";
            cout << "out:  " << out << "\n";
            cout << "exp:( " << exp << " )\n";
            cout << (OK ? "OK" : "ERROR") << "\n";
        }
        Tests.Append(OK);
        cout << std::dec;
    }

    {
        ChunkBits values[] {0x0, 0x1, 0x2, 0x3};
        BigDec_Arena a{alloc, &values[0], ArrayCount(values)};
        if (!only_errors || !OK) {
            cout << "test #" << Tests.TestCount << " : ctor(allocator, array) \n";
        }
        OK = true;
        OK &= a.length == ArrayCount(values);
        BigDec_Arena::ChunkList *cur = &a.data;
        for (i32 i = 0 ; i < a.length ; ++i) {
            OK &= cur->value == values[i];
            cur = cur->next;
        }

        cout << (OK ? "OK" : "ERROR") << " : " << a << "\n";
        Tests.Append(OK);


    }

    {
        f32 in = 3/7.f;
        BigDec_Arena a{in};
        f32 out = a.to_float();
        OK = true;
        OK &= in == out;

        if (!only_errors || !OK) {
            cout << "test #" << Tests.TestCount << " : ctor(float) \n";
            cout << "in : " << in << " , out : " << out << "\n";
            cout << (OK ? "OK" : "ERROR") << " : " << a << "\n";
        }
        Tests.Append(OK);
    }


    {
        f64 in = 2./3.;
        BigDec_Arena a{in};
        f64 out = a.to_double();
        OK = true;
        OK &= in == out;

        if (!only_errors || !OK) {
            cout << "test #" << Tests.TestCount << " : ctor(double) \n";
            std::printf("in  : %.16f\n", in);
            std::printf("out : %.16f\n", out);
            cout << (OK ? "OK" : "ERROR") << " : " << a << "\n";
        }
        Tests.Append(OK);

    }

    {
        f64 in = std::sqrt(2.);
        BigDec_Arena a{in};
        f64 out = a.to_double();
        OK = true;
        OK &= in == out;

        if (!only_errors || !OK) {
            cout << "test #" << Tests.TestCount << " : ctor(double) \n";
            std::printf("in  : %.16f\n", in);
            std::printf("out : %.16f\n", out);
            cout << (OK ? "OK" : "ERROR") << " : " << a << "\n";
        }
        Tests.Append(OK);

    }

    {
        u64 in[] {0x0,0x0,0x1};
        BigDec_Arena out{in, ArrayCount(in)};
        i32 expected_length = (sizeof(u64) / sizeof(ChunkBits)) * 2 + 1;
        OK = true;
        OK &= out.length == expected_length;

        if (!only_errors || !OK) {
            cout << "test #" << Tests.TestCount << " : initialize from array but don't copy leading zeroes \n";
            std::printf("in  : (hex){%#016llx, %016llx, %016llx}\n", in[0], in[1], in[2]);
            cout << out << "\n";
            std::printf("length  : %d\n", out.length);
            std::printf("(expect : %d)\n", expected_length);
        }

        Tests.Append(OK);
    }


    BigDec_Arena::close_context(true);

    cout << "|-> " << Tests << "\n\n";
    Tests.PrintResults();
    return Tests.FailCount;
}



int main() {
    i32 FailCount = 0;
    FailCount += Test_big_decimal();
    FailCount += Test_Parsing();
    FailCount += Test_new_allocator_interface();
    FailCount += Test_variable_chunkment_size(false);

    return FailCount;
}
