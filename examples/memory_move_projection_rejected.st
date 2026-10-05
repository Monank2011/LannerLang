Box[data: []i32]

main() i32:
    b = Box[data: [4, 5]]
    xs = b.data
    return xs[1]
