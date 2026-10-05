# **Security Policy**

Stable is a systems programming language and compiler with **memory safety as a core design goal**.

Security issues may include:

- **incorrect ownership checking**
- **invalid borrow acceptance**
- **use-after-free generation**
- **double-destruction generation**
- **memory corruption**
- **incorrect bounds-check elimination**
- **malformed LLVM IR**
- **compiler crashes on hostile input**
- **unsafe filesystem/process behavior**

## **Reporting**

For sensitive vulnerabilities, use a private vulnerability-reporting mechanism rather than publishing exploit details immediately.

Include:

- **Stable version**
- **OS/toolchain**
- **minimal reproducer**
- **exact command line**
- **observed behavior**
- **expected behavior**
- **sanitizer output**, when available

## **Security philosophy**

The Stable memory model aims to prevent common classes of memory misuse through compile-time ownership and borrowing checks.

Important invariants include **no use-after-move**, **no conflicting mutable aliases**, **no dangling borrows**, **bounds-safe indexing**, and **deterministic destruction**.

Testing remains essential because compiler correctness is not established by language design alone.
