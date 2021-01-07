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

#define NOMINMAX
#define NOGDI
#define WIN32_LEAN_AND_MEAN

#include <algorithm>

#include <cassert>
#include <cctype>
#include <cstdio>

#include <iostream>

#include <fstream>

#include "windows.h"
#include "avisynth.h"

#include "ScopeGuard.h"

#include "Calc.h"
#include "RemapFrames.h"
#include "RemapFramesParser.h"



// CLASS DEFINITIONS ---------------------------------------------------

bool RemapFrames::is_empty_string (const char *str_0)
{
    if (str_0 == 0)
    {
        return (true);
    }

    bool          empty_flag = true;
    while (*str_0 != '\0' && empty_flag)
    {
        empty_flag = (isspace (*str_0) != 0);
        ++ str_0;
    }

	return (empty_flag);
}



/** initSimpleMode
  *
  *     Initializer for RemapFramesSimple.
  *
  * PARAMETERS:
  *     IN filenameP - the name of the text file containing the frame
  *                      mappings;
  *                    may be NULL
  *     IN mappingsP - string containing additional frame mappings;
  *                    may be NULL
  *     IN/OUT envP  - pointer to the AviSynth scripting environment
  *
  * PRE:
  *     Either filenameP or mappingsP must be NULL, but not both.
  */
void RemapFrames::initSimpleMode(const char* filenameP, const char* mappingsP, const int audioBlendSamplesArg,
                                 bool tol_flag, IScriptEnvironment* envP)
{
    audioBlendSamples = audioBlendSamplesArg;
    if (filenameP == NULL && mappingsP == NULL)
    {
        if (tol_flag)
        {
            mappingsP = "";
        }
        else
        {
            envP->ThrowError("RemapFramesSimple: no filename nor mappings specified");
        }
    }
    else if (filenameP != NULL && mappingsP != NULL)
    {
        envP->ThrowError("RemapFramesSimple: filename and mappings cannot be used together");
    }

    try
    {
        if (mappingsP != NULL)
        {
            RemapFramesParser parser(mappingsP, &indices, sourceClip->GetVideoInfo().num_frames, tol_flag);
            try
            {
                vi.num_frames = parser.parseSimple();
            }
            catch (RemapFramesParser::MalformedException&)
            {
                envP->ThrowError("RemapFramesSimple: parse error in <mappings> string "
                                 "(line offset %u, column %u)",
                                 parser.getLineNumber(), parser.getColumn());
            }
            catch (RemapFramesParser::OverflowException&)
            {
                envP->ThrowError("RemapFramesSimple: integer overflow in <mappings> string "
                                 "(line offset %u, column %u)",
                                 parser.getLineNumber(), parser.getColumn());
            }
            catch (RemapFramesParser::BadValueException& e)
            {
                envP->ThrowError("RemapFramesSimple: value out of bounds in <mappings> string: %d "
                                 "(line offset %u)",
                                 e.val, parser.getLineNumber());
            }
        }
        else if (filenameP != NULL)
        {
            FILE* fileP = fopen(filenameP, "r");
            ON_BLOCK_EXIT(fclose, fileP);

            if (fileP == NULL)
            {
                envP->ThrowError("RemapFramesSimple: error opening file \"%s\"", filenameP);
            }
            else
            {
                RemapFramesParser parser(fileP, &indices, sourceClip->GetVideoInfo().num_frames, tol_flag);
                try
                {
                    vi.num_frames = parser.parseSimple();
                }
                catch (RemapFramesParser::MalformedException&)
                {
                    envP->ThrowError("RemapFramesSimple: parse error "
                                     "(%s, line %u, column %u)",
                                     filenameP, parser.getLineNumber(), parser.getColumn());
                }
                catch (RemapFramesParser::OverflowException&)
                {
                    envP->ThrowError("RemapFramesSimple: integer overflow "
                                     "(%s, line %u, column %u)",
                                     filenameP, parser.getLineNumber(), parser.getColumn());
                }
                catch (RemapFramesParser::BadValueException& e)
                {
                    envP->ThrowError("RemapFramesSimple: value out of bounds: %d "
                                     "(%s, line %u)",
                                     e.val, filenameP, parser.getLineNumber());
                }
            }
        }
    }
    catch (std::bad_alloc&)
    {
        envP->ThrowError("RemapFramesSimple: insufficient memory");
    }
}


