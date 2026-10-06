#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#if !defined(_WIN32)
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>
#else
#include <direct.h>
#include <sys/stat.h>
#include <io.h>
#endif

#if defined(_WIN32)
#include <malloc.h>
#endif


#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>
#define STABLE_POLLIN POLLRDNORM
#define STABLE_POLLOUT POLLWRNORM
#define STABLE_POLLERR POLLERR
#else
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#if defined(__linux__)
#include <sys/epoll.h>
#endif
#include <signal.h>
#include <unistd.h>
#define STABLE_POLLIN POLLIN
#define STABLE_POLLOUT POLLOUT
#define STABLE_POLLERR POLLERR
#endif

#if defined(_WIN32)
#define STABLE_TLS __declspec(thread)
#else
#define STABLE_TLS _Thread_local
#endif

/* Mobile platform ABI.  Hosted mobile builds override these weak hooks from
 * generated Android/iOS host bridges; the fallback keeps native-object builds
 * linkable and returns conservative values. */
#if defined(__GNUC__) || defined(__clang__)
#define STABLE_WEAK __attribute__((weak))
#else
#define STABLE_WEAK
#endif
static STABLE_TLS char stable_mobile_path_buf[4096];
static STABLE_TLS char stable_mobile_clipboard_buf[4096];
static int32_t stable_mobile_width = 0, stable_mobile_height = 0;
static double stable_mobile_scale_value = 1.0;
static int32_t stable_mobile_safe_top_value = 0, stable_mobile_safe_bottom_value = 0, stable_mobile_safe_left_value = 0, stable_mobile_safe_right_value = 0;
void __stable_mobile_set_metrics(int32_t w, int32_t h, double scale, int32_t top, int32_t bottom, int32_t left, int32_t right) {
    stable_mobile_width=w; stable_mobile_height=h; stable_mobile_scale_value=scale;
    stable_mobile_safe_top_value=top; stable_mobile_safe_bottom_value=bottom; stable_mobile_safe_left_value=left; stable_mobile_safe_right_value=right;
}
STABLE_WEAK void __stable_mobile_log(const char* s) { if (s) fprintf(stderr, "[Stable] %s\n", s); }
STABLE_WEAK const char* __stable_mobile_platform(void) {
#if defined(__ANDROID__)
    return "android";
#elif defined(__APPLE__) && defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__)
    return "ios";
#else
    return "mobile";
#endif
}
STABLE_WEAK const char* __stable_mobile_os_version(void) { return "unknown"; }
STABLE_WEAK int32_t __stable_mobile_is_simulator(void) {
#if defined(TARGET_OS_SIMULATOR) && TARGET_OS_SIMULATOR
    return 1;
#elif defined(__aarch64__) && defined(__APPLE__)
    return 0;
#else
    return 0;
#endif
}
STABLE_WEAK int32_t __stable_mobile_screen_width(void) { return stable_mobile_width; }
STABLE_WEAK int32_t __stable_mobile_screen_height(void) { return stable_mobile_height; }
STABLE_WEAK double __stable_mobile_device_scale(void) { return stable_mobile_scale_value; }
STABLE_WEAK int32_t __stable_mobile_safe_top(void) { return stable_mobile_safe_top_value; }
STABLE_WEAK int32_t __stable_mobile_safe_bottom(void) { return stable_mobile_safe_bottom_value; }
STABLE_WEAK int32_t __stable_mobile_safe_left(void) { return stable_mobile_safe_left_value; }
STABLE_WEAK int32_t __stable_mobile_safe_right(void) { return stable_mobile_safe_right_value; }
STABLE_WEAK int32_t __stable_mobile_open_url(const char* url) { (void)url; return 0; }
STABLE_WEAK int32_t __stable_mobile_vibrate(int32_t ms) { (void)ms; return 0; }
STABLE_WEAK int32_t __stable_mobile_request_permission(const char* permission) { (void)permission; return 0; }
STABLE_WEAK int32_t __stable_mobile_clipboard_set(const char* text) {
    if (!text) return 0;
    snprintf(stable_mobile_clipboard_buf, sizeof(stable_mobile_clipboard_buf), "%s", text);
    return 1;
}
void* ____stable_buffer_from_string(const char* text);
STABLE_WEAK void* __stable_mobile_clipboard_get(void) { return ____stable_buffer_from_string(stable_mobile_clipboard_buf); }
STABLE_WEAK int32_t __stable_mobile_camera_available(void) { return 0; }
STABLE_WEAK int32_t __stable_mobile_location_available(void) { return 0; }
STABLE_WEAK int32_t __stable_mobile_bluetooth_available(void) { return 0; }
STABLE_WEAK const char* __stable_mobile_app_data_path(void) { return stable_mobile_path_buf[0] ? stable_mobile_path_buf : ""; }
STABLE_WEAK const char* __stable_mobile_documents_path(void) { return stable_mobile_path_buf[0] ? stable_mobile_path_buf : ""; }
STABLE_WEAK const char* __stable_mobile_cache_path(void) { return stable_mobile_path_buf[0] ? stable_mobile_path_buf : ""; }

void* stable_aligned_alloc(size_t size, size_t alignment) {
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) return NULL;
#if defined(_WIN32)
    return _aligned_malloc(size, alignment);
#else
    void* p = NULL;
    if (posix_memalign(&p, alignment, size) != 0) return NULL;
    return p;
#endif
}

void stable_aligned_free(void* ptr) {
#if defined(_WIN32)
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

#if defined(_MSC_VER)
#define STABLE_TLS __declspec(thread)
#else
#define STABLE_TLS _Thread_local
#endif

typedef struct StableThreadHandle {
#if defined(_WIN32)
    HANDLE handle;
#else
    pthread_t handle;
#endif
} StableThreadHandle;

typedef void (*StableThreadFn)(void*);

typedef struct StableThreadStart {
    StableThreadFn fn;
} StableThreadStart;

static STABLE_TLS char *stable_line_buf = NULL;
static STABLE_TLS size_t stable_line_cap = 0;

static char* stable_line_buffer(void) {
    if (!stable_line_buf) {
        stable_line_cap = 4096;
        stable_line_buf = (char*)malloc(stable_line_cap);
        if (!stable_line_buf) abort();
    }
    return stable_line_buf;
}

const char* __stable_stdin_read_line(void) {
    char *buf = stable_line_buffer();
    for (;;) {
        if (fgets(buf, (int)stable_line_cap, stdin) != NULL) {
            size_t n = strlen(buf);
            while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) buf[--n] = '\0';
            return buf;
        }
        if (feof(stdin)) {
            buf[0] = '\0';
            clearerr(stdin);
            return buf;
        }
        clearerr(stdin);
        return buf;
    }
}

int __stable_stdin_has_input(void) {
#if defined(_WIN32)
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    if (h == INVALID_HANDLE_VALUE || h == NULL) return 0;
    DWORD type = GetFileType(h);
    if (type == FILE_TYPE_CHAR) {
        DWORD events = 0;
        if (!GetNumberOfConsoleInputEvents(h, &events)) return 0;
        return events > 0 ? 1 : 0;
    }
    if (type == FILE_TYPE_PIPE) {
        DWORD available = 0;
        if (!PeekNamedPipe(h, NULL, 0, NULL, &available, NULL)) return 0;
        return available > 0 ? 1 : 0;
    }
    return 0;
#else
    const int fd = fileno(stdin);
    if (fd < 0) return 0;
    fd_set set;
    FD_ZERO(&set);
    FD_SET(fd, &set);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    const int rc = select(fd + 1, &set, NULL, NULL, &tv);
    return rc > 0 && FD_ISSET(fd, &set) ? 1 : 0;
#endif
}

uint64_t __stable_clock_monotonic_nanos(void) {
#if defined(_WIN32)
    LARGE_INTEGER freq, now;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    return (uint64_t)((now.QuadPart * 1000000000ULL) / (uint64_t)freq.QuadPart);
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
#endif
}

void __stable_clock_sleep_nanos(uint64_t nanos) {
#if defined(_WIN32)
    DWORD ms = (DWORD)((nanos + 999999ULL) / 1000000ULL);
    Sleep(ms);
#else
    struct timespec req;
    req.tv_sec = (time_t)(nanos / 1000000000ULL);
    req.tv_nsec = (long)(nanos % 1000000000ULL);
    while (nanos > 0 && nanosleep(&req, &req) != 0 && errno == EINTR) {}
#endif
}

uint64_t __stable_thread_hardware_concurrency(void) {
#if defined(_WIN32)
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return (uint64_t)(info.dwNumberOfProcessors ? info.dwNumberOfProcessors : 1);
#else
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return (uint64_t)(n > 0 ? n : 1);
#endif
}

#if defined(_WIN32)
static unsigned __stdcall stable_thread_entry(void *arg) {
    StableThreadStart *start = (StableThreadStart*)arg;
    StableThreadFn fn = start->fn;
    free(start);
    fn(NULL);
    return 0;
}
#else
static void* stable_thread_entry(void *arg) {
    StableThreadStart *start = (StableThreadStart*)arg;
    StableThreadFn fn = start->fn;
    free(start);
    fn(NULL);
    return NULL;
}
#endif

void* __stable_thread_spawn(StableThreadFn fn) {
    if (!fn) return NULL;
    StableThreadStart *start = (StableThreadStart*)malloc(sizeof(*start));
    StableThreadHandle *handle = (StableThreadHandle*)malloc(sizeof(*handle));
    if (!start || !handle) { free(start); free(handle); return NULL; }
    start->fn = fn;
#if defined(_WIN32)
    handle->handle = (HANDLE)_beginthreadex(NULL, 0, stable_thread_entry, start, 0, NULL);
    if (!handle->handle) { free(start); free(handle); return NULL; }
#else
    if (pthread_create(&handle->handle, NULL, stable_thread_entry, start) != 0) {
        free(start); free(handle); return NULL;
    }
#endif
    return handle;
}

void __stable_thread_join(void *raw) {
    StableThreadHandle *handle = (StableThreadHandle*)raw;
    if (!handle) return;
#if defined(_WIN32)
    WaitForSingleObject(handle->handle, INFINITE);
    CloseHandle(handle->handle);
#else
    (void)pthread_join(handle->handle, NULL);
#endif
    free(handle);
}

void __stable_thread_detach(void *raw) {
    StableThreadHandle *handle = (StableThreadHandle*)raw;
    if (!handle) return;
#if defined(_WIN32)
    CloseHandle(handle->handle);
#else
    (void)pthread_detach(handle->handle);
#endif
    free(handle);
}

void __stable_thread_yield(void) {
#if defined(_WIN32)
    SwitchToThread();
#else
    sched_yield();
#endif
}


#if defined(_WIN32)
static void stable_net_init(void) {
    static LONG initialized = 0;
    if (InterlockedCompareExchange(&initialized, 1, 0) == 0) {
        WSADATA data;
        const int ok = WSAStartup(MAKEWORD(2, 2), &data) == 0;
        InterlockedExchange(&initialized, ok ? 2 : -1);
    } else {
        while (InterlockedCompareExchange(&initialized, 0, 0) == 1) Sleep(0);
    }
}
#define STABLE_INVALID_SOCKET ((SOCKET)(~(uintptr_t)0))
#else
static void stable_net_init(void) {}
#define STABLE_INVALID_SOCKET (-1)
#endif

typedef struct StableSocketHandle {
#if defined(_WIN32)
    SOCKET fd;
#else
    int fd;
#endif
} StableSocketHandle;

typedef struct StablePollerEntry {
    StableSocketHandle* socket;
    short events;
} StablePollerEntry;

typedef struct StablePollerReady {
    StableSocketHandle* socket;
    int mask;
} StablePollerReady;

typedef struct StablePoller {
    StablePollerEntry* entries;
    size_t count;
    size_t cap;
    StablePollerReady* ready;
    size_t ready_count;
    size_t ready_cap;
#if defined(__linux__)
    int backend_fd;
#endif
} StablePoller;

typedef struct StableMutexHandle {
#if defined(_WIN32)
    CRITICAL_SECTION value;
#else
    pthread_mutex_t value;
#endif
} StableMutexHandle;

typedef struct StableRWLockHandle {
#if defined(_WIN32)
    SRWLOCK value;
#else
    pthread_rwlock_t value;
#endif
} StableRWLockHandle;

typedef struct StableCondvarHandle {
#if defined(_WIN32)
    CONDITION_VARIABLE value;
#else
    pthread_cond_t value;
#endif
} StableCondvarHandle;

typedef struct StableSemaphoreHandle {
#if defined(_WIN32)
    HANDLE value;
#else
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    unsigned value;
#endif
} StableSemaphoreHandle;

typedef struct StableProcessHandle {
#if defined(_WIN32)
    HANDLE process;
    DWORD pid;
#else
    pid_t pid;
#endif
} StableProcessHandle;

typedef struct StableBuffer {
    unsigned char* data;
    size_t len;
    size_t cap;
} StableBuffer;

static STABLE_TLS int stable_net_last_error_code = 0;
static STABLE_TLS char stable_net_last_error_text[256];
static STABLE_TLS int stable_http_status_code = 0;

static void stable_set_error(int code, const char* text) {
    stable_net_last_error_code = code;
    if (!text) text = "unknown error";
    strncpy(stable_net_last_error_text, text, sizeof(stable_net_last_error_text) - 1);
    stable_net_last_error_text[sizeof(stable_net_last_error_text) - 1] = '\0';
}

static void stable_capture_socket_error(void) {
#if defined(_WIN32)
    int e = WSAGetLastError();
    stable_net_last_error_code = e;
    _snprintf_s(stable_net_last_error_text, sizeof(stable_net_last_error_text), _TRUNCATE, "socket error %d", e);
#else
    int e = errno;
    stable_net_last_error_code = e;
    const char* msg = strerror(e);
    stable_set_error(e, msg);
#endif
}

static int stable_socket_valid(const StableSocketHandle* s) {
    if (!s) return 0;
#if defined(_WIN32)
    return s->fd != INVALID_SOCKET;
#else
    return s->fd >= 0;
#endif
}

static int stable_set_nonblocking_fd(StableSocketHandle* s, int enabled) {
    if (!stable_socket_valid(s)) return 0;
#if defined(_WIN32)
    u_long mode = enabled ? 1UL : 0UL;
    if (ioctlsocket(s->fd, FIONBIO, &mode) != 0) { stable_capture_socket_error(); return 0; }
#else
    int flags = fcntl(s->fd, F_GETFL, 0);
    if (flags < 0 || fcntl(s->fd, F_SETFL, enabled ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK)) != 0) {
        stable_capture_socket_error(); return 0;
    }
#endif
    return 1;
}

static void stable_set_socket_timeout(StableSocketHandle* s, int timeout_ms) {
    if (!stable_socket_valid(s) || timeout_ms < 0) return;
#if defined(_WIN32)
    DWORD ms = (DWORD)timeout_ms;
    setsockopt(s->fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&ms, sizeof(ms));
    setsockopt(s->fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&ms, sizeof(ms));
#else
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(s->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(s->fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
}

static StableSocketHandle* stable_socket_new(
#if defined(_WIN32)
    SOCKET fd
#else
    int fd
#endif
) {
    StableSocketHandle* s = (StableSocketHandle*)malloc(sizeof(*s));
    if (!s) {
#if defined(_WIN32)
        closesocket(fd);
#else
        close(fd);
#endif
        return NULL;
    }
    s->fd = fd;
    return s;
}

static int stable_resolve(const char* host, const char* port, int socktype, int passive,
#if defined(_WIN32)
                          struct addrinfo** result
#else
                          struct addrinfo** result
#endif
) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = socktype;
    hints.ai_protocol = (socktype == SOCK_STREAM) ? IPPROTO_TCP : IPPROTO_UDP;
    if (passive) hints.ai_flags = AI_PASSIVE;
    int rc = getaddrinfo((host && *host) ? host : NULL, port, &hints, result);
    if (rc != 0) {
#if defined(_WIN32)
        stable_set_error(rc, gai_strerrorA(rc));
#else
        stable_set_error(rc, gai_strerror(rc));
#endif
        return 0;
    }
    return 1;
}

void* __stable_net_tcp_connect(const char* host, uint32_t port, int32_t timeout_ms) {
    stable_net_init();
    char port_text[16];
    snprintf(port_text, sizeof(port_text), "%u", (unsigned)port);
    struct addrinfo* results = NULL;
    if (!stable_resolve(host, port_text, SOCK_STREAM, 0, &results)) return NULL;
    StableSocketHandle* out = NULL;
    for (struct addrinfo* it = results; it; it = it->ai_next) {
#if defined(_WIN32)
        SOCKET fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (fd == INVALID_SOCKET) continue;
#else
        int fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (fd < 0) continue;
#endif
        StableSocketHandle* s = stable_socket_new(fd);
        if (!s) continue;
        int connected = 0;
        if (timeout_ms < 0) {
            connected = connect(fd, it->ai_addr, (int)it->ai_addrlen) == 0;
        } else {
            if (!stable_set_nonblocking_fd(s, 1)) {
                stable_capture_socket_error();
            } else {
                int rc = connect(fd, it->ai_addr, (int)it->ai_addrlen);
                if (rc == 0) {
                    connected = 1;
                } else {
#if defined(_WIN32)
                    const int e = WSAGetLastError();
                    const int pending = (e == WSAEWOULDBLOCK || e == WSAEINPROGRESS || e == WSAEALREADY);
#else
                    const int e = errno;
                    const int pending = (e == EINPROGRESS || e == EALREADY || e == EINTR);
#endif
                    if (pending) {
#if defined(_WIN32)
                        WSAPOLLFD pfd; pfd.fd = fd; pfd.events = STABLE_POLLOUT; pfd.revents = 0;
                        rc = WSAPoll(&pfd, 1, timeout_ms);
#else
                        struct pollfd pfd; pfd.fd = fd; pfd.events = STABLE_POLLOUT; pfd.revents = 0;
                        rc = poll(&pfd, 1, timeout_ms);
#endif
                        if (rc > 0) {
                            int so_error = 0;
#if defined(_WIN32)
                            int so_len = (int)sizeof(so_error);
                            getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*)&so_error, &so_len);
#else
                            socklen_t so_len = (socklen_t)sizeof(so_error);
                            getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &so_len);
#endif
                            connected = so_error == 0;
                            if (!connected) stable_net_last_error_code = so_error;
                        } else if (rc == 0) {
                            stable_set_error(-7, "TCP connect timed out");
                        }
                    } else {
                        stable_capture_socket_error();
                    }
                }
                if (connected) stable_set_nonblocking_fd(s, 0);
            }
        }
        if (connected) {
            if (timeout_ms >= 0) stable_set_socket_timeout(s, timeout_ms);
            out = s;
            break;
        }
        if (!stable_net_last_error_code) stable_capture_socket_error();
#if defined(_WIN32)
        closesocket(fd);
#else
        close(fd);
#endif
        free(s);
    }
    freeaddrinfo(results);
    return out;
}

void* __stable_net_tcp_listen(const char* host, uint32_t port, int32_t backlog) {
    stable_net_init();
    if (backlog <= 0) backlog = 128;
    char port_text[16];
    snprintf(port_text, sizeof(port_text), "%u", (unsigned)port);
    struct addrinfo* results = NULL;
    if (!stable_resolve(host, port_text, SOCK_STREAM, 1, &results)) return NULL;
    StableSocketHandle* out = NULL;
    for (struct addrinfo* it = results; it; it = it->ai_next) {
#if defined(_WIN32)
        SOCKET fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (fd == INVALID_SOCKET) continue;
        BOOL yes = TRUE;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));
#else
        int fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (fd < 0) continue;
        int yes = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
#endif
        if (bind(fd, it->ai_addr, (int)it->ai_addrlen) == 0 && listen(fd, backlog) == 0) {
            out = stable_socket_new(fd);
            if (out) break;
        } else stable_capture_socket_error();
#if defined(_WIN32)
        closesocket(fd);
#else
        close(fd);
#endif
    }
    freeaddrinfo(results);
    return out;
}

void* __stable_net_accept(void* raw) {
    StableSocketHandle* server = (StableSocketHandle*)raw;
    if (!stable_socket_valid(server)) return NULL;
#if defined(_WIN32)
    SOCKET fd = accept(server->fd, NULL, NULL);
    if (fd == INVALID_SOCKET) { stable_capture_socket_error(); return NULL; }
#else
    int fd = accept(server->fd, NULL, NULL);
    if (fd < 0) { stable_capture_socket_error(); return NULL; }
#endif
    return stable_socket_new(fd);
}

