# **LannerLang 1.0.0 Handbook**
> **The practical language guide**

Lanner was created and is developed by **Monank Gohil**, who began developing the language at age **15**.

A medium-detail introduction to the LannerLang programming language and its V1 capabilities.

## **1. WHAT IS LANNER?**

Lanner is a general-purpose native programming language designed for:

- predictable native performance
- compile-time ownership and borrow safety
- deterministic destruction
- explicit memory costs
- low-level control when needed
- LLVM-based optimization
- indentation-based, readable syntax

Lanner does not require a tracing garbage collector for ordinary programs and does not silently add reference counting to normal values.

Lanner is useful for systems software, servers, numerical/AI programs, web applications compiled to WebAssembly, mobile native cores, games, command-line tools, and high-performance programs such as chess engines and NNUE inference.

Important Version 1 boundary:
The published compiler itself is still primarily implemented in C++. Lanner has a real recursive self-hosting bootstrap for a supported compiler subset, but full compiler self-hosting is planned for a later version.

## **2. YOUR FIRST LANNER PROGRAM**

Lanner uses indentation-based blocks. A block follows a colon and is indented.

main() i32:
    print("Hello, Lanner!")
    return 0

Save as hello.lan.

Build:

    lanner hello.lan -o hello

Run:

    ./hello

You can also run a temporary executable with:

    lanner hello.lan --run

Check types without producing a normal executable:

    lanner hello.lan --check

Print LLVM IR:

    lanner hello.lan --emit-llvm

## **3. BASIC SYNTAX**

### **3.1 Variables**

Type inference:

x = 42
name = "Lanner"
ready = true

Explicit types:

x: i64 = 42
ratio: f64 = 3.5

Constants:

const answer: i32 = 42

Compile-time values:

comptime N = 4

Use comptime values when a value should be known during compilation, for example for fixed-array sizes.

### **3.2 Literals**

Integers:

42
0x2A
0b101010

Floating point:

3.5
-0.9

Boolean:

true
false

Optional none value:

none

String:

"hello"

### **3.3 Comments**

The V1 language reference does not define a comment syntax as part of the core grammar. Keep source examples in the documented grammar rather than assuming C/C++ comment syntax.

### **3.4 Function syntax**

add(a: i32, b: i32) i32:
    return a + b

main() i32:
    value = add(20, 22)
    return value

Functions can be referenced independent of textual declaration order in the supported production compiler, and mutual recursion is supported.

### **3.5 Operators**

Arithmetic:

+  -  *  /  %

Bitwise:

&  |  ^  ~  <<  >>

Logical:

&&  ||  !

Comparison:

==  !=  <  >  <=  >=

Casts:

x as i64
value as f64

## **4. TYPES**

### **4.1 Integer types**

i8   i16   i32   i64   isize
u8   u16   u32   u64   usize

Use fixed-width integers for exact data formats, bitboards, network packets, model weights, and binary files.
Use isize/usize for target-sized indexing and pointer-distance style operations.

### **4.2 Floating point**

f32
f64

### **4.3 Other built-ins**

bool
string
void

### **4.4 Compound types**

[N]T          fixed-size array
[]T           dynamic array
T?            optional value
Result[T,E]   success/error result
&T            shared read-only reference
&mut T        exclusive mutable reference
View[T]       shared read-only range view
EditView[T]   exclusive mutable range view
Arena         region allocator/owner

### **4.5 Structs**

Point[x: i32, y: i32]

main() i32:
    p = Point[x: 20, y: 22]
    return p.x + p.y

Fields participate in the ownership and borrow rules.

### **4.6 Enums**

enum ErrorCode:
    Empty
    Invalid = 7

### **4.7 Optionals**

x: i32? = none

Optionals represent a value that may be absent.

### **4.8 Results**

result: Result[i32, ErrorCode]

Constructors:

Ok(value)
Err(error)

Guards can destructure them:

r is Ok(v) -> return v
r is Err(e) -> return 1

## **5. CONTROL FLOW**

Conditionals:

if x > 0:
    return 1
else if x == 0:
    return 0
else:
    return -1

