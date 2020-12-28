/** RemapFrames
  *     An AviSynth plug-in that remaps the frame indices in a clip as
  *     specified by an input text file or by an input string.
  *
  * Last modified: 2004-12-24
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

// Windows grmrrmmlllff
#define NOMINMAX

#include <cstdio>
#include <cctype>
#include <cstring>
#include <cassert>
#include <cmath>

#include <algorithm>
#include <vector>

#include "Calc.h"
#include "ScopeGuard.h"

#include "getLine.h"
#include "RemapFramesParser.h"



// CLASS DEFINITIONS ---------------------------------------------------

/** init
  *
  *     Initializer for RemapFramesParser
  *
  * PARAMETERS:
  *     IN/OUT indicesP_ - the vector to store the rearranged indices;
  *                        must have size() > 0;
  *                        elements must be pre-initialized to their default
  *                          mappings
  *     IN max_          - the maximum value allowed for new indices
  *     IN tol_flag      - indicates if we tolerate out-of-range indices
  */
void RemapFramesParser::init(std::vector<MapIndex>* indicesP_, int max_) throw()
{
    assert(indicesP_ != NULL);

    indicesP = indicesP_;
    f_max = max_;
    lineP = NULL;

    pos.p = NULL;
    pos.line = 1;
    pos.col = 1;
}


/** RemapFramesParser constructor
  *
  * PARAMETERS:
  *     IN fileP_        - an open, readable file stream to the file to
  *                        parse
  *     IN/OUT indicesP_ - the vector to store the rearranged indices;
  *                        must have size() > 0;
  *                        elements must be pre-initialized to their default
  *                        mappings
  *     IN max_          - the maximum value allowed for new indices
  *     IN tol_flag      - indicates if we tolerate out-of-range indices
  */
RemapFramesParser::RemapFramesParser(FILE* fileP_, std::vector<MapIndex>* indicesP_, int max_, bool tol_flag)
: inputMode(MODE_FILE)
, _tol_flag (tol_flag)
{
    assert(fileP_ != NULL);

    init(indicesP_, max_);
    input.fileP = fileP_;
}


/** RemapFramesParser constructor
  *
  * PARAMETERS:
  *     IN mappingsP_    - the string to parse
  *     IN/OUT indicesP_ - the vector to store the rearranged indices;
  *                        must have size() > 0;
  *                        elements must be pre-initialized to their default
  *                        mappings
  *     IN max_          - the maximum value allowed for new indices
  */
RemapFramesParser::RemapFramesParser(const char* mappingsP_, std::vector<MapIndex>* indicesP_, int max_, bool tol_flag)
: inputMode(MODE_DIRECT)
, _tol_flag (tol_flag)
{
    assert(mappingsP_ != NULL);

    init(indicesP_, max_);
    input.mappingsP = mappingsP_;
}


/** getPos
  *
  *     Retrieves the current position (line and column numbers) within the
  *     input file.
  *
  * PARAMETERS:
  *     OUT lineP - on output, set to the current line number;
  *                 pass NULL if unwanted
  *     OUT colP  - on output, set to the current column number;
  *                 the current column number might not be accurate;
  *                 pass NULL if unwanted
  */
void RemapFramesParser::getPos(unsigned int* lineP, unsigned int* colP) const throw()
{
    if (lineP != NULL) { *lineP = pos.line; }
    if (colP != NULL) { *colP = pos.col; }
}


/** getLineNumber
  *
  * RETURNS:
  *     the current line number within the input file
  */
unsigned int RemapFramesParser::getLineNumber() const throw()
{
    return pos.line;
}


/** getColumn
  *
  * RETURNS:
  *     the current column number within the input file;
  *     might not be accurate
  */
unsigned int RemapFramesParser::getColumn() const throw()
{
    return pos.col;
}


/** setPos
  *
  *     Sets the values for the current line and column numbers.
  */
void RemapFramesParser::setPos() throw()
{
    // Doesn't account for tab characters properly (of course, it's not
    // possible to handle them accurately without knowing what the tab stops
    // are anyhow...).
    pos.col = (unsigned int) (pos.p - lineP + 1);
}


