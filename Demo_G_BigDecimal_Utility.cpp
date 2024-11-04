#include "G_Essentials.h"
#include "G_MemoryManagement_Service.h"
#include "G_Miscellany_Utility.h"
#include "G_BigDecimal_Utility.h"
#include "G_Math_Utility.h" //FLOAT_PRECISION

#include "G_UnsortedUtilities.h"

#include <iostream>

using std::cout;

template <typename T>
void print_arena_status(ArenaAlloc<T>& arena_alloc) {
	size_t arena_fill = arena_alloc.meta->arena.Used;
	size_t arena_capacity = arena_alloc.meta->arena.Size;
	f32 arena_fill_percent = (f32)arena_fill/arena_capacity;
	
	cout << "Arena storage: " << arena_fill_percent << "% (" << arena_fill << " / " << arena_capacity << " Bytes)\n\n";
	return;
}


using ChunkArena = ArenaAlloc<BigDecimal_::ChunkBits>;

f64 test_parse_double(char *input, f64 expected, BigDecimal<ChunkArena>& output, ChunkArena& arena_alloc) {
	BigDecimal<ChunkArena>::from_string(input, &output);
	f64 parsed = output.to_double();
	
	bool OK = true;
	OK &= parsed == expected;
	
	cout << "input   : " << input << "\n";
	cout << "parsed  : " << parsed << "\n";
	cout << "expected: " << expected << "\n";
	cout << (OK ? "MATCH" : "ERROR") << "\n";	
	
	print_arena_status(arena_alloc);
	
	return parsed;
}

i32 main() {
	
	cout << "Demo: BigDecimal\n\n";
	
	
	size_t arena_size = Kilobytes(8);
	u8 *arena_storage = new u8[arena_size]{}; 
	ChunkArena arena_alloc = ChunkArena{arena_size, arena_storage, std_deleter};
	BigDecimal<ChunkArena>::initialize_context(arena_alloc);
	
	BigDecimal<ChunkArena> big_decimal_01;
	f64 expected64, parsed64;
	
	{
		char decimal_string[] = "-9.75";
		expected64 = -9.75;	
		parsed64 = test_parse_double(decimal_string, expected64, big_decimal_01, arena_alloc);	
	}
	
	{
		char decimal_string[] = "123456789012345678901234567890.98765432109876543210987654321";	
		expected64 = 123456789012345678901234567890.98765432109876543210987654321;	
		parsed64 = test_parse_double(decimal_string, expected64, big_decimal_01, arena_alloc);	
	}
	
	{
		char decimal_string[] = "123456789012345678901234567890123456789012345678901234567890.98765432109876543210987654321098765432109876543210987654321";	
		expected64 = 123456789012345678901234567890123456789012345678901234567890.98765432109876543210987654321098765432109876543210987654321;	
		parsed64 = test_parse_double(decimal_string, expected64, big_decimal_01, arena_alloc);	
	}
	
	{
		char decimal_string[] = "123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890.98765432109876543210987654321098765432109876543210987654321098765432109876543210987654321098765432109876543210987654321";	
		expected64 = 123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890.98765432109876543210987654321098765432109876543210987654321098765432109876543210987654321098765432109876543210987654321;	
		parsed64 = test_parse_double(decimal_string, expected64, big_decimal_01, arena_alloc);	
	}
	
	BigDecimal<ChunkArena>::close_context(true);	
	return 0;
}