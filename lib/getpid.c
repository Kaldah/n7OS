#include <unistd.h>

/* code de la fonction
    pid_t getpid();
    pas d'argument -> appel à la fonction d'enveloppe syscall0
*/
syscall0(pid_t, getpid)