While loop:

while running:
    work()

For loop:

for item in xs:
    use(item)

Loop controls:

break
continue

## **6. ARRAYS, STRINGS, AND COLLECTIONS**

### **6.1 Fixed arrays**

comptime N = 4
xs: [N]i32 = [1, 2, 3, 4]

Fixed arrays support construction, indexing, mutation, passing, returning, references, and struct fields.

Indexing is bounds checked unless the compiler can prove the access safe.

### **6.2 Dynamic arrays**

xs = [1, 2, 3]
xs.push(4)
print(xs.len())

Dynamic arrays support:

- ownership transfer
- indexing and mutation
- length and empty checks
- growth through push
- slicing and views
- deterministic cleanup

Dynamic storage is subject to invalidation rules. Lanner will reject a potentially relocating mutation while a conflicting borrow/view is live.

### **6.3 String helpers**

Common string operations used by the V1 runtime include:

len()
equals(...)
equalsIgnoreCase(...)
startsWith(...)
contains(...)
find(...)
parseInt(...)
parseFloat(...)
parseU64At(...)

String and buffer indexing uses the documented bounds-checked path.

## **7. MEMORY MODEL: THE MAIN IDEA**

Lanner's central rule is:

    An owning value has one responsible owner.
    References and views borrow storage and do not own it.

### **7.1 Owned values**

T

An ordinary value is owned. For non-copy resources, moving transfers ownership.

### **7.2 Move semantics**

When a non-copy value moves from one owner to another, the old owner can no longer be used as though it still owns the resource.

This protects against use-after-move and double destruction.

### **7.3 Shared references**

&T

A shared reference is:

- non-owning
- read-only
- usable alongside other compatible shared borrows

Example:

read(v: &i64) i64:
    return v

### **7.4 Mutable references**

&mut T

A mutable reference is:

- non-owning
- writable
- exclusive over the relevant storage

Example:

inc(v: &mut i64) void:
    v = v + 1

### **7.5 Inferred borrow lifetimes**

Lanner does not require explicit lifetime parameters in ordinary code. The compiler determines how long a borrow is needed and rejects escapes that would outlive the source storage.

### **7.6 Views**

View[T] is a shared, read-only, non-owning range.
EditView[T] is an exclusive, mutable, non-owning range.

Typical slice form:

a[lo:hi]

Views do not become owners of the backing storage.

### **7.7 Arenas**

An Arena owns a region of allocations that share a lifetime.

arena = Arena.create(256)

Arenas are useful for:

- parsers
- compiler structures
- temporary graphs
- request-scoped data
- search structures
- bulk-lifetime objects

Arena-backed references and views cannot escape their region illegally.

## **8. SAFE AND UNSAFE SYSTEM PROGRAMMING**

Most Lanner code remains inside the safe ownership system. Low-level escape-hatch operations require explicit unsafe code.

### **8.1 Raw pointers**

Lanner supports:

*T
*mut T

Raw-pointer facilities include:

&raw x
pointer add/sub/offset
ptrDiff(pointerA, pointerB)
raw pointer and integer casts
null
raw dereference

### **8.2 Allocation**

alloc
realloc
dealloc
allocAligned
deallocAligned
stackAlloc

Memory utility functions:

memset
memcpy
memmove
memcmp

Volatile and unaligned operations:

volatileLoad
volatileStore
unalignedLoad
unalignedStore

### **8.3 Layout information**

sizeOf(value-or-type)
alignOf(value-or-type)
offsetOf(...)

Lanner also supports packed structures and explicit alignment/section/TLS storage controls.

Example:

packed Packet[opcode:u8, flags:u8, value:u32]

static align(64) section ".lanner.rodata" Words:[4]u64=[1,2,4,8]

### **8.4 Unsafe functions**

unsafe increment(p:*mut i32) i32:
    *p = *p + 1
    return *p

Unsafe functions remain marked unsafe when stored in function-pointer types.

### **8.5 Inline assembly**

Lanner supports explicit LLVM-backed assembly forms including:

asm
asmI32
asmI64
asmPtr

