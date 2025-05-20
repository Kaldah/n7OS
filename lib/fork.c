#include <unistd.h>
#include <n7OS/processus.h>

/* code de la fonction
    int fork();
    pas d'argument -> appel à la fonction d'enveloppe syscall0
*/
syscall0(int, fork)