static uint32_t stable_socket_port(void* raw, int peer) {
    StableSocketHandle* s = (StableSocketHandle*)raw;
    if (!stable_socket_valid(s)) return 0;
    struct sockaddr_storage addr;
#if defined(_WIN32)
    int len = (int)sizeof(addr);
    int rc = peer ? getpeername(s->fd, (struct sockaddr*)&addr, &len) : getsockname(s->fd, (struct sockaddr*)&addr, &len);
#else
    socklen_t len = (socklen_t)sizeof(addr);
    int rc = peer ? getpeername(s->fd, (struct sockaddr*)&addr, &len) : getsockname(s->fd, (struct sockaddr*)&addr, &len);
#endif
    if (rc != 0) { stable_capture_socket_error(); return 0; }
    if (addr.ss_family == AF_INET) return (uint32_t)ntohs(((struct sockaddr_in*)&addr)->sin_port);
    if (addr.ss_family == AF_INET6) return (uint32_t)ntohs(((struct sockaddr_in6*)&addr)->sin6_port);
    return 0;
}

uint32_t __stable_net_local_port(void* raw) { return stable_socket_port(raw, 0); }
uint32_t __stable_net_peer_port(void* raw) { return stable_socket_port(raw, 1); }

int64_t __stable_net_send(void* raw, const void* data, uint64_t len) {
    StableSocketHandle* s = (StableSocketHandle*)raw;
    if (!stable_socket_valid(s) || !data) return -1;
    size_t n = (size_t)len;
#if defined(_WIN32)
    int chunk = n > INT_MAX ? INT_MAX : (int)n;
    int rc = send(s->fd, (const char*)data, chunk, 0);
#else
    ssize_t rc = send(s->fd, data, n, MSG_NOSIGNAL);
#endif
    if (rc < 0) { stable_capture_socket_error(); return -1; }
    return (int64_t)rc;
}

int64_t __stable_net_recv(void* raw, void* data, uint64_t len) {
    StableSocketHandle* s = (StableSocketHandle*)raw;
    if (!stable_socket_valid(s) || !data) return -1;
    size_t n = (size_t)len;
#if defined(_WIN32)
    int chunk = n > INT_MAX ? INT_MAX : (int)n;
    int rc = recv(s->fd, (char*)data, chunk, 0);
#else
    ssize_t rc = recv(s->fd, data, n, 0);
#endif
    if (rc < 0) { stable_capture_socket_error(); return -1; }
    return (int64_t)rc;
}

int64_t __stable_net_send_string(void* raw, const char* text) {
    return __stable_net_send(raw, text, text ? strlen(text) : 0);
}

int __stable_net_set_nonblocking(void* raw, int enabled) {
    return stable_set_nonblocking_fd((StableSocketHandle*)raw, enabled);
}

int __stable_net_poll(void* raw, int events, int32_t timeout_ms) {
    StableSocketHandle* s = (StableSocketHandle*)raw;
    if (!stable_socket_valid(s)) return -1;
#if defined(_WIN32)
    WSAPOLLFD p;
    p.fd = s->fd;
    p.events = (SHORT)events;
    p.revents = 0;
    int rc = WSAPoll(&p, 1, timeout_ms < 0 ? -1 : timeout_ms);
#else
    struct pollfd p;
    p.fd = s->fd;
    p.events = (short)events;
    p.revents = 0;
    int rc = poll(&p, 1, timeout_ms < 0 ? -1 : timeout_ms);
#endif
    if (rc < 0) { stable_capture_socket_error(); return -1; }
    if (rc == 0) return 0;
#if defined(_WIN32)
    return (int)p.revents;
#else
    return (int)p.revents;
#endif
}

int __stable_net_tcp_nodelay(void* raw, int enabled) {
    StableSocketHandle* s = (StableSocketHandle*)raw;
    if (!stable_socket_valid(s)) return 0;
    int value = enabled ? 1 : 0;
#if defined(_WIN32)
    if (setsockopt(s->fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&value, sizeof(value)) != 0) { stable_capture_socket_error(); return 0; }
#else
    if (setsockopt(s->fd, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value)) != 0) { stable_capture_socket_error(); return 0; }
#endif
    return 1;
}

int __stable_net_shutdown(void* raw, int how) {
    StableSocketHandle* s = (StableSocketHandle*)raw;
    if (!stable_socket_valid(s)) return -1;
#if defined(_WIN32)
    return shutdown(s->fd, how);
#else
    return shutdown(s->fd, how);
#endif
}

void __stable_net_close(void* raw) {
    StableSocketHandle* s = (StableSocketHandle*)raw;
    if (!s) return;
#if defined(_WIN32)
    if (s->fd != INVALID_SOCKET) closesocket(s->fd);
#else
    if (s->fd >= 0) close(s->fd);
#endif
    free(s);
}

int __stable_net_last_error(void) { return stable_net_last_error_code; }
const char* __stable_net_error_string(void) { return stable_net_last_error_text; }

void* __stable_net_udp_open(void) {
    stable_net_init();
#if defined(_WIN32)
    SOCKET fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd == INVALID_SOCKET) { stable_capture_socket_error(); return NULL; }
#else
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) { stable_capture_socket_error(); return NULL; }
#endif
    return stable_socket_new(fd);
}

int __stable_net_udp_bind(void* raw, const char* host, uint32_t port) {
    StableSocketHandle* s = (StableSocketHandle*)raw;
    if (!stable_socket_valid(s)) return 0;
    char port_text[16]; snprintf(port_text, sizeof(port_text), "%u", (unsigned)port);
    struct addrinfo* results = NULL;
    if (!stable_resolve(host, port_text, SOCK_DGRAM, 1, &results)) return 0;
    int ok = 0;
    for (struct addrinfo* it = results; it; it = it->ai_next) {
        if (bind(s->fd, it->ai_addr, (int)it->ai_addrlen) == 0) { ok = 1; break; }
    }
    if (!ok) stable_capture_socket_error();
    freeaddrinfo(results);
    return ok;
}

int64_t __stable_net_udp_send_to(void* raw, const char* host, uint32_t port, const void* data, uint64_t len) {
    StableSocketHandle* s = (StableSocketHandle*)raw;
    if (!stable_socket_valid(s) || !data) return -1;
    char port_text[16]; snprintf(port_text, sizeof(port_text), "%u", (unsigned)port);
    struct addrinfo* results = NULL;
    if (!stable_resolve(host, port_text, SOCK_DGRAM, 0, &results)) return -1;
    int64_t result = -1;
    for (struct addrinfo* it = results; it; it = it->ai_next) {
#if defined(_WIN32)
        int chunk = len > INT_MAX ? INT_MAX : (int)len;
        int rc = sendto(s->fd, (const char*)data, chunk, 0, it->ai_addr, (int)it->ai_addrlen);
#else
        ssize_t rc = sendto(s->fd, data, (size_t)len, 0, it->ai_addr, it->ai_addrlen);
#endif
        if (rc >= 0) { result = (int64_t)rc; break; }
        stable_capture_socket_error();
    }
    freeaddrinfo(results);
    return result;
}


int64_t __stable_net_udp_recv(void* raw, void* data, uint64_t len) {
    StableSocketHandle* s = (StableSocketHandle*)raw;
    if (!stable_socket_valid(s) || !data) return -1;
#if defined(_WIN32)
    int chunk = len > INT_MAX ? INT_MAX : (int)len;
    int rc = recvfrom(s->fd, (char*)data, chunk, 0, NULL, NULL);
#else
    ssize_t rc = recvfrom(s->fd, data, (size_t)len, 0, NULL, NULL);
#endif
    if (rc < 0) { stable_capture_socket_error(); return -1; }
    return (int64_t)rc;
}

int __stable_cpu_has_avx2(void) {
#if defined(__x86_64__) || defined(__i386__)
#if defined(__clang__) || defined(__GNUC__)
    return __builtin_cpu_supports("avx2") ? 1 : 0;
#else
    return 0;
#endif
#else
    return 0;
#endif
}

int __stable_cpu_has_avx512(void) {
#if defined(__x86_64__) || defined(__i386__)
#if defined(__clang__) || defined(__GNUC__)
    return __builtin_cpu_supports("avx512f") ? 1 : 0;
#else
    return 0;
#endif
#else
    return 0;
#endif
}

int __stable_cpu_has_sse42(void) {
#if defined(__x86_64__) || defined(__i386__)
#if defined(__clang__) || defined(__GNUC__)
    return __builtin_cpu_supports("sse4.2") ? 1 : 0;
#else
    return 0;
#endif
#else
    return 0;
#endif
}

int __stable_cpu_has_bmi2(void) {
#if defined(__x86_64__) || defined(__i386__)
#if defined(__clang__) || defined(__GNUC__)
    return __builtin_cpu_supports("bmi2") ? 1 : 0;
#else
    return 0;
#endif
#else
    return 0;
#endif
}

int __stable_cpu_has_popcnt(void) {
#if defined(__x86_64__) || defined(__i386__)
#if defined(__clang__) || defined(__GNUC__)
    return __builtin_cpu_supports("popcnt") ? 1 : 0;
#else
    return 0;
#endif
#else
    return 0;
#endif
}

uint64_t __stable_cpu_rdtsc(void) {
#if defined(__x86_64__) || defined(__i386__)
#if defined(__clang__)
    return __builtin_readcyclecounter();
#elif defined(__GNUC__)
    unsigned hi = 0, lo = 0;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | (uint64_t)lo;
#else
    return 0;
#endif
#else
    return 0;
#endif
}

uint64_t __stable_string_parse_u64_at(const char *s, uint64_t start) {
    if (!s) return 0;
    const char *p = s + start;
    while (*p == ' ' || *p == '\t') ++p;
    char *end = NULL;
    unsigned long long value = strtoull(p, &end, 10);
    return end == p ? 0ULL : (uint64_t)value;
}

int __stable_string_starts_with(const char *s, const char *prefix) {
    if (!s || !prefix) return 0;
    const size_t n = strlen(prefix);
    return strncmp(s, prefix, n) == 0 ? 1 : 0;
}

int __stable_string_equals(const char *a, const char *b) {
    if (!a || !b) return a == b ? 1 : 0;
    return strcmp(a, b) == 0 ? 1 : 0;
}


/* ---------------- Backend / Cloud runtime ---------------- */

static int stable_poller_reserve(StablePoller* p, size_t need) {
    if (need <= p->cap) return 1;
    size_t cap = p->cap ? p->cap * 2 : 16;
    while (cap < need) {
        if (cap > SIZE_MAX / 2) return 0;
        cap *= 2;
    }
    StablePollerEntry* next = (StablePollerEntry*)realloc(p->entries, cap * sizeof(*next));
    if (!next) return 0;
    p->entries = next;
    p->cap = cap;
    return 1;
}

static int stable_poller_ready_reserve(StablePoller* p, size_t need) {
    if (need <= p->ready_cap) return 1;
    size_t cap = p->ready_cap ? p->ready_cap * 2 : 16;
    while (cap < need) {
        if (cap > SIZE_MAX / 2) return 0;
        cap *= 2;
    }
    StablePollerReady* next = (StablePollerReady*)realloc(p->ready, cap * sizeof(*next));
    if (!next) return 0;
    p->ready = next;
    p->ready_cap = cap;
    return 1;
}

static void stable_poller_clear_ready(StablePoller* p) { p->ready_count = 0; }

#if defined(__linux__)
static uint32_t stable_poller_to_epoll(short events) {
    uint32_t out = 0;
    if (events & STABLE_POLLIN) out |= EPOLLIN;
    if (events & STABLE_POLLOUT) out |= EPOLLOUT;
    if (events & POLLPRI) out |= EPOLLPRI;
    return out;
}
#endif

int __stable_poller_read_events(void) { return STABLE_POLLIN; }
int __stable_poller_write_events(void) { return STABLE_POLLOUT; }
int __stable_poller_error_events(void) { return STABLE_POLLERR; }

void* __stable_poller_create(void) {
    StablePoller* p = (StablePoller*)calloc(1, sizeof(*p));
    if (!p) return NULL;
#if defined(__linux__)
    p->backend_fd = epoll_create1(EPOLL_CLOEXEC);
    if (p->backend_fd < 0) { free(p); return NULL; }
#endif
    return p;
}

int __stable_poller_add(void* raw, void* socket_raw, int events) {
    StablePoller* p = (StablePoller*)raw;
    StableSocketHandle* s = (StableSocketHandle*)socket_raw;
    if (!p || !stable_socket_valid(s)) return 0;
    for (size_t i = 0; i < p->count; ++i) {
        if (p->entries[i].socket == s) {
            p->entries[i].events = (short)events;
#if defined(__linux__)
            struct epoll_event ev;
            memset(&ev, 0, sizeof(ev));
            ev.events = stable_poller_to_epoll((short)events);
            ev.data.ptr = s;
            if (epoll_ctl(p->backend_fd, EPOLL_CTL_MOD, s->fd, &ev) != 0) return 0;
#endif
            return 1;
        }
    }
    if (!stable_poller_reserve(p, p->count + 1)) return 0;
#if defined(__linux__)
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = stable_poller_to_epoll((short)events);
    ev.data.ptr = s;
    if (epoll_ctl(p->backend_fd, EPOLL_CTL_ADD, s->fd, &ev) != 0) return 0;
#endif
    p->entries[p->count].socket = s;
    p->entries[p->count].events = (short)events;
    ++p->count;
    return 1;
}

int __stable_poller_remove(void* raw, void* socket_raw) {
    StablePoller* p = (StablePoller*)raw;
    if (!p || !socket_raw) return 0;
    for (size_t i = 0; i < p->count; ++i) {
        if (p->entries[i].socket == (StableSocketHandle*)socket_raw) {
#if defined(__linux__)
            (void)epoll_ctl(p->backend_fd, EPOLL_CTL_DEL, p->entries[i].socket->fd, NULL);
#endif
            p->entries[i] = p->entries[p->count - 1];
            --p->count;
            return 1;
        }
    }
    return 0;
}

int __stable_poller_wait(void* raw, int32_t timeout_ms) {
    StablePoller* p = (StablePoller*)raw;
    if (!p) return -1;
    stable_poller_clear_ready(p);
    if (p->count == 0) {
        if (timeout_ms > 0) __stable_clock_sleep_nanos((uint64_t)timeout_ms * 1000000ULL);
        return 0;
    }
#if defined(__linux__)
    size_t want = p->count < 64 ? 64 : p->count;
    if (!stable_poller_ready_reserve(p, want)) return -1;
    struct epoll_event* events = (struct epoll_event*)malloc(want * sizeof(*events));
    if (!events) return -1;
    int rc;
    do {
        rc = epoll_wait(p->backend_fd, events, want, timeout_ms < 0 ? -1 : timeout_ms);
    } while (rc < 0 && errno == EINTR);
    if (rc < 0) { stable_set_error(errno, strerror(errno)); free(events); return -1; }
    if (!stable_poller_ready_reserve(p, (size_t)rc)) { free(events); return -1; }
    for (int i = 0; i < rc; ++i) {
        int mask = 0;
        if (events[i].events & (uint32_t)(EPOLLIN | EPOLLPRI)) mask |= STABLE_POLLIN;
        if (events[i].events & EPOLLOUT) mask |= STABLE_POLLOUT;
        if (events[i].events & (uint32_t)(EPOLLERR | EPOLLHUP | EPOLLRDHUP)) mask |= STABLE_POLLERR;
        p->ready[p->ready_count].socket = (StableSocketHandle*)events[i].data.ptr;
        p->ready[p->ready_count].mask = mask;
        ++p->ready_count;
    }
    free(events);
    return rc;
#else
#if defined(_WIN32)
    WSAPOLLFD* fds = (WSAPOLLFD*)malloc(p->count * sizeof(*fds));
#else
    struct pollfd* fds = (struct pollfd*)malloc(p->count * sizeof(*fds));
#endif
    if (!fds) return -1;
    for (size_t i = 0; i < p->count; ++i) {
        fds[i].fd = p->entries[i].socket ? p->entries[i].socket->fd : STABLE_INVALID_SOCKET;
        fds[i].events = p->entries[i].events;
        fds[i].revents = 0;
    }
#if defined(_WIN32)
    int rc = WSAPoll(fds, (ULONG)p->count, timeout_ms < 0 ? -1 : timeout_ms);
#else
    int rc = poll(fds, p->count, timeout_ms < 0 ? -1 : timeout_ms);
#endif
    if (rc < 0) { stable_capture_socket_error(); free(fds); return -1; }
    if (!stable_poller_ready_reserve(p, (size_t)rc)) { free(fds); return -1; }
    for (size_t i = 0; i < p->count; ++i) {
        if (!fds[i].revents) continue;
        p->ready[p->ready_count].socket = p->entries[i].socket;
        p->ready[p->ready_count].mask = fds[i].revents;
        ++p->ready_count;
    }
    free(fds);
    return rc;
#endif
}

int __stable_poller_count(void* raw) {
    StablePoller* p = (StablePoller*)raw;
    return p ? (int)p->ready_count : 0;
}

void* __stable_poller_event_socket(void* raw, int32_t index) {
    StablePoller* p = (StablePoller*)raw;
    if (!p || index < 0 || (size_t)index >= p->ready_count) return NULL;
    return p->ready[index].socket;
}

int __stable_poller_event_mask(void* raw, int32_t index) {
    StablePoller* p = (StablePoller*)raw;
    if (!p || index < 0 || (size_t)index >= p->ready_count) return 0;
    return p->ready[index].mask;
}

void __stable_poller_destroy(void* raw) {
    StablePoller* p = (StablePoller*)raw;
    if (!p) return;
#if defined(__linux__)
    if (p->backend_fd >= 0) close(p->backend_fd);
#endif
    free(p->entries);
    free(p->ready);
    free(p);
}

/* Synchronization primitives. */
void* __stable_mutex_create(void) {
    StableMutexHandle* m = (StableMutexHandle*)calloc(1, sizeof(*m));
    if (!m) return NULL;
#if defined(_WIN32)
    InitializeCriticalSection(&m->value);
#else
    if (pthread_mutex_init(&m->value, NULL) != 0) { free(m); return NULL; }
#endif
    return m;
}
void __stable_mutex_lock(void* raw) {
    StableMutexHandle* m=(StableMutexHandle*)raw; if(!m)return;
#if defined(_WIN32)
    EnterCriticalSection(&m->value);
#else
    (void)pthread_mutex_lock(&m->value);
#endif
}
int __stable_mutex_try_lock(void* raw) {
    StableMutexHandle* m=(StableMutexHandle*)raw; if(!m)return 0;
#if defined(_WIN32)
    return TryEnterCriticalSection(&m->value) ? 1 : 0;
#else
    return pthread_mutex_trylock(&m->value) == 0 ? 1 : 0;
#endif
}
void __stable_mutex_unlock(void* raw) {
    StableMutexHandle* m=(StableMutexHandle*)raw; if(!m)return;
#if defined(_WIN32)
    LeaveCriticalSection(&m->value);
#else
    (void)pthread_mutex_unlock(&m->value);
#endif
}
void __stable_mutex_destroy(void* raw) {
    StableMutexHandle* m=(StableMutexHandle*)raw; if(!m)return;
#if defined(_WIN32)
    DeleteCriticalSection(&m->value);
#else
    (void)pthread_mutex_destroy(&m->value);
#endif
    free(m);
}

void* __stable_rwlock_create(void) {
    StableRWLockHandle* r=(StableRWLockHandle*)calloc(1,sizeof(*r)); if(!r)return NULL;
#if defined(_WIN32)
    InitializeSRWLock(&r->value);
#else
    if(pthread_rwlock_init(&r->value,NULL)!=0){free(r);return NULL;}
#endif
    return r;
}
void __stable_rwlock_read_lock(void* raw) {
    StableRWLockHandle* r=(StableRWLockHandle*)raw;if(!r)return;
#if defined(_WIN32)
    AcquireSRWLockShared(&r->value);
#else
    (void)pthread_rwlock_rdlock(&r->value);
#endif
}
void __stable_rwlock_write_lock(void* raw) {
    StableRWLockHandle* r=(StableRWLockHandle*)raw;if(!r)return;
#if defined(_WIN32)
    AcquireSRWLockExclusive(&r->value);
#else
    (void)pthread_rwlock_wrlock(&r->value);
#endif
}
int __stable_rwlock_try_read_lock(void* raw) {
    StableRWLockHandle* r=(StableRWLockHandle*)raw;if(!r)return 0;
#if defined(_WIN32)
    return TryAcquireSRWLockShared(&r->value) ? 1 : 0;
#else
    return pthread_rwlock_tryrdlock(&r->value) == 0 ? 1 : 0;
#endif
}
int __stable_rwlock_try_write_lock(void* raw) {
    StableRWLockHandle* r=(StableRWLockHandle*)raw;if(!r)return 0;
#if defined(_WIN32)
    return TryAcquireSRWLockExclusive(&r->value) ? 1 : 0;
#else
    return pthread_rwlock_trywrlock(&r->value) == 0 ? 1 : 0;
#endif
}
void __stable_rwlock_unlock(void* raw, int writer) {
    StableRWLockHandle* r=(StableRWLockHandle*)raw;if(!r)return;
#if defined(_WIN32)
    if(writer) ReleaseSRWLockExclusive(&r->value); else ReleaseSRWLockShared(&r->value);
#else
    (void)writer; (void)pthread_rwlock_unlock(&r->value);
#endif
}
void __stable_rwlock_destroy(void* raw) {
    StableRWLockHandle* r=(StableRWLockHandle*)raw;if(!r)return;
#if defined(_WIN32)
    (void)r;
#else
    (void)pthread_rwlock_destroy(&r->value);
#endif
    free(r);
}

