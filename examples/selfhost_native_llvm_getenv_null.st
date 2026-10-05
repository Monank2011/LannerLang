main() i32:
    missing: string = getEnv("STABLE_NATIVE_LLVM_TEST_MUST_BE_UNSET_7F9C2A")
    if stringLen(missing) == 0:
        return 0
    return 9
