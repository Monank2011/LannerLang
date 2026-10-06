
isSpace(c: u8) bool:
    return c == 32 || c == 9 || c == 13 || c == 10

isDigit(c: u8) bool:
    return c >= 48 && c <= 57

isIdentStart(c: u8) bool:
    return (c >= 65 && c <= 90) || (c >= 97 && c <= 122) || c == 95

isIdentContinue(c: u8) bool:
    return isIdentStart(c) || isDigit(c)

lineEnd(src: &[]u8, start: usize) usize:
    i: usize = start
    while i < src.len() && src[i] != 10 && src[i] != 13:
        i = i + 1
    return i

nextLine(src: &[]u8, start: usize) usize:
    i: usize = lineEnd(src, start)
    if i < src.len() && src[i] == 13:
        i = i + 1
        if i < src.len() && src[i] == 10:
            i = i + 1
    else if i < src.len():
        i = i + 1
    return i

indentOf(src: &[]u8, start: usize, end: usize) usize:
    i: usize = start
    spaces: usize = 0
    while i < end && (src[i] == 32 || src[i] == 9):
        if src[i] == 9:
            spaces = spaces + 4
        else:
            spaces = spaces + 1
        i = i + 1
    return spaces

trimStart(src: &[]u8, start: usize, end: usize) usize:
    i: usize = start
    while i < end && (src[i] == 32 || src[i] == 9):
        i = i + 1
    return i

trimEnd(src: &[]u8, start: usize, end: usize) usize:
    i: usize = end
    while i > start && (src[i - 1] == 32 || src[i - 1] == 9):
        i = i - 1
    return i

startsWord(src: &[]u8, start: usize, end: usize, kind: i64) bool:
    if kind == 1:
        if end - start < 5:
            return false
        if src[start + 0] != 101:
            return false
        if src[start + 1] != 108:
            return false
        if src[start + 2] != 115:
            return false
        if src[start + 3] != 101:
            return false
        if src[start + 4] != 58:
            return false
        return true
    if kind == 2:
        if end - start < 6:
            return false
        if src[start + 0] != 119:
            return false
        if src[start + 1] != 104:
            return false
        if src[start + 2] != 105:
            return false
        if src[start + 3] != 108:
            return false
        if src[start + 4] != 101:
            return false
        if src[start + 5] != 32:
            return false
        return true
    if kind == 3:
        if end - start < 3:
            return false
        if src[start + 0] != 105:
            return false
        if src[start + 1] != 102:
            return false
        if src[start + 2] != 32:
            return false
        return true
    if kind == 4:
        if end - start < 5:
            return false
        if src[start + 0] != 109:
            return false
        if src[start + 1] != 97:
            return false
        if src[start + 2] != 105:
            return false
        if src[start + 3] != 110:
            return false
        if src[start + 4] != 40:
            return false
        return true
    if kind == 5:
        if end - start < 6:
            return false
        if src[start + 0] != 114:
            return false
        if src[start + 1] != 101:
            return false
        if src[start + 2] != 116:
            return false
        if src[start + 3] != 117:
            return false
        if src[start + 4] != 114:
            return false
        if src[start + 5] != 110:
            return false
        if end > start + 6 && isIdentContinue(src[start + 6]):
            return false
        return true
    if kind == 6:
        if end - start < 5:
            return false
        if src[start + 0] != 98:
            return false
        if src[start + 1] != 114:
            return false
        if src[start + 2] != 101:
            return false
        if src[start + 3] != 97:
            return false
        if src[start + 4] != 107:
            return false
        if end > start + 5 && isIdentContinue(src[start + 5]):
            return false
        return true
    if kind == 7:
        if end - start < 8:
            return false
        if src[start + 0] != 99:
            return false
        if src[start + 1] != 111:
            return false
        if src[start + 2] != 110:
            return false
        if src[start + 3] != 116:
            return false
        if src[start + 4] != 105:
            return false
        if src[start + 5] != 110:
            return false
        if src[start + 6] != 117:
            return false
        if src[start + 7] != 101:
            return false
        if end > start + 8 && isIdentContinue(src[start + 8]):
            return false
        return true
    if kind == 8:
        if end - start < 7:
            return false
        if src[start + 0] != 38:
            return false
        if src[start + 1] != 109:
            return false
        if src[start + 2] != 117:
            return false
        if src[start + 3] != 116:
            return false
        if src[start + 4] != 32:
            return false
        if src[start + 5] != 91:
            return false
        if src[start + 6] != 93:
            return false
        return true
    if kind == 9:
        if end - start < 3:
            return false
        if src[start + 0] != 38:
            return false
        if src[start + 1] != 91:
            return false
        if src[start + 2] != 93:
            return false
        return true
    if kind == 10:
        if end - start < 5:
            return false
        if src[start + 0] != 38:
            return false
        if src[start + 1] != 109:
            return false
        if src[start + 2] != 117:
            return false
        if src[start + 3] != 116:
            return false
        if src[start + 4] != 32:
            return false
        return true
    if kind == 11:
        if end - start < 1:
            return false
        if src[start + 0] != 38:
            return false
        return true
    if kind == 12:
        if end - start < 2:
            return false
        if src[start + 0] != 91:
            return false
        if src[start + 1] != 93:
            return false
        return true
    if kind == 13:
        if end - start < 2:
            return false
        if src[start + 0] != 105:
            return false
        if src[start + 1] != 56:
            return false
        return true
    if kind == 14:
        if end - start < 3:
            return false
        if src[start + 0] != 105:
            return false
        if src[start + 1] != 49:
            return false
        if src[start + 2] != 54:
            return false
        return true
    if kind == 15:
        if end - start < 3:
            return false
        if src[start + 0] != 105:
            return false
        if src[start + 1] != 51:
            return false
        if src[start + 2] != 50:
            return false
        return true
    if kind == 16:
        if end - start < 3:
            return false
        if src[start + 0] != 105:
            return false
        if src[start + 1] != 54:
            return false
        if src[start + 2] != 52:
            return false
        return true
    if kind == 17:
        if end - start < 5:
            return false
        if src[start + 0] != 105:
            return false
        if src[start + 1] != 115:
            return false
        if src[start + 2] != 105:
            return false
        if src[start + 3] != 122:
            return false
        if src[start + 4] != 101:
            return false
        return true
    if kind == 18:
        if end - start < 2:
            return false
        if src[start + 0] != 117:
            return false
        if src[start + 1] != 56:
            return false
        return true
    if kind == 19:
        if end - start < 3:
            return false
        if src[start + 0] != 117:
            return false
        if src[start + 1] != 49:
            return false
        if src[start + 2] != 54:
            return false
        return true
    if kind == 20:
        if end - start < 3:
            return false
        if src[start + 0] != 117:
            return false
        if src[start + 1] != 51:
            return false
        if src[start + 2] != 50:
            return false
        return true
    if kind == 21:
        if end - start < 3:
            return false
        if src[start + 0] != 117:
            return false
        if src[start + 1] != 54:
            return false
        if src[start + 2] != 52:
            return false
        return true
    if kind == 22:
        if end - start < 5:
            return false
        if src[start + 0] != 117:
            return false
        if src[start + 1] != 115:
            return false
        if src[start + 2] != 105:
            return false
        if src[start + 3] != 122:
            return false
        if src[start + 4] != 101:
            return false
        return true
    if kind == 46:
        if end - start < 3:
            return false
        return src[start] == 102 && src[start + 1] == 51 && src[start + 2] == 50
    if kind == 47:
        if end - start < 3:
            return false
        return src[start] == 102 && src[start + 1] == 54 && src[start + 2] == 52
    if kind == 23:
        if end - start < 4:
            return false
        if src[start + 0] != 98:
            return false
        if src[start + 1] != 111:
            return false
        if src[start + 2] != 111:
            return false
        if src[start + 3] != 108:
            return false
        return true
    if kind == 24:
        if end - start < 4:
            return false
        if src[start + 0] != 118:
            return false
        if src[start + 1] != 111:
            return false
        if src[start + 2] != 105:
            return false
        if src[start + 3] != 100:
            return false
        return true
    if kind == 25:
        if end - start < 6:
            return false
        if src[start + 0] != 115:
            return false
        if src[start + 1] != 116:
            return false
        if src[start + 2] != 114:
            return false
        if src[start + 3] != 105:
            return false
        if src[start + 4] != 110:
            return false
        if src[start + 5] != 103:
            return false
        return true
    if kind == 26:
        if end - start < 8:
            return false
        if src[start + 0] != 114:
            return false
        if src[start + 1] != 101:
            return false
        if src[start + 2] != 97:
            return false
        if src[start + 3] != 100:
            return false
        if src[start + 4] != 70:
            return false
        if src[start + 5] != 105:
            return false
        if src[start + 6] != 108:
            return false
        if src[start + 7] != 101:
            return false
        return true
    if kind == 27:
        if end - start < 6:
            return false
        if src[start + 0] != 103:
            return false
        if src[start + 1] != 101:
            return false
        if src[start + 2] != 116:
            return false
        if src[start + 3] != 69:
            return false
        if src[start + 4] != 110:
            return false
        if src[start + 5] != 118:
            return false
        return true
    if kind == 28:
        if end - start < 9:
            return false
        if src[start + 0] != 115:
            return false
        if src[start + 1] != 116:
            return false
        if src[start + 2] != 114:
            return false
        if src[start + 3] != 105:
            return false
        if src[start + 4] != 110:
            return false
        if src[start + 5] != 103:
            return false
        if src[start + 6] != 76:
            return false
        if src[start + 7] != 101:
            return false
        if src[start + 8] != 110:
            return false
        return true
    if kind == 29:
        if end - start < 8:
            return false
        if src[start + 0] != 119:
            return false
        if src[start + 1] != 114:
            return false
        if src[start + 2] != 105:
            return false
        if src[start + 3] != 116:
            return false
        if src[start + 4] != 101:
            return false
        if src[start + 5] != 82:
            return false
        if src[start + 6] != 97:
            return false
        if src[start + 7] != 119:
            return false
        return true
    if kind == 30:
        if end - start < 8:
            return false
        if src[start + 0] != 112:
            return false
        if src[start + 1] != 114:
            return false
        if src[start + 2] != 105:
            return false
        if src[start + 3] != 110:
            return false
        if src[start + 4] != 116:
            return false
        if src[start + 5] != 73:
            return false
        if src[start + 6] != 110:
            return false
        if src[start + 7] != 116:
            return false
        return true
    if kind == 31:
        if end - start < 12:
            return false
        if src[start + 0] != 119:
            return false
        if src[start + 1] != 114:
            return false
        if src[start + 2] != 105:
            return false
        if src[start + 3] != 116:
            return false
        if src[start + 4] != 101:
            return false
        if src[start + 5] != 66:
            return false
        if src[start + 6] != 121:
            return false
        if src[start + 7] != 116:
            return false
        if src[start + 8] != 101:
            return false
        if src[start + 9] != 82:
            return false
        if src[start + 10] != 97:
            return false
        if src[start + 11] != 119:
            return false
        return true
    if kind == 32:
        if end - start < 11:
            return false
        if src[start + 0] != 119:
            return false
        if src[start + 1] != 114:
            return false
        if src[start + 2] != 105:
            return false
        if src[start + 3] != 116:
            return false
        if src[start + 4] != 101:
            return false
        if src[start + 5] != 83:
            return false
        if src[start + 6] != 116:
            return false
        if src[start + 7] != 100:
            return false
        if src[start + 8] != 111:
            return false
        if src[start + 9] != 117:
            return false
        if src[start + 10] != 116:
            return false
        return true
    if kind == 33:
        if end - start < 4:
            return false
        if src[start + 0] != 112:
            return false
        if src[start + 1] != 117:
            return false
        if src[start + 2] != 115:
            return false
        if src[start + 3] != 104:
            return false
        return true
    if kind == 34:
        if end - start < 3:
            return false
        if src[start + 0] != 108:
            return false
        if src[start + 1] != 101:
            return false
        if src[start + 2] != 110:
            return false
        return true
    if kind == 35:
        if end - start < 8:
            return false
        if src[start + 0] != 101:
            return false
        if src[start + 1] != 108:
            return false
        if src[start + 2] != 115:
            return false
        if src[start + 3] != 101:
            return false
        if src[start + 4] != 32:
            return false
        if src[start + 5] != 105:
            return false
        if src[start + 6] != 102:
            return false
        if src[start + 7] != 32:
            return false
        return true
    if kind == 36:
        if end - start < 3:
            return false
        if src[start + 0] != 79:
            return false
        if src[start + 1] != 107:
            return false
        if src[start + 2] != 40:
            return false
        return true
    if kind == 37:
        if end - start < 4:
            return false
        if src[start + 0] != 69:
            return false
        if src[start + 1] != 114:
            return false
        if src[start + 2] != 114:
            return false
        if src[start + 3] != 40:
            return false
        return true
    if kind == 38:
        if end - start < 7:
            return false
        if src[start + 0] != 82:
            return false
        if src[start + 1] != 101:
            return false
        if src[start + 2] != 115:
            return false
        if src[start + 3] != 117:
            return false
        if src[start + 4] != 108:
            return false
        if src[start + 5] != 116:
            return false
        if src[start + 6] != 91:
            return false
        return true
    if kind == 39:
        if end - start < 5:
            return false
        if src[start + 0] != 101:
            return false
        if src[start + 1] != 110:
            return false
        if src[start + 2] != 117:
            return false
        if src[start + 3] != 109:
            return false
        if src[start + 4] != 32:
            return false
        return true
    if kind == 40:
        if end - start < 6:
            return false
        if src[start + 0] != 105:
            return false
        if src[start + 1] != 115:
            return false
        if src[start + 2] != 32:
            return false
        if src[start + 3] != 79:
            return false
        if src[start + 4] != 107:
            return false
        if src[start + 5] != 40:
            return false
        return true
    if kind == 41:
        if end - start < 7:
            return false
        if src[start + 0] != 105:
            return false
        if src[start + 1] != 115:
            return false
        if src[start + 2] != 32:
            return false
        if src[start + 3] != 69:
            return false
        if src[start + 4] != 114:
            return false
        if src[start + 5] != 114:
            return false
        if src[start + 6] != 40:
            return false
        return true
    if kind == 44:
        if end - start < 4:
            return false
        if src[start + 0] != 102 || src[start + 1] != 111 || src[start + 2] != 114 || src[start + 3] != 32:
            return false
        return true
    if kind == 45:
        if end - start < 9:
            return false
        if src[start + 0] != 99 || src[start + 1] != 111 || src[start + 2] != 109 || src[start + 3] != 112 || src[start + 4] != 116 || src[start + 5] != 105 || src[start + 6] != 109 || src[start + 7] != 101 || src[start + 8] != 32:
            return false
        return true
    if kind == 43:
        if end - start < 6:
            return false
        if src[start + 0] != 112:
            return false
        if src[start + 1] != 114:
            return false
        if src[start + 2] != 105:
            return false
        if src[start + 3] != 110:
            return false
        if src[start + 4] != 116:
            return false
        if src[start + 5] != 40:
            return false
        return true
    if kind == 42:
        if end - start < 7:
            return false
        if src[start + 0] != 65:
            return false
        if src[start + 1] != 115:
            return false
        if src[start + 2] != 116:
            return false
        if src[start + 3] != 78:
            return false
        if src[start + 4] != 111:
            return false
        if src[start + 5] != 100:
            return false
        if src[start + 6] != 101:
            return false
        return true
    return false

hasByte(src: &[]u8, start: usize, end: usize, value: u8) bool:
    i: usize = start
    while i < end:
        if src[i] == value:
            return true
        i = i + 1
    return false

# hasTopLevelEquals: depth-aware check for a `=` outside any [] or () nesting.
# A genuine struct/enum declaration line (`Name[f: T, ...]`) never contains a
# top-level `=`. A struct-literal expression used as a statement's RHS
# (`p: Point = Point[x: 20, y: 22]`, or a bare `p = Point[...]`) does.
# hirKind's bracket-shape check alone can't tell the two apart -- both
# "contain '[' and end in ']'" -- so this disambiguates them.
hasTopLevelEquals(src: &[]u8, start: usize, end: usize) bool:
    i: usize = start
    depth: usize = 0
    while i < end:
        if src[i] == 91 || src[i] == 40:
            depth = depth + 1
        else if src[i] == 93 || src[i] == 41:
            if depth > 0:
                depth = depth - 1
        else if depth == 0 && src[i] == 61:
            return true
        i = i + 1
    return false

emitRawRange(src: &[]u8, start: usize, end: usize) void:
    i: usize = start
    while i < end && src[i] != 0:
        writeByteRaw(src[i] as i64)
        i = i + 1

emitRaw(text: string) void:
    writeRaw(text)

rangeEq(src: &[]u8, a: usize, b: usize, c: usize, d: usize) bool:
    if b - a != d - c:
        return false
    i: usize = 0
    while i < b - a:
        if src[a + i] != src[c + i]:
            return false
        i = i + 1
    return true

hasTopLevelName(src: &[]u8, nameStart: usize, nameEnd: usize, declStarts: &[]i64, declEnds: &[]i64) bool:
    i: usize = 0
    while i < declStarts.len():
        if rangeEq(src, nameStart, nameEnd, declStarts[i] as usize, declEnds[i] as usize):
            return true
        i = i + 1
    return false

isTypeWord(src: &[]u8, start: usize, end: usize, wordKind: i64) bool:
    if !startsWord(src, start, end, wordKind):
        return false
    length: usize = end - start
    if wordKind == 13 || wordKind == 18:
        return length == 2
    if wordKind == 14 || wordKind == 15 || wordKind == 16 || wordKind == 19 || wordKind == 20 || wordKind == 21:
        return length == 3
    if wordKind == 17 || wordKind == 22:
        return length == 5
    if wordKind == 23 || wordKind == 24:
        return length == 4
    if wordKind == 46 || wordKind == 47:
        return length == 3
    if wordKind == 25:
        return length == 6
    return false

findTypeBaseEnd(src: &[]u8, start: usize, end: usize) usize:
    i: usize = start
    while i < end && isIdentContinue(src[i]):
        i = i + 1
    return i

splitGeneric(src: &[]u8, start: usize, end: usize) usize:
    depth: usize = 0
    i: usize = start
    while i < end:
        if src[i] == 91:
            depth = depth + 1
        else if src[i] == 93 && depth > 0:
            depth = depth - 1
        else if src[i] == 44 && depth == 0:
            return i
        i = i + 1
    return end

validateType(src: &[]u8, start: usize, end: usize, declStarts: &[]i64, declEnds: &[]i64) bool:
    s: usize = trimStart(src, start, end)
    t: usize = trimEnd(src, s, end)
    if s >= t:
        return false

    if src[t - 1] == 63:
        return validateType(src, s, t - 1, declStarts, declEnds)

    if t >= s + 5 && src[s] == 38 && src[s + 1] == 109 && src[s + 2] == 117 && src[s + 3] == 116 && src[s + 4] == 32:
        return validateType(src, s + 5, t, declStarts, declEnds)
    if src[s] == 38:
        return validateType(src, s + 1, t, declStarts, declEnds)

    if t >= s + 2 && src[s] == 91 && src[s + 1] == 93:
        return validateType(src, s + 2, t, declStarts, declEnds)
    if t >= s + 3 && src[s] == 91:
        i: usize = s + 1
        if i < t && isDigit(src[i]):
            while i < t && isDigit(src[i]):
                i = i + 1
            if i < t && src[i] == 93:
                return validateType(src, i + 1, t, declStarts, declEnds)
        return false

    baseEnd: usize = findTypeBaseEnd(src, s, t)
    if baseEnd < t && src[baseEnd] == 91 && src[t - 1] == 93:
        if startsWord(src, s, t, 38) || src[s] == 86 || src[s] == 69:
            comma: usize = splitGeneric(src, baseEnd + 1, t - 1)
            if startsWord(src, s, t, 38) && comma == t - 1:
                return false
            if comma == t - 1:
                return validateType(src, baseEnd + 1, t - 1, declStarts, declEnds)
            leftOk: bool = validateType(src, baseEnd + 1, comma, declStarts, declEnds)
            rightOk: bool = validateType(src, comma + 1, t - 1, declStarts, declEnds)
            return leftOk && rightOk

    if isTypeWord(src, s, t, 13):
        return true
    if isTypeWord(src, s, t, 14):
        return true
    if isTypeWord(src, s, t, 15):
        return true
    if isTypeWord(src, s, t, 16):
        return true
    if isTypeWord(src, s, t, 17):
        return true
    if isTypeWord(src, s, t, 18):
        return true
    if isTypeWord(src, s, t, 19):
        return true
    if isTypeWord(src, s, t, 20):
        return true
    if isTypeWord(src, s, t, 21):
        return true
    if isTypeWord(src, s, t, 22):
        return true
    if isTypeWord(src, s, t, 23):
        return true
    if isTypeWord(src, s, t, 24):
        return true
    if isTypeWord(src, s, t, 25):
        return true
    if isTypeWord(src, s, t, 46):
        return true
    if isTypeWord(src, s, t, 47):
        return true
    baseLen: usize = end - start
    if baseLen == 4 || baseLen == 5 || baseLen == 6:
        return true

    baseEnd2: usize = findTypeBaseEnd(src, s, t)
    if baseEnd2 == t && baseEnd2 > s:
        if rangeEq(src, s, t, s, s + 4):
            return true
        if hasTopLevelName(src, s, t, declStarts, declEnds):
            return true
        # User-defined type names may arrive from a declaration later in the file.
        return true
    return false

findFunctionReturnTypeStart(src: &[]u8, start: usize, end: usize) usize:
    close: usize = start
    while close < end && src[close] != 41:
        close = close + 1
    if close >= end:
        return end
    return trimStart(src, close + 1, end)

findFunctionReturnTypeEnd(src: &[]u8, start: usize, end: usize) usize:
    s: usize = findFunctionReturnTypeStart(src, start, end)
    t: usize = trimEnd(src, s, end)
    if t > s && src[t - 1] == 58:
        return t - 1
    return t

lineStartsFunction(src: &[]u8, s: usize, t: usize) bool:
    return hasByte(src, s, t, 40) && hasByte(src, s, t, 41) && t > s && src[t - 1] == 58

lineIsTypedDeclaration(src: &[]u8, s: usize, t: usize) bool:
    if startsWord(src, s, t, 1) || startsWord(src, s, t, 3) || startsWord(src, s, t, 2) || startsWord(src, s, t, 35):
        return false
    i: usize = s
    if i >= t || !isIdentStart(src[i]):
        return false
    while i < t && isIdentContinue(src[i]):
        i = i + 1
    i = trimStart(src, i, t)
    return i < t && src[i] == 58

lineDeclarationNameEnd(src: &[]u8, s: usize, t: usize) usize:
    i: usize = s
    while i < t && isIdentContinue(src[i]):
        i = i + 1
    return i

expressionLooksNone(src: &[]u8, s: usize, t: usize) bool:
    return t - s == 4 && src[s] == 110 && src[s + 1] == 111 && src[s + 2] == 110 && src[s + 3] == 101

expressionLooksResultCtor(src: &[]u8, s: usize, t: usize) bool:
    return startsWord(src, s, t, 36) || startsWord(src, s, t, 37)

semanticTypeTag(src: &[]u8, start: usize, end: usize) i64:
    s: usize = trimStart(src, start, end)
    t: usize = trimEnd(src, s, end)
    if s >= t:
        return 0
    if src[t - 1] == 63:
        return 6
    if src[s] == 38:
        return 5
    if startsWord(src, s, t, 38):
        return 7
    if startsWord(src, s, t, 12):
        return 4
    if src[s] == 91:
        return 4
    if startsWord(src, s, t, 25):
        return 3
    if startsWord(src, s, t, 23):
        return 1
    return 2

semanticCheck(source: &[]u8, nodes: &[]AstNode) bool:
    declStarts: []i64 = []i64
    declEnds: []i64 = []i64
    declKinds: []i64 = []i64
    if nodes.len() == 0:
        return false
    ni: usize = 0
    previousAstLevel: usize = 0
    while ni < nodes.len():
        if ni > 0 && nodes[ni].level > previousAstLevel + 1:
            return false
        previousAstLevel = nodes[ni].level
        ni = ni + 1

    pos: usize = 0
    haveMain: bool = false
    previousLevel: usize = 0
    inFunction: bool = false
    inTypeBody: bool = false
    functionReturnStart: usize = 0
    functionReturnEnd: usize = 0
    sawFunctionBody: bool = false
    viewNames: []i64 = []i64
    viewNameEnds: []i64 = []i64

    while pos < source.len():
        e: usize = lineEnd(source, pos)
        s: usize = trimStart(source, pos, e)
        t: usize = trimEnd(source, s, e)
        if s == t || source[s] == 35:
            pos = nextLine(source, pos)
            continue
        if source[s] == 0:
            break

        spaces: usize = indentOf(source, pos, s)
        if spaces % 4 != 0:
            return false
        level: usize = spaces / 4
        if level > previousLevel + 1:
            return false

        if level == 0:
            if inFunction:
                if !sawFunctionBody:
                    return false
                inFunction = false
                viewNames = []i64
                viewNameEnds = []i64
            if inTypeBody:
                inTypeBody = false
            if startsWord(source, s, t, 39):
                nameStart: usize = s + 5
                nameEnd: usize = nameStart
                while nameEnd < t && isIdentContinue(source[nameEnd]):
                    nameEnd = nameEnd + 1
                if nameEnd == nameStart || hasTopLevelName(source, nameStart, nameEnd, &declStarts, &declEnds):
                    return false
                declStarts.push(nameStart as i64)
                declEnds.push(nameEnd as i64)
                declKinds.push(2)
                inTypeBody = true
            else if hasByte(source, s, t, 91) && source[t - 1] == 93:
                nameStartStruct: usize = s
                nameEndStruct: usize = findTypeBaseEnd(source, s, t)
                if nameEndStruct == nameStartStruct || hasTopLevelName(source, nameStartStruct, nameEndStruct, &declStarts, &declEnds):
                    return false
                declStarts.push(nameStartStruct as i64)
                declEnds.push(nameEndStruct as i64)
                declKinds.push(3)
            else if lineIsTypedDeclaration(source, s, t):
                return false
            else if lineStartsFunction(source, s, t):
                nameEnd2: usize = s
                while nameEnd2 < t && isIdentContinue(source[nameEnd2]):
                    nameEnd2 = nameEnd2 + 1
                if nameEnd2 == s || hasTopLevelName(source, s, nameEnd2, &declStarts, &declEnds):
                    return false
                rs: usize = findFunctionReturnTypeStart(source, s, t)
                re: usize = findFunctionReturnTypeEnd(source, s, t)
                if !validateType(source, rs, re, &declStarts, &declEnds):
                    return false
                declStarts.push(s as i64)
                declEnds.push(nameEnd2 as i64)
                declKinds.push(1)
                inFunction = true
                functionReturnStart = rs
                functionReturnEnd = re
                sawFunctionBody = false
                viewNames = []i64
                viewNameEnds = []i64
                if startsWord(source, s, t, 4):
                    haveMain = true
            else if lineIsTypedDeclaration(source, s, t):
                return false
            else:
                return false
        else:
            if inTypeBody:
                if level != 1:
                    return false
                previousLevel = level
                pos = nextLine(source, pos)
                continue
            if !inFunction:
                return false
            sawFunctionBody = true
            if startsWord(source, s, t, 1) || startsWord(source, s, t, 2) || startsWord(source, s, t, 3):
                if t == s || source[t - 1] != 58:
                    return false
            if startsWord(source, s, t, 5):
                rs2: usize = functionReturnStart
                re2: usize = functionReturnEnd
                valueStart: usize = trimStart(source, s + 6, t)
                if re2 > rs2 && startsWord(source, rs2, re2, 24):
                    if valueStart < t:
                        return false
                else:
                    if valueStart >= t:
                        return false
                    if expressionLooksNone(source, valueStart, t) && (re2 <= rs2 || source[re2 - 1] != 63):
                        return false
                    if expressionLooksResultCtor(source, valueStart, t) && !(re2 >= rs2 + 7 && startsWord(source, rs2, re2, 38)):
                        return false
                    if valueStart < t && source[valueStart] == 38:
                        return false
                    if valueStart < t:
                        vnEnd: usize = valueStart
                        while vnEnd < t && isIdentContinue(source[vnEnd]):
                            vnEnd = vnEnd + 1
                        vi: usize = 0
                        while vi < viewNames.len():
                            if vnEnd == t && rangeEq(source, valueStart, vnEnd, viewNames[vi] as usize, viewNameEnds[vi] as usize):
                                return false
                            vi = vi + 1
            if lineIsTypedDeclaration(source, s, t):
                nameEnd3: usize = lineDeclarationNameEnd(source, s, t)
                colon3: usize = nameEnd3
                while colon3 < t && source[colon3] != 58:
                    colon3 = colon3 + 1
                eq3: usize = colon3
                while eq3 < t && source[eq3] != 61:
                    eq3 = eq3 + 1
                if colon3 >= t || eq3 >= t:
                    return false
                typeStart: usize = trimStart(source, colon3 + 1, eq3)
                if !validateType(source, typeStart, eq3, &declStarts, &declEnds):
                    return false
                if source[typeStart] == 38:
                    viewNames.push(s as i64)
                    viewNameEnds.push(nameEnd3 as i64)
                initStart: usize = trimStart(source, eq3 + 1, t)
                if expressionLooksNone(source, initStart, t) && !hasByte(source, typeStart, eq3, 63):
                    return false
                if expressionLooksResultCtor(source, initStart, t) && !startsWord(source, typeStart, eq3, 38):
                    return false

        previousLevel = level
        pos = nextLine(source, pos)

    if inFunction && !sawFunctionBody:
        return false
    return haveMain

matchKind(src: &[]u8, start: usize, end: usize) i64:
    i: usize = start
    while i + 5 < end:
        if src[i] == 32 && src[i + 1] == 105 && src[i + 2] == 115 && src[i + 3] == 32 && src[i + 4] == 79 && src[i + 5] == 107:
            return 1
        if i + 6 < end && src[i] == 32 && src[i + 1] == 105 && src[i + 2] == 115 && src[i + 3] == 32 && src[i + 4] == 69 && src[i + 5] == 114 && src[i + 6] == 114:
            return 2
        i = i + 1
    return 0

hirKind(src: &[]u8, start: usize, end: usize) i64:
    s: usize = trimStart(src, start, end)
    mk: i64 = matchKind(src, s, end)
    if mk != 0:
        return 12
    if startsWord(src, s, end, 35):
        return 5
    if startsWord(src, s, end, 5):
        if end > s + 7 && (startsWord(src, s + 7, end, 36) || startsWord(src, s + 7, end, 37)):
            return 13
        return 6
    if startsWord(src, s, end, 3):
        return 3
    if startsWord(src, s, end, 2):
        return 4
    if startsWord(src, s, end, 1):
        return 5
    if startsWord(src, s, end, 6):
        return 7
    if startsWord(src, s, end, 7):
        return 8
    if startsWord(src, s, end, 44):
        return 14
    if startsWord(src, s, end, 45):
        return 15
    if startsWord(src, s, end, 39):
        return 10
    if startsWord(src, s, end, 40) || startsWord(src, s, end, 41):
        return 12
    if hasByte(src, s, end, 91) && src[end - 1] == 93 && !startsWord(src, s, end, 12) && !hasTopLevelEquals(src, s, end):
        return 11
    if hasByte(src, s, end, 40) && hasByte(src, s, end, 41) && src[end - 1] == 58:
        return 1
    if hasTopLevelEquals(src, s, end):
        return 2
    return 9

# Canonical self-hosted typed AST/module records. Source spans remain the
# diagnostic identity, while semantic type, memory, provenance, and symbol
# identities are attached once and reused by HIR/backend stages.
AstNode[kind: i64, start: usize, end: usize, level: usize, childStart: usize, childCount: usize, typeCode: i64, typeBits: i64, typeSigned: bool, memoryKind: i64, originKind: i64, symbolId: i64]
TypedParamInfo[fnSymbolId: i64, nameStart: usize, nameEnd: usize, typeStart: usize, typeEnd: usize, typeCode: i64, typeBits: i64, typeSigned: bool, memoryKind: i64, originKind: i64, index: usize]
TypedFieldInfo[structSymbolId: i64, nameStart: usize, nameEnd: usize, typeStart: usize, typeEnd: usize, typeCode: i64, typeBits: i64, typeSigned: bool, memoryKind: i64, originKind: i64, index: usize]
TypedHIR[op: i64, start: usize, end: usize, level: usize, typeCode: i64, typeBits: i64, memoryKind: i64, originKind: i64, symbolId: i64]

typedRangeKnownNominal(src: &[]u8, nodes: &mut []AstNode, start: usize, end: usize) bool:
    s: usize = trimStart(src, start, end)
    t: usize = trimEnd(src, s, end)
    i: usize = 0
    while i < nodes.len():
        if nodes[i].level == 0 && (nodes[i].kind == 10 || nodes[i].kind == 11):
            ns: usize = trimStart(src, nodes[i].start, nodes[i].end)
            nt: usize = trimEnd(src, ns, nodes[i].end)
            nameEnd: usize = ns + 4
            if nodes[i].kind == 11:
                nameEnd = ns
                while nameEnd < nt && isIdentContinue(src[nameEnd]):
                    nameEnd = nameEnd + 1
            if nameEnd > ns && rangeEq(src, ns, nameEnd, s, t):
                return true
        i = i + 1
    return false

typedTypeCode(src: &[]u8, start: usize, end: usize) i64:
    s: usize = trimStart(src, start, end)
    t: usize = trimEnd(src, s, end)
    if s >= t:
        return 0
    if t - s >= 7 && src[s] == 38 && src[s + 1] == 109 && src[s + 2] == 117 && src[s + 3] == 116 && src[s + 4] == 32 && src[s + 5] == 91 && src[s + 6] == 93:
        return 9
    if t - s >= 3 && src[s] == 38 && src[s + 1] == 91 && src[s + 2] == 93:
        return 8
    if t - s >= 5 && src[s] == 38 && src[s + 1] == 109 && src[s + 2] == 117 && src[s + 3] == 116:
        return 7
    if t - s >= 1 && src[s] == 38:
        return 6
    if t - s >= 2 && src[s] == 91 && src[s + 1] == 93:
        return 4
    if t - s >= 2 && src[s] == 91:
        return 10
    if t - s > 0 && src[t - 1] == 63:
        return 12
    if startsWord(src, s, t, 38):
        return 13
    if startsWord(src, s, t, 46) || startsWord(src, s, t, 47):
        return 3
    if startsWord(src, s, t, 13) || startsWord(src, s, t, 14) || startsWord(src, s, t, 15) || startsWord(src, s, t, 16) || startsWord(src, s, t, 17) || startsWord(src, s, t, 18) || startsWord(src, s, t, 19) || startsWord(src, s, t, 20) || startsWord(src, s, t, 21) || startsWord(src, s, t, 22):
        return 1
    if startsWord(src, s, t, 23):
        return 2
    if startsWord(src, s, t, 24):
        return 3
    if startsWord(src, s, t, 25):
        return 5
    if startsWord(src, s, t, 26):
        return 11
    # Canonical non-owning range types are typed here so they are not mistaken
    # for user-defined nominal aggregates during the self-hosted semantic pass.
    if t - s >= 5 && src[s] == 86 && src[s + 1] == 105 && src[s + 2] == 101 && src[s + 3] == 119 && src[s + 4] == 91:
        return 15
    if t - s >= 9 && src[s] == 69 && src[s + 1] == 100 && src[s + 2] == 105 && src[s + 3] == 116 && src[s + 4] == 86 && src[s + 5] == 105 && src[s + 6] == 101 && src[s + 7] == 119 && src[s + 8] == 91:
        return 16
    return 14

typedTypeMemory(src: &[]u8, start: usize, end: usize, typeCode: i64) i64:
    if typeCode == 8 || typeCode == 6:
        return 1
    if typeCode == 9 || typeCode == 7:
        return 2
    return 0

typedTypeValid(src: &[]u8, nodes: &mut []AstNode, start: usize, end: usize) bool:
    tc: i64 = typedTypeCode(src, start, end)
    if tc == 0:
        return false
    if tc == 14:
        return typedRangeKnownNominal(src, nodes, start, end)
    return true

typedAnnotateAST(source: &[]u8, nodes: &mut []AstNode, params: &mut []TypedParamInfo, fields: &mut []TypedFieldInfo) bool:
    nextSymbol: i64 = 1
    i: usize = 0
    while i < nodes.len():
        if nodes[i].level == 0:
            if nodes[i].kind == 1 || nodes[i].kind == 10 || nodes[i].kind == 11:
                nodes[i].symbolId = nextSymbol
                nextSymbol = nextSymbol + 1
                if nodes[i].kind == 1:
                    s0: usize = trimStart(source, nodes[i].start, nodes[i].end)
                    t0: usize = trimEnd(source, s0, nodes[i].end)
                    open0: usize = s0
                    while open0 < t0 && source[open0] != 40:
                        open0 = open0 + 1
                    close0: usize = open0 + 1
                    depth0: usize = 1
                    while close0 < t0 && depth0 > 0:
                        if source[close0] == 40:
                            depth0 = depth0 + 1
                        else if source[close0] == 41:
                            depth0 = depth0 - 1
                        close0 = close0 + 1
                    if open0 >= t0 || close0 == 0:
                        return false
                    close0 = close0 - 1
                    ret0: usize = trimStart(source, close0 + 1, t0)
                    ret1: usize = trimEnd(source, ret0, t0)
                    if ret1 > ret0 && source[ret1 - 1] == 58:
                        ret1 = ret1 - 1
                        ret1 = trimEnd(source, ret0, ret1)
                    nodes[i].typeCode = typedTypeCode(source, ret0, ret1)
                    if !typedTypeValid(source, nodes, ret0, ret1):
                        return false
                    # Function parameters become first-class typed records.
                    p0: usize = open0 + 1
                    paramIndex: usize = 0
                    while p0 < close0:
                        q0: usize = p0
                        depthP: usize = 0
                        while q0 < close0:
                            if source[q0] == 91 || source[q0] == 40:
                                depthP = depthP + 1
                            else if source[q0] == 93 || source[q0] == 41:
                                if depthP > 0:
                                    depthP = depthP - 1
                            else if source[q0] == 44 && depthP == 0:
                                break
                            q0 = q0 + 1
                        ps: usize = trimStart(source, p0, q0)
                        pe: usize = trimEnd(source, ps, q0)
                        if ps < pe:
                            colonP: usize = ps
                            while colonP < pe && source[colonP] != 58:
                                colonP = colonP + 1
                            if colonP >= pe:
                                return false
                            pn0: usize = trimStart(source, ps, colonP)
                            pn1: usize = trimEnd(source, pn0, colonP)
                            pt0: usize = trimStart(source, colonP + 1, pe)
                            pt1: usize = trimEnd(source, pt0, pe)
                            ptype: i64 = typedTypeCode(source, pt0, pt1)
                            if !typedTypeValid(source, nodes, pt0, pt1):
                                return false
                            paramBits: i64 = semanticTypeTag(source, pt0, pt1)
                            paramMemory: i64 = typedTypeMemory(source, pt0, pt1, ptype)
                            params.push(TypedParamInfo[fnSymbolId: nodes[i].symbolId, nameStart: pn0, nameEnd: pn1, typeStart: pt0, typeEnd: pt1, typeCode: ptype, typeBits: paramBits, typeSigned: false, memoryKind: paramMemory, originKind: 2, index: paramIndex])
                            paramIndex = paramIndex + 1
                        p0 = q0 + 1
                else if nodes[i].kind == 11:
                    # Struct fields are also typed once here, before backend layout.
                    s1: usize = trimStart(source, nodes[i].start, nodes[i].end)
                    t1: usize = trimEnd(source, s1, nodes[i].end)
                    open1: usize = s1
                    while open1 < t1 && source[open1] != 91:
                        open1 = open1 + 1
                    if open1 >= t1:
                        return false
                    p1: usize = open1 + 1
                    fieldIndex: usize = 0
                    while p1 < t1 - 1:
                        q1: usize = p1
                        while q1 < t1 - 1 && source[q1] != 44:
                            q1 = q1 + 1
                        colonF: usize = p1
                        while colonF < q1 && source[colonF] != 58:
                            colonF = colonF + 1
                        if colonF < q1:
                            fn0: usize = trimStart(source, p1, colonF)
                            fn1: usize = trimEnd(source, fn0, colonF)
                            ft0: usize = trimStart(source, colonF + 1, q1)
                            ft1: usize = trimEnd(source, ft0, q1)
                            ftype: i64 = typedTypeCode(source, ft0, ft1)
                            if !typedTypeValid(source, nodes, ft0, ft1):
                                return false
                            fieldBits: i64 = semanticTypeTag(source, ft0, ft1)
                            fieldMemory: i64 = typedTypeMemory(source, ft0, ft1, ftype)
                            fields.push(TypedFieldInfo[structSymbolId: nodes[i].symbolId, nameStart: fn0, nameEnd: fn1, typeStart: ft0, typeEnd: ft1, typeCode: ftype, typeBits: fieldBits, typeSigned: false, memoryKind: fieldMemory, originKind: 0, index: fieldIndex])
                            fieldIndex = fieldIndex + 1
                        p1 = q1 + 1
        i = i + 1

    # Second pass: annotate executable declarations and control-flow constructs.
    i = 0
    while i < nodes.len():
        s: usize = trimStart(source, nodes[i].start, nodes[i].end)
        t: usize = trimEnd(source, s, nodes[i].end)
        if lineIsTypedDeclaration(source, s, t):
            nameEnd: usize = lineDeclarationNameEnd(source, s, t)
            colon: usize = nameEnd
            while colon < t && source[colon] != 58:
                colon = colon + 1
            eq: usize = colon
            while eq < t && source[eq] != 61:
                eq = eq + 1
            if colon >= t:
                return false
            typeStart: usize = trimStart(source, colon + 1, eq)
            typeEnd: usize = trimEnd(source, typeStart, eq)
            nodes[i].typeCode = typedTypeCode(source, typeStart, typeEnd)
            nodes[i].typeBits = semanticTypeTag(source, typeStart, typeEnd)
            if !typedTypeValid(source, nodes, typeStart, typeEnd):
                return false
            nodes[i].typeSigned = false
            nodes[i].memoryKind = typedTypeMemory(source, typeStart, typeEnd, nodes[i].typeCode)
            nodes[i].originKind = 1
            nodes[i].symbolId = nextSymbol
            nextSymbol = nextSymbol + 1
        else if startsWord(source, s, t, 3) || startsWord(source, s, t, 2):
            nodes[i].typeCode = 2
        else if startsWord(source, s, t, 5):
            nodes[i].typeCode = 14
        i = i + 1
    return true

typedValidateAST(source: &[]u8, nodes: &[]AstNode, params: &[]TypedParamInfo, fields: &[]TypedFieldInfo) bool:
    if nodes.len() == 0:
        return false
    i: usize = 0
    sawMain: bool = false
    while i < nodes.len():
        if nodes[i].symbolId > 0 && nodes[i].level == 0 && nodes[i].kind == 1:
            s: usize = trimStart(source, nodes[i].start, nodes[i].end)
            t: usize = trimEnd(source, s, nodes[i].end)
            if t - s >= 4 && source[s] == 109 && source[s + 1] == 97 && source[s + 2] == 105 && source[s + 3] == 110:
                sawMain = true
            if nodes[i].typeCode == 0:
                return false
        if lineIsTypedDeclaration(source, trimStart(source, nodes[i].start, nodes[i].end), trimEnd(source, trimStart(source, nodes[i].start, nodes[i].end), nodes[i].end)) && nodes[i].typeCode == 0:
            return false
        i = i + 1
    # Lanner parameter/field records must never carry unknown semantic types.
    i = 0
    while i < params.len():
        if params[i].typeCode == 0 || params[i].fnSymbolId == 0:
            return false
        i = i + 1
    i = 0
    while i < fields.len():
        if fields[i].typeCode == 0 || fields[i].structSymbolId == 0:
            return false
        i = i + 1
    return sawMain

lowerTypedHIR(source: &[]u8, nodes: &[]AstNode, hir: &mut []TypedHIR) void:
    i: usize = 0
    while i < nodes.len():
        hir.push(TypedHIR[op: nodes[i].kind, start: nodes[i].start, end: nodes[i].end, level: nodes[i].level, typeCode: nodes[i].typeCode, typeBits: nodes[i].typeBits, memoryKind: nodes[i].memoryKind, originKind: nodes[i].originKind, symbolId: nodes[i].symbolId])
        i = i + 1


buildAST(source: &[]u8, nodes: &mut []AstNode, children: &mut []i64) void:
    pos: usize = 0
    while pos < source.len():
        e: usize = lineEnd(source, pos)
        s: usize = trimStart(source, pos, e)
        t: usize = trimEnd(source, s, e)
        if s != t && source[s] != 35 && source[s] != 0:
            level: usize = indentOf(source, pos, s) / 4
            nodeId: usize = nodes.len()
            childStart: usize = children.len()
            nodeKind: i64 = hirKind(source, s, t)
            nodes.push(AstNode[kind: nodeKind, start: pos, end: e, level: level, childStart: childStart, childCount: 0, typeCode: 0, typeBits: 0, typeSigned: false, memoryKind: 0, originKind: 0, symbolId: 0])
            if level > 0:
                parent: usize = 0
                foundParent: bool = false
                back: usize = nodeId
                while back > 0:
                    back = back - 1
                    if nodes[back].level + 1 == level:
                        parent = back
                        foundParent = true
                        break
                    if nodes[back].level < level - 1:
                        break
                if foundParent:
                    nodes[parent].childCount = nodes[parent].childCount + 1
                    children.push(nodeId as i64)
        pos = nextLine(source, pos)

lowerToHIR(source: &[]u8, nodes: &[]AstNode, hir: &mut []i64) void:
    n: usize = 0
    while n < nodes.len():
        nodeStart: usize = nodes[n].start
        nodeEnd: usize = nodes[n].end
        nodeLevel: usize = nodes[n].level
        nodeKind: i64 = nodes[n].kind
        s: usize = trimStart(source, nodeStart, nodeEnd)
        t: usize = trimEnd(source, s, nodeEnd)
        hir.push(nodeKind)
        hir.push(nodeStart as i64)
        hir.push(nodeEnd as i64)
        hir.push(nodeLevel as i64)
        hir.push(1)
        tag: i64 = 0
        if lineIsTypedDeclaration(source, s, t):
            nameEnd: usize = lineDeclarationNameEnd(source, s, t)
            colon: usize = nameEnd
            while colon < t && source[colon] != 58:
                colon = colon + 1
            eq: usize = colon
            while eq < t && source[eq] != 61:
                eq = eq + 1
            if colon < t && eq < t:
                tag = semanticTypeTag(source, colon + 1, eq)
        else if startsWord(source, s, t, 3) || startsWord(source, s, t, 2):
            tag = 1
        else if nodeKind == 13:
            tag = 6
        else if nodeKind == 10 || nodeKind == 11:
            tag = 2
        hir.push(tag)
        n = n + 1


optimizeHIR(hir: &mut []i64) void:
    i: usize = 0
    trackingDead: bool = false
    deadLevel: usize = 0
    while i + 5 < hir.len():
        kind: i64 = hir[i]
        level: usize = hir[i + 3] as usize
        if trackingDead && level < deadLevel:
            trackingDead = false
        if trackingDead && level >= deadLevel:
            if kind == 9 || kind == 2 || kind == 6 || kind == 13 || kind == 7 || kind == 8:
                hir[i + 4] = 0
        if kind == 6 || kind == 13 || kind == 7 || kind == 8:
            trackingDead = true
            deadLevel = level
        i = i + 6


emitType(src: &[]u8, start: usize, end: usize) void:
    s: usize = trimStart(src, start, end)
    t: usize = trimEnd(src, s, end)
    if startsWord(src, s, t, 8):
        emitRaw("std::vector<")
        emitTypeName(src, s + 7, t)
        emitRaw(">&")
        return
    if startsWord(src, s, t, 9):
        emitRaw("const std::vector<")
        emitTypeName(src, s + 3, t)
        emitRaw(">&")
        return
    if startsWord(src, s, t, 10):
        emitTypeName(src, s + 5, t)
        emitRaw("&")
        return
    if startsWord(src, s, t, 11):
        emitRaw("const ")
        emitTypeName(src, s + 1, t)
        emitRaw("&")
        return
    emitTypeName(src, s, t)

emitTypeName(src: &[]u8, start: usize, end: usize) void:
    s: usize = trimStart(src, start, end)
    if startsWord(src, s, end, 12):
        emitRaw("std::vector<")
        emitTypeName(src, s + 2, end)
        emitRaw(">")
        return
    if startsWord(src, s, end, 13):
        emitRaw("int8_t")
        return
    if startsWord(src, s, end, 14):
        emitRaw("int16_t")
        return
    if startsWord(src, s, end, 15):
        emitRaw("int32_t")
        return
    if startsWord(src, s, end, 16):
        emitRaw("int64_t")
        return
    if startsWord(src, s, end, 17):
        emitRaw("int64_t")
        return
    if startsWord(src, s, end, 18):
        emitRaw("uint8_t")
        return
    if startsWord(src, s, end, 19):
        emitRaw("uint16_t")
        return
    if startsWord(src, s, end, 20):
        emitRaw("uint32_t")
        return
    if startsWord(src, s, end, 21):
        emitRaw("uint64_t")
        return
    if startsWord(src, s, end, 22):
        emitRaw("size_t")
        return
    if startsWord(src, s, end, 23):
        emitRaw("bool")
        return
    if startsWord(src, s, end, 24):
        emitRaw("void")
        return
    if startsWord(src, s, end, 25):
        emitRaw("std::string")
        return
    if startsWord(src, s, end, 38):
        emitRaw("LannerResult<")
        comma: usize = s + 7
        depth: usize = 0
        while comma < end:
            if src[comma] == 91:
                depth = depth + 1
            else if src[comma] == 93 && depth > 0:
                depth = depth - 1
            else if src[comma] == 44 && depth == 0:
                break
            comma = comma + 1
        emitTypeName(src, s + 7, comma)
        emitRaw(", ")
        emitTypeName(src, comma + 1, end - 1)
        emitRaw(">")
        return
    emitRawRange(src, s, end)

# findTopLevelOr: position of the first `||` in [s, t) that sits outside any
# (), [] nesting and outside any "..." string literal, or 0 if there is none.
findTopLevelOr(src: &[]u8, s: usize, t: usize) usize:
    depth: usize = 0
    i: usize = s
    while i < t:
        if src[i] == 34:
            i = i + 1
            while i < t && !(src[i] == 34 && src[i - 1] != 92):
                i = i + 1
            i = i + 1
        else if src[i] == 40 || src[i] == 91:
            depth = depth + 1
            i = i + 1
        else if src[i] == 41 || src[i] == 93:
            if depth > 0:
                depth = depth - 1
            i = i + 1
        else if depth == 0 && src[i] == 124 && i + 1 < t && src[i + 1] == 124:
            return i
        else:
            i = i + 1
    return 0

# hasTopLevelAnd: true if [s, t) contains a `&&` outside any (), [] nesting
# and outside any "..." string literal.
# matchParenEnd: given src[open] == '(', returns the position of its matching
# ')', skipping over nested (), [] and "..." string literals. Returns t if
# unmatched (defensive fallback; should not happen for well-formed input).
matchParenEnd(src: &[]u8, open: usize, t: usize) usize:
    depth: usize = 0
    i: usize = open
    while i < t:
        if src[i] == 34:
            i = i + 1
            while i < t && !(src[i] == 34 && src[i - 1] != 92):
                i = i + 1
            i = i + 1
        else if src[i] == 40 || src[i] == 91:
            depth = depth + 1
            i = i + 1
        else if src[i] == 41 || src[i] == 93:
            depth = depth - 1
            if depth == 0:
                return i
            i = i + 1
        else:
            i = i + 1
    return t

hasTopLevelAnd(src: &[]u8, s: usize, t: usize) bool:
    depth: usize = 0
    i: usize = s
    while i < t:
        if src[i] == 34:
            i = i + 1
            while i < t && !(src[i] == 34 && src[i - 1] != 92):
                i = i + 1
            i = i + 1
        else if src[i] == 40 || src[i] == 91:
            depth = depth + 1
            i = i + 1
        else if src[i] == 41 || src[i] == 93:
            if depth > 0:
                depth = depth - 1
            i = i + 1
        else if depth == 0 && src[i] == 38 && i + 1 < t && src[i + 1] == 38:
            return true
        else:
            i = i + 1
    return false

emitCPPStructFieldValue(src: &[]u8, start: usize, end: usize) void:
    s: usize = trimStart(src, start, end)
    t: usize = trimEnd(src, s, end)
    if t >= s + 6 && src[t - 6] == 46 && src[t - 5] == 108 && src[t - 4] == 101 && src[t - 3] == 110 && src[t - 2] == 40 && src[t - 1] == 41:
        emitRaw("static_cast<size_t>(")
        emitExpr(src, s, t)
        emitRaw(")")
        return
    if t >= s + 7 && src[t - 7] == 46 && src[t - 6] == 115 && src[t - 5] == 105 && src[t - 4] == 122 && src[t - 3] == 101 && src[t - 2] == 40 && src[t - 1] == 41:
        emitRaw("static_cast<int64_t>(")
        emitExpr(src, s, t)
        emitRaw(")")
        return
    emitExpr(src, s, t)

emitExpr(src: &[]u8, start: usize, end: usize) void:
    s: usize = trimStart(src, start, end)
    t: usize = trimEnd(src, s, end)
    if false:
        return
    # C++ warns (and -Werror rejects) `&&` mixed with `||` at the same nesting
    # level without disambiguating parens, even though both Lanner and C++
    # give `&&` higher precedence so the meaning is already unambiguous. Emit
    # each top-level `||`-separated segment individually, wrapping any segment
    # that itself contains a top-level `&&` in parens, so the generated C++
    # matches Lanner's semantics while satisfying the strict build.
    orPos: usize = findTopLevelOr(src, s, t)
    if orPos > 0:
        segStart: usize = s
        p: usize = s
        while p < t:
            opPos: usize = findTopLevelOr(src, p, t)
            segEnd: usize = t
            if opPos > 0:
                segEnd = opPos
            segS: usize = trimStart(src, segStart, segEnd)
            segE: usize = trimEnd(src, segS, segEnd)
            wrap: bool = hasTopLevelAnd(src, segS, segE)
            if wrap:
                emitRaw("(")
            emitExpr(src, segS, segE)
            if wrap:
                emitRaw(")")
            if opPos > 0:
                emitRaw(" || ")
                p = opPos + 2
                segStart = p
            else:
                p = t
        return
    if startsWord(src, s, t, 12):
        emitRaw("std::vector<")
        emitTypeName(src, s + 2, t)
        emitRaw(">{ }")
        return
    if s < t && src[s] == 91 && src[t - 1] == 93 && t > s + 1:
        emitRaw("std::vector<int64_t>{")
        p: usize = s + 1
        first: bool = true
        depth: usize = 0
        while p < t - 1:
            q: usize = p
            while q < t - 1:
                if src[q] == 91:
                    depth = depth + 1
                else if src[q] == 93 && depth > 0:
                    depth = depth - 1
                else if src[q] == 44 && depth == 0:
                    break
                q = q + 1
            if q > p:
                if !first:
                    emitRaw(", ")
                emitExpr(src, p, q)
                first = false
            p = q + 1
        emitRaw("}")
        return
    if s < t && src[s] >= 65 && src[s] <= 90:
        open: usize = s
        while open < t && src[open] != 91:
            open = open + 1
        if open < t && src[t - 1] == 93 && hasByte(src, open + 1, t, 58):
            emitRawRange(src, s, open)
            emitRaw("{")
            p: usize = open + 1
            first: bool = true
            while p < t - 1:
                q: usize = p
                depth: usize = 0
                while q < t - 1:
                    if src[q] == 91:
                        depth = depth + 1
                    else if src[q] == 93 && depth > 0:
                        depth = depth - 1
                    else if src[q] == 44 && depth == 0:
                        break
                    q = q + 1
                colon: usize = p
                while colon < q && src[colon] != 58:
                    colon = colon + 1
                if colon < q:
                    if !first:
                        emitRaw(", ")
                    # A struct-literal field value is emitted into a C++
                    # brace/list-initializer, where (unlike ordinary
                    # assignment or a call) a narrowing conversion is a hard
                    # error. A value ending in a top-level `as TYPE` cast
                    # means the source already intends a narrowing
                    # conversion, so translate it to an explicit static_cast
                    # instead of the usual strip-and-rely-on-implicit-
                    # conversion handling below.
                    fvS: usize = trimStart(src, colon + 1, q)
                    fvT: usize = trimEnd(src, fvS, q)
                    fvIdentStart: usize = fvT
                    while fvIdentStart > fvS && isIdentContinue(src[fvIdentStart - 1]):
                        fvIdentStart = fvIdentStart - 1
                    if fvIdentStart > fvS + 3 && fvIdentStart < fvT && src[fvIdentStart - 1] == 32 && src[fvIdentStart - 2] == 115 && src[fvIdentStart - 3] == 97 && src[fvIdentStart - 4] == 32:
                        emitRaw("static_cast<")
                        emitTypeName(src, fvIdentStart, fvT)
                        emitRaw(">(")
                        emitExpr(src, fvS, fvIdentStart - 4)
                        emitRaw(")")
                    else:
                        emitCPPStructFieldValue(src, fvS, fvT)
                    first = false
                p = q + 1
            emitRaw("}")
            return
    i: usize = s
    while i < t:
        if src[i] == 34:
            j: usize = i + 1
            while j < t:
                if src[j] == 34 && src[j - 1] != 92:
                    break
                j = j + 1
            while i <= j && i < t:
                emitByte(src[i])
                i = i + 1
            continue
        prev: usize = i
        while prev > s && isSpace(src[prev - 1]):
            prev = prev - 1
        atArgBoundary: bool = prev == s || src[prev - 1] == 40 || src[prev - 1] == 44
        if src[i] >= 65 && src[i] <= 90:
            open2: usize = i
            while open2 < t && isIdentContinue(src[open2]):
                open2 = open2 + 1
            if open2 < t && src[open2] == 91:
                close2: usize = open2 + 1
                depth2: usize = 1
                while close2 < t && depth2 > 0:
                    if src[close2] == 91:
                        depth2 = depth2 + 1
                    else if src[close2] == 93:
                        depth2 = depth2 - 1
                    close2 = close2 + 1
                if close2 <= t && depth2 == 0 && hasByte(src, open2 + 1, close2 - 1, 58):
                    emitRawRange(src, i, open2)
                    emitRaw("{")
                    p2: usize = open2 + 1
                    first2: bool = true
                    while p2 < close2 - 1:
                        q2: usize = p2
                        nested2: usize = 0
                        while q2 < close2 - 1:
                            if src[q2] == 91:
                                nested2 = nested2 + 1
                            else if src[q2] == 93 && nested2 > 0:
                                nested2 = nested2 - 1
                            else if src[q2] == 44 && nested2 == 0:
                                break
                            q2 = q2 + 1
                        c2: usize = p2
                        while c2 < q2 && src[c2] != 58:
                            c2 = c2 + 1
                        if c2 < q2:
                            if !first2:
                                emitRaw(", ")
                            fvS2: usize = trimStart(src, c2 + 1, q2)
                            fvT2: usize = trimEnd(src, fvS2, q2)
                            fvIdentStart2: usize = fvT2
                            while fvIdentStart2 > fvS2 && isIdentContinue(src[fvIdentStart2 - 1]):
                                fvIdentStart2 = fvIdentStart2 - 1
                            if fvIdentStart2 > fvS2 + 3 && fvIdentStart2 < fvT2 && src[fvIdentStart2 - 1] == 32 && src[fvIdentStart2 - 2] == 115 && src[fvIdentStart2 - 3] == 97 && src[fvIdentStart2 - 4] == 32:
                                emitRaw("static_cast<")
                                emitTypeName(src, fvIdentStart2, fvT2)
                                emitRaw(">(")
                                emitExpr(src, fvS2, fvIdentStart2 - 4)
                                emitRaw(")")
                            else:
                                emitCPPStructFieldValue(src, fvS2, fvT2)
                            first2 = false
                        p2 = q2 + 1
                    emitRaw("}")
                    i = close2
                    continue
        if i + 5 <= t && startsWord(src, i, t, 10):
            i = i + 5
            continue
        if atArgBoundary && src[i] == 38 && (i + 1 >= t || src[i + 1] != 38):
            i = i + 1
            continue
        if i + 3 < t && src[i] == 97 && src[i + 1] == 115 && src[i + 2] == 32:
            j: usize = i + 3
            while j < t && isIdentContinue(src[j]):
                j = j + 1
            i = j
            continue
        if i + 8 <= t && startsWord(src, i, t, 26):
            emitRaw("lanner_read_file")
            i = i + 8
            continue
        if i + 6 <= t && startsWord(src, i, t, 27):
            emitRaw("lanner_get_env")
            i = i + 6
            continue
        if i + 9 <= t && startsWord(src, i, t, 28):
            emitRaw("lanner_string_len")
            i = i + 9
            continue
        if i + 8 <= t && startsWord(src, i, t, 29):
            emitRaw("lanner_write_raw")
            i = i + 8
            continue
        if i + 6 <= t && startsWord(src, i, t, 43):
            emitRaw("lanner_print")
            i = i + 5
            continue
        if i + 8 <= t && startsWord(src, i, t, 30):
            emitRaw("lanner_print_int")
            i = i + 8
            continue
        if i + 12 <= t && startsWord(src, i, t, 31):
            emitRaw("lanner_write_byte_raw")
            i = i + 12
            continue
        if i + 11 <= t && startsWord(src, i, t, 32):
            emitRaw("lanner_write_raw")
            i = i + 11
            continue
        if src[i] == 46 && i + 5 <= t && startsWord(src, i + 1, t, 33):
            emitRaw(".push_back")
            i = i + 5
            continue
        if src[i] == 46 && i + 4 <= t && startsWord(src, i + 1, t, 34):
            emitRaw(".size")
            i = i + 4
            continue
        if src[i] == 46 && i + 1 < t && src[i + 1] >= 65 && src[i + 1] <= 90:
            emitRaw("::")
            i = i + 1
            continue
        if src[i] == 40:
            # Recurse into parenthesized groups (call arguments or plain
            # grouping) instead of copying their contents byte-for-byte, so
            # the top-level `||`/`&&` disambiguation above also applies
            # inside explicit parens, e.g. `!(a && b || c && d)`.
            closeParen: usize = matchParenEnd(src, i, t)
            emitRaw("(")
            emitExpr(src, i + 1, closeParen)
            emitRaw(")")
            i = closeParen + 1
            continue
        emitByte(src[i])
        i = i + 1
    
emitByte(c: u8) void:
    writeByteRaw(c as i64)

emitFunctionHeader(src: &[]u8, start: usize, end: usize) void:
    s: usize = trimStart(src, start, end)
    open: usize = s
    while open < end && src[open] != 40:
        open = open + 1
    close: usize = open
    while close < end && src[close] != 41:
        close = close + 1
    nameEnd: usize = open
    nameStart: usize = s
    emitType(src, close + 1, end - 1)
    emitRaw(" ")
    emitRawRange(src, nameStart, nameEnd)
    emitRaw("(")

    p: usize = open + 1
    first: bool = true
    while p < close:
        q: usize = p
        while q < close && src[q] != 44:
            q = q + 1
        ps: usize = trimStart(src, p, q)
        pe: usize = trimEnd(src, ps, q)
        if ps < pe:
            colon: usize = ps
            while colon < pe && src[colon] != 58:
                colon = colon + 1
            if !first:
                emitRaw(", ")
            emitType(src, colon + 1, pe)
            emitRaw(" ")
            emitRawRange(src, ps, colon)
            first = false
        p = q + 1
    emitRaw(")")
    emitRaw(" {")
    emitRaw("\n")
    # A strict build (-Wextra -Werror) rejects any parameter the function body
    # never reads. Rather than track real usage, mark every parameter used
    # with a no-op cast -- harmless for parameters that ARE read, and it
    # silences the warning for ones that legitimately aren't (kept for a
    # uniform signature across call sites).
    p3: usize = open + 1
    while p3 < close:
        q3: usize = p3
        while q3 < close && src[q3] != 44:
            q3 = q3 + 1
        ps3: usize = trimStart(src, p3, q3)
        pe3: usize = trimEnd(src, ps3, q3)
        if ps3 < pe3:
            colon3: usize = ps3
            while colon3 < pe3 && src[colon3] != 58:
                colon3 = colon3 + 1
            emitRaw("(void)")
            emitRawRange(src, ps3, colon3)
            emitRaw(";\n")
        p3 = q3 + 1

nameDeclaredBefore(src: &[]u8, current: usize, nameStart: usize, nameEnd: usize) bool:
    nameLen: usize = nameEnd - nameStart
    pos: usize = 0
    while pos < current:
        e: usize = lineEnd(src, pos)
        s: usize = trimStart(src, pos, e)
        if s + nameLen <= e:
            match: bool = true
            i: usize = 0
            while i < nameLen:
                if src[s + i] != src[nameStart + i]:
                    match = false
                    break
                i = i + 1
            if match:
                q: usize = s + nameLen
                while q < e && isSpace(src[q]):
                    q = q + 1
                if q < e && (src[q] == 58 || src[q] == 61):
                    return true
        pos = nextLine(src, pos)
    return false

emitStatement(src: &[]u8, start: usize, end: usize, level: usize, parentKind: i64) void:
    s: usize = trimStart(src, start, end)
    t: usize = trimEnd(src, s, end)
    if s >= t || src[s] == 35:
        return
    if t >= s + 8 && src[s + 0] == 101 && src[s + 1] == 109 && src[s + 2] == 105 && src[s + 3] == 116 && src[s + 4] == 82 && src[s + 5] == 97 && src[s + 6] == 119 && src[s + 7] == 40:
        emitRawRange(src, s, t)
        emitRaw(";\n")
        return
    if parentKind == 10 && level == 1:
        emitRawRange(src, s, t)
        emitRaw(",\n")
        return
    if startsWord(src, s, t, 1):
        emitRaw("else {\n")
        return
    if startsWord(src, s, t, 35):
        emitRaw("else if (")
        emitExpr(src, s + 8, t - 1)
        emitRaw(") {\n")
        return
    if startsWord(src, s, t, 3):
        emitRaw("if (")
        emitExpr(src, s + 3, t - 1)
        emitRaw(") {\n")
        return
    if startsWord(src, s, t, 2):
        emitRaw("while (")
        emitExpr(src, s + 6, t - 1)
        emitRaw(") {\n")
        return
    if startsWord(src, s, t, 36) || startsWord(src, s, t, 37):
        if startsWord(src, s, t, 36):
            emitRaw("return {true, ")
            emitExpr(src, s + 10, t - 1)
            emitRaw(", {}};\n")
        else:
            emitRaw("return {false, {}, ")
            emitExpr(src, s + 11, t - 1)
            emitRaw("};\n")
        return
    if startsWord(src, s, t, 5):
        if t > s + 7 && startsWord(src, s + 7, t, 36):
            emitRaw("return {true, ")
            emitExpr(src, s + 10, t - 1)
            emitRaw(", {}};\n")
            return
        if t > s + 8 && startsWord(src, s + 7, t, 37):
            emitRaw("return {false, {}, ")
            emitExpr(src, s + 11, t - 1)
            emitRaw("};\n")
            return
        emitRaw("return")
        if t > s + 6:
            emitRaw(" ")
            returnEnd: usize = t
            if returnEnd > s + 6 && src[returnEnd - 1] == 59:
                returnEnd = returnEnd - 1
            emitExpr(src, s + 6, returnEnd)
        emitRaw(";\n")
        return
    if startsWord(src, s, t, 7):
        emitRaw("continue;\n")
        return
    if startsWord(src, s, t, 6):
        emitRaw("break;\n")
        return
    if startsWord(src, s, t, 35):
        emitRaw("else if (")
        emitExpr(src, s + 8, t - 1)
        emitRaw(") {\n")
        return
    mk: i64 = matchKind(src, s, t)
    if mk != 0:
        lhsEnd: usize = s
        while lhsEnd < t && !isSpace(src[lhsEnd]):
            lhsEnd = lhsEnd + 1
        isOk: bool = mk == 1
        bindOpen: usize = lhsEnd + 3
        while bindOpen < t && src[bindOpen] != 40:
            bindOpen = bindOpen + 1
        bindClose: usize = bindOpen + 1
        while bindClose < t && src[bindClose] != 41:
            bindClose = bindClose + 1
        arrow: usize = bindClose + 1
        while arrow + 1 < t && !(src[arrow] == 45 && src[arrow + 1] == 62):
            arrow = arrow + 1
        emitRaw("if (")
        emitRawRange(src, s, lhsEnd)
        if isOk:
            emitRaw(".ok) {\n")
            emitRaw("auto& ")
            emitRawRange(src, bindOpen + 1, bindClose)
            emitRaw(" = ")
            emitRawRange(src, s, lhsEnd)
            emitRaw(".value;\n")
        else:
            emitRaw(".ok == false) {\n")
            emitRaw("auto& ")
            emitRawRange(src, bindOpen + 1, bindClose)
            emitRaw(" = ")
            emitRawRange(src, s, lhsEnd)
            emitRaw(".error;\n")
        if arrow + 2 < t:
            emitStatement(src, arrow + 2, t, level + 1, parentKind)
        return

    colon: usize = s
    while colon < t && src[colon] != 58 && src[colon] != 61:
        colon = colon + 1
    eq: usize = colon
    while eq < t && src[eq] != 61:
        eq = eq + 1

    if colon < t && src[colon] == 58 && eq < t:
        emitType(src, colon + 1, eq)
        emitRaw(" ")
        emitRawRange(src, s, colon)
        emitRaw(" = ")
        emitExpr(src, eq + 1, t)
        emitRaw(";\n")
        return

    if colon < t && src[colon] == 61:
        nameEnd: usize = trimEnd(src, s, colon)
        # A LHS containing '[' or '.' is an element/field assignment target
        # (xs[i] = v, ctx.field = v), not a bare identifier being declared or
        # reassigned. Those must never get "auto" or go through the
        # declared-before name lookup, which only makes sense for identifiers.
        isIndexOrFieldTarget: bool = hasByte(src, s, nameEnd, 91) || hasByte(src, s, nameEnd, 46)
        if isIndexOrFieldTarget:
            # The LHS may itself contain Lanner-only syntax (an `as` cast inside
            # an index expression, for example), so it must go through emitExpr's
            # translation rather than a verbatim byte copy.
            emitExpr(src, s, nameEnd)
        else:
            if !nameDeclaredBefore(src, start, s, nameEnd):
                emitRaw("auto ")
            emitRawRange(src, s, nameEnd)
        emitRaw(" = ")
        emitExpr(src, colon + 1, t)
        emitRaw(";\n")
        return

    emitExpr(src, s, t)
    emitRaw(";\n")

emitPreamble() void:
    emitRaw("#include <cstdint>\n")
    emitRaw("#include <cstddef>\n")
    emitRaw("#include <cstdio>\n")
    emitRaw("#include <cstdlib>\n")
    emitRaw("#include <cstring>\n")
    emitRaw("#include <iostream>\n")
    emitRaw("#include <fstream>\n")
    emitRaw("#include <sstream>\n")
    emitRaw("#include <string>\n")
    emitRaw("#include <vector>\n\n")
    emitRaw("template <typename T, typename E> struct LannerResult { bool ok; T value; E error; };\n")
    emitRaw("struct AstNode { int64_t kind; size_t start; size_t end; size_t level; size_t childStart; size_t childCount; int64_t typeCode; int64_t typeBits; bool typeSigned; int64_t memoryKind; int64_t originKind; int64_t symbolId; };\n")
    emitRaw("static std::vector<uint8_t> lanner_read_file(const std::string& path) { std::ifstream in(path, std::ios::binary); if (!in) std::abort(); std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()); data.push_back(0); return data; }\n")
    emitRaw("static std::string lanner_get_env(const std::string& name) { const char* p = std::getenv(name.c_str()); return p ? std::string(p) : std::string(); }\n")
    emitRaw("static size_t lanner_string_len(const std::string& s) { return s.size(); }\n")
    emitRaw("static void lanner_write_raw(const std::string& s) { std::cout << s; }\n")
    emitRaw("static void lanner_print_int(int64_t v) { std::cout << v << '\\n'; }\n")
    emitRaw("template <typename T> [[maybe_unused]] static void lanner_print(const T& v) { std::cout << v << '\\n'; }\n")
    emitRaw("[[maybe_unused]] static void lanner_print(bool v) { std::cout << (v ? \"true\" : \"false\") << '\\n'; }\n")
    emitRaw("static void lanner_write_byte_raw(int64_t v) { std::cout.put(static_cast<char>(v)); }\n")
    emitRaw("static void emitTypeName(const std::vector<uint8_t>&, size_t, size_t);\nstatic void emitByte(uint8_t);\n\n")

emitStructDefinition(src: &[]u8, start: usize, end: usize) void:
    s: usize = trimStart(src, start, end)
    t: usize = trimEnd(src, s, end)
    if startsWord(src, s, t, 42):
        return
    open: usize = s
    while open < t && src[open] != 91:
        open = open + 1
    emitRaw("struct ")
    emitRawRange(src, s, open)
    emitRaw(" { ")
    p: usize = open + 1
    while p < t - 1:
        comma: usize = p
        while comma < t - 1 && src[comma] != 44:
            comma = comma + 1
        colon: usize = p
        while colon < comma && src[colon] != 58:
            colon = colon + 1
        if colon < comma:
            emitType(src, colon + 1, comma)
            emitRaw(" ")
            emitRawRange(src, p, colon)
            emitRaw("; ")
        p = comma + 1
    emitRaw("};\n")

emitFunctionPrototype(src: &[]u8, start: usize, end: usize) void:
    s: usize = trimStart(src, start, end)
    open: usize = s
    while open < end && src[open] != 40:
        open = open + 1
    close: usize = open
    while close < end && src[close] != 41:
        close = close + 1
    emitType(src, close + 1, end - 1)
    emitRaw(" ")
    emitRawRange(src, s, open)
    emitRaw("(")
    p: usize = open + 1
    first: bool = true
    while p < close:
        q: usize = p
        while q < close && src[q] != 44:
            q = q + 1
        ps: usize = trimStart(src, p, q)
        pe: usize = trimEnd(src, ps, q)
        if ps < pe:
            colon: usize = ps
            while colon < pe && src[colon] != 58:
                colon = colon + 1
            if !first:
                emitRaw(", ")
            emitType(src, colon + 1, pe)
            emitRaw(" ")
            emitRawRange(src, ps, colon)
            first = false
        p = q + 1
    emitRaw(");\n")


emitCPP(source: &[]u8, hir: []i64) void:
    emitPreamble()

    # Forward-declare nominal types first.
    h: usize = 0
    while h + 5 < hir.len():
        kind0: i64 = hir[h]
        start0: usize = hir[h + 1] as usize
        end0: usize = hir[h + 2] as usize
        level0: usize = hir[h + 3] as usize
        live0: i64 = hir[h + 4]
        typeTag0: i64 = hir[h + 5]
        h = h + 6
        if level0 == 0 && live0 != 0 && typeTag0 >= 0 && kind0 == 11:
            ss0: usize = trimStart(source, start0, end0)
            ee0: usize = trimEnd(source, ss0, end0)
            open0: usize = ss0
            while open0 < ee0 && source[open0] != 91:
                open0 = open0 + 1
            emitRaw("struct ")
            emitRawRange(source, ss0, open0)
            emitRaw(";\n")
        else if level0 == 0 && live0 != 0 && typeTag0 >= 0 && kind0 == 10:
            ss1: usize = trimStart(source, start0, end0)
            ee1: usize = trimEnd(source, ss1, end0)
            p0: usize = ss1 + 5
            while p0 < ee1 && source[p0] != 58:
                p0 = p0 + 1
            emitRaw("enum class ")
            emitRawRange(source, ss1 + 5, p0)
            emitRaw(" : int64_t;\n")
    emitRaw("\n")

    # Define all nominal types before any function prototype. A by-value
    # parameter or return type must be complete at the declaration point.
    h = 0
    while h + 5 < hir.len():
        kind2: i64 = hir[h]
        start2: usize = hir[h + 1] as usize
        end2: usize = hir[h + 2] as usize
        level2: usize = hir[h + 3] as usize
        live2: i64 = hir[h + 4]
        typeTag2: i64 = hir[h + 5]
        h = h + 6
        if level2 == 0 && live2 != 0 && typeTag2 >= 0:
            ss2: usize = trimStart(source, start2, end2)
            ee2: usize = trimEnd(source, ss2, end2)
            if kind2 == 11:
                emitStructDefinition(source, ss2, ee2)
            else if kind2 == 10:
                emitRaw("enum class ")
                p2: usize = ss2 + 5
                while p2 < ee2 && source[p2] != 58:
                    p2 = p2 + 1
                emitRawRange(source, ss2 + 5, p2)
                emitRaw(" : int64_t {\n")
                scanEnum: usize = h
                firstEnum: bool = true
                while scanEnum + 5 < hir.len():
                    enumStart: usize = hir[scanEnum + 1] as usize
                    enumEnd: usize = hir[scanEnum + 2] as usize
                    enumLevel: usize = hir[scanEnum + 3] as usize
                    enumLive: i64 = hir[scanEnum + 4]
                    if enumLevel == 0:
                        break
                    if enumLevel == 1 && enumLive != 0:
                        enumS: usize = trimStart(source, enumStart, enumEnd)
                        enumT: usize = trimEnd(source, enumS, enumEnd)
                        enumEq: usize = enumS
                        while enumEq < enumT && source[enumEq] != 61:
                            enumEq = enumEq + 1
                        if enumEq < enumT:
                            if !firstEnum:
                                emitRaw(",\n")
                            emitRaw("  ")
                            emitRawRange(source, enumS, enumEq)
                            emitRaw(" = ")
                            emitExpr(source, enumEq + 1, enumT)
                            firstEnum = false
                            hir[scanEnum + 4] = 0
                    scanEnum = scanEnum + 6
                if !firstEnum:
                    emitRaw("\n")
                emitRaw("};\n")
    emitRaw("\n")

    # Emit every function declaration before any function body, removing the
    # old textual-order restriction and permitting mutual recursion.
    h = 0
    while h + 5 < hir.len():
        kind1: i64 = hir[h]
        start1: usize = hir[h + 1] as usize
        end1: usize = hir[h + 2] as usize
        level1: usize = hir[h + 3] as usize
        live1: i64 = hir[h + 4]
        typeTag1: i64 = hir[h + 5]
        h = h + 6
        if level1 == 0 && kind1 == 1 && live1 != 0 && typeTag1 >= 0:
            emitFunctionPrototype(source, start1, end1)
    emitRaw("\n")

    # Finally emit function bodies and other executable top-level items. The
    # nominal type definitions above are intentionally skipped here.
    h = 0
    openLevel: usize = 0
    parentKind: i64 = 0
    while h + 5 < hir.len():
        kind: i64 = hir[h]
        start: usize = hir[h + 1] as usize
        e: usize = hir[h + 2] as usize
        level: usize = hir[h + 3] as usize
        live: i64 = hir[h + 4]
        typeTag: i64 = hir[h + 5]
        h = h + 6
        if typeTag < 0:
            continue
        if live == 0:
            continue
        while openLevel > level:
            if parentKind == 10:
                emitRaw("};\n")
            else:
                emitRaw("}\n")
            openLevel = openLevel - 1
            parentKind = 0
        s: usize = trimStart(source, start, e)
        t: usize = trimEnd(source, s, e)
        if level == 0 && kind == 10:
            # Already emitted in the nominal-definition pass.
            continue
        else if level == 0 && kind == 1:
            emitFunctionHeader(source, s, t)
            openLevel = 1
            parentKind = 0
        else if level == 0 && kind == 11:
            # Already emitted in the nominal-definition pass.
            continue
        else if kind == 12:
            emitStatement(source, start, t, level, parentKind)
            openLevel = openLevel + 1
        else:
            emitStatement(source, start, t, level, parentKind)
            if kind == 3 || kind == 4 || kind == 5 || kind == 35:
                openLevel = openLevel + 1
                parentKind = 0

    while openLevel > 0:
        if parentKind == 10:
            emitRaw("};\n")
        else:
            emitRaw("}\n")
        openLevel = openLevel - 1

LLVMTypeInfo[code: i32, bits: i32, isSigned: bool]
LLVMValueInfo[code: i32, bits: i32, isSigned: bool, immediate: bool, immStart: usize, immEnd: usize, nameStart: usize, nameEnd: usize]
LLVMBinding[nameStart: usize, nameEnd: usize, typeCode: i32, bits: i32, isSigned: bool, owning: bool, initialized: bool, moved: bool]
LLVMFunctionSig[nameStart: usize, nameEnd: usize, returnCode: i32, returnBits: i32, returnSigned: bool, paramStart: usize, paramCount: usize]
LLVMParamInfo[fnIndex: usize, nameStart: usize, nameEnd: usize, typeCode: i32, bits: i32, isSigned: bool]
# Struct support, native LLVM backend: type code 4 means "struct", and the
# usual `bits` slot is repurposed to carry the struct's index into the
# `structs` registry below (mirrors how `bits` already carries int width for
# code 1). This lets a struct-typed value flow through every existing
# LLVMTypeInfo/LLVMBinding/LLVMParamInfo/value-table slot with no schema
# changes beyond this file's own two new record types.
LLVMStructInfo[nameStart: usize, nameEnd: usize, fieldStart: usize, fieldCount: usize]
LLVMStructFieldInfo[nameStart: usize, nameEnd: usize, typeCode: i32, bits: i32, isSigned: bool]
# Dynamic arrays: type code 5, `bits` repurposed (same convention as structs)
# to index into this registry, which records the ELEMENT type only -- every
# dynamic array shares one physical layout regardless of element type
# (mirrors the real C++ backend's %LannerDynArray: data ptr, len, cap, arena
# ptr), so nothing else needs to vary per array type.
LLVMArrayInfo[elemCode: i32, elemBits: i32, elemSigned: bool]
# Fixed arrays: type code 11, bits indexes this registry. The element type is
# stored directly so fixed arrays can be represented as first-class LLVM
# aggregate values and passed/returned by value.
LLVMFixedArrayInfo[elemCode: i32, elemBits: i32, elemSigned: bool, count: usize, role: i32]

llvmTypeFromRange(src: &[]u8, start: usize, end: usize) LLVMTypeInfo:
    s: usize = trimStart(src, start, end)
    t: usize = trimEnd(src, s, end)
    if t - s == 2 && src[s] == 105 && src[s + 1] == 56:
        return LLVMTypeInfo[code: 1, bits: 8, isSigned: true]
    if t - s == 3 && src[s] == 105 && src[s + 1] == 49 && src[s + 2] == 54:
        return LLVMTypeInfo[code: 1, bits: 16, isSigned: true]
    if t - s == 3 && src[s] == 105 && src[s + 1] == 51 && src[s + 2] == 50:
        return LLVMTypeInfo[code: 1, bits: 32, isSigned: true]
    if t - s == 3 && src[s] == 105 && src[s + 1] == 54 && src[s + 2] == 52:
        return LLVMTypeInfo[code: 1, bits: 64, isSigned: true]
    if t - s == 5 && src[s] == 105 && src[s + 1] == 115 && src[s + 2] == 105 && src[s + 3] == 122 && src[s + 4] == 101:
        return LLVMTypeInfo[code: 1, bits: 64, isSigned: true]
    if t - s == 2 && src[s] == 117 && src[s + 1] == 56:
        return LLVMTypeInfo[code: 1, bits: 8, isSigned: false]
    if t - s == 3 && src[s] == 117 && src[s + 1] == 49 && src[s + 2] == 54:
        return LLVMTypeInfo[code: 1, bits: 16, isSigned: false]
    if t - s == 3 && src[s] == 117 && src[s + 1] == 51 && src[s + 2] == 50:
        return LLVMTypeInfo[code: 1, bits: 32, isSigned: false]
    if t - s == 3 && src[s] == 117 && src[s + 1] == 54 && src[s + 2] == 52:
        return LLVMTypeInfo[code: 1, bits: 64, isSigned: false]
    if t - s == 5 && src[s] == 117 && src[s + 1] == 115 && src[s + 2] == 105 && src[s + 3] == 122 && src[s + 4] == 101:
        return LLVMTypeInfo[code: 1, bits: 64, isSigned: false]
    if t - s == 4 && src[s] == 98 && src[s + 1] == 111 && src[s + 2] == 111 && src[s + 3] == 108:
        return LLVMTypeInfo[code: 2, bits: 1, isSigned: false]
    if t - s == 6 && src[s] == 115 && src[s + 1] == 116 && src[s + 2] == 114 && src[s + 3] == 105 && src[s + 4] == 110 && src[s + 5] == 103:
        return LLVMTypeInfo[code: 9, bits: 0, isSigned: false]
    if t - s == 3 && src[s] == 102 && src[s + 1] == 51 && src[s + 2] == 50:
        return LLVMTypeInfo[code: 3, bits: 32, isSigned: true]
    if t - s == 3 && src[s] == 102 && src[s + 1] == 54 && src[s + 2] == 52:
        return LLVMTypeInfo[code: 3, bits: 64, isSigned: true]
    return LLVMTypeInfo[code: 0, bits: 0, isSigned: false]

llvmTypeSupported(src: &[]u8, start: usize, end: usize) bool:
    s: usize = trimStart(src, start, end)
    t: usize = trimEnd(src, s, end)
    if t > s && src[s] == 38:
        return false
    if t > s && src[s] == 91:
        return false
    if t > s && src[t - 1] == 63:
        return false
    info: LLVMTypeInfo = llvmTypeFromRange(src, s, t)
    if info.code != 0:
        return true
    if t - s == 4 && src[s] == 118 && src[s + 1] == 111 && src[s + 2] == 105 && src[s + 3] == 100:
        return true
    return false

emitLLVMType(src: &[]u8, start: usize, end: usize) bool:
    info: LLVMTypeInfo = llvmTypeFromRange(src, start, end)
    if info.code == 1:
        emitRaw("i")
        if info.bits == 8:
            emitRaw("8")
        else if info.bits == 16:
            emitRaw("16")
        else if info.bits == 32:
            emitRaw("32")
        else:
            emitRaw("64")
        return true
    if info.code == 2:
        emitRaw("i1")
        return true
    if info.code == 3:
        if info.bits == 32:
            emitRaw("float")
        else:
            emitRaw("double")
        return true
    if info.code == 9:
        emitRaw("ptr")
        return true
    s: usize = trimStart(src, start, end)
    t: usize = trimEnd(src, s, end)
    if t - s == 4 && src[s] == 118 && src[s + 1] == 111 && src[s + 2] == 105 && src[s + 3] == 100:
        emitRaw("void")
        return true
    return false

emitLLVMIntType(bits: i32) void:
    emitRaw("i")
    if bits == 1:
        emitRaw("1")
    else if bits == 8:
        emitRaw("8")
    else if bits == 16:
        emitRaw("16")
    else if bits == 32:
        emitRaw("32")
    else:
        emitRaw("64")

emitLLVMExprName(src: &[]u8, start: usize, end: usize) void:
    emitRaw("%\"e:")
    emitRawRange(src, start, end)
    emitRaw("\"")

emitLLVMSlotName(src: &[]u8, start: usize, end: usize) void:
    emitRaw("%\"slot:")
    emitRawRange(src, start, end)
    emitRaw("\"")

emitLLVMLabelName(src: &[]u8, start: usize, end: usize, prefix: string) void:
    emitRaw("%\"")
    emitRaw(prefix)
    emitRaw(":")
    emitRawRange(src, start, end)
    emitRaw("\"")

emitLLVMValue(src: &[]u8, v: &LLVMValueInfo) void:
    if v.immediate:
        if v.code == 2:
            if v.immEnd - v.immStart == 4 && src[v.immStart] == 116:
                emitRaw("1")
            else:
                emitRaw("0")
        else:
            emitRawRange(src, v.immStart, v.immEnd)
        return
    emitLLVMExprName(src, v.nameStart, v.nameEnd)

llvmFindTopLevelOp(src: &[]u8, start: usize, end: usize, wanted: i32) usize:
    depth: i64 = 0
    i: usize = start
    found: usize = 0
    while i < end:
        c: u8 = src[i]
        if c == 40 || c == 91:
            depth = depth + 1
            i = i + 1
            continue
        if c == 41 || c == 93:
            if depth > 0:
                depth = depth - 1
            i = i + 1
            continue
        if depth == 0:
            prec: i32 = -1
            oplen: usize = 1
            if i + 1 < end:
                d: u8 = src[i + 1]
                if c == 124 && d == 124:
                    prec = 1
                    oplen = 2
                else if c == 38 && d == 38:
                    prec = 2
                    oplen = 2
                else if c == 61 && d == 61:
                    prec = 3
                    oplen = 2
                else if c == 33 && d == 61:
                    prec = 3
                    oplen = 2
                else if c == 60 && d == 61:
                    prec = 4
                    oplen = 2
                else if c == 62 && d == 61:
                    prec = 4
                    oplen = 2
                else if c == 60 && d == 60:
                    prec = 8
                    oplen = 2
                else if c == 62 && d == 62:
                    prec = 8
                    oplen = 2
                else if c == 45 && d == 62:
                    prec = -1
                else if c == 61 && d != 61:
                    prec = -1
            if prec < 0:
                if c == 60 || c == 62:
                    prec = 4
                else if c == 124:
                    prec = 5
                else if c == 94:
                    prec = 6
                else if c == 38:
                    prec = 7
                else if c == 43 || c == 45:
                    prec = 9
                else if c == 42 || c == 47 || c == 37:
                    prec = 10
            if prec == wanted:
                found = i
            if oplen == 2:
                i = i + 2
                continue
        i = i + 1
    return found

llvmFindLowestOp(src: &[]u8, start: usize, end: usize, posOut: &mut []usize, lenOut: &mut []usize) i32:
    posOut.push(0)
    lenOut.push(0)
    p: i32 = 1
    while p <= 10:
        found: usize = llvmFindTopLevelOp(src, start, end, p)
        if found > 0:
            posOut[0] = found
            if p == 1 || p == 2 || p == 3 || p == 4 || p == 8:
                lenOut[0] = 2
            else:
                lenOut[0] = 1
            return p
        p = p + 1
    return -1

llvmStripParens(src: &[]u8, start: usize, end: usize) usize:
    s: usize = trimStart(src, start, end)
    t: usize = trimEnd(src, s, end)
    if t <= s + 1 || src[s] != 40 || src[t - 1] != 41:
        return 0
    depth: i64 = 0
    i: usize = s
    while i < t:
        if src[i] == 40:
            depth = depth + 1
        else if src[i] == 41:
            depth = depth - 1
            if depth == 0 && i != t - 1:
                return 0
        i = i + 1
    if depth == 0:
        return s + 1
    return 0

llvmFindCast(src: &[]u8, start: usize, end: usize) usize:
    depth: i64 = 0
    bracketDepth: i64 = 0
    i: usize = start
    found: usize = 0
    while i + 3 < end:
        if src[i] == 40:
            depth = depth + 1
        else if src[i] == 41 && depth > 0:
            depth = depth - 1
        else if src[i] == 91:
            bracketDepth = bracketDepth + 1
        else if src[i] == 93 && bracketDepth > 0:
            bracketDepth = bracketDepth - 1
        else if depth == 0 && bracketDepth == 0 && src[i] == 32 && src[i + 1] == 97 && src[i + 2] == 115 && src[i + 3] == 32:
            castTypeStart: usize = trimStart(src, i + 4, end)
            castTypeEnd: usize = trimEnd(src, castTypeStart, end)
            castType: LLVMTypeInfo = llvmTypeFromRange(src, castTypeStart, castTypeEnd)
            if castType.code != 0:
                found = i
        i = i + 1
    return found

llvmLookupBinding(src: &[]u8, bindings: &mut []LLVMBinding, start: usize, end: usize) i64:
    i: usize = 0
    while i < bindings.len():
        if rangeEq(src, start, end, bindings[i].nameStart, bindings[i].nameEnd):
            return i as i64
        i = i + 1
    return (0 as i64) - 1

llvmLookupFunction(src: &[]u8, funcs: &[]LLVMFunctionSig, start: usize, end: usize) i64:
    i: usize = 0
    while i < funcs.len():
        if rangeEq(src, start, end, funcs[i].nameStart, funcs[i].nameEnd):
            return i as i64
        i = i + 1
    return (0 as i64) - 1

llvmEmitExpr(src: &[]u8, start: usize, end: usize, bindings: &mut []LLVMBinding, funcs: &[]LLVMFunctionSig, params: &[]LLVMParamInfo) LLVMValueInfo:
    s: usize = trimStart(src, start, end)
    t: usize = trimEnd(src, s, end)
    stripped: usize = llvmStripParens(src, s, t)
    if stripped > 0:
        return llvmEmitExpr(src, stripped, t - 1, bindings, funcs, params)

    castPos: usize = llvmFindCast(src, s, t)
    if castPos > 0:
        left: LLVMValueInfo = llvmEmitExpr(src, s, castPos, bindings, funcs, params)
        dstStart: usize = trimStart(src, castPos + 4, t)
        dst: LLVMTypeInfo = llvmTypeFromRange(src, dstStart, t)
        if dst.code == 0:
            return LLVMValueInfo[code: 0, bits: 0, isSigned: false, immediate: true, immStart: s, immEnd: t, nameStart: s, nameEnd: t]
        if left.code == dst.code && left.bits == dst.bits:
            return LLVMValueInfo[code: left.code, bits: left.bits, isSigned: left.isSigned, immediate: left.immediate, immStart: left.immStart, immEnd: left.immEnd, nameStart: left.nameStart, nameEnd: left.nameEnd]
        emitLLVMExprName(src, s, t)
        emitRaw(" = ")
        if left.code == 1 && dst.code == 1:
            if left.bits < dst.bits:
                if left.isSigned:
                    emitRaw("sext ")
                else:
                    emitRaw("zext ")
                emitLLVMIntType(left.bits)
                emitRaw(" ")
                emitLLVMValue(src, &left)
                emitRaw(" to ")
                emitLLVMIntType(dst.bits)
            else if left.bits > dst.bits:
                emitRaw("trunc ")
                emitLLVMIntType(left.bits)
                emitRaw(" ")
                emitLLVMValue(src, &left)
                emitRaw(" to ")
                emitLLVMIntType(dst.bits)
            else:
                emitRaw("add ")
                emitLLVMIntType(dst.bits)
                emitRaw(" 0, ")
                emitLLVMValue(src, &left)
            return LLVMValueInfo[code: 1, bits: dst.bits, isSigned: dst.isSigned, immediate: false, immStart: 0, immEnd: 0, nameStart: s, nameEnd: t]
        if left.code == 2 && dst.code == 1:
            emitRaw("zext i1 ")
            emitLLVMValue(src, &left)
            emitRaw(" to ")
            emitLLVMIntType(dst.bits)
            return LLVMValueInfo[code: 1, bits: dst.bits, isSigned: dst.isSigned, immediate: false, immStart: 0, immEnd: 0, nameStart: s, nameEnd: t]
        if left.code == 1 && dst.code == 2:
            emitRaw("icmp ne ")
            emitLLVMIntType(left.bits)
            emitRaw(" ")
            emitLLVMValue(src, &left)
            emitRaw(", 0")
            return LLVMValueInfo[code: 2, bits: 1, isSigned: false, immediate: false, immStart: 0, immEnd: 0, nameStart: s, nameEnd: t]
        return LLVMValueInfo[code: 0, bits: 0, isSigned: false, immediate: false, immStart: 0, immEnd: 0, nameStart: s, nameEnd: t]

    if t > s && (src[s] == 45 || src[s] == 43 || src[s] == 33 || src[s] == 126):
        op: u8 = src[s]
        value: LLVMValueInfo = llvmEmitExpr(src, s + 1, t, bindings, funcs, params)
        if value.code == 0:
            return LLVMValueInfo[code: value.code, bits: value.bits, isSigned: value.isSigned, immediate: value.immediate, immStart: value.immStart, immEnd: value.immEnd, nameStart: value.nameStart, nameEnd: value.nameEnd]
        emitLLVMExprName(src, s, t)
        emitRaw(" = ")
        if op == 45:
            if value.code == 1:
                emitRaw("sub ")
                emitLLVMIntType(value.bits)
                emitRaw(" 0, ")
                emitLLVMValue(src, &value)
            else:
                emitRaw("fneg ")
                if value.bits == 32:
                    emitRaw("float ")
                else:
                    emitRaw("double ")
                emitLLVMValue(src, &value)
        else if op == 43:
            return LLVMValueInfo[code: value.code, bits: value.bits, isSigned: value.isSigned, immediate: value.immediate, immStart: value.immStart, immEnd: value.immEnd, nameStart: value.nameStart, nameEnd: value.nameEnd]
        else if op == 33 && value.code == 2:
            emitRaw("xor i1 1, ")
            emitLLVMValue(src, &value)
        else if op == 126 && value.code == 1:
            emitRaw("xor ")
            emitLLVMIntType(value.bits)
            emitRaw(" -1, ")
            emitLLVMValue(src, &value)
        else:
            return LLVMValueInfo[code: value.code, bits: value.bits, isSigned: value.isSigned, immediate: value.immediate, immStart: value.immStart, immEnd: value.immEnd, nameStart: value.nameStart, nameEnd: value.nameEnd]
        return LLVMValueInfo[code: value.code, bits: value.bits, isSigned: value.isSigned, immediate: false, immStart: 0, immEnd: 0, nameStart: s, nameEnd: t]

    if t - s == 4 && src[s] == 116 && src[s + 1] == 114 && src[s + 2] == 117 && src[s + 3] == 101:
        return LLVMValueInfo[code: 2, bits: 1, isSigned: false, immediate: true, immStart: s, immEnd: t, nameStart: s, nameEnd: t]
    if t - s == 5 && src[s] == 102 && src[s + 1] == 97 && src[s + 2] == 108 && src[s + 3] == 115 && src[s + 4] == 101:
        return LLVMValueInfo[code: 2, bits: 1, isSigned: false, immediate: true, immStart: s, immEnd: t, nameStart: s, nameEnd: t]

    i: usize = s
    numeric: bool = true
    while i < t:
        if !isDigit(src[i]):
            numeric = false
            break
        i = i + 1
    if numeric && i > s:
        return LLVMValueInfo[code: 1, bits: 64, isSigned: true, immediate: true, immStart: s, immEnd: t, nameStart: s, nameEnd: t]

    paren: usize = s
    while paren < t && src[paren] != 40:
        paren = paren + 1
    if paren < t && t > s + 1 && src[t - 1] == 41:
        calleeStart: usize = s
        calleeEnd: usize = trimEnd(src, s, paren)
        fnIndex: i64 = llvmLookupFunction(src, funcs, calleeStart, calleeEnd)
        if fnIndex < 0:
            return LLVMValueInfo[code: 0, bits: 0, isSigned: false, immediate: false, immStart: 0, immEnd: 0, nameStart: s, nameEnd: t]
        emitLLVMExprName(src, s, t)
        if funcs[fnIndex as usize].returnCode == 0:
            emitRaw(" = ")
        else:
            emitRaw(" = ")
        emitRaw("call ")
        if funcs[fnIndex as usize].returnCode == 1:
            emitLLVMIntType(funcs[fnIndex as usize].returnBits)
        else if funcs[fnIndex as usize].returnCode == 2:
            emitRaw("i1")
        else if funcs[fnIndex as usize].returnCode == 3:
            if funcs[fnIndex as usize].returnBits == 32:
                emitRaw("float")
            else:
                emitRaw("double")
        else:
            emitRaw("void")
        emitRaw(" @")
        emitRawRange(src, calleeStart, calleeEnd)
        emitRaw("(")
        p: usize = paren + 1
        depth2: usize = 0
        first: bool = true
        while p < t - 1:
            q: usize = p
            depth2 = 0
            while q < t - 1:
                if src[q] == 40 || src[q] == 91:
                    depth2 = depth2 + 1
                else if (src[q] == 41 || src[q] == 93) && depth2 > 0:
                    depth2 = depth2 - 1
                else if src[q] == 44 && depth2 == 0:
                    break
                q = q + 1
            arg: LLVMValueInfo = llvmEmitExpr(src, p, q, bindings, funcs, params)
            if !first:
                emitRaw(", ")
            if arg.code == 1:
                emitLLVMIntType(arg.bits)
            else if arg.code == 2:
                emitRaw("i1")
            else if arg.code == 3:
                if arg.bits == 32:
                    emitRaw("float")
                else:
                    emitRaw("double")
            emitRaw(" ")
            emitLLVMValue(src, &arg)
            first = false
            p = q + 1
        emitRaw(")\n")
        return LLVMValueInfo[code: funcs[fnIndex as usize].returnCode, bits: funcs[fnIndex as usize].returnBits, isSigned: funcs[fnIndex as usize].returnSigned, immediate: false, immStart: 0, immEnd: 0, nameStart: s, nameEnd: t]

    bindIndex: i64 = llvmLookupBinding(src, bindings, s, t)
    if bindIndex >= 0:
        b: &LLVMBinding = &bindings[bindIndex as usize]
        emitLLVMExprName(src, s, t)
        emitRaw(" = load ")
        if b.typeCode == 1:
            emitLLVMIntType(b.bits)
        else if b.typeCode == 3:
            if b.bits == 32:
                emitRaw("float")
            else:
                emitRaw("double")
        else if b.typeCode == 2:
            emitRaw("i1")
        else:
            if b.bits == 32:
                emitRaw("float")
            else:
                emitRaw("double")
        emitRaw(", ptr ")
        emitLLVMSlotName(src, b.nameStart, b.nameEnd)
        emitRaw("\n")
        return LLVMValueInfo[code: b.typeCode, bits: b.bits, isSigned: b.isSigned, immediate: false, immStart: 0, immEnd: 0, nameStart: s, nameEnd: t]

    opPos: []usize = []usize
    opLen: []usize = []usize
    prec: i32 = llvmFindLowestOp(src, s, t, &mut opPos, &mut opLen)
    if prec >= 0:
        left2: LLVMValueInfo = llvmEmitExpr(src, s, opPos[0], bindings, funcs, params)
        right2: LLVMValueInfo = llvmEmitExpr(src, opPos[0] + opLen[0], t, bindings, funcs, params)
        if left2.code == 0 || right2.code == 0:
            return LLVMValueInfo[code: left2.code, bits: left2.bits, isSigned: left2.isSigned, immediate: left2.immediate, immStart: left2.immStart, immEnd: left2.immEnd, nameStart: left2.nameStart, nameEnd: left2.nameEnd]
        if prec == 1 || prec == 2:
            return LLVMValueInfo[code: 0, bits: 0, isSigned: false, immediate: false, immStart: 0, immEnd: 0, nameStart: s, nameEnd: t]
        emitLLVMExprName(src, s, t)
        emitRaw(" = ")
        op: u8 = src[opPos[0]]
        if prec == 3 || prec == 4:
            emitRaw("icmp ")
            if prec == 3:
                if op == 61:
                    emitRaw("eq ")
                else:
                    emitRaw("ne ")
            else:
                if op == 60 && src[opPos[0] + 1] == 61:
                    if left2.isSigned:
                        emitRaw("sle ")
                    else:
                        emitRaw("ule ")
                else if op == 62 && src[opPos[0] + 1] == 61:
                    if left2.isSigned:
                        emitRaw("sge ")
                    else:
                        emitRaw("uge ")
                else if op == 60:
                    if left2.isSigned:
                        emitRaw("slt ")
                    else:
                        emitRaw("ult ")
                else:
                    if left2.isSigned:
                        emitRaw("sgt ")
                    else:
                        emitRaw("ugt ")
            emitLLVMIntType(left2.bits)
            emitRaw(" ")
            emitLLVMValue(src, &left2)
            emitRaw(", ")
            emitLLVMValue(src, &right2)
            return LLVMValueInfo[code: 2, bits: 1, isSigned: false, immediate: false, immStart: 0, immEnd: 0, nameStart: s, nameEnd: t]
        if prec == 5:
            emitRaw("or ")
        else if prec == 6:
            emitRaw("xor ")
        else if prec == 7:
            emitRaw("and ")
        else if prec == 8:
            if op == 60:
                emitRaw("shl ")
            else if left2.isSigned:
                emitRaw("ashr ")
            else:
                emitRaw("lshr ")
        else if prec == 9:
            if op == 43:
                emitRaw("add ")
            else:
                emitRaw("sub ")
        else:
            if op == 42:
                emitRaw("mul ")
            else if op == 47:
                if left2.isSigned:
                    emitRaw("sdiv ")
                else:
                    emitRaw("udiv ")
            else:
                if left2.isSigned:
                    emitRaw("srem ")
                else:
                    emitRaw("urem ")
        emitLLVMIntType(left2.bits)
        emitRaw(" ")
        emitLLVMValue(src, &left2)
        emitRaw(", ")
        emitLLVMValue(src, &right2)
        return LLVMValueInfo[code: left2.code, bits: left2.bits, isSigned: left2.isSigned, immediate: false, immStart: 0, immEnd: 0, nameStart: s, nameEnd: t]

    return LLVMValueInfo[code: 0, bits: 0, isSigned: false, immediate: false, immStart: 0, immEnd: 0, nameStart: s, nameEnd: t]

llvmParseDeclaredName(src: &[]u8, s: usize, t: usize, colonOut: &mut []i64, eqOut: &mut []i64) bool:
    colonOut.push((0 as i64) - 1)
    eqOut.push((0 as i64) - 1)
    i: usize = s
    depth: usize = 0
    while i < t:
        if src[i] == 91 || src[i] == 40:
            depth = depth + 1
        else if src[i] == 93 || src[i] == 41:
            if depth > 0:
                depth = depth - 1
        else if depth == 0 && src[i] == 58 && colonOut[0] < 0:
            colonOut[0] = i as i64
        else if depth == 0 && src[i] == 61:
            eqOut[0] = i as i64
            break
        i = i + 1
    return colonOut[0] >= 0 && eqOut[0] > colonOut[0]

llvmNative3TypeOwnsResourcesInner(code: i32, bits: i32, structs: &[]LLVMStructInfo, fields: &[]LLVMStructFieldInfo, arrays: &mut []LLVMArrayInfo, fixedArrays: &mut []LLVMFixedArrayInfo, seen: &[]i64) bool:
    if code == 5 || code == 18:
        return true
    if code == 1 || code == 2 || code == 6 || code == 7 || code == 9 || code == 12 || code == 15 || code == 16 || code == 17:
        return false
    key: i64 = (code as i64) * 1048576 + (bits as i64)
    si: usize = 0
    while si < seen.len():
        if seen[si] == key:
            # A recursive path through an owning aggregate is itself resource-bearing.
            return true
        si = si + 1
    nextSeen: []i64 = []i64
    si = 0
    while si < seen.len():
        nextSeen.push(seen[si])
        si = si + 1
    nextSeen.push(key)
    if code == 4:
        if bits < 0:
            return false
        structIndexBits: usize = bits as usize
        if structIndexBits >= structs.len():
            return false
        st: &LLVMStructInfo = &structs[structIndexBits]
        fi: usize = 0
        while fi < st.fieldCount:
            f: &LLVMStructFieldInfo = &fields[st.fieldStart + fi]
            if llvmNative3TypeOwnsResourcesInner(f.typeCode, f.bits, structs, fields, arrays, fixedArrays, &nextSeen):
                return true
            fi = fi + 1
        return false
    if code == 11:
        if bits < 0:
            return false
        fixedIndexBits: usize = bits as usize
        if fixedIndexBits >= fixedArrays.len():
            return false
        fa: &LLVMFixedArrayInfo = &fixedArrays[fixedIndexBits]
        return fa.count > 0 && llvmNative3TypeOwnsResourcesInner(fa.elemCode, fa.elemBits, structs, fields, arrays, fixedArrays, &nextSeen)
    if code == 13:
        if bits < 0:
            return false
        optionIndexBits: usize = bits as usize
        if optionIndexBits >= fixedArrays.len():
            return false
        opt: &LLVMFixedArrayInfo = &fixedArrays[optionIndexBits]
        return llvmNative3TypeOwnsResourcesInner(opt.elemCode, opt.elemBits, structs, fields, arrays, fixedArrays, &nextSeen)
    if code == 14:
        if bits < 0:
            return false
        resultIndexBits: usize = bits as usize
        if resultIndexBits + 1 >= fixedArrays.len():
            return false
        ok: &LLVMFixedArrayInfo = &fixedArrays[resultIndexBits]
        err: &LLVMFixedArrayInfo = &fixedArrays[resultIndexBits + 1]
        return llvmNative3TypeOwnsResourcesInner(ok.elemCode, ok.elemBits, structs, fields, arrays, fixedArrays, &nextSeen) || llvmNative3TypeOwnsResourcesInner(err.elemCode, err.elemBits, structs, fields, arrays, fixedArrays, &nextSeen)
    return false

llvmNative3TypeOwnsResources(code: i32, bits: i32, structs: &[]LLVMStructInfo, fields: &[]LLVMStructFieldInfo, arrays: &mut []LLVMArrayInfo, fixedArrays: &mut []LLVMFixedArrayInfo) bool:
    seen: []i64 = []i64
    return llvmNative3TypeOwnsResourcesInner(code, bits, structs, fields, arrays, fixedArrays, &seen)

llvmBindingMarkMoved(src: &[]u8, bindings: &mut []LLVMBinding, start: usize, end: usize) bool:
    s: usize = trimStart(src, start, end)
    t: usize = trimEnd(src, s, end)
    bindIndex: i64 = llvmLookupBinding(src, bindings, s, t)
    if bindIndex < 0:
        return true
    b: &mut LLVMBinding = &mut bindings[bindIndex as usize]
    if b.owning:
        if b.moved:
            return false
        b.moved = true
    return true

llvmNative3EmitDropCall(code: i32, bits: i32, src: &[]u8, addr: string) void:
    emitRaw("  call void @__lanner_drop_")
    if code == 4:
        emitRaw("struct_")
    else if code == 5:
        emitRaw("array_")
    else if code == 11:
        emitRaw("fixed_")
    else if code == 13:
        emitRaw("option_")
    else if code == 14:
        emitRaw("result_")
    else if code == 18:
        emitRaw("arena")
    else:
        emitRaw("unknown_")
    if code != 18:
        llvmNativeNumber(bits as usize)
    emitRaw("(ptr ")
    emitRaw(addr)
    emitRaw(")\n")

llvmNative3EmitDropCallValue(code: i32, bits: i32, src: &[]u8, ctx: &mut LLVMNativeContext, value: usize) void:
    emitRaw("  call void @__lanner_drop_")
    if code == 4:
        emitRaw("struct_")
    else if code == 5:
        emitRaw("array_")
    else if code == 11:
        emitRaw("fixed_")
    else if code == 13:
        emitRaw("option_")
    else if code == 14:
        emitRaw("result_")
    else if code == 18:
        emitRaw("arena")
    else:
        emitRaw("unknown_")
    if code != 18:
        llvmNativeNumber(bits as usize)
    emitRaw("(ptr ")
    llvmNative3EmitValue(src, ctx, value)
    emitRaw(")\n")

llvmNative3EmitDropFunction(code: i32, bits: i32, src: &[]u8, structs: &[]LLVMStructInfo, fields: &[]LLVMStructFieldInfo, arrays: &mut []LLVMArrayInfo, fixedArrays: &mut []LLVMFixedArrayInfo) bool:
    if !llvmNative3TypeOwnsResources(code, bits, structs, fields, arrays, fixedArrays):
        return true
    emitRaw("define internal void @__lanner_drop_")
    if code == 4:
        emitRaw("struct_")
        llvmNativeNumber(bits as usize)
    else if code == 5:
        emitRaw("array_")
        llvmNativeNumber(bits as usize)
    else if code == 11:
        emitRaw("fixed_")
        llvmNativeNumber(bits as usize)
    else if code == 13:
        emitRaw("option_")
        llvmNativeNumber(bits as usize)
    else if code == 14:
        emitRaw("result_")
        llvmNativeNumber(bits as usize)
    else if code == 18:
        emitRaw("arena")
    else:
        return false
    emitRaw("(ptr %addr) {\nentry:\n")
    if code == 18:
        emitRaw("  call void @__lanner_arena_destroy(ptr %addr)\n  ret void\n}\n\n")
        return true
    if code == 5:
        if bits < 0:
            return false
        arrayIndexBits: usize = bits as usize
        if arrayIndexBits >= arrays.len():
            return false
        a: &LLVMArrayInfo = &arrays[arrayIndexBits]
        elemOwns: bool = llvmNative3TypeOwnsResources(a.elemCode, a.elemBits, structs, fields, arrays, fixedArrays)
        emitRaw("  %arr = load %LannerDynArray, ptr %addr\n")
        emitRaw("  %data = extractvalue %LannerDynArray %arr, 0\n")
        emitRaw("  %len = extractvalue %LannerDynArray %arr, 1\n")
        if elemOwns:
            emitRaw("  %hasdata = icmp ne ptr %data, null\n  br i1 %hasdata, label %drop.loop, label %drop.after\n")
            emitRaw("drop.loop:\n  %idx = phi i64 [ 0, %entry ], [ %next, %drop.step ]\n  %active = icmp ult i64 %idx, %len\n  br i1 %active, label %drop.body, label %drop.after\n")
            emitRaw("drop.body:\n  %elem = getelementptr inbounds ")
            llvmEmitAllocaType(a.elemCode, a.elemBits, src, structs, arrays, fixedArrays)
            emitRaw(", ptr %data, i64 %idx\n")
            llvmNative3EmitDropCall(a.elemCode, a.elemBits, src, "%elem")
            emitRaw("  br label %drop.step\n")
            emitRaw("drop.step:\n  %next = add i64 %idx, 1\n  br label %drop.loop\n")
            emitRaw("drop.after:\n")
        emitRaw("  call void @__lanner_dyn_array_destroy(ptr %addr)\n  ret void\n}\n\n")
        return true
    if code == 11:
        if bits < 0:
            return false
        dropFixedIndex: usize = bits as usize
        if dropFixedIndex >= fixedArrays.len():
            return false
        fa: &LLVMFixedArrayInfo = &fixedArrays[dropFixedIndex]
        if !llvmNative3TypeOwnsResources(fa.elemCode, fa.elemBits, structs, fields, arrays, fixedArrays):
            return false
        emitRaw("  br label %drop.loop\n")
        emitRaw("drop.loop:\n  %idx = phi i64 [ 0, %entry ], [ %next, %drop.step ]\n  %active = icmp ult i64 %idx, ")
        llvmNativeNumber(fa.count)
        emitRaw("\n  br i1 %active, label %drop.body, label %drop.done\n")
        emitRaw("drop.body:\n  %elem = getelementptr inbounds ")
        llvmEmitAllocaType(11, bits, src, structs, arrays, fixedArrays)
        emitRaw(", ptr %addr, i64 0, i64 %idx\n")
        llvmNative3EmitDropCall(fa.elemCode, fa.elemBits, src, "%elem")
        emitRaw("  br label %drop.step\n")
        emitRaw("drop.step:\n  %next = add i64 %idx, 1\n  br label %drop.loop\n")
        emitRaw("drop.done:\n  ret void\n}\n\n")
        return true
    if code == 13:
        if bits < 0:
            return false
        dropOptionIndex: usize = bits as usize
        if dropOptionIndex >= fixedArrays.len():
            return false
        opt: &LLVMFixedArrayInfo = &fixedArrays[dropOptionIndex]
        emitRaw("  %value = load ")
        llvmEmitAllocaType(13, bits, src, structs, arrays, fixedArrays)
        emitRaw(", ptr %addr\n  %has = extractvalue ")
        llvmEmitAllocaType(13, bits, src, structs, arrays, fixedArrays)
        emitRaw(" %value, 0\n  br i1 %has, label %drop.payload, label %drop.done\n")
        emitRaw("drop.payload:\n  %payload = getelementptr inbounds ")
        llvmEmitAllocaType(13, bits, src, structs, arrays, fixedArrays)
        emitRaw(", ptr %addr, i64 0, i32 1\n")
        llvmNative3EmitDropCall(opt.elemCode, opt.elemBits, src, "%payload")
        emitRaw("  br label %drop.done\ndrop.done:\n  ret void\n}\n\n")
        return true
    if code == 14:
        if bits < 0:
            return false
        dropResultIndex: usize = bits as usize
        if dropResultIndex + 1 >= fixedArrays.len():
            return false
        ok: &LLVMFixedArrayInfo = &fixedArrays[dropResultIndex]
        err: &LLVMFixedArrayInfo = &fixedArrays[dropResultIndex + 1]
        emitRaw("  %value = load ")
        llvmEmitAllocaType(14, bits, src, structs, arrays, fixedArrays)
        emitRaw(", ptr %addr\n  %tag = extractvalue ")
        llvmEmitAllocaType(14, bits, src, structs, arrays, fixedArrays)
        emitRaw(" %value, 0\n  br i1 %tag, label %drop.ok, label %drop.err\n")
        emitRaw("drop.ok:\n")
        if llvmNative3TypeOwnsResources(ok.elemCode, ok.elemBits, structs, fields, arrays, fixedArrays):
            emitRaw("  %okptr = getelementptr inbounds ")
            llvmEmitAllocaType(14, bits, src, structs, arrays, fixedArrays)
            emitRaw(", ptr %addr, i64 0, i32 1\n")
            llvmNative3EmitDropCall(ok.elemCode, ok.elemBits, src, "%okptr")
        emitRaw("  br label %drop.done\n")
        emitRaw("drop.err:\n")
        if llvmNative3TypeOwnsResources(err.elemCode, err.elemBits, structs, fields, arrays, fixedArrays):
            emitRaw("  %errptr = getelementptr inbounds ")
            llvmEmitAllocaType(14, bits, src, structs, arrays, fixedArrays)
            emitRaw(", ptr %addr, i64 0, i32 2\n")
            llvmNative3EmitDropCall(err.elemCode, err.elemBits, src, "%errptr")
        emitRaw("  br label %drop.done\n")
        emitRaw("drop.done:\n  ret void\n}\n\n")
        return true
    if code == 4:
        if bits < 0:
            return false
        dropStructIndex: usize = bits as usize
        if dropStructIndex >= structs.len():
            return false
        st: &LLVMStructInfo = &structs[dropStructIndex]
        if st.fieldCount > 0:
            fi: usize = st.fieldCount
            while fi > 0:
                fi = fi - 1
                f: &LLVMStructFieldInfo = &fields[st.fieldStart + fi]
                if llvmNative3TypeOwnsResources(f.typeCode, f.bits, structs, fields, arrays, fixedArrays):
                    emitRaw("  %field")
                    llvmNativeNumber(fi)
                    emitRaw(" = getelementptr inbounds %")
                    emitRawRange(src, st.nameStart, st.nameEnd)
                    emitRaw(", ptr %addr, i64 0, i32 ")
                    llvmNativeNumber(fi)
                    emitRaw("\n")
                    emitRaw("  call void @__lanner_drop_")
                    if f.typeCode == 4:
                        emitRaw("struct_")
                    else if f.typeCode == 5:
                        emitRaw("array_")
                    else if f.typeCode == 11:
                        emitRaw("fixed_")
                    else if f.typeCode == 13:
                        emitRaw("option_")
                    else if f.typeCode == 14:
                        emitRaw("result_")
                    else if f.typeCode == 18:
                        emitRaw("arena")
                    if f.typeCode != 18:
                        llvmNativeNumber(f.bits as usize)
                    emitRaw("(ptr %field")
                    llvmNativeNumber(fi)
                    emitRaw(")\n")
            emitRaw("  ret void\n}\n\n")
            return true
        emitRaw("  ret void\n}\n\n")
        return true
    return false

llvmNative3EmitDropGlue(src: &[]u8, structs: &[]LLVMStructInfo, fields: &[]LLVMStructFieldInfo, arrays: &mut []LLVMArrayInfo, fixedArrays: &mut []LLVMFixedArrayInfo) bool:
    emitRaw("; Typed ownership drop glue\n")
    i: usize = 0
    while i < structs.len():
        if !llvmNative3EmitDropFunction(4, i as i32, src, structs, fields, arrays, fixedArrays):
            return false
        i = i + 1
    i = 0
    while i < arrays.len():
        if !llvmNative3EmitDropFunction(5, i as i32, src, structs, fields, arrays, fixedArrays):
            return false
        i = i + 1
    i = 0
    while i < fixedArrays.len():
        role: i32 = fixedArrays[i].role
        if role == 0:
            if !llvmNative3EmitDropFunction(11, i as i32, src, structs, fields, arrays, fixedArrays):
                return false
        else if role == 1:
            if !llvmNative3EmitDropFunction(13, i as i32, src, structs, fields, arrays, fixedArrays):
                return false
        else if role == 2:
            if !llvmNative3EmitDropFunction(14, i as i32, src, structs, fields, arrays, fixedArrays):
                return false
        i = i + 1
    if !llvmNative3EmitDropFunction(18, 0, src, structs, fields, arrays, fixedArrays):
        return false
    return true

llvmNative3DropBinding(src: &[]u8, bindings: &mut []LLVMBinding, bindIndex: usize) void:
    b: &mut LLVMBinding = &mut bindings[bindIndex]
    if b.owning == false || b.initialized == false || b.moved:
        return
    # Fail closed for all non-owning type codes. A stale/corrupted ownership
    # flag must never turn a reference, string, scalar, or view into an arena
    # destruction call.
    if b.typeCode == 1 || b.typeCode == 2 || b.typeCode == 6 || b.typeCode == 7 || b.typeCode == 9 || b.typeCode == 12 || b.typeCode == 15 || b.typeCode == 16 || b.typeCode == 17:
        return
    if b.typeCode != 4 && b.typeCode != 5 && b.typeCode != 11 && b.typeCode != 13 && b.typeCode != 14 && b.typeCode != 18:
        return
    emitRaw("  call void @__lanner_drop_")
    if b.typeCode == 4:
        emitRaw("struct_")
    else if b.typeCode == 5:
        emitRaw("array_")
    else if b.typeCode == 11:
        emitRaw("fixed_")
    else if b.typeCode == 13:
        emitRaw("option_")
    else if b.typeCode == 14:
        emitRaw("result_")
    else:
        emitRaw("arena")
    if b.typeCode != 18:
        llvmNativeNumber(b.bits as usize)
    emitRaw("(ptr ")
    emitLLVMSlotName(src, b.nameStart, b.nameEnd)
    emitRaw(")\n")
    b.moved = true

llvmNative3DropAllBindings(src: &[]u8, bindings: &mut []LLVMBinding) void:
    i: usize = bindings.len()
    while i > 0:
        i = i - 1
        llvmNative3DropBinding(src, bindings, i)

llvmNative3MergeGuardMoves(src: &[]u8, bindings: &mut []LLVMBinding, guardBindings: &[]LLVMBinding) void:
    gi: usize = 0
    while gi < guardBindings.len():
        gb: &LLVMBinding = &guardBindings[gi]
        if gb.moved:
            oi: i64 = llvmLookupBinding(src, bindings, gb.nameStart, gb.nameEnd)
            if oi >= 0:
                bindings[oi as usize].moved = true
        gi = gi + 1

llvmAddBinding(src: &[]u8, bindings: &mut []LLVMBinding, nameStart: usize, nameEnd: usize, typeCode: i32, bits: i32, isSigned: bool, owning: bool) bool:
    if llvmLookupBinding(src, bindings, nameStart, nameEnd) >= 0:
        return false
    bindings.push(LLVMBinding[nameStart: nameStart, nameEnd: nameEnd, typeCode: typeCode, bits: bits, isSigned: isSigned, owning: owning, initialized: false, moved: false])
    return true

llvmEmitStore(src: &[]u8, b: &LLVMBinding, value: &LLVMValueInfo) void:
    if b.typeCode == 1:
        emitRaw("store ")
        emitLLVMIntType(b.bits)
    else if b.typeCode == 2:
        emitRaw("store i1")
    else:
        emitRaw("store ")
        if b.bits == 32:
            emitRaw("float")
        else:
            emitRaw("double")
    emitRaw(" ")
    emitLLVMValue(src, &value)
    emitRaw(", ptr ")
    emitLLVMSlotName(src, b.nameStart, b.nameEnd)
    emitRaw("\n")

llvmEmitSequence(src: &[]u8, nodes: &[]AstNode, children: &[]i64, childStart: usize, childCount: usize, bindings: &mut []LLVMBinding, funcs: &[]LLVMFunctionSig, params: &[]LLVMParamInfo) bool:
    i: usize = 0
    terminated: bool = false
    while i < childCount && !terminated:
        nodeId: usize = children[childStart + i] as usize
        kind: i64 = nodes[nodeId].kind
        s: usize = trimStart(src, nodes[nodeId].start, nodes[nodeId].end)
        t: usize = trimEnd(src, s, nodes[nodeId].end)
        if kind == 3:
            cond: LLVMValueInfo = llvmEmitExpr(src, s + 3, t - 1, bindings, funcs, params)
            thenLabelStart: usize = s + 3
            thenLabelEnd: usize = t - 1
            emitRaw("  br i1 ")
            emitLLVMValue(src, &cond)
            emitRaw(", label ")
            emitLLVMLabelName(src, thenLabelStart, thenLabelEnd, "if.then")
            emitRaw(", label ")
            emitLLVMLabelName(src, thenLabelStart, thenLabelEnd, "if.else")
            emitRaw("\n")
            emitLLVMLabelName(src, thenLabelStart, thenLabelEnd, "if.then")
            emitRaw(":\n")
            thenTerminated: bool = llvmEmitSequence(src, nodes, children, nodes[nodeId].childStart, nodes[nodeId].childCount, bindings, funcs, params)
            if !thenTerminated:
                emitRaw("  br label ")
                emitLLVMLabelName(src, thenLabelStart, thenLabelEnd, "if.merge")
                emitRaw("\n")
            hasElse: bool = i + 1 < childCount && nodes[children[childStart + i + 1] as usize].kind == 5
            emitLLVMLabelName(src, thenLabelStart, thenLabelEnd, "if.else")
            emitRaw(":\n")
            elseTerminated: bool = false
            if hasElse:
                elseId: usize = children[childStart + i + 1] as usize
                elseTerminated = llvmEmitSequence(src, nodes, children, nodes[elseId].childStart, nodes[elseId].childCount, bindings, funcs, params)
            if !elseTerminated:
                emitRaw("  br label ")
                emitLLVMLabelName(src, thenLabelStart, thenLabelEnd, "if.merge")
                emitRaw("\n")
            emitLLVMLabelName(src, thenLabelStart, thenLabelEnd, "if.merge")
            emitRaw(":\n")
            # Keep structured conditionals conservative: an enclosing block
            # must not inherit termination from a child `if` and lose its own
            # merge edge. Unreachable merge blocks are valid LLVM.
            terminated = false
            if hasElse:
                i = i + 1
        else if kind == 4:
            condStart: usize = s + 6
            condEnd: usize = t - 1
            emitRaw("  br label ")
            emitLLVMLabelName(src, condStart, condEnd, "while.cond")
            emitRaw("\n")
            emitLLVMLabelName(src, condStart, condEnd, "while.cond")
            emitRaw(":\n")
            cond: LLVMValueInfo = llvmEmitExpr(src, condStart, condEnd, bindings, funcs, params)
            emitRaw("  br i1 ")
            emitLLVMValue(src, &cond)
            emitRaw(", label ")
            emitLLVMLabelName(src, condStart, condEnd, "while.body")
            emitRaw(", label ")
            emitLLVMLabelName(src, condStart, condEnd, "while.end")
            emitRaw("\n")
            emitLLVMLabelName(src, condStart, condEnd, "while.body")
            emitRaw(":\n")
            bodyTerminated: bool = llvmEmitSequence(src, nodes, children, nodes[nodeId].childStart, nodes[nodeId].childCount, bindings, funcs, params)
            if !bodyTerminated:
                emitRaw("  br label ")
                emitLLVMLabelName(src, condStart, condEnd, "while.cond")
                emitRaw("\n")
            emitLLVMLabelName(src, condStart, condEnd, "while.end")
            emitRaw(":\n")
        else if kind == 5:
            terminated = false
        else if kind == 6:
            if t <= s + 6:
                emitRaw("  ret void\n")
            else:
                value: LLVMValueInfo = llvmEmitExpr(src, s + 6, t, bindings, funcs, params)
                emitRaw("  ret ")
                if value.code == 1:
                    emitLLVMIntType(value.bits)
                else if value.code == 2:
                    emitRaw("i1")
                else if value.code == 3:
                    if value.bits == 32:
                        emitRaw("float")
                    else:
                        emitRaw("double")
                emitRaw(" ")
                emitLLVMValue(src, &value)
                emitRaw("\n")
            terminated = true
        else if kind == 7 || kind == 8:
            terminated = false
        else if kind == 2:
            colon: []i64 = []i64
            eq: []i64 = []i64
            if !llvmParseDeclaredName(src, s, t, &mut colon, &mut eq):
                terminated = false
            else:
                nameStart: usize = s
                nameEnd: usize = colon[0] as usize
                if t - s >= 6:
                    if src[s] == 99:
                        if src[s + 1] == 111:
                            if src[s + 2] == 110:
                                if src[s + 3] == 115:
                                    if src[s + 4] == 116:
                                        if src[s + 5] == 32:
                                            nameStart = s + 6
                                            while nameStart < t && isSpace(src[nameStart]):
                                                nameStart = nameStart + 1
                declaredStart: usize = trimStart(src, colon[0] as usize + 1, eq[0] as usize)
                info: LLVMTypeInfo = llvmTypeFromRange(src, declaredStart, eq[0] as usize)
                bindIndex: i64 = llvmLookupBinding(src, bindings, nameStart, nameEnd)
                if bindIndex < 0:
                    if info.code == 0 || !llvmAddBinding(src, bindings, nameStart, nameEnd, info.code, info.bits, info.isSigned, info.code == 5 || info.code == 18):
                        terminated = false
                    else:
                        emitRaw("  ")
                        emitLLVMSlotName(src, nameStart, nameEnd)
                        emitRaw(" = alloca ")
                        if info.code == 1:
                            emitLLVMIntType(info.bits)
                        else if info.code == 2:
                            emitRaw("i1")
                        else if info.code == 3:
                            if info.bits == 32:
                                emitRaw("float")
                            else:
                                emitRaw("double")
                        emitRaw("\n")
                        bindIndex = llvmLookupBinding(src, bindings, nameStart, nameEnd)
                if bindIndex >= 0:
                    value2: LLVMValueInfo = llvmEmitExpr(src, eq[0] as usize + 1, t, bindings, funcs, params)
                    if value2.code == bindings[bindIndex as usize].typeCode:
                        llvmEmitStore(src, &bindings[bindIndex as usize], &value2)
        else if kind == 9:
            value3: LLVMValueInfo = llvmEmitExpr(src, s, t, bindings, funcs, params)
            if value3.code == 0:
                terminated = false
        else if kind == 11 || kind == 10 || kind == 12:
            terminated = false
        i = i + 1
    return terminated

llvmLookupStruct(src: &[]u8, structs: &[]LLVMStructInfo, nameStart: usize, nameEnd: usize) i64:
    nameLen: usize = nameEnd - nameStart
    i: usize = 0
    while i < structs.len():
        if structs[i].nameEnd - structs[i].nameStart == nameLen:
            j: usize = 0
            m: bool = true
            while j < nameLen:
                if src[structs[i].nameStart + j] != src[nameStart + j]:
                    m = false
                    break
                j = j + 1
            if m:
                return i as i64
        i = i + 1
    return (0 as i64) - 1

llvmStructFieldIndex(src: &[]u8, fields: &[]LLVMStructFieldInfo, fieldStart: usize, fieldCount: usize, nameStart: usize, nameEnd: usize) i64:
    nameLen: usize = nameEnd - nameStart
    i: usize = 0
    while i < fieldCount:
        f: &LLVMStructFieldInfo = &fields[fieldStart + i]
        if f.nameEnd - f.nameStart == nameLen:
            j: usize = 0
            m: bool = true
            while j < nameLen:
                if src[f.nameStart + j] != src[nameStart + j]:
                    m = false
                    break
                j = j + 1
            if m:
                return i as i64
        i = i + 1
    return (0 as i64) - 1

llvmLookupStructMutable(src: &[]u8, structs: &mut []LLVMStructInfo, nameStart: usize, nameEnd: usize) i64:
    nameLen: usize = nameEnd - nameStart
    i: usize = 0
    while i < structs.len():
        if structs[i].nameEnd - structs[i].nameStart == nameLen:
            j: usize = 0
            same: bool = true
            while j < nameLen:
                if src[structs[i].nameStart + j] != src[nameStart + j]:
                    same = false
                    break
                j = j + 1
            if same:
                return i as i64
        i = i + 1
    return (0 as i64) - 1

llvmBuildStructRegistry(src: &[]u8, nodes: &[]AstNode, structs: &mut []LLVMStructInfo, fields: &mut []LLVMStructFieldInfo, arrays: &mut []LLVMArrayInfo, fixedArrays: &mut []LLVMFixedArrayInfo) bool:
    # Pass 1: register all struct names before resolving any fields.
    i: usize = 0
    while i < nodes.len():
        if nodes[i].level == 0 && nodes[i].kind == 11:
            s: usize = trimStart(src, nodes[i].start, nodes[i].end)
            t: usize = trimEnd(src, s, nodes[i].end)
            open: usize = s
            while open < t && src[open] != 91:
                open = open + 1
            if open >= t:
                return false
            structs.push(LLVMStructInfo[nameStart: s, nameEnd: open, fieldStart: 0, fieldCount: 0])
        i = i + 1
    # Pass 2: resolve scalar, struct, and dynamic-array fields.
    structOrdinal: usize = 0
    i = 0
    while i < nodes.len():
        if nodes[i].level == 0 && nodes[i].kind == 11:
            s2: usize = trimStart(src, nodes[i].start, nodes[i].end)
            t2: usize = trimEnd(src, s2, nodes[i].end)
            open2: usize = s2
            while open2 < t2 && src[open2] != 91:
                open2 = open2 + 1
            if open2 >= t2 || structOrdinal >= structs.len():
                return false
            structs[structOrdinal].fieldStart = fields.len()
            p: usize = open2 + 1
            while p < t2 - 1:
                comma: usize = p
                while comma < t2 - 1 && src[comma] != 44:
                    comma = comma + 1
                colon: usize = p
                while colon < comma && src[colon] != 58:
                    colon = colon + 1
                if colon < comma:
                    fieldNameStart: usize = trimStart(src, p, colon)
                    fieldNameEnd: usize = trimEnd(src, fieldNameStart, colon)
                    typeStart: usize = trimStart(src, colon + 1, comma)
                    typeEnd: usize = trimEnd(src, typeStart, comma)
                    info: LLVMTypeInfo = llvmTypeFromRange(src, typeStart, typeEnd)
                    if info.code == 0:
                        fieldStruct: i64 = llvmLookupStructMutable(src, structs, typeStart, typeEnd)
                        if fieldStruct >= 0:
                            info = LLVMTypeInfo[code: 4, bits: fieldStruct as i32, isSigned: false]
                        else if typeEnd - typeStart >= 2 && src[typeStart] == 91 && src[typeStart + 1] == 93:
                            elemStart: usize = trimStart(src, typeStart + 2, typeEnd)
                            elemEnd: usize = trimEnd(src, elemStart, typeEnd)
                            elemInfo: LLVMTypeInfo = llvmTypeFromRange(src, elemStart, elemEnd)
                            if elemInfo.code == 0:
                                elemStruct: i64 = llvmLookupStructMutable(src, structs, elemStart, elemEnd)
                                if elemStruct >= 0:
                                    elemInfo = LLVMTypeInfo[code: 4, bits: elemStruct as i32, isSigned: false]
                            if elemInfo.code != 1 && elemInfo.code != 2 && elemInfo.code != 3 && elemInfo.code != 4 && elemInfo.code != 17:
                                return false
                            arrayIdx: usize = llvmInternArray(arrays, elemInfo.code, elemInfo.bits, elemInfo.isSigned)
                            info = LLVMTypeInfo[code: 5, bits: arrayIdx as i32, isSigned: false]
                        else if typeEnd - typeStart >= 3 && src[typeStart] == 91:
                            closeFixed: usize = typeStart + 1
                            while closeFixed < typeEnd && src[closeFixed] >= 48 && src[closeFixed] <= 57:
                                closeFixed = closeFixed + 1
                            if closeFixed < typeEnd && src[closeFixed] == 93 && closeFixed > typeStart + 1:
                                countFixed: usize = 0
                                posFixed: usize = typeStart + 1
                                while posFixed < closeFixed:
                                    countFixed = countFixed * 10 + ((src[posFixed] - 48) as usize)
                                    posFixed = posFixed + 1
                                elemFixed: usize = trimStart(src, closeFixed + 1, typeEnd)
                                elemFixedEnd: usize = trimEnd(src, elemFixed, typeEnd)
                                fixedInfo: LLVMTypeInfo = llvmTypeFromRange(src, elemFixed, elemFixedEnd)
                                if fixedInfo.code == 0:
                                    fixedStruct: i64 = llvmLookupStructMutable(src, structs, elemFixed, elemFixedEnd)
                                    if fixedStruct >= 0:
                                        fixedInfo = LLVMTypeInfo[code: 4, bits: fixedStruct as i32, isSigned: false]
                                if countFixed == 0 || (fixedInfo.code != 1 && fixedInfo.code != 2 && fixedInfo.code != 3 && fixedInfo.code != 4 && fixedInfo.code != 17):
                                    return false
                                fixedIdx: usize = llvmInternFixedArray(fixedArrays, fixedInfo.code, fixedInfo.bits, fixedInfo.isSigned, countFixed)
                                info = LLVMTypeInfo[code: 11, bits: fixedIdx as i32, isSigned: false]
                            else:
                                return false
                        else:
                            return false
                    if info.code != 1 && info.code != 2 && info.code != 3 && info.code != 4 && info.code != 5 && info.code != 11 && info.code != 13 && info.code != 14 && info.code != 17 && info.code != 18:
                        return false
                    fields.push(LLVMStructFieldInfo[nameStart: fieldNameStart, nameEnd: fieldNameEnd, typeCode: info.code, bits: info.bits, isSigned: info.isSigned])
                    structs[structOrdinal].fieldCount = structs[structOrdinal].fieldCount + 1
                p = comma + 1
            structOrdinal = structOrdinal + 1
        i = i + 1
    return true

llvmEmitStructTypeDecls(src: &[]u8, structs: &[]LLVMStructInfo, fields: &[]LLVMStructFieldInfo, arrays: &mut []LLVMArrayInfo, fixedArrays: &mut []LLVMFixedArrayInfo) void:
    i: usize = 0
    while i < structs.len():
        emitRaw("%")
        emitRawRange(src, structs[i].nameStart, structs[i].nameEnd)
        emitRaw(" = type { ")
        count: usize = 0
        while count < structs[i].fieldCount:
            if count > 0:
                emitRaw(", ")
            f: &LLVMStructFieldInfo = &fields[structs[i].fieldStart + count]
            llvmEmitAllocaType(f.typeCode, f.bits, src, structs, arrays, fixedArrays)
            count = count + 1
        emitRaw(" }\n")
        i = i + 1
    emitRaw("\n")

llvmInternArray(arrays: &mut []LLVMArrayInfo, code: i32, bits: i32, isSigned: bool) usize:
    i: usize = 0
    while i < arrays.len():
        if arrays[i].elemCode == code && arrays[i].elemBits == bits && arrays[i].elemSigned == isSigned:
            return i
        i = i + 1
    arrays.push(LLVMArrayInfo[elemCode: code, elemBits: bits, elemSigned: isSigned])
    return arrays.len() - 1

llvmInternFixedArray(fixedArrays: &mut []LLVMFixedArrayInfo, code: i32, bits: i32, isSigned: bool, count: usize) usize:
    i: usize = 0
    while i < fixedArrays.len():
        if fixedArrays[i].elemCode == code && fixedArrays[i].elemBits == bits && fixedArrays[i].elemSigned == isSigned && fixedArrays[i].count == count:
            return i
        i = i + 1
    fixedArrays.push(LLVMFixedArrayInfo[elemCode: code, elemBits: bits, elemSigned: isSigned, count: count, role: 0])
    return fixedArrays.len() - 1

# The native bootstrap keeps Optional/Result payload metadata in the existing
# fixed-array registry. `count == 0` is reserved for these metadata entries,
# because real fixed arrays require a positive size. Optional uses one entry;
# Result uses two consecutive entries (Ok, Err).
llvmInternOptionalPayload(fixedArrays: &mut []LLVMFixedArrayInfo, code: i32, bits: i32, isSigned: bool) usize:
    i: usize = 0
    while i < fixedArrays.len():
        if fixedArrays[i].count == 0 && fixedArrays[i].elemCode == code && fixedArrays[i].elemBits == bits && fixedArrays[i].elemSigned == isSigned:
            return i
        i = i + 1
    fixedArrays.push(LLVMFixedArrayInfo[elemCode: code, elemBits: bits, elemSigned: isSigned, count: 0, role: 1])
    return fixedArrays.len() - 1

llvmInternResultPayloads(fixedArrays: &mut []LLVMFixedArrayInfo, okCode: i32, okBits: i32, okSigned: bool, errCode: i32, errBits: i32, errSigned: bool) usize:
    i: usize = 0
    while i + 1 < fixedArrays.len():
        if fixedArrays[i].count == 0 && fixedArrays[i + 1].count == 0 && fixedArrays[i].elemCode == okCode && fixedArrays[i].elemBits == okBits && fixedArrays[i].elemSigned == okSigned && fixedArrays[i + 1].elemCode == errCode && fixedArrays[i + 1].elemBits == errBits && fixedArrays[i + 1].elemSigned == errSigned:
            return i
        i = i + 1
    fixedArrays.push(LLVMFixedArrayInfo[elemCode: okCode, elemBits: okBits, elemSigned: okSigned, count: 0, role: 2])
    fixedArrays.push(LLVMFixedArrayInfo[elemCode: errCode, elemBits: errBits, elemSigned: errSigned, count: 0, role: 3])
    return fixedArrays.len() - 2

llvmParseEnumLiteral(src: &[]u8, start: usize, end: usize) i64:
    s: usize = trimStart(src, start, end)
    t: usize = trimEnd(src, s, end)
    if s >= t:
        return (0 as i64) - 1
    base: i64 = 10
    pos: usize = s
    if t - s > 2 && src[s] == 48 && (src[s + 1] == 120 || src[s + 1] == 88):
        base = 16
        pos = s + 2
    else if t - s > 2 && src[s] == 48 && (src[s + 1] == 98 || src[s + 1] == 66):
        base = 2
        pos = s + 2
    if pos >= t:
        return (0 as i64) - 1
    value: i64 = 0
    while pos < t:
        c: u8 = src[pos]
        digit: i64 = (0 as i64) - 1
        if c >= 48 && c <= 57:
            digit = (c - 48) as i64
        else if c >= 65 && c <= 70:
            digit = (c - 65) as i64 + 10
        else if c >= 97 && c <= 102:
            digit = (c - 97) as i64 + 10
        if digit < 0 || digit >= base:
            return (0 as i64) - 1
        value = value * base + digit
        pos = pos + 1
    return value

llvmLookupEnumType(src: &[]u8, start: usize, end: usize) bool:
    wantedStart: usize = trimStart(src, start, end)
    wantedEnd: usize = trimEnd(src, wantedStart, end)
    if wantedStart >= wantedEnd:
        return false
    pos: usize = 0
    while pos < src.len():
        lineEndPos: usize = lineEnd(src, pos)
        s: usize = trimStart(src, pos, lineEndPos)
        t: usize = trimEnd(src, s, lineEndPos)
        if s < t && startsWord(src, s, t, 39):
            nameStart: usize = s + 5
            while nameStart < t && isSpace(src[nameStart]):
                nameStart = nameStart + 1
            nameEnd: usize = nameStart
            while nameEnd < t && isIdentContinue(src[nameEnd]):
                nameEnd = nameEnd + 1
            if nameEnd > nameStart && rangeEq(src, nameStart, nameEnd, wantedStart, wantedEnd):
                return true
        pos = nextLine(src, pos)
    return false

llvmFindEnumVariantValue(src: &[]u8, start: usize, end: usize, outFound: &mut []i64) i64:
    s: usize = trimStart(src, start, end)
    t: usize = trimEnd(src, s, end)
    dot: usize = s
    while dot < t && src[dot] != 46:
        dot = dot + 1
    if dot <= s || dot + 1 >= t:
        outFound.push(0)
        return 0
    enumStart: usize = s
    enumEnd: usize = dot
    variantStart: usize = trimStart(src, dot + 1, t)
    variantEnd: usize = trimEnd(src, variantStart, t)
    if !llvmLookupEnumType(src, enumStart, enumEnd):
        outFound.push(0)
        return 0
    pos: usize = 0
    inside: bool = false
    nextValue: i64 = 0
    while pos < src.len():
        le: usize = lineEnd(src, pos)
        ls: usize = trimStart(src, pos, le)
        lt: usize = trimEnd(src, ls, le)
        if ls < lt && startsWord(src, ls, lt, 39):
            nameStart: usize = ls + 5
            while nameStart < lt && isSpace(src[nameStart]):
                nameStart = nameStart + 1
            nameEnd: usize = nameStart
            while nameEnd < lt && isIdentContinue(src[nameEnd]):
                nameEnd = nameEnd + 1
            inside = nameEnd > nameStart && rangeEq(src, nameStart, nameEnd, enumStart, enumEnd)
            nextValue = 0
            pos = nextLine(src, pos)
            continue
        if inside:
            indent: usize = indentOf(src, pos, ls)
            if indent == 0:
                inside = false
            else if indent == 4 && ls < lt:
                variantEndLine: usize = ls
                while variantEndLine < lt && isIdentContinue(src[variantEndLine]):
                    variantEndLine = variantEndLine + 1
                if variantEndLine > ls:
                    currentValue: i64 = nextValue
                    eq: usize = variantEndLine
                    while eq < lt && src[eq] != 61:
                        eq = eq + 1
                    if eq < lt:
                        parsed: i64 = llvmParseEnumLiteral(src, eq + 1, lt)
                        if parsed < 0:
                            outFound.push(0)
                            return 0
                        currentValue = parsed
                    if rangeEq(src, ls, variantEndLine, variantStart, variantEnd):
                        outFound.push(1)
                        return currentValue
                    nextValue = currentValue + 1
        pos = nextLine(src, pos)
    outFound.push(0)
    return 0

llvmFindComptimeExpr(src: &[]u8, nameStart: usize, nameEnd: usize, outStart: &mut []i64, outEnd: &mut []i64) bool:
    pos: usize = 0
    found: bool = false
    bestStart: usize = 0
    bestEnd: usize = 0
    while pos < src.len():
        lineEndPos: usize = lineEnd(src, pos)
        ls: usize = trimStart(src, pos, lineEndPos)
        lt: usize = trimEnd(src, ls, lineEndPos)
        if ls < lt && pos < nameStart && startsWord(src, ls, lt, 45):
            declNameStart: usize = ls + 9
            while declNameStart < lt && isSpace(src[declNameStart]):
                declNameStart = declNameStart + 1
            declNameEnd: usize = declNameStart
            while declNameEnd < lt && isIdentContinue(src[declNameEnd]):
                declNameEnd = declNameEnd + 1
            if rangeEq(src, declNameStart, declNameEnd, nameStart, nameEnd):
                eqPos: usize = declNameEnd
                while eqPos < lt && src[eqPos] != 61:
                    eqPos = eqPos + 1
                if eqPos < lt:
                    valueStart: usize = trimStart(src, eqPos + 1, lt)
                    valueEnd: usize = trimEnd(src, valueStart, lt)
                    if valueStart < valueEnd:
                        found = true
                        bestStart = valueStart
                        bestEnd = valueEnd
        pos = nextLine(src, pos)
    if found:
        outStart.push(bestStart as i64)
        outEnd.push(bestEnd as i64)
    return found

llvmEvalComptimeInt(src: &[]u8, start: usize, end: usize, depth: usize, outValue: &mut []i64) bool:
    if depth > 32:
        return false
    s: usize = trimStart(src, start, end)
    t: usize = trimEnd(src, s, end)
    if s >= t:
        return false
    inner: usize = llvmStripParens(src, s, t)
    if inner > 0:
        return llvmEvalComptimeInt(src, inner, t - 1, depth + 1, outValue)
    if src[s] == 43 || src[s] == 45:
        # A unary sign is not reported by llvmFindLowestOp at position zero.
        absValue: []i64 = []i64
        if !llvmEvalComptimeInt(src, s + 1, t, depth + 1, &mut absValue) || absValue.len() == 0:
            return false
        vSign: i64 = absValue[0]
        if src[s] == 45:
            vSign = -vSign
        outValue.push(vSign)
        return true
    opPos: []usize = []usize
    opLen: []usize = []usize
    prec: i32 = llvmFindLowestOp(src, s, t, &mut opPos, &mut opLen)
    if prec >= 9:
        leftVals: []i64 = []i64
        rightVals: []i64 = []i64
        if !llvmEvalComptimeInt(src, s, opPos[0], depth + 1, &mut leftVals):
            return false
        if !llvmEvalComptimeInt(src, opPos[0] + opLen[0], t, depth + 1, &mut rightVals):
            return false
        if leftVals.len() == 0 || rightVals.len() == 0:
            return false
        l: i64 = leftVals[0]
        r: i64 = rightVals[0]
        result: i64 = 0
        if prec == 9:
            if opLen[0] != 1:
                return false
            if src[opPos[0]] == 43:
                result = l + r
            else:
                result = l - r
        else:
            if src[opPos[0]] == 42:
                result = l * r
            else if src[opPos[0]] == 47:
                if r == (0 as i64) || (l == (-9223372036854775807 as i64 - 1 as i64) && r == (-1 as i64)):
                    return false
                result = l / r
            else if src[opPos[0]] == 37:
                if r == (0 as i64) || (l == (-9223372036854775807 as i64 - 1 as i64) && r == (-1 as i64)):
                    return false
                result = l % r
            else:
                return false
        outValue.push(result)
        return true
    # Decimal/hex/binary integer literal.
    literalValue: i64 = llvmParseEnumLiteral(src, s, t)
    if literalValue >= (0 as i64):
        outValue.push(literalValue)
        return true
    # Negative literals are handled above, so an identifier here must be a comptime name.
    if !isIdentStart(src[s]):
        return false
    nameEnd: usize = s + 1
    while nameEnd < t && isIdentContinue(src[nameEnd]):
        nameEnd = nameEnd + 1
    if nameEnd != t:
        return false
    exprS: []i64 = []i64
    exprE: []i64 = []i64
    if !llvmFindComptimeExpr(src, s, t, &mut exprS, &mut exprE):
        return false
    return llvmEvalComptimeInt(src, exprS[0] as usize, exprE[0] as usize, depth + 1, outValue)

llvmResolveType(src: &[]u8, start: usize, end: usize, structs: &[]LLVMStructInfo, arrays: &mut []LLVMArrayInfo, fixedArrays: &mut []LLVMFixedArrayInfo) LLVMTypeInfo:
    s: usize = trimStart(src, start, end)
    t: usize = trimEnd(src, s, end)
    if t > s && src[t - 1] == 63:
        innerInfo: LLVMTypeInfo = llvmResolveType(src, s, t - 1, structs, arrays, fixedArrays)
        if innerInfo.code == 0 || innerInfo.code == 6 || innerInfo.code == 7 || innerInfo.code == 9 || innerInfo.code == 12:
            return LLVMTypeInfo[code: 0, bits: 0, isSigned: false]
        optionalIdx: usize = llvmInternOptionalPayload(fixedArrays, innerInfo.code, innerInfo.bits, innerInfo.isSigned)
        return LLVMTypeInfo[code: 13, bits: optionalIdx as i32, isSigned: false]
    if t - s == 5 && src[s] == 65 && src[s + 1] == 114 && src[s + 2] == 101 && src[s + 3] == 110 && src[s + 4] == 97:
        return LLVMTypeInfo[code: 18, bits: 0, isSigned: false]
    baseEndResult: usize = findTypeBaseEnd(src, s, t)
    if baseEndResult < t && src[baseEndResult] == 91 && src[t - 1] == 93 && startsWord(src, s, t, 38):
        commaResult: usize = splitGeneric(src, baseEndResult + 1, t - 1)
        if commaResult == t - 1:
            return LLVMTypeInfo[code: 0, bits: 0, isSigned: false]
        okInfo: LLVMTypeInfo = llvmResolveType(src, baseEndResult + 1, commaResult, structs, arrays, fixedArrays)
        errInfo: LLVMTypeInfo = llvmResolveType(src, commaResult + 1, t - 1, structs, arrays, fixedArrays)
        if okInfo.code == 0 || errInfo.code == 0 || okInfo.code == 6 || okInfo.code == 7 || okInfo.code == 9 || okInfo.code == 12 || errInfo.code == 6 || errInfo.code == 7 || errInfo.code == 9 || errInfo.code == 12:
            return LLVMTypeInfo[code: 0, bits: 0, isSigned: false]
        resultIdx: usize = llvmInternResultPayloads(fixedArrays, okInfo.code, okInfo.bits, okInfo.isSigned, errInfo.code, errInfo.bits, errInfo.isSigned)
        return LLVMTypeInfo[code: 14, bits: resultIdx as i32, isSigned: false]
    if baseEndResult < t && src[baseEndResult] == 91 && src[t - 1] == 93:
        # Views are ABI-lanner {ptr, i64} pairs. Reuse the array registry for
        # their element metadata so the bootstrap does not need another table.
        isViewType: bool = t - s >= 5 && src[s] == 86 && src[s + 1] == 105 && src[s + 2] == 101 && src[s + 3] == 119 && src[s + 4] == 91
        isEditViewType: bool = t - s >= 10 && src[s] == 69 && src[s + 1] == 100 && src[s + 2] == 105 && src[s + 3] == 116 && src[s + 4] == 86 && src[s + 5] == 105 && src[s + 6] == 101 && src[s + 7] == 119 && src[s + 8] == 91
        if isViewType || isEditViewType:
            viewElem: LLVMTypeInfo = llvmResolveType(src, baseEndResult + 1, t - 1, structs, arrays, fixedArrays)
            if viewElem.code != 1 && viewElem.code != 2 && viewElem.code != 3 && viewElem.code != 4 && viewElem.code != 9 && viewElem.code != 11 && viewElem.code != 13 && viewElem.code != 14 && viewElem.code != 15 && viewElem.code != 16 && viewElem.code != 17:
                return LLVMTypeInfo[code: 0, bits: 0, isSigned: false]
            viewIdx: usize = llvmInternArray(arrays, viewElem.code, viewElem.bits, viewElem.isSigned)
            if isEditViewType:
                return LLVMTypeInfo[code: 16, bits: viewIdx as i32, isSigned: false]
            return LLVMTypeInfo[code: 15, bits: viewIdx as i32, isSigned: false]
    if t - s >= 2 && src[s] == 38:
        # Reference type: `&T` or `&mut T`. Only aggregates are supported as
        # reference targets for now: code 6 = reference to a dynamic array,
        # code 7 = reference to a struct (`bits` carries the same registry
        # index as the owned type; both lower to `ptr`).
        inner: usize = s + 1
        if inner + 3 <= t && src[inner] == 109 && src[inner + 1] == 117 && src[inner + 2] == 116 && (inner + 3 == t || isSpace(src[inner + 3])):
            inner = inner + 3
        while inner < t && isSpace(src[inner]):
            inner = inner + 1
        innerInfo: LLVMTypeInfo = llvmResolveType(src, inner, t, structs, arrays, fixedArrays)
        if innerInfo.code == 5:
            return LLVMTypeInfo[code: 6, bits: innerInfo.bits, isSigned: false]
        if innerInfo.code == 4:
            return LLVMTypeInfo[code: 7, bits: innerInfo.bits, isSigned: false]
        if innerInfo.code == 11:
            return LLVMTypeInfo[code: 12, bits: innerInfo.bits, isSigned: false]
        return LLVMTypeInfo[code: 0, bits: 0, isSigned: false]
    if t - s >= 2 && src[s] == 91 && src[s + 1] == 93:
        # Dynamic arrays may contain scalars or already-registered structs.
        elemInfo: LLVMTypeInfo = llvmResolveType(src, s + 2, t, structs, arrays, fixedArrays)
        if elemInfo.code != 1 && elemInfo.code != 2 && elemInfo.code != 3 && elemInfo.code != 4:
            return LLVMTypeInfo[code: 0, bits: 0, isSigned: false]
        idx: usize = llvmInternArray(arrays, elemInfo.code, elemInfo.bits, elemInfo.isSigned)
        return LLVMTypeInfo[code: 5, bits: idx as i32, isSigned: false]
    if t > s && src[s] == 91:
        closeFixed: usize = s + 1
        depthFixed: usize = 0
        while closeFixed < t:
            if src[closeFixed] == 40:
                depthFixed = depthFixed + 1
            else if src[closeFixed] == 41 && depthFixed > 0:
                depthFixed = depthFixed - 1
            else if src[closeFixed] == 93 && depthFixed == 0:
                break
            closeFixed = closeFixed + 1
        if closeFixed < t && src[closeFixed] == 93 && closeFixed > s + 1:
            countVals: []i64 = []i64
            if !llvmEvalComptimeInt(src, s + 1, closeFixed, 0, &mut countVals) || countVals.len() == 0 || countVals[0] <= 0:
                return LLVMTypeInfo[code: 0, bits: 0, isSigned: false]
            countFixed: usize = countVals[0] as usize
            elemFixed: LLVMTypeInfo = llvmResolveType(src, closeFixed + 1, t, structs, arrays, fixedArrays)
            if elemFixed.code != 1 && elemFixed.code != 2 && elemFixed.code != 3 && elemFixed.code != 4 && elemFixed.code != 17:
                return LLVMTypeInfo[code: 0, bits: 0, isSigned: false]
            fi: usize = 0
            foundFixed: bool = false
            fscan: usize = 0
            while fscan < fixedArrays.len():
                if fixedArrays[fscan].elemCode == elemFixed.code && fixedArrays[fscan].elemBits == elemFixed.bits && fixedArrays[fscan].elemSigned == elemFixed.isSigned && fixedArrays[fscan].count == countFixed:
                    fi = fscan
                    foundFixed = true
                    break
                fscan = fscan + 1
            if !foundFixed:
                fixedArrays.push(LLVMFixedArrayInfo[elemCode: elemFixed.code, elemBits: elemFixed.bits, elemSigned: elemFixed.isSigned, count: countFixed, role: 0])
                fi = fixedArrays.len() - 1
            return LLVMTypeInfo[code: 11, bits: fi as i32, isSigned: false]
        return LLVMTypeInfo[code: 0, bits: 0, isSigned: false]
    scalar: LLVMTypeInfo = llvmTypeFromRange(src, s, t)
    if scalar.code != 0:
        return scalar
    idx2: i64 = llvmLookupStruct(src, structs, s, t)
    if idx2 >= 0:
        return LLVMTypeInfo[code: 4, bits: idx2 as i32, isSigned: false]
    if llvmLookupEnumType(src, s, t):
        return LLVMTypeInfo[code: 17, bits: 64, isSigned: true]
    return LLVMTypeInfo[code: 0, bits: 0, isSigned: false]

llvmEmitDynArrayTypeDecl() void:
    emitRaw("%LannerDynArray = type { ptr, i64, i64, ptr }\n\n")

llvmElemLLVMType(a: &LLVMArrayInfo) LLVMTypeInfo:
    return LLVMTypeInfo[code: a.elemCode, bits: a.elemBits, isSigned: a.elemSigned]

llvmEmitElemType(src: &[]u8, structs: &[]LLVMStructInfo, a: &LLVMArrayInfo) void:
    if a.elemCode == 1:
        emitLLVMIntType(a.elemBits)
    else if a.elemCode == 2:
        emitRaw("i1")
    else if a.elemCode == 3:
        if a.elemBits == 32:
            emitRaw("float")
        else:
            emitRaw("double")
    else if a.elemCode == 17:
        emitRaw("i64")
    else if a.elemCode == 4:
        emitRaw("%")
        emitRawRange(src, structs[a.elemBits as usize].nameStart, structs[a.elemBits as usize].nameEnd)
    else:
        emitRaw("i1")

llvmScalarAlignBytes(code: i32, bits: i32) usize:
    if code == 1:
        if bits >= 64:
            return 8
        if bits >= 32:
            return 4
        if bits >= 16:
            return 2
        return 1
    if code == 3:
        if bits == 32:
            return 4
        return 8
    return 1

llvmScalarStorageBytes(code: i32, bits: i32) usize:
    if code == 1:
        b: usize = (bits as usize + 7) / 8
        if b == 0:
            return 1
        return b
    if code == 3:
        if bits == 32:
            return 4
        return 8
    return 1

llvmStructByteSize(structs: &[]LLVMStructInfo, fields: &[]LLVMStructFieldInfo, structIndex: usize) usize:
    st: &LLVMStructInfo = &structs[structIndex]
    offset: usize = 0
    maxAlign: usize = 1
    i: usize = 0
    while i < st.fieldCount:
        f: &LLVMStructFieldInfo = &fields[st.fieldStart + i]
        alignment: usize = 1
        size: usize = 0
        if f.typeCode == 4:
            alignment = 8
            size = llvmStructByteSize(structs, fields, f.bits as usize)
            if size == 0:
                return 0
        else if f.typeCode == 5:
            alignment = 8
            size = 32
        else if f.typeCode == 1 || f.typeCode == 2:
            alignment = llvmScalarAlignBytes(f.typeCode, f.bits)
            size = llvmScalarStorageBytes(f.typeCode, f.bits)
        else if f.typeCode == 17:
            alignment = 8
            size = 8
        else:
            return 0
        if alignment > maxAlign:
            maxAlign = alignment
        rem: usize = offset % alignment
        if rem != 0:
            offset = offset + alignment - rem
        offset = offset + size
        i = i + 1
    rem2: usize = offset % maxAlign
    if rem2 != 0:
        offset = offset + maxAlign - rem2
    return offset

llvmElemByteSize(structs: &[]LLVMStructInfo, fields: &[]LLVMStructFieldInfo, a: &LLVMArrayInfo) usize:
    if a.elemCode == 1 || a.elemCode == 2 || a.elemCode == 3:
        return llvmScalarStorageBytes(a.elemCode, a.elemBits)
    if a.elemCode == 17:
        return 8
    if a.elemCode == 4:
        return llvmStructByteSize(structs, fields, a.elemBits as usize)
    return 0

llvmNative3EmitDynamicArrayLiteral(src: &[]u8, start: usize, end: usize, arenaStart: usize, arenaEnd: usize, expectedElemCode: i32, expectedElemBits: i32, expectedElemSigned: bool, ctx: &mut LLVMNativeContext, bindings: &mut []LLVMBinding, funcs: &[]LLVMFunctionSig, params: &[]LLVMParamInfo, structs: &[]LLVMStructInfo, fields: &[]LLVMStructFieldInfo, arrays: &mut []LLVMArrayInfo, fixedArrays: &mut []LLVMFixedArrayInfo) usize:
    s: usize = trimStart(src, start, end)
    t: usize = trimEnd(src, s, end)
    if s >= t || src[s] != 91 || t <= s + 1 || src[t - 1] != 93:
        return 0
    arenaAddrLabel: usize = 0
    if arenaStart != 0:
        arenaBind: i64 = llvmLookupBinding(src, bindings, arenaStart, arenaEnd)
        if arenaBind < 0 || bindings[arenaBind as usize].typeCode != 18 || bindings[arenaBind as usize].moved:
            return 0
        arenaAddrLabel = ctx.nextLabel
        ctx.nextLabel = ctx.nextLabel + 1
        emitRaw("  %arena.literal.addr")
        llvmNativeNumber(arenaAddrLabel)
        emitRaw(" = getelementptr inbounds i8, ptr ")
        emitLLVMSlotName(src, bindings[arenaBind as usize].nameStart, bindings[arenaBind as usize].nameEnd)
        emitRaw(", i64 0\n")

    # `[]T` is the typed empty dynamic-array literal.
    if t - s >= 3 && src[s + 1] == 93:
        elemEmpty: LLVMTypeInfo = LLVMTypeInfo[code: expectedElemCode, bits: expectedElemBits, isSigned: expectedElemSigned]
        if expectedElemCode == 0:
            elemEmpty = llvmResolveType(src, s + 2, t, structs, arrays, fixedArrays)
        if elemEmpty.code != 1 && elemEmpty.code != 2 && elemEmpty.code != 3 && elemEmpty.code != 4 && elemEmpty.code != 17:
            return 0
        arrIdxEmpty: usize = llvmInternArray(arrays, elemEmpty.code, elemEmpty.bits, elemEmpty.isSigned)
        empty0: usize = llvmNative3New(ctx, 5, arrIdxEmpty as i32, false, false, 0, 0)
        emitRaw("%t")
        llvmNativeNumber(empty0)
        emitRaw(" = insertvalue %LannerDynArray zeroinitializer, ptr null, 0\n")
        if arenaStart != 0:
            empty1: usize = llvmNative3New(ctx, 5, arrIdxEmpty as i32, false, false, 0, 0)
            emitRaw("%t")
            llvmNativeNumber(empty1)
            emitRaw(" = insertvalue %LannerDynArray %t")
            llvmNativeNumber(empty0)
            emitRaw(", ptr %arena.literal.addr")
            llvmNativeNumber(arenaAddrLabel)
            emitRaw(", 3\n")
            return empty1
        return empty0

    # Split non-empty literals at top-level commas, preserving evaluation order.
    partStarts: []usize = []usize
    partEnds: []usize = []usize
    pscan: usize = s + 1
    depth: usize = 0
    partStart: usize = pscan
    while pscan < t - 1:
        if src[pscan] == 40 || src[pscan] == 91:
            depth = depth + 1
        else if src[pscan] == 41 || src[pscan] == 93:
            if depth > 0:
                depth = depth - 1
        else if src[pscan] == 44 && depth == 0:
            ps: usize = trimStart(src, partStart, pscan)
            pe: usize = trimEnd(src, ps, pscan)
            if ps >= pe:
                return 0
            partStarts.push(ps)
            partEnds.push(pe)
            partStart = pscan + 1
        pscan = pscan + 1
    psLast: usize = trimStart(src, partStart, t - 1)
    peLast: usize = trimEnd(src, psLast, t - 1)
    if psLast >= peLast:
        return 0
    partStarts.push(psLast)
    partEnds.push(peLast)

    values: []usize = []usize
    firstValue: usize = 0
    if expectedElemCode != 0:
        firstValue = llvmNative3ExprAsType(src, partStarts[0], partEnds[0], expectedElemCode, expectedElemBits, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
    else:
        firstValue = llvmNative3Expr(src, partStarts[0], partEnds[0], ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
    if firstValue == 0:
        return 0
    elemCode: i32 = llvmNative3Code(ctx, firstValue)
    elemBits: i32 = llvmNative3Bits(ctx, firstValue)
    elemSigned: bool = llvmNative3Signed(ctx, firstValue)
    if elemCode != 1 && elemCode != 2 && elemCode != 3 && elemCode != 4 && elemCode != 17:
        return 0
    values.push(firstValue)
    vi: usize = 1
    while vi < partStarts.len():
        v: usize = 0
        if expectedElemCode != 0:
            v = llvmNative3ExprAsType(src, partStarts[vi], partEnds[vi], expectedElemCode, expectedElemBits, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
        else:
            v = llvmNative3Expr(src, partStarts[vi], partEnds[vi], ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
        if v == 0 || llvmNative3Code(ctx, v) != elemCode:
            return 0
        if elemCode == 1 && !llvmNative3IsImmediate(ctx, v) && llvmNative3Bits(ctx, v) != elemBits:
            return 0
        if (elemCode == 4 || elemCode == 17) && llvmNative3Bits(ctx, v) != elemBits:
            return 0
        values.push(v)
        vi = vi + 1

    arrIdx: usize = llvmInternArray(arrays, elemCode, elemBits, elemSigned)
    elemBytes: usize = llvmElemByteSize(structs, fields, &arrays[arrIdx])
    if elemBytes == 0:
        return 0
    count: usize = values.len()
    maxUsize: usize = (0 as usize) - 1
    if count > maxUsize / elemBytes:
        return 0
    totalBytes: usize = count * elemBytes

    dataId: usize = ctx.nextLabel
    ctx.nextLabel = ctx.nextLabel + 1
    emitRaw("  %array.literal.data")
    llvmNativeNumber(dataId)
    if arenaStart != 0:
        emitRaw(" = call ptr @__lanner_arena_alloc(ptr %arena.literal.addr")
        llvmNativeNumber(arenaAddrLabel)
        emitRaw(", i64 ")
        llvmNativeNumber(totalBytes)
        emitRaw(")\n")
    else:
        emitRaw(" = call ptr @malloc(i64 ")
        llvmNativeNumber(totalBytes)
        emitRaw(")\n")
        dataOk: usize = ctx.nextLabel
        ctx.nextLabel = ctx.nextLabel + 1
        dataGood: usize = ctx.nextLabel
        ctx.nextLabel = ctx.nextLabel + 1
        dataBad: usize = ctx.nextLabel
        ctx.nextLabel = ctx.nextLabel + 1
        emitRaw("  %array.literal.ok")
        llvmNativeNumber(dataOk)
        emitRaw(" = icmp ne ptr %array.literal.data")
        llvmNativeNumber(dataId)
        emitRaw(", null\n  br i1 %array.literal.ok")
        llvmNativeNumber(dataOk)
        emitRaw(", label ")
        llvmNativeEmitLabelRef("array.literal.good", dataGood)
        emitRaw(", label ")
        llvmNativeEmitLabelRef("array.literal.bad", dataBad)
        emitRaw("\n")
        llvmNativeEmitLabelDef("array.literal.bad", dataBad)
        emitRaw("  call void @llvm.trap()\n  unreachable\n")
        llvmNativeEmitLabelDef("array.literal.good", dataGood)

    wi: usize = 0
    while wi < values.len():
        elemAddr: usize = ctx.nextLabel
        ctx.nextLabel = ctx.nextLabel + 1
        emitRaw("  %array.literal.elem")
        llvmNativeNumber(elemAddr)
        emitRaw(" = getelementptr ")
        llvmEmitElemType(src, structs, &arrays[arrIdx])
        emitRaw(", ptr %array.literal.data")
        llvmNativeNumber(dataId)
        emitRaw(", i64 ")
        llvmNativeNumber(wi)
        emitRaw("\n  store ")
        llvmEmitElemType(src, structs, &arrays[arrIdx])
        emitRaw(" ")
        llvmNative3EmitValue(src, ctx, values[wi])
        emitRaw(", ptr %array.literal.elem")
        llvmNativeNumber(elemAddr)
        emitRaw("\n")
        wi = wi + 1

    a0: usize = llvmNative3New(ctx, 5, arrIdx as i32, false, false, 0, 0)
    emitRaw("%t")
    llvmNativeNumber(a0)
    emitRaw(" = insertvalue %LannerDynArray zeroinitializer, ptr %array.literal.data")
    llvmNativeNumber(dataId)
    emitRaw(", 0\n")
    a1: usize = llvmNative3New(ctx, 5, arrIdx as i32, false, false, 0, 0)
    emitRaw("%t")
    llvmNativeNumber(a1)
    emitRaw(" = insertvalue %LannerDynArray %t")
    llvmNativeNumber(a0)
    emitRaw(", i64 ")
    llvmNativeNumber(count)
    emitRaw(", 1\n")
    a2: usize = llvmNative3New(ctx, 5, arrIdx as i32, false, false, 0, 0)
    emitRaw("%t")
    llvmNativeNumber(a2)
    emitRaw(" = insertvalue %LannerDynArray %t")
    llvmNativeNumber(a1)
    emitRaw(", i64 ")
    llvmNativeNumber(count)
    emitRaw(", 2\n")
    if arenaStart != 0:
        a3: usize = llvmNative3New(ctx, 5, arrIdx as i32, false, false, 0, 0)
        emitRaw("%t")
        llvmNativeNumber(a3)
        emitRaw(" = insertvalue %LannerDynArray %t")
        llvmNativeNumber(a2)
        emitRaw(", ptr %arena.literal.addr")
        llvmNativeNumber(arenaAddrLabel)
        emitRaw(", 3\n")
        return a3
    return a2

llvmFindStructLitOpen(src: &[]u8, s: usize, t: usize) usize:
    i: usize = s
    if i >= t || !(src[i] >= 65 && src[i] <= 90):
        return 0
    while i < t && (src[i] >= 65 && src[i] <= 90 || src[i] >= 97 && src[i] <= 122 || src[i] >= 48 && src[i] <= 57 || src[i] == 95):
        i = i + 1
    if i > s && i < t && src[i] == 91 && t > i && src[t - 1] == 93:
        return i
    return 0

llvmFindTrailingDot(src: &[]u8, s: usize, t: usize) usize:
    depth: usize = 0
    i: usize = s
    last: usize = 0
    while i < t:
        if src[i] == 40 || src[i] == 91:
            depth = depth + 1
        else if src[i] == 41 || src[i] == 93:
            if depth > 0:
                depth = depth - 1
        else if src[i] == 46 && depth == 0:
            last = i
        i = i + 1
    if last == 0:
        return 0
    if last + 1 >= t:
        return 0
    j: usize = last + 1
    while j < t:
        c: u8 = src[j]
        if !(c >= 97 && c <= 122 || c >= 65 && c <= 90 || c >= 48 && c <= 57 || c == 95):
            return 0
        j = j + 1
    if !(src[last + 1] >= 97 && src[last + 1] <= 122 || src[last + 1] >= 65 && src[last + 1] <= 90 || src[last + 1] == 95):
        return 0
    return last

llvmFindMethodDot(src: &[]u8, s: usize, t: usize) usize:
    if t == 0 || t <= s || src[t - 1] != 41:
        return 0
    depth: usize = 0
    i: usize = s
    last: usize = 0
    while i < t:
        if src[i] == 40 || src[i] == 91:
            depth = depth + 1
        else if src[i] == 41 || src[i] == 93:
            if depth > 0:
                depth = depth - 1
        else if src[i] == 46 && depth == 0:
            last = i
        i = i + 1
    return last

llvmFindTrailingIndex(src: &[]u8, s: usize, t: usize) usize:
    if t == 0 || t <= s || src[t - 1] != 93:
        return 0
    depth: usize = 0
    i: usize = s
    last: usize = 0
    while i < t:
        c: u8 = src[i]
        if c == 91:
            if depth == 0:
                last = i
            depth = depth + 1
        else if c == 40:
            depth = depth + 1
        else if c == 93 || c == 41:
            if depth > 0:
                depth = depth - 1
        i = i + 1
    return last

llvmFindSliceColon(src: &[]u8, open: usize, end: usize) usize:
    if open == 0 || end <= open + 1 || src[end - 1] != 93:
        return 0
    depth: usize = 0
    i: usize = open + 1
    while i < end - 1:
        if src[i] == 34:
            i = llvmNativeSkipString(src, i, end - 1)
            continue
        if src[i] == 40 || src[i] == 91:
            depth = depth + 1
        else if src[i] == 41 || src[i] == 93:
            if depth > 0:
                depth = depth - 1
        else if src[i] == 58 && depth == 0:
            return i
        i = i + 1
    return 0

llvmEmitAllocaType(code: i32, bits: i32, src: &[]u8, structs: &[]LLVMStructInfo, arrays: &mut []LLVMArrayInfo, fixedArrays: &mut []LLVMFixedArrayInfo) void:
    if code == 1:
        emitLLVMIntType(bits)
    else if code == 3:
        if bits == 32:
            emitRaw("float")
        else:
            emitRaw("double")
    else if code == 17:
        emitRaw("i64")
    else if code == 2:
        emitRaw("i1")
    else if code == 4:
        emitRaw("%")
        emitRawRange(src, structs[bits as usize].nameStart, structs[bits as usize].nameEnd)
    else if code == 5:
        emitRaw("%LannerDynArray")
    else if code == 6 || code == 7 || code == 9 || code == 12:
        emitRaw("ptr")
    else if code == 11:
        fa: &LLVMFixedArrayInfo = &fixedArrays[bits as usize]
        emitRaw("[")
        llvmNativeNumber(fa.count)
        emitRaw(" x ")
        llvmEmitAllocaType(fa.elemCode, fa.elemBits, src, structs, arrays, fixedArrays)
        emitRaw("]")
    else if code == 15 || code == 16:
        emitRaw("{ ptr, i64 }")
    else if code == 18:
        emitRaw("%LannerArena")
    else if code == 13:
        opt: &LLVMFixedArrayInfo = &fixedArrays[bits as usize]
        emitRaw("{ i1, ")
        llvmEmitAllocaType(opt.elemCode, opt.elemBits, src, structs, arrays, fixedArrays)
        emitRaw(" }")
    else if code == 14:
        ok: &LLVMFixedArrayInfo = &fixedArrays[bits as usize]
        err: &LLVMFixedArrayInfo = &fixedArrays[bits as usize + 1]
        emitRaw("{ i1, ")
        llvmEmitAllocaType(ok.elemCode, ok.elemBits, src, structs, arrays, fixedArrays)
        emitRaw(", ")
        llvmEmitAllocaType(err.elemCode, err.elemBits, src, structs, arrays, fixedArrays)
        emitRaw(" }")
    else:
        emitRaw("i1")

# Native backend registry builders consuming the canonical typed module. The
# backend still retains source spans for diagnostics/naming, but declaration
# structure and parameter/field boundaries come from the typed front end rather
# than reparsing each comma-separated declaration independently.
llvmStructIndexBySymbol(nodes: &[]AstNode, symbolId: i64) i64:
    ordinal: usize = 0
    i: usize = 0
    while i < nodes.len():
        if nodes[i].level == 0 && nodes[i].kind == 11:
            if nodes[i].symbolId == symbolId:
                return ordinal as i64
            ordinal = ordinal + 1
        i = i + 1
    return (0 as i64) - 1

llvmBuildTypedStructRegistry(src: &[]u8, nodes: &[]AstNode, typedFields: &[]TypedFieldInfo, structs: &mut []LLVMStructInfo, fields: &mut []LLVMStructFieldInfo, arrays: &mut []LLVMArrayInfo, fixedArrays: &mut []LLVMFixedArrayInfo) bool:
    i: usize = 0
    while i < nodes.len():
        if nodes[i].level == 0 && nodes[i].kind == 11:
            s: usize = trimStart(src, nodes[i].start, nodes[i].end)
            t: usize = trimEnd(src, s, nodes[i].end)
            open: usize = s
            while open < t && src[open] != 91:
                open = open + 1
            if open >= t:
                return false
            structs.push(LLVMStructInfo[nameStart: s, nameEnd: open, fieldStart: 0, fieldCount: 0])
        i = i + 1
    structNames: []LLVMStructInfo = []LLVMStructInfo
    i = 0
    while i < structs.len():
        structNames.push(LLVMStructInfo[nameStart: structs[i].nameStart, nameEnd: structs[i].nameEnd, fieldStart: structs[i].fieldStart, fieldCount: structs[i].fieldCount])
        i = i + 1
    i = 0
    while i < typedFields.len():
        si: i64 = llvmStructIndexBySymbol(nodes, typedFields[i].structSymbolId)
        if si < 0:
            return false
        if structs[si as usize].fieldCount == 0:
            structs[si as usize].fieldStart = fields.len()
        info: LLVMTypeInfo = llvmResolveType(src, typedFields[i].typeStart, typedFields[i].typeEnd, &structNames, arrays, fixedArrays)
        if info.code == 0:
            return false
        fields.push(LLVMStructFieldInfo[nameStart: typedFields[i].nameStart, nameEnd: typedFields[i].nameEnd, typeCode: info.code, bits: info.bits, isSigned: info.isSigned])
        structs[si as usize].fieldCount = structs[si as usize].fieldCount + 1
        i = i + 1
    return true

llvmBuildTypedFunctionSigs(src: &[]u8, nodes: &[]AstNode, typedParams: &[]TypedParamInfo, funcs: &mut []LLVMFunctionSig, params: &mut []LLVMParamInfo, structs: &[]LLVMStructInfo, arrays: &mut []LLVMArrayInfo, fixedArrays: &mut []LLVMFixedArrayInfo) bool:
    i: usize = 0
    while i < nodes.len():
        if nodes[i].level == 0 && nodes[i].kind == 1:
            s: usize = trimStart(src, nodes[i].start, nodes[i].end)
            t: usize = trimEnd(src, s, nodes[i].end)
            open: usize = s
            while open < t && src[open] != 40:
                open = open + 1
            close: usize = open + 1
            depth: usize = 1
            while close < t && depth > 0:
                if src[close] == 40:
                    depth = depth + 1
                else if src[close] == 41:
                    depth = depth - 1
                close = close + 1
            if open >= t || close == 0:
                return false
            close = close - 1
            retStart: usize = trimStart(src, close + 1, t)
            retEnd: usize = trimEnd(src, retStart, t)
            if retEnd > retStart && src[retEnd - 1] == 58:
                retEnd = trimEnd(src, retStart, retEnd - 1)
            ret: LLVMTypeInfo = llvmResolveType(src, retStart, retEnd, structs, arrays, fixedArrays)
            if ret.code == 0 && !(retEnd - retStart == 4 && src[retStart] == 118 && src[retStart + 1] == 111 && src[retStart + 2] == 105 && src[retStart + 3] == 100):
                return false
            fnIndex: usize = funcs.len()
            funcs.push(LLVMFunctionSig[nameStart: s, nameEnd: open, returnCode: ret.code, returnBits: ret.bits, returnSigned: ret.isSigned, paramStart: params.len(), paramCount: 0])
            j: usize = 0
            while j < typedParams.len():
                if typedParams[j].fnSymbolId == nodes[i].symbolId:
                    info: LLVMTypeInfo = llvmResolveType(src, typedParams[j].typeStart, typedParams[j].typeEnd, structs, arrays, fixedArrays)
                    if info.code == 0:
                        return false
                    params.push(LLVMParamInfo[fnIndex: fnIndex, nameStart: typedParams[j].nameStart, nameEnd: typedParams[j].nameEnd, typeCode: info.code, bits: info.bits, isSigned: info.isSigned])
                    funcs[fnIndex].paramCount = funcs[fnIndex].paramCount + 1
                j = j + 1
            i = i + 1
        else if nodes[i].level == 0 && nodes[i].kind != 0:
            if nodes[i].kind == 2 || nodes[i].kind == 9:
                return false
        i = i + 1
    return funcs.len() > 0

llvmBuildFunctionSigs(src: &[]u8, nodes: &[]AstNode, funcs: &mut []LLVMFunctionSig, params: &mut []LLVMParamInfo, structs: &[]LLVMStructInfo, arrays: &mut []LLVMArrayInfo, fixedArrays: &mut []LLVMFixedArrayInfo) bool:
    i: usize = 0
    while i < nodes.len():
        if nodes[i].level == 0 && nodes[i].kind == 1:
            s: usize = trimStart(src, nodes[i].start, nodes[i].end)
            t: usize = trimEnd(src, s, nodes[i].end)
            open: usize = s
            while open < t && src[open] != 40:
                open = open + 1
            close: usize = open + 1
            depth: usize = 1
            while close < t && depth > 0:
                if src[close] == 40:
                    depth = depth + 1
                else if src[close] == 41:
                    depth = depth - 1
                close = close + 1
            if open >= t || close == 0:
                return false
            close = close - 1
            retStart: usize = trimStart(src, close + 1, t)
            retEnd: usize = trimEnd(src, retStart, t - 1)
            ret: LLVMTypeInfo = llvmResolveType(src, retStart, retEnd, structs, arrays, fixedArrays)
            if ret.code == 0 && !(retEnd - retStart == 4 && src[retStart] == 118 && src[retStart + 1] == 111 && src[retStart + 2] == 105 && src[retStart + 3] == 100):
                return false
            fnIndex: usize = funcs.len()
            nameEnd: usize = open
            funcs.push(LLVMFunctionSig[nameStart: s, nameEnd: nameEnd, returnCode: ret.code, returnBits: ret.bits, returnSigned: ret.isSigned, paramStart: params.len(), paramCount: 0])
            p: usize = open + 1
            while p < close:
                q: usize = p
                depth2: usize = 0
                while q < close:
                    if src[q] == 91 || src[q] == 40:
                        depth2 = depth2 + 1
                    else if src[q] == 93 || src[q] == 41:
                        if depth2 > 0:
                            depth2 = depth2 - 1
                    else if src[q] == 44 && depth2 == 0:
                        break
                    q = q + 1
                ps: usize = trimStart(src, p, q)
                pe: usize = trimEnd(src, ps, q)
                if ps < pe:
                    colon: usize = ps
                    while colon < pe && src[colon] != 58:
                        colon = colon + 1
                    if colon >= pe:
                        return false
                    paramNameStart: usize = ps
                    paramNameEnd: usize = colon
                    typeStart: usize = trimStart(src, colon + 1, pe)
                    info: LLVMTypeInfo = llvmResolveType(src, typeStart, pe, structs, arrays, fixedArrays)
                    if info.code == 0:
                        return false
                    params.push(LLVMParamInfo[fnIndex: fnIndex, nameStart: paramNameStart, nameEnd: paramNameEnd, typeCode: info.code, bits: info.bits, isSigned: info.isSigned])
                    funcs[fnIndex].paramCount = funcs[fnIndex].paramCount + 1
                p = q + 1
            i = i + 1
        else if nodes[i].level == 0 && nodes[i].kind != 0:
            if nodes[i].kind == 2 || nodes[i].kind == 9:
                return false
        i = i + 1
    return funcs.len() > 0

emitLLVMFunction(src: &[]u8, nodes: &[]AstNode, children: &[]i64, fnNode: usize, funcs: &[]LLVMFunctionSig, params: &[]LLVMParamInfo) bool:
    s: usize = trimStart(src, nodes[fnNode].start, nodes[fnNode].end)
    t: usize = trimEnd(src, s, nodes[fnNode].end)
    open: usize = s
    while open < t && src[open] != 40:
        open = open + 1
    fnIndex: i64 = llvmLookupFunction(src, funcs, s, open)
    if fnIndex < 0:
        return false
    emitRaw("define ")
    if funcs[fnIndex as usize].returnCode == 0:
        emitRaw("void")
    else if funcs[fnIndex as usize].returnCode == 1:
        emitLLVMIntType(funcs[fnIndex as usize].returnBits)
    else if funcs[fnIndex as usize].returnCode == 2:
        emitRaw("i1")
    else if funcs[fnIndex as usize].returnCode == 3:
        if funcs[fnIndex as usize].returnBits == 32:
            emitRaw("float")
        else:
            emitRaw("double")
    emitRaw(" @")
    emitRawRange(src, s, open)
    emitRaw("(")
    pIndex: usize = funcs[fnIndex as usize].paramStart
    count: usize = 0
    while count < funcs[fnIndex as usize].paramCount:
        if count > 0:
            emitRaw(", ")
        p: &LLVMParamInfo = &params[pIndex + count]
        if p.typeCode == 1:
            emitLLVMIntType(p.bits)
        else if p.typeCode == 2:
            emitRaw("i1")
        else if p.typeCode == 3:
            if p.bits == 32:
                emitRaw("float")
            else:
                emitRaw("double")
        emitRaw(" %")
        emitRawRange(src, p.nameStart, p.nameEnd)
        count = count + 1
    emitRaw(") {\nentry:\n")

    bindings: []LLVMBinding = []LLVMBinding
    count = 0
    while count < funcs[fnIndex as usize].paramCount:
        p: &LLVMParamInfo = &params[pIndex + count]
        legacyParamOwns: bool = p.typeCode == 5 || p.typeCode == 18
        bindings.push(LLVMBinding[nameStart: p.nameStart, nameEnd: p.nameEnd, typeCode: p.typeCode, bits: p.bits, isSigned: p.isSigned, owning: legacyParamOwns, initialized: true, moved: false])
        emitRaw("  ")
        emitLLVMSlotName(src, p.nameStart, p.nameEnd)
        emitRaw(" = alloca ")
        if p.typeCode == 1:
            emitLLVMIntType(p.bits)
        else if p.typeCode == 2:
            emitRaw("i1")
        else if p.bits == 32:
            emitRaw("float")
        else:
            emitRaw("double")
        emitRaw("\n  store ")
        if p.typeCode == 1:
            emitLLVMIntType(p.bits)
        else if p.typeCode == 2:
            emitRaw("i1")
        else if p.bits == 32:
            emitRaw("float")
        else:
            emitRaw("double")
        emitRaw(" %")
        emitRawRange(src, p.nameStart, p.nameEnd)
        emitRaw(", ptr ")
        emitLLVMSlotName(src, p.nameStart, p.nameEnd)
        emitRaw("\n")
        count = count + 1

    bodyTerminated: bool = llvmEmitSequence(src, nodes, children, nodes[fnNode].childStart, nodes[fnNode].childCount, &mut bindings, funcs, params)
    if !bodyTerminated:
        if funcs[fnIndex as usize].returnCode == 0:
            emitRaw("  ret void\n")
        else if funcs[fnIndex as usize].returnCode == 1:
            emitRaw("  ret ")
            emitLLVMIntType(funcs[fnIndex as usize].returnBits)
            emitRaw(" 0\n")
        else if funcs[fnIndex as usize].returnCode == 2:
            emitRaw("  ret i1 0\n")
        else:
            return false
    emitRaw("}\n\n")
    return true

llvmNativeNumber(n: usize) void:
    if n >= 10:
        llvmNativeNumber(n / 10)
    digit: usize = n % 10
    if digit == 0:
        emitRaw("0")
    else if digit == 1:
        emitRaw("1")
    else if digit == 2:
        emitRaw("2")
    else if digit == 3:
        emitRaw("3")
    else if digit == 4:
        emitRaw("4")
    else if digit == 5:
        emitRaw("5")
    else if digit == 6:
        emitRaw("6")
    else if digit == 7:
        emitRaw("7")
    else if digit == 8:
        emitRaw("8")
    else:
        emitRaw("9")

llvmNativeEmitLabelRef(prefix: string, id: usize) void:
    emitRaw("%\"")
    emitRaw(prefix)
    llvmNativeNumber(id)
    emitRaw("\"")

llvmNativeEmitLabelDef(prefix: string, id: usize) void:
    emitRaw("\"")
    emitRaw(prefix)
    llvmNativeNumber(id)
    emitRaw("\":\n")

llvmNativeSubtreeEnd(nodes: &[]AstNode, nodeIndex: usize, end: usize) usize:
    level: usize = nodes[nodeIndex].level
    i: usize = nodeIndex + 1
    while i < end && nodes[i].level > level:
        i = i + 1
    return i

llvmNativeSkipString(src: &[]u8, start: usize, end: usize) usize:
    i: usize = start
    if i >= end || src[i] != 34:
        return i
    i = i + 1
    while i < end:
        if src[i] == 92:
            if i + 1 < end:
                i = i + 2
            else:
                i = i + 1
        else if src[i] == 34:
            return i + 1
        else:
            i = i + 1
    return end

llvmNativeFindCallOpen(src: &[]u8, s: usize, t: usize) usize:
    depth: usize = 0
    i: usize = s
    while i < t:
        if src[i] == 34:
            i = llvmNativeSkipString(src, i, t)
            continue
        if src[i] == 40:
            if depth == 0:
                # A top-level `(` is a call only when the immediate callee
                # token begins the whole expression. Parenthesized groups and
                # operators such as `a && foo(...)` must reach the operator
                # parser first. Method calls are parsed later via the dot path.
                j: usize = i
                while j > s && isSpace(src[j - 1]):
                    j = j - 1
                if j > s && isIdentContinue(src[j - 1]):
                    k: usize = j
                    while k > s && (isIdentContinue(src[k - 1]) || src[k - 1] == 46):
                        k = k - 1
                    if k == s:
                        return i
            depth = depth + 1
        else if src[i] == 41:
            if depth > 0:
                depth = depth - 1
        i = i + 1
    return 0

llvmNativeMatchParen(src: &[]u8, open: usize, t: usize) usize:
    depth: usize = 0
    i: usize = open
    while i < t:
        if src[i] == 34:
            i = llvmNativeSkipString(src, i, t)
            continue
        if src[i] == 40:
            depth = depth + 1
        else if src[i] == 41:
            if depth > 0:
                depth = depth - 1
            if depth == 0:
                return i
        i = i + 1
    return 0

LLVMComptimeDecl[nameStart: usize, nameEnd: usize, valueStart: usize, valueEnd: usize]
LLVMNativeContext[nextValue: usize, nextLabel: usize, values: []i64, comptimes: []LLVMComptimeDecl]

llvmNative3Init(ctx: &mut LLVMNativeContext, src: &[]u8) void:
    ctx.nextValue = 1
    ctx.nextLabel = 0
    ctx.comptimes = []LLVMComptimeDecl
    comptimeList: &mut []LLVMComptimeDecl = &mut ctx.comptimes
    posComptime: usize = 0
    while posComptime < src.len():
        lineEndComptime: usize = lineEnd(src, posComptime)
        lsComptime: usize = trimStart(src, posComptime, lineEndComptime)
        ltComptime: usize = trimEnd(src, lsComptime, lineEndComptime)
        if lsComptime < ltComptime && startsWord(src, lsComptime, ltComptime, 45):
            declNameStartComptime: usize = lsComptime + 9
            while declNameStartComptime < ltComptime && isSpace(src[declNameStartComptime]):
                declNameStartComptime = declNameStartComptime + 1
            declNameEndComptime: usize = declNameStartComptime
            while declNameEndComptime < ltComptime && isIdentContinue(src[declNameEndComptime]):
                declNameEndComptime = declNameEndComptime + 1
            eqComptime: usize = declNameEndComptime
            while eqComptime < ltComptime && src[eqComptime] != 61:
                eqComptime = eqComptime + 1
            if declNameEndComptime > declNameStartComptime && eqComptime < ltComptime:
                valueStartComptime: usize = trimStart(src, eqComptime + 1, ltComptime)
                valueEndComptime: usize = trimEnd(src, valueStartComptime, ltComptime)
                if valueStartComptime < valueEndComptime:
                    comptimeList.push(LLVMComptimeDecl[nameStart: declNameStartComptime, nameEnd: declNameEndComptime, valueStart: valueStartComptime, valueEnd: valueEndComptime])
        posComptime = nextLine(src, posComptime)
    values: &mut []i64 = &mut ctx.values
    values.push(0)
    values.push(0)
    values.push(0)
    values.push(0)
    values.push(0)
    values.push(0)

llvmFindComptimeValue(src: &[]u8, nameStart: usize, nameEnd: usize, ctx: &mut LLVMNativeContext, outStart: &mut []i64, outEnd: &mut []i64) bool:
    i: usize = 0
    found: bool = false
    best: usize = 0
    while i < ctx.comptimes.len():
        d: &LLVMComptimeDecl = &ctx.comptimes[i]
        if d.valueStart < nameStart && rangeEq(src, d.nameStart, d.nameEnd, nameStart, nameEnd):
            best = i
            found = true
        i = i + 1
    if found:
        outStart.push(ctx.comptimes[best].valueStart as i64)
        outEnd.push(ctx.comptimes[best].valueEnd as i64)
    return found

llvmNative3New(ctx: &mut LLVMNativeContext, code: i32, bits: i32, isSigned: bool, immediate: bool, start: usize, end: usize) usize:
    id: usize = ctx.nextValue
    ctx.nextValue = ctx.nextValue + 1
    values: &mut []i64 = &mut ctx.values
    values.push(code as i64)
    values.push(bits as i64)
    if isSigned:
        values.push(1)
    else:
        values.push(0)
    if immediate:
        values.push(1)
    else:
        values.push(0)
    values.push(start as i64)
    values.push(end as i64)
    return id

llvmNative3Code(ctx: &mut LLVMNativeContext, id: usize) i32:
    return ctx.values[id * 6] as i32

llvmNative3Bits(ctx: &mut LLVMNativeContext, id: usize) i32:
    return ctx.values[id * 6 + 1] as i32

llvmNative3Signed(ctx: &mut LLVMNativeContext, id: usize) bool:
    return ctx.values[id * 6 + 2] != 0

llvmNative3IsImmediate(ctx: &mut LLVMNativeContext, id: usize) bool:
    return ctx.values[id * 6 + 3] != 0

llvmNative3ImmStart(ctx: &mut LLVMNativeContext, id: usize) usize:
    return ctx.values[id * 6 + 4] as usize

llvmNative3ImmEnd(ctx: &mut LLVMNativeContext, id: usize) usize:
    return ctx.values[id * 6 + 5] as usize

llvmNativeStringId(src: &[]u8, target: usize) usize:
    id: usize = 0
    i: usize = 0
    while i < target && i < src.len():
        if src[i] == 35:
            i = lineEnd(src, i)
            continue
        if src[i] == 34:
            if i == target:
                return id
            id = id + 1
            i = i + 1
            while i < src.len():
                if src[i] == 92:
                    i = i + 2
                else if src[i] == 34:
                    i = i + 1
                    break
                else:
                    i = i + 1
            continue
        i = i + 1
    return id

llvmStringDecodedLen(src: &[]u8, start: usize, end: usize) usize:
    i: usize = start + 1
    outLen: usize = 0
    while i < end && i < src.len():
        if src[i] == 34:
            break
        if src[i] == 92 && i + 1 < end:
            outLen = outLen + 1
            i = i + 2
        else:
            outLen = outLen + 1
            i = i + 1
    return outLen

llvmStringBoundsById(src: &[]u8, wanted: usize, outStart: &mut []i64, outEnd: &mut []i64) bool:
    id: usize = 0
    i: usize = 0
    while i < src.len():
        if src[i] == 35:
            i = lineEnd(src, i)
            continue
        if src[i] == 34:
            start: usize = i
            i = i + 1
            while i < src.len():
                if src[i] == 92:
                    i = i + 2
                else if src[i] == 34:
                    i = i + 1
                    break
                else:
                    i = i + 1
            if id == wanted:
                outStart.push(start as i64)
                outEnd.push(i as i64)
                return true
            id = id + 1
            continue
        i = i + 1
    return false

emitLLVMHexByte(b: u8) void:
    emitRaw("\\")
    hi: u8 = b / 16
    lo: u8 = b % 16
    if hi < 10:
        writeByteRaw((48 as i64) + (hi as i64))
    else:
        writeByteRaw((65 as i64) + (hi as i64) - 10)
    if lo < 10:
        writeByteRaw((48 as i64) + (lo as i64))
    else:
        writeByteRaw((65 as i64) + (lo as i64) - 10)

emitNativeStringGlobals(src: &[]u8) void:
    id: usize = 0
    i: usize = 0
    while i < src.len():
        if src[i] == 35:
            i = lineEnd(src, i)
            continue
        if src[i] != 34:
            i = i + 1
            continue
        start: usize = i
        i = i + 1
        while i < src.len():
            if src[i] == 92:
                i = i + 2
            else if src[i] == 34:
                i = i + 1
                break
            else:
                i = i + 1
        decodedLen: usize = llvmStringDecodedLen(src, start, i)
        emitRaw("@.lanner.str.")
        llvmNativeNumber(id)
        emitRaw(" = private unnamed_addr constant [")
        llvmNativeNumber(decodedLen + 1)
        emitRaw(" x i8] c\"")
        p2: usize = start + 1
        while p2 < i - 1:
            b: u8 = src[p2]
            if b == 92 && p2 + 1 < i - 1:
                esc: u8 = src[p2 + 1]
                if esc == 110:
                    emitLLVMHexByte(10)
                else if esc == 114:
                    emitLLVMHexByte(13)
                else if esc == 116:
                    emitLLVMHexByte(9)
                else if esc == 48:
                    emitLLVMHexByte(0)
                else if esc == 92:
                    emitLLVMHexByte(92)
                else if esc == 34:
                    emitLLVMHexByte(34)
                else:
                    emitLLVMHexByte(esc)
                p2 = p2 + 2
            else:
                emitLLVMHexByte(b)
                p2 = p2 + 1
        emitRaw("\\00\"")
        emitRaw("\n")
        id = id + 1
    if id > 0:
        emitRaw("\n")

llvmNative3EmitValue(src: &[]u8, ctx: &mut LLVMNativeContext, id: usize) void:
    if llvmNative3Code(ctx, id) == 9 && llvmNative3IsImmediate(ctx, id):
        startList: []i64 = []i64
        endList: []i64 = []i64
        wanted: usize = llvmNative3ImmStart(ctx, id)
        if !llvmStringBoundsById(src, wanted, &mut startList, &mut endList):
            return
        decodedLen: usize = llvmStringDecodedLen(src, startList[0] as usize, endList[0] as usize)
        emitRaw("getelementptr inbounds ([")
        llvmNativeNumber(decodedLen + 1)
        emitRaw(" x i8], ptr @.lanner.str.")
        llvmNativeNumber(wanted)
        emitRaw(", i64 0, i64 0)")
        return
    if llvmNative3IsImmediate(ctx, id):
        emitRawRange(src, llvmNative3ImmStart(ctx, id), llvmNative3ImmEnd(ctx, id))
    else:
        emitRaw("%t")
        llvmNativeNumber(id)

llvmNative3Type(ctx: &mut LLVMNativeContext, id: usize) void:
    if llvmNative3Code(ctx, id) == 1:
        emitLLVMIntType(llvmNative3Bits(ctx, id))
    else if llvmNative3Code(ctx, id) == 3:
        if llvmNative3Bits(ctx, id) == 32:
            emitRaw("float")
        else:
            emitRaw("double")
    else if llvmNative3Code(ctx, id) == 2:
        emitRaw("i1")
    else if llvmNative3Code(ctx, id) == 17:
        emitRaw("i64")
    else if llvmNative3Code(ctx, id) == 9:
        emitRaw("ptr")
    else if llvmNative3Code(ctx, id) == 15 || llvmNative3Code(ctx, id) == 16:
        emitRaw("{ ptr, i64 }")
    else if llvmNative3Code(ctx, id) == 18:
        emitRaw("%LannerArena")
    else:
        emitRaw("i1")

llvmNative3SameType(ctx: &mut LLVMNativeContext, a: usize, b: usize) bool:
    if llvmNative3Code(ctx, a) != llvmNative3Code(ctx, b):
        return false
    if (llvmNative3Code(ctx, a) == 1 || llvmNative3Code(ctx, a) == 3 || llvmNative3Code(ctx, a) == 11 || llvmNative3Code(ctx, a) == 13 || llvmNative3Code(ctx, a) == 14 || llvmNative3Code(ctx, a) == 15 || llvmNative3Code(ctx, a) == 16) && !llvmNative3IsImmediate(ctx, a) && !llvmNative3IsImmediate(ctx, b) && llvmNative3Bits(ctx, a) != llvmNative3Bits(ctx, b):
        return false
    return true

llvmNative3Cast(src: &[]u8, ctx: &mut LLVMNativeContext, value: usize, dst: &LLVMTypeInfo) usize:
    if value == 0 || llvmNative3Code(ctx, value) == 0:
        return 0
    # Reference declarations consume the raw pointer produced by `&` / `&mut`.
    # code 8 is the internal LLVM pointer SSA kind; reference types are stored
    # as ptr as well, so no IR conversion is necessary here.
    if llvmNative3Code(ctx, value) == 8 && (dst.code == 6 || dst.code == 7):
        return value
    if llvmNative3Code(ctx, value) == dst.code && (llvmNative3Code(ctx, value) != 1 || llvmNative3Bits(ctx, value) == dst.bits):
        return value
    if llvmNative3Code(ctx, value) == 3 && dst.code == 3:
        if llvmNative3Bits(ctx, value) == dst.bits:
            return value
        idFloatCast: usize = llvmNative3New(ctx, 3, dst.bits, true, false, 0, 0)
        emitRaw("%t")
        llvmNativeNumber(idFloatCast)
        if llvmNative3Bits(ctx, value) < dst.bits:
            emitRaw(" = fpext ")
        else:
            emitRaw(" = fptrunc ")
        if llvmNative3Bits(ctx, value) == 32:
            emitRaw("float ")
        else:
            emitRaw("double ")
        llvmNative3EmitValue(src, ctx, value)
        emitRaw(" to ")
        if dst.bits == 32:
            emitRaw("float")
        else:
            emitRaw("double")
        emitRaw("\n")
        return idFloatCast
    if llvmNative3Code(ctx, value) == 1 && dst.code == 3:
        idIntFloat: usize = llvmNative3New(ctx, 3, dst.bits, true, false, 0, 0)
        emitRaw("%t")
        llvmNativeNumber(idIntFloat)
        if llvmNative3Signed(ctx, value):
            emitRaw(" = sitofp ")
        else:
            emitRaw(" = uitofp ")
        emitLLVMIntType(llvmNative3Bits(ctx, value))
        emitRaw(" ")
        llvmNative3EmitValue(src, ctx, value)
        emitRaw(" to ")
        if dst.bits == 32:
            emitRaw("float")
        else:
            emitRaw("double")
        emitRaw("\n")
        return idIntFloat
    if llvmNative3Code(ctx, value) == 2 && dst.code == 3:
        tmpBoolFloat: usize = llvmNative3New(ctx, 1, 8, false, false, 0, 0)
        emitRaw("%t")
        llvmNativeNumber(tmpBoolFloat)
        emitRaw(" = zext i1 ")
        llvmNative3EmitValue(src, ctx, value)
        emitRaw(" to i8\n")
        return llvmNative3Cast(src, ctx, tmpBoolFloat, dst)
    if llvmNative3Code(ctx, value) == 3 && dst.code == 1:
        idFloatInt: usize = llvmNative3New(ctx, 1, dst.bits, dst.isSigned, false, 0, 0)
        emitRaw("%t")
        llvmNativeNumber(idFloatInt)
        if dst.isSigned:
            emitRaw(" = fptosi ")
        else:
            emitRaw(" = fptoui ")
        if llvmNative3Bits(ctx, value) == 32:
            emitRaw("float ")
        else:
            emitRaw("double ")
        llvmNative3EmitValue(src, ctx, value)
        emitRaw(" to ")
        emitLLVMIntType(dst.bits)
        emitRaw("\n")
        return idFloatInt
    if llvmNative3Code(ctx, value) == 17 && dst.code == 1:
        idEnumCast: usize = llvmNative3New(ctx, 1, dst.bits, dst.isSigned, false, 0, 0)
        emitRaw("%t")
        llvmNativeNumber(idEnumCast)
        if dst.bits < 64:
            emitRaw(" = trunc i64 ")
            llvmNative3EmitValue(src, ctx, value)
            emitRaw(" to ")
            emitLLVMIntType(dst.bits)
        else:
            emitRaw(" = add i64 0, ")
            llvmNative3EmitValue(src, ctx, value)
        emitRaw("\n")
        return idEnumCast
    if llvmNative3Code(ctx, value) == 1 && dst.code == 17:
        idEnumCast2: usize = llvmNative3New(ctx, 17, 64, true, false, 0, 0)
        emitRaw("%t")
        llvmNativeNumber(idEnumCast2)
        if llvmNative3Bits(ctx, value) < 64:
            if llvmNative3Signed(ctx, value):
                emitRaw(" = sext ")
            else:
                emitRaw(" = zext ")
            emitLLVMIntType(llvmNative3Bits(ctx, value))
            emitRaw(" ")
            llvmNative3EmitValue(src, ctx, value)
            emitRaw(" to i64\n")
        else:
            emitRaw(" = add i64 0, ")
            llvmNative3EmitValue(src, ctx, value)
            emitRaw("\n")
        return idEnumCast2
    if llvmNative3Code(ctx, value) == 1 && dst.code == 1:
        id: usize = llvmNative3New(ctx, 1, dst.bits, dst.isSigned, false, 0, 0)
        emitRaw("%t")
        llvmNativeNumber(id)
        emitRaw(" = ")
        if dst.bits < llvmNative3Bits(ctx, value):
            emitRaw("trunc ")
        else if llvmNative3Signed(ctx, value):
            emitRaw("sext ")
        else:
            emitRaw("zext ")
        emitLLVMIntType(llvmNative3Bits(ctx, value))
        emitRaw(" ")
        llvmNative3EmitValue(src, ctx, value)
        emitRaw(" to ")
        emitLLVMIntType(dst.bits)
        emitRaw("\n")
        return id
    if llvmNative3Code(ctx, value) == 2 && dst.code == 1:
        id2: usize = llvmNative3New(ctx, 1, dst.bits, dst.isSigned, false, 0, 0)
        emitRaw("%t")
        llvmNativeNumber(id2)
        emitRaw(" = zext i1 ")
        llvmNative3EmitValue(src, ctx, value)
        emitRaw(" to ")
        emitLLVMIntType(dst.bits)
        emitRaw("\n")
        return id2
    if llvmNative3Code(ctx, value) == 1 && dst.code == 2:
        id3: usize = llvmNative3New(ctx, 2, 1, false, false, 0, 0)
        emitRaw("%t")
        llvmNativeNumber(id3)
        emitRaw(" = icmp ne ")
        emitLLVMIntType(llvmNative3Bits(ctx, value))
        emitRaw(" ")
        llvmNative3EmitValue(src, ctx, value)
        emitRaw(", 0\n")
        return id3
    return 0

llvmNative3ExprAsType(src: &[]u8, start: usize, end: usize, expectedCode: i32, expectedBits: i32, ctx: &mut LLVMNativeContext, bindings: &mut []LLVMBinding, funcs: &[]LLVMFunctionSig, params: &[]LLVMParamInfo, structs: &[]LLVMStructInfo, fields: &[]LLVMStructFieldInfo, arrays: &mut []LLVMArrayInfo, fixedArrays: &mut []LLVMFixedArrayInfo) usize:
    s: usize = trimStart(src, start, end)
    t: usize = trimEnd(src, s, end)
    if expectedCode == 15 || expectedCode == 16:
        sliceOpenExpected: usize = llvmFindTrailingIndex(src, s, t)
        if sliceOpenExpected > 0 && llvmFindSliceColon(src, sliceOpenExpected, t) > 0:
            return llvmNative3EmitSlice(src, s, t, expectedCode, expectedBits, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
    if expectedCode == 5 && t > s + 1 && src[s] == 91 && src[t - 1] == 93:
        aMeta: &LLVMArrayInfo = &arrays[expectedBits as usize]
        return llvmNative3EmitDynamicArrayLiteral(src, s, t, 0, 0, aMeta.elemCode, aMeta.elemBits, aMeta.elemSigned, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
    if expectedCode == 13:
        existingOptional: usize = llvmNative3Expr(src, s, t, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
        if existingOptional != 0 && llvmNative3Code(ctx, existingOptional) == 13 && llvmNative3Bits(ctx, existingOptional) == expectedBits:
            return existingOptional
        optCode: i32 = fixedArrays[expectedBits as usize].elemCode
        optBits: i32 = fixedArrays[expectedBits as usize].elemBits
        if expressionLooksNone(src, s, t):
            resultNone: usize = llvmNative3New(ctx, 13, expectedBits, false, false, 0, 0)
            emitRaw("%t")
            llvmNativeNumber(resultNone)
            emitRaw(" = insertvalue ")
            llvmEmitAllocaType(13, expectedBits, src, structs, arrays, fixedArrays)
            emitRaw(" zeroinitializer, i1 0, 0\n")
            return resultNone
        payloadValue: usize = llvmNative3ExprAsType(src, s, t, optCode, optBits, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
        if payloadValue == 0 || llvmNative3Code(ctx, payloadValue) != optCode:
            return 0
        if optCode == 1 && !llvmNative3IsImmediate(ctx, payloadValue) && llvmNative3Bits(ctx, payloadValue) != optBits:
            return 0
        if optCode == 4 || optCode == 5 || optCode == 11 || optCode == 13 || optCode == 14 || optCode == 18:
            if llvmNative3Bits(ctx, payloadValue) != optBits && optCode != 18:
                return 0
        if llvmNative3TypeOwnsResources(optCode, optBits, structs, fields, arrays, fixedArrays):
            callOpenOptionalMove: usize = llvmNativeFindCallOpen(src, s, t)
            if callOpenOptionalMove > s && t > callOpenOptionalMove && src[t - 1] == 41:
                payloadMoveS: usize = trimStart(src, callOpenOptionalMove + 1, t - 1)
                payloadMoveE: usize = trimEnd(src, payloadMoveS, t - 1)
                if !llvmBindingMarkMoved(src, bindings, payloadMoveS, payloadMoveE):
                    return 0
        tagValue: usize = llvmNative3New(ctx, 13, expectedBits, false, false, 0, 0)
        emitRaw("%t")
        llvmNativeNumber(tagValue)
        emitRaw(" = insertvalue ")
        llvmEmitAllocaType(13, expectedBits, src, structs, arrays, fixedArrays)
        emitRaw(" zeroinitializer, i1 1, 0\n")
        resultSome: usize = llvmNative3New(ctx, 13, expectedBits, false, false, 0, 0)
        emitRaw("%t")
        llvmNativeNumber(resultSome)
        emitRaw(" = insertvalue ")
        llvmEmitAllocaType(13, expectedBits, src, structs, arrays, fixedArrays)
        emitRaw(" %t")
        llvmNativeNumber(tagValue)
        emitRaw(", ")
        llvmEmitAllocaType(optCode, optBits, src, structs, arrays, fixedArrays)
        emitRaw(" ")
        llvmNative3EmitValue(src, ctx, payloadValue)
        emitRaw(", 1\n")
        return resultSome
    if expectedCode == 14:
        existingResult: usize = llvmNative3Expr(src, s, t, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
        if existingResult != 0 && llvmNative3Code(ctx, existingResult) == 14 && llvmNative3Bits(ctx, existingResult) == expectedBits:
            return existingResult
        okCode: i32 = fixedArrays[expectedBits as usize].elemCode
        okBits: i32 = fixedArrays[expectedBits as usize].elemBits
        errCode: i32 = fixedArrays[expectedBits as usize + 1].elemCode
        errBits: i32 = fixedArrays[expectedBits as usize + 1].elemBits
        callOpenResult: usize = llvmNativeFindCallOpen(src, s, t)
        if callOpenResult > s && t > callOpenResult && src[t - 1] == 41 && llvmNativeMatchParen(src, callOpenResult, t) == t - 1:
            calleeStartResult: usize = s
            calleeEndResult: usize = trimEnd(src, s, callOpenResult)
            ctorOk: bool = calleeEndResult - calleeStartResult == 2 && src[s] == 79 && src[s + 1] == 107
            ctorErr: bool = calleeEndResult - calleeStartResult == 3 && src[s] == 69 && src[s + 1] == 114 && src[s + 2] == 114
            if ctorOk || ctorErr:
                valueS: usize = trimStart(src, callOpenResult + 1, t - 1)
                valueT: usize = trimEnd(src, valueS, t - 1)
                if valueS >= valueT:
                    return 0
                payloadCodeResult: i32 = okCode
                payloadBitsResult: i32 = okBits
                if ctorErr:
                    payloadCodeResult = errCode
                    payloadBitsResult = errBits
                payloadValueResult: usize = llvmNative3ExprAsType(src, valueS, valueT, payloadCodeResult, payloadBitsResult, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
                if payloadValueResult == 0 || llvmNative3Code(ctx, payloadValueResult) != payloadCodeResult:
                    return 0
                if payloadCodeResult == 1 && !llvmNative3IsImmediate(ctx, payloadValueResult) && llvmNative3Bits(ctx, payloadValueResult) != payloadBitsResult:
                    return 0
                if payloadCodeResult == 4 || payloadCodeResult == 5 || payloadCodeResult == 11 || payloadCodeResult == 13 || payloadCodeResult == 14 || payloadCodeResult == 18:
                    if llvmNative3Bits(ctx, payloadValueResult) != payloadBitsResult && payloadCodeResult != 18:
                        return 0
                if llvmNative3TypeOwnsResources(payloadCodeResult, payloadBitsResult, structs, fields, arrays, fixedArrays):
                    payloadMoveSResult: usize = trimStart(src, callOpenResult + 1, t - 1)
                    payloadMoveEResult: usize = trimEnd(src, payloadMoveSResult, t - 1)
                    if !llvmBindingMarkMoved(src, bindings, payloadMoveSResult, payloadMoveEResult):
                        return 0
                tagValueResult: usize = llvmNative3New(ctx, 14, expectedBits, false, false, 0, 0)
                emitRaw("%t")
                llvmNativeNumber(tagValueResult)
                emitRaw(" = insertvalue ")
                llvmEmitAllocaType(14, expectedBits, src, structs, arrays, fixedArrays)
                emitRaw(" zeroinitializer, i1 ")
                if ctorOk:
                    emitRaw("1")
                else:
                    emitRaw("0")
                emitRaw(", 0\n")
                resultCtor: usize = llvmNative3New(ctx, 14, expectedBits, false, false, 0, 0)
                emitRaw("%t")
                llvmNativeNumber(resultCtor)
                emitRaw(" = insertvalue ")
                llvmEmitAllocaType(14, expectedBits, src, structs, arrays, fixedArrays)
                emitRaw(" %t")
                llvmNativeNumber(tagValueResult)
                emitRaw(", ")
                llvmEmitAllocaType(payloadCodeResult, payloadBitsResult, src, structs, arrays, fixedArrays)
                emitRaw(" ")
                llvmNative3EmitValue(src, ctx, payloadValueResult)
                emitRaw(", ")
                if ctorOk:
                    emitRaw("1")
                else:
                    emitRaw("2")
                emitRaw("\n")
                return resultCtor
        return llvmNative3Expr(src, start, end, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
    if expectedCode == 11 && t > s + 1 && src[s] == 91 && src[t - 1] == 93:
        fa: &LLVMFixedArrayInfo = &fixedArrays[expectedBits as usize]
        values: []usize = []usize
        p: usize = s + 1
        count: usize = 0
        while p < t - 1:
            q: usize = p
            depth: usize = 0
            while q < t - 1:
                if src[q] == 40 || src[q] == 91:
                    depth = depth + 1
                else if src[q] == 41 || src[q] == 93:
                    if depth > 0:
                        depth = depth - 1
                else if src[q] == 44 && depth == 0:
                    break
                q = q + 1
            es: usize = trimStart(src, p, q)
            ee: usize = trimEnd(src, es, q)
            if es >= ee:
                return 0
            v: usize = llvmNative3ExprAsType(src, es, ee, fa.elemCode, fa.elemBits, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
            if v == 0 || llvmNative3Code(ctx, v) != fa.elemCode:
                return 0
            if fa.elemCode == 1 && !llvmNative3IsImmediate(ctx, v) && llvmNative3Bits(ctx, v) != fa.elemBits:
                return 0
            if fa.elemCode == 4 && llvmNative3Bits(ctx, v) != fa.elemBits:
                return 0
            values.push(v)
            count = count + 1
            p = q + 1
        if count != fa.count:
            return 0
        addr: usize = ctx.nextLabel
        ctx.nextLabel = ctx.nextLabel + 1
        emitRaw("  %fixedlit.addr")
        llvmNativeNumber(addr)
        emitRaw(" = alloca ")
        llvmEmitAllocaType(expectedCode, expectedBits, src, structs, arrays, fixedArrays)
        emitRaw("\n")
        i: usize = 0
        while i < fa.count:
            elemAddr: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            emitRaw("  %fixedlit.elem")
            llvmNativeNumber(elemAddr)
            emitRaw(" = getelementptr inbounds ")
            llvmEmitAllocaType(expectedCode, expectedBits, src, structs, arrays, fixedArrays)
            emitRaw(", ptr %fixedlit.addr")
            llvmNativeNumber(addr)
            emitRaw(", i64 0, i64 ")
            llvmNativeNumber(i)
            emitRaw("\n  store ")
            llvmEmitAllocaType(fa.elemCode, fa.elemBits, src, structs, arrays, fixedArrays)
            emitRaw(" ")
            llvmNative3EmitValue(src, ctx, values[i])
            emitRaw(", ptr %fixedlit.elem")
            llvmNativeNumber(elemAddr)
            emitRaw("\n")
            i = i + 1
        result: usize = llvmNative3New(ctx, 11, expectedBits, false, false, 0, 0)
        emitRaw("%t")
        llvmNativeNumber(result)
        emitRaw(" = load ")
        llvmEmitAllocaType(expectedCode, expectedBits, src, structs, arrays, fixedArrays)
        emitRaw(", ptr %fixedlit.addr")
        llvmNativeNumber(addr)
        emitRaw("\n")
        return result
    return llvmNative3Expr(src, start, end, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)

llvmNative3EmitSlice(src: &[]u8, start: usize, end: usize, desiredCode: i32, desiredBits: i32, ctx: &mut LLVMNativeContext, bindings: &mut []LLVMBinding, funcs: &[]LLVMFunctionSig, params: &[]LLVMParamInfo, structs: &[]LLVMStructInfo, fields: &[]LLVMStructFieldInfo, arrays: &mut []LLVMArrayInfo, fixedArrays: &mut []LLVMFixedArrayInfo) usize:
    s: usize = trimStart(src, start, end)
    t: usize = trimEnd(src, s, end)
    open: usize = llvmFindTrailingIndex(src, s, t)
    if open == 0:
        return 0
    colon: usize = llvmFindSliceColon(src, open, t)
    if colon == 0:
        return 0
    base: usize = llvmNative3Expr(src, s, open, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
    if base == 0:
        return 0
    baseCode: i32 = llvmNative3Code(ctx, base)
    resultCode: i32 = desiredCode
    if resultCode == 0:
        if baseCode == 16:
            resultCode = 16
        else:
            resultCode = 15
    if baseCode != 5 && baseCode != 11 && baseCode != 15 && baseCode != 16:
        return 0
    elemCode: i32 = 0
    elemBits: i32 = 0
    elemSigned: bool = false
    if baseCode == 5:
        arrMeta: &LLVMArrayInfo = &arrays[llvmNative3Bits(ctx, base) as usize]
        elemCode = arrMeta.elemCode
        elemBits = arrMeta.elemBits
        elemSigned = arrMeta.elemSigned
    else if baseCode == 11:
        fixedMeta: &LLVMFixedArrayInfo = &fixedArrays[llvmNative3Bits(ctx, base) as usize]
        elemCode = fixedMeta.elemCode
        elemBits = fixedMeta.elemBits
        elemSigned = fixedMeta.elemSigned
    else if baseCode == 15 || baseCode == 16:
        viewMeta: &LLVMArrayInfo = &arrays[llvmNative3Bits(ctx, base) as usize]
        elemCode = viewMeta.elemCode
        elemBits = viewMeta.elemBits
        elemSigned = viewMeta.elemSigned
    else:
        return 0
    viewBits: i32 = desiredBits
    reuseViewBits: bool = desiredCode != 0
    if reuseViewBits:
        viewIndex: usize = viewBits as usize
        if viewIndex >= arrays.len():
            reuseViewBits = false
        else if arrays[viewIndex].elemCode != elemCode || arrays[viewIndex].elemBits != elemBits || arrays[viewIndex].elemSigned != elemSigned:
            reuseViewBits = false
    if !reuseViewBits:
        viewBits = llvmInternArray(arrays, elemCode, elemBits, elemSigned) as i32
    lowValue: usize = 0
    highValue: usize = 0
    lowStart: usize = trimStart(src, open + 1, colon)
    lowEnd: usize = trimEnd(src, lowStart, colon)
    highStart: usize = trimStart(src, colon + 1, t - 1)
    highEnd: usize = trimEnd(src, highStart, t - 1)
    limitId: usize = ctx.nextLabel
    ctx.nextLabel = ctx.nextLabel + 1
    if baseCode == 11:
        emitRaw("  %slice.limit")
        llvmNativeNumber(limitId)
        emitRaw(" = add i64 0, ")
        llvmNativeNumber(fixedArrays[llvmNative3Bits(ctx, base) as usize].count)
        emitRaw("\n")
    else if baseCode == 5:
        emitRaw("  %slice.limit")
        llvmNativeNumber(limitId)
        emitRaw(" = extractvalue %LannerDynArray ")
        llvmNative3EmitValue(src, ctx, base)
        emitRaw(", 1\n")
    else:
        emitRaw("  %slice.limit")
        llvmNativeNumber(limitId)
        emitRaw(" = extractvalue { ptr, i64 } ")
        llvmNative3EmitValue(src, ctx, base)
        emitRaw(", 1\n")
    if lowStart < lowEnd:
        lowValue = llvmNative3Expr(src, lowStart, lowEnd, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
        if lowValue == 0 || llvmNative3Code(ctx, lowValue) != 1:
            return 0
    else:
        lowValue = llvmNative3New(ctx, 1, 64, false, false, 0, 0)
        emitRaw("  %t")
        llvmNativeNumber(lowValue)
        emitRaw(" = add i64 0, 0\n")
    if highStart < highEnd:
        highValue = llvmNative3Expr(src, highStart, highEnd, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
        if highValue == 0 || llvmNative3Code(ctx, highValue) != 1:
            return 0
    else:
        highValue = llvmNative3New(ctx, 1, 64, false, false, 0, 0)
        emitRaw("%t")
        llvmNativeNumber(highValue)
        emitRaw(" = add i64 0, %slice.limit")
        llvmNativeNumber(limitId)
        emitRaw("\n")
    orderedId: usize = ctx.nextLabel
    ctx.nextLabel = ctx.nextLabel + 1
    emitRaw("  %slice.ordered")
    llvmNativeNumber(orderedId)
    emitRaw(" = icmp ule i64 ")
    llvmNative3EmitValue(src, ctx, lowValue)
    emitRaw(", ")
    llvmNative3EmitValue(src, ctx, highValue)
    emitRaw("\n")
    insideId: usize = ctx.nextLabel
    ctx.nextLabel = ctx.nextLabel + 1
    emitRaw("  %slice.inside")
    llvmNativeNumber(insideId)
    emitRaw(" = icmp ule i64 ")
    llvmNative3EmitValue(src, ctx, highValue)
    emitRaw(", %slice.limit")
    llvmNativeNumber(limitId)
    emitRaw("\n")
    validId: usize = ctx.nextLabel
    ctx.nextLabel = ctx.nextLabel + 1
    emitRaw("  %slice.valid")
    llvmNativeNumber(validId)
    emitRaw(" = and i1 %slice.ordered")
    llvmNativeNumber(orderedId)
    emitRaw(", %slice.inside")
    llvmNativeNumber(insideId)
    emitRaw("\n")
    okLabel: usize = ctx.nextLabel
    ctx.nextLabel = ctx.nextLabel + 1
    badLabel: usize = ctx.nextLabel
    ctx.nextLabel = ctx.nextLabel + 1
    emitRaw("  br i1 %slice.valid")
    llvmNativeNumber(validId)
    emitRaw(", label ")
    llvmNativeEmitLabelRef("slice.ok", okLabel)
    emitRaw(", label ")
    llvmNativeEmitLabelRef("slice.bad", badLabel)
    emitRaw("\n")
    llvmNativeEmitLabelDef("slice.bad", badLabel)
    emitRaw("  call void @llvm.trap()\n  unreachable\n")
    llvmNativeEmitLabelDef("slice.ok", okLabel)
    pointerId: usize = ctx.nextLabel
    ctx.nextLabel = ctx.nextLabel + 1
    if baseCode == 11:
        baseAddrCode: []i64 = []i64
        baseAddrBits: []i64 = []i64
        baseAddrSigned: []i64 = []i64
        addr: usize = llvmNative3LValueAddress(src, s, open, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays, &mut baseAddrCode, &mut baseAddrBits, &mut baseAddrSigned)
        if addr == 0:
            return 0
        emitRaw("  %slice.ptr")
        llvmNativeNumber(pointerId)
        emitRaw(" = getelementptr inbounds ")
        llvmEmitAllocaType(11, llvmNative3Bits(ctx, base) as i32, src, structs, arrays, fixedArrays)
        emitRaw(", ptr ")
        llvmNative3EmitValue(src, ctx, addr)
        emitRaw(", i64 0, i64 ")
        llvmNative3EmitValue(src, ctx, lowValue)
    else:
        basePtrId: usize = ctx.nextLabel
        ctx.nextLabel = ctx.nextLabel + 1
        emitRaw("  %slice.base")
        llvmNativeNumber(basePtrId)
        if baseCode == 5:
            emitRaw(" = extractvalue %LannerDynArray ")
        else:
            emitRaw(" = extractvalue { ptr, i64 } ")
        llvmNative3EmitValue(src, ctx, base)
        emitRaw(", 0\n")
        emitRaw("  %slice.ptr")
        llvmNativeNumber(pointerId)
        emitRaw(" = getelementptr inbounds ")
        llvmEmitAllocaType(elemCode, elemBits, src, structs, arrays, fixedArrays)
        emitRaw(", ptr %slice.base")
        llvmNativeNumber(basePtrId)
        emitRaw(", i64 ")
        llvmNative3EmitValue(src, ctx, lowValue)
    emitRaw("\n")
    lengthId: usize = ctx.nextLabel
    ctx.nextLabel = ctx.nextLabel + 1
    emitRaw("  %slice.len")
    llvmNativeNumber(lengthId)
    emitRaw(" = sub i64 ")
    llvmNative3EmitValue(src, ctx, highValue)
    emitRaw(", ")
    llvmNative3EmitValue(src, ctx, lowValue)
    emitRaw("\n")
    v0: usize = llvmNative3New(ctx, resultCode, viewBits, false, false, 0, 0)
    emitRaw("%t")
    llvmNativeNumber(v0)
    emitRaw(" = insertvalue { ptr, i64 } zeroinitializer, ptr %slice.ptr")
    llvmNativeNumber(pointerId)
    emitRaw(", 0\n")
    v1: usize = llvmNative3New(ctx, resultCode, viewBits, false, false, 0, 0)
    emitRaw("%t")
    llvmNativeNumber(v1)
    emitRaw(" = insertvalue { ptr, i64 } %t")
    llvmNativeNumber(v0)
    emitRaw(", i64 %slice.len")
    llvmNativeNumber(lengthId)
    emitRaw(", 1\n")
    return v1

llvmNative3Expr(src: &[]u8, start: usize, end: usize, ctx: &mut LLVMNativeContext, bindings: &mut []LLVMBinding, funcs: &[]LLVMFunctionSig, params: &[]LLVMParamInfo, structs: &[]LLVMStructInfo, fields: &[]LLVMStructFieldInfo, arrays: &mut []LLVMArrayInfo, fixedArrays: &mut []LLVMFixedArrayInfo) usize:
    s: usize = trimStart(src, start, end)
    t: usize = trimEnd(src, s, end)
    stripped: usize = llvmStripParens(src, s, t)
    if stripped > 0:
        return llvmNative3Expr(src, stripped, t - 1, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
    # Arena allocation is restricted by the typechecker to array literals.
    # Never retag an already-owned heap array, because that would alias the
    # same storage under two owners and make destruction unsafe.
    arenaIn: usize = 0
    scanIn: usize = s
    depthIn: usize = 0
    while scanIn + 3 <= t:
        if src[scanIn] == 40 || src[scanIn] == 91:
            depthIn = depthIn + 1
        else if src[scanIn] == 41 || src[scanIn] == 93:
            if depthIn > 0:
                depthIn = depthIn - 1
        else if src[scanIn] == 105 && scanIn + 1 < t && src[scanIn + 1] == 110 && depthIn == 0:
            leftGap: bool = scanIn == s || isSpace(src[scanIn - 1])
            rightGap: bool = scanIn + 2 >= t || isSpace(src[scanIn + 2])
            if leftGap && rightGap:
                arenaIn = scanIn
                break
        scanIn = scanIn + 1
    if arenaIn > s:
        arenaS: usize = trimStart(src, arenaIn + 2, t)
        arenaT: usize = trimEnd(src, arenaS, t)
        if arenaS >= arenaT:
            return 0
        arenaBindOnly: i64 = llvmLookupBinding(src, bindings, arenaS, arenaT)
        if arenaBindOnly < 0 || bindings[arenaBindOnly as usize].typeCode != 18 || bindings[arenaBindOnly as usize].moved:
            return 0
        return llvmNative3EmitDynamicArrayLiteral(src, s, arenaIn, arenaS, arenaT, 0, 0, false, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
    # Non-empty bracket literals are dynamic arrays unless the expected-type path
    # has already claimed them as fixed arrays. Keep one lowering path so heap
    # literals and arena literals share identical element initialization.
    if t > s + 1 && src[s] == 91 && src[t - 1] == 93:
        literalArray: usize = llvmNative3EmitDynamicArrayLiteral(src, s, t, 0, 0, 0, 0, false, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
        if literalArray != 0:
            return literalArray
    if t > s && src[s] == 38:
        refStart: usize = s + 1
        if refStart + 3 <= t && src[refStart] == 109 && src[refStart + 1] == 117 && src[refStart + 2] == 116:
            refStart = refStart + 3
        while refStart < t && isSpace(src[refStart]):
            refStart = refStart + 1
        if refStart >= t:
            return 0
        refCodeOut: []i64 = []i64
        refBitsOut: []i64 = []i64
        refSignedOut: []i64 = []i64
        refAddr: usize = llvmNative3LValueAddress(src, refStart, t, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays, &mut refCodeOut, &mut refBitsOut, &mut refSignedOut)
        if refAddr == 0 || refCodeOut.len() == 0:
            return 0
        if refCodeOut[0] != 4 && refCodeOut[0] != 5 && refCodeOut[0] != 11:
            return 0
        # LValueAddress already emitted the pointer-producing instruction.
        # code 8 is its internal raw-address SSA kind; reference declarations
        # consume the same SSA pointer and store it into their `ptr` slot.
        return refAddr
    litOpen: usize = llvmFindStructLitOpen(src, s, t)
    if litOpen > 0:
        structIdx: i64 = llvmLookupStruct(src, structs, s, litOpen)
        if structIdx < 0:
            return 0
        st: &LLVMStructInfo = &structs[structIdx as usize]
        # Track which declared fields were actually supplied, by index, so a
        # literal is only accepted once every field has a value -- same
        # completeness rule the real typechecker enforces for struct literals.
        suppliedCount: usize = 0
        fieldValues: []usize = []usize
        fieldSupplied: []bool = []bool
        fv: usize = 0
        while fv < st.fieldCount:
            fieldValues.push(0)
            fieldSupplied.push(false)
            fv = fv + 1
        p2: usize = litOpen + 1
        while p2 < t - 1:
            q2: usize = p2
            depth3: usize = 0
            while q2 < t - 1:
                if src[q2] == 40 || src[q2] == 91:
                    depth3 = depth3 + 1
                else if src[q2] == 41 || src[q2] == 93:
                    if depth3 > 0:
                        depth3 = depth3 - 1
                else if src[q2] == 44 && depth3 == 0:
                    break
                q2 = q2 + 1
            fs: usize = trimStart(src, p2, q2)
            fe: usize = trimEnd(src, fs, q2)
            if fs < fe:
                fcolon: usize = fs
                while fcolon < fe && src[fcolon] != 58:
                    fcolon = fcolon + 1
                if fcolon >= fe:
                    return 0
                fNameStart: usize = trimStart(src, fs, fcolon)
                fNameEnd: usize = trimEnd(src, fNameStart, fcolon)
                fIdx: i64 = llvmStructFieldIndex(src, fields, st.fieldStart, st.fieldCount, fNameStart, fNameEnd)
                if fIdx < 0 || fieldSupplied[fIdx as usize]:
                    return 0
                fInfo: &LLVMStructFieldInfo = &fields[st.fieldStart + (fIdx as usize)]
                fValStart: usize = trimStart(src, fcolon + 1, q2)
                fVal: usize = llvmNative3ExprAsType(src, fValStart, q2, fInfo.typeCode, fInfo.bits, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
                if fVal == 0 || llvmNative3Code(ctx, fVal) != fInfo.typeCode:
                    return 0
                if (fInfo.typeCode == 4 || fInfo.typeCode == 5 || fInfo.typeCode == 11 || fInfo.typeCode == 13 || fInfo.typeCode == 14) && llvmNative3Bits(ctx, fVal) != fInfo.bits:
                    return 0
                fieldValues[fIdx as usize] = fVal
                if llvmNative3TypeOwnsResources(fInfo.typeCode, fInfo.bits, structs, fields, arrays, fixedArrays):
                    if !llvmBindingMarkMoved(src, bindings, fValStart, q2):
                        return 0
                fieldSupplied[fIdx as usize] = true
                suppliedCount = suppliedCount + 1
            p2 = q2 + 1
        if suppliedCount != st.fieldCount:
            return 0
        addr: usize = ctx.nextLabel
        ctx.nextLabel = ctx.nextLabel + 1
        emitRaw("  %structlit.addr")
        llvmNativeNumber(addr)
        emitRaw(" = alloca %")
        emitRawRange(src, st.nameStart, st.nameEnd)
        emitRaw("\n")
        fw: usize = 0
        while fw < st.fieldCount:
            fInfo2: &LLVMStructFieldInfo = &fields[st.fieldStart + fw]
            faddr: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            emitRaw("  %structlit.field")
            llvmNativeNumber(faddr)
            emitRaw(" = getelementptr inbounds %")
            emitRawRange(src, st.nameStart, st.nameEnd)
            emitRaw(", ptr %structlit.addr")
            llvmNativeNumber(addr)
            emitRaw(", i32 0, i32 ")
            llvmNativeNumber(fw)
            emitRaw("\n  store ")
            llvmEmitAllocaType(fInfo2.typeCode, fInfo2.bits, src, structs, arrays, fixedArrays)
            emitRaw(" ")
            llvmNative3EmitValue(src, ctx, fieldValues[fw])
            emitRaw(", ptr %structlit.field")
            llvmNativeNumber(faddr)
            emitRaw("\n")
            fw = fw + 1
        resultId0: usize = llvmNative3New(ctx, 4, structIdx as i32, false, false, 0, 0)
        emitRaw("%t")
        llvmNativeNumber(resultId0)
        emitRaw(" = load %")
        emitRawRange(src, st.nameStart, st.nameEnd)
        emitRaw(", ptr %structlit.addr")
        llvmNativeNumber(addr)
        emitRaw("\n")
        return resultId0
    if t - s >= 2 && src[s] == 91 && src[s + 1] == 93:
        elemInfo2: LLVMTypeInfo = llvmResolveType(src, s + 2, t, structs, arrays, fixedArrays)
        if elemInfo2.code != 1 && elemInfo2.code != 2 && elemInfo2.code != 3 && elemInfo2.code != 4 && elemInfo2.code != 17:
            return 0
        arrIdxLit: usize = llvmInternArray(arrays, elemInfo2.code, elemInfo2.bits, elemInfo2.isSigned)
        litAddr: usize = ctx.nextLabel
        ctx.nextLabel = ctx.nextLabel + 1
        emitRaw("  %arraylit.addr")
        llvmNativeNumber(litAddr)
        emitRaw(" = alloca %LannerDynArray\n  store %LannerDynArray { ptr null, i64 0, i64 0, ptr null }, ptr %arraylit.addr")
        llvmNativeNumber(litAddr)
        emitRaw("\n")
        resultEmpty: usize = llvmNative3New(ctx, 5, arrIdxLit as i32, false, false, 0, 0)
        emitRaw("%t")
        llvmNativeNumber(resultEmpty)
        emitRaw(" = load %LannerDynArray, ptr %arraylit.addr")
        llvmNativeNumber(litAddr)
        emitRaw("\n")
        return resultEmpty
    sliceOpen: usize = llvmFindTrailingIndex(src, s, t)
    if sliceOpen > 0 && llvmFindSliceColon(src, sliceOpen, t) > 0:
        return llvmNative3EmitSlice(src, s, t, 0, 0, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
    if t > s && src[s] >= 48 && src[s] <= 57:
        i: usize = s
        sawDot: bool = false
        sawExp: bool = false
        validLiteral: bool = true
        while i < t:
            c: u8 = src[i]
            if c >= 48 && c <= 57:
                i = i + 1
                continue
            if c == 46 && !sawDot && !sawExp:
                sawDot = true
                i = i + 1
                continue
            if (c == 101 || c == 69) && !sawExp:
                sawExp = true
                i = i + 1
                if i < t && (src[i] == 43 || src[i] == 45):
                    i = i + 1
                continue
            validLiteral = false
            break
        if validLiteral && i == t:
            if sawDot || sawExp:
                return llvmNative3New(ctx, 3, 64, true, true, s, t)
            return llvmNative3New(ctx, 1, 32, true, true, s, t)
    # Hex/binary integer literals are immediate integer values. LLVM accepts
    # the decimalized spelling, so parse them here to keep the bootstrap
    # deterministic and independent of LLVM's source literal extensions.
    if t > s + 2 && src[s] == 48 && (src[s + 1] == 120 || src[s + 1] == 88 || src[s + 1] == 98 || src[s + 1] == 66):
        baseLit: i32 = 16
        if src[s + 1] == 98 || src[s + 1] == 66:
            baseLit = 2
        litPos: usize = s + 2
        litValue: usize = 0
        litValid: bool = litPos < t
        while litPos < t:
            digit: i32 = -1
            if src[litPos] >= 48 && src[litPos] <= 57:
                digit = (src[litPos] - 48) as i32
            else if src[litPos] >= 65 && src[litPos] <= 70:
                digit = (src[litPos] - 55) as i32
            else if src[litPos] >= 97 && src[litPos] <= 102:
                digit = (src[litPos] - 87) as i32
            if digit < 0 || digit >= baseLit:
                litValid = false
                break
            litValue = litValue * (baseLit as usize) + (digit as usize)
            litPos = litPos + 1
        if litValid:
            hexValue: usize = llvmNative3New(ctx, 1, 64, false, false, 0, 0)
            emitRaw("%t")
            llvmNativeNumber(hexValue)
            emitRaw(" = add i64 0, ")
            llvmNativeNumber(litValue)
            emitRaw("\n")
            return hexValue
    if t - s == 4 && src[s] == 116 && src[s + 1] == 114 && src[s + 2] == 117 && src[s + 3] == 101:
        return llvmNative3New(ctx, 2, 1, false, true, s, t)
    if t - s == 5 && src[s] == 102 && src[s + 1] == 97 && src[s + 2] == 108 && src[s + 3] == 115 && src[s + 4] == 101:
        return llvmNative3New(ctx, 2, 1, false, true, s, t)
    if t > s + 1 && src[s] == 34 && src[t - 1] == 34:
        stringId: usize = llvmNativeStringId(src, s)
        return llvmNative3New(ctx, 9, 0, false, true, stringId, 0)
    if t > s && (src[s] == 45 || src[s] == 43 || src[s] == 33 || src[s] == 126):
        unary: u8 = src[s]
        value2: usize = llvmNative3Expr(src, s + 1, t, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
        if value2 == 0 || llvmNative3Code(ctx, value2) == 0:
            return 0
        if unary == 43:
            return value2
        if unary == 33:
            if llvmNative3Code(ctx, value2) != 2:
                return 0
            id4: usize = llvmNative3New(ctx, 2, 1, false, false, 0, 0)
            emitRaw("%t")
            llvmNativeNumber(id4)
            emitRaw(" = xor i1 1, ")
            llvmNative3EmitValue(src, ctx, value2)
            emitRaw("\n")
            return id4
        if unary == 126 && llvmNative3Code(ctx, value2) == 1:
            id5: usize = llvmNative3New(ctx, 1, llvmNative3Bits(ctx, value2), llvmNative3Signed(ctx, value2), false, 0, 0)
            emitRaw("%t")
            llvmNativeNumber(id5)
            emitRaw(" = xor ")
            emitLLVMIntType(llvmNative3Bits(ctx, value2))
            emitRaw(" -1, ")
            llvmNative3EmitValue(src, ctx, value2)
            emitRaw("\n")
            return id5
        if unary == 45 && llvmNative3Code(ctx, value2) == 1:
            id6: usize = llvmNative3New(ctx, 1, llvmNative3Bits(ctx, value2), llvmNative3Signed(ctx, value2), false, 0, 0)
            emitRaw("%t")
            llvmNativeNumber(id6)
            emitRaw(" = sub ")
            emitLLVMIntType(llvmNative3Bits(ctx, value2))
            emitRaw(" 0, ")
            llvmNative3EmitValue(src, ctx, value2)
            emitRaw("\n")
            return id6
        return 0
    callOpen: usize = llvmNativeFindCallOpen(src, s, t)
    if callOpen > s && t > callOpen && src[t - 1] == 41 && llvmNativeMatchParen(src, callOpen, t) == t - 1:
        calleeStartBuiltin: usize = s
        calleeEndBuiltin: usize = trimEnd(src, s, callOpen)
        calleeLen: usize = calleeEndBuiltin - calleeStartBuiltin
        # Native bootstrap I/O builtins. These are intentionally handled here
        # rather than added to the function table because they are language
        # intrinsics, not user functions.
        isArenaCreate: bool = calleeLen == 12 && src[s] == 65 && src[s + 1] == 114 && src[s + 2] == 101 && src[s + 3] == 110 && src[s + 4] == 97 && src[s + 5] == 46 && src[s + 6] == 99 && src[s + 7] == 114 && src[s + 8] == 101 && src[s + 9] == 97 && src[s + 10] == 116 && src[s + 11] == 101
        isReadFile: bool = calleeLen == 8 && src[s] == 114 && src[s + 1] == 101 && src[s + 2] == 97 && src[s + 3] == 100 && src[s + 4] == 70 && src[s + 5] == 105 && src[s + 6] == 108 && src[s + 7] == 101
        isGetEnv: bool = calleeLen == 6 && src[s] == 103 && src[s + 1] == 101 && src[s + 2] == 116 && src[s + 3] == 69 && src[s + 4] == 110 && src[s + 5] == 118
        isStringLen: bool = calleeLen == 9 && src[s] == 115 && src[s + 1] == 116 && src[s + 2] == 114 && src[s + 3] == 105 && src[s + 4] == 110 && src[s + 5] == 103 && src[s + 6] == 76 && src[s + 7] == 101 && src[s + 8] == 110
        isWriteRaw: bool = calleeLen == 8 && src[s] == 119 && src[s + 1] == 114 && src[s + 2] == 105 && src[s + 3] == 116 && src[s + 4] == 101 && src[s + 5] == 82 && src[s + 6] == 97 && src[s + 7] == 119
        isWriteByteRaw: bool = calleeLen == 12 && src[s] == 119 && src[s + 1] == 114 && src[s + 2] == 105 && src[s + 3] == 116 && src[s + 4] == 101 && src[s + 5] == 66 && src[s + 6] == 121 && src[s + 7] == 116 && src[s + 8] == 101 && src[s + 9] == 82 && src[s + 10] == 97 && src[s + 11] == 119
        isPrintInt: bool = calleeLen == 8 && src[s] == 112 && src[s + 1] == 114 && src[s + 2] == 105 && src[s + 3] == 110 && src[s + 4] == 116 && src[s + 5] == 73 && src[s + 6] == 110 && src[s + 7] == 116
        if isArenaCreate:
            argAS: usize = trimStart(src, callOpen + 1, t - 1)
            argAT: usize = trimEnd(src, argAS, t - 1)
            sizeValue: usize = llvmNative3Expr(src, argAS, argAT, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
            if sizeValue == 0 || llvmNative3Code(ctx, sizeValue) != 1:
                return 0
            size64: usize = sizeValue
            if llvmNative3Bits(ctx, sizeValue) != 64:
                sizeInfo: LLVMTypeInfo = LLVMTypeInfo[code: 1, bits: 64, isSigned: false]
                size64 = llvmNative3Cast(src, ctx, sizeValue, &sizeInfo)
                if size64 == 0:
                    return 0
            arenaLit: usize = llvmNative3New(ctx, 18, 0, false, false, 0, 0)
            emitRaw("%t")
            llvmNativeNumber(arenaLit)
            emitRaw(" = insertvalue %LannerArena zeroinitializer, ptr null, 0\n")
            arena1: usize = llvmNative3New(ctx, 18, 0, false, false, 0, 0)
            emitRaw("  %t")
            llvmNativeNumber(arena1)
            emitRaw(" = insertvalue %LannerArena %t")
            llvmNativeNumber(arenaLit)
            emitRaw(", ptr null, 1\n")
            arena2: usize = llvmNative3New(ctx, 18, 0, false, false, 0, 0)
            emitRaw("  %t")
            llvmNativeNumber(arena2)
            emitRaw(" = insertvalue %LannerArena %t")
            llvmNativeNumber(arena1)
            emitRaw(", i64 0, 2\n")
            arena3: usize = llvmNative3New(ctx, 18, 0, false, false, 0, 0)
            emitRaw("  %t")
            llvmNativeNumber(arena3)
            emitRaw(" = insertvalue %LannerArena %t")
            llvmNativeNumber(arena2)
            emitRaw(", i64 ")
            llvmNative3EmitValue(src, ctx, size64)
            emitRaw(", 3\n")
            return arena3
        if isReadFile:
            argS: usize = trimStart(src, callOpen + 1, t - 1)
            argT: usize = trimEnd(src, argS, t - 1)
            arg: usize = llvmNative3Expr(src, argS, argT, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
            if arg == 0 || llvmNative3Code(ctx, arg) != 9:
                return 0
            arrayInfo: usize = llvmInternArray(arrays, 1, 8, false)
            out: usize = llvmNative3New(ctx, 5, arrayInfo as i32, false, false, 0, 0)
            emitRaw("%t")
            llvmNativeNumber(out)
            emitRaw(" = call %LannerDynArray @__lanner_read_file(ptr ")
            llvmNative3EmitValue(src, ctx, arg)
            emitRaw(")\n")
            return out
        if isGetEnv:
            argS2: usize = trimStart(src, callOpen + 1, t - 1)
            argT2: usize = trimEnd(src, argS2, t - 1)
            arg2: usize = llvmNative3Expr(src, argS2, argT2, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
            if arg2 == 0 || llvmNative3Code(ctx, arg2) != 9:
                return 0
            out2: usize = llvmNative3New(ctx, 9, 0, false, false, 0, 0)
            emitRaw("%t")
            llvmNativeNumber(out2)
            emitRaw(" = call ptr @getenv(ptr ")
            llvmNative3EmitValue(src, ctx, arg2)
            emitRaw(")\n")
            envNullId: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            emitRaw("%envnull")
            llvmNativeNumber(envNullId)
            emitRaw(" = icmp eq ptr %t")
            llvmNativeNumber(out2)
            emitRaw(", null\n")
            safeEnvId: usize = llvmNative3New(ctx, 9, 0, false, false, 0, 0)
            emitRaw("%t")
            llvmNativeNumber(safeEnvId)
            emitRaw(" = select i1 %envnull")
            llvmNativeNumber(envNullId)
            emitRaw(", ptr getelementptr inbounds ([1 x i8], ptr @.lanner.empty, i64 0, i64 0), ptr %t")
            llvmNativeNumber(out2)
            emitRaw("\n")
            return safeEnvId
        if isStringLen:
            argS3: usize = trimStart(src, callOpen + 1, t - 1)
            argT3: usize = trimEnd(src, argS3, t - 1)
            arg3: usize = llvmNative3Expr(src, argS3, argT3, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
            if arg3 == 0 || llvmNative3Code(ctx, arg3) != 9:
                return 0
            out3: usize = llvmNative3New(ctx, 1, 64, false, false, 0, 0)
            emitRaw("%t")
            llvmNativeNumber(out3)
            emitRaw(" = call i64 @strlen(ptr ")
            llvmNative3EmitValue(src, ctx, arg3)
            emitRaw(")\n")
            return out3
        if isWriteRaw:
            argS4: usize = trimStart(src, callOpen + 1, t - 1)
            argT4: usize = trimEnd(src, argS4, t - 1)
            arg4: usize = llvmNative3Expr(src, argS4, argT4, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
            if arg4 == 0 || llvmNative3Code(ctx, arg4) != 9:
                return 0
            lenTmp: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            outPtr: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            emitRaw("%builtin.len")
            llvmNativeNumber(lenTmp)
            emitRaw(" = call i64 @strlen(ptr ")
            llvmNative3EmitValue(src, ctx, arg4)
            emitRaw(")\n  %builtin.out")
            llvmNativeNumber(outPtr)
            emitRaw(" = load ptr, ptr @stdout\n  call i64 @fwrite(ptr ")
            llvmNative3EmitValue(src, ctx, arg4)
            emitRaw(", i64 1, i64 %builtin.len")
            llvmNativeNumber(lenTmp)
            emitRaw(", ptr %builtin.out")
            llvmNativeNumber(outPtr)
            emitRaw(")\n")
            return llvmNative3New(ctx, 2, 1, false, false, 0, 0)
        if isWriteByteRaw:
            argS5: usize = trimStart(src, callOpen + 1, t - 1)
            argT5: usize = trimEnd(src, argS5, t - 1)
            arg5: usize = llvmNative3Expr(src, argS5, argT5, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
            if arg5 == 0 || llvmNative3Code(ctx, arg5) != 1:
                return 0
            byte64: usize = arg5
            if llvmNative3Bits(ctx, arg5) != 32:
                cast32: LLVMTypeInfo = LLVMTypeInfo[code: 1, bits: 32, isSigned: false]
                byte64 = llvmNative3Cast(src, ctx, arg5, &cast32)
                if byte64 == 0:
                    return 0
            emitRaw("  call i32 @putchar(i32 ")
            llvmNative3EmitValue(src, ctx, byte64)
            emitRaw(")\n")
            return llvmNative3New(ctx, 2, 1, false, false, 0, 0)
        isPrint: bool = calleeLen == 5 && src[s] == 112 && src[s + 1] == 114 && src[s + 2] == 105 && src[s + 3] == 110 && src[s + 4] == 116
        if isPrint:
            argPS: usize = trimStart(src, callOpen + 1, t - 1)
            argPT: usize = trimEnd(src, argPS, t - 1)
            argPV: usize = llvmNative3Expr(src, argPS, argPT, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
            if argPV == 0:
                return 0
            argPC: i32 = llvmNative3Code(ctx, argPV)
            if argPC == 9:
                emitRaw("  call i32 (ptr, ...) @printf(ptr getelementptr inbounds ([3 x i8], ptr @.lanner.print_str, i64 0, i64 0), ptr ")
                llvmNative3EmitValue(src, ctx, argPV)
                emitRaw(")\n")
                return llvmNative3New(ctx, 2, 1, false, false, 0, 0)
            if argPC == 2:
                boolPtr: usize = llvmNative3New(ctx, 9, 0, false, false, 0, 0)
                emitRaw("  %t")
                llvmNativeNumber(boolPtr)
                emitRaw(" = select i1 ")
                llvmNative3EmitValue(src, ctx, argPV)
                emitRaw(", ptr getelementptr inbounds ([5 x i8], ptr @.lanner.true, i64 0, i64 0), ptr getelementptr inbounds ([6 x i8], ptr @.lanner.false, i64 0, i64 0)\n")
                emitRaw("  call i32 (ptr, ...) @printf(ptr getelementptr inbounds ([4 x i8], ptr @.lanner.print_bool, i64 0, i64 0), ptr ")
                emitRaw("%t")
                llvmNativeNumber(boolPtr)
                emitRaw(")\n")
                return llvmNative3New(ctx, 2, 1, false, false, 0, 0)
            if argPC == 1:
                intPrint: usize = argPV
                if llvmNative3Bits(ctx, argPV) != 64:
                    intPrintInfo: LLVMTypeInfo = LLVMTypeInfo[code: 1, bits: 64, isSigned: true]
                    intPrint = llvmNative3Cast(src, ctx, argPV, &intPrintInfo)
                    if intPrint == 0:
                        return 0
                emitRaw("  call i32 (ptr, ...) @printf(ptr @.lanner.print_i64, i64 ")
                llvmNative3EmitValue(src, ctx, intPrint)
                emitRaw(")\n")
                return llvmNative3New(ctx, 2, 1, false, false, 0, 0)
            if argPC == 3:
                floatPrint: usize = argPV
                if llvmNative3Bits(ctx, argPV) == 32:
                    emitRaw("  %print.f64")
                    llvmNativeNumber(floatPrint)
                    emitRaw(" = fpext float ")
                    llvmNative3EmitValue(src, ctx, argPV)
                    emitRaw(" to double\n")
                    # Use the existing value id as the lanner numbering seed
                    # only for the emitted temporary name; create a distinct
                    # IR value record for subsequent uses.
                    floatPrint64: usize = llvmNative3New(ctx, 3, 64, true, false, 0, 0)
                    emitRaw("  %t")
                    llvmNativeNumber(floatPrint64)
                    emitRaw(" = fpext float ")
                    llvmNative3EmitValue(src, ctx, argPV)
                    emitRaw(" to double\n")
                    floatPrint = floatPrint64
                emitRaw("  call i32 (ptr, ...) @printf(ptr @.lanner.print_f64, double ")
                llvmNative3EmitValue(src, ctx, floatPrint)
                emitRaw(")\n")
                return llvmNative3New(ctx, 2, 1, false, false, 0, 0)
        if isPrintInt:
            argS6: usize = trimStart(src, callOpen + 1, t - 1)
            argT6: usize = trimEnd(src, argS6, t - 1)
            arg6: usize = llvmNative3Expr(src, argS6, argT6, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
            if arg6 == 0 || llvmNative3Code(ctx, arg6) != 1:
                return 0
            print32: usize = arg6
            if llvmNative3Bits(ctx, arg6) != 64:
                widened: LLVMTypeInfo = LLVMTypeInfo[code: 1, bits: 64, isSigned: true]
                print32 = llvmNative3Cast(src, ctx, arg6, &widened)
                if print32 == 0:
                    return 0
            emitRaw("  call i32 (ptr, ...) @printf(ptr @.lanner.print_i64, i64 ")
            llvmNative3EmitValue(src, ctx, print32)
            emitRaw(")\n")
            return llvmNative3New(ctx, 2, 1, false, false, 0, 0)
    if callOpen > s && t > callOpen && src[t - 1] == 41 && !hasByte(src, s, callOpen, 46) && llvmNativeMatchParen(src, callOpen, t) == t - 1:
        calleeStart: usize = s
        calleeEnd: usize = trimEnd(src, s, callOpen)
        fnIndex: i64 = llvmLookupFunction(src, funcs, calleeStart, calleeEnd)
        if fnIndex < 0:
            return 0
        sig: &LLVMFunctionSig = &funcs[fnIndex as usize]
        # Evaluate every argument BEFORE printing any of the call line:
        # argument evaluation can itself emit instructions (loading a local,
        # extracting a field, ...), and those must land above the call, not
        # spliced into the middle of it.
        argIds: []usize = []usize
        p: usize = callOpen + 1
        argIndex: usize = 0
        while p < t - 1:
            q: usize = p
            depth2: usize = 0
            while q < t - 1:
                if src[q] == 34:
                    q = llvmNativeSkipString(src, q, t - 1)
                    continue
                if src[q] == 40 || src[q] == 91:
                    depth2 = depth2 + 1
                else if src[q] == 41 || src[q] == 93:
                    if depth2 > 0:
                        depth2 = depth2 - 1
                else if src[q] == 44 && depth2 == 0:
                    break
                q = q + 1
            if argIndex >= sig.paramCount:
                return 0
            paramInfo: &LLVMParamInfo = &params[sig.paramStart + argIndex]
            if paramInfo.typeCode == 6 || paramInfo.typeCode == 7 || paramInfo.typeCode == 12:
                # Reference parameter: pass the argument's address. Accept the
                # explicit `&x` / `&mut x` spelling (or a bare name), and
                # require a plain local so there is a real slot to point at.
                aStart: usize = trimStart(src, p, q)
                aEnd: usize = trimEnd(src, aStart, q)
                explicitRef: bool = aStart < aEnd && src[aStart] == 38
                if explicitRef:
                    refStart: usize = aStart + 1
                    if refStart + 3 <= aEnd && src[refStart] == 109 && src[refStart + 1] == 117 && src[refStart + 2] == 116:
                        refStart = refStart + 3
                    while refStart < aEnd && isSpace(src[refStart]):
                        refStart = refStart + 1
                    refCodeOut: []i64 = []i64
                    refBitsOut: []i64 = []i64
                    refSignedOut: []i64 = []i64
                    refAddr: usize = llvmNative3LValueAddress(src, refStart, aEnd, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays, &mut refCodeOut, &mut refBitsOut, &mut refSignedOut)
                    if refAddr == 0 || refCodeOut.len() == 0:
                        return 0
                    ownedCode: i32 = 5
                    if paramInfo.typeCode == 7:
                        ownedCode = 4
                    else if paramInfo.typeCode == 12:
                        ownedCode = 11
                    if refCodeOut[0] != ownedCode as i64:
                        return 0
                    if refBitsOut[0] != paramInfo.bits as i64:
                        return 0
                    argIds.push(refAddr)
                    argIndex = argIndex + 1
                    p = q + 1
                    continue
                refBind: i64 = llvmLookupBinding(src, bindings, aStart, aEnd)
                if refBind < 0:
                    return 0
                rb: &LLVMBinding = &bindings[refBind as usize]
                ownedCode2: i32 = 5
                if paramInfo.typeCode == 7:
                    ownedCode2 = 4
                else if paramInfo.typeCode == 12:
                    ownedCode2 = 11
                if rb.bits != paramInfo.bits:
                    return 0
                refArg: usize = llvmNative3New(ctx, paramInfo.typeCode, paramInfo.bits, false, false, 0, 0)
                emitRaw("%t")
                llvmNativeNumber(refArg)
                if rb.typeCode == ownedCode2:
                    emitRaw(" = getelementptr inbounds i8, ptr ")
                    emitLLVMSlotName(src, rb.nameStart, rb.nameEnd)
                    emitRaw(", i64 0\n")
                else if rb.typeCode == paramInfo.typeCode:
                    emitRaw(" = load ptr, ptr ")
                    emitLLVMSlotName(src, rb.nameStart, rb.nameEnd)
                    emitRaw("\n")
                else:
                    return 0
                argIds.push(refArg)
                argIndex = argIndex + 1
                p = q + 1
                continue
            arg: usize = llvmNative3ExprAsType(src, p, q, paramInfo.typeCode, paramInfo.bits as i32, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
            if arg == 0 || llvmNative3Code(ctx, arg) == 0 || llvmNative3Code(ctx, arg) != paramInfo.typeCode:
                return 0
            if paramInfo.typeCode == 1 && !llvmNative3IsImmediate(ctx, arg) && llvmNative3Bits(ctx, arg) != paramInfo.bits:
                return 0
            if (paramInfo.typeCode == 4 || paramInfo.typeCode == 5 || paramInfo.typeCode == 11 || paramInfo.typeCode == 13 || paramInfo.typeCode == 14 || paramInfo.typeCode == 18) && llvmNative3Bits(ctx, arg) != paramInfo.bits && paramInfo.typeCode != 18:
                return 0
            if llvmNative3TypeOwnsResources(paramInfo.typeCode, paramInfo.bits, structs, fields, arrays, fixedArrays):
                aSimple: usize = trimStart(src, p, q)
                aSimpleEnd: usize = trimEnd(src, aSimple, q)
                if !llvmBindingMarkMoved(src, bindings, aSimple, aSimpleEnd):
                    return 0
            argIds.push(arg)
            argIndex = argIndex + 1
            p = q + 1
        if argIndex != sig.paramCount:
            return 0
        resultId: usize = llvmNative3New(ctx, sig.returnCode, sig.returnBits, sig.returnSigned, false, 0, 0)
        if sig.returnCode != 0:
            emitRaw("%t")
            llvmNativeNumber(resultId)
            emitRaw(" = ")
        emitRaw("call ")
        if sig.returnCode == 0:
            emitRaw("void")
        else:
            llvmEmitAllocaType(sig.returnCode, sig.returnBits, src, structs, arrays, fixedArrays)
        emitRaw(" @")
        emitRawRange(src, calleeStart, calleeEnd)
        emitRaw("(")
        ai: usize = 0
        while ai < argIds.len():
            if ai > 0:
                emitRaw(", ")
            pi: &LLVMParamInfo = &params[sig.paramStart + ai]
            llvmEmitAllocaType(pi.typeCode, pi.bits, src, structs, arrays, fixedArrays)
            emitRaw(" ")
            llvmNative3EmitValue(src, ctx, argIds[ai])
            ai = ai + 1
        emitRaw(")\n")
        if sig.returnCode == 0:
            return llvmNative3New(ctx, 10, 0, false, false, 0, 0)
        return resultId
    bindIndex: i64 = llvmLookupBinding(src, bindings, s, t)
    if bindIndex >= 0:
        b: &LLVMBinding = &bindings[bindIndex as usize]
        if b.typeCode == 6 || b.typeCode == 7 || b.typeCode == 12:
            # Reading a reference-typed local yields the referent's value
            # (auto-deref): load the pointer from its slot, then load the
            # aggregate through it.
            refAddr: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            emitRaw("  %ref.addr")
            llvmNativeNumber(refAddr)
            emitRaw(" = load ptr, ptr ")
            emitLLVMSlotName(src, b.nameStart, b.nameEnd)
            emitRaw("\n")
            derefCode: i32 = 5
            if b.typeCode == 7:
                derefCode = 4
            else if b.typeCode == 12:
                derefCode = 11
            derefId: usize = llvmNative3New(ctx, derefCode, b.bits, false, false, 0, 0)
            emitRaw("%t")
            llvmNativeNumber(derefId)
            emitRaw(" = load ")
            llvmEmitAllocaType(derefCode, b.bits, src, structs, arrays, fixedArrays)
            emitRaw(", ptr %ref.addr")
            llvmNativeNumber(refAddr)
            emitRaw("\n")
            return derefId
        id7: usize = llvmNative3New(ctx, b.typeCode, b.bits, b.isSigned, false, 0, 0)
        emitRaw("%t")
        llvmNativeNumber(id7)
        emitRaw(" = load ")
        if b.typeCode == 1:
            emitLLVMIntType(b.bits)
        else if b.typeCode == 3:
            if b.bits == 32:
                emitRaw("float")
            else:
                emitRaw("double")
        else if b.typeCode == 2:
            emitRaw("i1")
        else if b.typeCode == 17:
            emitRaw("i64")
        else if b.typeCode == 4:
            emitRaw("%")
            emitRawRange(src, structs[b.bits as usize].nameStart, structs[b.bits as usize].nameEnd)
        else if b.typeCode == 5:
            emitRaw("%LannerDynArray")
        else if b.typeCode == 9 || b.typeCode == 12:
            emitRaw("ptr")
        else if b.typeCode == 15 || b.typeCode == 16:
            emitRaw("{ ptr, i64 }")
        else if b.typeCode == 18:
            emitRaw("%LannerArena")
        else if b.typeCode == 11 || b.typeCode == 13 || b.typeCode == 14:
            llvmEmitAllocaType(b.typeCode, b.bits, src, structs, arrays, fixedArrays)
        else:
            return 0
        emitRaw(", ptr ")
        emitLLVMSlotName(src, b.nameStart, b.nameEnd)
        emitRaw("\n")
        return id7
    comptimeStart: []i64 = []i64
    comptimeEnd: []i64 = []i64
    if llvmFindComptimeValue(src, s, t, ctx, &mut comptimeStart, &mut comptimeEnd):
        return llvmNative3Expr(src, comptimeStart[0] as usize, comptimeEnd[0] as usize, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
    opPos: []usize = []usize
    opLen: []usize = []usize
    prec: i32 = llvmFindLowestOp(src, s, t, &mut opPos, &mut opLen)
    if prec < 0:
        methodDot: usize = llvmFindMethodDot(src, s, t)
        if methodDot > 0:
            targetStart: usize = trimStart(src, s, methodDot)
            targetEnd: usize = trimEnd(src, targetStart, methodDot)
            methodNameStart: usize = methodDot + 1
            parenOpen2: usize = methodNameStart
            while parenOpen2 < t && src[parenOpen2] != 40:
                parenOpen2 = parenOpen2 + 1
            if parenOpen2 < t:
                methodNameEnd: usize = parenOpen2
                isLen: bool = methodNameEnd - methodNameStart == 3 && src[methodNameStart] == 108 && src[methodNameStart + 1] == 101 && src[methodNameStart + 2] == 110
                isEmpty: bool = methodNameEnd - methodNameStart == 7 && src[methodNameStart] == 105 && src[methodNameStart + 1] == 115 && src[methodNameStart + 2] == 69 && src[methodNameStart + 3] == 109 && src[methodNameStart + 4] == 112 && src[methodNameStart + 5] == 116 && src[methodNameStart + 6] == 121
                isPush: bool = methodNameEnd - methodNameStart == 4 && src[methodNameStart] == 112 && src[methodNameStart + 1] == 117 && src[methodNameStart + 2] == 115 && src[methodNameStart + 3] == 104
                if isLen:
                    targetVal: usize = llvmNative3Expr(src, targetStart, targetEnd, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
                    if targetVal == 0 || (llvmNative3Code(ctx, targetVal) != 5 && llvmNative3Code(ctx, targetVal) != 15 && llvmNative3Code(ctx, targetVal) != 16):
                        return 0
                    if llvmNative3Code(ctx, targetVal) == 15 || llvmNative3Code(ctx, targetVal) == 16:
                        viewLenResult: usize = llvmNative3New(ctx, 1, 64, false, false, 0, 0)
                        emitRaw("%t")
                        llvmNativeNumber(viewLenResult)
                        emitRaw(" = extractvalue { ptr, i64 } ")
                        llvmNative3EmitValue(src, ctx, targetVal)
                        emitRaw(", 1\n")
                        return viewLenResult
                    lenResult: usize = llvmNative3New(ctx, 1, 64, false, false, 0, 0)
                    emitRaw("%t")
                    llvmNativeNumber(lenResult)
                    emitRaw(" = extractvalue %LannerDynArray ")
                    llvmNative3EmitValue(src, ctx, targetVal)
                    emitRaw(", 1\n")
                    return lenResult
                if isEmpty:
                    targetVal2: usize = llvmNative3Expr(src, targetStart, targetEnd, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
                    if targetVal2 == 0 || (llvmNative3Code(ctx, targetVal2) != 5 && llvmNative3Code(ctx, targetVal2) != 15 && llvmNative3Code(ctx, targetVal2) != 16):
                        return 0
                    lenEmpty: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %empty.len")
                    llvmNativeNumber(lenEmpty)
                    if llvmNative3Code(ctx, targetVal2) == 15 || llvmNative3Code(ctx, targetVal2) == 16:
                        emitRaw(" = extractvalue { ptr, i64 } ")
                    else:
                        emitRaw(" = extractvalue %LannerDynArray ")
                    llvmNative3EmitValue(src, ctx, targetVal2)
                    emitRaw(", 1\n")
                    emptyResult: usize = llvmNative3New(ctx, 2, 1, false, false, 0, 0)
                    emitRaw("%t")
                    llvmNativeNumber(emptyResult)
                    emitRaw(" = icmp eq i64 %empty.len")
                    llvmNativeNumber(lenEmpty)
                    emitRaw(", 0\n")
                    return emptyResult
                if isPush:
                    bIdx: i64 = llvmLookupBinding(src, bindings, targetStart, targetEnd)
                    if bIdx < 0:
                        return 0
                    bnd: &LLVMBinding = &bindings[bIdx as usize]
                    if bnd.typeCode != 5 && bnd.typeCode != 6:
                        return 0
                    arrIdx3: usize = bnd.bits as usize
                    elemInfo3: LLVMTypeInfo = llvmElemLLVMType(&arrays[arrIdx3])
                    argVal: usize = llvmNative3Expr(src, parenOpen2 + 1, t - 1, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
                    if argVal == 0 || llvmNative3Code(ctx, argVal) != elemInfo3.code:
                        return 0
                    if elemInfo3.code == 1 && !llvmNative3IsImmediate(ctx, argVal) && llvmNative3Bits(ctx, argVal) != elemInfo3.bits:
                        return 0
                    pushAddr: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.addr")
                    llvmNativeNumber(pushAddr)
                    if bnd.typeCode == 6:
                        emitRaw(" = load ptr, ptr ")
                    else:
                        emitRaw(" = getelementptr inbounds i8, ptr ")
                    emitLLVMSlotName(src, bnd.nameStart, bnd.nameEnd)
                    if bnd.typeCode == 5:
                        emitRaw(", i64 0")
                    emitRaw("\n")
                    cur: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.cur")
                    llvmNativeNumber(cur)
                    emitRaw(" = load %LannerDynArray, ptr %push.addr")
                    llvmNativeNumber(pushAddr)
                    emitRaw("\n")
                    dataId: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.data")
                    llvmNativeNumber(dataId)
                    emitRaw(" = extractvalue %LannerDynArray %push.cur")
                    llvmNativeNumber(cur)
                    emitRaw(", 0\n")
                    lenId: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.len")
                    llvmNativeNumber(lenId)
                    emitRaw(" = extractvalue %LannerDynArray %push.cur")
                    llvmNativeNumber(cur)
                    emitRaw(", 1\n")
                    capId: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.cap")
                    llvmNativeNumber(capId)
                    emitRaw(" = extractvalue %LannerDynArray %push.cur")
                    llvmNativeNumber(cur)
                    emitRaw(", 2\n")
                    arenaId: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.arena")
                    llvmNativeNumber(arenaId)
                    emitRaw(" = extractvalue %LannerDynArray %push.cur")
                    llvmNativeNumber(cur)
                    emitRaw(", 3\n")
                    newLenId: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.newlen")
                    llvmNativeNumber(newLenId)
                    emitRaw(" = add i64 %push.len")
                    llvmNativeNumber(lenId)
                    emitRaw(", 1\n")
                    needGrowId: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.needgrow")
                    llvmNativeNumber(needGrowId)
                    emitRaw(" = icmp ult i64 %push.cap")
                    llvmNativeNumber(capId)
                    emitRaw(", %push.newlen")
                    llvmNativeNumber(newLenId)
                    emitRaw("\n  br i1 %push.needgrow")
                    llvmNativeNumber(needGrowId)
                    emitRaw(", label ")
                    growLabel: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    llvmNativeEmitLabelRef("push.grow", growLabel)
                    emitRaw(", label ")
                    keepLabel: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    llvmNativeEmitLabelRef("push.keep", keepLabel)
                    emitRaw("\n")
                    pushMergeLabel: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1

                    llvmNativeEmitLabelDef("push.grow", growLabel)
                    capNonZeroId: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.capnz")
                    llvmNativeNumber(capNonZeroId)
                    emitRaw(" = icmp ne i64 %push.cap")
                    llvmNativeNumber(capId)
                    emitRaw(", 0\n")
                    doubledCapId: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.doubled")
                    llvmNativeNumber(doubledCapId)
                    emitRaw(" = add i64 %push.cap")
                    llvmNativeNumber(capId)
                    emitRaw(", %push.cap")
                    llvmNativeNumber(capId)
                    emitRaw("\n")
                    grownCapId: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.grown")
                    llvmNativeNumber(grownCapId)
                    emitRaw(" = select i1 %push.capnz")
                    llvmNativeNumber(capNonZeroId)
                    emitRaw(", i64 %push.doubled")
                    llvmNativeNumber(doubledCapId)
                    emitRaw(", i64 1\n")
                    needCapId: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.needcap")
                    llvmNativeNumber(needCapId)
                    emitRaw(" = icmp ult i64 %push.grown")
                    llvmNativeNumber(grownCapId)
                    emitRaw(", %push.newlen")
                    llvmNativeNumber(newLenId)
                    emitRaw("\n")
                    growNewCapId: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.newcap.grow")
                    llvmNativeNumber(growNewCapId)
                    emitRaw(" = select i1 %push.needcap")
                    llvmNativeNumber(needCapId)
                    emitRaw(", i64 %push.newlen")
                    llvmNativeNumber(newLenId)
                    emitRaw(", i64 %push.grown")
                    llvmNativeNumber(grownCapId)
                    emitRaw("\n")
                    bytesPairId: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.bytes.pair")
                    llvmNativeNumber(bytesPairId)
                    emitRaw(" = call { i64, i1 } @llvm.umul.with.overflow.i64(i64 %push.newcap.grow")
                    llvmNativeNumber(growNewCapId)
                    emitRaw(", i64 ")
                    llvmNativeNumber(llvmElemByteSize(structs, fields, &arrays[arrIdx3]))
                    emitRaw(")\n")
                    bytesValueId: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.bytes.value")
                    llvmNativeNumber(bytesValueId)
                    emitRaw(" = extractvalue { i64, i1 } %push.bytes.pair")
                    llvmNativeNumber(bytesPairId)
                    emitRaw(", 0\n")
                    bytesOvId: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.bytes.ov")
                    llvmNativeNumber(bytesOvId)
                    emitRaw(" = extractvalue { i64, i1 } %push.bytes.pair")
                    llvmNativeNumber(bytesPairId)
                    emitRaw(", 1\n")
                    pushGrowBytesOk: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    pushGrowBytesBad: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  br i1 %push.bytes.ov")
                    llvmNativeNumber(bytesOvId)
                    emitRaw(", label ")
                    llvmNativeEmitLabelRef("push.bytes.bad", pushGrowBytesBad)
                    emitRaw(", label ")
                    llvmNativeEmitLabelRef("push.bytes.ok", pushGrowBytesOk)
                    emitRaw("\n")
                    llvmNativeEmitLabelDef("push.bytes.bad", pushGrowBytesBad)
                    emitRaw("  call void @llvm.trap()\n  unreachable\n")
                    llvmNativeEmitLabelDef("push.bytes.ok", pushGrowBytesOk)

                    arenaPresent: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.arenapresent")
                    llvmNativeNumber(arenaPresent)
                    emitRaw(" = icmp ne ptr %push.arena")
                    llvmNativeNumber(arenaId)
                    emitRaw(", null\n")
                    arenaGrowLabel: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    heapGrowLabel: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    pushGrowMerge: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  br i1 %push.arenapresent")
                    llvmNativeNumber(arenaPresent)
                    emitRaw(", label ")
                    llvmNativeEmitLabelRef("push.grow.arena", arenaGrowLabel)
                    emitRaw(", label ")
                    llvmNativeEmitLabelRef("push.grow.heap", heapGrowLabel)
                    emitRaw("\n")

                    llvmNativeEmitLabelDef("push.grow.arena", arenaGrowLabel)
                    arenaNewData: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.growndata.arena")
                    llvmNativeNumber(arenaNewData)
                    emitRaw(" = call ptr @__lanner_arena_alloc(ptr %push.arena")
                    llvmNativeNumber(arenaId)
                    emitRaw(", i64 %push.bytes.value")
                    llvmNativeNumber(bytesValueId)
                    emitRaw(")\n")
                    copyPairArena: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.copy.pair")
                    llvmNativeNumber(copyPairArena)
                    emitRaw(" = call { i64, i1 } @llvm.umul.with.overflow.i64(i64 %push.len")
                    llvmNativeNumber(lenId)
                    emitRaw(", i64 ")
                    llvmNativeNumber(llvmElemByteSize(structs, fields, &arrays[arrIdx3]))
                    emitRaw(")\n")
                    copyBytesArena: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.copy.bytes")
                    llvmNativeNumber(copyBytesArena)
                    emitRaw(" = extractvalue { i64, i1 } %push.copy.pair")
                    llvmNativeNumber(copyPairArena)
                    emitRaw(", 0\n")
                    copyOvArena: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.copy.ov")
                    llvmNativeNumber(copyOvArena)
                    emitRaw(" = extractvalue { i64, i1 } %push.copy.pair")
                    llvmNativeNumber(copyPairArena)
                    emitRaw(", 1\n")
                    copyOk: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    copyBad: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  br i1 %push.copy.ov")
                    llvmNativeNumber(copyOvArena)
                    emitRaw(", label ")
                    llvmNativeEmitLabelRef("push.copy.bad", copyBad)
                    emitRaw(", label ")
                    llvmNativeEmitLabelRef("push.copy.ok", copyOk)
                    emitRaw("\n")
                    llvmNativeEmitLabelDef("push.copy.bad", copyBad)
                    emitRaw("  call void @llvm.trap()\n  unreachable\n")
                    llvmNativeEmitLabelDef("push.copy.ok", copyOk)
                    oldDataPresent: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.old.present")
                    llvmNativeNumber(oldDataPresent)
                    emitRaw(" = icmp ne ptr %push.data")
                    llvmNativeNumber(dataId)
                    emitRaw(", null\n")
                    copyDo: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    copyNone: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    arenaDone: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  br i1 %push.old.present")
                    llvmNativeNumber(oldDataPresent)
                    emitRaw(", label ")
                    llvmNativeEmitLabelRef("push.copy.do", copyDo)
                    emitRaw(", label ")
                    llvmNativeEmitLabelRef("push.copy.none", copyNone)
                    emitRaw("\n")
                    llvmNativeEmitLabelDef("push.copy.do", copyDo)
                    emitRaw("  call void @llvm.memcpy.p0.p0.i64(ptr %push.growndata.arena")
                    llvmNativeNumber(arenaNewData)
                    emitRaw(", ptr %push.data")
                    llvmNativeNumber(dataId)
                    emitRaw(", i64 %push.copy.bytes")
                    llvmNativeNumber(copyBytesArena)
                    emitRaw(", i1 false)\n  br label ")
                    llvmNativeEmitLabelRef("push.copy.done", arenaDone)
                    emitRaw("\n")
                    llvmNativeEmitLabelDef("push.copy.none", copyNone)
                    emitRaw("  br label ")
                    llvmNativeEmitLabelRef("push.copy.done", arenaDone)
                    emitRaw("\n")
                    llvmNativeEmitLabelDef("push.copy.done", arenaDone)
                    emitRaw("  br label ")
                    llvmNativeEmitLabelRef("push.grow.merge", pushGrowMerge)
                    emitRaw("\n")

                    llvmNativeEmitLabelDef("push.grow.heap", heapGrowLabel)
                    grownDataId: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.growndata")
                    llvmNativeNumber(grownDataId)
                    emitRaw(" = call ptr @realloc(ptr %push.data")
                    llvmNativeNumber(dataId)
                    emitRaw(", i64 %push.bytes.value")
                    llvmNativeNumber(bytesValueId)
                    emitRaw(")\n")
                    heapPresent: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    heapOk: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    heapBad: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.heap.present")
                    llvmNativeNumber(heapPresent)
                    emitRaw(" = icmp ne ptr %push.growndata")
                    llvmNativeNumber(grownDataId)
                    emitRaw(", null\n  br i1 %push.heap.present")
                    llvmNativeNumber(heapPresent)
                    emitRaw(", label ")
                    llvmNativeEmitLabelRef("push.heap.alloc.ok", heapOk)
                    emitRaw(", label ")
                    llvmNativeEmitLabelRef("push.heap.alloc.bad", heapBad)
                    emitRaw("\n")
                    llvmNativeEmitLabelDef("push.heap.alloc.bad", heapBad)
                    emitRaw("  call void @llvm.trap()\n  unreachable\n")
                    llvmNativeEmitLabelDef("push.heap.alloc.ok", heapOk)
                    emitRaw("  br label ")
                    llvmNativeEmitLabelRef("push.grow.merge", pushGrowMerge)
                    emitRaw("\n")

                    llvmNativeEmitLabelDef("push.grow.merge", pushGrowMerge)
                    grownMergeDataId: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.newgrowdata")
                    llvmNativeNumber(grownMergeDataId)
                    emitRaw(" = phi ptr [ %push.growndata.arena")
                    llvmNativeNumber(arenaNewData)
                    emitRaw(", ")
                    llvmNativeEmitLabelRef("push.copy.done", arenaDone)
                    emitRaw(" ], [ %push.growndata")
                    llvmNativeNumber(grownDataId)
                    emitRaw(", ")
                    llvmNativeEmitLabelRef("push.heap.alloc.ok", heapOk)
                    emitRaw(" ]\n")
                    emitRaw("  br label ")
                    llvmNativeEmitLabelRef("push.merge", pushMergeLabel)
                    emitRaw("\n")

                    llvmNativeEmitLabelDef("push.keep", keepLabel)
                    emitRaw("  br label ")
                    llvmNativeEmitLabelRef("push.merge", pushMergeLabel)
                    emitRaw("\n")

                    llvmNativeEmitLabelDef("push.merge", pushMergeLabel)
                    newDataId: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.newdata")
                    llvmNativeNumber(newDataId)
                    emitRaw(" = phi ptr [ %push.newgrowdata")
                    llvmNativeNumber(grownMergeDataId)
                    emitRaw(", ")
                    llvmNativeEmitLabelRef("push.grow.merge", pushGrowMerge)
                    emitRaw(" ], [ %push.data")
                    llvmNativeNumber(dataId)
                    emitRaw(", ")
                    llvmNativeEmitLabelRef("push.keep", keepLabel)
                    emitRaw(" ]\n")
                    newCapId: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.newcap")
                    llvmNativeNumber(newCapId)
                    emitRaw(" = phi i64 [ %push.newcap.grow")
                    llvmNativeNumber(growNewCapId)
                    emitRaw(", ")
                    llvmNativeEmitLabelRef("push.grow.merge", pushGrowMerge)
                    emitRaw(" ], [ %push.cap")
                    llvmNativeNumber(capId)
                    emitRaw(", ")
                    llvmNativeEmitLabelRef("push.keep", keepLabel)
                    emitRaw(" ]\n")
                    s1: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.s1")
                    llvmNativeNumber(s1)
                    emitRaw(" = insertvalue %LannerDynArray %push.cur")
                    llvmNativeNumber(cur)
                    emitRaw(", ptr %push.newdata")
                    llvmNativeNumber(newDataId)
                    emitRaw(", 0\n")
                    s2: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.s2")
                    llvmNativeNumber(s2)
                    emitRaw(" = insertvalue %LannerDynArray %push.s1")
                    llvmNativeNumber(s1)
                    emitRaw(", i64 %push.newlen")
                    llvmNativeNumber(newLenId)
                    emitRaw(", 1\n")
                    s3: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.s3")
                    llvmNativeNumber(s3)
                    emitRaw(" = insertvalue %LannerDynArray %push.s2")
                    llvmNativeNumber(s2)
                    emitRaw(", i64 %push.newcap")
                    llvmNativeNumber(newCapId)
                    emitRaw(", 2\n  store %LannerDynArray %push.s3")
                    llvmNativeNumber(s3)
                    emitRaw(", ptr %push.addr")
                    llvmNativeNumber(pushAddr)
                    emitRaw("\n")
                    elemAddrId: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  %push.elemaddr")
                    llvmNativeNumber(elemAddrId)
                    emitRaw(" = getelementptr ")
                    llvmEmitElemType(src, structs, &arrays[arrIdx3])
                    emitRaw(", ptr %push.newdata")
                    llvmNativeNumber(newDataId)
                    emitRaw(", i64 %push.len")
                    llvmNativeNumber(lenId)
                    emitRaw("\n")
                    emitRaw("  store ")
                    llvmEmitElemType(src, structs, &arrays[arrIdx3])
                    emitRaw(" ")
                    llvmNative3EmitValue(src, ctx, argVal)
                    emitRaw(", ptr %push.elemaddr")
                    llvmNativeNumber(elemAddrId)
                    emitRaw("\n")
                    if llvmNative3TypeOwnsResources(elemInfo3.code, elemInfo3.bits, structs, fields, arrays, fixedArrays):
                        argMoveS: usize = trimStart(src, parenOpen2 + 1, t - 1)
                        argMoveE: usize = trimEnd(src, argMoveS, t - 1)
                        if !llvmBindingMarkMoved(src, bindings, argMoveS, argMoveE):
                            return 0
                    return llvmNative3New(ctx, 2, 1, false, false, 0, 0)
        enumFound: []i64 = []i64
        enumValue: i64 = llvmFindEnumVariantValue(src, s, t, &mut enumFound)
        if enumFound.len() > 0 && enumFound[0] != 0:
            enumId: usize = llvmNative3New(ctx, 17, 64, true, false, 0, 0)
            emitRaw("%t")
            llvmNativeNumber(enumId)
            emitRaw(" = add i64 0, ")
            llvmNativeNumber(enumValue as usize)
            emitRaw("\n")
            return enumId
        dotPos: usize = llvmFindTrailingDot(src, s, t)
        if dotPos > 0:
            target: usize = llvmNative3Expr(src, s, dotPos, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
            if target == 0 || llvmNative3Code(ctx, target) != 4:
                return 0
            structIdx2: usize = llvmNative3Bits(ctx, target) as usize
            st2: &LLVMStructInfo = &structs[structIdx2]
            fieldIdx: i64 = llvmStructFieldIndex(src, fields, st2.fieldStart, st2.fieldCount, dotPos + 1, t)
            if fieldIdx < 0:
                return 0
            fInfo3: &LLVMStructFieldInfo = &fields[st2.fieldStart + (fieldIdx as usize)]
            resultId1: usize = llvmNative3New(ctx, fInfo3.typeCode, fInfo3.bits, fInfo3.isSigned, false, 0, 0)
            emitRaw("%t")
            llvmNativeNumber(resultId1)
            emitRaw(" = extractvalue %")
            emitRawRange(src, st2.nameStart, st2.nameEnd)
            emitRaw(" ")
            llvmNative3EmitValue(src, ctx, target)
            emitRaw(", ")
            llvmNativeNumber(fieldIdx as usize)
            emitRaw("\n")
            return resultId1
        idxPos: usize = llvmFindTrailingIndex(src, s, t)
        if idxPos > 0:
            baseStartFixed: usize = trimStart(src, s, idxPos)
            baseEndFixed: usize = trimEnd(src, baseStartFixed, idxPos)
            baseFixed: usize = llvmNative3Expr(src, baseStartFixed, baseEndFixed, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
            if baseFixed != 0 && llvmNative3Code(ctx, baseFixed) == 11:
                idxFixed: usize = llvmNative3Expr(src, idxPos + 1, t - 1, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
                if idxFixed == 0 || llvmNative3Code(ctx, idxFixed) != 1:
                    return 0
                fixedIdx: usize = llvmNative3Bits(ctx, baseFixed) as usize
                faExpr: &LLVMFixedArrayInfo = &fixedArrays[fixedIdx]
                baseAddrFixed: []i64 = []i64
                baseBitsFixed: []i64 = []i64
                baseSignedFixed: []i64 = []i64
                fixedAddr: usize = llvmNative3LValueAddress(src, baseStartFixed, baseEndFixed, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays, &mut baseAddrFixed, &mut baseBitsFixed, &mut baseSignedFixed)
                if fixedAddr == 0 || baseAddrFixed.len() == 0 || baseAddrFixed[0] != 11:
                    return 0
                elemAddrFixed: usize = ctx.nextLabel
                ctx.nextLabel = ctx.nextLabel + 1
                emitRaw("  %fixed.idx.addr")
                llvmNativeNumber(elemAddrFixed)
                emitRaw(" = getelementptr inbounds ")
                llvmEmitAllocaType(11, fixedIdx as i32, src, structs, arrays, fixedArrays)
                emitRaw(", ptr ")
                llvmNative3EmitValue(src, ctx, fixedAddr)
                emitRaw(", i64 0, i64 ")
                llvmNative3EmitValue(src, ctx, idxFixed)
                emitRaw("\n")
                elemResultFixed: usize = llvmNative3New(ctx, faExpr.elemCode, faExpr.elemBits, faExpr.elemSigned, false, 0, 0)
                emitRaw("%t")
                llvmNativeNumber(elemResultFixed)
                emitRaw(" = load ")
                llvmEmitAllocaType(faExpr.elemCode, faExpr.elemBits, src, structs, arrays, fixedArrays)
                emitRaw(", ptr %fixed.idx.addr")
                llvmNativeNumber(elemAddrFixed)
                emitRaw("\n")
                return elemResultFixed
            if baseFixed != 0 && (llvmNative3Code(ctx, baseFixed) == 15 || llvmNative3Code(ctx, baseFixed) == 16):
                idxView: usize = llvmNative3Expr(src, idxPos + 1, t - 1, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
                if idxView == 0 || llvmNative3Code(ctx, idxView) != 1:
                    return 0
                viewIdxMeta: &LLVMArrayInfo = &arrays[llvmNative3Bits(ctx, baseFixed) as usize]
                viewDataId: usize = ctx.nextLabel
                ctx.nextLabel = ctx.nextLabel + 1
                emitRaw("  %view.data")
                llvmNativeNumber(viewDataId)
                emitRaw(" = extractvalue { ptr, i64 } ")
                llvmNative3EmitValue(src, ctx, baseFixed)
                emitRaw(", 0\n")
                viewLenId: usize = ctx.nextLabel
                ctx.nextLabel = ctx.nextLabel + 1
                emitRaw("  %view.len")
                llvmNativeNumber(viewLenId)
                emitRaw(" = extractvalue { ptr, i64 } ")
                llvmNative3EmitValue(src, ctx, baseFixed)
                emitRaw(", 1\n")
                viewBoundsId: usize = ctx.nextLabel
                ctx.nextLabel = ctx.nextLabel + 1
                emitRaw("  %view.inbounds")
                llvmNativeNumber(viewBoundsId)
                emitRaw(" = icmp ult i64 ")
                llvmNative3EmitValue(src, ctx, idxView)
                emitRaw(", %view.len")
                llvmNativeNumber(viewLenId)
                emitRaw("\n")
                viewOkLabel: usize = ctx.nextLabel
                ctx.nextLabel = ctx.nextLabel + 1
                viewBadLabel: usize = ctx.nextLabel
                ctx.nextLabel = ctx.nextLabel + 1
                emitRaw("  br i1 %view.inbounds")
                llvmNativeNumber(viewBoundsId)
                emitRaw(", label ")
                llvmNativeEmitLabelRef("view.idx.ok", viewOkLabel)
                emitRaw(", label ")
                llvmNativeEmitLabelRef("view.idx.bad", viewBadLabel)
                emitRaw("\n")
                llvmNativeEmitLabelDef("view.idx.bad", viewBadLabel)
                emitRaw("  call void @llvm.trap()\n  unreachable\n")
                llvmNativeEmitLabelDef("view.idx.ok", viewOkLabel)
                viewElemAddr: usize = ctx.nextLabel
                ctx.nextLabel = ctx.nextLabel + 1
                emitRaw("  %view.elem.addr")
                llvmNativeNumber(viewElemAddr)
                emitRaw(" = getelementptr inbounds ")
                llvmEmitAllocaType(viewIdxMeta.elemCode, viewIdxMeta.elemBits, src, structs, arrays, fixedArrays)
                emitRaw(", ptr %view.data")
                llvmNativeNumber(viewDataId)
                emitRaw(", i64 ")
                llvmNative3EmitValue(src, ctx, idxView)
                emitRaw("\n")
                viewResult: usize = llvmNative3New(ctx, viewIdxMeta.elemCode, viewIdxMeta.elemBits, viewIdxMeta.elemSigned, false, 0, 0)
                emitRaw("%t")
                llvmNativeNumber(viewResult)
                emitRaw(" = load ")
                llvmEmitAllocaType(viewIdxMeta.elemCode, viewIdxMeta.elemBits, src, structs, arrays, fixedArrays)
                emitRaw(", ptr %view.elem.addr")
                llvmNativeNumber(viewElemAddr)
                emitRaw("\n")
                return viewResult
            targetIdx: usize = baseFixed
            if targetIdx == 0 || llvmNative3Code(ctx, targetIdx) != 5:
                return 0
            arrIdx4: usize = llvmNative3Bits(ctx, targetIdx) as usize
            arr4: &LLVMArrayInfo = &arrays[arrIdx4]
            idxVal: usize = llvmNative3Expr(src, idxPos + 1, t - 1, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
            if idxVal == 0 || llvmNative3Code(ctx, idxVal) != 1:
                return 0
            dataPtr: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            emitRaw("  %idx.data")
            llvmNativeNumber(dataPtr)
            emitRaw(" = extractvalue %LannerDynArray ")
            llvmNative3EmitValue(src, ctx, targetIdx)
            emitRaw(", 0\n")
            elemAddr2: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            emitRaw("  %idx.addr")
            llvmNativeNumber(elemAddr2)
            emitRaw(" = getelementptr ")
            llvmEmitElemType(src, structs, arr4)
            emitRaw(", ptr %idx.data")
            llvmNativeNumber(dataPtr)
            emitRaw(", i64 ")
            llvmNative3EmitValue(src, ctx, idxVal)
            emitRaw("\n")
            resultIdx: usize = llvmNative3New(ctx, arr4.elemCode, arr4.elemBits, arr4.elemSigned, false, 0, 0)
            emitRaw("%t")
            llvmNativeNumber(resultIdx)
            emitRaw(" = load ")
            llvmEmitElemType(src, structs, arr4)
            emitRaw(", ptr %idx.addr")
            llvmNativeNumber(elemAddr2)
            emitRaw("\n")
            return resultIdx
        castPos: usize = llvmFindCast(src, s, t)
        if castPos > 0:
            sourceValue: usize = llvmNative3Expr(src, s, castPos, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
            dstStart: usize = trimStart(src, castPos + 4, t)
            dst: LLVMTypeInfo = llvmTypeFromRange(src, dstStart, t)
            resultCast: usize = llvmNative3Cast(src, ctx, sourceValue, &dst)
            return resultCast
        return 0
    if prec == 1 || prec == 2:
        logicLeft: usize = llvmNative3Expr(src, s, opPos[0], ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
        if logicLeft == 0 || llvmNative3Code(ctx, logicLeft) != 2:
            return 0
        logicLabel: usize = ctx.nextLabel
        ctx.nextLabel = ctx.nextLabel + 1
        logicSlot: usize = ctx.nextLabel
        ctx.nextLabel = ctx.nextLabel + 1
        logicMerge: usize = ctx.nextLabel
        ctx.nextLabel = ctx.nextLabel + 1
        emitRaw("  %logic.slot")
        llvmNativeNumber(logicSlot)
        emitRaw(" = alloca i1\n  br i1 ")
        llvmNative3EmitValue(src, ctx, logicLeft)
        emitRaw(", label ")
        if prec == 2:
            llvmNativeEmitLabelRef("logic.rhs", logicLabel)
            emitRaw(", label ")
            llvmNativeEmitLabelRef("logic.short", logicLabel)
        else:
            llvmNativeEmitLabelRef("logic.short", logicLabel)
            emitRaw(", label ")
            llvmNativeEmitLabelRef("logic.rhs", logicLabel)
        emitRaw("\n")
        llvmNativeEmitLabelDef("logic.short", logicLabel)
        emitRaw("  store i1 ")
        if prec == 2:
            emitRaw("0")
        else:
            emitRaw("1")
        emitRaw(", ptr %logic.slot")
        llvmNativeNumber(logicSlot)
        emitRaw("\n  br label ")
        llvmNativeEmitLabelRef("logic.merge", logicMerge)
        emitRaw("\n")
        llvmNativeEmitLabelDef("logic.rhs", logicLabel)
        logicRight: usize = llvmNative3Expr(src, opPos[0] + opLen[0], t, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
        if logicRight == 0 || llvmNative3Code(ctx, logicRight) != 2:
            return 0
        emitRaw("  store i1 ")
        llvmNative3EmitValue(src, ctx, logicRight)
        emitRaw(", ptr %logic.slot")
        llvmNativeNumber(logicSlot)
        emitRaw("\n  br label ")
        llvmNativeEmitLabelRef("logic.merge", logicMerge)
        emitRaw("\n")
        llvmNativeEmitLabelDef("logic.merge", logicMerge)
        logicResult: usize = llvmNative3New(ctx, 2, 1, false, false, 0, 0)
        emitRaw("%t")
        llvmNativeNumber(logicResult)
        emitRaw(" = load i1, ptr %logic.slot")
        llvmNativeNumber(logicSlot)
        emitRaw("\n")
        return logicResult
    left: usize = llvmNative3Expr(src, s, opPos[0], ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
    right: usize = llvmNative3Expr(src, opPos[0] + opLen[0], t, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
    if left == 0:
        return 0
    if right == 0:
        return 0
    leftCode: i32 = llvmNative3Code(ctx, left)
    rightCode: i32 = llvmNative3Code(ctx, right)
    if leftCode == 0:
        return 0
    if rightCode == 0:
        return 0
    if leftCode == 2:
        if rightCode == 2:
            if prec != 3 && prec != 4:
                return 0
    leftImmediate: bool = llvmNative3IsImmediate(ctx, left)
    rightImmediate: bool = llvmNative3IsImmediate(ctx, right)
    if !leftImmediate:
        if !rightImmediate:
            if !llvmNative3SameType(ctx, left, right):
                return 0
    if prec == 3 || prec == 4:
        id8: usize = llvmNative3New(ctx, 2, 1, false, false, 0, 0)
        emitRaw("%t")
        llvmNativeNumber(id8)
        if llvmNative3Code(ctx, left) == 3:
            emitRaw(" = fcmp ")
            opFloat: u8 = src[opPos[0]]
            if prec == 3:
                if opFloat == 61:
                    emitRaw("oeq ")
                else:
                    emitRaw("one ")
            else if opFloat == 60 && src[opPos[0] + 1] == 61:
                emitRaw("ole ")
            else if opFloat == 62 && src[opPos[0] + 1] == 61:
                emitRaw("oge ")
            else if opFloat == 60:
                emitRaw("olt ")
            else:
                emitRaw("ogt ")
            if llvmNative3Bits(ctx, left) == 32:
                emitRaw("float ")
            else:
                emitRaw("double ")
            llvmNative3EmitValue(src, ctx, left)
            emitRaw(", ")
            llvmNative3EmitValue(src, ctx, right)
            emitRaw("\n")
            return id8
        emitRaw(" = icmp ")
        op2: u8 = src[opPos[0]]
        if prec == 3:
            if op2 == 61:
                emitRaw("eq ")
            else:
                emitRaw("ne ")
        else if op2 == 60 && src[opPos[0] + 1] == 61:
            if llvmNative3Signed(ctx, left):
                emitRaw("sle ")
            else:
                emitRaw("ule ")
        else if op2 == 62 && src[opPos[0] + 1] == 61:
            if llvmNative3Signed(ctx, left):
                emitRaw("sge ")
            else:
                emitRaw("uge ")
        else if op2 == 60:
            if llvmNative3Signed(ctx, left):
                emitRaw("slt ")
            else:
                emitRaw("ult ")
        else if llvmNative3Signed(ctx, left):
            emitRaw("sgt ")
        else:
            emitRaw("ugt ")
        if llvmNative3Code(ctx, left) == 2:
            emitRaw("i1 ")
        else:
            emitLLVMIntType(llvmNative3Bits(ctx, left))
            emitRaw(" ")
        llvmNative3EmitValue(src, ctx, left)
        emitRaw(", ")
        llvmNative3EmitValue(src, ctx, right)
        emitRaw("\n")
        return id8
    if llvmNative3Code(ctx, left) == 3:
        idFloatOp: usize = llvmNative3New(ctx, 3, llvmNative3Bits(ctx, left), true, false, 0, 0)
        emitRaw("%t")
        llvmNativeNumber(idFloatOp)
        emitRaw(" = ")
        opFloat2: u8 = src[opPos[0]]
        if prec == 5:
            return 0
        else if prec == 6:
            return 0
        else if prec == 7:
            return 0
        else if prec == 8:
            return 0
        else if prec == 9:
            if opFloat2 == 43:
                emitRaw("fadd ")
            else:
                emitRaw("fsub ")
        else if prec == 10:
            if opFloat2 == 42:
                emitRaw("fmul ")
            else if opFloat2 == 47:
                emitRaw("fdiv ")
            else:
                emitRaw("frem ")
        else:
            return 0
        if llvmNative3Bits(ctx, left) == 32:
            emitRaw("float ")
        else:
            emitRaw("double ")
        llvmNative3EmitValue(src, ctx, left)
        emitRaw(", ")
        llvmNative3EmitValue(src, ctx, right)
        emitRaw("\n")
        return idFloatOp
    if llvmNative3Code(ctx, left) != 1:
        return 0
    id9: usize = llvmNative3New(ctx, 1, llvmNative3Bits(ctx, left), llvmNative3Signed(ctx, left), false, 0, 0)
    emitRaw("%t")
    llvmNativeNumber(id9)
    emitRaw(" = ")
    op3: u8 = src[opPos[0]]
    if prec == 5:
        emitRaw("or ")
    else if prec == 6:
        emitRaw("xor ")
    else if prec == 7:
        emitRaw("and ")
    else if prec == 8:
        if op3 == 60:
            emitRaw("shl ")
        else if llvmNative3Signed(ctx, left):
            emitRaw("ashr ")
        else:
            emitRaw("lshr ")
    else if prec == 9:
        if op3 == 43:
            emitRaw("add ")
        else:
            emitRaw("sub ")
    else if prec == 10:
        if op3 == 42:
            emitRaw("mul ")
        else if op3 == 47:
            if llvmNative3Signed(ctx, left):
                emitRaw("sdiv ")
            else:
                emitRaw("udiv ")
        else if llvmNative3Signed(ctx, left):
            emitRaw("srem ")
        else:
            emitRaw("urem ")
    else:
        return 0
    emitLLVMIntType(llvmNative3Bits(ctx, left))
    emitRaw(" ")
    llvmNative3EmitValue(src, ctx, left)
    emitRaw(", ")
    llvmNative3EmitValue(src, ctx, right)
    emitRaw("\n")
    return id9

llvmNative3FindAssignEq(src: &[]u8, start: usize, end: usize) usize:
    depth: usize = 0
    i: usize = start
    while i < end:
        if src[i] == 40 || src[i] == 91:
            depth = depth + 1
        else if src[i] == 41 || src[i] == 93:
            if depth > 0:
                depth = depth - 1
        else if src[i] == 61 && depth == 0:
            if i + 1 < end && src[i + 1] == 61:
                i = i + 2
                continue
            return i
        i = i + 1
    return 0

llvmNative3LValueAddress(src: &[]u8, start: usize, end: usize, ctx: &mut LLVMNativeContext, bindings: &mut []LLVMBinding, funcs: &[]LLVMFunctionSig, params: &[]LLVMParamInfo, structs: &[]LLVMStructInfo, fields: &[]LLVMStructFieldInfo, arrays: &mut []LLVMArrayInfo, fixedArrays: &mut []LLVMFixedArrayInfo, codeOut: &mut []i64, bitsOut: &mut []i64, signedOut: &mut []i64) usize:
    s: usize = trimStart(src, start, end)
    t: usize = trimEnd(src, s, end)
    dotPos: usize = llvmFindTrailingDot(src, s, t)
    if dotPos > 0:
        baseCode: []i64 = []i64
        baseBits: []i64 = []i64
        baseSigned: []i64 = []i64
        baseAddr: usize = llvmNative3LValueAddress(src, s, dotPos, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays, &mut baseCode, &mut baseBits, &mut baseSigned)
        if baseAddr == 0 || baseCode[0] != 4:
            return 0
        st: &LLVMStructInfo = &structs[baseBits[0] as usize]
        fieldIdx: i64 = llvmStructFieldIndex(src, fields, st.fieldStart, st.fieldCount, dotPos + 1, t)
        if fieldIdx < 0:
            return 0
        f: &LLVMStructFieldInfo = &fields[st.fieldStart + (fieldIdx as usize)]
        addrId: usize = llvmNative3New(ctx, 8, 0, false, false, 0, 0)
        emitRaw("%t")
        llvmNativeNumber(addrId)
        emitRaw(" = getelementptr inbounds %")
        emitRawRange(src, st.nameStart, st.nameEnd)
        emitRaw(", ptr ")
        llvmNative3EmitValue(src, ctx, baseAddr)
        emitRaw(", i32 0, i32 ")
        llvmNativeNumber(fieldIdx as usize)
        emitRaw("\n")
        codeOut.push(f.typeCode as i64)
        bitsOut.push(f.bits as i64)
        if f.isSigned:
            signedOut.push(1)
        else:
            signedOut.push(0)
        return addrId
    idxPos: usize = llvmFindTrailingIndex(src, s, t)
    if idxPos > 0:
        baseStartFixed: usize = trimStart(src, s, idxPos)
        baseEndFixed: usize = trimEnd(src, baseStartFixed, idxPos)
        baseCodeFixed: []i64 = []i64
        baseBitsFixed: []i64 = []i64
        baseSignedFixed: []i64 = []i64
        baseAddrFixed: usize = llvmNative3LValueAddress(src, baseStartFixed, baseEndFixed, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays, &mut baseCodeFixed, &mut baseBitsFixed, &mut baseSignedFixed)
        if baseAddrFixed != 0 && baseCodeFixed.len() > 0 && baseCodeFixed[0] == 11:
            idxFixed2: usize = llvmNative3Expr(src, idxPos + 1, t - 1, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
            if idxFixed2 == 0 || llvmNative3Code(ctx, idxFixed2) != 1:
                return 0
            fixedIdx2: usize = baseBitsFixed[0] as usize
            faLvalue: &LLVMFixedArrayInfo = &fixedArrays[fixedIdx2]
            addrFixed2: usize = llvmNative3New(ctx, 8, 0, false, false, 0, 0)
            emitRaw("%t")
            llvmNativeNumber(addrFixed2)
            emitRaw(" = getelementptr inbounds ")
            llvmEmitAllocaType(11, fixedIdx2 as i32, src, structs, arrays, fixedArrays)
            emitRaw(", ptr ")
            llvmNative3EmitValue(src, ctx, baseAddrFixed)
            emitRaw(", i64 0, i64 ")
            llvmNative3EmitValue(src, ctx, idxFixed2)
            emitRaw("\n")
            codeOut.push(faLvalue.elemCode as i64)
            bitsOut.push(faLvalue.elemBits as i64)
            if faLvalue.elemSigned:
                signedOut.push(1)
            else:
                signedOut.push(0)
            return addrFixed2
        baseViewVal: usize = llvmNative3Expr(src, baseStartFixed, baseEndFixed, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
        if baseViewVal != 0 && (llvmNative3Code(ctx, baseViewVal) == 15 || llvmNative3Code(ctx, baseViewVal) == 16):
            if llvmNative3Code(ctx, baseViewVal) == 15:
                return 0
            idxView2: usize = llvmNative3Expr(src, idxPos + 1, t - 1, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
            if idxView2 == 0 || llvmNative3Code(ctx, idxView2) != 1:
                return 0
            viewMeta2: &LLVMArrayInfo = &arrays[llvmNative3Bits(ctx, baseViewVal) as usize]
            viewDataId2: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            emitRaw("  %view.lvalue.data")
            llvmNativeNumber(viewDataId2)
            emitRaw(" = extractvalue { ptr, i64 } ")
            llvmNative3EmitValue(src, ctx, baseViewVal)
            emitRaw(", 0\n")
            viewLenId2: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            emitRaw("  %view.lvalue.len")
            llvmNativeNumber(viewLenId2)
            emitRaw(" = extractvalue { ptr, i64 } ")
            llvmNative3EmitValue(src, ctx, baseViewVal)
            emitRaw(", 1\n")
            bounds2: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            emitRaw("  %view.lvalue.inbounds")
            llvmNativeNumber(bounds2)
            emitRaw(" = icmp ult i64 ")
            llvmNative3EmitValue(src, ctx, idxView2)
            emitRaw(", %view.lvalue.len")
            llvmNativeNumber(viewLenId2)
            emitRaw("\n")
            ok2: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            bad2: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            emitRaw("  br i1 %view.lvalue.inbounds")
            llvmNativeNumber(bounds2)
            emitRaw(", label ")
            llvmNativeEmitLabelRef("view.lvalue.ok", ok2)
            emitRaw(", label ")
            llvmNativeEmitLabelRef("view.lvalue.bad", bad2)
            emitRaw("\n")
            llvmNativeEmitLabelDef("view.lvalue.bad", bad2)
            emitRaw("  call void @llvm.trap()\n  unreachable\n")
            llvmNativeEmitLabelDef("view.lvalue.ok", ok2)
            addrView2: usize = llvmNative3New(ctx, 8, 0, false, false, 0, 0)
            emitRaw("%t")
            llvmNativeNumber(addrView2)
            emitRaw(" = getelementptr inbounds ")
            llvmEmitAllocaType(viewMeta2.elemCode, viewMeta2.elemBits, src, structs, arrays, fixedArrays)
            emitRaw(", ptr %view.lvalue.data")
            llvmNativeNumber(viewDataId2)
            emitRaw(", i64 ")
            llvmNative3EmitValue(src, ctx, idxView2)
            emitRaw("\n")
            codeOut.push(viewMeta2.elemCode as i64)
            bitsOut.push(viewMeta2.elemBits as i64)
            if viewMeta2.elemSigned:
                signedOut.push(1)
            else:
                signedOut.push(0)
            return addrView2
        baseStart: usize = baseStartFixed
        baseEnd: usize = trimEnd(src, baseStart, idxPos)
        baseVal: usize = llvmNative3Expr(src, baseStart, baseEnd, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
        if baseVal == 0 || llvmNative3Code(ctx, baseVal) != 5:
            return 0
        idxVal: usize = llvmNative3Expr(src, idxPos + 1, t - 1, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
        if idxVal == 0 || llvmNative3Code(ctx, idxVal) != 1:
            return 0
        arrIdx: usize = llvmNative3Bits(ctx, baseVal) as usize
        arr: &LLVMArrayInfo = &arrays[arrIdx]
        dataId: usize = ctx.nextLabel
        ctx.nextLabel = ctx.nextLabel + 1
        emitRaw("  %lvalue.data")
        llvmNativeNumber(dataId)
        emitRaw(" = extractvalue %LannerDynArray ")
        llvmNative3EmitValue(src, ctx, baseVal)
        emitRaw(", 0\n")
        addrId4: usize = llvmNative3New(ctx, 8, 0, false, false, 0, 0)
        emitRaw("%t")
        llvmNativeNumber(addrId4)
        emitRaw(" = getelementptr inbounds ")
        llvmEmitElemType(src, structs, arr)
        emitRaw(", ptr %lvalue.data")
        llvmNativeNumber(dataId)
        emitRaw(", i64 ")
        llvmNative3EmitValue(src, ctx, idxVal)
        emitRaw("\n")
        codeOut.push(arr.elemCode as i64)
        bitsOut.push(arr.elemBits as i64)
        if arr.elemSigned:
            signedOut.push(1)
        else:
            signedOut.push(0)
        return addrId4
    bindIndex: i64 = llvmLookupBinding(src, bindings, s, t)
    if bindIndex < 0:
        return 0
    b: &LLVMBinding = &bindings[bindIndex as usize]
    if b.typeCode == 4 || b.typeCode == 5 || b.typeCode == 11 || b.typeCode == 18:
        addrId2: usize = llvmNative3New(ctx, 8, 0, false, false, 0, 0)
        emitRaw("%t")
        llvmNativeNumber(addrId2)
        emitRaw(" = getelementptr inbounds i8, ptr ")
        emitLLVMSlotName(src, b.nameStart, b.nameEnd)
        emitRaw(", i64 0\n")
        codeOut.push(b.typeCode as i64)
        bitsOut.push(b.bits as i64)
        if b.isSigned:
            signedOut.push(1)
        else:
            signedOut.push(0)
        return addrId2
    if b.typeCode == 6 || b.typeCode == 7 || b.typeCode == 12:
        addrId3: usize = llvmNative3New(ctx, 8, 0, false, false, 0, 0)
        emitRaw("%t")
        llvmNativeNumber(addrId3)
        emitRaw(" = load ptr, ptr ")
        emitLLVMSlotName(src, b.nameStart, b.nameEnd)
        emitRaw("\n")
        if b.typeCode == 6:
            codeOut.push(5)
        else if b.typeCode == 7:
            codeOut.push(4)
        else:
            codeOut.push(11)
        bitsOut.push(b.bits as i64)
        signedOut.push(0)
        return addrId3
    return 0

llvmNative3EmitLValueStore(src: &[]u8, lhsStart: usize, lhsEnd: usize, rhsStart: usize, rhsEnd: usize, ctx: &mut LLVMNativeContext, bindings: &mut []LLVMBinding, funcs: &[]LLVMFunctionSig, params: &[]LLVMParamInfo, structs: &[]LLVMStructInfo, fields: &[]LLVMStructFieldInfo, arrays: &mut []LLVMArrayInfo, fixedArrays: &mut []LLVMFixedArrayInfo) bool:
    codeOut: []i64 = []i64
    bitsOut: []i64 = []i64
    signedOut: []i64 = []i64
    addr: usize = llvmNative3LValueAddress(src, lhsStart, lhsEnd, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays, &mut codeOut, &mut bitsOut, &mut signedOut)
    if addr == 0:
        return false
    value: usize = llvmNative3Expr(src, rhsStart, rhsEnd, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
    if value == 0 || codeOut.len() == 0 || llvmNative3Code(ctx, value) != codeOut[0] as i32:
        return false
    if codeOut[0] as i32 == 1 && !llvmNative3IsImmediate(ctx, value) && llvmNative3Bits(ctx, value) != bitsOut[0] as i32:
        return false
    if (codeOut[0] as i32 == 4 || codeOut[0] as i32 == 5 || codeOut[0] as i32 == 11 || codeOut[0] as i32 == 13 || codeOut[0] as i32 == 14 || codeOut[0] as i32 == 18) && llvmNative3Bits(ctx, value) != bitsOut[0] as i32:
        return false
    if llvmNative3TypeOwnsResources(codeOut[0] as i32, bitsOut[0] as i32, structs, fields, arrays, fixedArrays):
        llvmNative3EmitDropCallValue(codeOut[0] as i32, bitsOut[0] as i32, src, ctx, addr)
    emitRaw("  store ")
    llvmEmitAllocaType(codeOut[0] as i32, bitsOut[0] as i32, src, structs, arrays, fixedArrays)
    emitRaw(" ")
    llvmNative3EmitValue(src, ctx, value)
    emitRaw(", ptr ")
    llvmNative3EmitValue(src, ctx, addr)
    emitRaw("\n")
    rhsMoveS: usize = trimStart(src, rhsStart, rhsEnd)
    rhsMoveE: usize = trimEnd(src, rhsMoveS, rhsEnd)
    if llvmNative3TypeOwnsResources(codeOut[0] as i32, bitsOut[0] as i32, structs, fields, arrays, fixedArrays):
        if !llvmBindingMarkMoved(src, bindings, rhsMoveS, rhsMoveE):
            return false
    return true

llvmNative3EmitStore(src: &[]u8, ctx: &mut LLVMNativeContext, b: &LLVMBinding, value: usize, structs: &[]LLVMStructInfo, arrays: &mut []LLVMArrayInfo, fixedArrays: &mut []LLVMFixedArrayInfo) bool:
    if value == 0:
        return false
    valueCode: i32 = llvmNative3Code(ctx, value)
    storeValue: usize = value
    if valueCode == 1 && b.typeCode == 1 && llvmNative3Bits(ctx, value) != b.bits:
        dstInfo: LLVMTypeInfo = LLVMTypeInfo[code: 1, bits: b.bits, isSigned: b.isSigned]
        storeValue = llvmNative3Cast(src, ctx, value, &dstInfo)
        if storeValue == 0:
            return false
        valueCode = llvmNative3Code(ctx, storeValue)
    if valueCode != b.typeCode && !(valueCode == 8 && (b.typeCode == 6 || b.typeCode == 7 || b.typeCode == 12)):
        return false
    if b.typeCode == 1 && llvmNative3Bits(ctx, storeValue) != b.bits:
        return false
    if (b.typeCode == 4 || b.typeCode == 5 || b.typeCode == 11 || b.typeCode == 13 || b.typeCode == 14 || b.typeCode == 15 || b.typeCode == 16) && llvmNative3Bits(ctx, storeValue) != b.bits:
        return false
    emitRaw("  store ")
    if b.typeCode == 1:
        emitLLVMIntType(b.bits)
    else if b.typeCode == 2:
        emitRaw("i1")
    else if b.typeCode == 3:
        if b.bits == 32:
            emitRaw("float")
        else:
            emitRaw("double")
    else if b.typeCode == 17:
        emitRaw("i64")
    else if b.typeCode == 4:
        emitRaw("%")
        emitRawRange(src, structs[b.bits as usize].nameStart, structs[b.bits as usize].nameEnd)
    else if b.typeCode == 5:
        emitRaw("%LannerDynArray")
    else if b.typeCode == 18:
        emitRaw("%LannerArena")
    else if b.typeCode == 6 || b.typeCode == 7 || b.typeCode == 9:
        emitRaw("ptr")
    else if b.typeCode == 12:
        emitRaw("ptr")
    else if b.typeCode == 11 || b.typeCode == 13 || b.typeCode == 14 || b.typeCode == 15 || b.typeCode == 16:
        llvmEmitAllocaType(b.typeCode, b.bits, src, structs, arrays, fixedArrays)
    else:
        return false
    emitRaw(" ")
    llvmNative3EmitValue(src, ctx, storeValue)
    emitRaw(", ptr ")
    emitLLVMSlotName(src, b.nameStart, b.nameEnd)
    emitRaw("\n")
    return true

llvmNativeBlockExplicitTerminator(src: &[]u8, nodes: &[]AstNode, start: usize, end: usize, parentLevel: usize) bool:
    i: usize = end
    while i > start:
        i = i - 1
        if nodes[i].level <= parentLevel:
            return false
        s: usize = trimStart(src, nodes[i].start, nodes[i].end)
        t: usize = trimEnd(src, s, nodes[i].end)
        if startsWord(src, s, t, 5) || startsWord(src, s, t, 6) || startsWord(src, s, t, 7):
            return true
        # The last source statement exists and is not an explicit terminator.
        return false
    return false

# Status: 0 = fallthrough, 1 = explicit control-flow termination, 2 = emission error.
llvmNative3EmitBlock(src: &[]u8, nodes: &[]AstNode, start: usize, end: usize, parentLevel: usize, ctx: &mut LLVMNativeContext, bindings: &mut []LLVMBinding, funcs: &[]LLVMFunctionSig, params: &[]LLVMParamInfo, structs: &[]LLVMStructInfo, fields: &[]LLVMStructFieldInfo, arrays: &mut []LLVMArrayInfo, fixedArrays: &mut []LLVMFixedArrayInfo, returnCode: i32, returnBits: i32, inLoop: bool, loopContinueLabel: usize, loopBreakLabel: usize) i32:
    i: usize = start
    terminated: bool = false
    while i < end && nodes[i].level > parentLevel && !terminated:
        if nodes[i].level != parentLevel + 1:
            i = i + 1
            continue
        kind: i64 = nodes[i].kind
        s: usize = trimStart(src, nodes[i].start, nodes[i].end)
        t: usize = trimEnd(src, s, nodes[i].end)
        if startsWord(src, s, t, 3):
            kind = 3
        else if startsWord(src, s, t, 2):
            kind = 4
        else if startsWord(src, s, t, 35):
            kind = 5
        else if startsWord(src, s, t, 5):
            kind = 6
        else if startsWord(src, s, t, 6):
            kind = 7
        else if startsWord(src, s, t, 7):
            kind = 8
        else if startsWord(src, s, t, 45):
            kind = 15
        if kind == 3:
            cond: usize = llvmNative3Expr(src, s + 3, t - 1, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
            if cond == 0 || llvmNative3Code(ctx, cond) != 2:
                return 2
            labelId: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            thenEnd: usize = llvmNativeSubtreeEnd(nodes, i, end)
            hasElse: bool = thenEnd < end && nodes[thenEnd].level == nodes[i].level && nodes[thenEnd].kind == 5
            emitRaw("  br i1 ")
            llvmNative3EmitValue(src, ctx, cond)
            emitRaw(", label ")
            llvmNativeEmitLabelRef("if.then", labelId)
            emitRaw(", label ")
            llvmNativeEmitLabelRef("if.else", labelId)
            emitRaw("\n")
            llvmNativeEmitLabelDef("if.then", labelId)
            thenStatus: i32 = llvmNative3EmitBlock(src, nodes, i + 1, thenEnd, nodes[i].level, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays, returnCode, returnBits, inLoop, loopContinueLabel, loopBreakLabel)
            if thenStatus == 2:
                return 2
            thenTerminated: bool = thenStatus == 1
            if thenTerminated && !llvmNativeBlockExplicitTerminator(src, nodes, i + 1, thenEnd, nodes[i].level):
                thenTerminated = false
            if !thenTerminated:
                emitRaw("  br label ")
                llvmNativeEmitLabelRef("if.merge", labelId)
                emitRaw("\n")

            llvmNativeEmitLabelDef("if.else", labelId)
            chainNode: usize = thenEnd
            chainLabel: usize = labelId
            chainFirst: bool = true
            chainAllTerminated: bool = thenTerminated
            while chainNode < end && nodes[chainNode].level == nodes[i].level && nodes[chainNode].kind == 5:
                if !chainFirst:
                    llvmNativeEmitLabelDef("if.chain", chainLabel)
                chainFirst = false
                elseStart: usize = trimStart(src, nodes[chainNode].start, nodes[chainNode].end)
                elseEndLine: usize = trimEnd(src, elseStart, nodes[chainNode].end)
                bodyEnd: usize = llvmNativeSubtreeEnd(nodes, chainNode, end)
                isElseIf: bool = elseEndLine - elseStart >= 8 && src[elseStart] == 101 && src[elseStart + 1] == 108 && src[elseStart + 2] == 115 && src[elseStart + 3] == 101 && src[elseStart + 4] == 32 && src[elseStart + 5] == 105 && src[elseStart + 6] == 102 && src[elseStart + 7] == 32
                if isElseIf:
                    chainCond: usize = llvmNative3Expr(src, elseStart + 8, elseEndLine - 1, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
                    if chainCond == 0 || llvmNative3Code(ctx, chainCond) != 2:
                        return 2
                    chainThenLabel: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    nextChainLabel: usize = ctx.nextLabel
                    ctx.nextLabel = ctx.nextLabel + 1
                    emitRaw("  br i1 ")
                    llvmNative3EmitValue(src, ctx, chainCond)
                    emitRaw(", label ")
                    llvmNativeEmitLabelRef("if.chain.then", chainThenLabel)
                    emitRaw(", label ")
                    llvmNativeEmitLabelRef("if.chain", nextChainLabel)
                    emitRaw("\n")
                    llvmNativeEmitLabelDef("if.chain.then", chainThenLabel)
                    branchStatus: i32 = llvmNative3EmitBlock(src, nodes, chainNode + 1, bodyEnd, nodes[chainNode].level, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays, returnCode, returnBits, inLoop, loopContinueLabel, loopBreakLabel)
                    if branchStatus == 2:
                        return 2
                    branchTerminated: bool = branchStatus == 1
                    if branchTerminated && !llvmNativeBlockExplicitTerminator(src, nodes, chainNode + 1, bodyEnd, nodes[chainNode].level):
                        branchTerminated = false
                    if !branchTerminated:
                        emitRaw("  br label ")
                        llvmNativeEmitLabelRef("if.merge", labelId)
                        emitRaw("\n")
                    chainAllTerminated = chainAllTerminated && branchTerminated
                    hasNextChain: bool = bodyEnd < end && nodes[bodyEnd].level == nodes[i].level && nodes[bodyEnd].kind == 5
                    chainLabel = nextChainLabel
                    chainNode = bodyEnd
                    if !hasNextChain:
                        llvmNativeEmitLabelDef("if.chain", chainLabel)
                        emitRaw("  br label ")
                        llvmNativeEmitLabelRef("if.merge", labelId)
                        emitRaw("\n")
                        chainAllTerminated = false
                        break
                    continue
                else:
                    finalStatus: i32 = llvmNative3EmitBlock(src, nodes, chainNode + 1, bodyEnd, nodes[chainNode].level, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays, returnCode, returnBits, inLoop, loopContinueLabel, loopBreakLabel)
                    if finalStatus == 2:
                        return 2
                    finalTerminated: bool = finalStatus == 1
                    if finalTerminated && !llvmNativeBlockExplicitTerminator(src, nodes, chainNode + 1, bodyEnd, nodes[chainNode].level):
                        finalTerminated = false
                    if !finalTerminated:
                        emitRaw("  br label ")
                        llvmNativeEmitLabelRef("if.merge", labelId)
                        emitRaw("\n")
                    chainAllTerminated = chainAllTerminated && finalTerminated
                    chainNode = bodyEnd
                    break

            if !hasElse:
                emitRaw("  br label ")
                llvmNativeEmitLabelRef("if.merge", labelId)
                emitRaw("\n")
                chainAllTerminated = false
            if !chainAllTerminated:
                llvmNativeEmitLabelDef("if.merge", labelId)
            # Structured conditionals are conservatively treated as fallthrough.
            # A fully terminating branch set can still leave an unreachable merge
            # block, but never declaring the whole construct terminated prevents
            # nested `if` blocks from dropping the parent branch terminator.
            terminated = false
            i = chainNode
            continue

        if kind == 4:
            condStart: usize = s + 6
            condEnd: usize = t - 1
            labelId2: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            bodyEnd: usize = llvmNativeSubtreeEnd(nodes, i, end)
            emitRaw("  br label ")
            llvmNativeEmitLabelRef("while.cond", labelId2)
            emitRaw("\n")
            llvmNativeEmitLabelDef("while.cond", labelId2)
            cond: usize = llvmNative3Expr(src, condStart, condEnd, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
            if cond == 0 || llvmNative3Code(ctx, cond) != 2:
                return 2
            emitRaw("  br i1 ")
            llvmNative3EmitValue(src, ctx, cond)
            emitRaw(", label ")
            llvmNativeEmitLabelRef("while.body", labelId2)
            emitRaw(", label ")
            llvmNativeEmitLabelRef("while.end", labelId2)
            emitRaw("\n")
            llvmNativeEmitLabelDef("while.body", labelId2)
            bodyStatus: i32 = llvmNative3EmitBlock(src, nodes, i + 1, bodyEnd, nodes[i].level, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays, returnCode, returnBits, true, labelId2, labelId2)
            if bodyStatus == 2:
                return 2
            bodyTerminated: bool = bodyStatus == 1
            if !bodyTerminated:
                emitRaw("  br label ")
                llvmNativeEmitLabelRef("while.cond", labelId2)
                emitRaw("\n")
            llvmNativeEmitLabelDef("while.end", labelId2)
            i = bodyEnd
            continue
        if kind == 14:
            # for name in collection: lowers to an index loop over a dynamic/fixed array or View.
            # The iteration binding is a non-owning reference to the current element, matching the
            # production typechecker semantics.
            bodyEndFor: usize = llvmNativeSubtreeEnd(nodes, i, end)
            varStartFor: usize = s + 4
            while varStartFor < t && isSpace(src[varStartFor]):
                varStartFor = varStartFor + 1
            varEndFor: usize = varStartFor
            while varEndFor < t && isIdentContinue(src[varEndFor]):
                varEndFor = varEndFor + 1
            inPosFor: usize = varEndFor
            while inPosFor + 3 < t:
                if src[inPosFor] == 32 && src[inPosFor + 1] == 105 && src[inPosFor + 2] == 110 && src[inPosFor + 3] == 32:
                    break
                inPosFor = inPosFor + 1
            if inPosFor + 3 >= t || varEndFor <= varStartFor:
                return 2
            iterStartFor: usize = trimStart(src, inPosFor + 4, t - 1)
            iterEndFor: usize = trimEnd(src, iterStartFor, t - 1)
            iterableFor: usize = llvmNative3Expr(src, iterStartFor, iterEndFor, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
            if iterableFor == 0:
                return 2
            iterCodeFor: i32 = llvmNative3Code(ctx, iterableFor)
            elemCodeFor: i32 = 0
            elemBitsFor: i32 = 0
            elemSignedFor: bool = false
            baseAddrFor: usize = 0
            lenFor: usize = 0
            if iterCodeFor == 5:
                metaFor: &LLVMArrayInfo = &arrays[llvmNative3Bits(ctx, iterableFor) as usize]
                elemCodeFor = metaFor.elemCode
                elemBitsFor = metaFor.elemBits
                elemSignedFor = metaFor.elemSigned
                lenFor = ctx.nextLabel
                ctx.nextLabel = ctx.nextLabel + 1
                emitRaw("  %for.len")
                llvmNativeNumber(lenFor)
                emitRaw(" = extractvalue %LannerDynArray ")
                llvmNative3EmitValue(src, ctx, iterableFor)
                emitRaw(", 1\n")
                baseAddrFor = ctx.nextLabel
                ctx.nextLabel = ctx.nextLabel + 1
                emitRaw("  %for.base")
                llvmNativeNumber(baseAddrFor)
                emitRaw(" = extractvalue %LannerDynArray ")
                llvmNative3EmitValue(src, ctx, iterableFor)
                emitRaw(", 0\n")
            else if iterCodeFor == 11:
                metaFixedFor: &LLVMFixedArrayInfo = &fixedArrays[llvmNative3Bits(ctx, iterableFor) as usize]
                elemCodeFor = metaFixedFor.elemCode
                elemBitsFor = metaFixedFor.elemBits
                elemSignedFor = metaFixedFor.elemSigned
                lenFor = ctx.nextLabel
                ctx.nextLabel = ctx.nextLabel + 1
                emitRaw("  %for.len")
                llvmNativeNumber(lenFor)
                emitRaw(" = add i64 0, ")
                llvmNativeNumber(metaFixedFor.count)
                # Materialize a lanner address for the first-class fixed-array value.
                fixedAddrFor: usize = ctx.nextLabel
                ctx.nextLabel = ctx.nextLabel + 1
                emitRaw("  %for.fixed.addr")
                llvmNativeNumber(fixedAddrFor)
                emitRaw(" = alloca ")
                llvmEmitAllocaType(11, llvmNative3Bits(ctx, iterableFor), src, structs, arrays, fixedArrays)
                emitRaw("\n  store ")
                llvmEmitAllocaType(11, llvmNative3Bits(ctx, iterableFor), src, structs, arrays, fixedArrays)
                emitRaw(" ")
                llvmNative3EmitValue(src, ctx, iterableFor)
                emitRaw(", ptr %for.fixed.addr")
                llvmNativeNumber(fixedAddrFor)
                emitRaw("\n")
                baseAddrFor = fixedAddrFor
            else if iterCodeFor == 15 || iterCodeFor == 16:
                metaViewFor: &LLVMArrayInfo = &arrays[llvmNative3Bits(ctx, iterableFor) as usize]
                elemCodeFor = metaViewFor.elemCode
                elemBitsFor = metaViewFor.elemBits
                elemSignedFor = metaViewFor.elemSigned
                lenFor = ctx.nextLabel
                ctx.nextLabel = ctx.nextLabel + 1
                emitRaw("  %for.len")
                llvmNativeNumber(lenFor)
                emitRaw(" = extractvalue { ptr, i64 } ")
                llvmNative3EmitValue(src, ctx, iterableFor)
                emitRaw(", 1\n")
                baseAddrFor = ctx.nextLabel
                ctx.nextLabel = ctx.nextLabel + 1
                emitRaw("  %for.base")
                llvmNativeNumber(baseAddrFor)
                emitRaw(" = extractvalue { ptr, i64 } ")
                llvmNative3EmitValue(src, ctx, iterableFor)
                emitRaw(", 0\n")
            else:
                return 2
            # Scalar loop variables are materialized as typed locals. Aggregate
            # elements stay first-class references so no owning payload is copied.
            isScalarFor: bool = elemCodeFor == 1 || elemCodeFor == 2 || elemCodeFor == 3 || elemCodeFor == 17
            if !isScalarFor && elemCodeFor != 4 && elemCodeFor != 5 && elemCodeFor != 11:
                return 2
            refCodeFor: i32 = elemCodeFor
            if elemCodeFor == 4:
                refCodeFor = 7
            else if elemCodeFor == 5:
                refCodeFor = 6
            else if elemCodeFor == 11:
                refCodeFor = 12
            llvmOkFor: bool = llvmAddBinding(src, bindings, varStartFor, varEndFor, refCodeFor, elemBitsFor, elemSignedFor, false)
            if !llvmOkFor:
                return 2
            emitRaw("  ")
            emitLLVMSlotName(src, varStartFor, varEndFor)
            if isScalarFor:
                emitRaw(" = alloca ")
                llvmEmitAllocaType(elemCodeFor, elemBitsFor, src, structs, arrays, fixedArrays)
            else:
                emitRaw(" = alloca ptr")
            emitRaw("\n")
            idxFor: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            condFor: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            bodyFor: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            nextFor: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            endFor: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            emitRaw("  %for.idx")
            llvmNativeNumber(idxFor)
            emitRaw(" = alloca i64\n  store i64 0, ptr %for.idx")
            llvmNativeNumber(idxFor)
            emitRaw("\n  br label ")
            llvmNativeEmitLabelRef("while.cond", condFor)
            emitRaw("\n")
            llvmNativeEmitLabelDef("while.cond", condFor)
            idxLoadFor: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            emitRaw("  %for.idx.load")
            llvmNativeNumber(idxLoadFor)
            emitRaw(" = load i64, ptr %for.idx")
            llvmNativeNumber(idxFor)
            emitRaw("\n  %for.cond")
            llvmNativeNumber(ctx.nextLabel)
            condValueFor: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            emitRaw(" = icmp ult i64 %for.idx.load")
            llvmNativeNumber(idxLoadFor)
            emitRaw(", %for.len")
            llvmNativeNumber(lenFor)
            emitRaw("\n  br i1 %for.cond")
            llvmNativeNumber(condValueFor)
            emitRaw(", label ")
            llvmNativeEmitLabelRef("while.body", bodyFor)
            emitRaw(", label ")
            llvmNativeEmitLabelRef("while.end", endFor)
            emitRaw("\n")
            llvmNativeEmitLabelDef("while.body", bodyFor)
            elemAddrFor: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            emitRaw("  %for.elem.addr")
            llvmNativeNumber(elemAddrFor)
            emitRaw(" = getelementptr inbounds ")
            llvmEmitAllocaType(elemCodeFor, elemBitsFor, src, structs, arrays, fixedArrays)
            emitRaw(", ptr %for.base")
            llvmNativeNumber(baseAddrFor)
            if iterCodeFor == 11:
                emitRaw(", i64 0, i64 %for.idx.load")
                llvmNativeNumber(idxLoadFor)
            else:
                emitRaw(", i64 %for.idx.load")
                llvmNativeNumber(idxLoadFor)
            if isScalarFor:
                loadFor: usize = ctx.nextLabel
                ctx.nextLabel = ctx.nextLabel + 1
                emitRaw("  %for.elem")
                llvmNativeNumber(loadFor)
                emitRaw(" = load ")
                llvmEmitAllocaType(elemCodeFor, elemBitsFor, src, structs, arrays, fixedArrays)
                emitRaw(", ptr %for.elem.addr")
                llvmNativeNumber(elemAddrFor)
                emitRaw("\n  store ")
                llvmEmitAllocaType(elemCodeFor, elemBitsFor, src, structs, arrays, fixedArrays)
                emitRaw(" %for.elem")
                llvmNativeNumber(loadFor)
                emitRaw(", ptr ")
                emitLLVMSlotName(src, varStartFor, varEndFor)
                emitRaw("\n")
            else:
                emitRaw("\n  store ptr %for.elem.addr")
                llvmNativeNumber(elemAddrFor)
                emitRaw(", ptr ")
                emitLLVMSlotName(src, varStartFor, varEndFor)
                emitRaw("\n")
            bodyStatusFor: i32 = llvmNative3EmitBlock(src, nodes, i + 1, bodyEndFor, nodes[i].level, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays, returnCode, returnBits, true, nextFor, endFor)
            if bodyStatusFor == 2:
                return 2
            if bodyStatusFor != 1:
                emitRaw("  br label ")
                llvmNativeEmitLabelRef("while.next", nextFor)
                emitRaw("\n")
            llvmNativeEmitLabelDef("while.next", nextFor)
            nextIdxFor: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            emitRaw("  %for.next.idx")
            llvmNativeNumber(nextIdxFor)
            emitRaw(" = add i64 %for.idx.load")
            llvmNativeNumber(idxLoadFor)
            emitRaw(", 1\n  store i64 %for.next.idx")
            llvmNativeNumber(nextIdxFor)
            emitRaw(", ptr %for.idx")
            llvmNativeNumber(idxFor)
            emitRaw("\n  br label ")
            llvmNativeEmitLabelRef("while.cond", condFor)
            emitRaw("\n")
            llvmNativeEmitLabelDef("while.end", endFor)
            i = bodyEndFor
            continue
        if kind == 15:
            # Compile-time declarations are erased after their value has been
            # registered in the source-level lookup table used by expression
            # lowering. They intentionally emit no runtime instructions.
            i = i + 1
            continue
        if kind == 12:
            matchKindValue: i64 = matchKind(src, s, t)
            if matchKindValue == 0:
                return 2
            arrowGuard: usize = 0
            jGuard: usize = s
            while jGuard + 1 < t:
                if src[jGuard] == 45 && src[jGuard + 1] == 62:
                    arrowGuard = jGuard
                    break
                jGuard = jGuard + 1
            if arrowGuard == 0:
                return 2
            matchMarker: usize = s
            while matchMarker + 3 < arrowGuard:
                if src[matchMarker] == 32 && src[matchMarker + 1] == 105 && src[matchMarker + 2] == 115 && src[matchMarker + 3] == 32:
                    break
                matchMarker = matchMarker + 1
            if matchMarker + 3 >= arrowGuard:
                return 2
            condStartGuard: usize = trimStart(src, s, matchMarker)
            condEndGuard: usize = trimEnd(src, condStartGuard, matchMarker)
            resultGuard: usize = llvmNative3Expr(src, condStartGuard, condEndGuard, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
            if resultGuard == 0 || llvmNative3Code(ctx, resultGuard) != 14:
                return 2
            resultBitsGuard: usize = llvmNative3Bits(ctx, resultGuard) as usize
            if resultBitsGuard + 1 >= fixedArrays.len():
                return 2
            resultOkCodeGuard: i32 = fixedArrays[resultBitsGuard].elemCode
            resultOkBitsGuard: i32 = fixedArrays[resultBitsGuard].elemBits
            resultOkSignedGuard: bool = fixedArrays[resultBitsGuard].elemSigned
            resultErrCodeGuard: i32 = fixedArrays[resultBitsGuard + 1].elemCode
            resultErrBitsGuard: i32 = fixedArrays[resultBitsGuard + 1].elemBits
            resultErrSignedGuard: bool = fixedArrays[resultBitsGuard + 1].elemSigned
            tagGuard: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            emitRaw("  %guard.tag")
            llvmNativeNumber(tagGuard)
            emitRaw(" = extractvalue ")
            llvmEmitAllocaType(14, resultBitsGuard as i32, src, structs, arrays, fixedArrays)
            emitRaw(" ")
            llvmNative3EmitValue(src, ctx, resultGuard)
            emitRaw(", 0\n")
            notGuard: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            if matchKindValue == 2:
                emitRaw("  %guard.not")
                llvmNativeNumber(notGuard)
                emitRaw(" = xor i1 1, %guard.tag")
                llvmNativeNumber(tagGuard)
                emitRaw("\n")
            emitRaw("  br i1 ")
            if matchKindValue == 1:
                emitRaw("%guard.tag")
                llvmNativeNumber(tagGuard)
            else:
                emitRaw("%guard.not")
                llvmNativeNumber(notGuard)
            emitRaw(", label ")
            branchGuard: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            llvmNativeEmitLabelRef("guard.then", branchGuard)
            emitRaw(", label ")
            endGuard: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            llvmNativeEmitLabelRef("guard.end", endGuard)
            emitRaw("\n")
            llvmNativeEmitLabelDef("guard.then", branchGuard)
            bindMarkerGuard: usize = matchMarker + 4
            openBindGuard: usize = 0
            while bindMarkerGuard < arrowGuard:
                if src[bindMarkerGuard] == 40:
                    openBindGuard = bindMarkerGuard
                    break
                bindMarkerGuard = bindMarkerGuard + 1
            if openBindGuard == 0:
                return 2
            bindNameStartGuard: usize = openBindGuard + 1
            bindNameEndGuard: usize = bindNameStartGuard
            while bindNameEndGuard < arrowGuard && src[bindNameEndGuard] != 41:
                bindNameEndGuard = bindNameEndGuard + 1
            if bindNameEndGuard >= arrowGuard:
                return 2
            bindNameStartGuard = trimStart(src, bindNameStartGuard, bindNameEndGuard)
            bindNameEndGuard = trimEnd(src, bindNameStartGuard, bindNameEndGuard)
            payloadIndexGuard: usize = 1
            payloadCodeGuard: i32 = resultOkCodeGuard
            payloadBitsGuard: i32 = resultOkBitsGuard
            payloadSignedGuard: bool = resultOkSignedGuard
            if matchKindValue == 2:
                payloadIndexGuard = 2
                payloadCodeGuard = resultErrCodeGuard
                payloadBitsGuard = resultErrBitsGuard
                payloadSignedGuard = resultErrSignedGuard
            bodyStartGuard: usize = trimStart(src, arrowGuard + 2, t)
            bodyEndGuard: usize = trimEnd(src, bodyStartGuard, t)
            if bodyStartGuard + 6 > bodyEndGuard || !startsWord(src, bodyStartGuard, bodyEndGuard, 5):
                return 2
            returnValueStartGuard: usize = trimStart(src, bodyStartGuard + 6, bodyEndGuard)
            if returnValueStartGuard >= bodyEndGuard:
                return 2
            # Keep the matched Result alive in a temporary slot. This also makes
            # non-copy payload guards safe to borrow through the payload field.
            resultSlotGuard: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            emitRaw("  %guard.result.slot")
            llvmNativeNumber(resultSlotGuard)
            emitRaw(" = alloca ")
            llvmEmitAllocaType(14, resultBitsGuard as i32, src, structs, arrays, fixedArrays)
            emitRaw("\n  store ")
            llvmEmitAllocaType(14, resultBitsGuard as i32, src, structs, arrays, fixedArrays)
            emitRaw(" ")
            llvmNative3EmitValue(src, ctx, resultGuard)
            emitRaw(", ptr %guard.result.slot")
            llvmNativeNumber(resultSlotGuard)
            emitRaw("\n")
            payloadAddrGuard: usize = ctx.nextLabel
            ctx.nextLabel = ctx.nextLabel + 1
            emitRaw("  %guard.payload.addr")
            llvmNativeNumber(payloadAddrGuard)
            emitRaw(" = getelementptr inbounds ")
            llvmEmitAllocaType(14, resultBitsGuard as i32, src, structs, arrays, fixedArrays)
            emitRaw(", ptr %guard.result.slot")
            llvmNativeNumber(resultSlotGuard)
            emitRaw(", i64 0, i32 ")
            llvmNativeNumber(payloadIndexGuard)
            emitRaw("\n")
            guardBindings: []LLVMBinding = []LLVMBinding
            copyGuardBinding: usize = 0
            while copyGuardBinding < bindings.len():
                oldGuardBinding: &LLVMBinding = &bindings[copyGuardBinding]
                guardBindings.push(LLVMBinding[nameStart: oldGuardBinding.nameStart, nameEnd: oldGuardBinding.nameEnd, typeCode: oldGuardBinding.typeCode, bits: oldGuardBinding.bits, isSigned: oldGuardBinding.isSigned, owning: oldGuardBinding.owning, initialized: oldGuardBinding.initialized, moved: oldGuardBinding.moved])
                copyGuardBinding = copyGuardBinding + 1
            isCopyGuard: bool = payloadCodeGuard == 1 || payloadCodeGuard == 2 || payloadCodeGuard == 3 || payloadCodeGuard == 17
            if isCopyGuard:
                emitRaw("  ")
                emitLLVMSlotName(src, bindNameStartGuard, bindNameEndGuard)
                emitRaw(" = alloca ")
                llvmEmitAllocaType(payloadCodeGuard, payloadBitsGuard, src, structs, arrays, fixedArrays)
                emitRaw("\n")
                extractGuard: usize = llvmNative3New(ctx, payloadCodeGuard, payloadBitsGuard, payloadSignedGuard, false, 0, 0)
                emitRaw("%t")
                llvmNativeNumber(extractGuard)
                emitRaw(" = extractvalue ")
                llvmEmitAllocaType(14, resultBitsGuard as i32, src, structs, arrays, fixedArrays)
                emitRaw(" ")
                llvmNative3EmitValue(src, ctx, resultGuard)
                emitRaw(", ")
                llvmNativeNumber(payloadIndexGuard)
                emitRaw("\n  store ")
                llvmEmitAllocaType(payloadCodeGuard, payloadBitsGuard, src, structs, arrays, fixedArrays)
                emitRaw(" ")
                llvmNative3EmitValue(src, ctx, extractGuard)
                emitRaw(", ptr ")
                emitLLVMSlotName(src, bindNameStartGuard, bindNameEndGuard)
                emitRaw("\n")
                guardPayloadOwns: bool = llvmNative3TypeOwnsResources(payloadCodeGuard, payloadBitsGuard, structs, fields, arrays, fixedArrays)
                guardBindings.push(LLVMBinding[nameStart: bindNameStartGuard, nameEnd: bindNameEndGuard, typeCode: payloadCodeGuard, bits: payloadBitsGuard, isSigned: payloadSignedGuard, owning: guardPayloadOwns, initialized: true, moved: false])
                returnValueGuard: usize = llvmNative3ExprAsType(src, returnValueStartGuard, bodyEndGuard, returnCode, returnBits, ctx, &mut guardBindings, funcs, params, structs, fields, arrays, fixedArrays)
                if returnValueGuard == 0 || llvmNative3Code(ctx, returnValueGuard) == 0:
                    return 2
                if llvmNative3Code(ctx, returnValueGuard) == 1 && returnCode == 1 && !llvmNative3IsImmediate(ctx, returnValueGuard) && llvmNative3Bits(ctx, returnValueGuard) != returnBits:
                    return 2
                # The guard body compiled against a copy of the outer binding ledger.
                # Propagate ownership transfers back to the real ledger before cleanup.
                llvmNative3MergeGuardMoves(src, bindings, &guardBindings)
                llvmNative3DropAllBindings(src, bindings)
                emitRaw("  ret ")
                llvmEmitAllocaType(returnCode, returnBits, src, structs, arrays, fixedArrays)
                emitRaw(" ")
                llvmNative3EmitValue(src, ctx, returnValueGuard)
                emitRaw("\n")
            else:
                refCodeGuard: i32 = 6
                if payloadCodeGuard == 4:
                    refCodeGuard = 7
                else if payloadCodeGuard == 11:
                    refCodeGuard = 12
                else if payloadCodeGuard == 5:
                    refCodeGuard = 6
                else:
                    return 2
                emitRaw("  ")
                emitLLVMSlotName(src, bindNameStartGuard, bindNameEndGuard)
                emitRaw(" = alloca ptr\n  store ptr %guard.payload.addr")
                llvmNativeNumber(payloadAddrGuard)
                emitRaw(", ptr ")
                emitLLVMSlotName(src, bindNameStartGuard, bindNameEndGuard)
                emitRaw("\n")
                guardBindings.push(LLVMBinding[nameStart: bindNameStartGuard, nameEnd: bindNameEndGuard, typeCode: refCodeGuard, bits: payloadBitsGuard, isSigned: false, owning: false, initialized: true, moved: false])
                returnValueGuard2: usize = llvmNative3ExprAsType(src, returnValueStartGuard, bodyEndGuard, returnCode, returnBits, ctx, &mut guardBindings, funcs, params, structs, fields, arrays, fixedArrays)
                if returnValueGuard2 == 0:
                    return 2
                llvmNative3MergeGuardMoves(src, bindings, &guardBindings)
                llvmNative3DropAllBindings(src, bindings)
                emitRaw("  ret ")
                llvmEmitAllocaType(returnCode, returnBits, src, structs, arrays, fixedArrays)
                emitRaw(" ")
                llvmNative3EmitValue(src, ctx, returnValueGuard2)
                emitRaw("\n")
            llvmNativeEmitLabelDef("guard.end", endGuard)
            # The match-guard's arrow body may terminate the true branch, but
            # the false branch continues with the following source statement.
            terminated = false
            i = i + 1
            continue
        if kind == 5:
            return 2
        if kind == 7:
            if inLoop:
                emitRaw("  br label ")
                llvmNativeEmitLabelRef("while.end", loopBreakLabel)
                emitRaw("\n")
                return 1
            emitRaw("  unreachable\n")
            return 1
        if kind == 8:
            if inLoop:
                emitRaw("  br label ")
                llvmNativeEmitLabelRef("while.cond", loopContinueLabel)
                emitRaw("\n")
                return 1
            emitRaw("  unreachable\n")
            return 1
        if kind == 6:
            emitRaw(";DBG_RETURN_BEGIN\n")
            if t <= s + 6:
                emitRaw("  ret void\n")
            else:
                value2: usize = llvmNative3ExprAsType(src, s + 6, t, returnCode, returnBits, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
                emitRaw(";DBG_RETURN_AFTER_EXPR\n")
                if value2 == 0 || llvmNative3Code(ctx, value2) == 0:
                    return 2
                if llvmNative3Code(ctx, value2) == 1 && returnCode == 1 && llvmNative3Bits(ctx, value2) != returnBits:
                    retInfo: LLVMTypeInfo = LLVMTypeInfo[code: 1, bits: returnBits, isSigned: true]
                    value2 = llvmNative3Cast(src, ctx, value2, &retInfo)
                    if value2 == 0:
                        return 2
                if llvmNative3Code(ctx, value2) != returnCode:
                    return 2
                if returnCode == 1 && llvmNative3Bits(ctx, value2) != returnBits:
                    return 2
                if (returnCode == 4 || returnCode == 5 || returnCode == 11 || returnCode == 13 || returnCode == 14 || returnCode == 18) && returnCode != 18 && llvmNative3Bits(ctx, value2) != returnBits:
                    return 2
                returnSimpleS: usize = trimStart(src, s + 6, t)
                returnSimpleE: usize = trimEnd(src, returnSimpleS, t)
                if llvmNative3TypeOwnsResources(returnCode, returnBits, structs, fields, arrays, fixedArrays):
                    if !llvmBindingMarkMoved(src, bindings, returnSimpleS, returnSimpleE):
                        return 2
                llvmNative3DropAllBindings(src, bindings)
                emitRaw("  ret ")
                llvmEmitAllocaType(returnCode, returnBits, src, structs, arrays, fixedArrays)
                emitRaw(" ")
                llvmNative3EmitValue(src, ctx, value2)
                emitRaw("\n")
            terminated = true
            i = i + 1
            continue
        if kind == 2:
            colon: []i64 = []i64
            eq: []i64 = []i64
            parseDecl: bool = llvmParseDeclaredName(src, s, t, &mut colon, &mut eq)
            if parseDecl:
                nameStart: usize = s
                nameEnd: usize = colon[0] as usize
                if t - s >= 6:
                    if src[s] == 99:
                        if src[s + 1] == 111:
                            if src[s + 2] == 110:
                                if src[s + 3] == 115:
                                    if src[s + 4] == 116:
                                        if src[s + 5] == 32:
                                            nameStart = s + 6
                                            while nameStart < nameEnd && isSpace(src[nameStart]):
                                                nameStart = nameStart + 1
                declaredStart: usize = trimStart(src, colon[0] as usize + 1, eq[0] as usize)
                info: LLVMTypeInfo = llvmResolveType(src, declaredStart, eq[0] as usize, structs, arrays, fixedArrays)
                if info.code != 1 && info.code != 2 && info.code != 3 && info.code != 4 && info.code != 5 && info.code != 6 && info.code != 7 && info.code != 9 && info.code != 11 && info.code != 12 && info.code != 13 && info.code != 14 && info.code != 15 && info.code != 16 && info.code != 17 && info.code != 18:
                    return 2
                existing: i64 = llvmLookupBinding(src, bindings, nameStart, nameEnd)
                if existing < 0:
                    if !llvmAddBinding(src, bindings, nameStart, nameEnd, info.code, info.bits, info.isSigned, llvmNative3TypeOwnsResources(info.code, info.bits, structs, fields, arrays, fixedArrays)):
                        return 2
                    emitRaw("  ")
                    emitLLVMSlotName(src, nameStart, nameEnd)
                    emitRaw(" = alloca ")
                    llvmEmitAllocaType(info.code, info.bits, src, structs, arrays, fixedArrays)
                    emitRaw("\n")
                    existing = llvmLookupBinding(src, bindings, nameStart, nameEnd)
                value3: usize = llvmNative3ExprAsType(src, eq[0] as usize + 1, t, info.code, info.bits, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
                if existing < 0:
                    return 2
                value3 = llvmNative3Cast(src, ctx, value3, &info)
                if value3 == 0 || !llvmNative3EmitStore(src, ctx, &bindings[existing as usize], value3, structs, arrays, fixedArrays):
                    return 2
                bindings[existing as usize].initialized = true
                bindings[existing as usize].moved = false
                if llvmNative3TypeOwnsResources(info.code, info.bits, structs, fields, arrays, fixedArrays):
                    rhsDeclS: usize = trimStart(src, eq[0] as usize + 1, t)
                    rhsDeclE: usize = trimEnd(src, rhsDeclS, t)
                    if !llvmBindingMarkMoved(src, bindings, rhsDeclS, rhsDeclE):
                        return 2
            else:
                assignEq: usize = llvmNative3FindAssignEq(src, s, t)
                if assignEq == 0:
                    return 2
                lhsStart: usize = trimStart(src, s, assignEq)
                lhsEnd: usize = trimEnd(src, lhsStart, assignEq)
                lvalueDot: usize = llvmFindTrailingDot(src, lhsStart, lhsEnd)
                lvalueIndex: usize = llvmFindTrailingIndex(src, lhsStart, lhsEnd)
                if lvalueDot > 0 || lvalueIndex > 0:
                    if !llvmNative3EmitLValueStore(src, lhsStart, lhsEnd, assignEq + 1, t, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays):
                        return 2
                    i = i + 1
                    continue
                target: i64 = llvmLookupBinding(src, bindings, lhsStart, lhsEnd)
                if target < 0:
                    # Not a known local yet -- treat as a fresh inferred-type
                    # declaration rather than failing outright: evaluate the
                    # RHS first (its result carries the type), THEN alloca+
                    # store. Order is safe here because this backend doesn't
                    # require allocas hoisted to the entry block.
                    valueInf: usize = llvmNative3Expr(src, assignEq + 1, t, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
                    if valueInf == 0:
                        return 2
                    infCode: i32 = llvmNative3Code(ctx, valueInf)
                    if infCode == 0:
                        return 2
                    infBits: i32 = llvmNative3Bits(ctx, valueInf)
                    infSigned: bool = llvmNative3Signed(ctx, valueInf)
                    if !llvmAddBinding(src, bindings, lhsStart, lhsEnd, infCode, infBits, infSigned, llvmNative3TypeOwnsResources(infCode, infBits, structs, fields, arrays, fixedArrays)):
                        return 2
                    emitRaw("  ")
                    emitLLVMSlotName(src, lhsStart, lhsEnd)
                    emitRaw(" = alloca ")
                    llvmEmitAllocaType(infCode, infBits, src, structs, arrays, fixedArrays)
                    emitRaw("\n")
                    newTarget: i64 = llvmLookupBinding(src, bindings, lhsStart, lhsEnd)
                    if newTarget < 0 || !llvmNative3EmitStore(src, ctx, &bindings[newTarget as usize], valueInf, structs, arrays, fixedArrays):
                        return 2
                    bindings[newTarget as usize].initialized = true
                    bindings[newTarget as usize].moved = false
                    if llvmNative3TypeOwnsResources(infCode, infBits, structs, fields, arrays, fixedArrays):
                        rhsInfS: usize = trimStart(src, assignEq + 1, t)
                        rhsInfE: usize = trimEnd(src, rhsInfS, t)
                        if !llvmBindingMarkMoved(src, bindings, rhsInfS, rhsInfE):
                            return 2
                else:
                    rhsSelfS: usize = trimStart(src, assignEq + 1, t)
                    rhsSelfE: usize = trimEnd(src, rhsSelfS, t)
                    rhsSelfBind: i64 = llvmLookupBinding(src, bindings, rhsSelfS, rhsSelfE)
                    if rhsSelfBind == target:
                        i = i + 1
                        continue
                    value4: usize = llvmNative3Expr(src, assignEq + 1, t, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
                    if value4 == 0:
                        return 2
                    if bindings[target as usize].owning && bindings[target as usize].initialized && !bindings[target as usize].moved:
                        llvmNative3DropBinding(src, bindings, target as usize)
                    if !llvmNative3EmitStore(src, ctx, &bindings[target as usize], value4, structs, arrays, fixedArrays):
                        return 2
                    bindings[target as usize].initialized = true
                    bindings[target as usize].moved = false
                    rhsAssignS: usize = trimStart(src, assignEq + 1, t)
                    rhsAssignE: usize = trimEnd(src, rhsAssignS, t)
                    if bindings[target as usize].owning:
                        if !llvmBindingMarkMoved(src, bindings, rhsAssignS, rhsAssignE):
                            return 2
            i = i + 1
            continue
        if kind == 9:
            exprResult: usize = llvmNative3Expr(src, s, t, ctx, bindings, funcs, params, structs, fields, arrays, fixedArrays)
            if exprResult == 0:
                return 2
            i = i + 1
            continue
        return 2
    if terminated:
        return 1
    return 0

llvmNative3HoistLocals(src: &[]u8, nodes: &[]AstNode, start: usize, end: usize, functionLevel: usize, bindings: &mut []LLVMBinding, structs: &[]LLVMStructInfo, fields: &[]LLVMStructFieldInfo, arrays: &mut []LLVMArrayInfo, fixedArrays: &mut []LLVMFixedArrayInfo) bool:
    i: usize = start
    while i < end:
        if nodes[i].level > functionLevel && nodes[i].kind == 2:
            s: usize = trimStart(src, nodes[i].start, nodes[i].end)
            t: usize = trimEnd(src, s, nodes[i].end)
            colon: []i64 = []i64
            eq: []i64 = []i64
            if llvmParseDeclaredName(src, s, t, &mut colon, &mut eq):
                nameStart: usize = s
                nameEnd: usize = colon[0] as usize
                if t - s >= 6 && src[s] == 99 && src[s + 1] == 111 && src[s + 2] == 110 && src[s + 3] == 115 && src[s + 4] == 116 && src[s + 5] == 32:
                    nameStart = s + 6
                    while nameStart < nameEnd && isSpace(src[nameStart]):
                        nameStart = nameStart + 1
                declaredStart: usize = trimStart(src, colon[0] as usize + 1, eq[0] as usize)
                info: LLVMTypeInfo = llvmResolveType(src, declaredStart, eq[0] as usize, structs, arrays, fixedArrays)
                if info.code == 0:
                    return false
                if info.code != 1 && info.code != 2 && info.code != 3 && info.code != 4 && info.code != 5 && info.code != 6 && info.code != 7 && info.code != 9 && info.code != 11 && info.code != 12 && info.code != 13 && info.code != 14 && info.code != 15 && info.code != 16 && info.code != 17 && info.code != 18:
                    return false
                if llvmLookupBinding(src, bindings, nameStart, nameEnd) < 0:
                    if !llvmAddBinding(src, bindings, nameStart, nameEnd, info.code, info.bits, info.isSigned, llvmNative3TypeOwnsResources(info.code, info.bits, structs, fields, arrays, fixedArrays)):
                        return false
        i = i + 1
    return true

llvmNative3EmitFunction(src: &[]u8, nodes: &[]AstNode, fnNode: usize, funcs: &[]LLVMFunctionSig, params: &[]LLVMParamInfo, structs: &[]LLVMStructInfo, fields: &[]LLVMStructFieldInfo, arrays: &mut []LLVMArrayInfo, fixedArrays: &mut []LLVMFixedArrayInfo) bool:
    s: usize = trimStart(src, nodes[fnNode].start, nodes[fnNode].end)
    t: usize = trimEnd(src, s, nodes[fnNode].end)
    open: usize = s
    while open < t && src[open] != 40:
        open = open + 1
    fnIndex: i64 = llvmLookupFunction(src, funcs, s, open)
    if fnIndex < 0:
        return false
    sig: &LLVMFunctionSig = &funcs[fnIndex as usize]
    emitRaw(";BEGIN_NATIVE_FUNC ")
    emitRawRange(src, s, open)
    emitRaw("\n")
    emitRaw("define ")
    if sig.returnCode == 0:
        emitRaw("void")
    else:
        llvmEmitAllocaType(sig.returnCode, sig.returnBits, src, structs, arrays, fixedArrays)
    emitRaw(" @")
    emitRawRange(src, s, open)
    emitRaw("(")
    pIndex: usize = sig.paramStart
    count: usize = 0
    while count < sig.paramCount:
        if count > 0:
            emitRaw(", ")
        p: &LLVMParamInfo = &params[pIndex + count]
        llvmEmitAllocaType(p.typeCode, p.bits, src, structs, arrays, fixedArrays)
        emitRaw(" %")
        emitRawRange(src, p.nameStart, p.nameEnd)
        count = count + 1
    emitRaw(") {\nentry:\n")
    ctx: LLVMNativeContext = LLVMNativeContext[nextValue: 1, nextLabel: 0, values: []i64, comptimes: []LLVMComptimeDecl]
    llvmNative3Init(&mut ctx, src)
    bindings: []LLVMBinding = []LLVMBinding
    count = 0
    while count < sig.paramCount:
        p2: &LLVMParamInfo = &params[pIndex + count]
        paramOwnsResources: bool = llvmNative3TypeOwnsResources(p2.typeCode, p2.bits, structs, fields, arrays, fixedArrays)
        bindings.push(LLVMBinding[nameStart: p2.nameStart, nameEnd: p2.nameEnd, typeCode: p2.typeCode, bits: p2.bits, isSigned: p2.isSigned, owning: paramOwnsResources, initialized: true, moved: false])
        emitRaw("  ")
        emitLLVMSlotName(src, p2.nameStart, p2.nameEnd)
        emitRaw(" = alloca ")
        llvmEmitAllocaType(p2.typeCode, p2.bits, src, structs, arrays, fixedArrays)
        emitRaw("\n  store ")
        llvmEmitAllocaType(p2.typeCode, p2.bits, src, structs, arrays, fixedArrays)
        emitRaw(" %")
        emitRawRange(src, p2.nameStart, p2.nameEnd)
        emitRaw(", ptr ")
        emitLLVMSlotName(src, p2.nameStart, p2.nameEnd)
        emitRaw("\n")
        count = count + 1
    nextNode: usize = llvmNativeSubtreeEnd(nodes, fnNode, nodes.len())
    if !llvmNative3HoistLocals(src, nodes, fnNode + 1, nextNode, nodes[fnNode].level, &mut bindings, structs, fields, arrays, fixedArrays):
        return false
    count = sig.paramCount
    while count < bindings.len():
        localBinding: &mut LLVMBinding = &mut bindings[count]
        emitRaw("  ")
        emitLLVMSlotName(src, localBinding.nameStart, localBinding.nameEnd)
        emitRaw(" = alloca ")
        llvmEmitAllocaType(localBinding.typeCode, localBinding.bits, src, structs, arrays, fixedArrays)
        emitRaw("\n")
        if localBinding.owning:
            emitRaw("  store ")
            llvmEmitAllocaType(localBinding.typeCode, localBinding.bits, src, structs, arrays, fixedArrays)
            emitRaw(" zeroinitializer, ptr ")
            emitLLVMSlotName(src, localBinding.nameStart, localBinding.nameEnd)
            emitRaw("\n")
            localBinding.initialized = true
            localBinding.moved = false
        count = count + 1
    blockStatus: i32 = llvmNative3EmitBlock(src, nodes, fnNode + 1, nextNode, nodes[fnNode].level, &mut ctx, &mut bindings, funcs, params, structs, fields, arrays, fixedArrays, sig.returnCode, sig.returnBits, false, 0, 0)
    if blockStatus == 2:
        return false
    terminated: bool = blockStatus == 1
    if !terminated:
        llvmNative3DropAllBindings(src, &mut bindings)
        if sig.returnCode == 0:
            emitRaw("  ret void\n")
        else if sig.returnCode == 1:
            emitRaw("  ret ")
            emitLLVMIntType(sig.returnBits)
            emitRaw(" 0\n")
        else if sig.returnCode == 2:
            emitRaw("  ret i1 0\n")
        else:
            emitRaw("  ret ")
            llvmEmitAllocaType(sig.returnCode, sig.returnBits, src, structs, arrays, fixedArrays)
            emitRaw(" zeroinitializer\n")
    emitRaw("}\n\n")
    return true

emitNativeLLVM3(source: &[]u8, nodes: &[]AstNode, typedParams: &[]TypedParamInfo, typedFields: &[]TypedFieldInfo) bool:
    structs: []LLVMStructInfo = []LLVMStructInfo
    fields: []LLVMStructFieldInfo = []LLVMStructFieldInfo
    arrays: []LLVMArrayInfo = []LLVMArrayInfo
    fixedArrays: []LLVMFixedArrayInfo = []LLVMFixedArrayInfo
    if !llvmBuildTypedStructRegistry(source, nodes, typedFields, &mut structs, &mut fields, &mut arrays, &mut fixedArrays):
        return false
    funcs: []LLVMFunctionSig = []LLVMFunctionSig
    params: []LLVMParamInfo = []LLVMParamInfo
    if !llvmBuildTypedFunctionSigs(source, nodes, typedParams, &mut funcs, &mut params, &structs, &mut arrays, &mut fixedArrays):
        return false
    i: usize = 0
    while i < funcs.len():
        if funcs[i].paramStart + funcs[i].paramCount > params.len():
            return false
        i = i + 1
    mainIndex: i64 = (0 as i64) - 1
    i = 0
    while i < funcs.len():
        if funcs[i].nameEnd - funcs[i].nameStart == 4 && source[funcs[i].nameStart] == 109 && source[funcs[i].nameStart + 1] == 97 && source[funcs[i].nameStart + 2] == 105 && source[funcs[i].nameStart + 3] == 110:
            mainIndex = i as i64
        i = i + 1
    if mainIndex < 0:
        return false
    emitRaw("; Lanner native LLVM replacement layer\n")
    emitRaw("source_filename = \"lanner-selfhost\"\n\n")
    emitRaw("declare void @llvm.trap()\n")
    emitRaw("declare ptr @realloc(ptr, i64)\n")
    emitRaw("declare ptr @malloc(i64)\n")
    emitRaw("declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1)\n")
    emitRaw("declare { i64, i1 } @llvm.umul.with.overflow.i64(i64, i64)\n")
    emitRaw("declare void @free(ptr)\n")
    emitRaw("declare ptr @fopen(ptr, ptr)\n")
    emitRaw("declare i32 @fseek(ptr, i64, i32)\n")
    emitRaw("declare i64 @ftell(ptr)\n")
    emitRaw("declare i64 @fread(ptr, i64, i64, ptr)\n")
    emitRaw("declare i32 @fclose(ptr)\n")
    emitRaw("declare ptr @getenv(ptr)\n")
    emitRaw("declare i64 @strlen(ptr)\n")
    emitRaw("declare i64 @fwrite(ptr, i64, i64, ptr)\n")
    emitRaw("@stdout = external global ptr\n")
    emitRaw("declare i32 @putchar(i32)\n")
    emitRaw("declare i32 @printf(ptr, ...)\n")
    emitRaw("@.lanner.rb = private unnamed_addr constant [3 x i8] c\"rb\\00\"\n")
    emitRaw("@.lanner.print_i64 = private unnamed_addr constant [5 x i8] c\"%ld\\0A\\00\"\n")
    emitRaw("@.lanner.print_f64 = private unnamed_addr constant [4 x i8] c\"%g\\0A\\00\"\n")
    emitRaw("@.lanner.print_str = private unnamed_addr constant [4 x i8] c\"%s\\0A\\00\"\n")
    emitRaw("@.lanner.print_bool = private unnamed_addr constant [4 x i8] c\"%s\\0A\\00\"\n")
    emitRaw("@.lanner.true = private unnamed_addr constant [5 x i8] c\"true\\00\"\n")
    emitRaw("@.lanner.false = private unnamed_addr constant [6 x i8] c\"false\\00\"\n")
    emitRaw("@.lanner.empty = private unnamed_addr constant [1 x i8] c\"\\00\"\n")
    emitRaw("\n")
    emitNativeStringGlobals(source)
    llvmEmitStructTypeDecls(source, &structs, &fields, &mut arrays, &mut fixedArrays)
    llvmEmitDynArrayTypeDecl()
    emitRaw("%LannerArena = type { ptr, ptr, i64, i64 }\n")
    emitRaw("%LannerArenaNode = type { ptr, i8 }\n\n")
    emitRaw("define internal ptr @__lanner_arena_alloc(ptr %arena, i64 %bytes) {\n")
    emitRaw("entry:\n")
    emitRaw("  %rem.addr = getelementptr inbounds %LannerArena, ptr %arena, i32 0, i32 2\n")
    emitRaw("  %cur.addr = getelementptr inbounds %LannerArena, ptr %arena, i32 0, i32 1\n")
    emitRaw("  %head.addr = getelementptr inbounds %LannerArena, ptr %arena, i32 0, i32 0\n")
    emitRaw("  %chunk.addr = getelementptr inbounds %LannerArena, ptr %arena, i32 0, i32 3\n")
    emitRaw("  %remaining = load i64, ptr %rem.addr\n")
    emitRaw("  %current = load ptr, ptr %cur.addr\n")
    emitRaw("  %sum = add i64 %bytes, 7\n")
    emitRaw("  %ov = icmp ult i64 %sum, %bytes\n")
    emitRaw("  br i1 %ov, label %oom, label %aligned\n")
    emitRaw("aligned:\n")
    emitRaw("  %alloc = and i64 %sum, -8\n")
    emitRaw("  %has = icmp ne ptr %current, null\n")
    emitRaw("  br i1 %has, label %check, label %newchunk\n")
    emitRaw("check:\n")
    emitRaw("  %fits = icmp ule i64 %alloc, %remaining\n")
    emitRaw("  br i1 %fits, label %bump, label %newchunk\n")
    emitRaw("bump:\n")
    emitRaw("  %next = getelementptr i8, ptr %current, i64 %alloc\n")
    emitRaw("  %left = sub i64 %remaining, %alloc\n")
    emitRaw("  store ptr %next, ptr %cur.addr\n")
    emitRaw("  store i64 %left, ptr %rem.addr\n")
    emitRaw("  ret ptr %current\n")
    emitRaw("newchunk:\n")
    emitRaw("  %chunk = load i64, ptr %chunk.addr\n")
    emitRaw("  %zero = icmp eq i64 %chunk, 0\n")
    emitRaw("  %base = select i1 %zero, i64 %alloc, i64 %chunk\n")
    emitRaw("  %dbl = add i64 %base, %base\n")
    emitRaw("  %wrap = icmp ult i64 %dbl, %base\n")
    emitRaw("  %grown = select i1 %wrap, i64 %base, i64 %dbl\n")
    emitRaw("  %need = icmp ult i64 %grown, %alloc\n")
    emitRaw("  %cap = select i1 %need, i64 %alloc, i64 %grown\n")
    emitRaw("  %total = add i64 %cap, 8\n")
    emitRaw("  %totalov = icmp ult i64 %total, %cap\n")
    emitRaw("  br i1 %totalov, label %oom, label %allocchunk\n")
    emitRaw("allocchunk:\n")
    emitRaw("  %mem = call ptr @malloc(i64 %total)\n")
    emitRaw("  %null = icmp eq ptr %mem, null\n")
    emitRaw("  br i1 %null, label %oom, label %link\n")
    emitRaw("link:\n")
    emitRaw("  %head = load ptr, ptr %head.addr\n")
    emitRaw("  store ptr %head, ptr %mem\n")
    emitRaw("  store ptr %mem, ptr %head.addr\n")
    emitRaw("  %payload = getelementptr inbounds %LannerArenaNode, ptr %mem, i32 0, i32 1\n")
    emitRaw("  store ptr %payload, ptr %cur.addr\n")
    emitRaw("  store i64 %cap, ptr %rem.addr\n")
    emitRaw("  store i64 %cap, ptr %chunk.addr\n")
    emitRaw("  br label %after\n")
    emitRaw("after:\n")
    emitRaw("  %fresh = load ptr, ptr %cur.addr\n")
    emitRaw("  %freshrem = load i64, ptr %rem.addr\n")
    emitRaw("  %nextfresh = getelementptr i8, ptr %fresh, i64 %alloc\n")
    emitRaw("  %leftfresh = sub i64 %freshrem, %alloc\n")
    emitRaw("  store ptr %nextfresh, ptr %cur.addr\n")
    emitRaw("  store i64 %leftfresh, ptr %rem.addr\n")
    emitRaw("  ret ptr %fresh\n")
    emitRaw("oom:\n")
    emitRaw("  call void @llvm.trap()\n  unreachable\n")
    emitRaw("}\n\n")
    emitRaw("define internal void @__lanner_arena_destroy(ptr %arena) {\n")
    emitRaw("entry:\n  %first = load ptr, ptr %arena\n  br label %loop\n")
    emitRaw("loop:\n  %node = phi ptr [ %first, %entry ], [ %next, %step ]\n  %has = icmp ne ptr %node, null\n  br i1 %has, label %free, label %done\n")
    emitRaw("free:\n  %next = load ptr, ptr %node\n  call void @free(ptr %node)\n  br label %step\n")
    emitRaw("step:\n  br label %loop\n")
    emitRaw("done:\n  store ptr null, ptr %arena\n  %cur0 = getelementptr inbounds %LannerArena, ptr %arena, i32 0, i32 1\n  %rem0 = getelementptr inbounds %LannerArena, ptr %arena, i32 0, i32 2\n  store ptr null, ptr %cur0\n  store i64 0, ptr %rem0\n  ret void\n}\n\n")
    emitRaw("define internal void @__lanner_dyn_array_destroy(ptr %array) {\n")
    emitRaw("entry:\n  %data.addr = getelementptr inbounds %LannerDynArray, ptr %array, i32 0, i32 0\n  %arena.addr = getelementptr inbounds %LannerDynArray, ptr %array, i32 0, i32 3\n  %data = load ptr, ptr %data.addr\n  %arena = load ptr, ptr %arena.addr\n  %has = icmp ne ptr %arena, null\n  %freeptr = select i1 %has, ptr null, ptr %data\n  %nonnull = icmp ne ptr %freeptr, null\n  br i1 %nonnull, label %free, label %done\n")
    emitRaw("free:\n  call void @free(ptr %freeptr)\n  br label %done\n")
    emitRaw("done:\n  store ptr null, ptr %data.addr\n  %len.addr = getelementptr inbounds %LannerDynArray, ptr %array, i32 0, i32 1\n  %cap.addr = getelementptr inbounds %LannerDynArray, ptr %array, i32 0, i32 2\n  store i64 0, ptr %len.addr\n  store i64 0, ptr %cap.addr\n  ret void\n}\n\n")
    emitRaw("define internal %LannerDynArray @__lanner_read_file(ptr %path) {\n")
    emitRaw("entry:\n")
    emitRaw("  %f = call ptr @fopen(ptr %path, ptr @.lanner.rb)\n")
    emitRaw("  %missing = icmp eq ptr %f, null\n")
    emitRaw("  br i1 %missing, label %fail, label %seek\n")
    emitRaw("seek:\n")
    emitRaw("  %seekrc = call i32 @fseek(ptr %f, i64 0, i32 2)\n")
    emitRaw("  %seekbad = icmp ne i32 %seekrc, 0\n")
    emitRaw("  br i1 %seekbad, label %closefail, label %file_size\n")
    emitRaw("file_size:\n")
    emitRaw("  %fsize = call i64 @ftell(ptr %f)\n")
    emitRaw("  %sizebad = icmp slt i64 %fsize, 0\n")
    emitRaw("  br i1 %sizebad, label %closefail, label %rewind\n")
    emitRaw("rewind:\n")
    emitRaw("  %rewindrc = call i32 @fseek(ptr %f, i64 0, i32 0)\n")
    emitRaw("  %rewindbad = icmp ne i32 %rewindrc, 0\n")
    emitRaw("  br i1 %rewindbad, label %closefail, label %alloc\n")
    emitRaw("alloc:\n")
    emitRaw("  %allocSize = add i64 %fsize, 1\n")
    emitRaw("  %overflow = icmp ult i64 %allocSize, %fsize\n")
    emitRaw("  br i1 %overflow, label %closefail, label %mallocData\n")
    emitRaw("mallocData:\n")
    emitRaw("  %data = call ptr @malloc(i64 %allocSize)\n")
    emitRaw("  %allocbad = icmp eq ptr %data, null\n")
    emitRaw("  br i1 %allocbad, label %closefail, label %read_data\n")
    emitRaw("read_data:\n")
    emitRaw("  %read = call i64 @fread(ptr %data, i64 1, i64 %fsize, ptr %f)\n")
    emitRaw("  %readbad = icmp ne i64 %read, %fsize\n")
    emitRaw("  br i1 %readbad, label %freefail, label %finish\n")
    emitRaw("finish:\n")
    emitRaw("  %nul = getelementptr inbounds i8, ptr %data, i64 %fsize\n")
    emitRaw("  store i8 0, ptr %nul\n")
    emitRaw("  %close = call i32 @fclose(ptr %f)\n")
    emitRaw("  %a0 = insertvalue %LannerDynArray { ptr null, i64 0, i64 0, ptr null }, ptr %data, 0\n")
    emitRaw("  %a1 = insertvalue %LannerDynArray %a0, i64 %fsize, 1\n")
    emitRaw("  %a2 = insertvalue %LannerDynArray %a1, i64 %fsize, 2\n")
    emitRaw("  ret %LannerDynArray %a2\n")
    emitRaw("freefail:\n")
    emitRaw("  call void @free(ptr %data)\n")
    emitRaw("  br label %closefail\n")
    emitRaw("closefail:\n")
    emitRaw("  call i32 @fclose(ptr %f)\n")
    emitRaw("  br label %fail\n")
    emitRaw("fail:\n")
    emitRaw("  call void @llvm.trap()\n")
    emitRaw("  unreachable\n")
    emitRaw("}\n\n")
    i = 0
    while i < nodes.len():
        if nodes[i].level == 0 && nodes[i].kind == 1:
            if !llvmNative3EmitFunction(source, nodes, i, &funcs, &params, &structs, &fields, &mut arrays, &mut fixedArrays):
                return false
        i = i + 1
    if !llvmNative3EmitDropGlue(source, &structs, &fields, &mut arrays, &mut fixedArrays):
        return false
    return true

main() i32:
    path: string = getEnv("LANNER_SELFHOST_INPUT")
    if stringLen(path) == 0:
        path = "examples/selfhost_input.st"
    source: []u8 = readFile(path)
    # Self-hosted compiler pipeline: AST -> semantic analysis -> HIR -> optimization -> backend.
    nodes: []AstNode = []AstNode
    children: []i64 = []i64
    buildAST(&source, &mut nodes, &mut children)
    typedParams: []TypedParamInfo = []TypedParamInfo
    typedFields: []TypedFieldInfo = []TypedFieldInfo
    if !typedAnnotateAST(&source, &mut nodes, &mut typedParams, &mut typedFields):
        printInt(1)
        return 1
    if !typedValidateAST(&source, &nodes, &typedParams, &typedFields):
        printInt(1)
        return 1
    typedHIR: []TypedHIR = []TypedHIR
    lowerTypedHIR(&source, &nodes, &mut typedHIR)
    if typedHIR.len() != nodes.len():
        printInt(1)
        return 1
    referenceMode: string = getEnv("LANNER_SELFHOST_REFERENCE")
    if stringLen(referenceMode) == 0:
        if emitNativeLLVM3(&source, &nodes, &typedParams, &typedFields):
            return 0
        printInt(2)
        return 2

    # Explicit compatibility/reference path only. The production self-hosting
    # path above is typed AST/HIR -> native LLVM and does not depend on this code.
    if !semanticCheck(&source, &nodes):
        printInt(1)
        return 1
    hir: []i64 = []i64
    lowerToHIR(&source, &nodes, &mut hir)
    optimizeHIR(&mut hir)
    emitCPP(&source, hir)
    return 0
