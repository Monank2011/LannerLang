make(x: bool) Result[[]i32, i32]:
    xs = []i32
    if x:
        xs.push(40)
        xs.push(2)
        return Ok(xs)
    return Err(9)

main() i32:
    r = make(true)
    r is Ok(v) -> return v[0] + v[1]
    return 1
