Node[data: []i32]

main() i32:
    arena = Arena.create(256)
    nodes = [Node[data: [10]], Node[data: [20]]] in arena
    return nodes[0].data[0] + nodes[1].data[0]
