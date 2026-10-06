static PIECE_VALUE: [6]i32 = [100, 320, 330, 500, 900, 20000]
static KNIGHT_ATTACKS: [64]u64 = [132096, 329728, 659712, 1319424, 2638848, 5277696, 10489856, 4202496, 33816580, 84410376, 168886289, 337772578, 675545156, 1351090312, 2685403152, 1075839008, 8657044482, 21609056261, 43234889994, 86469779988, 172939559976, 345879119952, 687463207072, 275414786112, 2216203387392, 5531918402816, 11068131838464, 22136263676928, 44272527353856, 88545054707712, 175990581010432, 70506185244672, 567348067172352, 1416171111120896, 2833441750646784, 5666883501293568, 11333767002587136, 22667534005174272, 45053588738670592, 18049583422636032, 145241105196122112, 362539804446949376, 725361088165576704, 1450722176331153408, 2901444352662306816, 5802888705324613632, 11533718717099671552, 4620693356194824192, 288234782788157440, 576469569871282176, 1224997833292120064, 2449995666584240128, 4899991333168480256, 9799982666336960512, 1152939783987658752, 2305878468463689728, 1128098930098176, 2257297371824128, 4796069720358912, 9592139440717824, 19184278881435648, 38368557762871296, 4679521487814656, 9077567998918656]
comptime MAX_PLY = 6

countKnightMoves(knights:u64, own:u64) i32:
    bb:u64 = knights
    total:i32 = 0
    while bb != 0:
        s:u32 = Cpu.ctz(bb) as u32
        one:u64 = 1
        bb = bb & (bb - one)
        legal:u64 = KNIGHT_ATTACKS[s as usize] & ~own
        total = total + (Cpu.popcount(legal) as i32)
    return total

search(depth:i32, knights:u64, own:u64) i32:
    if depth <= 0:
        return (Cpu.popcount(knights) as i32) * PIECE_VALUE[1]
    moves:i32 = countKnightMoves(knights, own)
    if moves == 0:
        return -PIECE_VALUE[5]
    bb:u64 = knights
    first:u32 = Cpu.ctz(bb) as u32
    one:u64 = 1
    fromMask:u64 = one << (first as u64)
    destinations:u64 = KNIGHT_ATTACKS[first as usize] & ~own
    to:u32 = Cpu.ctz(destinations) as u32
    nextKnights:u64 = (knights & ~fromMask) | (one << (to as u64))
    return moves * 3 - search(depth - 1, nextKnights, own)

main() i32:
    knights:u64 = (1 as u64) << 1 | (1 as u64) << 6
    own:u64 = (1 as u64) << 1
    score:i32 = search(MAX_PLY, knights, own)
    if score == -20000:
        return 1
    if countKnightMoves(knights, own) < 2:
        return 2
    if Cpu.popcount(knights) != 2:
        return 3
    return 42
