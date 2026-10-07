# Lanner Game Development

Lanner 2.0.0 includes a native game-development layer designed around predictable native execution, explicit ownership, and low-level graphics access.

## Runtime layers

### `Game`

`Game` provides a lightweight SDL2-backed platform layer:

- native windows and resize/fullscreen/high-DPI flags
- event polling and quit state
- keyboard and mouse state
- game-controller state and normalized axes
- SDL2 accelerated or vsync-capable 2D renderer
- render colors, lines, rectangles, textures
- queued PCM audio
- monotonic frame timing and sleep

The runtime loads SDL2 dynamically. This keeps the Lanner compiler independent of SDL2 development headers while allowing a generated game project to ship its own runtime source.

### `Graphics`

`Graphics` is the explicit low-level GPU boundary. It provides:

- backend availability checks for Vulkan/OpenGL and platform graphics APIs
- native procedure loading
- OpenGL buffer, VAO, shader, program and draw wrappers
- raw pointer access for GPU memory transfer and object-generation APIs

Vulkan, Metal, and Direct3D can be accessed by loading their native procedures through `Graphics.loadProc()` and Lanner's existing unsafe FFI layer. Lanner does not bake a gigantic vendor-specific graphics object model into the language.

## Example loop

```lanner
main() i32:
    flags = Game.windowFlags(false, true, false, false)
    window = Game.createWindow("Lanner Game", 320, 240, flags)
    if Game.width(window) != 320:
        Game.destroyWindow(window)
        return 1
    renderer = Game.createRenderer(window, Game.rendererFlags(true, false))
    frame = 0
    while frame < 3:
        Game.poll(window)
        Game.setDrawColor(renderer, 18, 28, 48, 255)
        Game.clear(renderer)
        Game.setDrawColor(renderer, 220, 80, 60, 255)
        Game.fillRect(renderer, 20 + frame * 10, 30, 80, 50)
        Game.drawLine(renderer, 0, 0, 319, 239)
        Game.presentRenderer(renderer)
        Game.deltaSeconds()
        Game.sleepNanos(1000000)
        frame = frame + 1
    Game.destroyRenderer(renderer)
    Game.requestClose(window)
    Game.destroyWindow(window)
    return 0
```

See `examples/game_full.lan` for the regression version, including SDL2 audio initialization.

## Project generation

Generate a portable CMake game project with:

```sh
lanner main.lan --game-project MyGame --game-name MyGame
```

The generated project contains the Lanner source, a copy of the Lanner runtime, an `assets/` directory, CMake build files, and build scripts. The Lanner source is compiled to an object without the hosted runtime, then CMake links the generated runtime for the host platform.

## Performance model

Game code remains ordinary native Lanner code. There is no tracing garbage collector or hidden frame allocator. Arenas, arrays, views, raw pointers, atomics, threads, SIMD, and the explicit unsafe boundary remain available to engine code.

Use a fixed-timestep accumulator for deterministic simulation and `Game.deltaSeconds()` for render-time interpolation. Platform and graphics resources are represented as explicit opaque handles and must be destroyed by the program.

## Backend architecture

The intended layering is:

```text
Lanner game code
    |
    +-- Game       window/input/2D/audio/time
    |
    +-- Graphics  OpenGL + dynamic native GPU procedures
    |
    +-- unsafe FFI / raw pointers
    |
    +-- native OS + driver stack
```

This leaves large engine frameworks, ECS designs, physics solvers, asset formats, animation systems, and vendor-specific rendering engines as normal Lanner libraries or C/C++ bindings rather than compiler intrinsics.
