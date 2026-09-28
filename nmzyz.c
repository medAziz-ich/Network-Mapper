/*
 * nmzyz - a small multi-threaded TCP connect scanner.
 *
 * Only scan hosts you own or have explicit permission to test.
 */
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define DEFAULT_THREADS 100
#define DEFAULT_TIMEOUT_MS 1000
#define MAX_THREADS 1024

typedef enum { PORT_OPEN, PORT_CLOSED, PORT_FILTERED, PORT_ERROR } PortState;

typedef struct {
    uint32_t start_ip;      /* host byte order */
    uint64_t num_ports;
    uint16_t start_port;
    uint64_t total;         /* num_ips * num_ports */
    int timeout_ms;
    int verbose;
    atomic_uint_fast64_t next;
    atomic_uint_fast64_t n_open;
    pthread_mutex_t out_lock;
} ScanJob;

/* Try one TCP connect with a real timeout (non-blocking connect + poll). */
static PortState probe(uint32_t ip_host, uint16_t port, int timeout_ms) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        return PORT_ERROR;
    }

    int flags = fcntl(s, F_GETFL, 0);
    if (flags < 0 || fcntl(s, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(s);
        return PORT_ERROR;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(ip_host);

    PortState state;
    int ret = connect(s, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == 0) {
        state = PORT_OPEN;
    } else if (errno == EINPROGRESS || errno == EINTR) {
        struct pollfd pfd = { .fd = s, .events = POLLOUT };
        int pr;
        do {
            pr = poll(&pfd, 1, timeout_ms);
        } while (pr < 0 && errno == EINTR);

        if (pr == 0) {
            state = PORT_FILTERED;
        } else if (pr < 0) {
            state = PORT_ERROR;
        } else {
            int err = 0;
            socklen_t len = sizeof(err);
            if (getsockopt(s, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
                state = PORT_ERROR;
            } else if (err == 0) {
                state = PORT_OPEN;
            } else if (err == ECONNREFUSED) {
                state = PORT_CLOSED;
            } else {
                state = PORT_FILTERED;  /* unreachable, no route, ... */
            }
        }
    } else if (errno == ECONNREFUSED) {
        state = PORT_CLOSED;
    } else if (errno == EMFILE || errno == ENFILE) {
        state = PORT_ERROR;
    } else {
        state = PORT_FILTERED;
    }

    close(s);
    return state;
}

/* Workers pull the next (ip, port) pair from a shared counter. */
static void *worker(void *arg) {
    ScanJob *job = arg;

    for (;;) {
        uint64_t idx = atomic_fetch_add(&job->next, 1);
        if (idx >= job->total) {
            break;
        }

        uint32_t ip = job->start_ip + (uint32_t)(idx / job->num_ports);
        uint16_t port = (uint16_t)(job->start_port + (idx % job->num_ports));

        PortState st = probe(ip, port, job->timeout_ms);

        if (st == PORT_OPEN) {
            atomic_fetch_add(&job->n_open, 1);
        }
        if (st == PORT_OPEN || job->verbose) {
            struct in_addr a = { .s_addr = htonl(ip) };
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &a, ip_str, sizeof(ip_str));

            const char *label = "open";
            char mark = '+';
            if (st == PORT_CLOSED)   { label = "closed";   mark = '-'; }
            if (st == PORT_FILTERED) { label = "filtered"; mark = '~'; }
            if (st == PORT_ERROR)    { label = "error";    mark = '!'; }

            pthread_mutex_lock(&job->out_lock);
            printf("[%c] %s:%u %s\n", mark, ip_str, port, label);
            fflush(stdout);
            pthread_mutex_unlock(&job->out_lock);
        }
    }
    return NULL;
}

static int resolve(const char *host, uint32_t *out_host_order) {
    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int rc = getaddrinfo(host, NULL, &hints, &res);
    if (rc != 0) {
        fprintf(stderr, "Cannot resolve '%s': %s\n", host, gai_strerror(rc));
        return -1;
    }
    struct sockaddr_in *sin = (struct sockaddr_in *)res->ai_addr;
    *out_host_order = ntohl(sin->sin_addr.s_addr);
    freeaddrinfo(res);
    return 0;
}

static int parse_port(const char *s, uint16_t *out) {
    char *end;
    errno = 0;
    long v = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0' || v < 1 || v > 65535) {
        return -1;
    }
    *out = (uint16_t)v;
    return 0;
}

