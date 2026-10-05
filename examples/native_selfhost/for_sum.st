main() i32:
    sum: i32 = 0
    xs: []i32 = [10, 20, 30]
    for x in xs:
        sum = sum + x
    return sum
