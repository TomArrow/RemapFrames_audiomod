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

#ifndef REMAPFRAMESPARSER_H
#define REMAPFRAMESPARSER_H

#include <cassert>
#include <string>
#include <vector>
#include "RemapFrames.h"



// CLASS PROTOTYPES ----------------------------------------------------

class Calc;

class RemapFramesParser
{
public:
    class MalformedException { };
    class OverflowException { };
    class BadValueException
    {
    public:
        int val;
        BadValueException(int val_) : val(val_) { }
    };

    RemapFramesParser(FILE* fileP, std::vector<MapIndex>* indicesP, int max_, bool tol_flag);
    RemapFramesParser(const char* mappingsP, std::vector<MapIndex>* indicesP, int max_, bool tol_flag);

    void getPos(unsigned int* lineP, unsigned int* colP) const throw();
    unsigned int getLineNumber() const throw();
    unsigned int getColumn() const throw();

    void parse()
        throw(std::bad_alloc, MalformedException, OverflowException, BadValueException);
    int parseSimple()
        throw(std::bad_alloc, MalformedException, OverflowException, BadValueException);
    void parseReplaceSimple()
        throw(std::bad_alloc, MalformedException, OverflowException, BadValueException);
    void parseTransform(std::string &result, const Calc &calc, bool hopen_flag, bool discrete_flag)
        throw(std::bad_alloc, MalformedException, OverflowException, BadValueException);

private:
    typedef enum { MODE_FILE, MODE_DIRECT } mode_t;

    mode_t inputMode;
    union
    {
        // the file to parse
        FILE* fileP;

        // string that directly contains the frame index mappings
        const char* mappingsP;
    } input;

    typedef struct
    {
        int start;
        int end;
    } range_t;

    // stores the rearranged frame indices
    std::vector<MapIndex>* indicesP;

    int f_max;

    // points to the start of the current line
    char* lineP;

    // Indicates that we tolerate (and ignore) frames out of range
    bool _tol_flag;

    struct
    {
        // points to the current position within the line
        const char* p;

        // the current line and column
        // (the column might not be accurate)
        unsigned int line;
        unsigned int col;
    } pos;

    void init(std::vector<MapIndex>* indicesP_, int max_) throw();

    void setPos() throw();

    bool readLine() throw(std::bad_alloc);
    void freeLine() throw();

    void skipWhitespace() throw();
    bool isLineEmpty() const throw();

    bool matchComment() throw();
    bool matchChar(char c) throw();
    bool matchInt(int* valP) throw(OverflowException);
    bool matchRange(range_t* rangeP) throw();

    void setFrame(int i, int j) throw(BadValueException);
    void appendFrame(int j) throw(BadValueException);
    void fillRange(const range_t& rangeIn, int j) throw(BadValueException);
    void setRange(const range_t& rangeIn, const range_t& rangeOut) throw(BadValueException);

    static std::string map_range (const Calc &calc, bool hopen_flag, bool discrete_flag, int beg, int end);
    static void append_range (std::string &range_str, int beg, int end);
};


#endif // REMAPFRAMESPARSER_H