Assembly requires unsafe use and passes templates/constraints to LLVM.

### **8.6 Freestanding and cross-target compilation**

Important CLI options:

--target=<triple>
--sysroot=<path>
--cpu <name>
--features <f1,f2,...>
--linker <name>
--linker-script <path>
--entry <symbol>
--freestanding
--no-runtime

Lanner can emit objects or assembly for a selected target without requiring a hosted Lanner runtime. Final firmware/kernel linking still depends on the platform toolchain, sysroot, linker, startup code, and hardware ABI.

## **9. CPU FEATURES, SIMD, THREADS, AND ATOMICS**

### **9.1 CPU feature checks**

Cpu.hasAvx2()
Cpu.hasAvx512()
Cpu.hasSse42()
Cpu.hasBmi2()
Cpu.hasPopcnt()

Cpu.has(...) can query a named feature where supported by the implementation.

### **9.2 Low-level CPU operations**

Cpu.rdtsc()
Cpu.popcount(x)
Cpu.ctz(x)
Cpu.clz(x)
Cpu.bswap(x)
Cpu.pext(x, mask)
Cpu.pdep(x, mask)
Cpu.prefetch(&value)

BMI2 operations should be guarded by Cpu.hasBmi2() when code must run on CPUs without BMI2.

### **9.3 Fixed-width vector types**

Lanner exposes v128, v256, and v512 value types and load/store/zero helpers such as:

Cpu.loadV128(...)
Cpu.loadV256(...)
Cpu.loadV512(...)
Cpu.zeroV128()
Cpu.zeroV256()
Cpu.zeroV512()

Vector values expose operations such as xor and extraction helpers.

### **9.4 Threads**

worker() void:
    work()

main() i32:
    t: Thread = Thread.spawn(worker)
    t.join()
    return 0

Available thread operations include:

Thread.spawn
Thread.join
Thread.detach
Thread.yield
Thread.hardwareConcurrency

### **9.5 Atomics**

Atomic[T] provides explicit atomic storage.

static mut Nodes: Atomic[u64] = 0

Nodes.fetchAdd(1)
count = Nodes.load()
Nodes.lanore(0)
Nodes.compareExchange(expected, desired)

Atomic load/store/fetch/CAS calls accept optional memory-order strings such as relaxed, acquire, release, acq_rel, and seq_cst where valid for the operation. Lanner also provides atomic and compiler fences.

## **10. STATIC/GLOBAL STORAGE**

Use static storage for tables and process-wide state.

static PIECE_VALUES: [8]i32 = [100, 320, 330, 500, 900, 20000, 0, 0]
static mut Nodes: Atomic[u64] = 0

Static initializers are required to be compile-time values, keeping program startup deterministic and avoiding hidden initialization functions for lookup tables.

V1 also supports explicit alignment, custom native sections, mutability, and thread-local static storage.

## **11. C FFI**

Lanner can call native C ABI functions explicitly.

extern abs(x: i32) i32

main() i32:
    return abs(-42)

Link additional libraries or objects with:

lanner ffi.lan --link path/to/object.o -o ffi
lanner ffi.lan --link -lm -o ffi

The V1 FFI supports C-compatible scalar, pointer, string-pointer, function-pointer forms, and external globals. Complex by-value C aggregates are intentionally restricted; use an explicit pointer-based ABI or C wrapper when the native ABI needs platform-specific aggregate coercions.

Function pointers and callbacks can be used through the explicit unsafe/ABI model.

## **12. COMPTIME**

Lanner supports compile-time integer, floating-point, boolean, and string evaluation.

Example:

comptime N = 2 + 2
xs: [N]i32 = [10, 20, 30, 40]

Comptime evaluation detects errors such as overflow and division by zero when those can be determined during compilation.

Use comptime for:

- fixed-size tables
- specialized algorithms
- compile-time configuration
- generated constants
- lookup tables
- invariant checking

## **13. STANDARD HOSTED RUNTIME**

The hosted runtime is implemented in C and provides the small native ABI layer used by Lanner applications. It does not imply that Lanner's values are garbage collected or reference counted.