/** initReplaceSimpleMode
  *
  *     Initializer for RemapFrames.
  *
  * PARAMETERS:
  *     IN filenameP - the name of the text file containing the frame
  *                      mappings;
  *                    may be NULL
  *     IN mappingsP - string containing additional frame mappings;
  *                    may be NULL
  *     IN/OUT envP  - pointer to the AviSynth scripting environment
  *
  * PRE:
  *     At least one of filenameP or mappingsP must not be NULL.
  */
/*
void RemapFrames::initReplaceSimpleMode(const char* filenameP, const char* mappingsP,
                                        bool tol_flag, IScriptEnvironment* envP)
{
    if (filenameP == NULL && mappingsP == NULL)
    {
        if (tol_flag)
        {
            mappingsP = "";
        }
        else
        {
            envP->ThrowError("ReplaceFramesSimple: no filename nor mappings specified");
        }
    }

    try
    {
        // Initialize the values in indices.
        // Each frame by default gets mapped to itself.
        indices.resize(vi.num_frames);
        for (int i = 0; i < vi.num_frames; i++)
        {
            indices[i].clipIndex = 0;
            indices[i].frame = i;
        }

        if (filenameP != NULL)
        {
            FILE* fileP = fopen(filenameP, "r");
            ON_BLOCK_EXIT(fclose, fileP);

            if (fileP == NULL)
            {
                envP->ThrowError("ReplaceFramesSimple: error opening file \"%s\"", filenameP);
            }
            else
            {
                RemapFramesParser parser(fileP, &indices, sourceClip->GetVideoInfo().num_frames, tol_flag);
                try
                {
                    parser.parseReplaceSimple();
                }
                catch (RemapFramesParser::MalformedException&)
                {
                    envP->ThrowError("ReplaceFramesSimple: parse error "
                                     "(%s, line %u, column %u)",
                                     filenameP, parser.getLineNumber(), parser.getColumn());
                }
                catch (RemapFramesParser::OverflowException&)
                {
                    envP->ThrowError("ReplaceFramesSimple: integer overflow "
                                     "(%s, line %u, column %u)",
                                     filenameP, parser.getLineNumber(), parser.getColumn());
                }
                catch (RemapFramesParser::BadValueException& e)
                {
                    envP->ThrowError("ReplaceFramesSimple: value out of bounds: %d "
                                     "(%s, line %u)",
                                     e.val, filenameP, parser.getLineNumber());
                }
            }
        }

        if (mappingsP != NULL)
        {
            RemapFramesParser parser(mappingsP, &indices, sourceClip->GetVideoInfo().num_frames, tol_flag);
            try
            {
                parser.parseReplaceSimple();
            }
            catch (RemapFramesParser::MalformedException&)
            {
                envP->ThrowError("ReplaceFramesSimple: parse error in <mappings> string "
                                 "(line offset %u, column %u)",
                                 parser.getLineNumber(), parser.getColumn());
            }
            catch (RemapFramesParser::OverflowException&)
            {
                envP->ThrowError("ReplaceFramesSimple: integer overflow in <mappings> string "
                                 "(line offset %u, column %u)",
                                 parser.getLineNumber(), parser.getColumn());
            }
            catch (RemapFramesParser::BadValueException& e)
            {
                envP->ThrowError("ReplaceFramesSimple: value out of bounds in <mappings> string: %d "
                                 "(line offset %u)",
                                 e.val, parser.getLineNumber());
            }
        }
    }
    catch (std::bad_alloc&)
    {
        envP->ThrowError("ReplaceFramesSimple: insufficient memory");
    }
}
*/

