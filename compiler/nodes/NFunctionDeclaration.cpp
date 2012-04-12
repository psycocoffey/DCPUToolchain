/**

	File:			NFunctionDeclaration.cpp

	Project:		DCPU-16 Tools
	Component:		Compiler

	Authors:		James Rhodes

	Description:	Defines the NFunctionDeclaration AST class.

**/

#include "../asmgen.h"
#include "NFunctionDeclaration.h"
#include "NFunctionSignature.h"

NFunctionDeclaration::NFunctionDeclaration(const NType& type, const NIdentifier& id, const VariableList& arguments, NBlock& block)
	: id(id), block(block), pointerType(NULL), NDeclaration("function"), NFunctionSignature(type, arguments)
{
	// We need to generate an NFunctionPointerType for when we are resolved
	// as a pointer (for storing a reference to us into a variable).
	this->pointerType = new NFunctionPointerType(type, arguments);
}

NFunctionDeclaration::~NFunctionDeclaration()
{
	delete this->pointerType;
}

AsmBlock* NFunctionDeclaration::compile(AsmGenerator& context)
{
	AsmBlock* block = new AsmBlock();

	// Output a safety boundary if the assembler supports
	// it and we want to output in debug mode.
	if (context.isAssemblerDebug())
	{
		if (context.getAssembler().supportsSafetyBoundary)
			*block << ".BOUNDARY" << std::endl;
		else if (context.getAssembler().supportsDataInstruction)
			*block << "DAT 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0" << std::endl;
	}

	// Calculate total stack space required.
	StackFrame* frame = context.generateStackFrame(this, false);

	// Output the leading information and immediate jump.
	*block <<  ":cfunc_" << this->id.name << std::endl;
	*block <<  "	SET PC, cfunc_" << this->id.name << "_actual" << std::endl;
	*block <<  "	DAT " << frame->getSize() << std::endl;
	*block <<  ":cfunc_" << this->id.name << "_actual" << std::endl;

	// Now compile the block.
	AsmBlock* iblock = this->block.compile(context);
	*block << *iblock;
	delete iblock;

	// Return from this function.
	*block <<  "	SET A, 0xFFFF" << std::endl;
	*block <<  "	SET X, " << frame->getSize() << std::endl;
	*block <<  "	SET PC, _stack_return" << std::endl;

	// Clean up frame.
	context.finishStackFrame(frame);

	return block;
}

AsmBlock* NFunctionDeclaration::reference(AsmGenerator& context)
{
	throw new CompilerException("Unable to get reference to the result of a function declaration.");
}