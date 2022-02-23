# JSON C/C++ Library for IoT Communication

The JSON C library is designed for use in resource constrained micro controllers such as the Cortex-M0. See the [JSON C/C++ Library home page](https://realtimelogic.com/products/json/) for details.

This standalone version of the JSON library is also embedded in the [Barracuda App Server (repository)](https://github.com/RealTimeLogic/BAS), which provides a Lua API via the [Lua Server Pages Engine](https://realtimelogic.com/products/lua-server-pages/).

**The library can be used in three modes:**

* Using dynamic allocation (Complexity level: easy)
* Using static allocation based on the [JSON Parser Value Factory](https://realtimelogic.com/ba/doc/en/C/reference/html/structJParserValFact.html) (Complexity level: moderate)
* Using static allocation based on the [JSON Decoder](https://realtimelogic.com/ba/doc/en/C/reference/html/structJDecoder.html) class (Complexity level: advanced)

A detailed explanation on the three different modes can be found in the [reference manual](https://realtimelogic.com/ba/doc/en/C/reference/html/md_en_C_md_JSON.html) .

# Included JSON Examples

* The m2m-led.c IoT example is using the JSON parser in continuous stream parser mode and is using the more complex JSON Decoder setup. This setup is explained in the online [M2M LED JSON Client Documentation](https://realtimelogic.com/ba/doc/en/C/reference/html/md_en_C_md_JSON.html#M2MLED).
* The StaticAllocatorEx.cpp shows how to use a static allocator with the [JSON Parser Value Factory](https://realtimelogic.com/ba/doc/en/C/reference/html/structJParserValFact.html).
* Generic test/example program: test/test1.cpp

# Additional JSON IoT Examples
 * The [Minnow WebSocket Server's Reference Example](https://realtimelogic.com/articles/Creating-SinglePage-Apps-with-the-Minnow-Server) uses JSON for messages sent over WebSockets.
 * The [SMQ IoT Protocol](https://realtimelogic.com/products/simplemq/)'s [publisher and subscriber examples](https://github.com/RealTimeLogic/SMQ#1-introductory-smq-examples) send and receive JSON encoded messages. The readme file includes [additional information](https://github.com/RealTimeLogic/SMQ#simplified-cc-design) on how to use this JSON library for sending a large JSON message as chunks and how to parse the chunks as they trickle in.

# Compiling

Decide if you want to use the more complex JDecoder with static allocation or the easier to use JSON Parser Value Factory (JParserValFact) with dynamic allocation or static allocation. See the documentation for details regarding these two classes.

* If using JParserValFact, include all C files in the build, except JDecoder.c
* If using JDecoder, include all C files in the build, except JVal.c and define the macro NO_JVAL_DEPENDENCY when compiling the code. See the comment at the top of the source file JEncoder.c for details.

The included example Makefile, which is configured for GCC, builds 3 libraries. See the Makefile for details.

NOTE: The C files in the src directory MUST be compiled by a C compiler and not by a C++ compiler. Files ending with .c must be compiled by a C compiler and files ending with .cpp must be compiled by a C++ compiler.



