/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/



namespace hise {
using namespace juce;


MPEModulator::MPEModulator(MainController *mc, const String &id, int voiceAmount, Modulation::Mode m) :
	EnvelopeModulator(mc, id, voiceAmount, m),
	Modulation(m),
	table(new SampleLookupTable()),
	monoState(-1),
	g((Gesture)(int)getDefaultValue(GestureCC)),
	smoothedIntensity(getIntensity())
{
	setAttribute(DefaultValue, getDefaultValue(DefaultValue), dontSendNotification);

	parameterNames.add("GestureCC");
	parameterNames.add("SmoothingTime");
	parameterNames.add("DefaultValue");
	parameterNames.add("SmoothedIntensity");

	getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData().sendAmountChangeMessage();

	getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData().addListener(this);

	for (int i = 0; i < polyManager.getVoiceAmount(); i++) states.add(createSubclassedState(i));
}

MPEModulator::~MPEModulator()
{
	getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData().removeListener(this);
	
	getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData().removeConnection(this);
}

void MPEModulator::mpeModeChanged(bool isEnabled)
{
	const bool isConnected = getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData().contains(this);

	isActive = isEnabled;

	for (int i = 0; i < states.size(); i++)
	{
		reset(i);
	}

	const bool shouldBeBypassed = !isActive || !isConnected;

	setBypassed(shouldBeBypassed);

	sendChangeMessage();

	

}

void MPEModulator::mpeModulatorAssigned(MPEModulator* m, bool wasAssigned)
{
	if (m == this)
	{
		const bool shouldBeBypassed = !isActive || !wasAssigned;

		setBypassed(shouldBeBypassed, sendNotification);
		sendChangeMessage();
	}
}

void MPEModulator::mpeDataReloaded()
{
	
}

void MPEModulator::setInternalAttribute(int parameterIndex, float newValue)
{
	if (parameterIndex < EnvelopeModulator::Parameters::numParameters)
	{
		EnvelopeModulator::setInternalAttribute(parameterIndex, newValue);
	}

	if (parameterIndex == EnvelopeModulator::Parameters::Monophonic)
	{
		monophonicVoiceCounter = 0;

		activeStates.clear();

		if (isMonophonic)
		{
			activeStates.insert(&monoState);
			mpeValues.reset();
		}

		for (int i = 0; i < states.size(); i++)
			reset(i);
	}
	else if (parameterIndex == SpecialParameters::GestureCC)
	{
		g = (Gesture)(int)newValue;

		for (int i = 0; i < states.size(); i++)
			reset(i);

		setAttribute(DefaultValue, getDefaultValue(DefaultValue), dontSendNotification);

		mpeValues.reset();

	}
	else if (parameterIndex == SpecialParameters::SmoothingTime)
	{
		updateSmoothingTime(newValue);
	}
	else if (parameterIndex == SpecialParameters::DefaultValue)
	{
		if (getMode() == Modulation::GainMode)
			defaultValue = jlimit<float>(0.0f, 1.0f, newValue);
		else
			defaultValue = jlimit<float>(0.0f, 1.0f, newValue / 24.0f + 0.5f);

	}
	else if (parameterIndex == SpecialParameters::SmoothedIntensity)
	{
		smoothedIntensity = getMode() == Modulation::GainMode ? newValue : newValue / 12.0f;
		setIntensity(smoothedIntensity);
	}
		
}

float MPEModulator::getDefaultValue(int parameterIndex) const
{
	if (parameterIndex < EnvelopeModulator::Parameters::numParameters)
	{
		return EnvelopeModulator::getDefaultValue(parameterIndex);
	}

	else if (parameterIndex == SpecialParameters::GestureCC)
	{
		return (float)(int)(getMode() == Modulation::GainMode ? Gesture::Press : Gesture::Glide);
	}
	else if (parameterIndex == SpecialParameters::SmoothingTime)
	{
		return 200.0f;
	}
	else if (parameterIndex == SpecialParameters::DefaultValue)
	{
		if (getMode() == Modulation::PitchMode)
			return 0.0f;

		switch (g)
		{
		case Press: return 0.0f;
		case Slide: return 0.5f;
		case Glide: return 0.5f;
		case Stroke: return 0.0f;
		case Lift: return 0.0f;
		case numGestures: return 0.0f;
		}
	}
	else if (parameterIndex == SpecialParameters::SmoothedIntensity)
	{
		return getMode() == Modulation::GainMode ? 1.0f : 0.0f;
	}
		

	return 0.0f;
}

void MPEModulator::resetToDefault()
{
	g = (Gesture)(int)getDefaultValue(SpecialParameters::GestureCC);
	
	
	setAttribute(DefaultValue, getDefaultValue(DefaultValue), dontSendNotification);

	updateSmoothingTime(getDefaultValue(SpecialParameters::SmoothingTime));
	smoothedIntensity = getDefaultValue(SpecialParameters::SmoothedIntensity);
	setIntensity(smoothedIntensity);
	table->reset();
	table->sendChangeMessage();
	sendTableIndexChangeMessage(false, table, 0);
	sendChangeMessage();
}

float MPEModulator::getAttribute(int parameterIndex) const
{
	if (parameterIndex < EnvelopeModulator::Parameters::numParameters)
	{
		return EnvelopeModulator::getAttribute(parameterIndex);
	}

	if (parameterIndex == SpecialParameters::GestureCC)
	{
		return (float)(int)g;
	}
	else if (parameterIndex == SpecialParameters::SmoothingTime)
		return smoothingTime;
	else if (parameterIndex == DefaultValue)
	{
		if (getMode() == Modulation::GainMode)
			return defaultValue;
		else
			return (defaultValue - 0.5f) * 24.0f;
	}
	else if (parameterIndex == SpecialParameters::SmoothedIntensity)
	{
		return getMode() == Modulation::GainMode ? smoothedIntensity : smoothedIntensity * 12.0f;
	}
		

	return 0.0f;
}



void MPEModulator::restoreFromValueTree(const ValueTree &v)
{
	EnvelopeModulator::restoreFromValueTree(v);

	loadAttribute(GestureCC, "GestureCC");
	loadAttribute(SmoothingTime, "SmoothingTime");
	loadAttribute(DefaultValue, "DefaultValue");
	loadAttribute(SmoothedIntensity, "SmoothedIntensity");
	loadTable(table, "Table");
}

juce::ValueTree MPEModulator::exportAsValueTree() const
{
	ValueTree v = EnvelopeModulator::exportAsValueTree();

	saveAttribute(GestureCC, "GestureCC");
	saveAttribute(SmoothingTime, "SmoothingTime");
	saveAttribute(DefaultValue, "DefaultValue");
	saveAttribute(SmoothedIntensity, "SmoothedIntensity");
	saveTable(table, "Table");

	return v;
}

void MPEModulator::startVoice(int voiceIndex)
{
	EnvelopeModulator::startVoice(voiceIndex);

	if (auto s = getState(voiceIndex))
	{
		s->isRingingOff = false;

		float startValue = defaultValue;
		if (g == Press)
			startValue *= unsavedStrokeValue;

		if (isMonophonic)
		{
			s->midiChannel = unsavedChannel;

			if (monophonicVoiceCounter > 0)
			{
				if (shouldRetrigger)
				{
					monoState.smoother.setDefaultValue(startValue);
					monoState.smoother.resetToValue(startValue, 5.0f);
				}
			}
			else
			{
				monoState.isPressed = true;
				monoState.smoother.setDefaultValue(startValue);
				monoState.smoother.resetToValue(startValue);

				if (g == Stroke)
					monoState.targetValue = unsavedStrokeValue;

			}

			monophonicVoiceCounter++;
		}
		else
		{
			s->midiChannel = unsavedChannel;
			s->isPressed = true;
			s->smoother.setDefaultValue(startValue);
			s->smoother.resetToValue(startValue);

			if (g == Stroke)
				s->targetValue = unsavedStrokeValue;

			else if (g == Press)
				s->targetValue = startValue;

			//s->targetValue = g == Stroke ? unsavedStrokeValue : defaultValue;

			activeStates.insert(s);
		}

		
	}
}

void MPEModulator::stopVoice(int voiceIndex)
{
	EnvelopeModulator::stopVoice(voiceIndex);

	if (isMonophonic)
	{
	}
	else
	{
		if (auto s = getState(voiceIndex))
		{
			s->isPressed = false;
			s->isRingingOff = s->targetValue == 0.0f;
		}
	}
}

void MPEModulator::reset(int voiceIndex)
{
	EnvelopeModulator::reset(voiceIndex);

	if (isMonophonic)
	{
		monophonicVoiceCounter = jmax<int>(0, monophonicVoiceCounter - 1);

		if (monophonicVoiceCounter == 0)
		{
			monoState.targetValue = defaultValue;
			
			monoState.isPressed = false;

			mpeValues.reset();
		}
			
	}
	else
	{
		if (auto s = getState(voiceIndex))
		{
			activeStates.remove(s);

			s->smoother.setDefaultValue(defaultValue);
			s->smoother.resetToValue(defaultValue);
			s->targetValue = defaultValue;
			s->midiChannel = -1;
			s->isPressed = false;
		}
	}
}

bool MPEModulator::isPlaying(int voiceIndex) const
{
	if (isMonophonic || (getIntensity() < 1.0f))
	{
		return true;
	}
	else if(auto s = getState(voiceIndex))
	{
		if (s->isRingingOff && !s->smoother.isSmoothingActive())
			return false;
	}

	return true;
}

void MPEModulator::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	EnvelopeModulator::prepareToPlay(sampleRate, samplesPerBlock);

