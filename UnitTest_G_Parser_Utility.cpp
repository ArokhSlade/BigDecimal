#include "G_Essentials.h"
#include "G_Miscellany_Utility.h"
#include "G_MemoryManagement_Service.h"
#include "G_PlatformGame_2_Module.h"
#include "G_Parser_Utility.h"

#include<iostream>
#include <sstream>
#include <iomanip>
#include "Memoryapi.h" //windows: VirtualAlloc
#include <fstream>


namespace unit_test {

const char PASS[] = "PASS";
const char FAIL[] = "FAIL";


auto operator<<(std::ostream& Out, parse_info& Info) -> std::ostream& {
    const char *WIN = "WIN";
    const char *FAIL = "FAIL";
    const char *Msg = Info ? WIN : FAIL;
    return Out<<Msg;
}



int32 Test_ParseInt32() {

    std::cout << __func__ << "\n\n";

    int32 FailCount= 0;

    int32 Buf[1];

    {
        std::cout << "Should succeed ... \n";
        const char Input[][64] = {  "1",     "0",       "-1",       "123",
                                    "-456",  "7E+1",    "89e-1",    "0e0",
                                    "2E-200","   +123", "   456e2"
                                 };
        int32 Expected[ArrayCount(Input)] = { 1,  0, -1, 123,
                                              -456, (int32)7E+1, (int32)89e-1, (int32)0e0,
                                              (int32)2E-200, 123, (int32)456e2
                                            };
        for (size_t i = 0 ; i < ArrayCount(Input) ; ++i) {
            parse_info Info = ParseInt32(static_cast<const char *>(Input[i]),Buf);
            bool32 Passed = Info && Buf[0] == Expected[i];
            const char *CheckMsg = Passed ? unit_test::PASS  : unit_test::FAIL;
            std::cout << CheckMsg << " ( " << Info << " ) : " << Input[i] << " -> " << Buf[0] << '\n';
            if (!Passed) ++FailCount;
        }
    }

    {
        std::cout << "Should fail ... \n";
        const char Input[][64] = { "0e","123123123123", "-2e10"};
        for (size_t i = 0 ; i < ArrayCount(Input) ; ++i) {
            parse_info Info = ParseInt32(static_cast<const char *>(Input[i]),Buf);
            bool32 Passed = !Info;
            const char *CheckMsg = Passed ? unit_test::PASS : unit_test::FAIL;
            std::cout << CheckMsg << " ( " << Info << " ) : " << Input[i] << " -> " << Buf[0] << '\n';
            if (!Passed) ++FailCount;
        }
    }

    std::cout << std::endl;

    return FailCount;
}

//TODO(## 2023 11 16):test inf and -inf
int32 Test_ParseIntegralPart() {

    std::cout << __func__ << "\n\n";
    std::cout << "PASS? | Parse Status | Input | Parsed | Expected " << '\n';

    int32 FailCount= 0;

    real32 Buf[1] = { +1.234f }; //arbitrary positive value

    {
        std::cout << "\nShould succeed ... \n";
        const char Input[][64] = {
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
            Buf[0] = 0.f;
            parse_info Info = ParseIntegralPart((Input[i]),Buf);
            bool32 Passed = Info && Buf[0] == Expected[i];
            const char *CheckMsg = Passed ? unit_test::PASS  : unit_test::FAIL;
            std::cout << CheckMsg << " ( " << Info << " ) : " << Input[i] << " -> " << Buf[0] << " ( " << Expected[i] << " )" << '\n';
            if (!Passed) ++FailCount;
        }
    }

    {
        std::cout << "\nShould succeed ... \n";
        std::cout << "\n(Output Buffer initialized to < 0 => parse results <0)\n" << '\n';
        real32 NegativeInit = -.98765f; //arbitrary negative value
        const char Input[][64] = {
            "1",  "123",  "999",  "0"
        };
        real32 Expected[ArrayCount(Input)] = {
            -1.f, -123.f, -999.f, -0.f
            };

        for (size_t i = 0 ; i < ArrayCount(Input) ; ++i) {
            Buf[0] = NegativeInit;
            parse_info Info = ParseIntegralPart((Input[i]),Buf);
            bool32 Passed = Info && Buf[0] == Expected[i];
            const char *CheckMsg = Passed ? unit_test::PASS  : unit_test::FAIL;
            std::cout << CheckMsg << " ( " << Info << " ) : " << Input[i] << " -> " << Buf[0] << " ( " << Expected[i] << " )" << '\n';
            if (!Passed) ++FailCount;
        }
    }

    {
        std::cout << "\nShould fail ... \n";

        const char Input[][64] = { ".0","+1e10", "-2e10", "abc", "lOS"};
        for (size_t i = 0 ; i < ArrayCount(Input) ; ++i) {
            parse_info Info = ParseIntegralPart((Input[i]),Buf);
            const char PASS[] = "PASS";
            const char FAIL[] = "FAIL";
            bool32 Passed = !Info;
            const char *CheckMsg = Passed ? PASS : FAIL;
            std::cout << CheckMsg << " ( " << Info << " ) : " << Input[i] << " -> " << Buf[0] << '\n';
            if (!Passed) ++FailCount;
        }
    }

    std::cout << std::endl;

    return FailCount;
}


//TODO(ArokhSlade##2024 08 13): this is copy-pasta from UnitTest_G_PlatformGame_2_Module.cpp. extract!
struct expected_big_int {
    uint32 Length;
    uint32 *Values;
    bool IsNegative;
};

//TODO(ArokhSlade##2024 08 13): this is copy-pasta from UnitTest_G_PlatformGame_2_Module.cpp. extract!
typedef uint32 success_bits;
struct test_result {
    //TODO(##2024 05 04): this limit is arbitrary. if you run out of tests cases, increase it.
    static const uint32 MaxTestWords = 8;
    uint32 TestCount;
    int32 FailCount;
    success_bits SuccessBits[MaxTestWords];

    //TODO(## 2023 11 19): maybe "CheckAndAppend" ?
    void Append(bool32 Condition) {
        if (Condition) {
            SetBit32(SuccessBits, MaxTestWords, TestCount);
        }
        ++TestCount;
        FailCount += !Condition;
    }

    //TODO(## 2023 11 19): why a different name than the one taking a condition?
    template<typename t_allocator>
    static bool32 Check(const big_int<t_allocator>& BigInt, const expected_big_int& Expected) {
        bool32 Result =
            ( BigInt.Length == Expected.Length
            && BigInt.IsNegative == Expected.IsNegative
            && [&]() { //TODO (## 2023 11 19): implement this in terms of "map(T* Begin, size_t Length, T* Next(T*), x->f(x))", i.e. algorithm+iterator
                bool32 AllValuesEqual = true;
                uint32 Idx = 0;
                uint32 *ExpectedVal = Expected.Values;
                const typename big_int<t_allocator>::chunk_list *Actual = &BigInt.Data;
                for (
                    ; Idx < Expected.Length
                    ; ++Idx, Actual = Actual->Next, ++ExpectedVal) {
                    //TODO(## 2023 11 19): ensure there are enough Blocks, i.e. no nullptrs before Idx==Length
                    AllValuesEqual &= Actual->Value == *ExpectedVal;
                }
                return AllValuesEqual;
            }() );

        return Result;
    }