/** initAdvancedMode
  *
  *     Initializer for RemapFrames.
  *
  * PARAMETERS:
  *     IN filenameP - the name of the text file containing the frame
  *                      mappings;
  *                    may be NULL
  *     IN mappingsP - string containing additional frame mappings;
  *                    may be NULL
  *     IN/OUT envP  - pointer to the AviSynth scripting environment
  *
  * PRE:
  *     At least one of filenameP or mappingsP must not be NULL.
  *//*
void RemapFrames::initAdvancedMode(const char* filenameP, const char* mappingsP,
                                   bool tol_flag, IScriptEnvironment* envP)
{
    if (filenameP == NULL && mappingsP == NULL)
    {
        if (tol_flag)
        {
            mappingsP = "";
        }
        else
        {
            envP->ThrowError("RemapFrames: no filename nor mappings specified");
        }
    }

    try
    {
        // Initialize the values in indices.
        // Each frame by default gets mapped to itself.
        indices.resize(vi.num_frames);
        for (int i = 0; i < vi.num_frames; i++)
        {
            indices[i].clipIndex = 0;
            indices[i].frame = i;
        }

        if (filenameP != NULL)
        {
            FILE* fileP = fopen(filenameP, "r");
            ON_BLOCK_EXIT(fclose, fileP);

            if (fileP == NULL)
            {
                envP->ThrowError("RemapFrames: error opening file \"%s\"", filenameP);
            }
            else
            {
                RemapFramesParser parser(fileP, &indices, sourceClip->GetVideoInfo().num_frames, tol_flag);
                try
                {
                    parser.parse();
                }
                catch (RemapFramesParser::MalformedException&)
                {
                    envP->ThrowError("RemapFrames: parse error "
                                     "(%s, line %u, column %u)",
                                     filenameP, parser.getLineNumber(), parser.getColumn());
                }
                catch (RemapFramesParser::OverflowException&)
                {
                    envP->ThrowError("RemapFrames: integer overflow "
                                     "(%s, line %u, column %u)",
                                     filenameP, parser.getLineNumber(), parser.getColumn());
                }
                catch (RemapFramesParser::BadValueException& e)
                {
                    envP->ThrowError("RemapFrames: value out of bounds: %d "
                                     "(%s, line %u)",
                                     e.val, filenameP, parser.getLineNumber());
                }
            }
        }

        if (mappingsP != NULL)
        {
            RemapFramesParser parser(mappingsP, &indices, sourceClip->GetVideoInfo().num_frames, tol_flag);
            try
            {
                parser.parse();
            }
            catch (RemapFramesParser::MalformedException&)
            {
                envP->ThrowError("RemapFrames: parse error in <mappings> string "
                                 "(line offset %u, column %u)",
                                 parser.getLineNumber(), parser.getColumn());
            }
            catch (RemapFramesParser::OverflowException&)
            {
                envP->ThrowError("RemapFrames: integer overflow in <mappings> string "
                                 "(line offset %u, column %u)",
                                 parser.getLineNumber(), parser.getColumn());
            }
            catch (RemapFramesParser::BadValueException& e)
            {
                envP->ThrowError("RemapFrames: value out of bounds in <mappings> string: %d "
                                 "(line offset %u)",
                                 e.val, parser.getLineNumber());
            }
        }
    }
    catch (std::bad_alloc&)
    {
        envP->ThrowError("RemapFrames: insufficient memory");
    }
}
*/

/** RemapFrames constructor
  *
  * PARAMETERS:
  *     IN child_      - pointer to the base clip;
  *                      must have > 0 frames
  *     IN sourceClip_ - pointer to the source clip;
  *                      must have > 0 frames
  *     IN filenameP   - the name of the text file containing the frame
  *                        mappings;
  *                      may be NULL
  *     IN mappingsP   - string containing additional frame mappings;
  *                      may be NULL
  *     IN/OUT envP    - pointer to the AviSynth scripting environment
  */