static int parse_int(const char *s, long min, long max, long *out) {
    char *end;
    errno = 0;
    long v = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0' || v < min || v > max) {
        return -1;
    }
    *out = v;
    return 0;
}

static void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [-t threads] [-w timeout_ms] [-v] "
        "<start IP|host> <end IP|host> <start port> <end port>\n"
        "  -t N   worker threads (default %d, max %d)\n"
        "  -w MS  per-connection timeout in ms (default %d)\n"
        "  -v     also print closed/filtered ports\n",
        prog, DEFAULT_THREADS, MAX_THREADS, DEFAULT_TIMEOUT_MS);
}

int main(int argc, char *argv[]) {
    long threads = DEFAULT_THREADS;
    long timeout_ms = DEFAULT_TIMEOUT_MS;
    int verbose = 0;
    int opt;

    while ((opt = getopt(argc, argv, "t:w:vh")) != -1) {
        switch (opt) {
        case 't':
            if (parse_int(optarg, 1, MAX_THREADS, &threads) != 0) {
                fprintf(stderr, "Invalid thread count.\n");
                return EXIT_FAILURE;
            }
            break;
        case 'w':
            if (parse_int(optarg, 1, 600000, &timeout_ms) != 0) {
                fprintf(stderr, "Invalid timeout.\n");
                return EXIT_FAILURE;
            }
            break;
        case 'v':
            verbose = 1;
            break;
        case 'h':
            usage(argv[0]);
            return EXIT_SUCCESS;
        default:
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (argc - optind != 4) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    uint32_t start_ip, end_ip;
    uint16_t start_port, end_port;

    if (resolve(argv[optind], &start_ip) != 0 ||
        resolve(argv[optind + 1], &end_ip) != 0) {
        return EXIT_FAILURE;
    }
    if (start_ip > end_ip) {
        fprintf(stderr, "Start IP must not be greater than end IP.\n");
        return EXIT_FAILURE;
    }
    if (parse_port(argv[optind + 2], &start_port) != 0 ||
        parse_port(argv[optind + 3], &end_port) != 0 ||
        start_port > end_port) {
        fprintf(stderr, "Invalid port range (1-65535, start <= end).\n");
        return EXIT_FAILURE;
    }

    ScanJob job;
    memset(&job, 0, sizeof(job));
    job.start_ip = start_ip;
    job.start_port = start_port;
    job.num_ports = (uint64_t)end_port - start_port + 1;
    job.total = ((uint64_t)end_ip - start_ip + 1) * job.num_ports;
    job.timeout_ms = (int)timeout_ms;
    job.verbose = verbose;
    atomic_init(&job.next, 0);
    atomic_init(&job.n_open, 0);
    pthread_mutex_init(&job.out_lock, NULL);

    if ((uint64_t)threads > job.total) {
        threads = (long)job.total;
    }

    pthread_t *tids = calloc((size_t)threads, sizeof(*tids));
    if (!tids) {
        perror("calloc");
        return EXIT_FAILURE;
    }

    long started = 0;
    for (long i = 0; i < threads; i++) {
        int rc = pthread_create(&tids[i], NULL, worker, &job);
        if (rc != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(rc));
            break;
        }
        started++;
    }
    if (started == 0) {
        free(tids);
        return EXIT_FAILURE;
    }
    for (long i = 0; i < started; i++) {
        pthread_join(tids[i], NULL);
    }

    fprintf(stderr, "Done: %llu probes, %llu open.\n",
            (unsigned long long)job.total,
            (unsigned long long)atomic_load(&job.n_open));

    pthread_mutex_destroy(&job.out_lock);
    free(tids);
    return EXIT_SUCCESS;
}