void* __stable_condvar_create(void) {
    StableCondvarHandle* c=(StableCondvarHandle*)calloc(1,sizeof(*c));if(!c)return NULL;
#if defined(_WIN32)
    InitializeConditionVariable(&c->value);
#else
    if(pthread_cond_init(&c->value,NULL)!=0){free(c);return NULL;}
#endif
    return c;
}
int __stable_condvar_wait(void* raw, void* mutex_raw, int32_t timeout_ms) {
    StableCondvarHandle* c=(StableCondvarHandle*)raw; StableMutexHandle* m=(StableMutexHandle*)mutex_raw;
    if(!c||!m)return 0;
#if defined(_WIN32)
    DWORD timeout = timeout_ms < 0 ? INFINITE : (DWORD)timeout_ms;
    return SleepConditionVariableCS(&c->value,&m->value,timeout) ? 1 : 0;
#else
    if(timeout_ms < 0) return pthread_cond_wait(&c->value,&m->value) == 0 ? 1 : 0;
    struct timespec ts;
    if(clock_gettime(CLOCK_REALTIME,&ts)!=0) return 0;
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (long)(timeout_ms % 1000) * 1000000L;
    if(ts.tv_nsec >= 1000000000L){ ts.tv_sec += 1; ts.tv_nsec -= 1000000000L; }
    return pthread_cond_timedwait(&c->value,&m->value,&ts) == 0 ? 1 : 0;
#endif
}
void __stable_condvar_signal(void* raw) {
    StableCondvarHandle* c=(StableCondvarHandle*)raw;if(!c)return;
#if defined(_WIN32)
    WakeConditionVariable(&c->value);
#else
    (void)pthread_cond_signal(&c->value);
#endif
}
void __stable_condvar_broadcast(void* raw) {
    StableCondvarHandle* c=(StableCondvarHandle*)raw;if(!c)return;
#if defined(_WIN32)
    WakeAllConditionVariable(&c->value);
#else
    (void)pthread_cond_broadcast(&c->value);
#endif
}
void __stable_condvar_destroy(void* raw) {
    StableCondvarHandle* c=(StableCondvarHandle*)raw;if(!c)return;
#if defined(_WIN32)
    (void)c;
#else
    (void)pthread_cond_destroy(&c->value);
#endif
    free(c);
}

void* __stable_semaphore_create(uint32_t initial) {
    StableSemaphoreHandle* s=(StableSemaphoreHandle*)calloc(1,sizeof(*s));if(!s)return NULL;
#if defined(_WIN32)
    s->value=CreateSemaphoreA(NULL,(LONG)initial,LONG_MAX,NULL);
    if(!s->value){free(s);return NULL;}
#else
    if(pthread_mutex_init(&s->mutex,NULL)!=0 || pthread_cond_init(&s->cond,NULL)!=0){pthread_mutex_destroy(&s->mutex);pthread_cond_destroy(&s->cond);free(s);return NULL;}
    s->value=initial;
#endif
    return s;
}
int __stable_semaphore_wait(void* raw,int32_t timeout_ms){
    StableSemaphoreHandle* s=(StableSemaphoreHandle*)raw;if(!s)return 0;
#if defined(_WIN32)
    DWORD timeout=timeout_ms<0?INFINITE:(DWORD)timeout_ms;
    return WaitForSingleObject(s->value,timeout)==WAIT_OBJECT_0?1:0;
#else
    if(pthread_mutex_lock(&s->mutex)!=0)return 0;
    while(s->value==0){
        if(timeout_ms<0){if(pthread_cond_wait(&s->cond,&s->mutex)!=0){pthread_mutex_unlock(&s->mutex);return 0;}}
        else {struct timespec ts;if(clock_gettime(CLOCK_REALTIME,&ts)!=0){pthread_mutex_unlock(&s->mutex);return 0;}ts.tv_sec+=timeout_ms/1000;ts.tv_nsec+=(long)(timeout_ms%1000)*1000000L;if(ts.tv_nsec>=1000000000L){ts.tv_sec+=1;ts.tv_nsec-=1000000000L;}int rc=pthread_cond_timedwait(&s->cond,&s->mutex,&ts);if(rc!=0){pthread_mutex_unlock(&s->mutex);return 0;}}
    }
    --s->value;
    pthread_mutex_unlock(&s->mutex);
    return 1;
#endif
}
int __stable_semaphore_try_wait(void* raw){
    StableSemaphoreHandle* s=(StableSemaphoreHandle*)raw;if(!s)return 0;
#if defined(_WIN32)
    return WaitForSingleObject(s->value,0)==WAIT_OBJECT_0?1:0;
#else
    if(pthread_mutex_lock(&s->mutex)!=0)return 0;
    int ok=s->value>0?1:0;if(ok)--s->value;pthread_mutex_unlock(&s->mutex);return ok;
#endif
}
void __stable_semaphore_post(void* raw){
    StableSemaphoreHandle* s=(StableSemaphoreHandle*)raw;if(!s)return;
#if defined(_WIN32)
    (void)ReleaseSemaphore(s->value,1,NULL);
#else
    if (pthread_mutex_lock(&s->mutex) != 0) return;
    s->value += 1;
    pthread_cond_signal(&s->cond);
    pthread_mutex_unlock(&s->mutex);
#endif
}
void __stable_semaphore_destroy(void* raw){
    StableSemaphoreHandle* s=(StableSemaphoreHandle*)raw;if(!s)return;
#if defined(_WIN32)
    CloseHandle(s->value);
#else
    pthread_cond_destroy(&s->cond);pthread_mutex_destroy(&s->mutex);
#endif
    free(s);
}

int __stable_process_run(const char* command) {
    if (!command) return -1;
    int rc = system(command);
#if defined(_WIN32)
    return rc;
#else
    if (rc < 0) return -1;
    if (WIFEXITED(rc)) return WEXITSTATUS(rc);
    if (WIFSIGNALED(rc)) return 128 + WTERMSIG(rc);
    return rc;
#endif
}

void* __stable_process_spawn(const char* command) {
    if (!command) return NULL;
    StableProcessHandle* p = (StableProcessHandle*)calloc(1, sizeof(*p));
    if (!p) return NULL;
#if defined(_WIN32)
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);
    const size_t command_len = strlen(command);
    const char* shell = "cmd.exe /C ";
    char* cmd = (char*)malloc(strlen(shell) + command_len + 1);
    if (!cmd) { free(p); return NULL; }
    memcpy(cmd, shell, strlen(shell));
    memcpy(cmd + strlen(shell), command, command_len + 1);
    BOOL ok = CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    free(cmd);
    if (!ok) { free(p); return NULL; }
    CloseHandle(pi.hThread);
    p->process = pi.hProcess;
    p->pid = pi.dwProcessId;
#else
    pid_t pid = fork();
    if (pid < 0) { free(p); return NULL; }
    if (pid == 0) {
        execl("/bin/sh", "sh", "-c", command, (char*)NULL);
        _exit(127);
    }
    p->pid = pid;
#endif
    return p;
}

int __stable_process_wait(void* raw) {
    StableProcessHandle* p = (StableProcessHandle*)raw;
    if (!p) return -1;
#if defined(_WIN32)
    if (!p->process) { free(p); return -1; }
    DWORD wait_rc = WaitForSingleObject(p->process, INFINITE);
    DWORD code = 1;
    if (wait_rc == WAIT_OBJECT_0) (void)GetExitCodeProcess(p->process, &code);
    CloseHandle(p->process);
    free(p);
    return wait_rc == WAIT_OBJECT_0 ? (int)code : -1;
#else
    int status = 0;
    pid_t rc;
    do { rc = waitpid(p->pid, &status, 0); } while (rc < 0 && errno == EINTR);
    free(p);
    if (rc < 0) return -1;
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return status;
#endif
}

uint64_t __stable_process_pid(void* raw) {
    StableProcessHandle* p = (StableProcessHandle*)raw;
    if (!p) return 0;
#if defined(_WIN32)
    return (uint64_t)p->pid;
#else
    return p->pid > 0 ? (uint64_t)p->pid : 0;
#endif
}

int __stable_process_terminate(void* raw) {
    StableProcessHandle* p = (StableProcessHandle*)raw;
    if (!p) return 0;
#if defined(_WIN32)
    return p->process && TerminateProcess(p->process, 1) ? 1 : 0;
#else
    return p->pid > 0 && kill(p->pid, SIGTERM) == 0 ? 1 : 0;
#endif
}

uint64_t __stable_process_arg_count(int argc) {
    (void)argc;
#if defined(_WIN32)
    int n = 0;
    char** args = __argv;
    while (args && args[n]) ++n;
    return (uint64_t)n;
#else
    return 0;
#endif
}

static int stable_argc_saved = 0;
static char** stable_argv_saved = NULL;
void __stable_process_set_argv(int argc, char** argv) { stable_argc_saved = argc; stable_argv_saved = argv; }
uint64_t __stable_process_argc(void) { return (uint64_t)(stable_argc_saved < 0 ? 0 : stable_argc_saved); }
const char* __stable_process_argv_at(uint64_t index) {
    if (!stable_argv_saved || index >= (uint64_t)stable_argc_saved) return "";
    return stable_argv_saved[index] ? stable_argv_saved[index] : "";
}

int __stable_set_env(const char* name, const char* value) {
    if (!name || !*name || !value) return 0;
#if defined(_WIN32)
    return _putenv_s(name, value) == 0 ? 1 : 0;
#else
    return setenv(name, value, 1) == 0 ? 1 : 0;
#endif
}

void* __stable_process_output(const char* command);

static StableBuffer* stable_buffer_new(size_t cap) {
    StableBuffer* b = (StableBuffer*)calloc(1, sizeof(*b));
    if (!b) return NULL;
    b->cap = cap ? cap : 1;
    b->data = (unsigned char*)malloc(b->cap);
    if (!b->data) { free(b); return NULL; }
    return b;
}

static int stable_buffer_reserve(StableBuffer* b, size_t need) {
    if (need <= b->cap) return 1;
    size_t cap = b->cap;
    while (cap < need) {
        if (cap > SIZE_MAX / 2) return 0;
        cap *= 2;
    }
    unsigned char* data = (unsigned char*)realloc(b->data, cap);
    if (!data) return 0;
    b->data = data; b->cap = cap;
    return 1;
}

static int stable_buffer_append(StableBuffer* b, const void* data, size_t len) {
    if (!stable_buffer_reserve(b, b->len + len + 1)) return 0;
    memcpy(b->data + b->len, data, len);
    b->len += len;
    b->data[b->len] = 0;
    return 1;
}

void* __stable_buffer_new(uint64_t capacity) { return stable_buffer_new((size_t)capacity); }
void* ____stable_buffer_from_string(const char* text) {
    if (!text) return NULL;
    StableBuffer* b = stable_buffer_new(strlen(text) + 1);
    if (!b) return NULL;
    if (!stable_buffer_append(b, text, strlen(text))) { free(b->data); free(b); return NULL; }
    return b;
}
uint64_t __stable_buffer_len(void* raw) { StableBuffer* b=(StableBuffer*)raw; return b ? (uint64_t)b->len : 0; }
void* __stable_buffer_data(void* raw) { StableBuffer* b=(StableBuffer*)raw; return b ? b->data : NULL; }
const char* __stable_buffer_cstr(void* raw) { StableBuffer* b=(StableBuffer*)raw; return b && b->data ? (const char*)b->data : ""; }
void __stable_buffer_free(void* raw) { StableBuffer* b=(StableBuffer*)raw; if(!b)return; free(b->data); free(b); }


int __stable_env_has(const char* name){ return name && getenv(name) ? 1 : 0; }
int __stable_set_env_value(const char* name,const char* value){ return __stable_set_env(name,value); }
int __stable_set_env_unset(const char* name){
    if(!name||!*name)return 0;
#if defined(_WIN32)
    return _putenv_s(name,"")==0 ? 1:0;
#else
    return unsetenv(name)==0 ? 1:0;
#endif
}
static int stable_fs_stat(const char* p,struct stat* st){return p&&st&&stat(p,st)==0;}
int __stable_fs_exists(const char*p){struct stat st;return stable_fs_stat(p,&st);}
int __stable_fs_isFile(const char*p){struct stat st;return stable_fs_stat(p,&st)&&((st.st_mode&S_IFMT)==S_IFREG);}
int __stable_fs_isDir(const char*p){struct stat st;return stable_fs_stat(p,&st)&&((st.st_mode&S_IFMT)==S_IFDIR);}
int64_t __stable_fs_file_size(const char*p){struct stat st;if(!stable_fs_stat(p,&st)||st.st_size<0)return -1;return(int64_t)st.st_size;}
void* __stable_fs_read(const char*p){if(!p)return NULL;FILE*f=fopen(p,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)!=0){fclose(f);return NULL;}long sz=ftell(f);if(sz<0){fclose(f);return NULL;}rewind(f);StableBuffer*b=stable_buffer_new((size_t)sz+1);if(!b){fclose(f);return NULL;}b->len=fread(b->data,1,(size_t)sz,f);b->data[b->len]=0;fclose(f);return b;}
static int stable_fs_write_core(const char*p,const void*d,size_t n,const char*mode){if(!p||(!d&&n))return 0;FILE*f=fopen(p,mode);if(!f)return 0;size_t w=fwrite(d,1,n,f);int ok=(w==n&&fclose(f)==0);if(w!=n)fclose(f);return ok;}
int __stable_fs_write(const char*p,const char*t){return t?stable_fs_write_core(p,t,strlen(t),"wb"):0;}
int __stable_fs_append(const char*p,const char*t){return t?stable_fs_write_core(p,t,strlen(t),"ab"):0;}
int __stable_fs_write_buffer(const char*p,const void*d,uint64_t n){return stable_fs_write_core(p,d,(size_t)n,"wb");}
int __stable_fs_remove(const char*p){
#if defined(_WIN32)
return p&&DeleteFileA(p)?1:0;
#else
return p&&unlink(p)==0?1:0;
#endif
}
int __stable_fs_mkdir(const char*p){
#if defined(_WIN32)
return p&&_mkdir(p)==0?1:0;
#else
return p&&mkdir(p,0777)==0?1:0;
#endif
}
int __stable_fs_rmdir(const char*p){
#if defined(_WIN32)
return p&&_rmdir(p)==0?1:0;
#else
return p&&rmdir(p)==0?1:0;
#endif
}
int __stable_fs_rename(const char*a,const char*b){return a&&b&&rename(a,b)==0?1:0;}
int __stable_fs_copy(const char*a,const char*b){if(!a||!b)return 0;FILE*in=fopen(a,"rb");if(!in)return 0;FILE*out=fopen(b,"wb");if(!out){fclose(in);return 0;}char buf[65536];size_t n;int ok=1;while((n=fread(buf,1,sizeof(buf),in))>0){if(fwrite(buf,1,n,out)!=n){ok=0;break;}}if(ferror(in))ok=0;if(fclose(in)!=0)ok=0;if(fclose(out)!=0)ok=0;return ok;}
static STABLE_TLS char stable_fs_cwd_buf[4096];
const char* __stable_fs_cwd(void){
#if defined(_WIN32)
return _getcwd(stable_fs_cwd_buf,sizeof(stable_fs_cwd_buf))?stable_fs_cwd_buf:"";
#else
return getcwd(stable_fs_cwd_buf,sizeof(stable_fs_cwd_buf))?stable_fs_cwd_buf:"";
#endif
}
int __stable_fs_chdir(const char*p){
#if defined(_WIN32)
return p&&_chdir(p)==0?1:0;
#else
return p&&chdir(p)==0?1:0;
#endif
}
void* __stable_fs_list(const char*p){if(!p)return NULL;size_t cap=256,len=0;char*out=(char*)malloc(cap);if(!out)return NULL;out[0]=0;
#if defined(_WIN32)
char pattern[4096];snprintf(pattern,sizeof(pattern),"%s\\*",p);WIN32_FIND_DATAA fd;HANDLE h=FindFirstFileA(pattern,&fd);if(h==INVALID_HANDLE_VALUE){free(out);return NULL;}do{const char*n=fd.cFileName;if(strcmp(n,".")==0||strcmp(n,"..")==0)continue;size_t z=strlen(n);if(len+z+2>cap){while(len+z+2>cap)cap*=2;char*t=(char*)realloc(out,cap);if(!t){FindClose(h);free(out);return NULL;}out=t;}memcpy(out+len,n,z);len+=z;out[len++]='\n';out[len]=0;}while(FindNextFileA(h,&fd));FindClose(h);
#else
DIR*d=opendir(p);if(!d){free(out);return NULL;}struct dirent*e;while((e=readdir(d))){const char*n=e->d_name;if(strcmp(n,".")==0||strcmp(n,"..")==0)continue;size_t z=strlen(n);if(len+z+2>cap){while(len+z+2>cap)cap*=2;char*t=(char*)realloc(out,cap);if(!t){closedir(d);free(out);return NULL;}out=t;}memcpy(out+len,n,z);len+=z;out[len++]='\n';out[len]=0;}closedir(d);
#endif
StableBuffer*b=stable_buffer_new(len+1);if(!b){free(out);return NULL;}stable_buffer_append(b,out,len);free(out);return b;}
static STABLE_TLS char stable_path_ring[8][4096];static STABLE_TLS unsigned stable_path_slot;static char* stable_path_buf(void){char*b=stable_path_ring[stable_path_slot++&7u];b[0]=0;return b;}
static const char* stable_last_sep(const char*p){const char*a=strrchr(p,'/');const char*b=strrchr(p,'\\');return a>b?a:b;}
const char* __stable_path_join(const char*a,const char*b){char*out=stable_path_buf();if(!a)return out;if(!b||!*b){snprintf(out,4096,"%s",a);return out;}size_t n=strlen(a);int sep=n&&a[n-1]!='/'&&a[n-1]!='\\';snprintf(out,4096,"%s%s%s",a,sep?"/":"",b);return out;}
const char* __stable_path_basename(const char*p){char*out=stable_path_buf();if(!p)return out;const char*s=stable_last_sep(p);snprintf(out,4096,"%s",s?s+1:p);return out;}
const char* __stable_path_dirname(const char*p){char*out=stable_path_buf();if(!p){snprintf(out,4096,".");return out;}const char*s=stable_last_sep(p);if(!s){snprintf(out,4096,".");return out;}if(s==p){snprintf(out,4096,"%c",*s);return out;}size_t n=(size_t)(s-p);if(n>4095)n=4095;memcpy(out,p,n);out[n]=0;return out;}
const char* __stable_path_extension(const char*p){char*out=stable_path_buf();if(!p)return out;const char*b=stable_last_sep(p);b=b?b+1:p;const char*d=strrchr(b,'.');if(!d||d==b){out[0]=0;return out;}snprintf(out,4096,"%s",d);return out;}
const char* __stable_path_stem(const char*p){char*out=stable_path_buf();if(!p)return out;const char*b=stable_last_sep(p);b=b?b+1:p;snprintf(out,4096,"%s",b);char*d=strrchr(out,'.');if(d&&d!=out)*d=0;return out;}
const char* __stable_path_normalize(const char*p){char*out=stable_path_buf();if(!p)return out;snprintf(out,4096,"%s",p);size_t w=0;for(size_t i=0;out[i];++i){if(out[i]=='\\')out[i]='/';if(out[i]=='/'&&w&&out[w-1]=='/')continue;out[w++]=out[i];}out[w]=0;return out;}
int __stable_path_is_absolute(const char*p){if(!p||!*p)return 0;
#if defined(_WIN32)
return ((strlen(p)>=3&&((p[0]>='A'&&p[0]<='Z')||(p[0]>='a'&&p[0]<='z'))&&p[1]==':'&&(p[2]=='/'||p[2]=='\\'))||(strlen(p)>=2&&p[0]=='\\'&&p[1]=='\\'))?1:0;
#else
return p[0]=='/'?1:0;
#endif
}
const char* __stable_path_absolute(const char*p){char*out=stable_path_buf();if(!p)return out;
#if defined(_WIN32)
if(!_fullpath(out,p,4096))out[0]=0;
#else
if(__stable_path_is_absolute(p)){snprintf(out,4096,"%s",p);}else{snprintf(out,4096,"%s/%s",__stable_fs_cwd(),p);}
#endif
return out;}