    void PrintResults() {
        for (uint32 TestIdx = 0 ; TestIdx < TestCount ; ++TestIdx ) {
            bool32 PassedCurrent = GetBit32(SuccessBits, MaxTestWords, TestIdx);
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
auto Print(big_int<arena_allocator>& A, memory_arena *StringArena) -> void {
    memory_arena *TempArena = PushArena(StringArena);
    std::cout << Str(A, TempArena) << "\n";
    PopArena(TempArena);
}


//TODO(ArokhSlade##2024 08 13): this is copy-pasta from UnitTest_G_PlatformGame_2_Module.cpp. extract!
int32 Test_BigInt() {
    using std::cout;

    int32 FailCount { 0 };

    cout << __func__ << '\n';

    test_result Tests {};

    memory_index ArenaSize = 256*32768*sizeof(uint32);
    uint8 *ArenaBase = (uint8*)VirtualAlloc(0, ArenaSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    memory_arena MyArena;
    InitializeArena(&MyArena, ArenaSize, ArenaBase);

    memory_index StringArenaSize = 4096 * sizeof(char);
    uint8 *StringArenaBase = (uint8*)VirtualAlloc(0, StringArenaSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    memory_arena StringArena;
    InitializeArena(&StringArena, StringArenaSize, StringArenaBase);

    arena_allocator MyArenaAllocator { &MyArena };
    big_int<arena_allocator>::InitializeContext (&MyArenaAllocator);

    big_int<arena_allocator> A {MyArenaAllocator, 9};
    big_int<arena_allocator> OldA {MyArenaAllocator, A};
    big_int<arena_allocator> B {MyArenaAllocator, 10};

    big_int<arena_allocator> Expected (MyArenaAllocator, 0);

    auto CheckA = [&](){
        Tests.Append(Equals(A,Expected)); };

    auto PrintTestOutput = [&Tests](big_int<arena_allocator>& Before, big_int<arena_allocator>& After, big_int<arena_allocator>& Wanted,
                                    char *BeforeName, char *AfterName, char*WantedName, char *TestName){
        cout << "Test #" << Tests.TestCount;
        if (TestName)   cout << " : " << TestName;
                        cout << "\n";
        if (BeforeName) cout << BeforeName << " = " << Before.to_string() << "\n" ;
        if (AfterName)  cout << AfterName << " = " << After.to_string() << "\n" ;
        if (WantedName) cout << WantedName << " = " << Wanted.to_string() << "\n" ;
    };
    char AName[] = "       A";
    char OldAName[] = "   Old A";
    char ExpectedName[] = "Expected";
    auto PrintTestA = [&](char *TestName){PrintTestOutput(OldA, A, Expected, OldAName, AName, ExpectedName, TestName);};

    Tests.Append(A.Length >= 1 && A.GetChunk(0)->Value == int32{9});
    Print(A,&StringArena);

    Tests.Append(B.Length >= 1 && B.GetChunk(0)->Value == int32{10});
    Print(B,&StringArena);

    B.Add(A);
    Tests.Append(B.Length == 1 && B.GetChunk(0)->Value == 19U);
    Print(B,&StringArena);

    A.Neg();
    Tests.Append(A.Length == 1 && A.GetChunk(0)->Value == 9U && A.IsNegative);
    Print(A,&StringArena);

    A.Set(1,0,0);
    B.Set(2,1,1);
    Print(A,&StringArena);
    Print(B,&StringArena);
    Tests.Append(true == LessThan<arena_allocator>(Abs<arena_allocator>(&A),Abs<arena_allocator>(&B)));
    Tests.Append(false == LessThan<arena_allocator>(Abs<arena_allocator>(&B),Abs<arena_allocator>(&A)));

    B.Neg();
    Print(B,&StringArena);
    Tests.Append(true == LessThan<arena_allocator>(&A,&B));
    Tests.Append(false == LessThan<arena_allocator>(&B,&A));

    //trying to get non-existent chunks will return a common fallback
    Tests.Append(B.GetChunk(1) == B.GetChunk(12345) && B.GetChunk(1) == nullptr);
    Print(B,&StringArena);



    //Add(+,+) 1,1 -> 2 blocks
    A.Data.Value = MAX_UINT32;
    B.Data.Value = 1U;
    A.Add(B);
    {
        // 0xFFFFFFFF + 1 = 0x1'0000'0000
        uint32 ExpectedValues[] = {0,1};
        Tests.Append(Tests.Check(A, {ArrayCount(ExpectedValues), ExpectedValues, 0}));
    }


    //Add(+,+) 2,1 -> 2 blocks
    A.Add(B);
    {
        // 0x1'0000'0000 + 1 = 0x1'0000'0001
        uint32 ExpectedValues[] = {1,1};
        Tests.Append(Tests.Check(A, {ArrayCount(ExpectedValues), ExpectedValues, 0}));
    }

    //Add(+,+) 1,2 -> 2 blocks
    B.Add(A);
    {
        // 1 + 0x1'0000'0001 = 0x1'0000'0002
        uint32 Expected[] = {2,1};
        Tests.Append(Tests.Check(B, {ArrayCount(Expected), Expected, 0}));
    }


    {
        //Add(+,+) 1,1 -> 2 blocks
        uint32 AVals[] = {0xFFFFFFFE};
        A.Set(AVals, 1);
        B.Set(0x4,0,0);
        Expected.Set(2,0,0);
        Expected.Push(1);
        A.Add(B);
        CheckA();
    }

    //Add(+,-)
    A.Set(0,0,0);
    B.Set(1,0,0);
    B.Neg();
    A.Add(B);
    {
        // 0 + -1 = -1
        uint32 Expected[] = {1};
        Tests.Append(test_result::Check(A, {ArrayCount(Expected), Expected, 1}));
        Tests.Append(test_result::Check(B, {ArrayCount(Expected), Expected, 1}));
    }



    //Add(-,+)
    A.Set(1,1,0);
    B.Set(2,0,0);
    A.Add(B);
    Expected.Set(1,0,0);
    Tests.Append(Equals(A, Expected));

    //Add(-,-)
    A.Set(2,1,0);
    B.Set(2,1,0);
    A.Add(B);
    Expected.Set(4,1,0);
    Tests.Append(Equals(A,Expected));


    {
    //Add(-,-) Length 1->2
        uint32 Vals[] = {0xFFFFFFFF};
        A.Set(Vals, 1);

        A.Set(1,1,0);
        B.Set(Vals,1);
        B.Neg();
        A.Add(B);
        Expected.Set(0,0,0).Push(1);
        Expected.Neg();
        CheckA();
    }


    //Sub(+,+) >0 -> >0
    A.Set(123,0,Log2I(123));
    B.Set(23,0,Log2I(23));
    A.Sub(B);
    {
        // 123-23 = 100
        uint32 ExpectedValues[] = {100};
        Tests.Append(Tests.Check(A, {ArrayCount(ExpectedValues), ExpectedValues, 0}));
    }

    //Sub(+,+) >0 to <0
    A.Set(0,0,0);
    B.Set(1,0,0);
    A.Sub(B);
    {
        // 0 - 1 = -1
        uint32 Expected[] = {1};
        Tests.Append(test_result::Check(A, {ArrayCount(Expected), Expected, 1}));
    }

    {
        //Sub(+,-)
        cout << "Sub(+,-), Test# " << Tests.TestCount << "...\n";
        A.Set(1,0,0);
        B.Set(1,1,0);
        Expected.Set(2,0,0);
        A.Sub(B);
        CheckA();
    }

    {
        //Sub (x,x) == 0 for long chains
        cout << "Test# " << Tests.TestCount << " : A.Sub(B) == 0 (where A==B) for long chains ...\n";
        uint32 ValuesA[] { 0x0000'0000, 0x0000'0000, 0x0000'0000, 0x8000'0000 };
        A.Set(ValuesA,ArrayCount(ValuesA));
        B.Set(ValuesA,ArrayCount(ValuesA));
        A.Sub(B);
        Expected.Set(0,0,0);
        CheckA();

        cout << "Test# " << Tests.TestCount << " : A.Sub(A) == 0 for long chains ...\n";
        A.Set(ValuesA,ArrayCount(ValuesA));
        A.Sub(A);
        Expected.Set(0,0,0);
        Tests.Append(IsNormalizedZero(A));

        cout << "Test# " << Tests.TestCount << " : Sub for long chains changes the Length and uses carries ...\n";
        A.Set(ValuesA,ArrayCount(ValuesA));
        uint32 ValuesB[] { 0x0000'0000, 0x8000'0000, 0xFFFF'FFFF, 0x7FFF'FFFF };
        B.Set(ValuesB, ArrayCount(ValuesB));
        A.Sub(B);
        uint32 ValuesExpected[] { 0x0000'0000, 0x8000'0000 };
        Expected.Set(ValuesExpected,ArrayCount(ValuesExpected));
        CheckA();
    }

    //Sub(-,+) Length 1->2
    A.Set(2,1,0);
    uint32 Vals[] = {0xFFFFffff};
    B.Set(Vals, 1);
    A.Sub(B);
    Expected.Set(1,1,0).Push(1);
    Tests.Append(Equals(A, Expected));

    //Sub(-,-)
    A.Set(2,1,0);
    B.Set(1,1,0);
    A.Sub(B);
    Expected.Set(1,1,0);
    CheckA(); //#25

    B.Set(2,1,0);
    A.Sub(B);
    Expected.Set(1,0,0);
    CheckA();


    A.Set(1,0,0);
    A.Add(A);
    Expected.Set(2,0,0);
    CheckA();

    A.Set(1,0,0);
    A.Sub(A);
    Expected.Set(0,0,0);
    CheckA();

    {
    //#29
    A.Set(0,0,0).Push(1);
    uint32 Vals[] = {0xFFFFFFFF};
    Expected.Set(Vals,1);
    B.Set(1,0,0);
    Expected.Add(B);
    CheckA();
    }

    //test that Add won't allocate chunks when there are enough.
    {
        auto X = big_int<arena_allocator>(&MyArena, 1);
        auto Y = big_int<arena_allocator>(&MyArena, 1);
        uint32 Vals[] = {0xFFFFFFFF};
        X.Set(Vals,1);
        bool32 AllTestsPass = true;
        bool32 InitialCapacityIsOne = X.Length == X.ChunkCount && X.Length == 1;
        AllTestsPass &= InitialCapacityIsOne;

        X.Add(Y);
        //ensure that allocation has happened
        X.Length == X.ChunkCount && X.Length == 2;
        bool32 RequiredCapcaityGotAllocated = X.Length == X.ChunkCount && X.Length == 2;
        AllTestsPass &= RequiredCapcaityGotAllocated;

        X.Set(Vals,1);
        bool32 SetPreservesCapacity = X.Length == 1 && X.ChunkCount == 2;
        AllTestsPass &= SetPreservesCapacity;

        X.Add(Y);
        //ensure no memory leak on overflow
        bool32 OverflowUsesAvailableCapacity = X.Length == X.ChunkCount && X.Length == 2;
        AllTestsPass &= OverflowUsesAvailableCapacity;

        Tests.Append(AllTestsPass);
    }



    {
        //ensure Set(Array,Count) behaves as Set.Push.Push...
        uint32 Values[4] = {1,2,4,8};
        auto X = big_int<arena_allocator>(&MyArena, 0);
        Expected.Set(1,0,0).Push(2).Push(4).Push(8);
        X.Set(Values, ArrayCount(Values));
        Tests.Append(Equals(X,Expected));

        //ensure Push does not allocate memory when enough chunks are available
        X.Set(1,0,0).Push(2);
        Tests.Append(X.Length == 2 && X.ChunkCount == 4 );

        //ensure Set(Array,Count) also does not allocate memory when enough chunks are already available
        X.Set(Values,3);
        Tests.Append(X.Length == 3 && X.ChunkCount == 4 );
    }


    //Mul
    uint32 FullProd[2] = {};
    FullMul32(0xFFFFFFFF,0xFFFFFFFF, FullProd);
    Tests.Append(FullProd[0] == 0x00000001 && FullProd[1] == 0xFFFF'FFFE);

    A.Set(1,0,0);
    B.Set(1,1,0);
    A.Mul(B);
    Expected.Set(1,1,0);
    CheckA();

    A.Set(1,1,0);
    B.Set(1,0,0);
    A.Mul(B);
    Expected.Set(1,1,0);
    CheckA();

    A.Set(1,1,0);
    B.Set(1,1,0);
    A.Mul(B);
    Expected.Set(1,0,0);
    CheckA();

    {
        cout << "Test#" << Tests.TestCount << "\n";
        uint32 ValA[] = {4'000'000'000U};
        A.Set(ValA,1); //0xee6b2800
        B.Set(10,0,0);
        A.Mul(B); //0x9502f9000 == { 0x502f9000 , 0x9 } == { 1345294336 , 9}
        Expected.Set(1345294336,0,0).Push(9);
        Tests.Append(Equals(A, Expected));
    }

    {
        uint32 Vals[] = {0xFFFF'FFFF};
        A.Set(Vals,1);
    //    B.Set(A); //TODO(##2023 12 01) implement
        B.Set(Vals,1);
        A.Mul(B);
        Expected.Set(1,0,0).Push(0xFFFF'FFFE);
        Tests.Append(Equals(A, Expected));
        cout << "Mul 0xFFFF'FFFF 0xFFFF'FFFF => {1,0xFFFF'FFFE}  " << '\n';
        cout << std::hex <<  Expected.to_string() << std::dec << '\n';
    }

    {
        uint32 ValA[] = {0x1000'0000};
        A.Set(ValA,1);
        B.Set(0x1000,0,Log2I(0x1000));
        // 1000'0000'000
        //=100'0000'0000
        Expected.Set(0,0,0).Push(0x100);
        A.Mul(B);
        CheckA();
    }

    {
        uint32 ValuesA[] = {0,0,0x1000'0000};
        uint32 ValuesB[] = {0,0x1000'0000};
        A.Set(ValuesA, ArrayCount(ValuesA));
        B.Set(ValuesB, ArrayCount(ValuesB));
        uint32 ValuesExp[] = {0,0,0,0,0x100'0000};
        A.Mul(B);
        Expected.Set(ValuesExp, ArrayCount(ValuesExp));
        CheckA();
    }


    { //TODO(##2023 12 17): verify all bits, not just the last N ones
        A.Set(1,0,0);
        B.Set(10,0,3);
        for (uint32 Pow = 1 ; Pow < 100 ; ++Pow) {
            A.Mul(B);
            cout << A.to_string() << '\n';
            uint32 LeastBitIdx = Pow%32;
            uint32 LeastChunkIdx = Pow/32;

            bool32 LastNBitsGood = A.GetChunk(LeastChunkIdx)->Value & (1<<LeastBitIdx);
            if (!LastNBitsGood) {
                cout << "ERROR: " << Pow << ", " << Pow << '\n';
            }
            for (uint32 BitIdx = 0 ; BitIdx < Pow-1 ; ++BitIdx) {
                uint32 LocalBitIdx = BitIdx%32;
                uint32 ChunkIdx = BitIdx/32;
                LastNBitsGood = LastNBitsGood && ~(A.GetChunk(ChunkIdx)->Value)&(1<<LocalBitIdx);
                if (!LastNBitsGood) {
                    cout << "ERROR: " << Pow << ", " << BitIdx << '\n';
                }
            }
//            cout << "Last N Bits Good: " << (LastNBitsGood ? "Yes" : "No") << '\n';
        }
    }


    //TODO(##2023 12 04): test CopyTo(), make sure allocations happen only when necessary, and that length and capacity have correct values in all scenarios

    //tests for ShiftLeft(N)

    {
        //case 1: new leading idx > old leading idx
        uint32 ValuesA[] {0xF000F000, 0x0000FFFF};
        A.Set(ValuesA, ArrayCount(ValuesA));
        uint32 ShiftAmount = 16;
        uint32 ValuesExpected[] {0xF000'0000, 0xFFFFF000 };
        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected));
        A.ShiftLeft(ShiftAmount);
        CheckA();
    }

    {
        //case 2: new leading idx < old leading idx
        uint32 ValuesA[] {0xF000F000, 0x0000FFFF};
        A.Set(ValuesA, ArrayCount(ValuesA));
        uint32 ShiftAmount = 24;
        uint32 ValuesExpected[] {0x0000'0000, 0xFFF0'00F0, 0x0000'00FF };
        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected));
        A.ShiftLeft(ShiftAmount);
        CheckA();
    }

    {
        //case 3: new leading idx == old leading idx
        uint32 ValuesA[] {0xF000F000, 0x0000FFFF};
        A.Set(ValuesA, ArrayCount(ValuesA));
        uint32 ShiftAmount = 32;
        uint32 ValuesExpected[] {0x0000'0000, 0xF000'F000, 0x0000FFFF };
        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected));
        A.ShiftLeft(ShiftAmount);
        CheckA();
    }

    {
        //case 4
        A.Set(1);
        OldA.Set(A);
        uint32 ShiftAmount = 132;
        uint32 ValuesExpected[] {0x0000'0000, 0x0000'0000, 0x0000'0000, 0x0000'0000, 0x10 };
        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected));
        A.ShiftLeft(ShiftAmount);
        char TestName[] = "ShiftLeft 1<<132";
        PrintTestA(TestName);
        CheckA();
    }

    {
        //case 5
        cout << "Test#" << Tests.TestCount << " : 1 << [0...255]\n";
        bool32 TestPassed = true;
        for (int32 i = 0 ; i<256 ; ++i) {
            A.Set(1);
            A.ShiftLeft(i);
            int32 MSB = GetMSB(A);
            TestPassed &= MSB == i;
            cout << "1<<" << i << " = " << A.to_string() << "\n";
        }
        Tests.Append(TestPassed);
    }

    {
        //print different shifts
        A.Set(1,0,0);
        for (uint32 i = 0 ; i < 64 ; ++i) {
            cout << A.to_string() << '\n';
            A.ShiftLeft(i);
        }
        cout << A.to_string() << '\n';
    }

    {
        //print different shifts
        cout << A.to_string() << '\n';
        for (uint32 i = 0 ; i < 64 ; ++i) {
            A.ShiftRight(64-1-i);
            cout << A.to_string() << '\n';
        }
        A.ShiftRight(1);
        cout << A.to_string() << '\n';
    }


    //tests for ShiftRight(N)

    {
        uint32 ValuesA[] {0xFFFF'FFFF, 0xFFFF'FFFF, 0xFFFF'FFFF, 0xFFFF'FFFF, };
        A.Set(ValuesA, ArrayCount(ValuesA));
        uint32 ShiftAmount = 0;
        uint32 ValuesExpected[] {0xFFFF'FFFF, 0xFFFF'FFFF, 0xFFFF'FFFF, 0xFFFF'FFFF, };
        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected));
        A.ShiftRight(ShiftAmount);
        CheckA();
    }

    {
        uint32 ValuesA[] {0x1};
        A.Set(ValuesA, ArrayCount(ValuesA));
        uint32 ShiftAmount = 1;
        uint32 ValuesExpected[] {0x0};
        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected));
        A.ShiftRight(ShiftAmount);
        CheckA();
    }

    {
        uint32 ValuesA[] {0x2};
        A.Set(ValuesA, ArrayCount(ValuesA));
        uint32 ShiftAmount = 1;
        uint32 ValuesExpected[] {0x1};
        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected));
        A.ShiftRight(ShiftAmount);
        CheckA();
    }

    {
        uint32 ValuesA[] {0x00000000, 0xFFFFFFFF};
        A.Set(ValuesA, ArrayCount(ValuesA));
        uint32 ShiftAmount = 32;
        uint32 ValuesExpected[] {0xFFFF'FFFF};
        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected));
        A.ShiftRight(ShiftAmount);
        CheckA();
    }

