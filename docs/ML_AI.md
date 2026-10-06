# ML / AI

Lanner's native ML layer is designed around explicit tensor ownership, predictable native layouts, and an ABI that can bind optimized external compute libraries without a mandatory Python runtime.

## Tensors

`Tensor` supports f32/f64 data, ranks 1-8, shape/stride queries, element access, reshape, transpose, slicing, cloning, reductions, elementwise arithmetic, scaling, ReLU, sigmoid, tanh, softmax, dot products, argmax, matrix multiplication, and a CPU NCHW convolution kernel. `Tensor.slice()` returns an owned tensor copy; it does not introduce hidden reference counting. `Tensor.argmax(axis)` is currently a scalar reduction for rank-1 tensors with axis 0.

Examples: `examples/ml_tensor.lan` and `examples/ml_autodiff.lan`.

## Autodiff

`Grad.create()` creates an explicit tape. `Grad.watch()` registers tensors, and the tape supports add, multiply, matrix multiplication, ReLU, tanh, sum, scaling, backward propagation, and gradient retrieval. The tape is explicitly freed and therefore has no hidden GC/RC requirement. Tensors referenced by a tape must remain alive until `Grad.free()`; the tape stores explicit tensor handles rather than retaining them implicitly.

## Optimized libraries

Lanner already provides direct C ABI interop and `--link`, so BLAS/LAPACK, oneDNN, Accelerate, MKL, cuBLAS, CUDA/HIP runtimes, and other native libraries can be bound as ordinary `extern` declarations. `Accel.blasAvailable()`, `Accel.cudaAvailable()`, `Accel.rocmAvailable()`, `Accel.metalAvailable()`, and `Accel.backend()` provide lightweight host discovery.

The bundled CPU tensor kernels are dependency-free reference/portable kernels. Projects that need vendor-tuned kernels should bind the vendor library through FFI rather than forcing a heavyweight dependency into the Lanner runtime.

## Python interoperability

Python remains an optional host ecosystem rather than a compiler requirement. Lanner's FFI can call CPython C APIs or native extension libraries when a project needs Python interop, while pure Lanner binaries remain independent of a Python installation.

## Design boundary

Tensor handles are explicit runtime resources. Callers keep source tensors alive while an autodiff tape records operations, and call `free()` when those resources are no longer needed. This keeps resource lifetime visible rather than silently introducing tracing GC or reference counting into the Lanner language.
