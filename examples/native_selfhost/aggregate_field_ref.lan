Meta[x: i64]
Box[id: i64, vals: []i64, meta: Meta]

main() i32:
    b = Box[id: 7, vals: []i64, meta: Meta[x: 5]]
    b.meta.x = 20
    vals: &mut []i64 = &mut b.vals
    vals.push(10)
    vals.push(30)
    return (b.id + b.meta.x + b.vals[0] + b.vals[1]) as i32
