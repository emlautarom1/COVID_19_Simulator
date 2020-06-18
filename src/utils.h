#include <time.h>

#if defined(DEBUG) && DEBUG
#define DEBUG_PRINT(fmt, args...) printf("[DBG]: %s:%d:%s(): " fmt, \
                                         __FILE__, __LINE__, __func__, ##args)
#else
#define DEBUG_PRINT(fmt, args...)
#endif

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
