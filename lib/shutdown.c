#include <unistd.h>

/* code de la fonction
   int shutdown(int n);
   1 argument -> appel à la fonction d'enveloppe syscall1
*/
syscall1(int, shutdown, int, n)
