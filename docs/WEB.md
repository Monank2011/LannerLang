# Lanner Web Frontend

Lanner can target browser WebAssembly with `lanner --web`.

## Build

```text
lanner app.lan --web -O3 -o app.wasm
```

This produces:

```text
app.wasm
app.js
```

The generated JavaScript file is an ES module loader. In a browser it uses
`WebAssembly.instantiateStreaming()` when possible and falls back to buffered
instantiation when the response is not streamable.

## JavaScript interop

Lanner `extern` functions are emitted as WebAssembly imports. The generated
loader accepts an optional second argument containing host functions:

```lanner
extern host_add(a: i32, b: i32) i32

main() i32:
    return host_add(19, 23)
```

```javascript
import {loadLanner} from "./app.js";
const lanner = await loadLanner("./app.wasm", {
    host_add: (a, b) => a + b
});
console.log(lanner.exports.main());
```

## Browser namespace

`Web` is available only for a wasm32 browser build:

```lanner
Web.log("hello")
Web.setText("#status", "hello from Lanner")
Web.setTimeout("tick", 1000)
Web.requestAnimationFrame("frame")
Web.addEventListener("#button", "click", "clicked")
Web.queueMicrotask("microtask")
Web.fetchText("/data.txt", "loaded")
```

Callback names are checked when they are string literals. The compiler checks
arity, parameter types, and `void` return type for the callback ABI.

Web callbacks use ordinary browser event-loop scheduling. `fetchText` starts an
asynchronous Fetch operation and later invokes the named Lanner callback with
`status`, a raw byte pointer, and byte length. The callback owns the returned
buffer and can call `Web.freeBuffer()` when finished.

## Runtime boundary

Browser builds are freestanding and do not link the hosted Lanner runtime.
The loader supplies the minimal linear-memory services needed by the language,
including `malloc`, `realloc`, `free`, `memset`, `memcpy`, `memmove`, and
`memcmp`, plus the browser APIs above.

Hosted namespaces such as `Stdin`, `Clock`, `Thread`, and `Cpu` are rejected
for browser targets. Use the `Web` APIs or explicit JavaScript imports instead.

The browser target is currently `wasm32`; native WebAssembly component-model,
WASI, WebGPU, and language-level `async`/`await` lowering remain separate
future language/runtime work.
