/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace hnode {
namespace jit {
using namespace juce;
using namespace asmjit;


struct BaseCompiler
{
	struct DeadCodeException
	{
		DeadCodeException(ParserHelpers::CodeLocation l) : location(l) {};

		ParserHelpers::CodeLocation location;
	};

	enum MessageType
	{
		Error,
		Warning,
		PassMessage,
		ProcessMessage,
		VerboseProcessMessage,
		AsmJitMessage,
		numMessageTypes
	};

	virtual ~BaseCompiler() {};

	enum Pass
	{
		Parsing,
		ResolvingSymbols,
		TypeCheck,
		FunctionParsing,

		FunctionCompilation,
		RegisterAllocation,
		CodeGeneration,
		numPasses
	};

	struct OptimizationPassBase
	{
		virtual ~OptimizationPassBase() {};
		virtual String getName() const = 0;
		virtual Result process(SyntaxTree* tree) = 0;
	};

	void setDebugHandler(Compiler::DebugHandler* l)
	{
		debugHandler = l;
	}

	void logMessage(MessageType level, const String& s)
	{
		if (level > verbosity)
			return;

		String m;

		switch (level)
		{
		case Error:  m << "ERROR: "; break;
		case Warning:  m << "WARNING: "; break;
		case PassMessage:  m << "PASS: "; break;
		case ProcessMessage: m << "- "; break;
		case VerboseProcessMessage: m << "-- "; break;
		}

		m << s << "\n";

		if (debugHandler != nullptr)
			debugHandler->logMessage(m);
	}

	void setMessageLevel(MessageType maxVerbosity)
	{
		verbosity = maxVerbosity;
	}

	void setCurrentPass(Pass p)
	{
		currentPass = p;

		switch (currentPass)
		{
		case Parsing:logMessage(PassMessage, "Parsing class statements"); break;
		case FunctionParsing:	    logMessage(PassMessage, "Parsing Functions"); break;
		case ResolvingSymbols:		logMessage(PassMessage, "Resolving symbols"); break;
		case RegisterAllocation:	logMessage(PassMessage, "Allocating Registers"); break;
		case TypeCheck:				logMessage(PassMessage, "Checking Types"); break;
		case FunctionCompilation:	logMessage(PassMessage, "Compiling Functions"); break;
		case CodeGeneration:	logMessage(PassMessage, "Generating assembly code"); break;
		case numPasses:
		default:
			break;
		}
	}

	Pass getCurrentPass() { return currentPass; }

	void executePass(Pass p, BaseScope* scope, SyntaxTree* statements);

	void addOptimization(OptimizationPassBase* newPass)
	{
		passes.add(newPass);
	}

	AssemblyRegister::Ptr getRegFromPool(Types::ID type)
	{
		return registerPool.getNextFreeRegister(type);
	}

	AssemblyRegisterPool registerPool;

private:

	OwnedArray<OptimizationPassBase> passes;

	MessageType verbosity = numMessageTypes;

	Pass currentPass;

	WeakReference<Compiler::DebugHandler> debugHandler;

	JUCE_DECLARE_WEAK_REFERENCEABLE(BaseCompiler)
};

}
}