Basic I/O helpers include print-style functions and raw stdout output.

Examples:

print("hello")
print(42)
print(true)
print(3.5)
writeRaw("ready\n")

## **14. UCI / CHESS-ENGINE RUNTIME**

Lanner has a focused native engine surface, but chess is only one use case.

Stdin:

Stdin.readLine()
Stdin.hasInput()

Clock:

Clock.monotonicNanos()
Clock.deadlineAfterNanos(duration)
Clock.remainingNanos(deadline)
Clock.expired(deadline)
Clock.sleepNanos(duration)

These primitives are suitable for interactive protocol engines and iterative-deepening search.

Example shape:

while running:
    line: string = Stdin.readLine()
    if line.equals("quit"):
        running = false

The repository contains examples/uci_engine.lan and chess/NNUE audit fixtures.

## **15. BACKEND AND CLOUD PROGRAMMING**

### **15.1 Networking**

Net provides TCP and UDP primitives, including:

Net.tcpListen
Net.accept
Net.recv
Net.sendString
Net.setNonblocking
Net.udpOpen
Net.udpBind
Net.udpRecv
Net.udpSendTo
Net.close

### **15.2 Scalable readiness polling**

Poller provides readiness-based I/O multiplexing.

Poller.create
Poller.add
Poller.remove
Poller.wait
Poller.count
Poller.readEvents
Poller.eventMask
Poller.destroy

Linux uses epoll; hosted non-Linux systems use the available portable readiness mechanism.

### **15.3 Synchronization**

Mutex:

Mutex.create
Mutex.lock
Mutex.tryLock
Mutex.unlock
Mutex.destroy

RwLock:

RwLock.create
RwLock.readLock
RwLock.writeLock
RwLock.unlock
RwLock.destroy

Condvar:

Condvar.create
Condvar.wait
Condvar.destroy

Semaphore:

Semaphore.create
Semaphore.wait
Semaphore.tryWait
Semaphore.post
Semaphore.destroy

### **15.4 Processes**

Process.run
Process.spawn
Process.pid
Process.wait
Process.terminate
Process.output
Process.setEnv
Process.argCount
Process.arg

### **15.5 HTTP**

Http.get
Http.post
Http.lanatus

The V1 client handles bounded HTTP/1.1 operations including Content-Length, connection-close responses, and chunked response decoding.

TLS is deliberately not hidden in the core runtime. Bind a native TLS implementation through FFI or build a Lanner library around one.

### **15.6 JSON**

Json.validate
Json.quote
Json.int
Json.float
Json.bool
Json.nullValue

### **15.7 Buffers**

Buffer is an owned growable byte buffer used by filesystem, HTTP, JSON, process, Web, mobile, and ML code.

Construction and operations include:

Buffer.new(capacity)
Buffer.fromString(text)
Buffer.len()
Buffer.data()
Buffer.cstr()
Buffer.appendString(text)
Buffer.appendBuffer(other)
Buffer.free()

`data()` returns a raw mutable byte pointer and therefore belongs in unsafe code.

## **16. DEVOPS AND SCRIPTING**

Lanner can be used for command-line tools and compiled scripts.

Script execution:

lanner tool.lan --script -- arg1 arg2

The arguments after -- become the script's process arguments.

The same forwarding style is supported by --run.

### **16.1 Arguments**

Args.count()
Args.at(index)

### **16.2 Environment**

Env.has(name)
Env.get(name)
Env.set(name, value)
Env.unset(name)

### **16.3 Filesystem**

FS.exists
FS.isFile
FS.isDir
FS.fileSize
FS.read
FS.write
FS.append
FS.writeBuffer
FS.copy
FS.rename
FS.remove
FS.mkdir
FS.rmdir
FS.list
FS.cwd
FS.chdir

### **16.4 Paths**

Path.join
Path.equals
Path.basename
Path.dirname
Path.extension
Path.stem
Path.normalize
Path.isAbsolute
Path.absolute

The V1 Path API is intentionally small. Path helper results may use temporary/native scratch storage; copy results into owned Lanner storage when they need to survive subsequent helper calls.

### **16.5 Regex**

