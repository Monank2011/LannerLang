packed PackedPair[a:u8, b:u32]
static align(64) section ".lanner.rodata" Words:[4]u64=[1,2,4,8]

unsafe increment(p:*mut i32) i32:
    *p=*p+1
    return *p

main() i32:
    x:i32=41
    pair:PackedPair=PackedPair[a:7,b:9]
    unsafe:
        p:*mut i32=&raw x
        p2:*mut i32=p.add(1)
        distance:isize=ptrDiff(p2,p)
        f:unsafe fn(*mut i32)i32=increment
        v:i32=f(p)
        raw:*mut u8=allocAligned(16,16)
        memset(raw,0,16)
        unalignedStore(raw,7)
        byte:u8=unalignedLoad(raw)
        address:usize=raw as usize
        raw2:*mut u8=address as *mut u8
        if raw2==null:
            return 0
        deallocAligned(raw)
        machine:i32=asmI32("movl $$42, $0", "=r")
        total:i32=(v as i32)+(byte as i32)+machine
        sz:i32=sizeOf(pair) as i32
        return total+sz+(distance as i32)-59
