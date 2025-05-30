#include <unistd.h>

/* code de la fonction
    int sys_execve(const char *filename, char *const argv[], char *const envp[])
    3 arguments -> appel à la fonction d'enveloppe syscall3
*/

syscall3(int, execve, const char*, filename, char *const*, argv, char *const*, envp)