typedef struct StableRegex{char*pattern;}StableRegex;

static int rx_class_match(const char **pp, char ch) {
    const char *p=*pp;
    if (*p!='[') return 0;
    p++;
    int neg=0, hit=0;
    if (*p=='^') { neg=1; p++; }
    int closed=0;
    while (*p) {
        if (*p==']') { closed=1; p++; break; }
        unsigned char lo=(unsigned char)*p++;
        if (lo=='\\') {
            if (!*p) return -1;
            lo=(unsigned char)*p++;
        }
        if (*p=='-' && p[1] && p[1]!=']') {
            p++;
            unsigned char hi=(unsigned char)*p++;
            if (hi=='\\') {
                if (!*p) return -1;
                hi=(unsigned char)*p++;
            }
            if ((unsigned char)ch>=lo && (unsigned char)ch<=hi) hit=1;
        } else if ((unsigned char)ch==lo) {
            hit=1;
        }
    }
    if (!closed) return -1;
    *pp=p;
    return neg ? !hit : hit;
}

static int rx_atom_match(const char **pp, char ch) {
    const char *p=*pp;
    if (!*p) return 0;
    if (*p=='[') return rx_class_match(pp,ch);
    if (*p=='\\') {
        if (!p[1]) return -1;
        *pp=p+2;
        return (unsigned char)ch==(unsigned char)p[1];
    }
    *pp=p+1;
    return *p=='.' || *p==ch;
}

static int rx_valid_pattern(const char *p) {
    if (!p) return 0;
    int atomAvailable=0;
    int first=1;
    while (*p) {
        if (*p=='^') {
            if (!first) return 0;
            ++p; first=0; continue;
        }
        first=0;
        if (*p=='$') {
            if (p[1]) return 0;
            ++p; atomAvailable=0; continue;
        }
        const char *next=p;
        char probe='x';
        int m=rx_atom_match(&next,probe);
        if (m<0) return 0;
        if (next==p) return 0;
        atomAvailable=1;
        p=next;
        if (*p=='*'||*p=='+'||*p=='?') {
            if (!atomAvailable) return 0;
            ++p;
        }
    }
    return 1;
}

static int rx_match_here(const char *p, const char *s) {
    if (!*p) return 1;
    if (*p=='$' && !p[1]) return *s==0;

    const char *atomEnd=p;
    int probe=rx_atom_match(&atomEnd, *s ? *s : '\0');
    if (probe<0) return 0;
    if (atomEnd==p) return 0;
    const char quant=*atomEnd;
    const char *rest=(quant=='*'||quant=='+'||quant=='?') ? atomEnd+1 : atomEnd;

    if (quant!='*' && quant!='+' && quant!='?') {
        if (!*s || !probe) return 0;
        return rx_match_here(rest,s+1);
    }

    if (quant=='?') {
        if (rx_match_here(rest,s)) return 1;
        if (*s && probe) return rx_match_here(rest,s+1);
        return 0;
    }

    const char *q=s;
    size_t count=0;
    if (quant=='+') {
        if (!*q || !probe) return 0;
        ++q; ++count;
    }
    while (*q) {
        const char *after=q;
        const int hit=rx_atom_match(&after,*q);
        if (hit<=0) break;
        q=after==q ? q+1 : q+1;
        ++count;
        (void)count;
    }
    for (;;) {
        if (rx_match_here(rest,q)) return 1;
        if (q==s) break;
        --q;
    }
    return 0;
}

static int rx_match(const char *p,const char*s){
    if (!rx_valid_pattern(p) || !s) return 0;
    if (*p=='^') return rx_match_here(p+1,s);
    for(const char*q=s;;++q){if(rx_match_here(p,q))return 1;if(!*q)break;}
    return 0;
}

void* __stable_regex_compile(const char*p){
    if(!rx_valid_pattern(p))return NULL;
    StableRegex*r=(StableRegex*)calloc(1,sizeof(*r));
    if(!r)return NULL;
    r->pattern=(char*)malloc(strlen(p)+1);
    if(!r->pattern){free(r);return NULL;}
    strcpy(r->pattern,p);
    return r;
}
int __stable_regex_match(void*raw,const char*s){StableRegex*r=(StableRegex*)raw;return r&&s?rx_match(r->pattern,s):0;}
int64_t __stable_regex_find(void*raw,const char*s){StableRegex*r=(StableRegex*)raw;if(!r||!s)return -1;if(r->pattern[0]=='^')return rx_match_here(r->pattern+1,s)?0:-1;for(size_t i=0;;++i){if(rx_match_here(r->pattern,s+i))return(int64_t)i;if(!s[i])break;}return -1;}
void __stable_regex_free(void*raw){StableRegex*r=(StableRegex*)raw;if(!r)return;free(r->pattern);free(r);}

const char* __stable_shell_which(const char*name){static STABLE_TLS char out[4096];out[0]=0;if(!name||!*name)return out;const char*path=getenv("PATH");if(!path)return out;
#if defined(_WIN32)
char*copy=_strdup(path);if(!copy)return out;char*ctx=NULL;for(char*t=strtok_s(copy,";",&ctx);t;t=strtok_s(NULL,";",&ctx)){snprintf(out,sizeof(out),"%s\\%s.exe",t,name);if(__stable_fs_isFile(out)){free(copy);return out;}snprintf(out,sizeof(out),"%s\\%s",t,name);if(__stable_fs_isFile(out)){free(copy);return out;}}free(copy);
#else
char*copy=strdup(path);if(!copy)return out;char*ctx=NULL;for(char*t=strtok_r(copy,":",&ctx);t;t=strtok_r(NULL,":",&ctx)){snprintf(out,sizeof(out),"%s/%s",t,name);if(access(out,X_OK)==0){free(copy);return out;}}free(copy);
#endif
out[0]=0;return out;}
int64_t __stable_string_parse_i64(const char*s){if(!s)return 0;char*e=NULL;long long v=strtoll(s,&e,0);return(e==s)?0:(int64_t)v;}
double __stable_string_parse_f64(const char*s){if(!s)return 0.0;char*e=NULL;double v=strtod(s,&e);return(e==s)?0.0:v;}
int __stable_string_contains(const char*a,const char*b){return a&&b&&strstr(a,b)?1:0;}
int __stable_string_startsWith(const char*a,const char*b){if(!a||!b)return 0;size_t n=strlen(b);return strncmp(a,b,n)==0;}
int __stable_string_endsWith(const char*a,const char*b){if(!a||!b)return 0;size_t na=strlen(a),nb=strlen(b);return nb<=na&&memcmp(a+na-nb,b,nb)==0;}
int __stable_string_equalsIgnoreCase(const char*a,const char*b){if(!a||!b)return 0;
#if defined(_WIN32)
return _stricmp(a,b)==0;
#else
return strcasecmp(a,b)==0;
#endif
}
int64_t __stable_string_find(const char*a,const char*b){if(!a||!b)return -1;const char*q=strstr(a,b);return q?(int64_t)(q-a):-1;}
int __stable_buffer_append_string(void* raw, const char* text) { StableBuffer* b=(StableBuffer*)raw; if(!b||!text)return 0; return stable_buffer_append(b,text,strlen(text)); }
int __stable_buffer_append_buffer(void* raw, void* other) { StableBuffer* b=(StableBuffer*)raw; StableBuffer* o=(StableBuffer*)other; if(!b||!o)return 0; return stable_buffer_append(b,o->data,o->len); }

void* __stable_process_output(const char* command) {
    if (!command) return NULL;
#if defined(_WIN32)
    FILE* pipe = _popen(command, "r");
#else
    FILE* pipe = popen(command, "r");
#endif
    if (!pipe) return NULL;
    StableBuffer* out = stable_buffer_new(4096);
    if (!out) {
#if defined(_WIN32)
        _pclose(pipe);
#else
        pclose(pipe);
#endif
        return NULL;
    }
    unsigned char chunk[4096];
    size_t n;
    while ((n = fread(chunk, 1, sizeof(chunk), pipe)) != 0) {
        if (!stable_buffer_append(out, chunk, n)) {
            __stable_buffer_free(out);
#if defined(_WIN32)
            _pclose(pipe);
#else
            pclose(pipe);
#endif
            return NULL;
        }
    }
#if defined(_WIN32)
    (void)_pclose(pipe);
#else
    (void)pclose(pipe);
#endif
    return out;
}

void* __stable_json_int(int64_t value) { char tmp[64]; snprintf(tmp,sizeof(tmp),"%lld",(long long)value); return ____stable_buffer_from_string(tmp); }
void* __stable_json_float(double value) {
    if (!isfinite(value)) return NULL;
    char tmp[128];
    int n = snprintf(tmp,sizeof(tmp),"%.17g",value);
    if (n < 0 || (size_t)n >= sizeof(tmp)) return NULL;
    return ____stable_buffer_from_string(tmp);
}
void* __stable_json_bool(int value) { return ____stable_buffer_from_string(value ? "true" : "false"); }
void* __stable_json_null(void) { return ____stable_buffer_from_string("null"); }

static int stable_url_parse(const char* url, const char** host, size_t* host_len, uint32_t* port, const char** path) {
    if (!url) return 0;
    const char* p = NULL;
    uint32_t default_port = 80;
    if (strncmp(url, "http://", 7) == 0) { p = url + 7; default_port = 80; }
    else if (strncmp(url, "https://", 8) == 0) { stable_set_error(-2, "built-in Http supports http://; use TLS through FFI for https://"); return 0; }
    else { stable_set_error(-2, "URL must use http:// or https://"); return 0; }
    const char* slash = strchr(p, '/');
    const char* end = slash ? slash : p + strlen(p);
    const char* colon = NULL;
    if (p < end && *p == '[') {
        const char* close = strchr(p, ']');
        if (close && close < end) { if (close + 1 < end && close[1] == ':') colon = close + 1; }
    } else {
        for (const char* q = p; q < end; ++q) if (*q == ':') colon = q;
    }
    *host = p;
    *host_len = (size_t)(colon ? colon - p : end - p);
    *port = default_port;
    if (colon) {
        unsigned long parsed = strtoul(colon + 1, NULL, 10);
        if (parsed > 65535UL) return 0;
        *port = (uint32_t)parsed;
    }
    *path = slash ? slash : "/";
    return *host_len > 0;
}

static char* stable_copy_range(const char* p, size_t n) {
    char* out = (char*)malloc(n + 1);
    if (!out) return NULL;
    memcpy(out, p, n); out[n] = 0; return out;
}

static int stable_ci_n_equal(const char* a, const char* b, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];
        if (ca >= 'A' && ca <= 'Z') ca = (unsigned char)(ca + ('a' - 'A'));
        if (cb >= 'A' && cb <= 'Z') cb = (unsigned char)(cb + ('a' - 'A'));
        if (ca != cb) return 0;
    }
    return 1;
}

static int stable_ci_contains(const char* text, size_t len, const char* needle) {
    const size_t n = strlen(needle);
    if (n == 0 || n > len) return 0;
    for (size_t i = 0; i + n <= len; ++i) if (stable_ci_n_equal(text + i, needle, n)) return 1;
    return 0;
}

static int stable_http_header_value(const unsigned char* data, size_t header_len, const char* name, const char** value, size_t* value_len) {
    size_t pos = 0;
    const size_t name_len = strlen(name);
    while (pos < header_len) {
        size_t end = pos;
        while (end + 1 < header_len && !(data[end] == '\r' && data[end + 1] == '\n')) ++end;
        if (end >= pos + name_len + 1 && stable_ci_n_equal((const char*)data + pos, name, name_len) && data[pos + name_len] == ':') {
            size_t v = pos + name_len + 1;
            while (v < end && (data[v] == ' ' || data[v] == '\t')) ++v;
            size_t ve = end;
            while (ve > v && (data[ve - 1] == ' ' || data[ve - 1] == '\t')) --ve;
            *value = (const char*)data + v;
            *value_len = ve - v;
            return 1;
        }
        if (end + 1 >= header_len) break;
        pos = end + 2;
    }
    return 0;
}

static StableBuffer* stable_http_decode_chunked(const unsigned char* data, size_t len) {
    StableBuffer* out = stable_buffer_new(len + 1);
    if (!out) return NULL;
    size_t pos = 0;
    while (pos < len) {
        size_t line_end = pos;
        while (line_end + 1 < len && !(data[line_end] == '\r' && data[line_end + 1] == '\n')) ++line_end;
        if (line_end + 1 >= len) { __stable_buffer_free(out); return NULL; }
        char line[64];
        size_t line_len = line_end - pos;
        if (line_len >= sizeof(line)) { __stable_buffer_free(out); return NULL; }
        memcpy(line, data + pos, line_len); line[line_len] = '\0';
        char* semicolon = strchr(line, ';');
        if (semicolon) *semicolon = '\0';
        char* endptr = NULL;
        unsigned long long chunk_size = strtoull(line, &endptr, 16);
        if (endptr == line) { __stable_buffer_free(out); return NULL; }
        pos = line_end + 2;
        if (chunk_size == 0) return out;
        if (chunk_size > SIZE_MAX || (size_t)chunk_size > len - pos) { __stable_buffer_free(out); return NULL; }
        if (!stable_buffer_append(out, data + pos, (size_t)chunk_size)) { __stable_buffer_free(out); return NULL; }
        pos += (size_t)chunk_size;
        if (pos + 2 > len || data[pos] != '\r' || data[pos + 1] != '\n') { __stable_buffer_free(out); return NULL; }
        pos += 2;
    }
    return out;
}

static StableBuffer* stable_http_read_response(StableSocketHandle* s) {
    StableBuffer* all = stable_buffer_new(8192);
    if (!all) return NULL;
    unsigned char chunk[8192];
    for (;;) {
        int64_t n = __stable_net_recv(s, chunk, sizeof(chunk));
        if (n == 0) break;
        if (n < 0) { __stable_buffer_free(all); return NULL; }
        if (n > 0 && !stable_buffer_append(all, chunk, (size_t)n)) { __stable_buffer_free(all); return NULL; }
        if (all->len > 64ULL * 1024ULL * 1024ULL) { __stable_buffer_free(all); stable_set_error(-5, "HTTP response exceeds 64 MiB"); return NULL; }
    }
    size_t header_end = 0;
    int found = 0;
    for (size_t i = 3; i < all->len; ++i) {
        if (all->data[i-3]=='\r' && all->data[i-2]=='\n' && all->data[i-1]=='\r' && all->data[i]=='\n') {
            header_end = i + 1; found = 1; break;
        }
    }
    if (!found) { __stable_buffer_free(all); stable_set_error(-4, "invalid HTTP response"); return NULL; }
    char* status_line_end = strstr((char*)all->data, "\r\n");
    if (!status_line_end) { __stable_buffer_free(all); return NULL; }
    stable_http_status_code = 0;
    sscanf((char*)all->data, "HTTP/%*s %d", &stable_http_status_code);
    const size_t body_len = all->len - header_end;
    const unsigned char* body = all->data + header_end;
    const size_t headers_len = header_end - 2;
    const char* te = NULL; size_t te_len = 0;
    const char* cl = NULL; size_t cl_len = 0;
    const int chunked = stable_http_header_value(all->data, headers_len, "Transfer-Encoding", &te, &te_len) && stable_ci_contains(te, te_len, "chunked");
    if (stable_http_header_value(all->data, headers_len, "Content-Length", &cl, &cl_len) && !chunked) {
        char num[32];
        if (cl_len >= sizeof(num)) { __stable_buffer_free(all); stable_set_error(-4, "invalid Content-Length"); return NULL; }
        memcpy(num, cl, cl_len); num[cl_len] = '\0';
        char* endptr = NULL;
        unsigned long long expected = strtoull(num, &endptr, 10);
        if (endptr == num || expected > SIZE_MAX || (size_t)expected > body_len) { __stable_buffer_free(all); stable_set_error(-4, "incomplete HTTP response body"); return NULL; }
        StableBuffer* out = stable_buffer_new((size_t)expected + 1);
        if (!out || (expected && !stable_buffer_append(out, body, (size_t)expected))) { if(out) __stable_buffer_free(out); __stable_buffer_free(all); return NULL; }
        __stable_buffer_free(all);
        return out;
    }
    if (chunked) {
        StableBuffer* out = stable_http_decode_chunked(body, body_len);
        if (!out) stable_set_error(-4, "invalid chunked HTTP response");
        __stable_buffer_free(all);
        return out;
    }
    StableBuffer* out = stable_buffer_new(body_len + 1);
    if (!out || (body_len && !stable_buffer_append(out, body, body_len))) { if(out) __stable_buffer_free(out); __stable_buffer_free(all); return NULL; }
    __stable_buffer_free(all);
    return out;
}

