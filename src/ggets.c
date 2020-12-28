/* File ggets.c  - goodgets is a safe alternative to gets */
/* By C.B. Falconer.  Public domain 2002-06-22            */
/*    attribution appreciated.                            */

/* Revised 2002-06-26  New prototype.
           2002-06-27  Incomplete final lines
 */

/* fggets and ggets [which is fggets(ln, stdin)] set *ln to
   a buffer filled with the next complete line from the text
   stream f.  The storage has been allocated within fggets,
   and is normally reduced to be an exact fit.  The trailing
   \n has been removed, so the resultant line is ready for
   dumping with puts.  The buffer will be as large as is
   required to hold the complete line.

   Note: this means a final file line without a \n terminator
   has an effective \n appended, as EOF occurs within the read.

   If no error occurs fggets returns 0.  If an EOF occurs on
   the input file, EOF is returned.  For memory allocation
   errors some positive value is returned.  In this case *ln
   may point to a partial line.  For other errors memory is
   freed and *ln is set to NULL.

   Freeing of assigned storage is the callers responsibility
 */

#include <stdio.h>
#include <string.h>  /* strchr */
#include <stdlib.h>
#include "ggets.h"

#define INITSIZE   112  /* power of 2 minus 16, helps malloc */
#define DELTASIZE (INITSIZE + 16)

enum {OK = 0, NOMEM};

int fggets(char* *ln, FILE *f)
{
   int     cursize, rdsize;
   char   *buffer, *temp, *rdpoint, *crlocn;

   *ln = NULL; /* default */
   if (NULL == (buffer = malloc(INITSIZE)))
      return NOMEM;
   cursize = rdsize = INITSIZE;
   rdpoint = buffer;
   *buffer = '\0';

   if (NULL == fgets(rdpoint, rdsize, f)) {
      free(buffer);
      return EOF;
   }
   /* initial read succeeded, now decide about expansion */
   while (NULL == (crlocn = strchr(rdpoint, '\n'))) {
      /* line is not completed, expand */

      /* set up cursize, rdpoint and rdsize, expand buffer */
      rdsize = DELTASIZE + 1;   /* allow for a final '\0' */
      cursize += DELTASIZE;
      if (NULL == (temp = realloc(buffer, (size_t)cursize))) {
         /* ran out of memory */
         *ln = buffer; /* partial line, next call may fail */
         return NOMEM;
      }
      buffer = temp;
      /* Read into the '\0' up */
      rdpoint = buffer + (cursize - DELTASIZE - 1);

      /* get the next piece of this line */
      if (NULL == fgets(rdpoint, rdsize, f)) {
         /* early EOF encountered */
         crlocn = strchr(buffer, '\0');
         break;
      }
   } /* while line not complete */

   *crlocn = '\0';  /* mark line end, strip \n */
   rdsize = (int) (crlocn - buffer);
   if (NULL == (temp = realloc(buffer, (size_t)rdsize + 1))) {
      *ln = buffer;  /* without reducing it */
      return OK;
   }
   *ln = temp;
   return OK;
} /* fggets */
/* End of ggets.c */
