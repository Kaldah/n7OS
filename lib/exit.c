#include <unistd.h>

/* code de la fonction
   int exit();
   pas d'argument -> appel à la fonction d'enveloppe syscall0
*/
syscall0(int, exit)