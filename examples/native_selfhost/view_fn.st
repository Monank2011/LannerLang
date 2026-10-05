sumView(v: View[i32]) i32:
    return v[0] + v[1]

identityView(v: View[i32]) View[i32]:
    return v

main() i32:
    xs: [2]i32 = [5, 8]
    view: View[i32] = identityView(xs[:])
    return sumView(view)