Regex.compile
Regex.isMatch
Regex.find
Regex.free

The bundled regex engine is deliberately small. Supported constructs include literals, ., character classes, negated classes, anchors, escapes, and common *, +, ? quantifiers.

### **16.6 Shell**

Shell.run(command)
Shell.output(command)
Shell.which(program)

Use Process APIs when you need explicit process lifecycle management rather than a one-shot shell command.

## **17. WEB FRONTEND / WEBASSEMBLY**

Build a browser-oriented WebAssembly module with:

lanner app.lan --web -O3 -o app.wasm

The compiler produces:

app.wasm
app.js

The generated JS module performs browser WebAssembly loading and provides the host boundary.

### **17.1 JavaScript FFI**

extern host_add(a: i32, b: i32) i32

main() i32:
    return host_add(19, 23)

JavaScript can provide the imported function.

### **17.2 Browser API surface**

Web.log
Web.warn
Web.error
Web.setText
Web.setHtml
Web.setAttribute
Web.addClass
Web.removeClass
Web.remove
Web.queryCount
Web.focus
Web.setTimeout
Web.clearTimeout
Web.requestAnimationFrame
Web.cancelAnimationFrame
Web.queueMicrotask
Web.addEventListener
Web.removeEventListener
Web.fetchText
Web.freeBuffer

Lanner checks callback signatures when callback names are string literals where the compiler has that information.

### **17.3 Important Web boundary**

Browser builds are freestanding WebAssembly and do not link the hosted native runtime.
Hosted namespaces such as Stdin, Clock, Thread, and Cpu are rejected for browser targets.

V1 has browser event-loop APIs and asynchronous Fetch callbacks. Full language-level async/await coroutine lowering is not yet a completed V1 language feature.

## **18. MOBILE**

Lanner can generate native ARM application cores for Android and iOS and place the UI/platform boundary in the native mobile ecosystem.

### **18.1 Android targets**

arm64-v8a
aarmeabi-v7a
x86_64
x86

Example:

lanner app.lan --target=aarch64-linux-android24 --emit-object -o app.o

Project generation:

lanner app.lan --android-project MyAndroidApp --android-abi arm64-v8a --deployment 24 --mobile-name LannerMobile --bundle-id com.example.lannermobile

### **18.2 iOS targets**

aarm64 device
aarm64 Simulator

Project generation:

lanner app.lan --ios-project MyIOSApp --deployment 16.0 --mobile-name LannerMobile --bundle-id com.example.lannermobile

Use --ios-simulator for the arm64 Simulator.

### **18.3 Mobile API**

Mobile.log
Mobile.platform
Mobile.osVersion
Mobile.isSimulator
Mobile.screenWidth
Mobile.screenHeight
Mobile.deviceScale
Mobile.safeAreaTop
Mobile.safeAreaBottom
Mobile.safeAreaLeft
Mobile.safeAreaRight
Mobile.appDataPath
Mobile.documentsPath
Mobile.cachePath
Mobile.openUrl
Mobile.vibrate
Mobile.requestPermission
Mobile.clipboardSet
Mobile.clipboardGet
Mobile.cameraAvailable
Mobile.locationAvailable
Mobile.bluetoothAvailable

Architecture:

Lanner native core
    |
    +-- Android: native/JNI bridge -> Android UI/platform APIs
    |
    +-- iOS: C/Objective-C bridge -> SwiftUI/Apple frameworks

V1 does not attempt to replace the Android or Apple UI ecosystems. Lanner is the native high-performance core and explicit interop layer.

## **19. GAME DEVELOPMENT**

Game development is built on the same native/performance facilities as systems programming.

### **19.1 Game namespace**

The built-in handle types include GameWindow, GameRenderer, GameTexture, and GameAudio.

Game provides an SDL2-backed platform layer with:

- windows
- resize/fullscreen/high-DPI flags
- event polling
- keyboard state
- mouse state
- controller state and normalized axes
- accelerated/vsync-capable 2D rendering
- textures
- queued PCM audio
- frame timing
- sleep

Common calls include:

Game.createWindow
Game.destroyWindow
Game.poll
Game.shouldClose
Game.requestClose
Game.setTitle
Game.width
Game.height
Game.setVSync
Game.makeGLContext
Game.present
Game.windowFlags
Game.rendererFlags
Game.createRenderer
Game.destroyRenderer
Game.setDrawColor
Game.clear
Game.fillRect
Game.drawLine
Game.presentRenderer
Game.createTexture
Game.updateTexture
Game.copyTexture
Game.destroyTexture
Game.eventType
Game.eventCode
Game.eventX
Game.eventY
Game.eventText
Game.keyDown
Game.mouseButtonDown
Game.mouseX
Game.mouseY
Game.controllerConnected
Game.controllerAxis
Game.controllerButtonDown
Game.audioOpen
Game.audioWrite
Game.audioQueued
Game.audioPause
Game.audioClose
Game.timeNanos
Game.deltaSeconds
Game.sleepNanos

### **19.2 Graphics namespace**

Graphics.available
Graphics.backend
Graphics.loadProc
Graphics.glClearColor
Graphics.glClear
Graphics.glEnable
Graphics.glDisable
Graphics.glBindBuffer
Graphics.glUseProgram
Graphics.glDrawArrays
Graphics.glBindVertexArray
Graphics.glEnableVertexAttribArray
Graphics.glDeleteShader
Graphics.glDeleteProgram
Graphics.glViewport
Graphics.glGenBuffers
Graphics.glGenVertexArrays
Graphics.glBufferData
Graphics.glCreateShader
Graphics.glShaderSource
Graphics.glCompileShader
Graphics.glShaderStatus
Graphics.glShaderLog
Graphics.glCreateProgram
Graphics.glAttachShader
Graphics.glLinkProgram
Graphics.glProgramStatus
Graphics.glProgramLog
Graphics.glVertexAttribPointer
Graphics.glDeleteBuffers
Graphics.glDeleteVertexArrays
Graphics.glGetError

The low-level Graphics boundary can dynamically load native graphics procedures. This lets Lanner access larger APIs such as Vulkan, Metal, and Direct3D through explicit native interop without baking every vendor API into the language.

### **19.3 Game project generation**

lanner main.lan --game-project MyGame --game-name MyGame

The generated project includes a CMake build, the Lanner source, runtime source, and an assets directory.

### **19.4 Performance model**

There is no tracing GC or hidden frame allocator. Use arrays, arenas, references/views, SIMD, threads, atomics, raw pointers, and unsafe operations when the engine design genuinely requires them.

## **20. ML / AI**

Lanner's ML layer is explicit and native. It does not require Python to execute a pure Lanner program.

### **20.1 Tensor capabilities**

V1 Tensor supports:

- f32/f64 data
- ranks 1 through 8
- shape/stride queries
- element access through get1/get2/get3 and set1/set2/set3
- reshape2/reshape3/reshape4
- transpose2
- slicing
- cloning and contiguous conversion
- reductions: sum, mean, l2Norm
- elementwise add/sub/mul/div
- scaling
- ReLU
- sigmoid
- tanh
- softmax
- dot products
- argmax
- matrix multiplication
- CPU NCHW convolution

Constructors include zeros1/2/3/4, ones1/2/3/4, zerosF32/onesF32, zerosF64/onesF64, and unsafe from1F32/from1F64/from2F32/from2F64 forms for wrapping native data.

Tensor.slice() returns an owned tensor copy. It is not a hidden reference-counted view.

Tensor resources are explicitly freed.

### **20.2 Autodiff**

Grad creates an explicit tape. The V1 tape can watch tensors and record multiply, sum, tanh, and other implemented graph operations, then run reverse-mode backward propagation and return gradients.

Grad.create
Grad.watch
Grad.add
Grad.mul
Grad.matmul
Grad.relu
Grad.tanh
Grad.sum
Grad.scale
Grad.backward
Grad.grad
Grad.free

The tape has an explicit lifetime. Tensors used by the tape must remain alive until the tape is finished.

### **20.3 Optimized native libraries**