/** readLine
  *
  *     Reads the next line of input.
  *
  * RETURNS:
  *     true if a line was read;
  *     false otherwise
  *
  * SIDE EFFECTS:
  *     allocates and fills <lineP>;
  *     it is the caller's responsibility to free <lineP> with freeLine() when
  *       done
  *
  * THROWS:
  *     std::bad_alloc - insufficient memory
  */
bool RemapFramesParser::readLine() throw(std::bad_alloc)
{
    return (inputMode == MODE_FILE)
           ? ::getLine(input.fileP, &lineP)
           : // inputMode == MODE_DIRECT
             ::getLine(&input.mappingsP, &lineP);
}


/** freeLine
  *
  *     Frees the input line.
  */
void RemapFramesParser::freeLine() throw()
{
    ::freeLine(lineP);
    lineP = NULL;
}


/** skipWhitespace
  *
  *     Skips whitespace in the line.
  *
  * SIDE EFFECTS:
  *     advances <pos.p> to point to the next non-whitespace character or
  *       to the end of the string
  */
void RemapFramesParser::skipWhitespace() throw()
{
    while (*pos.p != '\0' && isspace(*pos.p))
    {
        ++pos.p;
    }
}


/** isLineEmpty
  *
  * RETURNS:
  *     true if there are no more tokens in the line;
  *     false otherwise
  */
bool RemapFramesParser::isLineEmpty() const throw()
{
    RemapFramesParser tempState = *this;
    tempState.skipWhitespace();
    return *tempState.pos.p == '\0';
}


/** matchComment
  *
  * RETURNS:
  *     true if there's a comment at the current location;
  *     false otherwise
  *
  * SIDE EFFECTS:
  *     if matched, advances <pos.p> to the end of the line
  */
bool RemapFramesParser::matchComment() throw()
{
    bool matched = false;

    skipWhitespace();
    setPos();

    if (matchChar('#'))
    {
        pos.p += strlen(pos.p);
        matched = true;
    }

    return matched;
}


/** matchChar
  *
  * PARAMETERS:
  *     c - the character to match
  *
  * RETURNS:
  *     true if the specified character matches the current token in the
  *       line;
  *     false otherwise
  *
  * SIDE EFFECTS:
  *     if matched, advances <pos.p> to point to the start of the next
  *       unparsed token
  */
bool RemapFramesParser::matchChar(char c) throw()
{
    bool matched = false;

    skipWhitespace();
    setPos();

    if (*pos.p == c)
    {
        ++pos.p;
        matched = true;
    }

    return matched;
}


/** matchInt
  *
  * PARAMETERS:
  *     OUT valP - on output, the found integer;
  *                if the current token is not an integer, left untouched
  *
  * RETURNS:
  *     true if an integer is the current token in the line;
  *     false otherwise
  *
  * SIDE EFFECTS:
  *     if matched, advances <pos.p> to point to the start of the next
  *       unparsed token
  *
  * THROWS:
  *     OverflowException - the magnitude of the integer is too large to
  *                           handle
  */
bool RemapFramesParser::matchInt(int* valP) throw(OverflowException)
{
    assert(valP != NULL);

    bool matched = false;

    skipWhitespace();
    setPos();

    {
        char* endP;
        int val = strtol(pos.p, &endP, 10);

        // check that we matched at least one digit
        // (strtol will return 0 if fed a string with no digits)
        if (endP > pos.p && isdigit(*(endP - 1)))
        {
            if (val == LONG_MIN || val == LONG_MAX)
            {
                throw OverflowException();
            }
            else
            {
                *valP = val;
                pos.p = endP;
                matched = true;
            }
        }
    }

    return matched;
}


/** matchRange
  *
  *     Matches an integer range.  An integer range appears as:
  *         [i j]
  *     in the input file.
  *
  * PARAMETERS:
  *     OUT rangeP - on output, the found integer range;
  *                  if the current token is not an integer range, left
  *                    untouched
  *
  * RETURNS:
  *     true if an integer range are the current tokens in the line;
  *     false otherwise
  *
  * SIDE EFFECTS:
  *     if matched, advances <pos.p> to point to the start of the next
  *       unparsed token
  */
