#ifndef G_UNSORTED_UTILITIES
#define G_UNSORTED_UTILITIES


//macro for function prototype for deleter functions
//given as parameter to arena allocators so they can release their memory
inline FUN_DELETER(std_deleter) {
    delete[] (memory);
}

#endif // G_UNSORTED_UTILITIES