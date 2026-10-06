make(x: i32) Result[i32, i32]:
    if x == 0:
        return Err(9)
    return Ok(x + 1)

main() i32:
    r = make(41)
    r is Ok(v) -> return v
    return 1
