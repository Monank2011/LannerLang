enum ErrCode:
    Empty

make(x: i32) Result[i32, ErrCode]:
    if x == 0:
        return Err(ErrCode.Empty)
    return Ok(x + 1)

main() i32:
    r = make(41)
    r is Ok(v) -> return v
    return 1