void* __stable_http_get(const char* url, int32_t timeout_ms) {
    stable_http_status_code = 0;
    const char* host_part = NULL; const char* path = NULL; size_t host_len = 0; uint32_t port = 80;
    if (!stable_url_parse(url, &host_part, &host_len, &port, &path)) return NULL;
    char* host = stable_copy_range(host_part, host_len);
    if (!host) return NULL;
    StableSocketHandle* s = (StableSocketHandle*)__stable_net_tcp_connect(host, port, timeout_ms);
    if (!s) { free(host); return NULL; }
    char request[4096];
    char host_header[320];
    if (host_len >= sizeof(host_header) - 16) { free(host); __stable_net_close(s); stable_set_error(-3, "HTTP host name too long"); return NULL; }
    if (port == 80) snprintf(host_header, sizeof(host_header), "%.*s", (int)host_len, host_part);
    else snprintf(host_header, sizeof(host_header), "%.*s:%u", (int)host_len, host_part, (unsigned)port);
    if (strlen(path) > 3500) { free(host); __stable_net_close(s); stable_set_error(-3, "HTTP request target too long"); return NULL; }
    int req_len = snprintf(request, sizeof(request), "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nAccept: */*\r\nUser-Agent: Stable/1.0\r\n\r\n", path, host_header);
    free(host);
    if (req_len < 0 || (size_t)req_len >= sizeof(request) || __stable_net_send(s, request, (uint64_t)req_len) < 0) { __stable_net_close(s); return NULL; }
    StableBuffer* body = stable_http_read_response(s);
    __stable_net_close(s);
    return body;
}
void* __stable_http_post(const char* url, const char* body, int32_t timeout_ms) {
    stable_http_status_code = 0;
    if (!url || !body) return NULL;
    const char* host_part = NULL; const char* path = NULL; size_t host_len = 0; uint32_t port = 80;
    if (!stable_url_parse(url, &host_part, &host_len, &port, &path)) return NULL;
    char* host = stable_copy_range(host_part, host_len);
    if (!host) return NULL;
    StableSocketHandle* s = (StableSocketHandle*)__stable_net_tcp_connect(host, port, timeout_ms);
    if (!s) { free(host); return NULL; }
    char host_header[320];
    if (host_len >= sizeof(host_header) - 16) { free(host); __stable_net_close(s); stable_set_error(-3, "HTTP host name too long"); return NULL; }
    if (port == 80) snprintf(host_header, sizeof(host_header), "%.*s", (int)host_len, host_part);
    else snprintf(host_header, sizeof(host_header), "%.*s:%u", (int)host_len, host_part, (unsigned)port);
    size_t body_len = strlen(body);
    if (body_len > 8U * 1024U * 1024U) { free(host); __stable_net_close(s); stable_set_error(-3, "HTTP request body too large"); return NULL; }
    char prefix[8192];
    int prefix_len_i = snprintf(prefix, sizeof(prefix), "POST %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nContent-Type: application/json\r\nContent-Length: %zu\r\nAccept: */*\r\nUser-Agent: Stable/1.0\r\n\r\n", path, host_header, body_len);
    if (prefix_len_i < 0 || (size_t)prefix_len_i >= sizeof(prefix)) { free(host); __stable_net_close(s); stable_set_error(-3, "HTTP request header too large"); return NULL; }
    size_t prefix_len = (size_t)prefix_len_i;
    size_t total = prefix_len + body_len;
    if (total < body_len) { free(host); __stable_net_close(s); stable_set_error(-3, "HTTP request size overflow"); return NULL; }
    char* request = (char*)malloc(total + 1);
    if (!request) { free(host); __stable_net_close(s); return NULL; }
    memcpy(request, prefix, prefix_len);
    memcpy(request + prefix_len, body, body_len);
    request[total] = '\0';
    if (__stable_net_send(s, request, (uint64_t)total) < 0) { free(request); free(host); __stable_net_close(s); return NULL; }
    free(request); free(host);
    StableBuffer* response = stable_http_read_response(s);
    __stable_net_close(s);
    return response;
}
int __stable_http_status(void) { return stable_http_status_code; }
static const char* stable_json_ws(const char* p) { while (*p==' '||*p=='\t'||*p=='\r'||*p=='\n') ++p; return p; }
static const char* stable_json_string_end(const char* p) {
    if (*p != '"') return NULL;
    ++p;
    for (;;) {
        if (!*p) return NULL;
        if (*p == '\\') {
            ++p;
            if (!*p) return NULL;
            if (*p == 'u') {
                for (int i=0;i<4;++i) { ++p; if (!*p || !((*p>='0'&&*p<='9')||(*p>='a'&&*p<='f')||(*p>='A'&&*p<='F'))) return NULL; }
                ++p;
                continue;
            }
            if (!(*p == '"' || *p == '\\' || *p == '/' || *p == 'b' || *p == 'f' || *p == 'n' || *p == 'r' || *p == 't')) return NULL;
            ++p;
            continue;
        }
        if (*p=='"') return p+1;
        if ((unsigned char)*p < 0x20) return NULL;
        ++p;
    }
}
static const char* stable_json_value_end(const char* p, int depth);
static const char* stable_json_number_end(const char* p) {
    const char* q=p; if (*q=='-') ++q;
    if (*q=='0') ++q;
    else { if (*q<'1'||*q>'9') return NULL; while (*q>='0'&&*q<='9') ++q; }
    if (*q=='.') { ++q; if (*q<'0'||*q>'9') return NULL; while (*q>='0'&&*q<='9') ++q; }
    if (*q=='e'||*q=='E') { ++q; if (*q=='+'||*q=='-') ++q; if (*q<'0'||*q>'9') return NULL; while (*q>='0'&&*q<='9') ++q; }
    return q;
}
static const char* stable_json_value_end(const char* p, int depth) {
    if (depth > 512) return NULL;
    p = stable_json_ws(p);
    if (*p=='"') return stable_json_string_end(p);
    if (*p=='-' || (*p>='0'&&*p<='9')) return stable_json_number_end(p);
    if (strncmp(p,"true",4)==0) return p+4;
    if (strncmp(p,"false",5)==0) return p+5;
    if (strncmp(p,"null",4)==0) return p+4;
    if (*p=='[') { p=stable_json_ws(p+1); if(*p==']') return p+1; for(;;){ p=stable_json_value_end(p,depth+1); if(!p)return NULL; p=stable_json_ws(p); if(*p==']')return p+1; if(*p!=',')return NULL; p=stable_json_ws(p+1);} }
    if (*p=='{') { p=stable_json_ws(p+1); if(*p=='}')return p+1; for(;;){ if(*p!='"')return NULL; p=stable_json_string_end(p); if(!p)return NULL; p=stable_json_ws(p); if(*p!=':')return NULL; p=stable_json_value_end(stable_json_ws(p+1),depth+1); if(!p)return NULL; p=stable_json_ws(p); if(*p=='}')return p+1; if(*p!=',')return NULL; p=stable_json_ws(p+1);} }
    return NULL;
}
int __stable_json_validate(const char* json) {
    if (!json) return 0;
    const char* end = stable_json_value_end(json, 0);
    return end && *stable_json_ws(end) == '\0' ? 1 : 0;
}

void* __stable_json_quote(const char* text) {
    if (!text) return NULL;
    StableBuffer* b = stable_buffer_new(strlen(text) + 3);
    if (!b) return NULL;
    const char quote='"'; if(!stable_buffer_append(b,&quote,1)){__stable_buffer_free(b);return NULL;}
    for (const unsigned char* p=(const unsigned char*)text; *p; ++p) {
        char esc[7]; size_t n=0;
        switch(*p) {
            case '"': esc[0]='\\'; esc[1]='"'; n=2; break;
            case '\\': esc[0]='\\'; esc[1]='\\'; n=2; break;
            case '\b': esc[0]='\\'; esc[1]='b'; n=2; break;
            case '\f': esc[0]='\\'; esc[1]='f'; n=2; break;
            case '\n': esc[0]='\\'; esc[1]='n'; n=2; break;
            case '\r': esc[0]='\\'; esc[1]='r'; n=2; break;
            case '\t': esc[0]='\\'; esc[1]='t'; n=2; break;
            default:
                if (*p < 0x20) { snprintf(esc,sizeof(esc),"\\u%04x",(unsigned)*p); n=6; }
                else { esc[0]=(char)*p; n=1; }
        }
        if(!stable_buffer_append(b,esc,n)){__stable_buffer_free(b);return NULL;}
    }
    if(!stable_buffer_append(b,&quote,1)){__stable_buffer_free(b);return NULL;}
    return b;
}



/* ---------------------------- Game / SDL2 runtime ------------------------ */

#if defined(_WIN32)
typedef void* StableDLHandle;
static StableDLHandle stable_game_sdl_lib = NULL;
static void* stable_game_dlopen(const char* name) { return (void*)LoadLibraryA(name); }
static void* stable_game_dlsym(void* h, const char* name) { return h ? (void*)GetProcAddress((HMODULE)h, name) : NULL; }
#else
#include <dlfcn.h>
typedef void* StableDLHandle;
static StableDLHandle stable_game_sdl_lib = NULL;
static void* stable_game_dlopen(const char* name) { return dlopen(name, RTLD_LAZY | RTLD_LOCAL); }
static void* stable_game_dlsym(void* h, const char* name) { return h ? dlsym(h, name) : NULL; }
#endif

typedef struct StableSDLWindow StableSDLWindow;
typedef struct StableSDLRenderer StableSDLRenderer;
typedef struct StableSDLTexture StableSDLTexture;
typedef struct StableSDLAudio StableSDLAudio;

typedef unsigned int (*StableSDL_Init)(unsigned int);
typedef void (*StableSDL_Quit)(void);
typedef const char* (*StableSDL_GetError)(void);
typedef StableSDLWindow* (*StableSDL_CreateWindow)(const char*, int, int, int, int, unsigned int);
typedef void (*StableSDL_DestroyWindow)(StableSDLWindow*);
typedef int (*StableSDL_PollEvent)(void*);
typedef void (*StableSDL_SetWindowTitle)(StableSDLWindow*, const char*);
typedef void (*StableSDL_GetWindowSize)(StableSDLWindow*, int*, int*);
typedef unsigned int (*StableSDL_GetWindowID)(StableSDLWindow*);
typedef void (*StableSDL_GL_SwapWindow)(StableSDLWindow*);
typedef void (*StableSDL_GL_DeleteContext)(void*);
typedef void* (*StableSDL_GL_CreateContext)(StableSDLWindow*);
typedef int (*StableSDL_GL_SetSwapInterval)(int);
typedef void* (*StableSDL_GL_GetProcAddress)(const char*);
typedef const unsigned char* (*StableSDL_GetKeyboardState)(int*);
typedef unsigned int (*StableSDL_GetMouseState)(int*, int*);
typedef StableSDLRenderer* (*StableSDL_CreateRenderer)(StableSDLWindow*, int, unsigned int);
typedef void (*StableSDL_DestroyRenderer)(StableSDLRenderer*);
typedef int (*StableSDL_SetRenderDrawColor)(StableSDLRenderer*, unsigned char, unsigned char, unsigned char, unsigned char);
typedef int (*StableSDL_RenderClear)(StableSDLRenderer*);
typedef void (*StableSDL_RenderPresent)(StableSDLRenderer*);
typedef int (*StableSDL_RenderDrawLine)(StableSDLRenderer*, int, int, int, int);
typedef int (*StableSDL_RenderFillRect)(StableSDLRenderer*, const void*);
typedef StableSDLTexture* (*StableSDL_CreateTexture)(StableSDLRenderer*, unsigned int, int, int, int);
typedef void (*StableSDL_DestroyTexture)(StableSDLTexture*);
typedef int (*StableSDL_UpdateTexture)(StableSDLTexture*, const void*, const void*, int);
typedef int (*StableSDL_RenderCopy)(StableSDLRenderer*, StableSDLTexture*, const void*, const void*);
typedef unsigned int (*StableSDL_OpenAudioDevice)(const char*, int, const void*, void*, int);
typedef void (*StableSDL_CloseAudioDevice)(unsigned int);
typedef int (*StableSDL_QueueAudio)(unsigned int, const void*, unsigned int);
typedef unsigned int (*StableSDL_GetQueuedAudioSize)(unsigned int);
typedef void (*StableSDL_PauseAudioDevice)(unsigned int, int);
typedef int (*StableSDL_GameControllerOpen)(int);
typedef int (*StableSDL_GameControllerGetAttached)(int);
typedef int (*StableSDL_GameControllerGetAxis)(int, int);
typedef int (*StableSDL_GameControllerGetButton)(int, int);
typedef void (*StableSDL_GameControllerClose)(int);
typedef void* (*StableSDL_GameControllerHandleOpen)(int);
typedef int (*StableSDL_GameControllerAttachedHandle)(void*);
typedef int (*StableSDL_GameControllerAxisHandle)(void*, int);
typedef int (*StableSDL_GameControllerButtonHandle)(void*, int);
typedef void (*StableSDL_GameControllerCloseHandle)(void*);

typedef struct StableSDLFns {
    StableSDL_Init Init;
    StableSDL_Quit Quit;
    StableSDL_GetError GetError;
    StableSDL_CreateWindow CreateWindow;
    StableSDL_DestroyWindow DestroyWindow;
    StableSDL_PollEvent PollEvent;
    StableSDL_SetWindowTitle SetWindowTitle;
    StableSDL_GetWindowSize GetWindowSize;
    StableSDL_GetWindowID GetWindowID;
    StableSDL_GL_SwapWindow GLSwapWindow;
    StableSDL_GL_DeleteContext GLDeleteContext;
    StableSDL_GL_CreateContext GLCreateContext;
    StableSDL_GL_SetSwapInterval GLSetSwapInterval;
    StableSDL_GL_GetProcAddress GLGetProcAddress;
    StableSDL_GetKeyboardState GetKeyboardState;
    StableSDL_GetMouseState GetMouseState;
    StableSDL_CreateRenderer CreateRenderer;
    StableSDL_DestroyRenderer DestroyRenderer;
    StableSDL_SetRenderDrawColor SetRenderDrawColor;
    StableSDL_RenderClear RenderClear;
    StableSDL_RenderPresent RenderPresent;
    StableSDL_RenderDrawLine RenderDrawLine;
    StableSDL_RenderFillRect RenderFillRect;
    StableSDL_CreateTexture CreateTexture;
    StableSDL_DestroyTexture DestroyTexture;
    StableSDL_UpdateTexture UpdateTexture;
    StableSDL_RenderCopy RenderCopy;
    StableSDL_OpenAudioDevice OpenAudioDevice;
    StableSDL_CloseAudioDevice CloseAudioDevice;
    StableSDL_QueueAudio QueueAudio;
    StableSDL_GetQueuedAudioSize GetQueuedAudioSize;
    StableSDL_PauseAudioDevice PauseAudioDevice;
    StableSDL_GameControllerHandleOpen OpenController;
    StableSDL_GameControllerAttachedHandle ControllerAttached;
    StableSDL_GameControllerAxisHandle ControllerAxis;
    StableSDL_GameControllerButtonHandle ControllerButton;
    StableSDL_GameControllerCloseHandle CloseController;
} StableSDLFns;

static StableSDLFns stable_game_sdl;
static int stable_game_sdl_ready = 0;
static int stable_game_sdl_refs = 0;
static void* stable_game_controllers[8];
static _Thread_local unsigned char stable_game_event[64];
static _Thread_local int stable_game_event_type_value = 0;
static _Thread_local int stable_game_event_code_value = 0;
static _Thread_local int stable_game_event_x_value = 0;
static _Thread_local int stable_game_event_y_value = 0;
static _Thread_local char stable_game_event_text_value[64];
static _Thread_local int64_t stable_game_last_frame_ns = 0;
static _Thread_local double stable_game_delta_value = 0.0;

static void* stable_game_sym(void* handle, const char* name) { return stable_game_dlsym(handle, name); }
#define STABLE_GAME_LOAD_FIELD(field,type,name) do { union { void* p; type f; } stable_u; stable_u.p = stable_game_sym(stable_game_sdl_lib, name); stable_game_sdl.field = stable_u.f; if (!stable_game_sdl.field) return 0; } while (0)

static int stable_game_load_sdl(void) {
    if (stable_game_sdl_ready) return 1;
#if defined(_WIN32)
    const char* names[] = { "SDL2.dll", "SDL2-2.0.dll" };
#elif defined(__APPLE__)
    const char* names[] = { "libSDL2-2.0.0.dylib", "libSDL2.dylib" };
#else
    const char* names[] = { "libSDL2-2.0.so.0", "libSDL2.so" };
#endif
    for (size_t i=0;i<sizeof(names)/sizeof(names[0]);++i) {
        stable_game_sdl_lib = (StableDLHandle)stable_game_dlopen(names[i]);
        if (stable_game_sdl_lib) break;
    }
    if (!stable_game_sdl_lib) return 0;
    memset(&stable_game_sdl, 0, sizeof(stable_game_sdl));
    STABLE_GAME_LOAD_FIELD(Init,StableSDL_Init,"SDL_Init");
    STABLE_GAME_LOAD_FIELD(Quit,StableSDL_Quit,"SDL_Quit");
    STABLE_GAME_LOAD_FIELD(GetError,StableSDL_GetError,"SDL_GetError");
    STABLE_GAME_LOAD_FIELD(CreateWindow,StableSDL_CreateWindow,"SDL_CreateWindow");
    STABLE_GAME_LOAD_FIELD(DestroyWindow,StableSDL_DestroyWindow,"SDL_DestroyWindow");
    STABLE_GAME_LOAD_FIELD(PollEvent,StableSDL_PollEvent,"SDL_PollEvent");
    STABLE_GAME_LOAD_FIELD(SetWindowTitle,StableSDL_SetWindowTitle,"SDL_SetWindowTitle");
    STABLE_GAME_LOAD_FIELD(GetWindowSize,StableSDL_GetWindowSize,"SDL_GetWindowSize");
    STABLE_GAME_LOAD_FIELD(GetWindowID,StableSDL_GetWindowID,"SDL_GetWindowID");
    STABLE_GAME_LOAD_FIELD(GLSwapWindow,StableSDL_GL_SwapWindow,"SDL_GL_SwapWindow");
    STABLE_GAME_LOAD_FIELD(GLDeleteContext,StableSDL_GL_DeleteContext,"SDL_GL_DeleteContext");
    STABLE_GAME_LOAD_FIELD(GLCreateContext,StableSDL_GL_CreateContext,"SDL_GL_CreateContext");
    STABLE_GAME_LOAD_FIELD(GLSetSwapInterval,StableSDL_GL_SetSwapInterval,"SDL_GL_SetSwapInterval");
    STABLE_GAME_LOAD_FIELD(GLGetProcAddress,StableSDL_GL_GetProcAddress,"SDL_GL_GetProcAddress");
    STABLE_GAME_LOAD_FIELD(GetKeyboardState,StableSDL_GetKeyboardState,"SDL_GetKeyboardState");
    STABLE_GAME_LOAD_FIELD(GetMouseState,StableSDL_GetMouseState,"SDL_GetMouseState");
    STABLE_GAME_LOAD_FIELD(CreateRenderer,StableSDL_CreateRenderer,"SDL_CreateRenderer");
    STABLE_GAME_LOAD_FIELD(DestroyRenderer,StableSDL_DestroyRenderer,"SDL_DestroyRenderer");
    STABLE_GAME_LOAD_FIELD(SetRenderDrawColor,StableSDL_SetRenderDrawColor,"SDL_SetRenderDrawColor");
    STABLE_GAME_LOAD_FIELD(RenderClear,StableSDL_RenderClear,"SDL_RenderClear");
    STABLE_GAME_LOAD_FIELD(RenderPresent,StableSDL_RenderPresent,"SDL_RenderPresent");
    STABLE_GAME_LOAD_FIELD(RenderDrawLine,StableSDL_RenderDrawLine,"SDL_RenderDrawLine");
    STABLE_GAME_LOAD_FIELD(RenderFillRect,StableSDL_RenderFillRect,"SDL_RenderFillRect");
    STABLE_GAME_LOAD_FIELD(CreateTexture,StableSDL_CreateTexture,"SDL_CreateTexture");
    STABLE_GAME_LOAD_FIELD(DestroyTexture,StableSDL_DestroyTexture,"SDL_DestroyTexture");
    STABLE_GAME_LOAD_FIELD(UpdateTexture,StableSDL_UpdateTexture,"SDL_UpdateTexture");
    STABLE_GAME_LOAD_FIELD(RenderCopy,StableSDL_RenderCopy,"SDL_RenderCopy");
    STABLE_GAME_LOAD_FIELD(OpenAudioDevice,StableSDL_OpenAudioDevice,"SDL_OpenAudioDevice");
    STABLE_GAME_LOAD_FIELD(CloseAudioDevice,StableSDL_CloseAudioDevice,"SDL_CloseAudioDevice");
    STABLE_GAME_LOAD_FIELD(QueueAudio,StableSDL_QueueAudio,"SDL_QueueAudio");
    STABLE_GAME_LOAD_FIELD(GetQueuedAudioSize,StableSDL_GetQueuedAudioSize,"SDL_GetQueuedAudioSize");
    STABLE_GAME_LOAD_FIELD(PauseAudioDevice,StableSDL_PauseAudioDevice,"SDL_PauseAudioDevice");
    union { void* p; StableSDL_GameControllerHandleOpen f; } u0; u0.p=stable_game_sym(stable_game_sdl_lib,"SDL_GameControllerOpen"); stable_game_sdl.OpenController=u0.f;
    union { void* p; StableSDL_GameControllerAttachedHandle f; } u1; u1.p=stable_game_sym(stable_game_sdl_lib,"SDL_GameControllerGetAttached"); stable_game_sdl.ControllerAttached=u1.f;
    union { void* p; StableSDL_GameControllerAxisHandle f; } u2; u2.p=stable_game_sym(stable_game_sdl_lib,"SDL_GameControllerGetAxis"); stable_game_sdl.ControllerAxis=u2.f;
    union { void* p; StableSDL_GameControllerButtonHandle f; } u3; u3.p=stable_game_sym(stable_game_sdl_lib,"SDL_GameControllerGetButton"); stable_game_sdl.ControllerButton=u3.f;
    union { void* p; StableSDL_GameControllerCloseHandle f; } u4; u4.p=stable_game_sym(stable_game_sdl_lib,"SDL_GameControllerClose"); stable_game_sdl.CloseController=u4.f;
    if (!stable_game_sdl.OpenController || !stable_game_sdl.ControllerAttached || !stable_game_sdl.ControllerAxis || !stable_game_sdl.ControllerButton || !stable_game_sdl.CloseController) return 0;
    stable_game_sdl_ready = 1;
    return 1;
}
#undef STABLE_GAME_LOAD_FIELD

static int stable_game_init(void) {
    if (!stable_game_load_sdl()) return 0;
    if (stable_game_sdl_refs == 0) {
        if (stable_game_sdl.Init(0x00000010u | 0x00000020u | 0x00002000u | 0x00004000u) != 0) return 0;
    }
    ++stable_game_sdl_refs;
    return 1;
}

static void stable_game_shutdown_ref(void) {
    if (stable_game_sdl_refs <= 0) return;
    --stable_game_sdl_refs;
    if (stable_game_sdl_refs == 0) {
        for (int i = 0; i < 8; ++i) {
            if (stable_game_controllers[i]) {
                stable_game_sdl.CloseController(stable_game_controllers[i]);
                stable_game_controllers[i] = NULL;
            }
        }
        stable_game_sdl.Quit();
    }
}

typedef struct StableGameWindow {
    StableSDLWindow* window;
    void* gl_context;
    int should_close;
} StableGameWindow;

