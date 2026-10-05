total(xs: []i64) i64:
    return xs[0] + xs[1] + xs[2]

main() i32:
    xs = []i64
    xs.push(10)
    xs.push(20)
    xs.push(30)
    t: i64 = total(xs)
    return t as i32
