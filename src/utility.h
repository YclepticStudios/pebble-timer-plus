//! @file utility.h
//! @brief File containing simple convenience functions.
//!
//! This file contains simple convenience functions that may be used
//! in several different places. An example would be the "assert" function
//! which terminates program execution based on the state of a pointer.
//!
//! @author Eric D. Phillips
//! @date August 29, 2015
//! @bugs No known bugs


//! Terminate program if null pointer
//! @param ptr The pointer to check for null
#define ASSERT(ptr) assert(ptr, __FILE__, __LINE__)

//! Malloc with failure check
//! @param size The size of the memory to allocate
#define MALLOC(size) malloc_check(size, __FILE__, __LINE__)