typedef struct StableGameRenderer { StableSDLRenderer* renderer; } StableGameRenderer;
typedef struct StableGameTexture { StableSDLTexture* texture; } StableGameTexture;
typedef struct StableGameAudio { unsigned int device; int channels; } StableGameAudio;

typedef struct StableGameRect { int x,y,w,h; } StableGameRect;
typedef struct StableGameAudioSpec { int freq; unsigned short format; unsigned char channels; unsigned char silence; unsigned short samples; unsigned short padding; unsigned int size; void* callback; void* userdata; } StableGameAudioSpec;

static int64_t stable_game_now_ns(void) {
#if defined(_WIN32)
    static LARGE_INTEGER freq = {0};
    LARGE_INTEGER now;
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    return (int64_t)((now.QuadPart * 1000000000LL) / freq.QuadPart);
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
#endif
}

void* __stable_game_window_create(const char* title, int32_t width, int32_t height, uint32_t flags) {
    if (!stable_game_init()) return NULL;
    StableGameWindow* out=(StableGameWindow*)calloc(1,sizeof(*out));
    if(!out){stable_game_shutdown_ref();return NULL;}
    out->window=stable_game_sdl.CreateWindow(title?title:"Stable",0x2fff0000,0x2fff0000,width,height,flags);
    if(!out->window){free(out);stable_game_shutdown_ref();return NULL;}
    return out;
}
void __stable_game_window_destroy(void* raw){StableGameWindow*w=(StableGameWindow*)raw;if(!w)return;if(w->gl_context)stable_game_sdl.GLDeleteContext(w->gl_context);if(w->window)stable_game_sdl.DestroyWindow(w->window);free(w);stable_game_shutdown_ref();}
int32_t __stable_game_poll(void* raw){StableGameWindow*w=(StableGameWindow*)raw;if(!w)return 0;int count=0;stable_game_event_type_value=0;stable_game_event_code_value=0;stable_game_event_x_value=0;stable_game_event_y_value=0;stable_game_event_text_value[0]=0;while(stable_game_sdl.PollEvent(stable_game_event)){++count;uint32_t type=0;memcpy(&type,stable_game_event,4);stable_game_event_type_value=(int)type;if(type==0x100u){stable_game_event_code_value=0;w->should_close=1;}else if(type==0x300u||type==0x301u){int32_t code=0;memcpy(&code,stable_game_event+16,4);stable_game_event_code_value=code;}else if(type==0x400u){int32_t x=0,y=0;memcpy(&x,stable_game_event+20,4);memcpy(&y,stable_game_event+24,4);stable_game_event_x_value=x;stable_game_event_y_value=y;}else if(type==0x401u||type==0x402u){int32_t btn=0,x=0,y=0;memcpy(&btn,stable_game_event+16,1);memcpy(&x,stable_game_event+20,4);memcpy(&y,stable_game_event+24,4);stable_game_event_code_value=btn;stable_game_event_x_value=x;stable_game_event_y_value=y;}else if(type==0x303u){memcpy(stable_game_event_text_value,stable_game_event+12,32);stable_game_event_text_value[32]=0;}}return count;}
int32_t __stable_game_should_close(void* raw){StableGameWindow*w=(StableGameWindow*)raw;return w&&w->should_close?1:0;}
void __stable_game_request_close(void* raw){StableGameWindow*w=(StableGameWindow*)raw;if(w)w->should_close=1;stable_game_event_type_value=0x100;}
void __stable_game_set_title(void* raw,const char*title){StableGameWindow*w=(StableGameWindow*)raw;if(w&&w->window)stable_game_sdl.SetWindowTitle(w->window,title?title:"Stable");}
int32_t __stable_game_window_width(void* raw){StableGameWindow*w=(StableGameWindow*)raw;int x=0,y=0;if(w&&w->window)stable_game_sdl.GetWindowSize(w->window,&x,&y);return x;}
int32_t __stable_game_window_height(void* raw){StableGameWindow*w=(StableGameWindow*)raw;int x=0,y=0;if(w&&w->window)stable_game_sdl.GetWindowSize(w->window,&x,&y);return y;}
int32_t __stable_game_set_vsync(void* raw,int on){StableGameWindow*w=(StableGameWindow*)raw;if(!w||!w->gl_context||!stable_game_sdl.GLSetSwapInterval)return 0;return stable_game_sdl.GLSetSwapInterval(on?1:0)==0;}
int32_t __stable_game_make_gl_context(void* raw){StableGameWindow*w=(StableGameWindow*)raw;if(!w||!w->window)return 0;if(w->gl_context)return 1;w->gl_context=stable_game_sdl.GLCreateContext(w->window);return w->gl_context?1:0;}
void __stable_game_present(void*raw){StableGameWindow*w=(StableGameWindow*)raw;if(w&&w->window&&w->gl_context&&stable_game_sdl.GLSwapWindow)stable_game_sdl.GLSwapWindow(w->window);}
int32_t __stable_game_window_flags(int opengl,int resizable,int fullscreen,int highdpi){unsigned int f=0;if(opengl)f|=0x00000002u;if(resizable)f|=0x00000020u;if(fullscreen)f|=0x00001001u;if(highdpi)f|=0x00002000u;return (int32_t)f;}
int32_t __stable_game_renderer_flags(int accelerated,int vsync){unsigned int f=0;if(accelerated)f|=0x00000002u;if(vsync)f|=0x00000004u;return (int32_t)f;}
void* __stable_game_renderer_create(void*raw,uint32_t flags){StableGameWindow*w=(StableGameWindow*)raw;if(!w||!w->window)return NULL;StableGameRenderer*r=(StableGameRenderer*)calloc(1,sizeof(*r));if(!r)return NULL;r->renderer=stable_game_sdl.CreateRenderer(w->window,-1,flags);if(!r->renderer){free(r);return NULL;}return r;}
void __stable_game_renderer_destroy(void*raw){StableGameRenderer*r=(StableGameRenderer*)raw;if(!r)return;if(r->renderer)stable_game_sdl.DestroyRenderer(r->renderer);free(r);}
int32_t __stable_game_renderer_set_color(void*raw,uint8_t r,uint8_t g,uint8_t b,uint8_t a){StableGameRenderer*x=(StableGameRenderer*)raw;return x&&x->renderer?stable_game_sdl.SetRenderDrawColor(x->renderer,r,g,b,a)==0:0;}
int32_t __stable_game_renderer_clear(void*raw){StableGameRenderer*x=(StableGameRenderer*)raw;return x&&x->renderer?stable_game_sdl.RenderClear(x->renderer)==0:0;}
void __stable_game_renderer_present(void*raw){StableGameRenderer*x=(StableGameRenderer*)raw;if(x&&x->renderer)stable_game_sdl.RenderPresent(x->renderer);}
int32_t __stable_game_renderer_line(void*raw,int x1,int y1,int x2,int y2){StableGameRenderer*x=(StableGameRenderer*)raw;return x&&x->renderer?stable_game_sdl.RenderDrawLine(x->renderer,x1,y1,x2,y2)==0:0;}
int32_t __stable_game_renderer_fill_rect(void*raw,int x,int y,int w,int h){StableGameRenderer*r=(StableGameRenderer*)raw;if(!r||!r->renderer)return 0;StableGameRect rect={x,y,w,h};return stable_game_sdl.RenderFillRect(r->renderer,&rect)==0;}
void* __stable_game_texture_create(void*raw,uint32_t renderer_format,uint32_t access,int w,int h){StableGameRenderer*r=(StableGameRenderer*)raw;if(!r||!r->renderer)return NULL;StableGameTexture*t=(StableGameTexture*)calloc(1,sizeof(*t));if(!t)return NULL;t->texture=stable_game_sdl.CreateTexture(r->renderer,renderer_format,(int)access,w,h);if(!t->texture){free(t);return NULL;}return t;}
void __stable_game_texture_destroy(void*raw){StableGameTexture*t=(StableGameTexture*)raw;if(!t)return;if(t->texture)stable_game_sdl.DestroyTexture(t->texture);free(t);}
int32_t __stable_game_texture_update(void*raw,const void*pixels,int pitch){StableGameTexture*t=(StableGameTexture*)raw;if(!t||!t->texture)return 0;return stable_game_sdl.UpdateTexture(t->texture,NULL,pixels,pitch)==0;}
int32_t __stable_game_texture_copy(void*raw,void*tex,int x,int y,int w,int h){StableGameRenderer*r=(StableGameRenderer*)raw;StableGameTexture*t=(StableGameTexture*)tex;if(!r||!r->renderer||!t||!t->texture)return 0;StableGameRect dst={x,y,w,h};return stable_game_sdl.RenderCopy(r->renderer,t->texture,NULL,&dst)==0;}
int32_t __stable_game_event_type(void){return stable_game_event_type_value;}
int32_t __stable_game_event_code(void){return stable_game_event_code_value;}
int32_t __stable_game_event_x(void){return stable_game_event_x_value;}
int32_t __stable_game_event_y(void){return stable_game_event_y_value;}
const char* __stable_game_event_text(void){return stable_game_event_text_value;}
int32_t __stable_game_key_down(int32_t scancode){int n=0;const unsigned char*p=stable_game_sdl.GetKeyboardState(&n);return p&&scancode>=0&&scancode<n?p[scancode]!=0:0;}
int32_t __stable_game_mouse_button_down(int32_t button){int x=0,y=0;unsigned int m=stable_game_sdl.GetMouseState(&x,&y);if(button<1||button>5)return 0;return (m & (1u<<(button-1)))!=0;}
int32_t __stable_game_mouse_x(void){int x=0,y=0;(void)stable_game_sdl.GetMouseState(&x,&y);return x;}
int32_t __stable_game_mouse_y(void){int x=0,y=0;(void)stable_game_sdl.GetMouseState(&x,&y);return y;}
int32_t __stable_game_controller_connected(int index){if(index<0||index>=8)return 0;if(!stable_game_controllers[index])stable_game_controllers[index]=stable_game_sdl.OpenController(index);return stable_game_controllers[index]?stable_game_sdl.ControllerAttached(stable_game_controllers[index]):0;}
float __stable_game_controller_axis(int index,int axis){if(index<0||index>=8)return 0.0f;if(!stable_game_controllers[index])stable_game_controllers[index]=stable_game_sdl.OpenController(index);if(!stable_game_controllers[index])return 0.0f;int v=stable_game_sdl.ControllerAxis(stable_game_controllers[index],axis);if(v>=0)return (float)v/32767.0f;return (float)v/32768.0f;}
int32_t __stable_game_controller_button(int index,int button){if(index<0||index>=8)return 0;if(!stable_game_controllers[index])stable_game_controllers[index]=stable_game_sdl.OpenController(index);return stable_game_controllers[index]?stable_game_sdl.ControllerButton(stable_game_controllers[index],button)!=0:0;}
void* __stable_game_audio_open(int32_t sample_rate,int32_t channels,int32_t samples){if(!stable_game_init())return NULL;StableGameAudio*audio=(StableGameAudio*)calloc(1,sizeof(*audio));if(!audio){stable_game_shutdown_ref();return NULL;}StableGameAudioSpec want;memset(&want,0,sizeof(want));want.freq=sample_rate;want.format=0x8010;want.channels=(unsigned char)channels;want.samples=(unsigned short)samples;unsigned int dev=stable_game_sdl.OpenAudioDevice(NULL,0,&want,&want,0);if(dev==0){free(audio);stable_game_shutdown_ref();return NULL;}audio->device=dev;audio->channels=channels;stable_game_sdl.PauseAudioDevice(dev,0);return audio;}
int64_t __stable_game_audio_write(void*raw,const void*pixels,uint64_t bytes){StableGameAudio*a=(StableGameAudio*)raw;if(!a||a->device==0||bytes>0xffffffffULL)return -1;if(stable_game_sdl.QueueAudio(a->device,pixels,(unsigned int)bytes)!=0)return -1;return (int64_t)bytes;}
uint64_t __stable_game_audio_queued(void*raw){StableGameAudio*a=(StableGameAudio*)raw;return a?(uint64_t)stable_game_sdl.GetQueuedAudioSize(a->device):0;}
void __stable_game_audio_pause(void*raw,int pause){StableGameAudio*a=(StableGameAudio*)raw;if(a)stable_game_sdl.PauseAudioDevice(a->device,pause?1:0);}
void __stable_game_audio_close(void*raw){StableGameAudio*a=(StableGameAudio*)raw;if(!a)return;if(a->device)stable_game_sdl.CloseAudioDevice(a->device);free(a);stable_game_shutdown_ref();}
int64_t __stable_game_time_nanos(void){return stable_game_now_ns();}
double __stable_game_delta_seconds(void){int64_t now=stable_game_now_ns();if(stable_game_last_frame_ns==0){stable_game_last_frame_ns=now;stable_game_delta_value=0.0;}else{int64_t dt=now-stable_game_last_frame_ns;stable_game_last_frame_ns=now;stable_game_delta_value=(double)dt/1000000000.0;}return stable_game_delta_value;}
void __stable_game_sleep_nanos(int64_t nanos){
    if(nanos<=0)return;
#if defined(_WIN32)
    Sleep((DWORD)((nanos+999999)/1000000));
#else
    struct timespec ts;
    ts.tv_sec=(time_t)(nanos/1000000000LL);
    ts.tv_nsec=(long)(nanos%1000000000LL);
    nanosleep(&ts,NULL);
#endif
}

static StableDLHandle stable_gfx_cached[4] = { NULL, NULL, NULL, NULL };
static const char* stable_gfx_names[4] = { "vulkan", "opengl", "d3d12", "metal" };

static int stable_gfx_index(const char* api) {
    if (!api) return -1;
    for (int i = 0; i < 4; ++i) {
#if defined(_WIN32)
        if (_stricmp(api, stable_gfx_names[i]) == 0) return i;
#else
        if (strcasecmp(api, stable_gfx_names[i]) == 0) return i;
#endif
    }
#if defined(_WIN32)
    if (_stricmp(api, "d3d11") == 0) return 2;
#else
    if (strcasecmp(api, "d3d11") == 0) return 2;
#endif
    return -1;
}

static void* stable_gfx_open_api(const char* api){
    if(!api)return NULL;
    const int cachedIndex = stable_gfx_index(api);
    if (cachedIndex >= 0 && cachedIndex < 4 && stable_gfx_cached[cachedIndex]) return stable_gfx_cached[cachedIndex];
    const char* names[6]={0}; int n=0;
#if defined(_WIN32)
    if(_stricmp(api,"vulkan")==0){names[n++]="vulkan-1.dll";} else if(_stricmp(api,"opengl")==0){names[n++]="opengl32.dll";} else if(_stricmp(api,"d3d12")==0){names[n++]="d3d12.dll";} else if(_stricmp(api,"d3d11")==0){names[n++]="d3d11.dll";}
#elif defined(__APPLE__)
    if(strcasecmp(api,"vulkan")==0){names[n++]="libvulkan.1.dylib";names[n++]="libvulkan.dylib";} else if(strcasecmp(api,"opengl")==0){names[n++]="/System/Library/Frameworks/OpenGL.framework/OpenGL";} else if(strcasecmp(api,"metal")==0){names[n++]="/System/Library/Frameworks/Metal.framework/Metal";}
#else
    if(strcasecmp(api,"vulkan")==0){names[n++]="libvulkan.so.1";names[n++]="libvulkan.so";} else if(strcasecmp(api,"opengl")==0){names[n++]="libGL.so.1";names[n++]="libOpenGL.so.0";}
#endif
    for(int i=0;i<n;++i){void*h=stable_game_dlopen(names[i]);if(h){if(cachedIndex>=0&&cachedIndex<4)stable_gfx_cached[cachedIndex]=h;return h;}}
    return NULL;
}
int32_t __stable_gfx_available(const char*api){void*h=stable_gfx_open_api(api);return h?1:0;}
const char* __stable_gfx_backend(void){
    if(__stable_gfx_available("vulkan"))return "vulkan";
    if(__stable_gfx_available("opengl"))return "opengl";
#if defined(_WIN32)
    if(__stable_gfx_available("d3d12"))return "d3d12";
#elif defined(__APPLE__)
    if(__stable_gfx_available("metal"))return "metal";
#endif
    return "none";
}
void* __stable_gfx_load_proc(const char*api,const char*name){if(!api||!name)return NULL;void*h=stable_gfx_open_api(api);if(!h)return NULL;void*p=stable_game_dlsym(h,name);if(!p&&strcasecmp(api,"opengl")==0&&stable_game_sdl.GLGetProcAddress)p=stable_game_sdl.GLGetProcAddress(name);return p;}

typedef void (*StableGLClearColor)(float,float,float,float);
typedef void (*StableGLClear)(unsigned int);
typedef void (*StableGLViewport)(int,int,int,int);
typedef void (*StableGLEnable)(unsigned int);
typedef void (*StableGLDisable)(unsigned int);
typedef void (*StableGLGenBuffers)(int,unsigned int*);
typedef void (*StableGLBindBuffer)(unsigned int,unsigned int);
typedef void (*StableGLBufferData)(unsigned int,intptr_t,const void*,unsigned int);
typedef unsigned int (*StableGLCreateShader)(unsigned int);
typedef void (*StableGLShaderSource)(unsigned int,int,const char* const*,const int*);
typedef void (*StableGLCompileShader)(unsigned int);
typedef void (*StableGLGetShaderiv)(unsigned int,unsigned int,int*);
typedef void (*StableGLGetShaderInfoLog)(unsigned int,int,int*,char*);
typedef void (*StableGLDeleteShader)(unsigned int);
typedef unsigned int (*StableGLCreateProgram)(void);
typedef void (*StableGLAttachShader)(unsigned int,unsigned int);
typedef void (*StableGLLinkProgram)(unsigned int);
typedef void (*StableGLGetProgramiv)(unsigned int,unsigned int,int*);
typedef void (*StableGLGetProgramInfoLog)(unsigned int,int,int*,char*);
typedef void (*StableGLUseProgram)(unsigned int);
typedef void (*StableGLDrawArrays)(unsigned int,int,int);
typedef void (*StableGLGenVertexArrays)(int,unsigned int*);
typedef void (*StableGLBindVertexArray)(unsigned int);
typedef void (*StableGLEnableVertexAttribArray)(unsigned int);
typedef void (*StableGLVertexAttribPointer)(unsigned int,int,unsigned int,unsigned char,int,const void*);
typedef unsigned int (*StableGLGetError)(void);
typedef void (*StableGLDeleteProgram)(unsigned int);
typedef void (*StableGLDeleteBuffers)(int,const unsigned int*);
typedef void (*StableGLDeleteVertexArrays)(int,const unsigned int*);

#define STABLE_GL_PROC(name,type) static type name##_fn(void){ union{void*p;type f;}u;u.p=__stable_gfx_load_proc("opengl",#name);return u.f; }
STABLE_GL_PROC(glClearColor,StableGLClearColor)
STABLE_GL_PROC(glClear,StableGLClear)
STABLE_GL_PROC(glViewport,StableGLViewport)
STABLE_GL_PROC(glEnable,StableGLEnable)
STABLE_GL_PROC(glDisable,StableGLDisable)
STABLE_GL_PROC(glGenBuffers,StableGLGenBuffers)
STABLE_GL_PROC(glBindBuffer,StableGLBindBuffer)
STABLE_GL_PROC(glBufferData,StableGLBufferData)
STABLE_GL_PROC(glCreateShader,StableGLCreateShader)
STABLE_GL_PROC(glShaderSource,StableGLShaderSource)
STABLE_GL_PROC(glCompileShader,StableGLCompileShader)
STABLE_GL_PROC(glGetShaderiv,StableGLGetShaderiv)
STABLE_GL_PROC(glGetShaderInfoLog,StableGLGetShaderInfoLog)
STABLE_GL_PROC(glDeleteShader,StableGLDeleteShader)
STABLE_GL_PROC(glCreateProgram,StableGLCreateProgram)
STABLE_GL_PROC(glAttachShader,StableGLAttachShader)
STABLE_GL_PROC(glLinkProgram,StableGLLinkProgram)
STABLE_GL_PROC(glGetProgramiv,StableGLGetProgramiv)
STABLE_GL_PROC(glGetProgramInfoLog,StableGLGetProgramInfoLog)
STABLE_GL_PROC(glUseProgram,StableGLUseProgram)
STABLE_GL_PROC(glDrawArrays,StableGLDrawArrays)
STABLE_GL_PROC(glGenVertexArrays,StableGLGenVertexArrays)
STABLE_GL_PROC(glBindVertexArray,StableGLBindVertexArray)
STABLE_GL_PROC(glEnableVertexAttribArray,StableGLEnableVertexAttribArray)
STABLE_GL_PROC(glVertexAttribPointer,StableGLVertexAttribPointer)
STABLE_GL_PROC(glGetError,StableGLGetError)
STABLE_GL_PROC(glDeleteProgram,StableGLDeleteProgram)
STABLE_GL_PROC(glDeleteBuffers,StableGLDeleteBuffers)
STABLE_GL_PROC(glDeleteVertexArrays,StableGLDeleteVertexArrays)
#undef STABLE_GL_PROC

