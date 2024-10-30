#ifndef MEMORY_MANAGEMENT_SERVICE_H
#define MEMORY_MANAGEMENT_SERVICE_H

#include <iostream> //for debug prints only

#include "G_Essentials.h"
#include "G_Miscellany_Utility.h"

#ifdef STATIC_LIB_BUILD_G_MEMORYMANAGEMENT_SERVICE
    #define DLL_KEYWORD
#else
    #ifdef DLL_HEADER_BUILD_G_MEMORYMANAGEMENT_SERVICE
        #define DLL_KEYWORD __declspec(dllexport)
    #else
        #define DLL_KEYWORD __declspec(dllimport)
    #endif
#endif

struct DLL_KEYWORD memory_arena
{
	memory_index Size;
	uint8 *Base;
	memory_index Used;
	memory_arena *Parent;
};

struct DLL_KEYWORD counted_memory
{
	void *Data;
	memory_index Count;
};

DLL_KEYWORD void InitializeArena(memory_arena *Arena, memory_index Size, uint8 *Base);

#define PushStruct(Arena, type) (type *)PushSize_(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type *)PushSize_(Arena, (Count)*sizeof(type))
DLL_KEYWORD void *PushSize_(memory_arena *Arena, memory_index Size);

#define PopStruct(Arena, type) PopSize_(Arena, sizeof(type))
#define PopArray(Arena, Count, type) PopSize_(Arena, Count * sizeof(type))
DLL_KEYWORD void PopSize_(memory_arena *Arena, memory_index Size);

DLL_KEYWORD memory_arena *PushArena(memory_arena *Arena);
DLL_KEYWORD memory_arena *PopArena(memory_arena *Arena);

DLL_KEYWORD void ResetArena(memory_arena *Arena);

#include <initializer_list>
template<class T>
array<T> MakeArray(memory_arena& Arena, std::initializer_list<T> Args) {
    int32 Size = Args.size();
    T* Addr = PushArray(&Arena,Size,T);
    array<T> Array{Addr, Size, Size};
    for (T Arg : Args) { //TODO(##2024 07 14) use & but solve const problem
        *Addr++ = Arg;
    }
    return Array;
}
//
//
//
//template<typename T>
//struct Arena_Allocator {
//
//    using value_type = T;
//
//    //TODO(2024 09 18##ArokhSlade):Delete this
//    static constexpr size_t s_default_capacity = Kilobytes(1);
//
//    memory_arena *m_arena_p = nullptr;
//
//    T* allocate(size_t n) {
//        T* result = PushArray(m_arena_p, n, T);
//        return result;
//    }
//
//    void deallocate(T* p, size_t n) {
//        /*arena doesn't deallocate*/
//        return;
//    }
//
//    void construct(T* p, const T& val) {
//        new(p) T{val};
//    }
//    void destroy(T* p) {
//        ~*p;
//    };
//
//    Arena_Allocator(size_t capacity, u8 *storage)
//    : m_arena_p {(capacity && storage ? new(storage) memory_arena : nullptr)} {
//
//
//        if (capacity == 0 || storage == nullptr) return;
//
//        HardAssert(capacity > sizeof(memory_arena));
//        HardAssert(storage);
//
//        u8 *real_base = (u8*)m_arena_p + sizeof(memory_arena);
//        size_t real_size = capacity - sizeof(memory_arena);
//        InitializeArena(m_arena_p, real_size, real_base);
//    }
//
//    Arena_Allocator(memory_arena *arena_p = nullptr) : m_arena_p{arena_p}
//    {}
//
//    template <typename U>
//    explicit Arena_Allocator(Arena_Allocator<U> const& other)
//    : m_arena_p{other.m_arena_p}
//    {}
//
//
//    template <typename U>
//    int operator==(Arena_Allocator<U> const& other)
//    {
//        return m_arena_p == other.m_arena_p;
//    }
//
//    ~Arena_Allocator() {
//        std::cout << "~Arena_Allocator()\n" ;
//    }
//
//    //TODO(ArokhSlade##2024 08 28): comparison with any other type (should return false)
//};
//

#define FUN_DELETER(name) void name (u8 *memory, size_t size)
typedef FUN_DELETER((*FunPtrDeleter));

