enum ErrorCode:
    Empty = 3

Item[data: []i64]

makeItems(ok: bool) Result[[]Item, ErrorCode]:
    items: []Item = []Item
    if ok:
        items.push(Item[data: [1 as i64, 2 as i64, 3 as i64]])
        return Ok(items)
    return Err(ErrorCode.Empty)

main() i32:
    result: Result[[]Item, ErrorCode] = makeItems(true)
    result is Ok(items) -> return items[0].data[1] as i32
    return 1
