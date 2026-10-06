main() i32:
    mutex: Mutex = Mutex.create()
    Mutex.lock(mutex)
    blocked: bool = Mutex.tryLock(mutex)
    Mutex.unlock(mutex)
    Mutex.destroy(mutex)
    if blocked:
        return 2

    rw: RwLock = RwLock.create()
    RwLock.readLock(rw)
    RwLock.unlock(rw, false)
    RwLock.writeLock(rw)
    RwLock.unlock(rw, true)
    RwLock.destroy(rw)

    sem: Semaphore = Semaphore.create(1)
    first: bool = Semaphore.tryWait(sem)
    second: bool = Semaphore.tryWait(sem)
    Semaphore.post(sem)
    third: bool = Semaphore.wait(sem, 10)
    Semaphore.destroy(sem)
    if !first || second || !third:
        return 5

    m2: Mutex = Mutex.create()
    cv: Condvar = Condvar.create()
    timedOut: bool = true
    Mutex.lock(m2)
    timedOut = !Condvar.wait(cv, m2, 0)
    Mutex.unlock(m2)
    Condvar.destroy(cv)
    Mutex.destroy(m2)
    if !timedOut:
        return 7
    return 42
