main() i32:
    arena = Arena.create(64)
    ys = [3, 4] in arena
    ys.push(5)
    return ys[0] + ys[2]