    {
        uint32 ValuesA[] {0x8000'8000, 0xFFFF'0000, 0xAAAA'AAAA};
        A.Set(ValuesA, ArrayCount(ValuesA));
        uint32 ShiftAmount = 48;
        uint32 ValuesExpected[] {0xAAAA'FFFF, 0x0000'AAAA };
        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected));
        A.ShiftRight(ShiftAmount);
        CheckA();
    }

    {
        uint32 ValuesA[] {0x0000'0000, 0x0000'8000};
        A.Set(ValuesA, ArrayCount(ValuesA));
        uint32 ShiftAmount = 17;
        uint32 ValuesExpected[] {0x4000'0000 };
        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected));
        A.ShiftRight(ShiftAmount);
        HardAssert(A.Length == 1);
        CheckA();
    }

    {
        //print different shifts
        uint32 ValuesA[] {0x5555'8888, 0xFFFF'6666, 0xAAAA'9999};
        A.Set(ValuesA, ArrayCount(ValuesA));
        for (uint32 i = 0 ; i <= 96 ; ++i) {
            cout << A.to_string() << '\n';
            A.ShiftRight(1);
        }
        cout << A.to_string() << '\n';
    }
    cout << Tests.TestCount << "\n";
    big_int<arena_allocator> ExpectedInteger (MyArenaAllocator, 0);
    big_int<arena_allocator> ExpectedFraction {MyArenaAllocator, 0};
    big_int<arena_allocator> QuoInt {MyArenaAllocator, 0};
    big_int<arena_allocator> QuoFrac {MyArenaAllocator, 0};
    auto CheckQuo = [&](){
        //The Algorithm is optimized to append multiple 0's in a row when possible.
        //we truncate the result if its precision is higher than that of Expected
        int Diff = CountBits(QuoFrac)-CountBits(ExpectedFraction);
        if (Diff < 0) {
            //Quo was not computed to the desired Precision
            Tests.Append(false);
            return;
        }
        QuoFrac.ShiftRight(Diff);
        Tests.Append( Equals( QuoInt, ExpectedInteger ) &&
                      Equals( QuoFrac, ExpectedFraction ) );
    };

    {
        A.Set(24);
        B.Set(4);
        ExpectedInteger.Set(6);
        ExpectedFraction.Set(0,0,0);
        Div<arena_allocator>(A, B, QuoInt, QuoFrac, MyArenaAllocator);
        CheckQuo();
    }

    {
        A.Set(24);
        B.Set(24);
        ExpectedInteger.Set(1,0,0);
        ExpectedFraction.Set(0,0,0);
        Div<arena_allocator>(A, B, QuoInt, QuoFrac, MyArenaAllocator);
        CheckQuo();
    }

    {
        A.Set(1,0,0);
        B.Set(2);
        ExpectedInteger.Set(0,0,0);
        ExpectedFraction.Set(1,0,0);
        ExpectedFraction.Exponent = -1;
        Div<arena_allocator>(A, B, QuoInt, QuoFrac, MyArenaAllocator);
        cout << "Test#" << Tests.TestCount << "\n";
        CheckQuo();

    }

    {

        // 1478/2048=0.7216796875 = 0.10'1110'0011 ( BIN 10'1110'0011 = DEC 739 )
        A.Set(1478);
        B.Set(2046);
        ExpectedInteger.Set(0,0,0);
        ExpectedFraction.Set(739);
        ExpectedFraction.Exponent = -1;
        Div<arena_allocator>(A, B, QuoInt, QuoFrac, MyArenaAllocator, 10);
        CheckQuo();
    }


    {
        //3456/2123=1.62788506829957602128899907257...
        A.Set(3456);
        B.Set(2123);
        ExpectedInteger.Set(1,0,0);
        uint32 FracVal[]{0b1010'00001011'11010001'00110110'1001};
        ExpectedFraction.Set(FracVal,1);
        ExpectedFraction.Exponent = -1;
        Div<arena_allocator>(A, B, QuoInt, QuoFrac, MyArenaAllocator, 32);
        CheckQuo();
    }

    {
        //3456/2123=1.62788506829957602128899907257...
        A.Set(3456,1);
        B.Set(2123,1);
        ExpectedInteger.Set(1,0,0);     //1010'00001011'11010001'00110110'1001    1111'11100100'11111101
        uint32 FractionChunks[2] = {0b00110110'10011111'11100100'11111101, 0b1010'00001011'11010001};
        ExpectedFraction.Set(FractionChunks, ArrayCount(FractionChunks));
        ExpectedFraction.Exponent = -1;
        Div<arena_allocator>(A, B, QuoInt, QuoFrac, MyArenaAllocator, 52);
        CheckQuo();
    }
    {
        //#57
        cout << "Test#" << Tests.TestCount << "\n";
        A.Set(10,1);
        B.Set(4);
        ExpectedInteger.Set(2, 1, 1);
        uint32 FractionChunks[1] = {0b1};
        ExpectedFraction.Set(FractionChunks, ArrayCount(FractionChunks));
        ExpectedFraction.Exponent = -1;
        Div<arena_allocator>(A, B, QuoInt, QuoFrac, MyArenaAllocator, 1);
        CheckQuo();
    }

    {
        A.Set(10);
        B.Set(3,1);
        ExpectedInteger.Set(3,1,Log2I(3)); //dec 3.3333333... == bin 11.0101010101...
        uint32 FractionChunks[2] = {0b01010101'01010101'01010101'01010101, 0b0101'01010101'01010101};
        ExpectedFraction.Set(FractionChunks, ArrayCount(FractionChunks));
        ExpectedFraction.Exponent = -2;
        Div<arena_allocator>(A, B, QuoInt, QuoFrac, MyArenaAllocator, 52);
        CheckQuo();
    }

    {
        A.Set(1,0,0);
        B.Set(1'000'000);

        Div<arena_allocator>(A, B, QuoInt, QuoFrac, MyArenaAllocator, 52);
        cout << "1 millionth (fraction part ): " << QuoFrac << ", " << QuoFrac.Exponent << " Exponent, " << CountBits(QuoFrac) << " explicit significant fraction bits" << '\n';

        B.Set(1'000'000'000);
        Div<arena_allocator>(A, B, QuoInt, QuoFrac, MyArenaAllocator, 52);
        cout << "1 billionth (fraction part ): " << QuoFrac << ", " << QuoFrac.Exponent<< " Exponent, " << CountBits(QuoFrac) << " explicit significant fraction bits" << '\n';

//        B.Set(1'000'000'000'000);
//        Div<arena_allocator>(A, B, QuoInt, QuoFrac, MyArenaAllocator, 52);
//        cout << "1 trillionth (fraction part ): " << QuoFrac << '\n';
//
//        B.Set(1'000'000'000'000'000);
//        Div<arena_allocator>(A, B, QuoInt, QuoFrac, MyArenaAllocator, 52);
//        cout << "1 quadrillionth (fraction part ): " << QuoFrac << '\n';
    }


    {
//        test multi-block division
        cout << "Test#" << Tests.TestCount << " : multi block division\n";
        uint32 ValuesA[] {0x8000'0000, 0x0000'0000, 0x0000'0000, 0x8000'0000};
        uint32 ValuesB[] {0x8000'0000};
        A.Set(ValuesA, ArrayCount(ValuesA));
        B.Set(ValuesB, ArrayCount(ValuesB));
        uint32 ValuesExpected[] {0x0000'0001,0x0000'0000,0x0000'0000,0x0000'0001};
        ExpectedInteger.Set(ValuesExpected, ArrayCount(ValuesExpected));
        ExpectedFraction.Set(0,0,0);
        ExpectedFraction.Exponent = 0;

        Div<arena_allocator> (A, B, QuoInt, QuoFrac, MyArenaAllocator, 52);
        CheckQuo();
    }





    {
        char NumStr[] = "4294967296";
        ParseIntegral<arena_allocator>(NumStr, &A);
        uint32 ValuesExpected[] {0x0000'0000, 0x0000'0001};
        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected),0,32);
        CheckA();
    }


    {

        char NumStr[] = "999999999999999999";
        ParseIntegral<arena_allocator>(NumStr, &A);
        //DE0 B6B3 A763 FFFF
        uint32 ValuesExpected[] {0xA763'FFFF, 0x0DE0'B6B3};
        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected),0,59);
        CheckA();
        cout << "999999999999999999 = " << A.to_string() << '\n';
    }

    {
        char NumStr[] = "1000000000000000";
        ParseIntegral<arena_allocator>(NumStr, &A);
        //3 8D7E A4C6 8000
        uint32 ValuesExpected[] {0xA4C6'8000, 0x0003'8D7E};
        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected),0,49);
        CheckA();
        cout << "1000000000000000 = " << A.to_string() << '\n';
    }

    {
        cout << "Test #: " << Tests.TestCount << "\n";
        char NumStr[] = "999999999999999999";
        int MaxLen = StringLength(NumStr);
        int DigitCount = MaxLen;
        for (int i = 0 ; i < MaxLen  ; --DigitCount, ++i) {
            char * SubString = ( NumStr+MaxLen-DigitCount );
            ParseIntegral<arena_allocator>(SubString, &A);
            //DE0 B6B3 A763 FFFF
            uint32 ValuesExpected[][2] = {
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
            A.Exponent = 0; //check only the bits
            Expected.Set(ValuesExpected[i], ArrayCount(ValuesExpected[i]));
            CheckA();
            cout << SubString << " = " << A.to_string() << '\n';
        }
    }

    {
        char NumStr[] = "999999999999999999999999999999999999";
        ParseIntegral<arena_allocator>(NumStr, &A);
//        uint32 ValuesExpected[] {0xA763'FFFF, 0x0DE0'B6B3};
//        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected));
//        CheckA();
        cout << "999999999999999999999999999999999999 = " << A.to_string() << '\n';
    }

    {
        char NumStr[] = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
        ParseIntegral<arena_allocator>(NumStr, &A);
//        uint32 ValuesExpected[] {0xA763'FFFF, 0x0DE0'B6B3};
//        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected));
//        CheckA();
        cout << "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890 = " << A.to_string() << '\n';
    }

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
        ParseIntegral<arena_allocator>(NumStr, &A);
//        uint32 ValuesExpected[] {0xA763'FFFF, 0x0DE0'B6B3};
//        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected));
//        CheckA();
        cout << "\
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
123456789012345678901234 = " << A.to_string() << '\n';
    }

    {
        // test Combine(), represent decimal number with big_int
        A.Set(3);
        B.Set(2);
        Div(A, B, QuoInt, QuoFrac, MyArenaAllocator);
        A = Combine (QuoInt, QuoFrac, A, MyArenaAllocator);
        //A.Normalize();
        uint32 ValuesExpected[] {0x3};
        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected));
        Expected.Exponent = 0;
        CheckA();
    }

    {
        //Test copy constructor
        uint32 ValuesA[] { 0x0000'0000, 0x0FFFF'FFFF };
        A.Set(ValuesA, ArrayCount(ValuesA));

        big_int<arena_allocator> A_ {MyArenaAllocator,A};
        //big_int<arena_allocator> A__ {A}; //disallowed, no shallow copies, deep copy requires an allocator as parameter.

        uint32 NewValues[] { 0xFFFF'FFFF, 0x8000'0000 };
        A_.Set(NewValues,ArrayCount(NewValues));
        uint32 ValuesExpected[] { 0x0000'0000, 0x0FFFF'FFFF };
        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected));
        CheckA();
    }

    {
        //test TruncateTrailingZeroBits (from fractional part)

        uint32 ValuesA[] { 0xAAAA'0000, 0xAAAA'AAAA };
        A.Set(ValuesA, ArrayCount(ValuesA), 0, 47);
        cout << "Test #" << Tests.TestCount << " : Test TruncateTrailingZeroBits : A = " << A.to_string();
        A.TruncateTrailingZeroBits();
        cout << " -> " << A.to_string() << "\n";
        uint32 ValuesExpected[] { 0x5555'5555, 0x5555 };
        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected), 0, 47 );
        CheckA();

        //---------------------------------------------v fractional point here
        uint32 ValuesA_1[] { 0x0, 0b1111'1111'1111'1111'0101'1000'0000'0000 };
        A.Set( ValuesA_1, ArrayCount(ValuesA_1), 0, 15 );
        cout << "Test #" << Tests.TestCount << " : Test TruncateTrailingZeroBits : A = " << A.to_string();
        A.TruncateTrailingZeroBits();
        cout << " -> " << A.to_string() << "\n";
        //----------------------------------------------------------v fractional point here
        uint32 ValuesExpected_1[] { 0b00000000'000'11111111'11111111'01011 };
        Expected.Set( ValuesExpected_1, ArrayCount(ValuesExpected_1), 0, 15 );
        CheckA();

        //--------------------------------------------------v fractional point here
        uint32 ValuesA_2[] { 0x0, 0b1111'1111'1111'1111'0101'1000'0000'0000 };
        A.Set( ValuesA_2, ArrayCount(ValuesA_2), 0, 19 );
        cout << "Test #" << Tests.TestCount << " : Test TruncateTrailingZeroBits : A = " << A.to_string();
        A.TruncateTrailingZeroBits();
        cout << " -> " << A.to_string() << "\n";
        //---------------------------------------------------------------v fractional point here
        uint32 ValuesExpected_2[] { 0b00000000'000'11111111'11111111'0101'1 };
        Expected.Set( ValuesExpected_2, ArrayCount(ValuesExpected_2), 0, 19 );
        CheckA();

        //-----------------------------------------------------v fractional point here
        uint32 ValuesA_3[] { 0x0, 0b1111'1111'1111'1111'0101'10'00'0000'0000 };
        A.Set( ValuesA_3, ArrayCount(ValuesA_3), 0, 21 );
        cout << "Test #" << Tests.TestCount << " : Test TruncateTrailingZeroBits : A = " << A.to_string();
        A.TruncateTrailingZeroBits();
        cout << " -> " << A.to_string() << "\n";
        //-------------------------------------------------------------------v fractional point here
        uint32 ValuesExpected_3[] { 0b0'0000'0000'0011'1111'1111'1111'1101'011 };
        Expected.Set( ValuesExpected_3, ArrayCount(ValuesExpected_3), 0, 21 );
        CheckA();
    }



    {
        //use big_int to represent arbitrary precision fractional numbers
        A.Set(3);
        B.Set(2);
        uint32 ValuesExpected[] = { 0b1'1 };
        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected), 0, 0);

        A.Div(B); // A= 3/2
        cout << "Test #" << Tests.TestCount << " : " << A.to_string() << "\n";
        CheckA();


        ValuesExpected[0] = { 0b11 };
        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected), 0, 1);

        B.Set(A);  // B= 3/2
        A.AddF(A); // A= 6/2 = 3
        cout << "Test #" << Tests.TestCount << " : " << A.to_string() << "\n"; //#88
        CheckA();



        ValuesExpected[0] = { 0b100'1 };
        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected), 0, 2);

        A.AddF(B); // A= 3+3/2 = 4.5
        cout << "Test #" << Tests.TestCount << " : " << A.to_string() << "\n";
        CheckA();

        A.Set(81,1); //-81
        B.Set(6);
        ValuesExpected[0] = 0b1101'1;
        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected), 1, 3);

        A.Div(B); // C= -81/6 = -13.5
        cout << "Test #" << Tests.TestCount << " got/exp :\n" << A.to_string() << "\n" << Expected.to_string() << "\n";;
        CheckA();

        A.Set(3);
        A.Exponent = Log2I(3);
        A.Normalize();
        B.Set(2);
        B.Exponent = Log2I(2);
        B.Normalize();
        A.DivF(B); // A= 3/2
        B.DivF(A,33); // B= 4/3
        uint32 ValuesExpected_1[] = { 0b01010101'01010101'01010101'01010101, 0b101 };
        Expected.Set(ValuesExpected_1, ArrayCount(ValuesExpected_1), 0, 0);
        cout << "Test #" << Tests.TestCount << " : \nB= \t  " << B.to_string();
        cout << "\nExpected= " << Expected.to_string() << "\n" ;
        Tests.Append(Equals(B,Expected) && CountBits(B) == 35);

        A.AddF(B); // A= 17/6
        ValuesExpected_1[1] = 0b1011;
        ValuesExpected_1[0] = 0b01010101'01010101'01010101'01010101;
        Expected.Set(ValuesExpected_1, ArrayCount(ValuesExpected_1), 0, 1);
        cout << "Test #" << Tests.TestCount << " : \nA= \t  ";
        cout << A.to_string();
        cout << "\nExpected= " << Expected.to_string() << "\n" ;
        Tests.Append(Equals(A,Expected) && CountBits(A) == 36);

        B.SubF(A); // B= -9/6
        ValuesExpected[0] = 0b1'1;
        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected), 1, 0);
        cout << "Test #" << Tests.TestCount << " : \nB= \t  ";
        cout << B.to_string();
        cout << "\nExpected= " << Expected.to_string() << "\n" ;
        Tests.Append(Equals(B,Expected));

        cout << "Test #" << Tests.TestCount << " : \nA.RoundToNSignificantBits(33);\n";
        A.RoundToNSignificantBits(33);
        cout << "=" << A.to_string() << "\n";
        ValuesExpected_1[1] = 0b1;
        ValuesExpected_1[0] = 0b0'1101'0101'0101'0101'0101'0101'0101'011;
        Expected.Set(ValuesExpected_1, ArrayCount(ValuesExpected_1), 0, 1);
        cout << "(" << Expected.to_string() << ")\n";
        CheckA();

        cout << "Test #" << Tests.TestCount << " : \nA= \t  ";
        cout << A.to_string() << "\n*\t  " << B.to_string() << "\n=\t  ";
        //17/6 * -9/6 = 2.833333333333 * -1.5
        A.MulF(B); // A= -51/12 == -4.25
        ValuesExpected_1[1] = 0b100;
        ValuesExpected_1[0] = 0b0100'0000'0000'0000'0000'0000'0000'0001; //NOTE(##2024 06 12)not sure if this is what float would produce?
        Expected.Set(ValuesExpected_1, ArrayCount(ValuesExpected_1), 1, 2);
        cout << A.to_string();
        cout << "\nExpected= " << Expected.to_string() << "\n" ;
        CheckA();

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

        A.Set(3,0,1);
        A.Normalize();
        B.Set(2,0,1);
        B.Normalize();
        A.DivF(B,200); // A= 3/2
        B.DivF(A,200); // B= 4/3

        A.AddF(B); // A= 17/6

        cout << "Test #" << Tests.TestCount << "\n";
        cout << "A= " << A.to_string() << "\n";

        B.SubF(A); // B= -9/6

        cout << "B= " << B.to_string() << "\n";

        A.RoundToNSignificantBits(192);
        cout << "A.RoundToNSignificantBits(192);\n";
        cout << "A= " << A.to_string() << "\n";

        A.MulF(B); // A= -51/12 == -4.25
        ValuesExpected[0] = 0b100'01;
        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected), 1, 2);
