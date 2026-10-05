main() i32:
    xs: [2]i32 = [5, 8]
    view: EditView[i32] = xs[:]
    view[0] = 21
    return xs[0]