void __stable_gl_clear_color(float r,float g,float b,float a){StableGLClearColor f=glClearColor_fn();if(f)f(r,g,b,a);}
void __stable_gl_clear(uint32_t mask){StableGLClear f=glClear_fn();if(f)f(mask);}
void __stable_gl_viewport(int32_t x,int32_t y,int32_t w,int32_t h){StableGLViewport f=glViewport_fn();if(f)f(x,y,w,h);}
void __stable_gl_enable(uint32_t c){StableGLEnable f=glEnable_fn();if(f)f(c);}
void __stable_gl_disable(uint32_t c){StableGLDisable f=glDisable_fn();if(f)f(c);}
void __stable_gl_gen_buffers(int32_t n,uint32_t*out){StableGLGenBuffers f=glGenBuffers_fn();if(f)f(n,out);}
void __stable_gl_bind_buffer(uint32_t t,uint32_t b){StableGLBindBuffer f=glBindBuffer_fn();if(f)f(t,b);}
void __stable_gl_buffer_data(uint32_t t,int64_t size,const void*d,uint32_t usage){StableGLBufferData f=glBufferData_fn();if(f)f(t,(intptr_t)size,d,usage);}
uint32_t __stable_gl_create_shader(uint32_t t){StableGLCreateShader f=glCreateShader_fn();return f?f(t):0;}
void __stable_gl_shader_source(uint32_t s,const char*src){StableGLShaderSource f=glShaderSource_fn();if(f){const char*p=src;f(s,1,&p,NULL);}}
void __stable_gl_compile_shader(uint32_t s){StableGLCompileShader f=glCompileShader_fn();if(f)f(s);}
int32_t __stable_gl_shader_status(uint32_t s){StableGLGetShaderiv f=glGetShaderiv_fn();int v=0;if(f)f(s,0x8B81u,&v);return v!=0;}
void* __stable_gl_shader_log(uint32_t s){StableGLGetShaderiv q=glGetShaderiv_fn();StableGLGetShaderInfoLog f=glGetShaderInfoLog_fn();if(!q||!f)return ____stable_buffer_from_string("");int n=0;q(s,0x8B84u,&n);if(n<=0)return ____stable_buffer_from_string("");char*buf=(char*)malloc((size_t)n+1);if(!buf)return NULL;int got=0;f(s,n,&got,buf);buf[got>0?got:0]='\0';StableBuffer*b=____stable_buffer_from_string(buf);free(buf);return b;}
void __stable_gl_delete_shader(uint32_t s){StableGLDeleteShader f=glDeleteShader_fn();if(f)f(s);}
uint32_t __stable_gl_create_program(void){StableGLCreateProgram f=glCreateProgram_fn();return f?f():0;}
void __stable_gl_attach_shader(uint32_t p,uint32_t s){StableGLAttachShader f=glAttachShader_fn();if(f)f(p,s);}
void __stable_gl_link_program(uint32_t p){StableGLLinkProgram f=glLinkProgram_fn();if(f)f(p);}
int32_t __stable_gl_program_status(uint32_t p){StableGLGetProgramiv f=glGetProgramiv_fn();int v=0;if(f)f(p,0x8B82u,&v);return v!=0;}
void* __stable_gl_program_log(uint32_t p){StableGLGetProgramiv q=glGetProgramiv_fn();StableGLGetProgramInfoLog f=glGetProgramInfoLog_fn();if(!q||!f)return ____stable_buffer_from_string("");int n=0;q(p,0x8B84u,&n);if(n<=0)return ____stable_buffer_from_string("");char*buf=(char*)malloc((size_t)n+1);if(!buf)return NULL;int got=0;f(p,n,&got,buf);buf[got>0?got:0]='\0';StableBuffer*b=____stable_buffer_from_string(buf);free(buf);return b;}
void __stable_gl_use_program(uint32_t p){StableGLUseProgram f=glUseProgram_fn();if(f)f(p);}
void __stable_gl_draw_arrays(uint32_t mode,int32_t first,int32_t count){StableGLDrawArrays f=glDrawArrays_fn();if(f)f(mode,first,count);}
void __stable_gl_gen_vertex_arrays(int32_t n,uint32_t*out){StableGLGenVertexArrays f=glGenVertexArrays_fn();if(f)f(n,out);}
void __stable_gl_bind_vertex_array(uint32_t a){StableGLBindVertexArray f=glBindVertexArray_fn();if(f)f(a);}
void __stable_gl_enable_vertex_attrib(uint32_t a){StableGLEnableVertexAttribArray f=glEnableVertexAttribArray_fn();if(f)f(a);}
void __stable_gl_vertex_attrib_pointer(int32_t index,int32_t size,uint32_t type,int normalized,int32_t stride,uint64_t offset){StableGLVertexAttribPointer f=glVertexAttribPointer_fn();if(f)f((unsigned)index,size,type,(unsigned char)(normalized?1:0),stride,(const void*)(uintptr_t)offset);}
uint32_t __stable_gl_get_error(void){StableGLGetError f=glGetError_fn();return f?f():0;}
void __stable_gl_delete_program(uint32_t p){StableGLDeleteProgram f=glDeleteProgram_fn();if(f)f(p);}
void __stable_gl_delete_buffers(int32_t n,const uint32_t*p){StableGLDeleteBuffers f=glDeleteBuffers_fn();if(f)f(n,p);}
void __stable_gl_delete_vertex_arrays(int32_t n,const uint32_t*p){StableGLDeleteVertexArrays f=glDeleteVertexArrays_fn();if(f)f(n,p);}



/* ------------------------------ ML / AI runtime --------------------------- */

typedef enum StableTensorDType { STABLE_TENSOR_F32 = 1, STABLE_TENSOR_F64 = 2 } StableTensorDType;
#define STABLE_TENSOR_MAX_RANK 8

typedef struct StableTensor {
    uint8_t dtype;
    uint8_t rank;
    uint8_t contiguous;
    uint8_t reserved;
    size_t len;
    size_t shape[STABLE_TENSOR_MAX_RANK];
    size_t stride[STABLE_TENSOR_MAX_RANK];
    void* data;
} StableTensor;

typedef struct StableGradNode {
    int op;
    StableTensor* a;
    StableTensor* b;
    StableTensor* out;
    double scalar;
} StableGradNode;

typedef struct StableGradEntry {
    StableTensor* tensor;
    StableTensor* grad;
} StableGradEntry;

typedef struct StableGradTape {
    StableGradNode* nodes;
    size_t node_count;
    size_t node_cap;
    StableGradEntry* entries;
    size_t entry_count;
    size_t entry_cap;
} StableGradTape;

static void stable_tensor_die(const char* message) { stable_set_error(-30, message); }

static size_t stable_tensor_numel(uint8_t rank, const size_t* shape) {
    size_t n = 1;
    for (uint8_t i = 0; i < rank; ++i) {
        if (shape[i] == 0 || n > SIZE_MAX / shape[i]) return 0;
        n *= shape[i];
    }
    return n;
}

static StableTensor* stable_tensor_alloc(uint8_t dtype, uint8_t rank, const size_t* shape) {
    if (rank == 0 || rank > STABLE_TENSOR_MAX_RANK) { stable_tensor_die("tensor rank must be 1..8"); return NULL; }
    const size_t n = stable_tensor_numel(rank, shape);
    if (!n) { stable_tensor_die("invalid or overflowing tensor shape"); return NULL; }
    const size_t element_size = dtype == STABLE_TENSOR_F32 ? sizeof(float) : sizeof(double);
    if (n > SIZE_MAX / element_size) { stable_tensor_die("tensor allocation overflow"); return NULL; }
    StableTensor* t = (StableTensor*)calloc(1, sizeof(*t));
    if (!t) { stable_tensor_die("tensor metadata allocation failed"); return NULL; }
    t->data = malloc(n * element_size);
    if (!t->data) { free(t); stable_tensor_die("tensor data allocation failed"); return NULL; }
    t->dtype = dtype; t->rank = rank; t->len = n; t->contiguous = 1;
    for (uint8_t i = 0; i < rank; ++i) t->shape[i] = shape[i];
    t->stride[rank - 1] = 1;
    for (int i = (int)rank - 2; i >= 0; --i) t->stride[i] = t->stride[i + 1] * t->shape[i + 1];
    return t;
}

static StableTensor* stable_tensor_zeros_shape(uint8_t dtype, uint8_t rank, const size_t* shape, int ones) {
    StableTensor* t = stable_tensor_alloc(dtype, rank, shape); if (!t) return NULL;
    const size_t bytes = t->len * (dtype == STABLE_TENSOR_F32 ? sizeof(float) : sizeof(double));
    if (ones) {
        if (dtype == STABLE_TENSOR_F32) { float* p=(float*)t->data; for(size_t i=0;i<t->len;++i)p[i]=1.0f; }
        else { double* p=(double*)t->data; for(size_t i=0;i<t->len;++i)p[i]=1.0; }
    } else memset(t->data, 0, bytes);
    return t;
}

void* __stable_tensor_zeros1(uint64_t a){size_t d[1]={(size_t)a};return stable_tensor_zeros_shape(STABLE_TENSOR_F32,1,d,0);}
void* __stable_tensor_zeros2(uint64_t a,uint64_t b){size_t d[2]={(size_t)a,(size_t)b};return stable_tensor_zeros_shape(STABLE_TENSOR_F32,2,d,0);}
void* __stable_tensor_zeros3(uint64_t a,uint64_t b,uint64_t c){size_t d[3]={(size_t)a,(size_t)b,(size_t)c};return stable_tensor_zeros_shape(STABLE_TENSOR_F32,3,d,0);}
void* __stable_tensor_zeros4(uint64_t a,uint64_t b,uint64_t c,uint64_t d0){size_t d[4]={(size_t)a,(size_t)b,(size_t)c,(size_t)d0};return stable_tensor_zeros_shape(STABLE_TENSOR_F32,4,d,0);}
void* __stable_tensor_ones1(uint64_t a){size_t d[1]={(size_t)a};return stable_tensor_zeros_shape(STABLE_TENSOR_F32,1,d,1);}
void* __stable_tensor_ones2(uint64_t a,uint64_t b){size_t d[2]={(size_t)a,(size_t)b};return stable_tensor_zeros_shape(STABLE_TENSOR_F32,2,d,1);}
void* __stable_tensor_ones3(uint64_t a,uint64_t b,uint64_t c){size_t d[3]={(size_t)a,(size_t)b,(size_t)c};return stable_tensor_zeros_shape(STABLE_TENSOR_F32,3,d,1);}
void* __stable_tensor_ones4(uint64_t a,uint64_t b,uint64_t c,uint64_t d0){size_t d[4]={(size_t)a,(size_t)b,(size_t)c,(size_t)d0};return stable_tensor_zeros_shape(STABLE_TENSOR_F32,4,d,1);}
void* __stable_tensor_zeros_f32(uint64_t n){return __stable_tensor_zeros1(n);}
void* __stable_tensor_ones_f32(uint64_t n){return __stable_tensor_ones1(n);}
void* __stable_tensor_zeros_f64(uint64_t n){size_t d[1]={(size_t)n};return stable_tensor_zeros_shape(STABLE_TENSOR_F64,1,d,0);}
void* __stable_tensor_ones_f64(uint64_t n){size_t d[1]={(size_t)n};return stable_tensor_zeros_shape(STABLE_TENSOR_F64,1,d,1);}

void* __stable_tensor_from1_f32(const float* data, uint64_t n){size_t d[1]={(size_t)n};StableTensor*t=stable_tensor_alloc(STABLE_TENSOR_F32,1,d);if(t&&data)memcpy(t->data,data,(size_t)n*sizeof(float));return t;}
void* __stable_tensor_from1_f64(const double* data, uint64_t n){size_t d[1]={(size_t)n};StableTensor*t=stable_tensor_alloc(STABLE_TENSOR_F64,1,d);if(t&&data)memcpy(t->data,data,(size_t)n*sizeof(double));return t;}
void* __stable_tensor_from2_f32(const float* data,uint64_t r,uint64_t c){size_t d[2]={(size_t)r,(size_t)c};StableTensor*t=stable_tensor_alloc(STABLE_TENSOR_F32,2,d);if(t&&data)memcpy(t->data,data,t->len*sizeof(float));return t;}
void* __stable_tensor_from2_f64(const double* data,uint64_t r,uint64_t c){size_t d[2]={(size_t)r,(size_t)c};StableTensor*t=stable_tensor_alloc(STABLE_TENSOR_F64,2,d);if(t&&data)memcpy(t->data,data,t->len*sizeof(double));return t;}

void* __stable_tensor_clone(void* raw){StableTensor*t=(StableTensor*)raw;if(!t)return NULL;StableTensor*o=stable_tensor_alloc(t->dtype,t->rank,t->shape);if(!o)return NULL;memcpy(o->data,t->data,t->len*(t->dtype==STABLE_TENSOR_F32?sizeof(float):sizeof(double)));return o;}
void* __stable_tensor_contiguous(void* raw){return __stable_tensor_clone(raw);}
void __stable_tensor_free(void* raw){StableTensor*t=(StableTensor*)raw;if(!t)return;free(t->data);free(t);}
uint64_t __stable_tensor_rank(void* raw){StableTensor*t=(StableTensor*)raw;return t?t->rank:0;}
uint64_t __stable_tensor_len(void* raw){StableTensor*t=(StableTensor*)raw;return t?(uint64_t)t->len:0;}
uint64_t __stable_tensor_dim(void* raw,uint64_t axis){StableTensor*t=(StableTensor*)raw;if(!t||axis>=t->rank)return 0;return (uint64_t)t->shape[axis];}
uint64_t __stable_tensor_stride(void* raw,uint64_t axis){StableTensor*t=(StableTensor*)raw;if(!t||axis>=t->rank)return 0;return (uint64_t)t->stride[axis];}
uint32_t __stable_tensor_dtype(void* raw){StableTensor*t=(StableTensor*)raw;return t?t->dtype:0;}
uint32_t __stable_tensor_is_contiguous(void* raw){StableTensor*t=(StableTensor*)raw;return t&&t->contiguous;}
void* __stable_tensor_data_f32(void* raw){StableTensor*t=(StableTensor*)raw;if(!t||t->dtype!=STABLE_TENSOR_F32){stable_tensor_die("Tensor.dataF32 requires f32 tensor");return NULL;}return t->data;}
void* __stable_tensor_data_f64(void* raw){StableTensor*t=(StableTensor*)raw;if(!t||t->dtype!=STABLE_TENSOR_F64){stable_tensor_die("Tensor.dataF64 requires f64 tensor");return NULL;}return t->data;}

static size_t stable_tensor_offset(const StableTensor*t,const uint64_t* idx){size_t off=0;for(uint8_t i=0;i<t->rank;++i){if(idx[i]>=t->shape[i])return SIZE_MAX;off += (size_t)idx[i]*t->stride[i];}return off;}
static double stable_tensor_get_idx(const StableTensor*t,const uint64_t*idx){size_t off=stable_tensor_offset(t,idx);if(off==SIZE_MAX){stable_tensor_die("tensor index out of bounds");return 0.0;}return t->dtype==STABLE_TENSOR_F32?(double)((float*)t->data)[off]:((double*)t->data)[off];}
static void stable_tensor_set_idx(StableTensor*t,const uint64_t*idx,double v){size_t off=stable_tensor_offset(t,idx);if(off==SIZE_MAX){stable_tensor_die("tensor index out of bounds");return;}if(t->dtype==STABLE_TENSOR_F32)((float*)t->data)[off]=(float)v;else((double*)t->data)[off]=v;}
double __stable_tensor_get1(void*raw,uint64_t a){uint64_t i[1]={a};return stable_tensor_get_idx((StableTensor*)raw,i);}
double __stable_tensor_get2(void*raw,uint64_t a,uint64_t b){uint64_t i[2]={a,b};return stable_tensor_get_idx((StableTensor*)raw,i);}
double __stable_tensor_get3(void*raw,uint64_t a,uint64_t b,uint64_t c){uint64_t i[3]={a,b,c};return stable_tensor_get_idx((StableTensor*)raw,i);}
void __stable_tensor_set1(void*raw,uint64_t a,double v){uint64_t i[1]={a};stable_tensor_set_idx((StableTensor*)raw,i,v);}
void __stable_tensor_set2(void*raw,uint64_t a,uint64_t b,double v){uint64_t i[2]={a,b};stable_tensor_set_idx((StableTensor*)raw,i,v);}
void __stable_tensor_set3(void*raw,uint64_t a,uint64_t b,uint64_t c,double v){uint64_t i[3]={a,b,c};stable_tensor_set_idx((StableTensor*)raw,i,v);}

static int stable_tensor_same_shape(const StableTensor*a,const StableTensor*b){if(!a||!b||a->dtype!=b->dtype||a->rank!=b->rank||a->len!=b->len)return 0;for(uint8_t i=0;i<a->rank;++i)if(a->shape[i]!=b->shape[i])return 0;return 1;}
static void stable_tensor_apply_binary(StableTensor*o,const StableTensor*a,const StableTensor*b,int op){if(a->dtype==STABLE_TENSOR_F32){const float*x=(const float*)a->data,*y=(const float*)b->data;float*z=(float*)o->data;for(size_t i=0;i<a->len;++i)z[i]=op==0?x[i]+y[i]:op==1?x[i]-y[i]:op==2?x[i]*y[i]:(y[i]==0?0:x[i]/y[i]);}else{const double*x=(const double*)a->data,*y=(const double*)b->data;double*z=(double*)o->data;for(size_t i=0;i<a->len;++i)z[i]=op==0?x[i]+y[i]:op==1?x[i]-y[i]:op==2?x[i]*y[i]:(y[i]==0?0:x[i]/y[i]);}}
static void* stable_tensor_binary(void*ra,void*rb,int op){StableTensor*a=(StableTensor*)ra,*b=(StableTensor*)rb;if(!stable_tensor_same_shape(a,b)){stable_tensor_die("tensor binary operations require identical shapes and dtypes");return NULL;}StableTensor*o=stable_tensor_alloc(a->dtype,a->rank,a->shape);if(o)stable_tensor_apply_binary(o,a,b,op);return o;}
void* __stable_tensor_add(void*a,void*b){return stable_tensor_binary(a,b,0);} void* __stable_tensor_sub(void*a,void*b){return stable_tensor_binary(a,b,1);} void* __stable_tensor_mul(void*a,void*b){return stable_tensor_binary(a,b,2);} void* __stable_tensor_div(void*a,void*b){return stable_tensor_binary(a,b,3);}

void* __stable_tensor_scale(void*raw,double scalar){StableTensor*a=(StableTensor*)raw;if(!a)return NULL;StableTensor*o=stable_tensor_alloc(a->dtype,a->rank,a->shape);if(!o)return NULL;if(a->dtype==STABLE_TENSOR_F32){const float*x=a->data;float*y=o->data;for(size_t i=0;i<a->len;++i)y[i]=(float)(x[i]*scalar);}else{const double*x=a->data;double*y=o->data;for(size_t i=0;i<a->len;++i)y[i]=x[i]*scalar;}return o;}

static void* stable_tensor_unary(void*raw,int op){StableTensor*a=(StableTensor*)raw;if(!a)return NULL;StableTensor*o=stable_tensor_alloc(a->dtype,a->rank,a->shape);if(!o)return NULL;for(size_t i=0;i<a->len;++i){double x=a->dtype==STABLE_TENSOR_F32?(double)((float*)a->data)[i]:((double*)a->data)[i];double y=op==0?(x>0?x:0):op==1?1.0/(1.0+exp(-x)):tanh(x);if(o->dtype==STABLE_TENSOR_F32)((float*)o->data)[i]=(float)y;else((double*)o->data)[i]=y;}return o;}
void* __stable_tensor_relu(void*a){return stable_tensor_unary(a,0);} void* __stable_tensor_sigmoid(void*a){return stable_tensor_unary(a,1);} void* __stable_tensor_tanh(void*a){return stable_tensor_unary(a,2);}