//        ValuesExpected_1[1]= 4;
//        ValuesExpected_1[0]= 0b0011'0101'0101'0101'0101'0101'0101'0101;
//        Expected.Set(ValuesExpected_1, ArrayCount(ValuesExpected_1), 1, 32);

        cout << "A*B= " << A.to_string() << "\n";

        A.RoundToNSignificantBits(53);
        cout << "A.RoundToNSignificantBits(53);\n";
        cout << "A= " << A.to_string() << "\n";
        cout << "\nExpected= " << Expected.to_string() << "\n" ;
        CheckA(); //#96

        //---------

        A.Set(1);
        B.Set(3);
        int32 Precision = 4;
        cout << A.to_string() << " / " << B.to_string() << "(Prec=" << Precision << ")\n= ";
        A.DivF(B,Precision);
        cout << A.to_string() << "\n";
        A.Set(1);
        Precision = 5;
        cout << A.to_string() << " / " << B.to_string() << "(Prec=" << Precision << ")\n= ";
        A.DivF(B,Precision);
        cout << A.to_string() << "\n";
        cout << "\n";

        A.Set(1);
        A.Div(B,32); //B=3
        A.RoundToNSignificantBits(30);
        ValuesExpected[0] = 0b0'0101'0101'0101'0101'0101'0101'0101'011;
        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected), 0, -2);
        CheckA(); //#97
        cout << A.to_string() << "\n";
        cout << Expected.to_string() << "\n";
