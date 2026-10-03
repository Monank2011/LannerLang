Stable Language Reference

This is the concise reference for the Stable 1.0.0 language surface currently implemented by the compiler.
1. Program structure
A Stable program is composed of top-level:
function declarations
struct declarations
enum declarations
compile-time declarations
Blocks are indentation-based.
Example:
main() i32:
    print("Hello, STABLE!")
    return 0
2. Function syntax
name(parameter: Type, parameter: Type) ReturnType:
    ...
Example:
add(a: i32, b: i32) i32:
    return a + b
3. Variable bindings
Inference:
x = 42
Explicit type:
x: i64 = 42
Constant:
const x = 42
Compile-time:
comptime x = 42
4. Primitive types
void
bool
string

i8
i16
i32
i64
isize

u8
u16
u32
u64
usize

f32
f64
5. Compound types
Fixed array
[N]T
Dynamic array
[]T
Optional
T?
Result
Result[T, E]
Shared reference
&T
Exclusive reference
&mut T
Shared range view
View[T]
Exclusive range view
EditView[T]
Arena
Arena
6. Numeric literals
Examples:
42
0x2A
0b101010
3.5
-0.9
The compiler propagates expected numeric context through unary + and -.
7. Strings
Example:
message = "Hello, STABLE!"
print(message)
Current string operations include:
string.len()
stringLen(string)
8. Arithmetic
+
-
*
/
%
9. Bitwise operations
&
|
^
~
<<
>>
10. Logical operations
&&
||
!
11. Comparisons
==
!=
<
>
<=
>=
12. Casts
Explicit conversion:
x as i64
x as i32
x as f64
Conversions are checked according to the current type system.
13. Conditional control flow
if condition:
    ...
if condition:
    ...
else:
    ...
if condition:
    ...
else if other:
    ...
else:
    ...
14. While loops
while condition:
    ...
15. For loops
for item in collection:
    ...
The current type checker accepts arrays and views as iteration sources.
Loop-held borrows participate in normal ownership rules.
16. Loop control
break
continue
17. Return
return value
or:
return
for void functions.
18. Structs
Point[x: i32, y: i32]
Construct:
p = Point[x: 10, y: 20]
Access:
x = p.x
Mutate:
p.x = 42
19. Enums
enum ErrorCode:
    Empty
    Invalid = 7
Use:
ErrorCode.Empty
20. Optionals
x: i32? = none
An ordinary value can be lifted contextually into its Optional type where supported.
21. Results
Result[i32, ErrorCode]
Create:
Ok(42)
Err(ErrorCode.Empty)
Guard:
r is Ok(v) -> return v
or:
r is Err(e) -> return 1
22. Fixed arrays
xs: [4]i32 = [1, 2, 3, 4]
Compile-time sizing:
comptime N = 4

xs: [N]i32 = [1, 2, 3, 4]
Indexed access:
x = xs[2]
Indexed mutation:
xs[2] = 99
Fixed arrays do not support push().
23. Dynamic arrays
xs = [1, 2, 3]
Append:
xs.push(4)
Length:
n = xs.len()
Empty test:
empty = xs.isEmpty()
24. Slices
Range slicing:
part = xs[1:4]
The resulting value may be used as a View[T] where required.
25. References
Shared:
fn_name(x: &i32) ...
Exclusive:
fn_name(x: &mut i32) ...
A mutable reference can be used to modify its pointee:
inc(x: &mut i64) void:
    x = x + 1
26. Views
view: View[i32] = xs[0:3]
Read:
x = view[1]
Length:
n = view.len()
A View[T] is non-owning and read-only.
27. EditViews
An exclusive mutable view allows modification of its represented range.
The compiler prevents conflicting accesses to the backing storage while the edit view remains active.
28. Arenas
Create:
arena = Arena.create(256)
Allocate a value/aggregate in a region:
nodes = [Node[data: [10]], Node[data: [20]]] in arena
Arena-backed values remain associated with the region for lifetime and escape checking.
29. Comptime
Example:
comptime N = 8
Compile-time expressions currently include:
integer expressions
float expressions
string expressions
boolean expressions
They may be used to specialize compile-time-known values such as fixed-array dimensions.
30. Built-in output
print("hello")
print(42)
print(true)
print(3.5)
Raw output primitives are available for the bootstrap/runtime layer:
writeStdout
writeRaw
writeIntRaw
writeByteRaw
printInt
31. File/environment builtins
data = readFile("input.txt")
name = getEnv("NAME")
length = stringLen("hello")
The method form:
length = "hello".len()
is also supported.
32. Bounds checks
Index operations are checked by default.
Invalid indexing traps rather than silently accessing unrelated memory.
33. Ownership examples
Move:
main() i32:
    xs = [1, 2, 3]
    ys = xs
    return ys[0]
The old xs owner is no longer valid after the move.
34. Borrow conflict
Conceptually invalid:
xs = [1, 2, 3]
view = xs[:]

xs.push(4)
when view remains live and the mutation could invalidate it.
35. Const values
Constants cannot be modified through normal assignment or mutation operations.
36. Compile-time and runtime distinction
A comptime value is resolved during compilation.
A normal value exists during program execution.
Stable can therefore specialize source constructs without requiring programmers to manually duplicate code paths.
37. Current language boundaries
The 1.0.0 implementation does not yet claim all future features.
Not part of the current complete language surface:
general-purpose user-defined generic functions
exhaustive match syntax
modules and package manager
general safe FFI
concurrency primitives
atomics
channels
SIMD/vector abstractions
full comptime reflection/function system
full PGO framework
integrated debugger/LSP
These are tracked in ROADMAP.md.
