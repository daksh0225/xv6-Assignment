typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef uint pde_t;

#define MAXAGE 30

int qsize[5];
int front[5],rear[5],qticks[5];
struct proc* q[5][200];