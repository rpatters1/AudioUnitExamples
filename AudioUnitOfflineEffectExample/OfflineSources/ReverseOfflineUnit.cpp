/*
Copyright (C) 2016 Apple Inc. All Rights Reserved.
See LICENSE.txt for this sample’s licensing information

Abstract:
Offline AU
*/

/*
	An effect unit will work on the Float32 audio unit sample format
	Its input and output formats are equivalent - it does NO transformation of
	the format of its input to its output.
	
	It assumes that there will only ever be one input bus and one output bus.
*/

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	ReverseOfflineUnit.cpp
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "AUBase.h"
#include "ReverseOfflineUnitVersion.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark ____ReverseOfflineUnit

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// THIS UNIT HAS NO PARAMETERS...
// 

class ReverseOfflineUnit : public AUBase
{
public:
								ReverseOfflineUnit(AudioUnit component);
	
	virtual OSStatus			GetPropertyInfo(	AudioUnitPropertyID		inID,
													AudioUnitScope			inScope,
													AudioUnitElement		inElement,
													UInt32 &				outDataSize,
													Boolean	&				outWritable );

	virtual OSStatus			GetProperty(		AudioUnitPropertyID 	inID,
													AudioUnitScope 			inScope,
													AudioUnitElement 		inElement,
													void 					* outData );

	virtual OSStatus			SetProperty(		AudioUnitPropertyID 	inID,
													AudioUnitScope 			inScope,
													AudioUnitElement 		inElement,
													const void *			inData,
													UInt32 					inDataSize);

		// same logic as AUEffectBase
	virtual OSStatus 	Initialize();
														
	virtual bool				StreamFormatWritable(	AudioUnitScope		scope,
														AudioUnitElement	element);

	virtual OSStatus 	Render(	AudioUnitRenderActionFlags 			& ioActionFlags,
											const AudioTimeStamp 			& inTimeStamp,
											UInt32							nFrames);

		// in this case we don't have a preflight string
		// outStr can be NULL
		// if returning a string, this has to be a fresh copy that will be released
		// by the host after its done with it.
	virtual bool				GetPreflightString(CFStringRef *outStr) const { return false; }

	virtual OSStatus		Version() { return kReverseOfflineUnitVersion; }