struct ArenaAllocMeta {
    memory_arena arena;
    FunPtrDeleter deleter;
    i32 ref_count = 0;
//    ArenaAllocMeta *parent = nullptr; //TODO(ArokhSlade##2024 09 19)
//    ArenaAllocMeta *first_child = nullptr;
//    ArenaAllocMeta *next_sibling = nullptr;

    bool is_initialized() {
        HardAssert(arena.Base == 0 || arena.Base == (u8*)this);
        return arena.Size > 0 && arena.Base && arena.Base == (u8*)this && ref_count > 0 && deleter;
    }

    void erase() {
        deleter(reinterpret_cast<u8 *>(this), arena.Size);
    }


    ArenaAllocMeta (size_t size, FUN_DELETER(deleter_)) {
        InitializeArena(&arena, size, (u8*)this); //NOTE(ArokhSlade##2024 09 20): placement-new
        arena.Used = sizeof(ArenaAllocMeta); //NOTE(ArokhSlade##2024 09 20): *this always lives as arena's first data
        deleter = deleter_;
    }

    ArenaAllocMeta(const ArenaAllocMeta& other) = delete;
    ArenaAllocMeta& operator=(const ArenaAllocMeta& other) =delete;


    void* allocate(size_t size) {
        void * result = PushSize_(&arena, size);
        return result;
    }
};

/**
 *  \brief      ref-counted arena allocator (multiple allocators can be tied to the same arena, thanks to their common meta data pointer
 *  \warning    careful with using placement-new: in order ref-counting to work, you can't placement-new over an existing allocators address. use operator= instead
 **/
template<typename T>
struct ArenaAlloc{
    using value_type = T;

    ArenaAllocMeta *meta;


    i32 get_ref_count() {
        return (meta ? meta->ref_count : -1 );
    }

    bool is_valid() {
        bool completely_clean = meta == nullptr;
        bool properly_initialized = meta && meta->is_initialized();
        return completely_clean || properly_initialized;
    }

    //NOTE(ArokhSlade##2024 09 20): space needs to be zeroed out
    ArenaAlloc(size_t size, u8 *space, FUN_DELETER(deleter_)) {
        HardAssert(size > sizeof(ArenaAllocMeta));
        HardAssert(space);
        //TODO(ArokhSlade##2024 09 20): reevaluate whether we need the whole arena to be cleared or just the meta block.
        //might even clear it ourselves.
        HardAssert(reinterpret_cast<ArenaAllocMeta *>(space)->is_initialized() == false);
        meta = new(space) ArenaAllocMeta(size, deleter_);
        ++meta->ref_count;
    }

    ArenaAlloc() : meta(nullptr){
    }

    ArenaAlloc(const ArenaAlloc& other)
    : meta(other.meta) {
        if (meta) {
            ++meta->ref_count;
        }
    }

    template <typename U>
    explicit ArenaAlloc(ArenaAlloc<U> const& other)
    : meta(other.meta) {
        if (meta) {
            ++meta->ref_count;
        }
    }

    template <typename U>
    int operator==(ArenaAlloc<U> const& other)
    {
        return meta == other.meta;
    }

    void release_arena() {
        HardAssert(meta);
        meta->erase();
    }

    ArenaAlloc& operator=(const ArenaAlloc& other) {
        if (meta) {
            --meta->ref_count;
            if (meta->ref_count == 0) {
                release_arena();
            }
        }
        meta = other.meta;
        if (meta) {
            ++meta->ref_count;
        }
        return *this;
    }

    ~ArenaAlloc() {
        if (meta == nullptr) return;

        HardAssert (meta->ref_count > 0);
        --meta->ref_count;

        if ( 0 == meta->ref_count ) {
            release_arena();
        }

        meta = nullptr;
    }

    T* allocate(size_t n) {
        T* result = reinterpret_cast<T*>(meta->allocate(n * sizeof(T)));
        return result;
    }

    void deallocate(T* p, size_t n) {
        /*arena doesn't deallocate*/
        return;
    }
};

// conflict with charvonv.h:81 "multiple definition"
//FUN_DELETER(std_deleter) {
//    delete[] (memory);
//}


#endif //MEMORY_MANAGEMENT_SERVICE_H
