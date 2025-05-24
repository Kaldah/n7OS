#include <unistd.h>

/* code de la fonction
    int get_pid();
    pas d'argument -> appel à la fonction d'enveloppe syscall0
*/
syscall0(int, get_pid)