RemapFrames::RemapFrames(PClip child_, PClip sourceClip_, mode_t mode,
                         const char* filenameP, const char* mappingsP, const int audioBlendSamplesArg,
                         bool tol_flag, IScriptEnvironment* envP)
: GenericVideoFilter(child_),
  sourceClip(sourceClip_),
  indices()
{
    assert(vi.num_frames > 0);
    assert(sourceClip->GetVideoInfo().num_frames > 0);


    switch (mode)
    {
        case MODE_SIMPLE:
            initSimpleMode(filenameP, mappingsP, audioBlendSamplesArg, tol_flag, envP);
            break;
/*
        case MODE_REPLACE_SIMPLE:
            initReplaceSimpleMode(filenameP, mappingsP, tol_flag, envP);
            break;

        case MODE_ADVANCED:
            initAdvancedMode(filenameP, mappingsP, tol_flag, envP);
            break;
            */
        default:
            assert(false);
    }
}


/** GetFrame
  *
  * PARAMETERS:
  *     n           - the index of the frame to retrieve
  *     IN/OUT envP - pointer to the AviSynth scripting environment
  *
  * RETURNS:
  *     the specified video frame
  */
PVideoFrame __stdcall RemapFrames::GetFrame(int n, IScriptEnvironment* envP)
{
    const int     n_c = std::min (std::max (n, 0), int (indices.size () - 1));
    MapIndex& element = indices [n_c];
    return ((element.clipIndex == 0)
            ? child
            : sourceClip)->GetFrame(element.frame, envP);
}


struct RemapFrames::remappedAudioSample {
    long long audioSample;
    long long realFrame;
    bool backwards;
};

