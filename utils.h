#ifndef _UTILS_H
#define _UTILS_H

#ifndef SIZE_MAX
#define SIZE_MAX ~((size_t)1)
#endif

#ifdef __GNUC__
#define UNUSED __attribute__ ((unused))
#else
#define UNUSED
#endif

#include <sys/param.h>
#include <stdbool.h>

typedef enum {
	LOG_DEBUG = 0,
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR,
	LOG_CRITICAL,
} log_level;

#define	RED	"\x1b[31;1m"
#define MAGENTA "\x1b[35;1m"
#define CYAN	"\x1b[36;1m"
#define BLUE	"\x1b[34;1m"
#define GREEN	"\x1b[32;1m"
#define WHITE	"\x1b[37;1m"
#define NOCOLOR	"\x1b[0m"


#ifdef DEBUG
#define GEN_FMT "in `%s'(%s:%d) "
#define __xlog(t, ...) _xlog(t, __VA_ARGS__)
#define xlog(t, _f, ...) __xlog(t, GEN_FMT _f, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)

#else
#define xlog(t, ...) _xlog(t, __VA_ARGS__)
#endif

void	_xlog(int type, const char* fmt, ...);
void* 	proxenet_xmalloc(size_t size);
void 	proxenet_xfree(void* ptr);
void 	proxenet_xzero(void* buf, size_t buflen);
void* 	proxenet_xrealloc(void* oldptr, size_t new_size);
bool	is_valid_path(char*);
bool	is_file(char*);
bool	is_readable_file(char*);
	
#endif /* _UTILS_H */

