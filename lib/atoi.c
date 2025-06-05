#include <stdio.h>
#include <ctype.h>

int atoi(const char *nptr)
{
  int base=1;
  int res=0;
  int i;

  /* Make sure all chars are numbers */
  for (i=0; *(nptr+i); ++i) {
    if(!isdigit(*(nptr+i)))
      return -1;
  }

  i=0;
  while(nptr[++i])
    base *= 10;

  /* Actual conversion. It works like this: for example, 123 is obtained with
   1*100 + 2*10 + 3*1 */
  for (i=0; *(nptr+i); ++i ) {
    res += ((int)nptr[i] - 48) * base;
    base /= 10;
  }

  return res;
}