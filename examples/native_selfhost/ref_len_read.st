sumAll(xs: &[]i64) i64:
    return xs[0] + xs[1] + xs[2]

main() i32:
    xs = []i64
    xs.push(5)
    xs.push(6)
    xs.push(7)
    return sumAll(&xs) as i32
