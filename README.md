### BigDecimal - Header-only Library

This library provides a floating-point-like data-type for storing numbers of arbitrary precision.   
(Today, I would probably call it BigFloat, as that seems more accurate.)   
Basic arithmetic operations (+,-,*,/) are provided.   
   
The internal representation is a list of integral chunks that store the bits of the binary representation, as well as sign bit and exponent.
The type is configurable as to the data type used for the chunks (byte, int, etc.),
as well as the allocator type used for providing additonal memory (must be compatible with a subset of the std::allocator interface).

#### Sample project
Demo and UnitTest can compile to .exe files.
Copy the .dll from lib/ folder into the same location as the .exe files to get them to run.

#### Motivation
I built this because I wanted to track / analyze floating point error in one of my other projects by mirroring the operations with a type that provides arbitrary precision.
There, I had a custom polygon intersection algorithm that was struggling with rounding errors.
It was a rather involved series of self-learning projects ...
