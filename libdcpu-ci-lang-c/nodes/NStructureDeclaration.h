/**

    File:           NStructureDeclaration.h

    Project:        DCPU-16 Tools
    Component:      LibDCPU-ci-lang-c

    Authors:        James Rhodes, Patrick Flick

    Description:    Declares the NStructureDeclaration AST class.

**/

#ifndef __DCPU_COMP_NODES_STRUCTURE_DECLARATION_H
#define __DCPU_COMP_NODES_STRUCTURE_DECLARATION_H

#include "NDeclaration.h"
#include "NIdentifier.h"
#include "Lists.h"

class NStructureDeclaration : public NDeclaration
{
public:
    const NIdentifier& id;
    DeclarationList fields;
    NStructureDeclaration(const NIdentifier& id, const DeclarationList& fields) :
        id(id), fields(fields), NDeclaration("structure") { };
    virtual AsmBlock* compile(AsmGenerator& context);
    virtual AsmBlock* reference(AsmGenerator& context);
    virtual void analyse(AsmGenerator& context, bool reference);
    virtual size_t getWordSize(AsmGenerator& context);
    
    virtual void insertIntoScope(AsmGenerator& context, SymbolTableScope& scope, ObjectPosition position);
};

#endif
