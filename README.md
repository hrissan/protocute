# protocute

## About Protobuf and protocute

Protobuf is well-known somewhat poorly designed data exchange format.

Besides problems with design, it has really awful C++ implementation. 

Not only average-sized `protobuf` files compile into *megabytes* of garbage-like c++ code, they are useless without linking to the `protobuf` library...

What's problem with linking? Many. Two most obvious is compiler options and library paths. If you build for many platforms, and 2 versions for some platforms (32-bit and 64-bit), linking to 3-d party library quickly becomes really tedious error-prone task. Another problem is that sometimes libraries compile so that `.a` and `.so` files end up in the same folder, which on some platforms cannot be selected during build - `-lprotobuf` will often select wrong one, making wrong static/dynamic linking choice.

We know a good example of projects which can be used without linking, like `sqlite` or `lmdb`. You drop a single source and single header file into your project and you are done, it is always compiled with the same options as the rest of the project, and you have no problem with library paths whatsoever.

BTW `protobuf` is just a byte-manipulation code, so it should be immediately easily portable.

We have an example of `protobuf` python implementation, which produces beautiful minimalistic code.

`protocute` is an experimental attempt to make minimalistic portable implementation of `protobuf`, so that using `protobuf` from C++ becomes a simple task.

It is being implemented step by step as more complicated `.proto` definitions are required in my own project (integration with 3-d party server which uses `protobuf` as an exchange format).

If you like `protocute` but it lacks some `protobuf` feature you need - just add an issue and I will try to look into it.

## How `protocute` compiles various features of `.proto` files

### package

```
package test123;
```
C++
```
namespace test123 { 
```

### enum

```
enum Side {
    LEFT  = 0;
    RIGHT = 1;
    UNKNOWN_SIDE = 99;
}
```
C++
```
enum class Side {
	LEFT = 0,
	RIGHT = 1,
	UNKNOWN_SIDE = 99
};
```

### message

```
message Ints {
    optional int32 value1 = 1;
    repeated uint64 value2 = 2 [packed = true];
    optional Side side = 3;
}
```
C++
```
struct Ints { 
	int32_t value1 = 0;
	std::vector<uint64_t> value2;
	Side side = ::test123::Side::LEFT;
};
```

`string` is compiled into `std::string`, integer types to fixed-size integers,
`repeated` is compiled into `std::vector`.


### optional

```
message Point {
    optional int64 x = 1;
    optional int64 y = 2;
    optional int64 z = 3 [ protocute.optional = true ];
}
```
C++
```
struct Point { 
	int64_t x = 0;
	int64_t y = 0;
	std::optional<int64_t> z;
};
```

`optional` and `required` are ignored, `protocute.optional` can be added to get `std::optional`


### oneof

```
message Something {
    oneof position {
        string name = 5;
        string name_alt = 6;
        Point pos = 7;
    }
}
```
C++
```
struct Something { 
	std::variant<std::monostate,
		std::string,
		std::string,
		Point
	> position;
	enum position_num {
		i_name = 1,
		i_name_alt = 2,
		i_pos = 3
	};
};
```

`std::variant` is generated, plus `enum` mapped to variant index (if identical types are used, you can only set and get by index).

`Enum` contains field index in `std::variant`, not field number used as an identifier on wire (your code does not need to know that).

So in your code you can access particular field as 
```
auto pos = std::get<Something::i_pos>(something.position);

something.emplace<Something::i_name>("Example");
```

### default values

For now, `protocute` does not read `default` option from `.proto` definitions, and default value is selected based on field type.
This is actually what Google recommends for all new `.proto` definitions.

### encode/decode

```
void read(::test123::Point & v, iterator s, iterator e);
std::string write(const ::test123::Point & v);
```

