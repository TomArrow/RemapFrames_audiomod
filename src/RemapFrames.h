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

#ifndef REMAPFRAMES_H
#define REMAPFRAMES_H

#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <avisynth.h>



// MACROS --------------------------------------------------------------

#define ARRAY_LENGTH(arr) (sizeof (arr) / sizeof *(arr))



// CLASS PROTOTYPES ----------------------------------------------------

struct MapIndex
{
    int clipIndex;
    int frame;
};


class RemapFrames : public GenericVideoFilter
{
public:
    typedef enum { MODE_SIMPLE, MODE_REPLACE_SIMPLE, MODE_ADVANCED } mode_t;

    virtual PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* envP);
    void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
    virtual bool __stdcall GetParity(int n);

    static AVSValue __cdecl Create(AVSValue args, void* userDataP, IScriptEnvironment* envP);
    static AVSValue __cdecl CreateSimple(AVSValue args, void* userDataP, IScriptEnvironment* envP);
    static AVSValue __cdecl CreateReplaceSimple(AVSValue args, void* userDataP, IScriptEnvironment* envP);
    static AVSValue __cdecl CreateTransform(AVSValue args, void* userDataP, IScriptEnvironment* envP);
    static AVSValue __cdecl CreateMerge(AVSValue args, void* userDataP, IScriptEnvironment* envP);

private:
    PClip sourceClip;

    // Stores the rearranged frame indices.
    std::vector<MapIndex> indices;

    static bool is_empty_string (const char *str_0);

    void initSimpleMode(const char* filenameP, const char* mappingsP, bool tol_flag, IScriptEnvironment* envP);
    void initReplaceSimpleMode(const char* filenameP, const char* mappingsP, bool tol_flag, IScriptEnvironment* envP);
    void initAdvancedMode(const char* filenameP, const char* mappingsP, bool tol_flag, IScriptEnvironment* envP);

    explicit RemapFrames(PClip child_, PClip sourceClip_, mode_t mode,
                         const char* filenameP, const char* mappingsP,
                         bool tol_flag, IScriptEnvironment* envP);
};


#endif // REMAPFRAMES_H
