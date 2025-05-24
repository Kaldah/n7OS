#include <unistd.h>

/* code de la fonction
    int spawn(void (*entry)(void), const char *name);
    2 arguments -> appel à la fonction d'enveloppe syscall2
*/

syscall2(int, spawn, void*, entry, const char*, name)
