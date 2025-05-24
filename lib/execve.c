#include <unistd.h>

/* code de la fonction
    int execve(void (*entry)(void))
    1 argument -> appel à la fonction d'enveloppe syscall1
*/

syscall1(int, execve, void*, entry)