Every `enum` and `message` will get the following pair of functions in `protocute` namespace generated,
where `iterator` is defined as `typedef const char * iterator;` so that any memory buffer can be decoded
without resorting to template code (for `std::string str`, just use `read(v, str.data(), str.data() + str.size());`.

## Protobuf design deficiencies, that we cannot fix

### Optional is broken

There is a distinction between optional on wire and optional in code, which is broken in Protobuf.

Let's start with simple struct `Point`.

```
struct Point {
	int x = 0;
	int y = 0;
	int z = 0;
}
```

We wish to save traffic, so we make fields `optional` on the wire, omitting some or all fields when we save.

We save x, y, z only if their value is not default, so the point `Point{3, 5, 0}` will be saved on wire as `{x=3, y=5}`.

What happens when the point is read back from the wire?

```
Point p; // x = 0, y = 0, z = 0
p.x = 3;
p.y = 5;
```

So we get exactly the same point which we saved.

This design is extendable. For example, we later modify `Point` to contain projective coordinate `t`.

```
struct Point {
	int x = 0;
	int y = 0;
	int z = 0;
	int t = 1;
}
```

If we read the previous version of the point `{x=3, y=5}` to the new point, we will get `Point{3, 5, 0, 1}`.

If we save the point with `t == 1` by the new code and read it with the old code, we will get exactly same point as before.

If we save the point with `t != 1` by the new code and read it with the old code, we will get `x, y, z` and lose `t`.

But if we mark fields `optional` in `Protobuf`, compiler will make them optional in the code, breaking intent and making working with the struct much harder.
 
Something like that (google uses its own wrapper instead of `boost::optional` but this does not matter for this discussion)
```
struct PointGoogle {
	std::optional<int> x;
	std::optional<int> y;
	std::optional<int> z;
	std::optional<int> t;
}
```

This is not what we want. And if we mark those fields as `required` we cannot read points of old version from the new code, because they contain no `t`.

So actual good design would be that all fields are optional on the wire, and the word `optional` would be used rarely to generate optional fields in structs.

`protocute` would just generates simple fields with no regard to `optional` keyword.

`protocute` would generate `if(v != default_v)` for saving `optional` fields, and would save `required` fields independent of their value.

If actual `optional` field is required, just add `[ protocute.optional = true ];` to the required field.


### Signed is broken

On the wire, there is `varint` type (`field_type == 0`), which is interpreted differently depending on the field type.

Let's make a demo first.

```
syntax = "proto2";
package test;

message Tint32 {
    optional int32 value = 1;
}

message Tsint32 {
    optional sint32 value = 1;
}

message Tuint32 {
    optional uint32 value = 1;
}

message Tint64 {
    optional int32 value = 1;
}

message Tsint64 {
    optional sint32 value = 1;
}

message Tuint64 {
    optional uint32 value = 1;
}
```

Let's see how values of `4` and `-4` are saved (unsigned value cannot have value of -4, so marked with `impossible`).

value  |  4      |   -4 
 ---   |    ---  |   --- 
int32  |  04     | fc ff ff ff ff ff ff ff ff 01 
uint32 |  04     | impossible   
sint32 |  08     | 07
int64  |  04     | fc ff ff ff ff ff ff ff ff 01 
uint64 |  04     | impossible
sint64 |  08     | 07

Ok, looks like we have actually 2 different representations of `4`, and 2 different representation of `-4`.
Moreover we have not way to know if signed or unsigned value was saved.

This means, that if we had `uintXX`, we cannot change it into `sintXX`, because all saved values will be doubled when read from wire. Reverse is also true.

Also, if we had `uint64` field with `2^64-1` value and later changed field type to `int64`, we would always get silent conversion to `-1` during reading.

### Fixed types and packed option are somewhat broken

After observation that large values require more bytes in varint, then in fixed coding, `protobuf` team added fixed types.

First problem with that is `fixed32` has the same format on wire as `float` (same with `fixed64` and `double`), so changing `fixed32` field to `float` and back is impossible, because all saved values (even so mundane as `4`) will be broken after read.

Second problem is with changing `fixed32` into `sfixed64`. Values large than `2^31` will be broken on read.

Third problem is with repeated fields with `packed = true` option.

As the field type is lost when `packed = true` is used, the content of the packed bytes will be interpreted according to the declared type, so if you change `int` type to `fixed32`, all saved values (even so mundane as `4`) will be broken after read.

## Some observations on how `protobuf` library behaves (tests made for version 3.6.1)

`fixed` values on the wire are never even attempted to be read into `varint` fields and vice verse, so that changing field type between `fixed` and `varint` will not work anyway.

also `fixed32` will never be attempted to be read as `fixed64` and vice versa.

Of cause, repeated `fixed32` field with size `2 * x` will be happily read into repeated `fixed64` field of size `x`...

## Improving design

Largest 64-bit value in varint encoding actually stores 70 bit of data, more than enough to store actual value, with explicit sign.

So actual values from `-2^63` to `2^64-1` will be stored. When reading, check will be performed if the actual value can be assigned to destination type.
Negative values obviously cannot be assigned to unsigned ints, and very large unsigned values cannot be assigned to signed ints.

In case the silent conversion is desired for some field instead of error, the field could be marked with an option `autoconvert = true` or similar.

Similar technique will not work for `fixed`, by design there is no place to distinguish between signed and unsigned values, so best idea would be to allow it only with `packed = true` repeated fields, as an additional option `fixed = true`.

Type of the original field would be saved in the first byte of `packed header` (`varint` `uint32` `int32` `uint64` or `int64`)

So that again no value would be silently converted if cannot be stored in the destination type during read (`autoconvert = true` can also be used here).

With this design we need just 4 normal integer types `uint32` `uint64` `int32` and `int64`, not 10.

If 128-bit or larger values are required, design is easily extended without losing compatibility with buffers saved by previous versions unaware about longer types.