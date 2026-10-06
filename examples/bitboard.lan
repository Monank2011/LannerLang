popcountMask(mask: u64) u64:
    x: u64 = mask
    x = x ^ (x >> 32)
    return x & 0xFFFFFFFFFFFFFFFF

main() i32:
    value = popcountMask(0x8000000000000001)
    return (value & 1) as i32
