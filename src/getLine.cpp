/** getLine
  *     Gets the next line of text input from the specified file stream
  *     or memory buffer.
  *
  * Last modified: 2004-08-14
  *
  * Copyright (c) 2004 James D. Lin
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted provided that the following conditions are
  * met:
  *
  * * Redistributions of source code must retain the above copyright
  *   notice, this list of conditions and the following disclaimer.
  *
  * * Redistributions in binary form must reproduce the above copyright
  *   notice, this list of conditions and the following disclaimer in the
  *   documentation and/or other materials provided with the distribution.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
  * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
  * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
  * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  */

#pragma warning (4 : 4290)

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

#include "ggets.h"
#include "getLine.h"


/** getLine
  *
  *     Reads the next line of input.
  *
  * PARAMETERS:
  *     IN/OUT fp  - the file stream to read from
  *     OUT linePP - on output, the retrieved line;
  *                  on error, left untouched
  *
  * RETURNS:
  *     true if a line was read;
  *     false otherwise
  *
  * SIDE EFFECTS:
  *     allocates and fills <*linePP>;
  *     it is the caller's responsibility to free <*linePP> with freeLine
  *       when done
  *
  * THROWS:
  *     std::bad_alloc - insufficient memory
  */
bool getLine(FILE* fp, char** linePP) throw(std::bad_alloc)
{
    bool empty = false;

    assert(fp != NULL && linePP != NULL);

    char* lineP = NULL;
    int err = fggets(&lineP, fp);
    if (err == EOF)
    {
        empty = true;
    }
    else if (err > 0)
    {
        throw std::bad_alloc();
    }

    *linePP = lineP;
    return !empty;
}


/** getLine
  *
  *     Overloaded version of getLine.  See above.
  *
  * PARAMETERS:
  *     IN/OUT s   - on input, the memory buffer to read from;
  *                  on output, points to the start of the next line
  *     OUT linePP - on output, the retrieved line;
  *                  on error, left untouched
  *
  * RETURNS:
  *     See above.
  *
  * SIDE EFFECTS:
  *     See above.
  *
  * THROWS:
  *     See above.
  */
bool getLine(const char** s, char** linePP) throw(std::bad_alloc)
{
    bool empty = false;

    assert(s != NULL && linePP != NULL);

    const char* inP = *s;
    assert(inP != NULL);
    if (inP[0] == '\0')
    {
        empty = true;
        *linePP = NULL;
    }
    else
    {
        const char* p = strchr(inP, '\n');
        const char* nextP;
        char* lineP;

        if (p == NULL)
        {
            p = strchr(inP, '\0');
            nextP = p;
        }
        else
        {
            nextP = p + 1;
        }
        assert(p != NULL);

        size_t len = p - inP;
        lineP = (char*) malloc(len + 1);
        if (lineP == NULL) { throw std::bad_alloc(); }

        lineP[0] = '\0';
        strncat(lineP, inP, len);

        *linePP = lineP;
        *s = nextP;
    }
    return !empty;
}


void freeLine(char* lineP) throw()
{
    free(lineP);
}
