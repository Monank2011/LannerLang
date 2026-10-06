comptime INPUTS = 45192
comptime ACC = 16
comptime HIDDEN = 32

nnueEval(weights: Buffer) i32:
    accum: [ACC]i32 = [10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25]
    features: [8]usize = [0, 64, 127, 4096, 8191, 16384, 32000, 45191]
    unsafe:
        base: *i8 = weights.data() as *i8
        firstOffset: usize = (ACC as usize) * 2
        fi: usize = 0
        while fi < features.len():
            f: usize = features[fi]
            if f < INPUTS:
                p: *i8 = base.add((firstOffset + f * (ACC as usize)) as isize)
                lane: usize = 0
                while lane < (ACC as usize):
                    w: i8 = *p.add(lane as isize)
                    accum[lane] = accum[lane] + (w as i32)
                    lane = lane + 1
            fi = fi + 1
        hiddenOffset: usize = firstOffset + (INPUTS as usize) * (ACC as usize)
        h: usize = 0
        out: i32 = 0
        while h < (HIDDEN as usize):
            sum: i32 = *(base.add((hiddenOffset + h * 4) as isize) as *i32)
            lane2: usize = 0
            while lane2 < (ACC as usize):
                a: i32 = accum[lane2]
                if a < 0:
                    a = 0
                if a > 127:
                    a = 127
                w: i8 = *(base.add((hiddenOffset + (HIDDEN as usize) * 4 + lane2 * (HIDDEN as usize) + h) as isize) as *i8)
                sum = sum + a * (w as i32)
                lane2 = lane2 + 1
            ow: i16 = *(base.add((hiddenOffset + (HIDDEN as usize) * 4 + (ACC as usize) * (HIDDEN as usize) + 4 + h * 2) as isize) as *i16)
            out = out + (sum / 127) * (ow as i32)
            h = h + 1
        out = out + *(base.add((hiddenOffset + (HIDDEN as usize) * 4 + (ACC as usize) * (HIDDEN as usize)) as isize) as *i32)
        return out

main() i32:
    b: Buffer = FS.read("tests/data/nnue_audit_weights.bin")
    if b.len() == 0:
        return 1
    score: i32 = nnueEval(b)
    b.free()
    writeIntRaw(score as i64)
    if score != 3:
        return 1
    return 42
