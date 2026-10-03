Stable Benchmarking

Stable is benchmarked for both performance and bug discovery.
The project treats benchmarks as compiler torture tests, not only as stopwatch contests.
Method
Where practical:
same source algorithm
same LLVM/Clang toolchain
same optimization level
same hardware
repeated timing
result verification
sanitizer validation where appropriate
The benchmark harness is designed to prevent incorrect outputs from being counted as performance wins.
Step 1: Basic computation
The initial scalar-computation benchmark covered:
integer mixing
bitwise operations
branches
integer division/modulo
floating-point arithmetic
floating-point branches
function calls
integer/floating conversion
signed arithmetic
recursion
The first pass found real compiler issues involving:
f32 LLVM literal emission
typed negative f32 literals
typed negative i64 literals
These were repaired and turned into regression tests.
Step 2: Basic memory
The next test covered:
scalar references
mutable references
fixed-array mutation
dynamic arrays
nested aggregates
ownership transfer
View[T]
EditView[T]
nested memory structures
The test found:
scalar &mut T assignment incorrectly targeting the reference binding instead of its pointee
string.len() lowering missing from backend paths
Both were repaired and regression-tested.
Step 3: Heavy memory
The heavy-memory campaign exercised:
large dynamic allocations
repeated reallocation
allocator churn
nested ownership
arena growth
multiple large arrays
large views
EditView mutation
repeated owner movement
repeated lifecycle destruction
The stress tests reached roughly:
1.37 GiB peak RSS
in the largest red-zone arena workload.
Sanitizer validation
Heavy tests were run under:
AddressSanitizer
UndefinedBehaviorSanitizer
LeakSanitizer
The tested Stable and C++ workloads produced no sanitizer findings in the completed heavy benchmark set.
This is evidence from the tested workloads, not a mathematical proof that every future compiler program is bug-free.
Heavy-memory timing snapshot
Representative median results from the heavy suite showed Stable and C++ in the same general performance class, with Stable ahead on several ownership/reallocation workloads and C++ ahead on others.
The purpose of the benchmark is not to claim universal superiority.
The important release criterion is:
correctness
+
memory safety
+
performance
+
reproducibility 
