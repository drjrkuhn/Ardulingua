# Ardulingua

Library for JSON-RPC-like communication between arduino and Micro-manager device drivers

# Json Dispatch communication scheme

Simplified JSON-RPC scheme designed for fast lookup/access on microcontrollers. Similar to the [JSON-RPC 2.0 specification](https://www.jsonrpc.org) but with the following changes

- NO `{"jsonrpc": "2.0"}` tag at the beginning of the message
- NO named parameters, only positional parameters
- "m" short for "method"
- "p" short for "params"
- "i" short for "id"
- "r" short for "result"
- errors are not returned as a {code,message} named tuples,
  just the numeric code.
  - instead of "error": {"code": -32600, "message": "..."}
  - error returned as  "e": -32600
- remote methods with void returns just the message id
- Uses [ArduinoJson](https://arduinojson.org/) for JSON processing.
- Compiler switch allows a quick switch to a more compact [MessagePack](https://msgpack.org/) binary serial format. 
-  [SlipInPlace](#slipinplace) SLIP+NULL encoding allowing binary formats over string-like streams.
    - Unique terminator character at the end of each message
    - No `\0` (null) characters anywhere in the encoded message, so messages can be treated like standard C-strings.


## Example communication streams

RPC call with positional parameters
```
--> {"m": "subtract", "p": [42, 23], "i": 1}
<-- {"r": 19, "i": 1}
```

RPC call with 'void' return [SET]
```
--> {"m": "setfoo", "p": [42], "i": 2}
<-- {"i": 2}
```

RPC call with return and no parameters [GET]
```
--> {"m": "getfoo", "i": 3}
<-- {"r": 42, "i": 3}
```

RPC notification (no id means no reply requested)
```
--> {"m": "update", "p": [1,2,3,4,5]}
--> {"m": "foobar"}
```

RPC call with error return
```
--> {"m": "subtract", "p": [42], "i": 3}
<-- {"e": -32600, "i": 3}
```

RPC set-notify/get get pair [SETN-GET]
```
--> {"m": "setfoo", "p": [3.1999]}
--> {"m": "gettfoo", "i": 4}
<-- {"r": 3.2, "i": 4}
```

# Function delegates

The library contains a set of Arduino/C++11 compatible generic function delegates and stubs. There is a separate set of delegates and stubs designed for dispatching on ArduinoJson documents.

Delegates are based on ["The Impossibly Fast C++ Delegates"](https://www.codeproject.com/articles/11015/the-impossibly-fast-c-delegates) by Sergey A Kryukov.

> ** WARNING **
> Stubs and delegates do not use smart_pointers and have no destructors. They do not check if an object or function has gone out-of-context. Use them for permanent (top-level) or semi-permanent objects and functions that last the lifetime of the program. If you need function delegates with delete, move, etc, use std::function found in the STL header <functional>

# Remote properties

Remote properties are layered on top of this compact JSON-RPC protocol. This scheme is designed specifically for micro-manager Device Properties.

## Briefs and Property Codes

Property get/set/sequencing rely on JsonDelegate method names with special prepended codes. The remote property name should be a short, unique character sequence like "prop". We will call this abbreviation of the property name the property `brief`.

We prepend a single-character opcode to the property brief to denote a standard property operation. The server is responsible for dispatching the coded brief to the appropriate function. A possible mechanism is detailed below, but dispatch tables are flexible and can use pure callback functions, class methods, or lambda with capture.

> NOTE: codes are prepended rather than appended to make string matching terminate earlier during dispatch map method string lookup. `brief` method tags should be kept to a few characters for the same reason. Even a fast hash-map will need to loop through the entire string to compute the hash-value for lookup. So use a brief method tag like "dacv" rather than "MyDACOutputValueInVolts".

|opcode| operation                          |meth[^1]| server signature[^2]                      |
|:----:|:-----------------------------------|:-----:|:-------------------------------------------|
|  ?   | GET value                          | get   | `call<T,EX...>("?brief",ex...)->T`           |
|  !   | SET value                          | set   | `call<void,T,EX...>("!brief",t,ex...)`       |
|  !   | NSET value - no reply              | set   | `notify<void,T,EX...>("!brief",t,ex...)`     |
|  *   | ACT task                           | act   | `call<void,EX...>("*brief",ex...)`           |
|  *   | NOTIFY task (DO without response)  | act   | `notify<void,EX...>("*brief",ex...)`         |
|  --  |       SEQUENCE/ARRAY COMMANDS      | --    | --                                         |
|  ^   | GET maximum size of seq array[^3]  | array | `call<long,EX...>("^brief",ex...)->long`     |
|  #   | GET number of values in seq array  | array | `call<long,EX...>("#brief",ex...)->long`     |
|  0   | CLEAR seq array                    | array | `notify<long,EX...>("0brief",ex...)->dummy`  |
|  +   | ADD value to sequence array        | set   | `notify<void,T,EX...>("+brief",ex...)`       |
|  *   | ACT task doubles as start seq.     | act   | `call<void,EX...>("*brief",ex...)`           |
|  *   | NOTIFY task to start seq.          | act   | `notify<void,EX...>("*brief",ex...)`         |
|  \~  | STOP sequence                      | act   | `call<void,EX...>("\~brief",ex...)`          |
|  \~  | STOP sequence                      | act   | `notify<void,EX...>("\~brief",ex...)`        |

[^1]: meth is the client method whose parameters match the call/notify signature
[^2]: Signature of the server method. T is the property type on the device, EX... are an 
optional set of extra parameters such as channel number
[^3]: Micro-manager makes several calls to GET maximum sequence size. Maximum sequence size is checked only once and the value is cached by the device driver.

## Client transform/dispatch methods

From the signature table above, we need four local method signatures for transforming MM Properties into eventual RPC calls on the server. The client method might also transform the MM::PropertyType into the type T required by the server. Each method type includes an optional set of compile-time extra parameters such as channel number, pin number, etc. What the server does with this information depends on the method opcode.

- get: gets the remote property value
- set: sets the remote property value
- act (action): performs some task associated with the property and check status. Can be called or notified.
- array: (array actions) gets either the current or maximum array size or clears the array

## Set/Get pair and volatile remote properties

The normal 'SET' call method doesn't return the value actually set on the remote device - just an OK (returns caller id) or error number. If we want to verify and retrive the value that was actually set to the device We can use a NSET then GET. A normal SET-GET RPC pair would need to wait for two replies, one from the SET call, one from the GET call. Instead we can use NSET (notify-SET, i.e. no reply) followed immediately by a GET call.

A **volatile** remote property can cange behind-the-scenes. We cannot rely on a cached value and the remote property might not preserve the exact value of a SET operation. Volatile properties must:
- Always Use NSET-GET pairs when setting
- Always use GET and never use cached values.

## Sequences and array value streaming (notify)

For sequence arrays, the client can send a stream of array notifications Use `0prop` to clear then a string of `+prop`, `+prop`, ... to add values to the array in order. Array setting can be speedy because the client doesn't need to wait for replies to each array optimizatoin. Instead, the client can check on the progress every, say, 20th value with a `#prop` GET call. The results should match the number of  array values sent so far. A final `#prop` GET call at the end can verify the array was properly filled. The number of consecutive values to send safely will depend on the size of the transmission buffer and the MessagePack'd size of each json notification call.

Clients should first send a `^prop` GET call to query the maximum array size on the remote device.

## Server decoding

Lambda methods in the server's dispatch map can make the process of routing opcodes simpler. The server can hard-code each coded method call with a series of key/lambda function pairs. 

For example in pseudo-code, a property with a value and possible sequence might be coded as lambda captures (pseudocode):
```cpp
map<String,function> dispatch_map {
    {"?prop", call<int>(     [&prop_]()->int            { return prop_; })},
    {"!prop", call<void,int>([&prop_](int val)          { prop_ = val; })},
    {"^prop", call<int>(     [&pseq_max_]()->int        { return pseq_max_; })},
    {"#prop", call<int>(     [&pseq_count_]()->int      { return pseq_count_; })},
    {"0prop", call<int>(     [&pseq_count_]()->int      { pseq_count_ = 0; return 0; })},
    {"+prop", call<void,int>([&pseq_,&acount_](int val) { pseq_[pseq_count_++] = val; })}
};
```
Future version of remote dispatch might incorporate auto-decoding dispatch for sequencable/array properties at the server level.

## Extra parameters

Properties may have extra call parameters for routing the command to the appropriate place. For example, a short `channel` parameter might be used to route a property to the appropriate DAC channel or a `pin` parameter could indicate a digital I/O pin to set.

The MM driver client is responsible for populating these extra values during the `call` dispatch command and the server is responsible for decoding the extra parameter and taking the appropriate action. The client/server RPC dispatch mechanism has variadic template arguments for precisely this reason!

## Transforimg properties

Some properties need different types for the client and server. For example, the client MM device might want to set analog ouput as a floating-point number, but the remote device DAC only takes 16-bit integers. Or the client device uses `state` strings but the remote device expects numeric `enum` state values.

A transforming property allows the client device to add a lambda function (with possible [&] captures) to transform the property from the client type to the required server type on-the-fly before sending the remote property.


# SlipInPlace

An Arduino-compatible (C++11) Serial Line Internet Protocol ([SLIP](https://datatracker.ietf.org/doc/html/rfc1055)) encoder and decoder.

SlipInPlace can perform both in-place and out-of-place encoding and decoding of SLIP packets. This is useful for fixed size buffer.

## SLIP Encoding 
The normal SLIP protocol encodes a packet of bytes with unique `END` code at the end of a stream and a guarantee that the `END` code will **not** appear anywhere else in the packet. The standard SLIP code table is below.

| Char Code     | Octal   | Hex    | encoded as             |
|:--------------|--------:|-------:|------------------------|
| END           | `\300`  | `0xC0` | `\333\334`, `0xDBDC`   |
| ESC           | `\333`  | `0xDB` | `\333\335`, `0xDBDD`   |
| EscapedEND    | `\334`  | `0xDC` | _never alone_          |
| EscapedESC    | `\335`  | `0xDD` | _never alone_          |

Before adding the `END` code to the end of the stream, SLIP searches for any existing characters matching `END` and replaces them with a two-character `ESC-EscapedEND` codelet. It also has to guarantee that there are no spurious `ESC` codes in the original data. So it also searches for `ESC` codes in the original data and replaces them with a two-character `ESC-EscapedESC` codelet.

The protocol then adds an `END` code at the end of the packet that serial handlers can search for as the 'terminator' character in an incoming stream. The terminator (`END`) is guaranteed only to appear at the end of a packet.

Decoding works in reverse by shrinking the packet and replacing the escaped codes by their non-escaped originals.

## SLIP+NULL Encoding

This library has another SLIP+NULL encoder that replaces `END` and `ESC` as above, but _also_ searches for and replaces any `NULL` characters (`\0`) in the middle of the packet with a two-byte `ESC-EscapedNULL` sequence. That way we can guarantee no `NULL` bytes in the encoded packet. Serial communication handlers [^5] that only deal with C-strings (ending in `\0`) can safely handle data containing the byte zero after the packet has been encoded to eliminate any internal zeros. 

[^5]: I'm looking at you, [MicroManager](https://micro-manager.org/) device driver `GetSerialAnswer()`.


| Char Code     | Octal   | Hex    | encoded as             |
|:--------------|--------:|-------:|------------------------|
| NULL          |  `\000` | `0x00` | `\333\336`, `0xDBDE`   |
| EscapedNULL   |  `\336` | `0xDE` | _never alone_          |


## Standard encoding

The standard SLIP encoder and decoder are pre-defined. Two simple use-cases are below

Out-of-place encoding and decoding:

```C++
char ebuf[16]; 
const char* source = "Lo\300rus"; // Note END in middle of string
// encoding
size_t esize = slip::encoder::encode(ebuf, 16, source, strlen(source));
// ebuf == "Lo\333\334rus\300"; esize == 8;

// decoding
char dbuf[16]; 
size_t dsize = slip::decoder::decode(dbuf, 16, ebuf, esize);
// dbuf == "Lo\300rus"; dsize == 6;

string final(dbuf, dsize);
// final == "Lo\300rus";
```

In-place encoding and decoding:
```C++
char buffer[16]; 
const char* source = "Lo\300rus"; // Note END in middle of string
strcpy(buffer, source);

// encoding
size_t esize = slip::encoder::encode(buffer, 16, buffer, strlen(source));
// buffer == "Lo\333\334rus\300"; esize == 8;

// decoding
size_t dsize = slip::decoder::decode(buffer, 16, buffer, esize);
// buffer == "Lo\300rus"; dsize == 6;

string final(buffer, dsize);
// final == "Lo\300rus";
```

Communication protocols are usually byte oriented rather than character oriented. In C and C++ `char` can also encode UTF-8 strings with two-byte characters. The default SLIP encoder/decoder pairs work with `unsigned chars` (`uint8_t`) and includes additional `encode()` and `decode()` functions that translate `char*` as `unsigned char*` via `reinterpret_cast<>`.

You can declare a char encoder or decoder that works with `chars` (`uint8_t`) through `slip_decoder_base` and `slip_encoder_base`

```C++
//=== With 'typedef' directives
typedef slip::encoder_base<char> char_encoder;
typedef slip::decoder_base<char> char_decoder;

//=== OR with 'using' directives
using encoder = slip_encoder_base<char>;
using decoder = slip_decoder_base<char>;

```

## Other encodings

Codes are defined at compile time using template parameters. For example, `\test\hrslip.h` defines a human-readable SLIP coded used for almost all algorithm and unit testing. 

```C++
typedef slip::encoder_base<char,'#','^','D','[','0','@'> encoder_hrnull;
typedef slip::decoder_base<char,'#','^','D','[','0','@'> decoder_hrnull;
```

The resulting codelets (in template order)

| Char Code     | Char  | encoded as        |
|:--------------|------:|-------------------|
| END           |  `#`  | `^D`              |
| ESC           |  `^`  | `^[`              |
| EscapedEND    |  `D`  | _never alone_     |
| EscapedESC    |  `[`  | _never alone_     |
| NULL          |  `0`  | `^@`              |
| EscapedNULL   |  `@`  | _never alone_     |

So for example

```C++
const size_t bsize = 10;
char buffer[32];
char* source = "Lo^#rus";

// encoding
size_t esize = encoder_hrnull::encode(buffer,32,source,strlen(source));
//> ebuf == "Lo^[^Drus#"; esize == 10;
// Original ESC^ is now ^[, END# is now ^D and END# only at terminus

// decoding
size_t dsize = decoder_hrnull::decode(buffer,32,buffer,esize);
//> dbuf == "Lo^#rus"; dsize == 7;
```

The `\examples\sip_encode` folder contains a similar human-readable encoding that prints encoded and decoded streams on the Arduino's serial port. 

(You can get a glimpse of how in-place _vs_ out-of-place encoding works by looking at the diagnostic buffer outputs.)

### Tests and Examples

The encoding and decoding libraries have unit tests of various scenarios. See the `\tests` directory for Unit tests.

See `\examples` for Arduino sample sketches.