	monoState.smoother.prepareToPlay(sampleRate);
	monoState.smoother.setSmoothingTime(smoothingTime);
	
	for (auto s_ : states)
	{
		auto s = static_cast<MPEState*>(s_);
		
		s->smoother.prepareToPlay(sampleRate);
		s->smoother.setSmoothingTime(smoothingTime);
	}
}

void MPEModulator::calculateBlock(int startSample, int numSamples)
{
	const int voiceIndex = isMonophonic ? -1 : polyManager.getCurrentVoice();

	if (auto s = getState(voiceIndex))
	{
		auto w = internalBuffer.getWritePointer(0, startSample);

		s->smoother.fillBufferWithSmoothedValue(s->targetValue, w, numSamples);

		if (isMonophonic || polyManager.getLastStartedVoice() == voiceIndex)
		{
			setOutputValue(w[0]);
		}
	}
}

void MPEModulator::handleHiseEvent(const HiseEvent& m)
{
	EnvelopeModulator::handleHiseEvent(m);

	auto c = m.getChannel();

	float midiValue;

	if (m.isNoteOn())
	{
		unsavedChannel = c;

		midiValue = (float)m.getVelocity() / 127.0f;
		midiValue = jlimit(0.0f, 1.0f, midiValue);

		if (g == Stroke)
		{
			const float targetValue = table->getInterpolatedValue(midiValue * (float)SAMPLE_LOOKUP_TABLE_SIZE);
			sendTableIndexChangeMessage(false, table, midiValue);
			unsavedStrokeValue = targetValue;
		}
		else
		{
			unsavedStrokeValue = midiValue;
		}
		
		return;
	}
	
	if (g == Press && m.isChannelPressure())
	{
		midiValue = (float)m.getNoteNumber() / 127.0f;
	}
	else if (g == Slide && m.isControllerOfType(74))
	{
		midiValue = (float)m.getControllerValue() / 127.0f;
	}
	else if (g == Glide && m.isPitchWheel())
	{
		midiValue = ((float)m.getPitchWheelValue() - 8192.0f) / 2048.0f;
		midiValue = 0.5f * midiValue + 0.5f;

	}
	else if (g == Lift && m.isNoteOff())
	{
		midiValue = (float)m.getVelocity() / 127.0f;
	}
	else
	{
		return;
	}

	

	midiValue = jlimit(0.0f, 1.0f, midiValue);

	if (isMonophonic)
	{
		midiValue = mpeValues.storeAndGetMaxValue(g, c, midiValue);
	}

	const float targetValue = table->getInterpolatedValue(midiValue * (float)SAMPLE_LOOKUP_TABLE_SIZE);

	bool found = false;

	for (auto s : activeStates)
	{
		const bool midiChannelMatches = isMonophonic || s->midiChannel == c;

		if (s->isPressed && midiChannelMatches)
		{
			s->targetValue = targetValue;

			const bool voiceIndexMatches = isMonophonic || s->index == polyManager.getLastStartedVoice();

			if (!found && voiceIndexMatches)
			{
				sendTableIndexChangeMessage(false, table, midiValue);
				found = true;
			}
		}
			
	}

}

