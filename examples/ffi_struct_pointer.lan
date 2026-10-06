Pair[x:i32, y:i32]
extern inspectPair(p:*Pair) i32

main() i32:
    p:Pair=Pair[x:11,y:31]
    unsafe:
        return inspectPair((&raw p) as *Pair)