void* __stable_tensor_matmul(void*ra,void*rb){StableTensor*a=(StableTensor*)ra,*b=(StableTensor*)rb;if(!a||!b||a->rank!=2||b->rank!=2||a->shape[1]!=b->shape[0]||a->dtype!=b->dtype){stable_tensor_die("Tensor.matmul requires compatible rank-2 tensors with matching dtype");return NULL;}size_t d[2]={a->shape[0],b->shape[1]};StableTensor*o=stable_tensor_alloc(a->dtype,2,d);if(!o)return NULL;const size_t m=a->shape[0],k=a->shape[1],n=b->shape[1];if(a->dtype==STABLE_TENSOR_F32){const float*A=a->data,*B=b->data;float*C=o->data;memset(C,0,o->len*sizeof(float));const size_t BS=32;for(size_t ii=0;ii<m;ii+=BS)for(size_t kk=0;kk<k;kk+=BS)for(size_t jj=0;jj<n;jj+=BS){size_t iend=ii+BS<m?ii+BS:m,kend=kk+BS<k?kk+BS:k,jend=jj+BS<n?jj+BS:n;for(size_t i=ii;i<iend;++i)for(size_t p=kk;p<kend;++p){const float av=A[i*k+p];for(size_t j=jj;j<jend;++j)C[i*n+j]+=av*B[p*n+j];}}}else{const double*A=a->data,*B=b->data;double*C=o->data;memset(C,0,o->len*sizeof(double));const size_t BS=32;for(size_t ii=0;ii<m;ii+=BS)for(size_t kk=0;kk<k;kk+=BS)for(size_t jj=0;jj<n;jj+=BS){size_t iend=ii+BS<m?ii+BS:m,kend=kk+BS<k?kk+BS:k,jend=jj+BS<n?jj+BS:n;for(size_t i=ii;i<iend;++i)for(size_t p=kk;p<kend;++p){const double av=A[i*k+p];for(size_t j=jj;j<jend;++j)C[i*n+j]+=av*B[p*n+j];}}}return o;}

void* __stable_tensor_softmax(void*raw,uint64_t axis){StableTensor*a=(StableTensor*)raw;if(!a||axis>=a->rank){stable_tensor_die("softmax axis out of range");return NULL;}StableTensor*o=stable_tensor_alloc(a->dtype,a->rank,a->shape);if(!o)return NULL; if(a->dtype==STABLE_TENSOR_F32){float*x=a->data,*y=o->data;size_t outer=1;for(size_t i=0;i<axis;++i)outer*=a->shape[i];size_t inner=1;for(size_t i=axis+1;i<a->rank;++i)inner*=a->shape[i];size_t dim=a->shape[axis];for(size_t q=0;q<outer;++q)for(size_t r=0;r<inner;++r){float mx=-INFINITY;for(size_t j=0;j<dim;++j){float v=x[q*dim*inner+j*inner+r];if(v>mx)mx=v;}double sum=0;for(size_t j=0;j<dim;++j){double e=exp((double)x[q*dim*inner+j*inner+r]-mx);y[q*dim*inner+j*inner+r]=(float)e;sum+=e;}for(size_t j=0;j<dim;++j)y[q*dim*inner+j*inner+r]=(float)(y[q*dim*inner+j*inner+r]/sum);}}else{double*x=a->data,*y=o->data;size_t outer=1;for(size_t i=0;i<axis;++i)outer*=a->shape[i];size_t inner=1;for(size_t i=axis+1;i<a->rank;++i)inner*=a->shape[i];size_t dim=a->shape[axis];for(size_t q=0;q<outer;++q)for(size_t r=0;r<inner;++r){double mx=-INFINITY;for(size_t j=0;j<dim;++j){double v=x[q*dim*inner+j*inner+r];if(v>mx)mx=v;}double sum=0;for(size_t j=0;j<dim;++j){double e=exp(x[q*dim*inner+j*inner+r]-mx);y[q*dim*inner+j*inner+r]=e;sum+=e;}for(size_t j=0;j<dim;++j)y[q*dim*inner+j*inner+r]/=sum;}}return o;}

double __stable_tensor_sum(void*raw){StableTensor*t=(StableTensor*)raw;if(!t)return 0;double s=0;if(t->dtype==STABLE_TENSOR_F32){float*p=t->data;for(size_t i=0;i<t->len;++i)s+=p[i];}else{double*p=t->data;for(size_t i=0;i<t->len;++i)s+=p[i];}return s;}
double __stable_tensor_mean(void*raw){StableTensor*t=(StableTensor*)raw;if(!t||!t->len)return 0;return __stable_tensor_sum(raw)/(double)t->len;}
double __stable_tensor_l2norm(void*raw){StableTensor*t=(StableTensor*)raw;if(!t)return 0;double s=0;if(t->dtype==STABLE_TENSOR_F32){float*p=t->data;for(size_t i=0;i<t->len;++i)s+=(double)p[i]*p[i];}else{double*p=t->data;for(size_t i=0;i<t->len;++i)s+=p[i]*p[i];}return sqrt(s);}
double __stable_tensor_dot(void*ra,void*rb){StableTensor*a=(StableTensor*)ra,*b=(StableTensor*)rb;if(!stable_tensor_same_shape(a,b)){stable_tensor_die("Tensor.dot requires identical shapes and dtypes");return 0;}double s=0;if(a->dtype==STABLE_TENSOR_F32){float*x=a->data,*y=b->data;for(size_t i=0;i<a->len;++i)s+=(double)x[i]*y[i];}else{double*x=a->data,*y=b->data;for(size_t i=0;i<a->len;++i)s+=x[i]*y[i];}return s;}
uint64_t __stable_tensor_argmax(void*raw,uint64_t axis){StableTensor*t=(StableTensor*)raw;if(!t||t->rank!=1||axis!=0){stable_tensor_die("Tensor.argmax currently requires a rank-1 tensor and axis 0");return 0;}uint64_t best=0;double bestv=-INFINITY;for(size_t i=0;i<t->len;++i){double v=t->dtype==STABLE_TENSOR_F32?(double)((float*)t->data)[i]:((double*)t->data)[i];if(v>bestv){bestv=v;best=(uint64_t)i;}}return best;}

static void* stable_tensor_reshape(void*raw,uint8_t rank,const uint64_t*dims){StableTensor*t=(StableTensor*)raw;if(!t||rank>STABLE_TENSOR_MAX_RANK||rank==0)return NULL;size_t shape[STABLE_TENSOR_MAX_RANK]={0};for(uint8_t i=0;i<rank;++i)shape[i]=(size_t)dims[i];if(stable_tensor_numel(rank,shape)!=t->len){stable_tensor_die("reshape changes tensor element count");return NULL;}StableTensor*o=stable_tensor_alloc(t->dtype,rank,shape);if(!o)return NULL;memcpy(o->data,t->data,t->len*(t->dtype==STABLE_TENSOR_F32?sizeof(float):sizeof(double)));return o;}
void* __stable_tensor_reshape2(void*raw,uint64_t a,uint64_t b){uint64_t d[2]={a,b};return stable_tensor_reshape(raw,2,d);}void* __stable_tensor_reshape3(void*raw,uint64_t a,uint64_t b,uint64_t c){uint64_t d[3]={a,b,c};return stable_tensor_reshape(raw,3,d);}void* __stable_tensor_reshape4(void*raw,uint64_t a,uint64_t b,uint64_t c,uint64_t e){uint64_t d[4]={a,b,c,e};return stable_tensor_reshape(raw,4,d);}
void* __stable_tensor_transpose2(void*raw){StableTensor*a=(StableTensor*)raw;if(!a||a->rank!=2){stable_tensor_die("transpose2 requires rank-2 tensor");return NULL;}size_t d[2]={a->shape[1],a->shape[0]};StableTensor*o=stable_tensor_alloc(a->dtype,2,d);if(!o)return NULL;for(size_t i=0;i<a->shape[0];++i)for(size_t j=0;j<a->shape[1];++j){double v=__stable_tensor_get2(a,i,j);__stable_tensor_set2(o,j,i,v);}return o;}
void* __stable_tensor_slice(void*raw,uint64_t axis,uint64_t start,uint64_t end,uint64_t step){StableTensor*a=(StableTensor*)raw;if(!a||axis>=a->rank||step==0||start>end||end>a->shape[axis]){stable_tensor_die("invalid tensor slice");return NULL;}size_t count=(size_t)((end-start+step-1)/step);size_t shape[STABLE_TENSOR_MAX_RANK];for(uint8_t i=0;i<a->rank;++i)shape[i]=a->shape[i];shape[axis]=count;StableTensor*o=stable_tensor_alloc(a->dtype,a->rank,shape);if(!o)return NULL;for(size_t dst=0;dst<o->len;++dst){size_t rem=dst,src_off=0;for(uint8_t i=0;i<o->rank;++i){size_t idx=rem/o->stride[i];rem%=o->stride[i];size_t src_idx=(i==axis)?(size_t)start+idx*(size_t)step:idx;src_off+=src_idx*a->stride[i];}if(a->dtype==STABLE_TENSOR_F32)((float*)o->data)[dst]=((float*)a->data)[src_off];else((double*)o->data)[dst]=((double*)a->data)[src_off];}return o;}
void __stable_tensor_fill(void*raw,double v){StableTensor*t=(StableTensor*)raw;if(!t)return;if(t->dtype==STABLE_TENSOR_F32){float*p=t->data;for(size_t i=0;i<t->len;++i)p[i]=(float)v;}else{double*p=t->data;for(size_t i=0;i<t->len;++i)p[i]=v;}}
void* __stable_tensor_conv2d(void*ri,void*rw,uint64_t stride,uint64_t padding){StableTensor*in=(StableTensor*)ri,*w=(StableTensor*)rw;if(!in||!w||in->rank!=4||w->rank!=4||in->dtype!=w->dtype||in->shape[1]!=w->shape[1]||stride==0){stable_tensor_die("conv2d expects NCHW input and OIHW weights");return NULL;}size_t n=in->shape[0],c=in->shape[1],h=in->shape[2],ww=in->shape[3],oc=w->shape[0],kh=w->shape[2],kw=w->shape[3];if(h+2*padding<kh||ww+2*padding<kw)return NULL;size_t oh=(h+2*padding-kh)/stride+1,ow=(ww+2*padding-kw)/stride+1;size_t d[4]={n,oc,oh,ow};StableTensor*o=stable_tensor_alloc(in->dtype,4,d);if(!o)return NULL;for(size_t bn=0;bn<n;++bn)for(size_t co=0;co<oc;++co)for(size_t oy=0;oy<oh;++oy)for(size_t ox=0;ox<ow;++ox){double acc=0;for(size_t ci=0;ci<c;++ci)for(size_t ky=0;ky<kh;++ky)for(size_t kx=0;kx<kw;++kx){int iy=(int)(oy*stride+ky)-(int)padding,ix=(int)(ox*stride+kx)-(int)padding;if(iy<0||ix<0||iy>=(int)h||ix>=(int)ww)continue;uint64_t ii[4]={bn,ci,(uint64_t)iy,(uint64_t)ix},wi[4]={co,ci,ky,kx};acc+=stable_tensor_get_idx(in,ii)*stable_tensor_get_idx(w,wi);}uint64_t oi[4]={bn,co,oy,ox};stable_tensor_set_idx(o,oi,acc);}return o;}

static StableGradEntry* stable_grad_entry(StableGradTape*t,StableTensor*x,int create){for(size_t i=0;i<t->entry_count;++i)if(t->entries[i].tensor==x)return &t->entries[i];if(!create)return NULL;if(t->entry_count==t->entry_cap){size_t cap=t->entry_cap?t->entry_cap*2:16;StableGradEntry*e=(StableGradEntry*)realloc(t->entries,cap*sizeof(*e));if(!e)return NULL;t->entries=e;t->entry_cap=cap;}StableGradEntry*r=&t->entries[t->entry_count++];r->tensor=x;r->grad=NULL;return r;}
static int stable_grad_push_node(StableGradTape*t,int op,StableTensor*a,StableTensor*b,StableTensor*out,double scalar){if(t->node_count==t->node_cap){size_t cap=t->node_cap?t->node_cap*2:32;StableGradNode*n=(StableGradNode*)realloc(t->nodes,cap*sizeof(*n));if(!n)return 0;t->nodes=n;t->node_cap=cap;}t->nodes[t->node_count++]=(StableGradNode){op,a,b,out,scalar};(void)stable_grad_entry(t,a,1);if(b)(void)stable_grad_entry(t,b,1);(void)stable_grad_entry(t,out,1);return 1;}
void* __stable_grad_create(void){return calloc(1,sizeof(StableGradTape));}
void __stable_grad_watch(void*raw,void*tensor){StableGradTape*t=(StableGradTape*)raw;if(t)stable_grad_entry(t,(StableTensor*)tensor,1);}
static void* stable_grad_op2(void*tr,void*ra,void*rb,int op){StableGradTape*t=tr;StableTensor*o= op==0?__stable_tensor_add(ra,rb):op==1?__stable_tensor_mul(ra,rb):__stable_tensor_matmul(ra,rb);if(o&&t&&!stable_grad_push_node(t,op,ra,rb,o,0)){__stable_tensor_free(o);return NULL;}return o;}
void* __stable_grad_add(void*t,void*a,void*b){return stable_grad_op2(t,a,b,0);} void* __stable_grad_mul(void*t,void*a,void*b){return stable_grad_op2(t,a,b,1);} void* __stable_grad_matmul(void*t,void*a,void*b){return stable_grad_op2(t,a,b,2);}
static void* stable_grad_unary(void*tr,void*ra,int op){StableGradTape*t=tr;StableTensor*o=op==3?__stable_tensor_relu(ra):__stable_tensor_tanh(ra);if(o&&t&&!stable_grad_push_node(t,op,ra,NULL,o,0)){__stable_tensor_free(o);return NULL;}return o;}
void* __stable_grad_relu(void*t,void*a){return stable_grad_unary(t,a,3);}void*__stable_grad_tanh(void*t,void*a){return stable_grad_unary(t,a,4);}
void* __stable_grad_sum(void*tr,void*a){StableGradTape*t=tr;double v=__stable_tensor_sum(a);size_t d[1]={1};StableTensor*o=stable_tensor_alloc(((StableTensor*)a)->dtype,1,d);if(!o)return NULL;stable_tensor_set_idx(o,(uint64_t[]){0},v);if(t&&!stable_grad_push_node(t,5,a,NULL,o,0)){__stable_tensor_free(o);return NULL;}return o;}
void* __stable_grad_scale(void*tr,void*a,double s){StableGradTape*t=tr;StableTensor*o=__stable_tensor_scale(a,s);if(o&&t&&!stable_grad_push_node(t,6,a,NULL,o,s)){__stable_tensor_free(o);return NULL;}return o;}
static StableTensor* stable_grad_zeros_like(const StableTensor*t){return stable_tensor_zeros_shape(t->dtype,t->rank,t->shape,0);}
static void stable_grad_accum(StableTensor**slot,StableTensor*delta){if(!delta)return;if(!*slot){*slot=delta;return;}StableTensor*sum=__stable_tensor_add(*slot,delta);__stable_tensor_free(*slot);__stable_tensor_free(delta);*slot=sum;}
void __stable_grad_backward(void*tr,void*loss){StableGradTape*t=tr;StableTensor*l=(StableTensor*)loss;if(!t||!l)return;StableGradEntry*le=stable_grad_entry(t,l,1);if(!le->grad){le->grad=stable_grad_zeros_like(l);__stable_tensor_fill(le->grad,1.0);}for(size_t ni=t->node_count;ni>0;--ni){StableGradNode*n=&t->nodes[ni-1];StableGradEntry*oe=stable_grad_entry(t,n->out,0);if(!oe||!oe->grad)continue;StableTensor*g=oe->grad;if(n->op==0){stable_grad_accum(&stable_grad_entry(t,n->a,1)->grad,__stable_tensor_clone(g));stable_grad_accum(&stable_grad_entry(t,n->b,1)->grad,__stable_tensor_clone(g));}
 else if(n->op==1){StableTensor*ga=__stable_tensor_mul(g,n->b),*gb=__stable_tensor_mul(g,n->a);stable_grad_accum(&stable_grad_entry(t,n->a,1)->grad,ga);stable_grad_accum(&stable_grad_entry(t,n->b,1)->grad,gb);}
 else if(n->op==2){StableTensor*bt=__stable_tensor_transpose2(n->b),*at=__stable_tensor_transpose2(n->a);StableTensor*ga=bt?__stable_tensor_matmul(g,bt):NULL;StableTensor*gb=at?__stable_tensor_matmul(at,g):NULL;if(bt)__stable_tensor_free(bt);if(at)__stable_tensor_free(at);stable_grad_accum(&stable_grad_entry(t,n->a,1)->grad,ga);stable_grad_accum(&stable_grad_entry(t,n->b,1)->grad,gb);}
 else if(n->op==3){StableTensor*mask=__stable_tensor_clone(n->a);if(mask){if(mask->dtype==STABLE_TENSOR_F32){float*p=mask->data;for(size_t i=0;i<mask->len;++i)p[i]=p[i]>0?1.0f:0.0f;}else{double*p=mask->data;for(size_t i=0;i<mask->len;++i)p[i]=p[i]>0?1.0:0.0;}}stable_grad_accum(&stable_grad_entry(t,n->a,1)->grad,__stable_tensor_mul(g,mask));if(mask)__stable_tensor_free(mask);}
 else if(n->op==4){StableTensor*sq=__stable_tensor_mul(n->out,n->out);StableTensor*one=stable_tensor_zeros_shape(n->out->dtype,n->out->rank,n->out->shape,1);StableTensor*d=stable_tensor_binary(one,sq,1);StableTensor*grad=__stable_tensor_mul(g,d);if(sq)__stable_tensor_free(sq);if(one)__stable_tensor_free(one);if(d)__stable_tensor_free(d);stable_grad_accum(&stable_grad_entry(t,n->a,1)->grad,grad);}
 else if(n->op==5){StableTensor*ga=stable_grad_zeros_like(n->a);if(ga){__stable_tensor_fill(ga,__stable_tensor_sum(g));stable_grad_accum(&stable_grad_entry(t,n->a,1)->grad,ga);}}
 else if(n->op==6){stable_grad_accum(&stable_grad_entry(t,n->a,1)->grad,__stable_tensor_scale(g,n->scalar));}}
}
void* __stable_grad_get(void*tr,void*tensor){StableGradTape*t=tr;StableGradEntry*e=t?stable_grad_entry(t,(StableTensor*)tensor,0):NULL;return e&&e->grad?__stable_tensor_clone(e->grad):NULL;}
void __stable_grad_free(void*raw){StableGradTape*t=raw;if(!t)return;for(size_t i=0;i<t->entry_count;++i)if(t->entries[i].grad)__stable_tensor_free(t->entries[i].grad);free(t->entries);free(t->nodes);free(t);}

#if defined(_WIN32)
static int stable_library_exists(const char* name){HMODULE h=LoadLibraryA(name);if(!h)return 0;FreeLibrary(h);return 1;}
#else
#include <dlfcn.h>
static int stable_library_exists(const char* name){void*h=dlopen(name,RTLD_LAZY|RTLD_LOCAL);if(!h)return 0;dlclose(h);return 1;}
#endif
uint32_t __stable_accel_cuda_available(void){
#if defined(_WIN32)
    return stable_library_exists("nvcuda.dll") || stable_library_exists("cudart64_12.dll") || stable_library_exists("cudart64_11.dll");
#else
    return stable_library_exists("libcudart.so") || stable_library_exists("libcudart.so.12") || stable_library_exists("libcuda.so.1");
#endif
}
uint32_t __stable_accel_rocm_available(void){
#if defined(_WIN32)
    return stable_library_exists("amdhip64.dll");
#else
    return stable_library_exists("libamdhip64.so") || stable_library_exists("libhiprtc.so");
#endif
}
uint32_t __stable_accel_metal_available(void){
#if defined(__APPLE__)
    return 1;
#else
    return 0;
#endif
}
uint32_t __stable_accel_blas_available(void){
#if defined(_WIN32)
    return stable_library_exists("openblas.dll") || stable_library_exists("blas.dll");
#else
    return stable_library_exists("libopenblas.so") || stable_library_exists("libopenblas.so.0") || stable_library_exists("libblas.so.3");
#endif
}
const char* __stable_accel_backend(void){if(__stable_accel_cuda_available())return "cuda";if(__stable_accel_rocm_available())return "rocm";if(__stable_accel_metal_available())return "metal";if(__stable_accel_blas_available())return "cpu-blas";return "cpu";}
