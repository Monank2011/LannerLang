push3(xs: &mut []i64) void:
    xs.push(1)
    xs.push(2)
    xs.push(3)

fill(xs: &mut []i64) void:
    push3(&mut xs)
    xs.push(4)

main() i32:
    xs = []i64
    fill(&mut xs)
    return (xs[0] + xs[1] + xs[2] + xs[3] + xs.len()) as i32
