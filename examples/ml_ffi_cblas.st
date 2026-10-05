extern cblas_sdot(n: i32, x: *f32, incx: i32, y: *f32, incy: i32) f32

main() i32:
    a = Tensor.onesF32(4)
    b = Tensor.onesF32(4)
    unsafe:
        pa = a.dataF32()
        pb = b.dataF32()
        score = cblas_sdot(4, pa, 1, pb, 1)
    a.free()
    b.free()
    return 42