bool RemapFramesParser::matchRange(range_t* rangeP) throw()
{
    assert(rangeP != NULL);

    bool matched = false;

    setPos();

    if (matchChar('['))
    {
        RemapFramesParser tempState = *this;
        range_t range;
        if (   tempState.matchInt(&range.start)
            && tempState.matchInt(&range.end)
            && tempState.matchChar(']'))
        {
            *rangeP = range;
            *this = tempState; // commit
            matched = true;
        }
    }

    return matched;
}


/** setFrame
  *
  *     Maps a single frame to another single frame.
  *
  * PARAMETERS:
  *     i - the old frame index
  *     j - the new frame index
  *
  * SIDE EFFECTS:
  *     mutates <indicesP>
  *
  * THROWS:
  *     BadValueException - <i> or <j> is out of bounds
  */
void RemapFramesParser::setFrame(int i, int j) throw(BadValueException)
{
    int n = int (indicesP->size());
    if (!(i >= 0 && i < n))
    {
        if (! _tol_flag)
        {
            throw BadValueException(i);
        }
    }
    else if (!(j >= 0 && j < f_max) && ! _tol_flag)
    {
        throw BadValueException(j);
    }
    else
    {
        j =   (j >= f_max) ? f_max - 1
            : (j <    0) ? 0
            :              j;
        MapIndex& element = (*indicesP)[i];
        element.clipIndex = 1;
        element.frame = j;
    }
}

void RemapFramesParser::appendFrame(int j) throw(BadValueException)
{
    const bool in_range_flag = (j >= 0 && j < f_max);
    if (! in_range_flag && (! _tol_flag || f_max == 0))
    {
        throw BadValueException(j);
    }
    else
    {
        j = (in_range_flag) ? j : (j >= f_max) ? f_max - 1 : 0;
        MapIndex element;
        element.clipIndex = 1;
        element.frame = j;
        indicesP->push_back(element);
    }
}


/** fillRange
  *
  *     Maps a range of frames to a single frame.
  *
  * PARAMETERS:
  *     IN rangeIn - the input range of frames
  *     j          - the output frame index
  *
  * SIDE EFFECTS:
  *     mutates <indicesP>
  *
  * THROWS:
  *     BadValueException - <rangeInP> or <j> is out of bounds
  */
void RemapFramesParser::fillRange(const range_t& rangeIn, int j) throw(BadValueException)
{
    int n = int (indicesP->size());
    if (!(rangeIn.start >= 0 && rangeIn.start < n) && ! _tol_flag)
    {
        throw BadValueException(rangeIn.start);
    }
    else if (!(rangeIn.end < n) && ! _tol_flag)
    {
        throw BadValueException(rangeIn.end);
    }
    else if (!(rangeIn.start <= rangeIn.end))
    {
        throw BadValueException(rangeIn.end);
    }
    else if (!(j >= 0 && j < f_max) && ! _tol_flag)
    {
        throw BadValueException(j);
    }
    else
    {
        int m = rangeIn.end - rangeIn.start + 1;
        if (_tol_flag)
        {
            j =   (j >= f_max) ? f_max - 1
                : (j <      0) ? 0
                :                j;
            for (int i = 0; i < m; i++)
            {
                const int ri = rangeIn.start + i;
                if (ri >= 0 && ri < n)
                {
                    MapIndex& element = (*indicesP)[ri];
                    element.clipIndex = 1;
                    element.frame = j;
                }
            }
        }
        else
        {
            for (int i = 0; i < m; i++)
            {
                MapIndex& element = (*indicesP)[rangeIn.start + i];
                element.clipIndex = 1;
                element.frame = j;
            }
        }
    }
}


/** setRange
  *
  *     Maps a range of frames to another range of frames.
  *
  *     If the input and output ranges are not of equal size, the output
  *     range will be interpolated across the input range.
  *
  * PARAMETERS:
  *     IN rangeIn  - the input range of frames
  *     IN rangeOut - the output range of frames
  *
  * SIDE EFFECTS:
  *     mutates <indicesP>
  *
  * THROWS:
  *     BadValueException - <rangeInP> or <rangeOutP> is out of bounds
  */
