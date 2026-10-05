# Backend and Cloud Runtime

Stable provides a native backend runtime intended for long-running servers, network services, concurrent workers, and command-line service processes.

## Networking

`Net` exposes TCP and UDP sockets, connect/listen/accept, send/receive, nonblocking mode, TCP_NODELAY, shutdown, socket error reporting, and local/peer port queries.

`Poller` provides readiness-based multiplexing. Linux uses `epoll`; other hosted targets use the available portable readiness API. A poller returns ready socket handles and event masks.

## Synchronization

Stable provides `Mutex`, `RwLock`, `Condvar`, and `Semaphore`, plus the existing `Thread` and `Atomic[T]` primitives.

## Processes

`Process.run()` executes a command synchronously. `Process.spawn()` creates a real child-process handle without attaching an output pipe, `Process.pid()` reports its identifier, and `Process.wait()` reaps it. `Process.terminate()` requests termination. `Process.output()` is the separate output-capture API.

## HTTP

`Http.get()` and `Http.post()` implement bounded HTTP/1.1 client requests with connection-close handling, `Content-Length`, and chunked response decoding. `Http.post()` uses JSON as its content type. HTTPS is intentionally not hidden inside the runtime: TLS should be supplied through an explicit native TLS library via Stable FFI.

## JSON and buffers

`Json.validate()` validates JSON syntax, while `Json.quote()`, `Json.int()`, `Json.float()`, `Json.bool()`, and `Json.nullValue()` provide allocation-backed serialization primitives. `Buffer` owns growable byte storage with data, C-string, append, and length access.

## Browser boundary

The backend runtime is rejected for browser WebAssembly targets. Network, process, synchronization, and hosted HTTP APIs therefore cannot accidentally become unsupported browser imports.

## Design boundary

The backend layer is a native runtime foundation rather than a giant framework. Higher-level TLS, database, gRPC, and cloud-provider clients remain normal Stable libraries or explicit FFI bindings, keeping the language ABI and runtime small and predictable.
