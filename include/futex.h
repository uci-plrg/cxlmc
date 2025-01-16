#include <err.h>
#include <errno.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <limits.h>
#include <stdint.h>

inline void fwait(uint32_t *uaddr, uint32_t val) {
	if (syscall(SYS_futex, uaddr, FUTEX_WAIT, val, NULL, NULL, 0)==-1)
		if (errno != EAGAIN) {
			perror("Futex Wait");
			exit(1);
		}
}

inline void fwake(uint32_t *uaddr) {
	if(syscall(SYS_futex, uaddr, FUTEX_WAKE, INT_MAX, NULL, NULL, 0)==-1) {
		perror("Futex Wake");
		exit(1);
	}
}