void RemapFramesParser::setRange(const range_t& rangeIn, const range_t& rangeOut) throw(BadValueException)
{
    int n = int (indicesP->size());
    if (!(rangeIn.start >= 0 && rangeIn.start < n) && ! _tol_flag)
    {
        throw BadValueException(rangeIn.start);
    }
    else if (!(rangeIn.end < n) && ! _tol_flag)
    {
        throw BadValueException(rangeIn.end);
    }
    else if (!(rangeIn.start <= rangeIn.end))
    {
        throw BadValueException(rangeIn.end);
    }
    else if (!(rangeOut.start >= 0 && rangeOut.start < f_max) && ! _tol_flag)
    {
        throw BadValueException(rangeOut.start);
    }
    else if (!(rangeOut.end >= 0 && rangeOut.end < f_max) && ! _tol_flag)
    {
        throw BadValueException(rangeOut.end);
    }
    else
    {
        int m = rangeIn.end - rangeIn.start + 1;
        double d = rangeOut.end - rangeOut.start;
        d += (d < 0) ? -1 : +1;

        assert(m != 0);

        d /= m;
        if (_tol_flag)
        {
            for (int i = 0; i < m; i++)
            {
                const int ri = rangeIn.start + i;
                if (ri >= 0 && ri < n)
                {
                    int rj = int(rangeOut.start + d * i);
                    rj =   (rj >= f_max) ? f_max - 1
                         : (rj <      0) ? 0
                         :                 rj;

                    MapIndex& element = (*indicesP)[ri];
                    element.clipIndex = 1;
                    element.frame = rj;
                }
            }
        }
        else
        {
            for (int i = 0; i < m; i++)
            {
                MapIndex& element = (*indicesP)[rangeIn.start + i];
                element.clipIndex = 1;
                element.frame = int(rangeOut.start + d * i);
            }
        }
    }
}


/** parse
  *
  *     Parses the input file.
  *
  * PARAMETERS:
  *     simple - pass true to use simple mode
  *
  * SIDE EFFECTS:
  *     mutates indicesP;
  *     sets <pos.p>, <pos.line>, and <pos.col>
  *
  * THROWS:
  *     MalformedException - parse error; the input is malformed
  *     OverflowException  - the magnitude of a parsed integer is too large
  *                            to handle
  *     BadValueException  - the value of a parsed integer is invalid for
  *                            its context
  */
void RemapFramesParser::parse() throw(std::bad_alloc, MalformedException, OverflowException, BadValueException)
{
    int i,
        j;
    range_t rangeIn,
            rangeOut;

    assert(lineP == NULL);

    while (readLine())
    {
        ON_BLOCK_EXIT_OBJ(*this, &RemapFramesParser::freeLine);

        pos.p = lineP;
        if (matchInt(&i))
        {
            if (!matchInt(&j))
            {
                throw MalformedException();
            }
            else
            {
                setFrame(i, j);
            }
        }
        else if (matchRange(&rangeIn))
        {
            if (matchInt(&j))
            {
                fillRange(rangeIn, j);
            }
            else if (matchRange(&rangeOut))
            {
                setRange(rangeIn, rangeOut);
            }
            else
            {
                throw MalformedException();
            }
        }

        (void) matchComment();

        if (!isLineEmpty()) { throw MalformedException(); }

        ++pos.line;
    }
}


// Returns the new number of frames.
int RemapFramesParser::parseSimple() throw(std::bad_alloc, MalformedException, OverflowException, BadValueException)
{
    int numFrames = 0,
        i;
    assert(lineP == NULL);

    while (readLine())
    {
        ON_BLOCK_EXIT_OBJ(*this, &RemapFramesParser::freeLine);

        pos.p = lineP;
        while (matchInt(&i))
        {
            appendFrame(i);
            ++numFrames;
        }

        (void) matchComment();

        if (!isLineEmpty()) { throw MalformedException(); }

        ++pos.line;
    }
    return numFrames;
}