Use Lanner FFI and --link to connect to native numerical libraries such as BLAS/LAPACK and platform/vendor math libraries.

Accelerator discovery helpers include:

Accel.blasAvailable
Accel.cudaAvailable
Accel.rocmAvailable
Accel.metalAvailable
Accel.backend

Python can remain an optional host ecosystem. Lanner can bind native Python C APIs or extensions when a project actually needs Python interop, but Python is not required to run a pure Lanner binary.

## **21. LOW-LEVEL BINARY AND ABI WORK**

Lanner is suitable for code that needs exact data representations.

Use:

- fixed-width integers
- packed structs
- explicit alignment
- section placement
- thread-local storage
- raw pointers
- unaligned loads/stores
- volatile memory operations
- external C globals
- function pointers
- inline assembly

Example:

extern static mut errno: i32

This is useful for:

- binary file formats
- protocol parsers
- hardware-facing code
- operating-system interfaces
- optimized lookup tables
- engine memory layouts

## **22. BUILD SYSTEM AND COMPILER OPTIONS**

Basic:

lanner file.lan
lanner file.lan -o program
lanner file.lan --check
lanner file.lan --run
lanner file.lan --script
lanner file.lan --emit-llvm
lanner file.lan --emit-object
lanner file.lan --emit-asm

Targets and toolchain:

--target=<triple>
--sysroot=<path>
--cpu <name>
--features <f1,f2,...>
--linker <name>
--linker-script <path>
--entry <symbol>
--link <object-or-library>

Runtime modes:

--freestanding
--no-runtime

Specialized project/build modes:

--web
--android-project <dir>
--ios-project <dir>
--ios-simulator
--android-abi <abi>
--mobile-name <name>
--bundle-id <id>
--deployment <version>
--game-project <dir>
--game-name <name>
--game-backend <name>

Debug/inspection compatibility options also include HIR emission modes and the backend selection options documented by the compiler help output.

LLVM toolchain selection can use environment variables such as:

LANNER_CLANG=/path/to/clang lanner file.lan

or:

LLVM_CC=/path/to/clang lanner file.lan

The modern opaque-pointer LLVM IR path is supported with LLVM/Clang 15+ as documented by the project.

## **23. ERROR HANDLING AND SAFETY**

Lanner's preferred model is to make invalid ownership states compiler errors and normal absence/failure values explicit with optionals and Result values.

Safety checks include protection against:

- use-after-move
- double destruction
- dangling references/views
- conflicting exclusive access
- mutation through shared access
- unsafe dynamic-array relocation while a borrow is live
- illegal arena escapes
- out-of-bounds indexing

When low-level code intentionally leaves the safe model, mark the operation unsafe. The goal is not to ban unsafe programming; the goal is to make unsafe regions visible.

## **24. WHAT LANNER IS GOOD AT IN V1**

Systems:
Native code, manual memory, raw pointers, FFI, SIMD, assembly, freestanding objects, cross-target code.

Web:
Browser WebAssembly, JS imports, DOM helpers, timers, events, Fetch callbacks.

Backend/cloud:
TCP/UDP, epoll-style polling, threads, atomics, locks, processes, HTTP/1.1, JSON, buffers.

ML/AI:
Native tensors, math kernels, autodiff, BLAS/native accelerator binding.

Mobile:
Native Android/iOS ARM cores plus generated project/interop scaffolding.

Games:
SDL2 window/input/audio/2D runtime and low-level graphics integration.

DevOps/scripting:
Args, environment, filesystem, paths, regex, process/shell APIs, compiled script mode.

Chess/NNUE:
Bitboards, static tables, CPU intrinsics, UCI runtime, multithreading, timing, binary model loading, and a verified representative 45,192 -> 16 -> 32 -> 1 NNUE inference path are present in V1.

Chess is a demanding test workload, not Lanner's only purpose.

## **25. WHAT IS NOT COMPLETE IN V1**

Do not confuse "general-purpose V1" with "every ecosystem library already exists."

The following are intentionally outside the completed V1 core or remain limited:

