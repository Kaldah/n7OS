#include <unistd.h>

/* code de la fonction
    int write(const char *s, int len);
    2 arguments -> appel à la fonction d'enveloppe syscall2
*/

syscall2(int, write, const char *, s, int, len)