	virtual bool			CanScheduleParameters() const { return false; }
	
private:
	UInt64			mNumInputSamples;
	UInt64			mStartOffset;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

AUDIOCOMPONENT_ENTRY(AUBaseFactory, ReverseOfflineUnit)

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	ReverseOfflineUnit::ReverseOfflineUnit
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ReverseOfflineUnit::ReverseOfflineUnit(AudioUnit component)
	: AUBase(component, 1, 1), 
	  mNumInputSamples(0),
	  mStartOffset (0)
{
}


#pragma mark ____ReverseOfflineProperties
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	ReverseOfflineUnit::GetPropertyInfo
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus			ReverseOfflineUnit::GetPropertyInfo (AudioUnitPropertyID	inID,
												AudioUnitScope					inScope,
												AudioUnitElement				inElement,
												UInt32 &						outDataSize,
												Boolean &						outWritable)
{
	if (inScope == kAudioUnitScope_Global) {
		switch (inID) {
			case kAudioUnitOfflineProperty_InputSize:
				outDataSize = sizeof(mNumInputSamples);
				outWritable = true;
				return noErr;
			case kAudioUnitOfflineProperty_OutputSize:
				outDataSize = sizeof(mNumInputSamples); // in this case, this is the same
				outWritable = false;
				return noErr;
			case kAudioUnitOfflineProperty_StartOffset:
				outDataSize = sizeof(mStartOffset);
				outWritable = false;
				return noErr;
			case kAudioUnitOfflineProperty_PreflightRequirements:
				outDataSize = sizeof (UInt32);
				outWritable = false;
				return noErr;
			case kAudioUnitOfflineProperty_PreflightName:
				if (GetPreflightString (NULL)) {
					outDataSize = sizeof (CFStringRef);
					outWritable = false;
					return noErr;
				}
				return kAudioUnitErr_InvalidProperty;
		}
	}
	return AUBase::GetPropertyInfo (inID, inScope, inElement, outDataSize, outWritable);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	ReverseOfflineUnit::GetProperty
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus			ReverseOfflineUnit::GetProperty (AudioUnitPropertyID 		inID,
												AudioUnitScope 					inScope,
												AudioUnitElement			 	inElement,
												void *							outData)
{
	if (inScope == kAudioUnitScope_Global) {
		switch (inID) {
			case kAudioUnitOfflineProperty_OutputSize:
				*(UInt64*)outData = (mNumInputSamples - mStartOffset);
				return noErr;
			case kAudioUnitOfflineProperty_InputSize:
				*(UInt64*)outData = mNumInputSamples;
				return noErr;
			case kAudioUnitOfflineProperty_StartOffset:
				*(UInt64*)outData = mStartOffset;
				return noErr;
			case kAudioUnitOfflineProperty_PreflightRequirements:
				*(UInt32*)outData = kOfflinePreflight_NotRequired;
				return noErr;

			case kAudioUnitOfflineProperty_PreflightName:
				if (GetPreflightString (NULL)) {
					GetPreflightString ((CFStringRef*)outData);
					return noErr;
				}
				return kAudioUnitErr_InvalidProperty;
		}
	}
	return AUBase::GetProperty (inID, inScope, inElement, outData);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	ReverseOfflineUnit::GetProperty
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus			ReverseOfflineUnit::SetProperty(AudioUnitPropertyID 	inID,
													AudioUnitScope 			inScope,
													AudioUnitElement 		inElement,
													const void *			inData,
													UInt32 					inDataSize)
{
	if (inScope == kAudioUnitScope_Global) {
		switch (inID) {
				//whenever these properties are set we *MAY* take this to mean the input has changed
				// at this point we require preflighting again...
			case kAudioUnitOfflineProperty_InputSize:
				if (inDataSize < sizeof(UInt64)) return kAudioUnitErr_InvalidPropertyValue;
				mNumInputSamples = *(UInt64*)inData;
				return noErr;

			case kAudioUnitOfflineProperty_StartOffset:
				if (inDataSize < sizeof(UInt64)) return kAudioUnitErr_InvalidPropertyValue;
				mStartOffset = *(UInt64*)inData;
				return noErr;
		}
	}
	return AUBase::SetProperty (inID, inScope, inElement, inData, inDataSize);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark ____Initialization_Formats

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	ReverseOfflineUnit::Initialize
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus 	ReverseOfflineUnit::Initialize()
{
    const AUChannelInfo *auChannelConfigs = NULL;
    UInt32 numIOconfigs = SupportedNumChannels(&auChannelConfigs);
		// does the unit publish specific information about channel configurations?
    if ((numIOconfigs > 0) && (auChannelConfigs != NULL))
    {
        SInt16 auNumInputs = (SInt16) GetStreamFormat(kAudioUnitScope_Input, 0).mChannelsPerFrame;
        SInt16 auNumOutputs = (SInt16) GetStreamFormat(kAudioUnitScope_Output, 0).mChannelsPerFrame;
        bool foundMatch = false;
        for (UInt32 i = 0; (i < numIOconfigs) && !foundMatch; ++i)
        {
            SInt16 configNumInputs = auChannelConfigs[i].inChannels;
            SInt16 configNumOutputs = auChannelConfigs[i].outChannels;
            if ((configNumInputs < 0) && (configNumOutputs < 0))
            {
					// unit accepts any number of channels on input and output
                if (((configNumInputs == -1) && (configNumOutputs == -2)) || ((configNumInputs == -2) && (configNumOutputs == -1)))
                    foundMatch = true;
					// unit accepts any number of channels on input and output IFF they are the same number on both scopes
                else if (((configNumInputs == -1) && (configNumOutputs == -1)) && (auNumInputs == auNumOutputs))
                    foundMatch = true;
					// unit has specified a particular number of channels on both scopes
                else
                    continue;
            }
            else
            {
					// the -1 case on either scope is saying that the unit doesn't care about the 
					// number of channels on that scope
                bool inputMatch = (auNumInputs == configNumInputs) || (configNumInputs == -1);
                bool outputMatch = (auNumOutputs == configNumOutputs) || (configNumOutputs == -1);
                if (inputMatch && outputMatch)
                    foundMatch = true;
            }
        }
        if (!foundMatch)
            return kAudioUnitErr_FormatNotSupported;
    }
    else
    {
			// there is no specifically published channel info
			// so for those kinds of effects, the assumption is that the channels (whatever their number)
			// should match on both scopes
		UInt32 outputChannels = GetOutput(0)->GetStreamFormat().mChannelsPerFrame;
		UInt32 inputChannels = GetInput(0)->GetStreamFormat().mChannelsPerFrame;
        if ((outputChannels != inputChannels) || (outputChannels == 0))
            return kAudioUnitErr_FormatNotSupported;
    }

    return noErr;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	ReverseOfflineUnit::StreamFormatWritable
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool		ReverseOfflineUnit::StreamFormatWritable(	AudioUnitScope					scope,
														AudioUnitElement				element)
{
	return IsInitialized() ? false : true;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark ____Render

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	ReverseOfflineUnit::Render
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus 	ReverseOfflineUnit::Render(	AudioUnitRenderActionFlags &ioActionFlags,
											const AudioTimeStamp &			inTimeStamp,
											UInt32							nFrames)
{
	if (!HasInput(0))
		return kAudioUnitErr_NoConnection;

	// first we have to make sure that the rendering flag matches our internal state...
	bool preflight = (ioActionFlags & kAudioOfflineUnitRenderAction_Preflight);
	bool doRender = (ioActionFlags & kAudioOfflineUnitRenderAction_Render);
		
		// one of these two flags have to be provided
	if (!preflight && !doRender)
		return kAudioUnitErr_InvalidOfflineRender;

	if (preflight) {
		ioActionFlags |= kAudioOfflineUnitRenderAction_Complete;
		return noErr;
	}
	
	// OK - now we have to figure out which input we want based on the output time
	// we're asked for
	
	// in this case, as we're a reverse unit this is a simple calculation...
	// we need a new time stamp based on the one we were given.
	
	AudioTimeStamp ts (inTimeStamp);
	ts.mSampleTime = (mNumInputSamples - mStartOffset) - nFrames - inTimeStamp.mSampleTime;

	UInt32 numFramesToPull = nFrames;
	bool renderPhaseComplete = false;
	
	if (ts.mSampleTime < mStartOffset) {

		// one word of caution.. if we're preflighting we need to change that state to be ready to render
		// as to get here we're basically done with our sample processing
		
			// do we have a partial buffer to fill
		ts.mSampleTime += nFrames;
		if (ts.mSampleTime > mStartOffset) {
			numFramesToPull = (UInt32)ts.mSampleTime;
			ts.mSampleTime = mStartOffset;
			renderPhaseComplete = true;
		} else {
				// this is just a protection if someone pulls us for data past what we have...
			ioActionFlags |= kAudioOfflineUnitRenderAction_Complete;
		}
	}

	// OK - now we are good to go...
		
	AUOutputElement *theOutput = GetOutput(0);	// throws if error
	AUInputElement *theInput = GetInput(0);
	
	OSStatus result = theInput->PullInput (ioActionFlags, ts, 0 /* element */, numFramesToPull);
	
	if (result) return result;

	// ok - now we reverse our input data
	// if we have a remainder we need to zero out the output buffer
	
	AudioBufferList &inputBuffer = theInput->GetBufferList();
	AudioBufferList &outputBuffer = theOutput->GetBufferList();
	
	// we'll do the reverse one channel at a time...
	for (UInt32 i = 0; i < inputBuffer.mNumberBuffers; ++i) 
	{
		Float32* inSampleData = (Float32*)inputBuffer.mBuffers[i].mData;
		Float32* outSampleData = (Float32*)outputBuffer.mBuffers[i].mData;
		
		
		for (SInt32 in = numFramesToPull, out = 0; --in >= 0 ;++out)
			outSampleData[out] = inSampleData[in];
	}

	if (renderPhaseComplete) {
		UInt32 numValidBytes = numFramesToPull * sizeof (Float32);
			// we just need to reset the numbytes field as that indicates the valid portion of the buffer
		for (UInt32 i = 0; i < outputBuffer.mNumberBuffers; ++i) {
			outputBuffer.mBuffers[i].mDataByteSize = numValidBytes;
		}

		ioActionFlags |= kAudioOfflineUnitRenderAction_Complete;
	}

	return noErr;
}
