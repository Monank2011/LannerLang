bad() []i32:
    xs = []i32
    xs.push(1)
    view: &[]i32 = &xs
    return view

main() i32:
    return bad()[0]