//
//        Tests.Append(C.RoughlyEquals(A,16));


    }


    {
        char DecimalStr[] = "1.0";
        float ExpectedFloat = 1.0f;
        ParseDecimal(DecimalStr, A);
        A.RoundToNSignificantBits(DOUBLE_PRECISION);
        Expected.Set(0x1,0,0);
        PrintTestOutput(A,A,Expected,nullptr,"A       ","Expected", "Parse 1.0");
        CheckA();

    }

    {
        char DecimalStr[] = "1.1";
        float ExpectedFloat = 1.1f;
        ParseDecimal(DecimalStr, A);
        A.RoundToNSignificantBits(DOUBLE_PRECISION);
        uint32 ExpectedBits[] = {0xCCCCCCCD,0b1'0001'1001'1001'1001'100};
        Expected.Set(ExpectedBits, ArrayCount(ExpectedBits),0,0);
        PrintTestOutput(A,A,Expected,nullptr,"A       ","Expected", "Parse 1.1");
        CheckA();
    }



    {
        //test Normalize()
        char TestName[]="Normalize()";
        A.Zero(ZERO_EVERYTHING);
        for (int i = 0 ; i < 4 ; ++i ) A.ExtendLength();
        A.GetChunk(2)->Value = 0x8811;
        A.Exponent = 7;
        A.CopyTo(&OldA);
        A.Normalize();
        Expected.Set(0x8811);
        Expected.Exponent = 7;
        PrintTestA(TestName);
        CheckA();

    }

    {
        cout << "Test#" << Tests.TestCount << "\n";
        A.Set(0x10'000'000);
        A.Normalize();
        Expected.Set(1);
        Expected.Exponent = 0; //Normalize doesn't change exponent

        CheckA();

    }


    {
        char DecimalStr[] = "1.1";
        double ExpectedDouble = 1.1;
        ParseDecimal(DecimalStr, A);
        A.RoundToNSignificantBits(DOUBLE_PRECISION);
        //1.0001100110011001100110011001100110011001100110011010e0
        uint32 ExpectedBits[] = {0b10011001100110011001100110011010,0b100011001100110011001};
        Expected.Set(ExpectedBits, ArrayCount(ExpectedBits),0,Log2I(1));
        Expected.Normalize();
        PrintTestOutput(A,A,Expected,nullptr,"A       ","Expected", "Parse 1.1");
        CheckA();
    }

    {
        char DecimalStr[] = "1.12";
        double ExpectedDouble = 1.12;
        ParseDecimal(DecimalStr, A);
            A.RoundToNSignificantBits(DOUBLE_PRECISION);
           //100011110101110000101 00011110101110000101000111101100
        uint32 ExpectedBits[] = {0b00011110101110000101000111101100,0b100011110101110000101};
        Expected.Set(ExpectedBits, ArrayCount(ExpectedBits),0,Log2I(1));
        Expected.Normalize();
        PrintTestOutput(A,A,Expected,nullptr,"A       ","Expected", "Parse 1.12");
        CheckA();
    }

    {
        char DecimalStr[] = "12.98";
        double ExpectedDouble = 12.98;
        ParseDecimal(DecimalStr, A);
        A.RoundToNSignificantBits(DOUBLE_PRECISION);
        //11001111101011100001010001111010111000010100011110110
        uint32 ExpectedBits[] = {0b01000111101011100001010001111011,0b11001111101011100001};
        Expected.Set(ExpectedBits, ArrayCount(ExpectedBits),0,Log2I(12));
        Expected.Normalize();
        PrintTestOutput(A,A,Expected,nullptr,"A       ","Expected", "Parse 12.98");
        CheckA();
    }

    {
        char DecimalStr[] = "1234.9876";
        double ExpectedDouble = 1234.9876;
        ParseDecimal(DecimalStr, A);
        A.RoundToNSignificantBits(DOUBLE_PRECISION);
        //1.0011010010111111001101001101011010100001011000011110e10
        uint32 ExpectedBits[] = {0b10100110101101010000101100001111,0b10011010010111111001};
        Expected.Set(ExpectedBits, ArrayCount(ExpectedBits),0,Log2I(1234));
        Expected.Normalize();
        PrintTestOutput(A,A,Expected,nullptr,"A       ","Expected", "Parse 1234.9876");
        CheckA();
    }




    {
        char DecimalStr[] = "123456789.987654321";

        double ExpectedDouble = 123456789.987654321;
        ParseDecimal(DecimalStr, A);
        A.RoundToNSignificantBits(DOUBLE_PRECISION);
        // 11101011011110011010001010111111100110101101110101000e26
        uint32 ExpectedBits[] = {0b10001010111111100110101101110101,0b111010110111100110};
        Expected.Set(ExpectedBits, ArrayCount(ExpectedBits),0,Log2I(123456789));
        Expected.Normalize();
        PrintTestOutput(A,A,Expected,nullptr,"A       ","Expected", "Parse 123456789.987654321");
        CheckA();

    }

    {
        char DecimalStr[] = "-123456789.987654321";

        double ExpectedDouble = -123456789.987654321;
        ParseDecimal(DecimalStr, A);
        A.RoundToNSignificantBits(DOUBLE_PRECISION);
        // 11101011011110011010001010111111100110101101110101000e26
        uint32 ExpectedBits[] = {0b10001010111111100110101101110101,0b111010110111100110};
        Expected.Set(ExpectedBits, ArrayCount(ExpectedBits),1,Log2I(123456789));
        Expected.Normalize();
        PrintTestOutput(A,A,Expected,nullptr,"A       ","Expected", "Parse -123456789.987654321");
        CheckA();

    }

    {
        //2024 07 04: Test SetFloat
        cout << "\tTest #" << Tests.TestCount << "\n";
        char TestName[] = "Test SetFloat";
        float Challenge = -0x1.FFFFFEp127;
        A.SetFloat(Challenge);
        Expected.Set(0xFFFFFF,true,127);
        CheckA();
        cout << TestName << "\n";
        cout << std::hexfloat << Challenge << " -> " << "\n";
        cout << A.to_string() << "\n";
        cout << "(" << Expected.to_string() << ")\n";

        float Parsed = A.ToFloat();
        Tests.Append(Parsed==Challenge);
        cout << Parsed << " <- " << "\n";
    }

    {
        cout << "\tTest #" << Tests.TestCount << "\n";
        cout << "Parse 12345.6789012345f and compare to built_in representation\n";
        float Challenge = 12345.6789012345f;
        std::stringstream SS;
        SS << std::fixed << std::setprecision(20) << Challenge;
        std::string S = SS.str();
        char SBuf[64] = "";
        for (char *Src= &S[0], *Dst=SBuf ; *Src != '\0' ; ++Src, ++Dst) {
            *Dst = *Src;
        }
        ParseDecimal(SBuf,A);
        A.RoundToNSignificantBits(24);
        B.SetFloat(Challenge);
        Tests.Append(Equals(A,B));
        float Parsed = A.ToFloat();
        Tests.Append(Parsed==Challenge);

        cout << std::fixed << std::setprecision(20) << Challenge << " -> ParseDecimal()" << "\n";
        cout << A.to_string() << "\n";
        cout << Challenge << " -> SetFloat()" << "\n";
        cout << B.to_string() << "\n";
        cout << "A.ToFloat() -> " << "\n";
        cout << Parsed << "\n";
    }

    {
        cout << "\tTest #" << Tests.TestCount << "\n";
        cout << "Denormalized" << "\n";
        float Challenge = 0x1p-149f;
        A.SetFloat(Challenge);
        Expected.Set(0x1,false, -149);
        CheckA();
        float Parsed = A.ToFloat();
        Tests.Append(Parsed == Challenge);
        cout << std::hexfloat << Challenge << " -> " << "\n";
        cout << A.to_string() << "\n";
        cout << "(" << Expected.to_string() << ")\n";
        cout << Parsed << " <- " << "\n";
    }

    {
        cout << "\tTest #" << Tests.TestCount << "\n";
        cout << "Infinity" << "\n";
        float Challenge = 0x1p+128f;
        HardAssert (IsInf(Challenge));
        A.SetFloat(Challenge);
        Expected.Set(0x1,false, 128);
        CheckA();
        float Parsed = A.ToFloat();
        Tests.Append(Parsed == Challenge);
        cout << std::hexfloat << Challenge << " -> " << "\n";
        cout << A.to_string() << "\n";
        cout << "(" << Expected.to_string() << ")\n";
        cout << Parsed << " <- " << "\n";
    }

    {
        cout << "\tTest #" << Tests.TestCount << "\n";
        cout << "Zero" << "\n";
        A.Set(0x1,false,-150);
        float Parsed = A.ToFloat();
        Tests.Append(Parsed == 0.f);
        cout << A.to_string() << " -> " << std::hexfloat << Parsed << "\n";
    }

        cout << "\nTest parsed values where rounding changes exponent.\n";

    {
        cout << "\tTest #" << Tests.TestCount << "\n";
        cout << "for infinity\n";
        A.Set(0xFFFF'FFFF,true,127);

        A.CopyTo(&B);
        B.RoundToNSignificantBits(24);
        float Challenge = B.ToFloat();
        HardAssert(Challenge == -Inf);

        float Parsed = A.ToFloat();
        Tests.Append(Parsed == Challenge);

        cout << std::hexfloat << Challenge << " -> " << "\n";
        cout << A.to_string() << "\n";
        cout << "(" << Expected.to_string() << ")\n";
        cout << Parsed << " <- " << "\n";
    }


    {
        cout << "\tTest #" << Tests.TestCount << "\n";
        cout << "for subnormal\n";
        A.Set(0b11,true,-149);
        real32 Parsed = A.ToFloat();
        real32 Challenge = -0x1p-148f;
        Tests.Append(Parsed == Challenge);
        cout << A.to_string() << "-> ToFloat()\n";
        cout << std::hexfloat << Parsed << "\n";
        cout << "(" << Challenge << ")\n";
    }

    {
        cout << "\tTest #" << Tests.TestCount << "\n";
        cout << "for regular" << "\n";
        A.Set(0x1FFFFFF,true,0);
        real32 Parsed = A.ToFloat();
        real32 Challenge = -0x1p+1f;
        Tests.Append(Parsed == Challenge);
        cout << A.to_string() << "-> ToFloat()\n";
        cout << std::hexfloat << Parsed << "\n";
        cout << "(" << Challenge << ")\n";
    }

    {
        cout << "\tTest #" << Tests.TestCount << "\n";
        cout << "for almost zero:\n";
        A.Set(0b11,true,-150);
        real32 Parsed = A.ToFloat();
        real32 Challenge = -0x1p-149;
        Tests.Append(Parsed == Challenge);
        cout << A.to_string() << "-> ToFloat()\n";
        cout << std::hexfloat << Parsed << "\n";
        cout << "(" << Challenge << ")\n";
    }

    //TODO(##2024 07 13): test cases for round to even where Guard,Round,Sticky bits are needed. -> current rounding function may be biased towards infinity in certain cases.
    cout << "\nTest parity of ieee-754 round-to-even behavior, with guard, round and sticky bits\n\n";

    {
        auto Test = [&A,&B](uint32 Val, bool32 Neg, int32 Exp){
            A.Set(Val,Neg,Exp);
            B.Set(A);
            B.ToFloatPrecision();
            cout << A.to_string() << " ToFloat() -> " << B.to_string() << " -> "
            << A.ToFloat() << "\n";
        };
        typedef big_int<arena_allocator> bignum ;
        auto Test2 = [](bignum& A, bignum& B){
            B.Set(A);
            B.ToFloatPrecision();
            cout << A.to_string() << " ToFloat() -> " << B.to_string() << " -> "
            << A.ToFloat() << "\n";
        };


        Test(0x1,0,0);
        Test(0x1,0,-1);
        Test(0x1,0,-12);
        Test(0x1,0,-125);
        Test(0x1,0,-126);
        Test(0x1,0,-127);
        Test(0x1,0,-128);
        Test(0x1,0,-129);
        Test(0x1,0,-149);
        Test(0x1,0,-150);
        Test(0x1,0,-151);
        Test(0x1,0,-155);

//        Test(0x0,0,0);
//        Test(0x0,1,1);
//        Test(0x0,0,-1);
//        Test(0x0,0,-128);
//        Test(0x0,0,128);
//        Test(0x0,1,-128);
//        Test(0x0,1,128);



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
        Test(0x1,0,-149);
        Test(0x1,0,-150);

        cout << "\nguard and round bit set => round up\n";
        IEEE754(0x1.8p-150);
        Test(0x3,0,-150);

        cout << "\nguard and sticky bit set => round up\n";
        IEEE754(0x1.4p-150);
        Test(0x5,0,-149);

        cout << "\nguard and (very Small) sticky bit set => round up\n";
        IEEE754(0x1.000001p-150);
        Test(0x1000001,0,-150);

        cout << "\nonly guard bit set => round to even rule applies => bit before guard bit == 0 => round down\n";
        IEEE754(0x1.p-150);
        Test(0x1,0,-150);

        cout << "\nonly guard bit set => round to even rule applies => bit before guard bit == 1 => round up\n";
        IEEE754(0x1.8p-149);
        Test(0x3,0,-149);

        cout << "\nround-to-even-rule - more tests\n";
        bool32 OK;

        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 0, GRS==000\n";
        IEEE754(0x1.0000000p0);
        A.Set(0x10000000,0,0);
        A.Normalize();
        Test2(A,B);
        OK = A.ToFloat() == Small;
        cout << (OK ? "passed" : "ERROR" ) << "\n";
        Tests.Append(OK);

        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 1, GRS==000\n";
        IEEE754(0x1.0000020p0);
        A.Set(0x10000020,0,0);
        A.Normalize();
        Test2(A,B);
        OK = A.ToFloat() == Small;
        cout << (OK ? "passed" : "ERROR" ) << "\n";
        Tests.Append(OK);

        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 0, GRS==001\n";
        IEEE754(0x1.0000001p0);
        A.Set(0x10000001,0,0);
        A.Normalize();
        Test2(A,B);
        OK = A.ToFloat() == Small;
        cout << (OK ? "passed" : "ERROR" ) << "\n";
        Tests.Append(OK);

        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 1, GRS==001\n";
        IEEE754(0x1.0000021p0);
        A.Set(0x10000021,0,0);
        A.Normalize();
        Test2(A,B);
        OK = A.ToFloat() == Small;
        cout << (OK ? "passed" : "ERROR" ) << "\n";
        Tests.Append(OK);

        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 0, GRS==010\n";
        IEEE754(0x1.0000008p0);
        A.Set(0x10000008,0,0);
        A.Normalize();
        Test2(A,B);
        OK = A.ToFloat() == Small;
        cout << (OK ? "passed" : "ERROR" ) << "\n";
        Tests.Append(OK);

        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 1, GRS==010\n";
        IEEE754(0x1.0000028p0);
        A.Set(0x10000028,0,0);
        A.Normalize();
        Test2(A,B);
        OK = A.ToFloat() == Small;
        cout << (OK ? "passed" : "ERROR" ) << "\n";
        Tests.Append(OK);

        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 0, GRS==011\n";
        IEEE754(0x1.0000009p0);
        A.Set(0x10000009,0,0);
        A.Normalize();
        Test2(A,B);
        OK = A.ToFloat() == Small;
        cout << (OK ? "passed" : "ERROR" ) << "\n";
        Tests.Append(OK);

        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 1, GRS==011\n";
        IEEE754(0x1.0000029p0);
        A.Set(0x10000029p0,0,0);
        A.Normalize();
        Test2(A,B);
        OK = A.ToFloat() == Small;
        cout << (OK ? "passed" : "ERROR" ) << "\n";
        Tests.Append(OK);

        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 0, GRS==100\n";
        IEEE754(0x1.000001p0);
        A.Set(0x1000001,0,0);
        A.Normalize();
        Test2(A,B);
        OK = A.ToFloat() == Small;
        cout << (OK ? "passed" : "ERROR" ) << "\n";
        Tests.Append(OK);

        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 1, GRS==100\n";
        IEEE754(0x1.000003p0);
        A.Set(0x1000003,0,0);
        A.Normalize();
        Test2(A,B);
        OK = A.ToFloat() == Small;
        cout << (OK ? "passed" : "ERROR" ) << "\n";
        Tests.Append(OK);

        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 0, GRS==101\n";
        IEEE754(0x1.0000011p0);
        A.Set(0x10000011,0,0);
        A.Normalize();
        Test2(A,B);
        OK = A.ToFloat() == Small;
        cout << (OK ? "passed" : "ERROR" ) << "\n";
        Tests.Append(OK);

        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 1, GRS==101\n";
        IEEE754(0x1.0000031p0);
        A.Set(0x10000031,0,0);
        A.Normalize();
        Test2(A,B);
        OK = A.ToFloat() == Small;
        cout << (OK ? "passed" : "ERROR" ) << "\n";
        Tests.Append(OK);

        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 0, GRS==110\n";
        IEEE754(0x1.0000018p0);
        A.Set(0x10000018,0,0);
        A.Normalize();
        Test2(A,B);
        OK = A.ToFloat() == Small;
        cout << (OK ? "passed" : "ERROR" ) << "\n";
        Tests.Append(OK);

        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 1, GRS==110\n";
        IEEE754(0x1.0000038p0);
        A.Set(0x10000038,0,0);
        A.Normalize();
        Test2(A,B);
        OK = A.ToFloat() == Small;
        cout << (OK ? "passed" : "ERROR" ) << "\n";
        Tests.Append(OK);

        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 0, GRS==111\n";
        IEEE754(0x1.0000019p0);
        A.Set(0x10000019,0,0);
        A.Normalize();
        Test2(A,B);
        OK = A.ToFloat() == Small;
        cout << (OK ? "passed" : "ERROR" ) << "\n";
        Tests.Append(OK);

        cout << "\nTest #" << Tests.TestCount << " - ";
        cout << "\lsb == 1, GRS==111\n";
        IEEE754(0x1.0000039p0);
        A.Set(0x10000039,0,0);
        A.Normalize();
        Test2(A,B);
        OK = A.ToFloat() == Small;
        cout << (OK ? "passed" : "ERROR" ) << "\n";
        Tests.Append(OK);
    }

    {
        A.SetFloat(3.f);
        B.SetFloat(2.f);
        A.DivF(B,32); // A= 3/2
        B.DivF(A,31); // B= 4/3
        uint32 ValuesExpected_1[] = { 0b01010101'01010101'01010101'01010101, 0b1 };
        Expected.Set(ValuesExpected_1, ArrayCount(ValuesExpected_1), 0, 0);
        cout << "\nTest #" << Tests.TestCount;
        cout << "\nB=4/3 == 0b1.0101...01";
        cout << "\nB= \t  " << B.to_string();
        cout << "\nExpected= " << Expected.to_string() << "\n" ;
        Tests.Append(Equals(B,Expected) && CountBits(B) == 33);

        A.AddF(B); // A= 17/6
        ValuesExpected_1[1] = 0b10;
        ValuesExpected_1[0] = 0b11010101'01010101'01010101'01010101;
        Expected.Set(ValuesExpected_1, ArrayCount(ValuesExpected_1), 0, 1);
        cout << "\nTest #" << Tests.TestCount;
        cout << "\nA == 3/2; A += B;";
        cout << "\n3/2 + 4/3 = 17/6 == 0b10.1101...01";
        cout << "\nA = " << A.to_string();
        cout << "\nExpected= " << Expected.to_string() << "\n" ;
        Tests.Append(Equals(A,Expected) && CountBits(A) == 34);

        B.SubF(A); // B= -9/6
        uint32 ValuesExpected[] = {0b1'1};
        Expected.Set(ValuesExpected, ArrayCount(ValuesExpected), 1, 0);
        cout << "\nTest #" << Tests.TestCount;
        cout << "\n4/3 - 17/6 = -9/6 == -0b1.1";
        cout << "\nB = " << B.to_string();
        cout << "\nExpected = " << Expected.to_string() << "\n" ;
        Tests.Append(Equals(B,Expected));

        cout << "\nTest #" << Tests.TestCount << "\n";
        cout << "\nA.RoundToNSignificantBits(33) with A.BitCount == 34";
        cout << "\n=> A bits end with 010101";
        cout << "\n=>              LSB----^^---GuardBit";
        cout << "\n=> LSB == 0, Guard bit == 1, Round bit == Sticky bit == 0\n";
        cout << "\n=> round down (round-to-even)\n";
        A.RoundToNSignificantBits(33);
        cout << "=" << A.to_string() << "\n";
        Expected.Set(0b10'1101'0101'0101'0101'0101'0101'0101'01, 0, 1);
        cout << "(" << Expected.to_string() << ")\n";
        CheckA();

        double DA,DB;
        DA = 3.; DB = 2.;
        DA /= DB; DB /= DA; DA += DB;
        real32 FA=DA;
        cout << "\nTest #" << Tests.TestCount << "\n";
        cout << "double/float result: " << FA << "\n";
        cout << "big_int result:      " << A.ToFloat() << "\n";
        Tests.Append(FA == A.ToFloat());
    }

    //TODO(##2023 11 24): improve unit_test interface:
    //Tests.Assert(Condition)
    //Tests.AssertEqual(A,B)
    //Tests.AssertSatisfies(A,Predicate)
    //Tests.AssertAllSatisfy(ListA, Predicate) -> implement in terms of reduce/fold

    cout << "|-> " << Tests << "\n\n";
    Tests.PrintResults();
    return Tests.FailCount;
}

//TODO(ArokhSlade##2024 08 13): this is copy-pasta from UnitTest_G_PlatformGame_2_Module.cpp. extract!
void PrintOK(bool32 OK) {
    std::cout << (OK ? "passed" : "Error") << "\n\n";
}

int32 Test_Parser( bool OnlyErrors = false ) {
    using std::cout;
    cout << std::defaultfloat;
    cout << std::boolalpha;

    int32 FailCount { 0 };
    cout << __func__ << "\n\n";
    test_result Tests {};

    memory_index ArenaSize = (1u<<10)*sizeof(uint32);
    uint8 *ArenaBase = (uint8*)VirtualAlloc(0, ArenaSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    memory_arena MyArena;
    InitializeArena(&MyArena, ArenaSize, ArenaBase);

    arena_allocator MyArenaAllocator { &MyArena };
    big_int<arena_allocator>::InitializeContext (&MyArenaAllocator);


    {
        char Challenge[] = "-123.456";
        real32 Parsed = NAN;
        char *Next = ParseF32(Challenge,&Parsed);
        bool OK;
        OK = Parsed == -123.456f;

        if (!OnlyErrors || !OK) {
            cout << "Test #" << Tests.TestCount << " - Parse Float" << "\n";
            cout << "\"" << Challenge << "\"" << " -> " << Parsed << "\n";
            PrintOK(OK);
        }
        Tests.Append(OK);
    }


    cout << "entity Parser helper functions \n\n";

    {
        bool OK = true;
                          //01234 56 789 0123456789
        char Challenge[] = "    \r \n  \t asdfgh sdfgh";
        char *C = SkipSpace(Challenge,ArrayCount(Challenge));
        int32 Skipped = C-Challenge;
        int32 Expected = 11;
        OK &= Skipped == Expected;

        const char *CC = SkipSpace("  213");
        OK &= *CC == '2';

        if (!OnlyErrors || !OK) {
            cout << "Test #" << Tests.TestCount << " - SkipSpace()\n";
            cout << "Challenge:\n" << Challenge << "\n";
            cout << "Skipped to: " << *C << "\n";
            cout << "Challenge 2:\n" << "  213" << "\n";
            cout << "Skipped to: " << *CC << "\n";
            PrintOK(OK);
        }

        Tests.Append(OK);
    }

    {
        std::stringstream ss;
        ss <<  "entity_type{DEMON}";
        std::string s = ss.str();
        char *Text = const_cast<char*>(s.c_str());
        entity_type Parsed;
        entity_type Expected = ENTITY_DEMON;

        bool OK = ParseEntityType(Text, &Parsed) != nullptr;
        bool Match = Parsed == Expected;
        OK &= Match;

        if (!OnlyErrors || !OK) {
            cout << "Test #" << Tests.TestCount << " - Parse entity_type (tagged)\n";
            cout << "Challenge:\n" << ss.str() << "\n";
            cout << "Parse Result matches Expected: " << Match << "\n";
            cout << "Parsed: " << ToString(Parsed) << "\n";
            PrintOK(OK);
        }

        Tests.Append(OK);
    }


    {
        std::stringstream ss;
        ss <<  "DEMON";
        std::string s = ss.str();
        char *Text = const_cast<char*>(s.c_str());
        entity_type Parsed;
        entity_type Expected = ENTITY_DEMON;

        bool OK = ParseEntityType(Text, &Parsed) != nullptr;
        bool Match = Parsed == Expected;
        OK &= Match;

        if (!OnlyErrors || !OK) {
            cout << "Test #" << Tests.TestCount << " - Parse entity_type (untagged)\n";
            cout << "Challenge:\n" << ss.str() << "\n";
            cout << "Parse Result matches Expected: " << Match << "\n";
            cout << "Parsed: " << ToString(Parsed) << "\n";
            PrintOK(OK);
        }

        Tests.Append(OK);
    }


    {
        std::stringstream ss;
        ss <<  "v2{1.,-.8}";
        std::string s = ss.str();
        char *Text = const_cast<char*>(s.c_str());
        v2 Parsed {};
        v2 Expected = v2{1.,-.8};

//        bool OK = ParseV2(Text, &Parsed) != nullptr;
        bool OK = ParseValue<v2>(Text, &Parsed) != nullptr;
        bool Match = Parsed == Expected;
        OK &= Match;

        if (!OnlyErrors || !OK) {
            cout << "Test #" << Tests.TestCount << " - Parse v2\n";
            cout << "Challenge:\n" << ss.str() << "\n";
            cout << "Parse Result matches Expected: " << Match << "\n";
            cout << "Parsed: " << Parsed << "\n";
            PrintOK(OK);
        }

        Tests.Append(OK);
    }

    {
        std::stringstream ss;
        ss <<  "{1.,-.8}";
        std::string s = ss.str();
        char *Text = const_cast<char*>(s.c_str());
        v2 Parsed {};
        v2 Expected = v2{1.,-.8};

//        bool OK = ParseV2(Text, &Parsed) != nullptr;
        bool OK = ParseValue<v2>(Text, &Parsed) != nullptr;
        bool Match = Parsed == Expected;
        OK &= Match;

        if (!OnlyErrors || !OK) {
            cout << "Test #" << Tests.TestCount << " - Parse v2 - without type header\n";
            cout << "Challenge:\n" << ss.str() << "\n";
            cout << "Parse Result matches Expected: " << Match << "\n";
            cout << "Parsed: " << Parsed << "\n";
            PrintOK(OK);
        }

        Tests.Append(OK);
    }

    {
        std::stringstream ss;
        ss <<  "[1,8,2,4]";
        std::string s = ss.str();
        char *Text = const_cast<char*>(s.c_str());
        array<i32> Parsed {};
        array<i32> Expected = MakeArray<int32>(MyArena,std::initializer_list{1,8,2,4});

        bool OK = ParseArray<i32>(Text, &Parsed, &MyArena) != nullptr;
        bool Match = Parsed == Expected;
        OK &= Match;

        if (!OnlyErrors || !OK) {
            cout << "Test #" << Tests.TestCount << " - Parse Array (of ints)\n";
            cout << "Challenge:\n" << ss.str() << "\n";
            cout << "Parse Result matches Expected: " << (bool)Match << "\n";
            cout << "Parsed: " << Parsed << "\n";
            PrintOK(OK);
        }

        Tests.Append(OK);
    }

    {
        std::stringstream ss;
        ss <<  "v2 [{1.,1.},{2.,1.},{1.,3.}]";
        std::string s = ss.str();
        char *Text = const_cast<char*>(s.c_str());
        array<v2> Parsed {};
        array<v2> Expected = MakeArray<v2>(MyArena,std::initializer_list<v2>{{1.,1.},{2.,1.},{1.,3.}});

        bool OK = ParseArray<v2>(Text, &Parsed, &MyArena) != nullptr;
        bool Match = Parsed == Expected;
        OK &= Match;

        if (!OnlyErrors || !OK) {
            cout << "Test #" << Tests.TestCount << " - Parse Array<v2> - with array header\n";
            cout << "Challenge:\n" << ss.str() << "\n";
            cout << "Parse Result matches Expected: " << (bool)Match << "\n";
            cout << "Parsed: " << Parsed << "\n";
            PrintOK(OK);
        }

        Tests.Append(OK);
    }

    {
        std::stringstream ss;
        ss <<  "[v2{1.,1.},{2.,1.},{1.,3.}]";
        std::string s = ss.str();
        char *Text = const_cast<char*>(s.c_str());
        array<v2> Parsed {};
        array<v2> Expected = MakeArray<v2>(MyArena,std::initializer_list<v2>{{1.,1.},{2.,1.},{1.,3.}});

        bool OK = ParseArray<v2>(Text, &Parsed, &MyArena) != nullptr;
        bool Match = Parsed == Expected;
        OK &= Match;

        if (!OnlyErrors || !OK) {
            cout << "Test #" << Tests.TestCount << " - Parse Array<v2> - with headers\n";
            cout << "Challenge:\n" << ss.str() << "\n";
            cout << "Parse Result matches Expected: " << (bool)Match << "\n";
            cout << "Parsed: " << Parsed << "\n";
            PrintOK(OK);
        }

        Tests.Append(OK);
    }

    {
        std::stringstream ss;
        ss <<  "shape_2d{ [v2{1.,1.},{2.,1.},{1.,3.}] , v2{0.,0.} }";
        std::string s = ss.str();
        char *Text = const_cast<char*>(s.c_str());
        shape_2d Parsed {};
        auto Vertices = MakeArray<v2>(MyArena,std::initializer_list<v2>{{1.,1.},{2.,1.},{1.,3.}});
        shape_2d Expected = {.Vertices=Vertices.Ptr, .NumVertices=Vertices.Cnt, .Center=V2(0.,0.)};

        bool OK = ParseValue<shape_2d>(Text, &Parsed, &MyArena);
        bool Match = Parsed == Expected;
        OK &= Match;

        if (!OnlyErrors || !OK) {
            cout << "Test #" << Tests.TestCount << " - Parse shape_2d\n";
            cout << "Challenge:\n" << ss.str() << "\n";
            cout << "Parse Result matches Expected: " << (bool)Match << "\n";
            cout << "Parsed: " << Parsed << "\n";
            PrintOK(OK);
        }

        Tests.Append(OK);
    }

    {
        std::stringstream ss;
        ss << "0x12AB34CD";
        std::string s = ss.str();
        char *Text = const_cast<char*>(s.c_str());
        h32 Parsed = 0x0;
        h32 Expected = 0x12AB34CD;

        bool OK = ParseValue<h32>(Text, &Parsed) != nullptr;
        bool Match = Parsed == Expected;
        OK &= Match;

        if (!OnlyErrors || !OK) {
            cout << "Test #" << Tests.TestCount << " - Parse hexadecimal literal as u32\n";
            cout << "Challenge:\n" << ss.str() << "\n";
            cout << "Parse Result matches Expected: " << (bool)Match << "\n";
            cout << "Parsed: " << Parsed << "\n";
            PrintOK(OK);
        }

        Tests.Append(OK);
    }

    {
        std::stringstream ss;
        ss <<  "entity {\n";
        ss <<  "    Type : DEMON\n";
        ss <<  "    P : {1,2}\n";
        ss <<  "    Shape : { v2[ {-.5,-.5},{.5,-.5},{0.,1.} ] , {-.25,0.} }\n";
        ss <<  "    Color : 0xFF800080\n";
        ss <<  "}";

        std::string s = ss.str();
        char *Text = const_cast<char*>(s.c_str());
        entity Entity{};

        entity Expected{};
        Expected.Type = ENTITY_DEMON;
        Expected.P = {1,2};
        array<v2> Vertices = MakeArray( MyArena, std::initializer_list<v2>{ {-.5,-.5},{.5,-.5},{0.,1.}} );
        Expected.Shape = {Vertices.Ptr,Vertices.Cnt,{-.25,0.}};
        Expected.Color = 0xFF800080;

//        ParseValue<entity>(Text, &Entity, &MyArena);
        ParseEntity(Text, &Entity, &MyArena);
        bool Match = Entity == Expected;
        bool OK = Match;

        if (!OnlyErrors || !OK) {
            cout << "Test #" << Tests.TestCount << " - Parse Entity - 4 data members\n";
            cout << "Challenge:\n" << ss.str() << "\n";
            cout << "ParsedEntity matches Expected: " << Match << "\n";
            PrintOK(OK);
        }

        Tests.Append(OK);
    }

    {
        std::stringstream ss;
        ss <<  "{X:1. Y:-.8}";
        std::string s = ss.str();
        char *Text = const_cast<char*>(s.c_str());
        v2 Parsed {};
        v2 Expected = v2{1.,-.8};

        bool OK = ParseValue<v2>(Text, &Parsed) != nullptr;
        bool Match = Parsed == Expected;
        OK &= Match;

        if (!OnlyErrors || !OK) {
            cout << "Test #" << Tests.TestCount << " - Parse v2 - optional commas but mandatory separator (can be space)\n";
            cout << "Challenge:\n" << ss.str() << "\n";
            cout << "Parse Result matches Expected: " << Match << "\n";
            cout << "Parsed: " << Parsed << "\n";
            PrintOK(OK);
        }

        Tests.Append(OK);
    }

    {
        std::stringstream ss;
        ss <<  "shape_2d{ Vertices: [v2{1.,1.} {2.,1.} {1.,3.}] v2{0.,0.}}";
        std::string s = ss.str();
        char *Text = const_cast<char*>(s.c_str());
        shape_2d Parsed {};
        auto Vertices = MakeArray<v2>(MyArena,std::initializer_list<v2>{{1.,1.},{2.,1.},{1.,3.}});
        shape_2d Expected = {.Vertices=Vertices.Ptr, .NumVertices=Vertices.Cnt, .Center=V2(0.,0.)};

        bool OK = ParseValue<shape_2d>(Text, &Parsed, &MyArena);
        bool Match = Parsed == Expected;
        OK &= Match;

        if (!OnlyErrors || !OK) {
            cout << "Test #" << Tests.TestCount << " - Parse shape_2d - optional commas\n";
            cout << "Challenge:\n" << ss.str() << "\n";
            cout << "Parse Result matches Expected: " << (bool)Match << "\n";
            cout << "Parsed: " << Parsed << "\n";
            PrintOK(OK);
        }

        Tests.Append(OK);
    }


    {
        std::stringstream ss;
        ss <<  "entity {\n";
        ss <<  "    Type : entity_type{DEMON}\n";
        ss <<  "    {1,2},\n";
        ss <<  "    { Vertices : v2[ {-.5,-.5},{.5,-.5},{0.,1.} ], {-.25,0.} }\n";
        ss <<  "    Color : 0xFF800080\n";
        ss <<  "}";

        std::string s = ss.str();
        char *Text = const_cast<char*>(s.c_str());
        entity Entity{};

        entity Expected{};
        Expected.Type = ENTITY_DEMON;
        Expected.P = {1,2};
        array<v2> Vertices = MakeArray( MyArena, std::initializer_list<v2>{ {-.5,-.5},{.5,-.5},{0.,1.}} );
        Expected.Shape = {Vertices.Ptr,Vertices.Cnt,{-.25,0.}};
        Expected.Color = 0xFF800080;

        ParseEntity(Text, &Entity, &MyArena);

        bool Match = Entity == Expected;
        bool OK = Match;

        if (!OnlyErrors || !OK) {
            cout << "Test #" << Tests.TestCount << " - Parse Entity - optional comma after unlabeled fields\n";
            cout << "Challenge:\n" << ss.str() << "\n";
            cout << "ParsedEntity matches Expected: " << Match << "\n";
            PrintOK(OK);
        }

        Tests.Append(OK);
    }

    {
        std::stringstream ss;
        ss <<  "entity {\n";
        ss <<  "    Type : entity_type{DEMON}\n";
        ss <<  "    {1,2},\n";
        ss <<  "    { Vertices : [ {-.5,-.5},{.5,-.5},{0.,1.} ] {-.25,0.} },\n";
        ss <<  "    0xFF800080,\n";
        ss <<  "}";

        std::string s = ss.str();
        char *Text = const_cast<char*>(s.c_str());
        entity Entity{};

        entity Expected{};
        Expected.Type = ENTITY_DEMON;
        Expected.P = {1,2};
        array<v2> Vertices = MakeArray( MyArena, std::initializer_list<v2>{ {-.5,-.5},{.5,-.5},{0.,1.}} );
        Expected.Shape = {Vertices.Ptr,Vertices.Cnt,{-.25,0.}};
        Expected.Color = 0xFF800080;

        ParseEntity(Text, &Entity, &MyArena);

        bool Match = Entity == Expected;
        bool OK = Match;

        if (!OnlyErrors || !OK) {
            cout << "Test #" << Tests.TestCount << " - Parse Entity - comma after last field is permissible\n";
            cout << "Challenge:\n" << ss.str() << "\n";
            cout << "ParsedEntity matches Expected: " << Match << "\n";
            PrintOK(OK);
        }

        Tests.Append(OK);
    }

    cout << "Tests for FALSIFYing wrong inputs\n\n";
    {
        std::stringstream ss;
        ss <<  "shape_2d { \n";
        ss <<  "          { {-.5,-.5},{.5,-.5},{0.,1.}}\n";
        ss <<  " Center : {-.25,0.}\n";
        ss <<  "}";

        std::string s = ss.str();
        char *Text = const_cast<char*>(s.c_str());
        shape_2d Parsed {};

        bool OK = nullptr == ParseValue<shape_2d>(Text, &Parsed, &MyArena);

        if (!OnlyErrors || !OK) {
            cout << "Test #" << Tests.TestCount << " - Parse shape_2d - falsify wrong commas \n";
            cout << "Challenge:\n" << ss.str() << "\n";
            cout << "Parser returned error: " << (OK ? "yes (GOOD!)":"no(BAD!)")<< "\n";
            PrintOK(OK);
        }

        Tests.Append(OK);
    }

    {
        std::stringstream ss;
        ss <<  "entity {\n";
        ss <<  "    Type : entity_type{DEMON}\n";
        ss <<  "    {1,2}\n";
        ss <<  "    { Vertices : { {-.5,-.5},{.5,-.5},{0.,1.}} {-.25,0.} },\n";
        ss <<  "    Color : 0xFF800080\n";
        ss <<  "}";

        std::string s = ss.str();
        char *Text = const_cast<char*>(s.c_str());

        entity Parsed{};
        bool OK = nullptr == ParseEntity(Text, &Parsed, &MyArena);

        if (!OnlyErrors || !OK) {
            cout << "Test #" << Tests.TestCount << " - Parse Entity - rule: enum are not type-tagged or enclosed in braces\n";
            cout << "Challenge:\n" << ss.str() << "\n";
            cout << "Parser returned error: " << (OK ? "yes (GOOD!)":"no(BAD!)")<< "\n";
            PrintOK(OK);
        }

        Tests.Append(OK);
    }

    {
        std::ifstream ifs{"..\\..\\data\\test data\\parse_test.txt"};
        std::string pt;
        for (char c ; ifs.get(c) ; ) {
            pt += c;
        };
        char *Text = const_cast<char*>(pt.c_str());
        entity Entity{};
        bool32 OK = ParseEntity(Text, &Entity, &MyArena) != nullptr;

        if (!OnlyErrors || !OK) {
            cout << "Test #" << Tests.TestCount << " - Parse Entity File\n";
            PrintOK(OK);
        }

        Tests.Append(OK);
    }


    cout << "|-> " << Tests << "\n\n";
    Tests.PrintResults();
    return Tests.FailCount;
}

} //namespace unit_test



using namespace unit_test;
int main() {
    int32 FailCount = 0;
//    FailCount += Test_ParseInt32();
    FailCount += Test_ParseIntegralPart();
//    FailCount += Test_BigInt();
//    FailCount += Test_Parser(true);
    return FailCount;
}