hise::ProcessorEditorBody * MPEModulator::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new MPEModulatorEditor(parentEditor);

#else 

	ignoreUnused(parentEditor);

	jassertfalse;
	return nullptr;

#endif
}

void MPEModulator::updateSmoothingTime(float newTime)
{
	if (newTime != smoothingTime)
	{
		smoothingTime = newTime;

		for (int i = 0; i < states.size(); i++)
			getState(i)->smoother.setSmoothingTime(smoothingTime);
	}
}

hise::MPEModulator::MPEState * MPEModulator::getState(int voiceIndex)
{
	if (isMonophonic)
		return &monoState;

	if (auto s = states[voiceIndex])
		return static_cast<MPEState*>(s);

	return nullptr;
}

const hise::MPEModulator::MPEState * MPEModulator::getState(int voiceIndex) const
{
	if (isMonophonic)
		return &monoState;

	if (auto s = states[voiceIndex])
		return static_cast<MPEState*>(s);

	return nullptr;
}







void MPEModulator::MPEValues::reset()
{
	for (int i = 0; i < 16; i++)
	{
		pressValues[i] = 0.0f;
		strokeValues[i] = 0.f;
		slideValues[i] = 0.0f;
		glideValues[i] = 0.5f;
		liftValues[i] = 0.0f;
	}
}

float MPEModulator::MPEValues::storeAndGetMaxValue(Gesture g, int channel, float value)
{
	switch (g)
	{
	case Press:	pressValues[channel - 1] = value; return FloatVectorOperations::findMaximum(pressValues, 16);
	case Slide:	slideValues[channel - 1] = value; return FloatVectorOperations::findMaximum(slideValues, 16);
	case hise::MPEModulator::Glide:
	{
		glideValues[channel - 1] = value;

		int absIndex = -1;
		float maxAbsValue = 0.0f;

		for (int i = 0; i < 16; i++)
		{
			const float distance = fabsf(glideValues[i] - 0.5f);

			if (distance >= maxAbsValue)
			{
				maxAbsValue = distance;
				absIndex = i;
			}
		}

		return glideValues[absIndex];
	}

	case hise::MPEModulator::Stroke:
		break;
	case hise::MPEModulator::Lift:
		break;
	case hise::MPEModulator::numGestures:
		break;
	default:
		break;
	}

	return 1.0f;
}

}