- the full production compiler is still primarily C++
- full self-hosting of the complete compiler is a V2 goal
- the self-hosted compiler currently covers a supported subset
- language-level async/await coroutine lowering is not complete
- the browser target is currently wasm32 rather than a full Wasm component/WASI stack
- TLS is an external native library/FFI concern
- database drivers and gRPC are library/ecosystem work
- huge vendor-specific GPU object models are accessed through FFI/native procedures rather than baked into the compiler
- Python/ML ecosystem packages are optional integrations, not built into the language
- mobile UI is provided by Android/iOS native frameworks through generated bridges
- a full package registry/module ecosystem is a later maturity task

These are ecosystem or advanced compiler expansion areas, not reasons that ordinary V1 native applications cannot be built.

## **26. LEARNING PATH**

A good order for learning Lanner is:

## **1. Hello World and CLI options.**
## **2. Scalars, variables, operators, if/while/for.**
## **3. Structs, enums, Optionals, Result.**
## **4. Fixed and dynamic arrays.**
## **5. Ownership and moves.**
## **6. &T, &mut T, View[T], EditView[T].**
## **7. Arena allocation.**
## **8. Filesystem, processes, networking, and threads.**
## **9. Unsafe pointers and FFI only when needed.**
## **10. SIMD/CPU primitives for optimized workloads.**
## **11. Pick a domain: server, game, web, mobile, ML, systems, or engine development.**

Start in the safe subset. Reach for unsafe only when the algorithm or platform interface actually requires it.

## **27. USEFUL PROJECT EXAMPLES**

The V1 source tree contains representative programs, including:

examples/hello.lan
examples/bitboard.lan
examples/uci_engine.lan
examples/nnue_host_audit.lan
examples/chess_engine_kernel_audit.lan
examples/systems_low_level.lan
examples/ffi.lan
examples/atomic_threads.lan
examples/backend_cloud.lan
examples/backend_network.lan
examples/backend_poller.lan
examples/ml_tensor.lan
examples/ml_autodiff.lan
examples/ml_ffi_cblas.lan
examples/web_frontend.lan
examples/web_ffi.lan
examples/mobile_app.lan
examples/mobile_full.lan
examples/game_full.lan
examples/game_graphics.lan
examples/devops_script.lan

Use these as executable reference material. The compiler source and docs are also part of the repository.

## **28. DOCUMENTATION MAP**

For deeper details, read:

Language reference:
    docs/LANGUAGE.md

Memory and ownership:
    docs/MEMORY_MODEL.md

Systems/low-level:
    docs/SYSTEMS.md

Engine runtime:
    docs/ENGINE_RUNTIME.md

Backend/cloud:
    docs/BACKEND_CLOUD.md

Web:
    docs/WEB.md

ML/AI:
    docs/ML_AI.md

Mobile:
    docs/Mobile.md

Game development:
    docs/GAME_DEVELOPMENT.md

DevOps/scripting:
    docs/DEVOPS_SCRIPTING.md

Bootstrap:
    docs/BOOTSTRAP.md
    docs/BOOTSTRAP_AUDIT.md

V1 chess/NNUE audit:
    docs/CHESS_NNUE_AUDIT.md
    docs/V1_RELEASE_AUDIT.md

## **29. THE LANNER MINDSET**

Think about Lanner in three layers:

SAFE CORE
Values, ownership, references, views, arrays, structs, enums, Result/Optionals, control flow, comptime.

NATIVE POWER
Unsafe pointers, allocation, ABI, FFI, assembly, SIMD, atomics, threads, target control, freestanding builds.

DOMAIN LAYERS
Web, backend/cloud, ML/AI, mobile, games, DevOps, and specialized workloads such as chess engines and NNUE.

Use the safe core by default, make expensive or dangerous operations explicit, and use the low-level layer when the problem genuinely demands it.

## **30. VERSION 1 IN ONE SENTENCE**

Lanner 1.0 is a general-purpose, LLVM-native, ownership-safe language with a strong low-level escape hatch and a broad V1 runtime/toolchain surface; the next major milestone is replacing the remaining C++ compiler implementation with a fully bootstrapped Lanner compiler.

Welcome to Lanner.
