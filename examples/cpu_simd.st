static WORDS: [4]u64 = [1, 2, 4, 8]

main() i32:
    v: v256 = Cpu.loadV256(&WORDS[0])
    doubled: v256 = v.add(v)
    a: u64 = doubled.extractU64(0)
    b: u64 = doubled.extractU64(1)
    return (a + b) as i32
