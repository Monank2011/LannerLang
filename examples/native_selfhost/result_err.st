make(x: bool) Result[i32, i32]:
    if x:
        return Ok(7)
    return Err(9)

main() i32:
    r = make(false)
    r is Err(e) -> return e
    return 1
