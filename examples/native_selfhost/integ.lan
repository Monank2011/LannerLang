Stats[count: i64, total: i64]

fib(n: i64) i64:
    if n < 2:
        return n
    return fib(n - 1) + fib(n - 2)

fill(xs: &mut []i64, n: i64) void:
    i: i64 = 0
    while i < n:
        xs.push(fib(i))
        i = i + 1

summarize(xs: &[]i64) Stats:
    total: i64 = 0
    i: i64 = 0
    while i < xs.len():
        total = total + xs[i]
        i = i + 1
    return Stats[count: xs.len(), total: total]

main() i32:
    xs = []i64
    fill(&mut xs, 10)
    s = summarize(&xs)
    if s.count == 10:
        return s.total as i32
    return 1
