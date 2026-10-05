fn42() i32:
    return 42

static Fn:fn()i32=fn42

main() i32:
    f:fn()i32=Fn
    return f()
