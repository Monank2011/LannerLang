Box[data: []i32]

main() i32:
    xs = [1, 2]
    first = Box[data: xs]

    ys = [3, 4]
    second = Box[data: [5, 6]]
    second.data = ys

    inner = [7, 8]
    boxes = []Box
    boxes.push(Box[data: inner])

    return first.data[1] + second.data[0] + boxes[0].data[1]
