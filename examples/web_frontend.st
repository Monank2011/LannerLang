static message: string = "Stable is running in the browser"

onTick() void:
    Web.log("timer callback fired")

onFrame(t: f64) void:
    Web.log("animation frame")

onClick() void:
    Web.setText("#status", "Button clicked from Stable")

onFetch(status: i32, data: *mut u8, len: usize) void:
    Web.log("fetch callback fired")
    Web.setText("#status", "Fetch callback completed")
    Web.freeBuffer(data)

main() i32:
    Web.log(message)
    Web.setText("#status", message)
    Web.setAttribute("#status", "data-language", "stable")
    Web.addClass("#status", "stable-ready")
    Web.addEventListener("#run", "click", "onClick")
    Web.setTimeout("onTick", 50)
    Web.requestAnimationFrame("onFrame")
    Web.queueMicrotask("onTick")
    Web.fetchText("/data.txt", "onFetch")
    return Web.queryCount("body")
