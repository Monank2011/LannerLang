enum ErrCode:
    Empty

make(ok: bool) Result[[]i32, ErrCode]:
    xs = []i32
    if ok:
        xs.push(40)
        xs.push(2)
        return Ok(xs)
    return Err(ErrCode.Empty)

main() i32:
    r = make(true)
    r is Ok(v) -> return v[0] + v[1]
    return 1
