# JSON C/C++ Library for IoT Communication

The JSON C library is designed specifically for use in resource constrained micro controllers.

See the [JSON C/C++ Library home page](https://realtimelogic.com/products/json/) for details.

**The library can be used in three modes:**

* Using dynamic allocation (Complexity level: easy)
* Using static allocation based on the [JSON Parser Value Factory](https://realtimelogic.com/ba/doc/en/C/reference/html/structJParserValFact.html) (Complexity level: moderate)
* Using static allocation based on the [JSON Decoder](https://realtimelogic.com/ba/doc/en/C/reference/html/structJDecoder.html) class (Complexity level: advanced)

A detailed explanation on the three different modes can be found in the [reference manual](https://realtimelogic.com/ba/doc/en/C/reference/html/md_en_C_md_JSON.html) .

# Examples

* The m2m-led.c example is using the JSON parser in continuous stream parser mode and is using the more complex JSON Decoder setup.
* The StaticAllocatorEx.cpp shows how to use a static allocator with the JSON Parser Value Factory.
* Generic test/example program: test/test1.cpp

# Compiling

Decide if you want to use the more complex JDecoder with static allocation or the easier to use JSON Parser Value Factory (JParserValFact) with dynamic allocation or static allocation. See the documentation for details regarding these two classes.

* If using JParserValFact, include all C files in the build, except JDecoder.c
* If using JDecoder, include all C files in the build, except JVal.c and define the macro NO_JVAL_DEPENDENCY when compiling the code. See the comment at the top of the source file JEncoder.c for details.

The included example Makefile, which is configured for GCC, builds 3 libraries. See the Makefile for details.

NOTE: The C files in the src directory MUST be compiled by a C compiler and not by a C++ compiler. Files ending with .c must be compiled by a C compiler and files ending with .cpp must be compiled by a C++ compiler.



