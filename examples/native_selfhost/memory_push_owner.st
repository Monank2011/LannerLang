Box[data: []i32]

main() i32:
    inner: []i32 = []i32
    inner.push(5)
    b = Box[data: inner]
    boxes: []Box = []Box
    boxes.push(b)
    return boxes[0].data[0]