void __stdcall RemapFrames::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
    
    int channels = vi.AudioChannels();

    VideoInfo videoInfo = child->GetVideoInfo();
    long double videoFramerate = (long double)videoInfo.fps_numerator / (long double)videoInfo.fps_denominator;
    long double audioSampleRate = videoInfo.audio_samples_per_second;

    // get video frame range
    long double seconds, frame;
    int whichFrame,frameNext,framePrevious; 
    bool frameRunBackwards;
    long double frameOffset;
    long double samplesPerFrame = audioSampleRate / videoFramerate;
    long double invertedFrameOffset;
    __int64 sampleToGet = 0;
    long double actualFrameToGet;

    
    /*std::ofstream myfile;
    myfile.open("blah.txt", std::ios::out | std::ios::app);
    myfile <<  videoFramerate << "\n";
    myfile << audioSampleRate << "\n";
    myfile << audioBlendSamples << "\n";
    myfile.close();*/
    
    // Audio sample related variables
    long double framePlace;
    long double roundedFramePlace;
    long double distanceFromFrameBoundary;
    long double mainSampleIntensity;
    long double foreignSampleIntensity;
    long double mixSamplePosition;
    remappedAudioSample nextFrameSample, lastFrameSample, mainSample;

    SFLOAT* singleSampleBuffer = new SFLOAT[vi.AudioChannels()];
    SFLOAT* singleMixSampleBuffer = new SFLOAT[vi.AudioChannels()];

    __int64 absolutePlace;
    if (vi.SampleType() == SAMPLE_FLOAT) {
        SFLOAT* samples = (SFLOAT*)buf;
        for (int i = 0; i < count; i++) {
            
            // "Straightforward" just get the sample we want
            if (audioBlendSamples == 0) {

                // Calculate correct source sample
                absolutePlace = start + i;
                seconds = absolutePlace / audioSampleRate;
                frame = seconds * videoFramerate;
                whichFrame = std::min(std::max((int)frame, 0), int(indices.size() - 1));
                frameNext = std::min(std::max(whichFrame+1, 0), int(indices.size() - 1));
                framePrevious = std::min(std::max(whichFrame -1, 0), int(indices.size() - 1));

                // Determine if audio should run backwards.
                frameRunBackwards = indices[frameNext].frame < indices[whichFrame].frame && indices[framePrevious].frame > indices[whichFrame].frame;

                // Determine proper sample
                frameOffset = frame - (long double)whichFrame;
                invertedFrameOffset = 1.0 - frameOffset;
            
                actualFrameToGet = (frameRunBackwards ? (long double)indices[whichFrame].frame + invertedFrameOffset : (long double)indices[whichFrame].frame + frameOffset);
                sampleToGet = std::min(vi.num_audio_samples, std::max((__int64)0, (__int64)(0.5+actualFrameToGet/videoFramerate*audioSampleRate)));
                child->GetAudio(singleSampleBuffer, sampleToGet, 1, env);
            
                /*works: sampleToGet = start + (__int64)i;
            
                child->GetAudio(singleSampleBuffer, sampleToGet, 1, env);*/
            
                for (int j = 0; j < channels; j++) {
                    samples[i * channels + j] = singleSampleBuffer[j];
                }
            }
            // Or do blending ...
            else {
                // Calculate correct source sample
                absolutePlace = start + i;
                
                framePlace = ((long double)absolutePlace / samplesPerFrame);
                roundedFramePlace = round((long double)absolutePlace / samplesPerFrame);
                distanceFromFrameBoundary = abs( framePlace - roundedFramePlace) * samplesPerFrame;
                
                
                mainSample = remapAudioSample(absolutePlace, audioSampleRate, videoFramerate);
                
                if (distanceFromFrameBoundary > audioBlendSamples || roundedFramePlace == 0) {
                    // All good. No blending needed.
                    sampleToGet = mainSample.audioSample;
                    child->GetAudio(singleSampleBuffer, sampleToGet, 1, env);

                }
                else {
                    mainSampleIntensity = (long double)0.5 + ((long double)0.5 *  (distanceFromFrameBoundary / (long double)audioBlendSamples)); // 0.5 because we only blend half way, the other half is blended in the other frame.
                    foreignSampleIntensity = 1 - mainSampleIntensity;
                    if (roundedFramePlace > framePlace) {
                        nextFrameSample = remapAudioSample(absolutePlace + samplesPerFrame, audioSampleRate, videoFramerate);
                        mixSamplePosition = nextFrameSample.audioSample - (nextFrameSample.backwards ? -samplesPerFrame : samplesPerFrame);
                    }
                    else {
                        lastFrameSample = remapAudioSample(absolutePlace - samplesPerFrame, audioSampleRate, videoFramerate);
                        mixSamplePosition = lastFrameSample.audioSample + (lastFrameSample.backwards ? -samplesPerFrame : samplesPerFrame);
                    }
                    child->GetAudio(singleSampleBuffer, mainSample.audioSample, 1, env);

                    // No need to blend the same sample with itself, that's ridiculous.
                    if(mainSample.audioSample != mixSamplePosition){

                        child->GetAudio(singleMixSampleBuffer, mixSamplePosition, 1, env);
                        /*for (int j = 0; j < channels; j++) {
                            singleSampleBuffer[j] = mainSampleIntensity *singleSampleBuffer[j] + foreignSampleIntensity*singleMixSampleBuffer[j];
                        }*/
                        for (int j = 0; j < channels; j++) {
                            singleSampleBuffer[j] = sqrt(mainSampleIntensity) *singleSampleBuffer[j] + sqrt(foreignSampleIntensity)*singleMixSampleBuffer[j];
                        }
                    }
                    /*
                    std::ofstream myfile;
                    myfile.open("blah.txt", std::ios::out | std::ios::app);
                    myfile << distanceFromFrameBoundary << "\n";
                    myfile << mainSampleIntensity << "\n";
                    myfile << foreignSampleIntensity << "\n";
                    myfile << mainSample.realFrame << "\n";
                    if (roundedFramePlace > framePlace) {

                        myfile << "next " << nextFrameSample.realFrame << "\n";
                        myfile << "next is backwards " << nextFrameSample.backwards << "\n";
                        myfile << "here " << mainSample.audioSample << " next " << nextFrameSample.audioSample << " mixsampleposition " <<  mixSamplePosition << "\n";
                    }
                    else {

                        myfile << "last " << lastFrameSample.realFrame << "\n";
                        myfile << "last is backwards " << lastFrameSample.backwards << "\n";
                        myfile << "here " << mainSample.audioSample << " last " << nextFrameSample.audioSample << " mixsampleposition " << mixSamplePosition << "\n";
                    }
                    myfile << "\n\n";
                    myfile.close();*/
                }

                /*works: sampleToGet = start + (__int64)i;

                child->GetAudio(singleSampleBuffer, sampleToGet, 1, env);*/

                for (int j = 0; j < channels; j++) {
                    samples[i * channels + j] = singleSampleBuffer[j];
                }
            }
        }
    }
}


