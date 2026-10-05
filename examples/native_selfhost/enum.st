enum ErrCode:
    Empty
    Explicit = 7
    Next

make() Result[i32, ErrCode]:
    return Err(ErrCode.Explicit)

main() i32:
    r = make()
    r is Err(e) -> return 0
    return 1
