extern unsafe qsort(base:*mut u8, count:usize, size:usize, cmp:unsafe fn(*u8,*u8)i32) void

unsafe compare(a:*u8,b:*u8) i32:
    x:i32=*(a as *i32)
    y:i32=*(b as *i32)
    return x-y

main() i32:
    values:[4]i32=[4,1,3,2]
    unsafe:
        qsort((&raw values[0]) as *mut u8,4,4,compare)
    a:i32=values[0]
    b:i32=values[1]
    c:i32=values[2]
    d:i32=values[3]
    return a*1000+b*100+c*10+d