inline RemapFrames::remappedAudioSample RemapFrames::remapAudioSample(long long originalAudioSample, long double audioSampleRate, long double videoFramerate) {
    
    long double seconds, frame;
    int whichFrame, frameNext, framePrevious;
    bool frameRunBackwards;
    long double frameOffset;
    long double samplesPerFrame = audioSampleRate / videoFramerate;
    long double invertedFrameOffset;
    __int64 sampleToGet = 0;
    long double actualFrameToGet;

    seconds = originalAudioSample / audioSampleRate;
    frame = seconds * videoFramerate;
    whichFrame = std::min(std::max((int)frame, 0), int(indices.size() - 1));
    frameNext = std::min(std::max(whichFrame + 1, 0), int(indices.size() - 1));
    framePrevious = std::min(std::max(whichFrame - 1, 0), int(indices.size() - 1));

    // Determine if audio should run backwards.
    frameRunBackwards = indices[frameNext].frame < indices[whichFrame].frame&& indices[framePrevious].frame > indices[whichFrame].frame;

    // Determine proper sample
    frameOffset = frame - (long double)whichFrame;
    invertedFrameOffset = 1.0 - frameOffset;

    actualFrameToGet = (frameRunBackwards ? (long double)indices[whichFrame].frame + invertedFrameOffset : (long double)indices[whichFrame].frame + frameOffset);
    sampleToGet = std::min(vi.num_audio_samples, std::max((__int64)0, (__int64)(0.5 + actualFrameToGet / videoFramerate * audioSampleRate)));
    remappedAudioSample returnValue;
    returnValue.audioSample = sampleToGet;
    returnValue.backwards = frameRunBackwards;
    returnValue.realFrame = whichFrame;
    return returnValue;
}

/** GetParity
  *
  * PARAMETERS:
  *     n - the index of the frame
  *
  * RETURNS:
  *     the parity of the specified video frame
  */
bool __stdcall RemapFrames::GetParity(int n)
{
    const int     n_c = std::min (std::max (n, 0), int (indices.size () - 1));
    MapIndex& element = indices [n_c];
    return ((element.clipIndex == 0)
            ? child
            : sourceClip)->GetParity(element.frame);
}


/** Create
  *
  *     Creates a new instance of this filter.
  *
  *     Callback passed to the scripting environment.
  *
  * PARAMETERS:
  *     args             - the array of arguments passed to this filter
  *                          from the script;
  *                        see filter documentation for details
  *     IN/OUT userDataP - (unused)
  *     IN/OUT envP      - pointer to the AviSynth scripting environment
  *
  * RETURNS:
  *     the new video clip
  */
