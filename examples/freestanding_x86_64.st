unsafe _start() i32:
    syscall:i64=60
    status:i64=42
    asm("mov $0, %rax; mov $1, %rdi; syscall", "r,r,~{rax},~{rdi},~{rcx},~{r11}", syscall, status)
    return 0