void RemapFramesParser::parseReplaceSimple() throw(std::bad_alloc, MalformedException, OverflowException, BadValueException)
{
    int i;
    range_t range;
    assert(lineP == NULL);

    bool matched;

    while (readLine())
    {
        ON_BLOCK_EXIT_OBJ(*this, &RemapFramesParser::freeLine);

        pos.p = lineP;
        while (true)
        {
            matched = false;

            if (matchInt(&i))
            {
                setFrame(i, i);
                matched = true;
            }

            if (matchRange(&range))
            {
                setRange(range, range);
                matched = true;
            }

            if (!matched) { break; }
        }

        (void) matchComment();

        if (!isLineEmpty()) { throw MalformedException(); }

        ++pos.line;
    }
}



void RemapFramesParser::parseTransform (std::string &result, const Calc &calc, bool hopen_flag, bool discrete_flag) 
throw(std::bad_alloc, MalformedException, OverflowException, BadValueException)
{
    int i;
    range_t range;
    assert(lineP == NULL);

    bool matched;
    const char *delim_0 = "";

    result.clear ();

    while (readLine())
    {
        ON_BLOCK_EXIT_OBJ(*this, &RemapFramesParser::freeLine);

        pos.p = lineP;
        while (true)
        {
            matched = false;

            if (matchInt(&i))
            {
                result += delim_0;
                result += map_range (calc, hopen_flag, discrete_flag, i, i);
                matched = true;
            }
            else if (matchRange(&range))
            {
                if (range.start > range.end)
                {
                    throw BadValueException(range.end);
                }
                result += delim_0;
                result += map_range (calc, hopen_flag, discrete_flag, range.start, range.end);
                matched = true;
            }

            if (matched)
            {
                delim_0 = " ";
            }
            else
            {
                break;
            }
        }

        (void) matchComment();

        if (!isLineEmpty()) { throw MalformedException(); }

        delim_0 = "\n";

        ++pos.line;
    }
}



// Can return an empty string.
std::string	RemapFramesParser::map_range (const Calc &calc, bool hopen_flag, bool discrete_flag, int beg, int end)
{
    std::string     range_str;

    double          input [3] = { 0, 0, 0 };
    if (discrete_flag)
    {
        range_t         res_range = { INT_MAX, INT_MIN };
        for (int i = beg; i <= end; ++i)
        {
            input [0] = double (i);
            input [2] = input [0];
            const int       res = int (floor (calc.eval (input) + 0.5));
            if (res >= 0)
            {
                if (res == res_range.start - 1)
                {
                    res_range.start = res;
                }
                else if (res == res_range.end + 1)
                {
                    res_range.end = res;
                }
                else
                {
                    if (res_range.start <= res_range.end)
                    {
                        append_range (range_str, res_range.start, res_range.end);
                    }

                    res_range.start = res;
                    res_range.end   = res;
                }
            }
        }

        if (res_range.start <= res_range.end)
        {
            append_range (range_str, res_range.start, res_range.end);
        }
    }

    else
    {
        range_t         res_range;

        const int       offset = hopen_flag ? 1 : 0;
        input [0] = beg;
        input [1] = 0;
        input [2] = end + offset;
        res_range.start = int (floor (calc.eval (input) + 0.5));

        std::swap (input [0], input [2]);
        input [1] = 1;
        res_range.end = int (floor (calc.eval (input) + 0.5));

        if (res_range.end < res_range.start)
        {
            std::swap (res_range.start, res_range.end);
        }

        res_range.end -= offset;
        if (res_range.start <= res_range.end && res_range.end >= 0)
        {
            res_range.start = std::max (res_range.start, 0);
            append_range (range_str, res_range.start, res_range.end);
        }
    }

	return (range_str);
}



void    RemapFramesParser::append_range (std::string &range_str, int beg, int end)
{
    assert (&range_str != 0);
	assert (beg >= 0);
	assert (beg <= end);

    char            txt_0 [127+1];
    if (beg == end)
    {
        sprintf (txt_0, "%d", beg);
    }
    else
    {
        sprintf (txt_0, "[%d %d]", beg, end);
    }

    if (! range_str.empty ())
    {
        range_str += " ";
    }
	range_str += txt_0;
}