/*
AVSValue __cdecl RemapFrames::Create(AVSValue args, void* userDataP, IScriptEnvironment* envP)
{
    const int       i_f = (userDataP == 0) ? 1 : 2;
    const int       i_m = (userDataP == 0) ? 2 : 1;

    //const PClip& clip = args[0].AsClip();
    AVSValue CA_args[3] = { args[0], SAMPLE_FLOAT, SAMPLE_FLOAT };
    const PClip& clip = envP->Invoke("ConvertAudio", AVSValue(CA_args, 3)).AsClip();

    const char* filenameP = args[i_f].Defined () ? args[i_f].AsString() : 0;
    const char* mappingsP = args[i_m].Defined () ? args[i_m].AsString() : 0;
    const PClip& sourceClip = args[3].Defined()
                              ? args[3].AsClip()
                              : clip;


    if (   clip->GetVideoInfo().num_frames == 0
        || sourceClip->GetVideoInfo().num_frames == 0
        || (userDataP != 0 && is_empty_string (filenameP) && is_empty_string (mappingsP)))
    {
        // nothing to do; return the base clip unaltered
        return (args [0]);
    }

    return (AVSValue (new RemapFrames (
        clip, sourceClip, MODE_ADVANCED, filenameP, mappingsP, (userDataP != 0), envP
    )));
}*/


AVSValue __cdecl RemapFrames::CreateSimple(AVSValue args, void* userDataP, IScriptEnvironment* envP)
{
    const int       i_f = (userDataP == 0) ? 1 : 2;
    const int       i_m = (userDataP == 0) ? 2 : 1;

    //const PClip& clip = args[0].AsClip();
    AVSValue CA_args[3] = { args[0], SAMPLE_FLOAT, SAMPLE_FLOAT };
    const PClip& clip = envP->Invoke("ConvertAudio", AVSValue(CA_args, 3)).AsClip();


    const char* filenameP = args[i_f].Defined () ? args[i_f].AsString() : 0;
    const char* mappingsP = args[i_m].Defined () ? args[i_m].AsString() : 0;
    const int audioBlendSamplesArg = args[3].Defined () ? args[3].AsInt() : 0;



    /*std::ofstream myfile;
    myfile.open("blah.txt", std::ios::out | std::ios::app);
    myfile << args[3].Defined() << "\n";
    myfile << audioBlendSamplesArg << "\n";
    myfile.close();*/

    if (   clip->GetVideoInfo().num_frames == 0
        || (userDataP != 0 && is_empty_string (filenameP) && is_empty_string (mappingsP)))
    {
        // nothing to do; return the base clip unaltered
        return (args [0]);
    }

    return (AVSValue (new RemapFrames (
        clip, clip, MODE_SIMPLE, filenameP, mappingsP, audioBlendSamplesArg, (userDataP != 0), envP
    )));
}

