main() i32:
    Mobile.log("Lanner mobile core started")
    const w: i32 = Mobile.screenWidth()
    const h: i32 = Mobile.screenHeight()
    const d: f64 = Mobile.deviceScale()
    if w >= 0:
        return 0
    return 1
