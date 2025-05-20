#include <unistd.h>

/* code de la fonction
   int sleep(uint32_t seconds);
   pas d'argument -> appel à la fonction d'enveloppe syscall0
*/
syscall1(int, sleep, uint32_t, seconds)