/*
AVSValue __cdecl RemapFrames::CreateReplaceSimple(AVSValue args, void* userDataP, IScriptEnvironment* envP)
{
    const int       i_f = (userDataP == 0) ? 2 : 3;
    const int       i_m = (userDataP == 0) ? 3 : 2;
    const PClip& clip = args[0].AsClip();
    const PClip& sourceClip = args[1].AsClip();
    const char* filenameP = args[i_f].Defined () ? args[i_f].AsString() : 0;
    const char* mappingsP = args[i_m].Defined () ? args[i_m].AsString() : 0;

    if (   clip->GetVideoInfo().num_frames == 0
        || sourceClip->GetVideoInfo().num_frames == 0
        || (userDataP != 0 && is_empty_string (filenameP) && is_empty_string (mappingsP)))
    {
        // nothing to do; return the base clip unaltered
        return (args [0]);
    }

    return (AVSValue (new RemapFrames (
        clip, sourceClip, MODE_REPLACE_SIMPLE, filenameP, mappingsP, (userDataP != 0), envP
    )));
}



AVSValue __cdecl RemapFrames::CreateTransform (AVSValue args, void* userDataP, IScriptEnvironment* envP)
{
    const char *    range_list_0  = args [0].Defined () ? args [0].AsString () : 0;
    const char *    op_0          = args [1].AsString ("x");
    const bool      hopen_flag    = args [2].AsBool (true);
    const bool      discrete_flag = args [3].AsBool (false);

    if (range_list_0 == 0)
    {
        return (args [0]);
    }

    Calc            calc;
    int             ret_val = calc.parse (op_0, "xry");
    if (ret_val != 0)
    {
        envP->ThrowError ("rfs_transform: parse error in <op> string.");
    }

    std::vector <MapIndex> indices;
    RemapFramesParser parser(range_list_0, &indices, 999, true);

    std::string     result;
    try
    {
        parser.parseTransform (result, calc, hopen_flag, discrete_flag);
    }
    catch (RemapFramesParser::MalformedException&)
    {
        envP->ThrowError("rfs_transform: parse error in <mappings> string "
                         "(line offset %u, column %u)",
                         parser.getLineNumber(), parser.getColumn());
    }
    catch (RemapFramesParser::OverflowException&)
    {
        envP->ThrowError("rfs_transform: integer overflow in <mappings> string "
                         "(line offset %u, column %u)",
                         parser.getLineNumber(), parser.getColumn());
    }
    catch (RemapFramesParser::BadValueException& e)
    {
        envP->ThrowError("rfs_transform: value out of bounds in <mappings> string: %d "
                         "(line offset %u)",
                         e.val, parser.getLineNumber());
    }

    return (envP->SaveString (result.c_str (), int (result.length ())));
}



AVSValue __cdecl RemapFrames::CreateMerge (AVSValue args, void* userDataP, IScriptEnvironment* envP)
{
    bool            def_flag = false;
    std::string     result;
    const int       nbr_str = args.ArraySize ();

    for (int str_cnt = 0; str_cnt < nbr_str; ++str_cnt)
    {
        if (args [str_cnt].Defined ())
        {
            if (def_flag)
            {
                result += " ";
            }
            result += args [str_cnt].AsString ();
            def_flag = true;
        }
    }

    if (! def_flag)
    {
        return (AVSValue ());
    }

    return (envP->SaveString (result.c_str (), int (result.length ())));
}

*/

/** AvisynthPluginInit2
  *
  *     AviSynth 2.5x plug-in entry-point.
  *
  * PARAMETERS:
  *     IN/OUT envP - pointer to the AviSynth scripting environment
  *
  * RETURNS:
  *     the version string for this plug-in
  *
  * SIDE EFFECTS:
  *     adds functions to the AviSynth scripting environment
  */
const AVS_Linkage* AVS_linkage = nullptr;

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment* envP, const AVS_Linkage* const vectors)
{
    AVS_linkage = vectors;
    //envP->AddFunction("RemapFrames", "c[filename]s[mappings]s[sourceClip]c", RemapFrames::Create, NULL);
    envP->AddFunction("RemapFramesSimple_AudioMod", "c[filename]s[mappings]s[audioBlendSamples]i", RemapFrames::CreateSimple, NULL);
    //envP->AddFunction("ReplaceFramesSimple", "cc[filename]s[mappings]s", RemapFrames::CreateReplaceSimple, NULL);

    //envP->AddFunction("remf", "c[mappings]s[filename]s[sourceClip]c", RemapFrames::Create, (void *)1);
    //envP->AddFunction("remfs", "c[mappings]s[filename]s", RemapFrames::CreateSimple, (void *)1);
    //envP->AddFunction("rfs", "cc[mappings]s[filename]s", RemapFrames::CreateReplaceSimple, (void *)1);

    //envP->AddFunction("rfs_transform", "[mappings]s[op]s[open]b[discrete]b", RemapFrames::CreateTransform, 0);
    //envP->AddFunction("rfs_merge", "[s0]s[s1]s[s2]s[s3]s[s4]s[s5]s[s6]s[s7]s[s8]s[s9]s", RemapFrames::CreateMerge, 0);

    return "RemapFrames v0.4.1 (Audiomod) [" __DATE__ "]\nCopyright (c) 2005 James D. Lin";
}
