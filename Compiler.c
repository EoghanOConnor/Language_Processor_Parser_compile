/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*      File: Compiler.c                                                                                        */
/*                                                                                                              */
/*                                                                                                              */
/*           Group Members:                                                                                     */
/*                                                                                                              */
/*           Eoghan O'Connor                                                                                    */
/*                                                                                                              */
/*                                                                                                              */
/*                                                                                                              */
/*                                                                                                              */
/*                                                                                                              */
/*                                                                                                              */
/* Parser structure is a follows:                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*      <ParseProgram>             :==  "PROGRAM" <ParseVariable> ";"                                           */
/*                                      [<ParseDeclarations>] {<ParseProceDeclaration>} <ParseBlock> "."        */
/*                                                                                                              */
/*      <ParseDeclarations>        :==  "VAR" <Parsevariable> {"," <ParseVariable>} ";"                         */
/*                                                                                                              */
/*                                                                                                              */
/*      <ParseProceDeclarations>   :==  "PROCEDURE" <ParseVariable> [<ParseParameterList>]";"                   */
/*                                      [<ParseDeclarations>] {<ParseProceDeclaration>} <ParseBlock> ";"        */
/*                                                                                                              */
/*      <ParseParameterList>       :==  "(" <ParserFormalParameter> { "," <ParserFormalParameter> } ")"         */
/*                                                                                                              */
/*      <ParseFormalParameter>     :==  ["REF"] <ParserVariable>                                                */
/*                                                                                                              */
/*      <ParseBlock>               :==  "BEGIN" {<ParseStatement> ";"} "END"                                    */
/*                                                                                                              */
/*      <ParseStatement>           :==  <ParseSimpleStatement> | <ParseWhileStatement> |                        */
/*                                      <ParseifStatement> |<ParseReadStatement>|<ParseWriteStatement>          */
/*                                                                                                              */
/*      <ParseSimpleStatement>     :== <ParseVarOrProcName> <ParseRestOfStatement>                              */
/*                                                                                                              */
/*      <ParseRestOfStatement>     :== <ParseProcCallList> | <ParseAssignment>| ε                               */
/*                                                                                                              */
/*      <ParseProcCallList>        :== "(" <ParseActualParameter> {","<ParseActualParameter>} ")"               */
/*                                                                                                              */
/*      <ParseAssignment>          :== ":==" <ParseExpression>                                                  */
/*                                                                                                              */
/*      <ParseActualParameter>     :== <ParseVariable> | <ParseExpression>                                      */
/*                                                                                                              */
/*      <ParseWhileStatment>       :== "WHILE" <ParseBooleanExpression> "DO" <ParseBlock>                       */
/*                                                                                                              */
/*      <ParseifStatement>         :== "IF" <ParseBooleanExpression> "THEN" <ParseBlock> ["ELSE" <ParseBlock>]  */
/*                                                                                                              */
/*      <ParseReadStatement>       :== "READ" "(" <ParseVariable> {"," <ParseVariable>} ")"                     */
/*                                                                                                              */
/*      <ParseWriteStatement>      :== "WRITE" "(" <ParseExpression> {"," <ParseExpression>} ")"                */
/*                                                                                                              */
/*                                                                                                              */
/*      <ParseExpression>          :==  <ParseCompoundTerm> {("+"|"-") <ParseCompoundTerm>}                     */
/*                                                                                                              */
/*      <ParseCompoundTerm>        :==  <ParseTerm> {<ParseMultOp> <ParseTerm>}                                 */
/*                                                                                                              */
/*      <ParseTerm>                :==  ["-"]  <ParseSubTerm>                                                   */
/*                                                                                                              */
/*      <ParseSubTerm>             :==  <ParseVariable> | <ParseIntConst> | "(" <ParseExpression> ")"           */
/*                                                                                                              */
/*      <ParseBooleanExpression>   :==  <ParseExpression> <ParseRelOp> <ParseExpression>                        */
/*                                                                                                              */
/*      <ParseRelOp>               :==  "=" | "<=" | ">=" | "<" | ">"                                           */
/*                                                                                                              */
/*      <ParseVarOrProcName>       :==  <Identifier>                                                            */
/*                                                                                                              */
/*      <ParseVariable>            :==  <Identifier>                                                            */
/*                                                                                                              */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "scanner.h"
#include "line.h"
#include "sets.h"
#include "symbol.h"
#include "code.h"
#include "strtab.h"

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Global variables used by this compiler.                                 */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE FILE *InputFile;    /*  CPL source comes from here.          */
PRIVATE FILE *ListFile;     /*  For nicely-formatted syntax errors.  */
PRIVATE FILE *CodeFile;     /* File for the assembly code*/
PRIVATE TOKEN CurrentToken; /*  Parser lookahead token.  Updated by  */
                            /*  routine Accept (below).  Must be     */
                            /*  initialised before parser starts.    */

PRIVATE SET StatementFS_aug;
PRIVATE SET StatementFBS;
PRIVATE SET DeclarationFS_aug;
PRIVATE SET ProcDeclarationFS_aug;
PRIVATE SET ProcDeclarationFBS;

PRIVATE int ErrorFlag;
PRIVATE int scope;
PRIVATE int varaddress;

PRIVATE int scope; /*Variable to track the scope value of input code */

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Function prototypes                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE int OpenFiles(int argc, char *argv[]);
PRIVATE void ParseProgram(void);
PRIVATE void ParseDeclarations(int loc_flag);
PRIVATE void ParseProcDeclarations(void);
PRIVATE void ParseParameterList(void);
PRIVATE void ParseFormalParameter(void);
PRIVATE void ParseBlock(void);
PRIVATE void ParseStatement(void);
PRIVATE void ParseSimpleStatement(void);
PRIVATE void ParseRestofStatement(SYMBOL *target);
PRIVATE void ParseProcCallList(SYMBOL *target);
PRIVATE void ParseAssignment(void);
PRIVATE void ParseActualParameter(int isRef_flag);
PRIVATE void ParseWhileStatement(void);
PRIVATE void ParseIfStatement(void);
PRIVATE void ParseReadStatement(void);
PRIVATE void ParseWriteStatement(void);
PRIVATE void ParseExpression(void);
PRIVATE void ParseCompoundTerm(void);
PRIVATE void ParseTerm(void);
PRIVATE void ParseSubTerm(void);
PRIVATE void ParseBooleanExpression(void);
PRIVATE void ParseRelOp(void);
PRIVATE void ParseVariable(void);
PRIVATE void ParseVarOrProcName(void);
PRIVATE void Accept(int code);
/* Implements augmented S-Algol */
PRIVATE void Synchronise(SET *F, SET *FB);
PRIVATE void SetupSets(void);
PRIVATE SYMBOL *MakeSymbolTableEntry(int symtype, int *varaddress);
PRIVATE SYMBOL *LookupSymbol(void);
/*
PRIVATE void ReadToEndOfFile(void);
*/

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Main: Smallparser entry point.  Sets up parser globals (opens input and */
/*        output files, initialises current lookahead), then calls          */
/*        "ParseProgram" to start the parse.                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PUBLIC int main(int argc, char *argv[])
{
    ErrorFlag = 0;
    if (OpenFiles(argc, argv))
    {
        InitCharProcessor(InputFile, ListFile);
        InitCodeGenerator(CodeFile);
        SetupSets();
        CurrentToken = GetToken();
        ParseProgram();
        WriteCodeFile();
        fclose(InputFile);
        fclose(ListFile);
        if (ErrorFlag == 0)
        {
            printf("Valid syntax\n");
        }
        else
        {
            printf("SYNTAX INVALID\n");
        }
        return EXIT_SUCCESS;
    }
    else
        return EXIT_FAILURE;
}

/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*  Parser routines: Recursive-descent implementaion of the grammar's                                           */
/*                   productions.                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*  ParseProgram implements:                                                                                    */
/*                                                                                                              */
/*      <ParseProgram>             :==  "PROGRAM" <ParseVariable> ";"                                           */
/*                                      [<ParseDeclarations>] {<ParseProceDeclaration>} <ParseBlock> "."        */
/*                                                                                                              */
/*                                                                                                              */
/*      Inputs:       None                                                                                      */
/*                                                                                                              */
/*      Outputs:      None                                                                                      */
/*                                                                                                              */
/*      Returns:      Nothing                                                                                   */
/*                                                                                                              */
/*      Side Effects: Lookahead token advanced.                                                                 */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/

PRIVATE void ParseProgram(void)
{
    Accept(PROGRAM);
    MakeSymbolTableEntry(STYPE_PROGRAM, &varaddress);
    ParseVarOrProcName();
    Accept(SEMICOLON);
    /* Synch SET 1 */
    Synchronise(&DeclarationFS_aug, &ProcDeclarationFBS);
    if (CurrentToken.code == VAR)
        ParseDeclarations(0);
    /* Synch SET 2 */
    Synchronise(&ProcDeclarationFS_aug, &ProcDeclarationFBS);
    while (CurrentToken.code == PROCEDURE)
    {
        ParseProcDeclarations();
        /* resynch */
        Synchronise(&DeclarationFS_aug, &ProcDeclarationFBS);
    }
    ParseBlock();
    Emit(I_HALT, 0);
    Accept(ENDOFPROGRAM); /* Token "." has name ENDOFPROGRAM */
}

/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*  Parser routines: Recursive-descent implementaion of the grammar's                                           */
/*                   productions.                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*  ParseDeclarations implements:                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*                                                                                                              */
/*                                                                                                              */
/*      <ParseDeclarations>        :==  "VAR" <Parsevariable> {"," <ParseVariable>} ";"                         */
/*                                                                                                              */
/*      <ParseVariable>            :==  <Identifier>                                                            */
/*                                                                                                              */
/*      Inputs:       None                                                                                      */
/*                                                                                                              */
/*      Outputs:      None                                                                                      */
/*                                                                                                              */
/*      Returns:      Nothing                                                                                   */
/*                                                                                                              */
/*      Side Effects: Lookahead token advanced.                                                                 */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/

PRIVATE void ParseDeclarations(int loc_flag)
{

    Accept(VAR);
    if (loc_flag != 1)
    {
        MakeSymbolTableEntry(STYPE_VARIABLE, &varaddress);
        ParseVariable();
        while (CurrentToken.code == COMMA)
        {
            Accept(COMMA);
            MakeSymbolTableEntry(STYPE_VARIABLE, &varaddress);
            ParseVariable();
        }
    }
    else
    {
        MakeSymbolTableEntry(STYPE_LOCALVAR, &varaddress);
        ParseVariable();
        while (CurrentToken.code == COMMA)
        {
            Accept(COMMA);
            MakeSymbolTableEntry(STYPE_LOCALVAR, &varaddress);
            ParseVariable();
        }
    }

    Accept(SEMICOLON);
}
/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*  Parser routines: Recursive-descent implementaion of the grammar's                                           */
/*                   productions.                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*  ParseProcDeclarations implements:                                                                           */
/*                                                                                                              */
/*                                                                                                              */
/*      <ParseProceDeclarations>   :==  "PROCEDURE" <ParseVariable> [<ParseParameterList>]";"                   */
/*                                      [<ParseDeclarations>] {<ParseProceDeclaration>} <ParseBlock> ";"        */
/*                                                                                                              */
/*                                                                                                              */
/*      Inputs:       None                                                                                      */
/*                                                                                                              */
/*      Outputs:      None                                                                                      */
/*                                                                                                              */
/*      Returns:      Nothing                                                                                   */
/*                                                                                                              */
/*      Side Effects: Lookahead token advanced.                                                                 */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/

PRIVATE void ParseProcDeclarations(void)
{
    int backpatch_addr, loc_flag;
    /* int ptype[];  pcount,*/
    SYMBOL *procedure;

    Accept(PROCEDURE);

    procedure = MakeSymbolTableEntry(STYPE_PROCEDURE, NULL);

    ParseVarOrProcName();

    backpatch_addr = CurrentCodeAddress();
    Emit(I_BR, 9999);

    procedure->address = CurrentCodeAddress();
    scope++;

    if (CurrentToken.code == LEFTPARENTHESIS)
    {
        ParseParameterList();
    }
    Accept(SEMICOLON);
    /* Synch SET 1 */
    Synchronise(&DeclarationFS_aug, &ProcDeclarationFBS);
    if (CurrentToken.code == VAR)
    {
        loc_flag = 1;
        ParseDeclarations(loc_flag);
    }
    if (varaddress > 0)
    {

        Emit(I_INC, varaddress);
    }
    /* Synch SET 2 */
    Synchronise(&ProcDeclarationFS_aug, &ProcDeclarationFBS);
    while (CurrentToken.code == PROCEDURE)
    {
        ParseProcDeclarations();
        /* resynch */
        Synchronise(&ProcDeclarationFS_aug, &ProcDeclarationFBS);
    }
    ParseBlock();
    Accept(SEMICOLON);

    /* cleanup */
    Emit(I_DEC, varaddress);
    _Emit(I_RET);
    BackPatch(backpatch_addr, CurrentCodeAddress());
    RemoveSymbols(scope);
    scope--;
}

/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*  Parser routines: Recursive-descent implementaion of the grammar's                                           */
/*                   productions.                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*  ParseParameterList implements:                                                                              */
/*                                                                                                              */
/*                                                                                                              */
/*      <ParseParameterList>            :==  "(" <ParserFormalParameter> { "," <ParserFormalParameter> } ")"    */
/*                                                                                                              */
/*                                                                                                              */
/*      Inputs:       None                                                                                      */
/*                                                                                                              */
/*      Outputs:      None                                                                                      */
/*                                                                                                              */
/*      Returns:      Nothing                                                                                   */
/*                                                                                                              */
/*      Side Effects: Lookahead token advanced.                                                                 */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/

PRIVATE void ParseParameterList(void)
{
    Accept(LEFTPARENTHESIS);
    ParseFormalParameter();
    while (CurrentToken.code == COMMA)
    {

        Accept(COMMA);
        ParseFormalParameter();
    }
    Accept(RIGHTPARENTHESIS);
}

/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*  Parser routines: Recursive-descent implementaion of the grammar's                                           */
/*                   productions.                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*  ParseFormalParameter implements:                                                                            */
/*                                                                                                              */
/*                                                                                                              */
/*      <ParseFormalParameter>          :==  ["REF"] <ParserVariable>                                           */
/*                                                                                                              */
/*                                                                                                              */
/*                                                                                                              */
/*      Inputs:       None                                                                                      */
/*                                                                                                              */
/*      Outputs:      None                                                                                      */
/*                                                                                                              */
/*      Returns:      Nothing                                                                                   */
/*                                                                                                              */
/*      Side Effects: Lookahead token advanced.                                                                 */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/

PRIVATE void ParseFormalParameter(void)
{
    if (CurrentToken.code == REF)
    {
        Accept(REF);
        MakeSymbolTableEntry(STYPE_REFPAR, &varaddress);
    }
    else
    {
        MakeSymbolTableEntry(STYPE_VALUEPAR, &varaddress);
    }

    ParseVariable();
}

/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*  Parser routines: Recursive-descent implementaion of the grammar's                                           */
/*                   productions.                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*  ParseBlock implements:                                                                                      */
/*                                                                                                              */
/*                                                                                                              */
/*      <ParseBlock>               :==  "BEGIN" {<ParseStatement> ";"} "END"                                    */
/*                                                                                                              */
/*                                                                                                              */
/*      Inputs:       None                                                                                      */
/*                                                                                                              */
/*      Outputs:      None                                                                                      */
/*                                                                                                              */
/*      Returns:      Nothing                                                                                   */
/*                                                                                                              */
/*      Side Effects: Lookahead token advanced.                                                                 */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/

PRIVATE void ParseBlock(void)
{
    int token;
    Accept(BEGIN);
    /* Synch SET */
    Synchronise(&StatementFS_aug, &StatementFBS);
    while ((token = CurrentToken.code) == IDENTIFIER || token == WHILE || token == IF || token == READ || token == WRITE)
    {
        ParseStatement();
        Accept(SEMICOLON);
        /* reSynch SET  */
        Synchronise(&StatementFS_aug, &StatementFBS);
    }
    Accept(END);
}

/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*  Parser routines: Recursive-descent implementaion of the grammar's                                           */
/*                   productions.                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*  ParseStatement implements:                                                                                  */
/*                                                                                                              */
/*                                                                                                              */
/*      <ParseStatement>           :==  <ParseSimpleStatement> | <ParseWhileStatement> |                        */
/*                                      <ParseifStatement> |<ParseReadStatement>|<ParseWriteStatement>          */
/*                                                                                                              */
/*                                                                                                              */
/*      Inputs:       None                                                                                      */
/*                                                                                                              */
/*      Outputs:      None                                                                                      */
/*                                                                                                              */
/*      Returns:      Nothing                                                                                   */
/*                                                                                                              */
/*      Side Effects: Lookahead token advanced.                                                                 */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/

PRIVATE void ParseStatement(void)
{

    switch (CurrentToken.code)
    {

    case IDENTIFIER:
        ParseSimpleStatement();
        break;
    case WHILE:
        ParseWhileStatement();
        break;
    case IF:
        ParseIfStatement();
        break;
    case READ:
        ParseReadStatement();
        break;
    case WRITE:
        ParseWriteStatement();
        break;
    default:
        break;
    }
}

/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*  Parser routines: Recursive-descent implementaion of the grammar's                                           */
/*                   productions.                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*  ParseSimpleStatement implements:                                                                            */
/*                                                                                                              */
/*                                                                                                              */
/*      <ParseSimpleStatement>     :== <ParseVarOrProcName> <ParseRestOfStatement>                              */
/*                                                                                                              */
/*                                                                                                              */
/*      Inputs:       None                                                                                      */
/*                                                                                                              */
/*      Outputs:      None                                                                                      */
/*                                                                                                              */
/*      Returns:      Nothing                                                                                   */
/*                                                                                                              */
/*      Side Effects: Lookahead token advanced. Creates Symbol token to store                                   */
/*                    identifier used in other functions                                                        */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/

PRIVATE void ParseSimpleStatement(void)
{
    SYMBOL *target;
    target = LookupSymbol();
    ParseVarOrProcName();

    ParseRestofStatement(target);
}

/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*  Parser routines: Recursive-descent implementaion of the grammar's                                           */
/*                   productions.                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*  ParseRestofStatement implements:                                                                            */
/*                                                                                                              */
/*      <ParseRestOfStatement>     :== <ParseProcCallList> | <ParseAssignment>| ε                               */
/*                                                                                                              */
/*      Inputs:       SYMBOL *target                                                                            */
/*                                                                                                              */
/*      Outputs:      1) If target type is Procedure the address of the target is called to the "CodeFile"      */
/*                    2) If target type is Variable the value of the code in "CodeFile" is stored               */
/*                         at the targets address                                                               */
/*                                                                                                              */
/*      Returns:      Nothing                                                                                   */
/*                                                                                                              */
/*      Side Effects: Lookahead token advanced.                                                                 */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/

PRIVATE void ParseRestofStatement(SYMBOL *target)
{
    int i;
    int dS;

    switch (CurrentToken.code)
    {
    case LEFTPARENTHESIS:
        ParseProcCallList(target);
    case SEMICOLON:
        if (target != NULL && target->type == STYPE_PROCEDURE)
        {
            _Emit(I_PUSHFP);
            _Emit(I_BSF);
            Emit(I_CALL, target->address);
            _Emit(I_RSF);
        }
        else
        {
            printf("Not a Procedure \n");
            KillCodeGeneration();
        }
        break;
    case ASSIGNMENT:
    default:
        ParseAssignment();
        if (target != NULL)
        {
            switch (target->type)
            {
            case STYPE_VARIABLE:
                Emit(I_STOREA, target->address);
                break;
            case STYPE_LOCALVAR:
                dS = scope - target->scope;
                if (dS == 0)
                {
                    Emit(I_STOREFP, target->address);
                }
                else
                {
                    _Emit(I_STOREFP);
                    for (i = 0; i < dS; i++)
                    {
                        _Emit(I_STORESP);
                    }
                    Emit(I_STORESP, target->address);
                }
                break;
            case STYPE_VALUEPAR:
                Emit(I_STOREA, target->address);
                break;
            case STYPE_REFPAR:
                dS = scope - target->scope;
                if (dS == 0)
                {
                    Emit(I_STOREFP, target->address);
                }
                else
                {

                    _Emit(I_STOREFP);
                    for (i = 0; i < dS; i++)
                    {
                        _Emit(I_STORESP);
                    }
                    Emit(I_STORESP, target->address);
                }
                break;
            }
        }
        else
        {
            Error("ERROR: UNDECLARED VARIABLE", CurrentToken.pos);
        }
        break;
    }
}

/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*  Parser routines: Recursive-descent implementaion of the grammar's                                           */
/*                   productions.                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*  ParseProcCallList implements:                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*      <ParseProcCallList>        :== "(" <ParseActualParameter> {","<ParseActualParameter>} ")"               */
/*                                                                                                              */
/*                                                                                                              */
/*      Inputs:       SYMBOL *target                                                                            */
/*                                                                                                              */
/*      Outputs:      None                                                                                      */
/*                                                                                                              */
/*      Returns:      Nothing                                                                                   */
/*                                                                                                              */
/*      Side Effects: Lookahead token advanced.                                                                 */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/

PRIVATE void ParseProcCallList(SYMBOL *target)
{
    Accept(LEFTPARENTHESIS);
    ParseActualParameter(0);
    while (CurrentToken.code == COMMA)
    {
        Accept(COMMA);
        ParseActualParameter(0);
    }
    Accept(RIGHTPARENTHESIS);
}

/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*  Parser routines: Recursive-descent implementaion of the grammar's                                           */
/*                   productions.                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*  ParseAssignment implements:                                                                                 */
/*                                                                                                              */
/*      <ParseAssignment>          :== ":==" <ParseExpression>                                                  */
/*                                                                                                              */
/*      Inputs:       None                                                                                      */
/*                                                                                                              */
/*      Outputs:      None                                                                                      */
/*                                                                                                              */
/*      Returns:      Nothing                                                                                   */
/*                                                                                                              */
/*      Side Effects: Lookahead token advanced.                                                                 */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/

PRIVATE void ParseAssignment(void)
{
    Accept(ASSIGNMENT);
    ParseExpression();
}

/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*  Parser routines: Recursive-descent implementaion of the grammar's                                           */
/*                   productions.                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*  ParseActualParameter implements:                                                                            */
/*                                                                                                              */
/*                                                                                                              */
/*      <ParseActualParameter>     :== <ParseVariable> | <ParseExpression>                                      */
/*                                                                                                              */
/*                                                                                                              */
/*      Inputs:       None                                                                                      */
/*                                                                                                              */
/*      Outputs:      None                                                                                      */
/*                                                                                                              */
/*      Returns:      Nothing                                                                                   */
/*                                                                                                              */
/*      Side Effects: Lookahead token advanced.                                                                 */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/

PRIVATE void ParseActualParameter(int isRef_flag)
{
    SYMBOL *var;

    var = LookupSymbol();

    if (isRef_flag)
        var->type = STYPE_REFPAR;
    else
        var->type = STYPE_VALUEPAR;

    ParseExpression();
}

/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*  Parser routines: Recursive-descent implementaion of the grammar's                                           */
/*                   productions.                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*  ParseWhileStatment implements:                                                                              */
/*                                                                                                              */
/*      <ParseWhileStatment>       :== "WHILE" <ParseBooleanExpression> "DO" <ParseBlock>                       */
/*                                                                                                              */
/*                                                                                                              */
/*      Inputs:       None                                                                                      */
/*                                                                                                              */
/*      Outputs:      None                                                                                      */
/*                                                                                                              */
/*      Returns:      Nothing                                                                                   */
/*                                                                                                              */
/*      Side Effects: Lookahead token advanced.                                                                 */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/

PRIVATE void ParseWhileStatement(void)
{
    Accept(WHILE);
    ParseBooleanExpression();
    Accept(DO);
    ParseBlock();
}

/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*  Parser routines: Recursive-descent implementaion of the grammar's                                           */
/*                   productions.                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*  ParseifStatement implements:                                                                                */
/*                                                                                                              */
/*      <ParseifStatement>         :== "IF" <ParseBooleanExpression> "THEN" <ParseBlock> ["ELSE" <ParseBlock>]  */
/*                                                                                                              */
/*                                                                                                              */
/*      Inputs:       None                                                                                      */
/*                                                                                                              */
/*      Outputs:      None                                                                                      */
/*                                                                                                              */
/*      Returns:      Nothing                                                                                   */
/*                                                                                                              */
/*      Side Effects: Lookahead token advanced.                                                                 */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/

PRIVATE void ParseIfStatement(void)
{
    Accept(IF);
    ParseBooleanExpression();
    Accept(THEN);
    ParseBlock();

    if (CurrentToken.code == ELSE)
    {
        Accept(ELSE);
        ParseBlock();
    }
}

/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*  Parser routines: Recursive-descent implementaion of the grammar's                                           */
/*                   productions.                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*  ParseReadStatement implements:                                                                              */
/*                                                                                                              */
/*      <ParseReadStatement>       :== "READ" "(" <ParseVariable> {"," <ParseVariable>} ")"                     */
/*                                                                                                              */
/*                                                                                                              */
/*      Inputs:       None                                                                                      */
/*                                                                                                              */
/*      Outputs:      None                                                                                      */
/*                                                                                                              */
/*      Returns:      Nothing                                                                                   */
/*                                                                                                              */
/*      Side Effects: Lookahead token advanced.                                                                 */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/

PRIVATE void ParseReadStatement(void)
{
    Accept(READ);
    Accept(LEFTPARENTHESIS);
    ParseVarOrProcName();
    while (CurrentToken.code == COMMA)
    {
        Accept(COMMA);
        ParseVarOrProcName();
    }
    Accept(RIGHTPARENTHESIS);
    _Emit(I_READ);
}

/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*  Parser routines: Recursive-descent implementaion of the grammar's                                           */
/*                   productions.                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*  ParseWriteStatement implements:                                                                             */
/*                                                                                                              */
/*      <ParseWriteStatement>      :== "WRITE" "(" <ParseExpression> {"," <ParseExpression>} ")"                */
/*                                                                                                              */
/*                                                                                                              */
/*      Inputs:       None                                                                                      */
/*                                                                                                              */
/*      Outputs:      None                                                                                      */
/*                                                                                                              */
/*      Returns:      Nothing                                                                                   */
/*                                                                                                              */
/*      Side Effects: Lookahead token advanced.                                                                 */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/

PRIVATE void ParseWriteStatement(void)
{
    Accept(WRITE);
    Accept(LEFTPARENTHESIS);
    ParseExpression();
    while (CurrentToken.code == COMMA)
    {
        Accept(COMMA);
        ParseExpression();
    }
    Accept(RIGHTPARENTHESIS);
    _Emit(I_WRITE);
}

/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*  Parser routines: Recursive-descent implementaion of the grammar's                                           */
/*                   productions.                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*  ParseExpression implements:                                                                                 */
/*                                                                                                              */
/*                                                                                                              */
/*      <ParseExpression>          :==  <ParseCompoundTerm> {("+"|"-") <ParseCompoundTerm>}                     */
/*                                                                                                              */
/*                                                                                                              */
/*      Inputs:       None                                                                                      */
/*                                                                                                              */
/*      Outputs:      1) If Add is processed from the "inputFile"  "ADD" is loaded to the "CodeFile"            */
/*                    2) if Subtract is processed from the "inputFile" "SUB" is loaded to the "CodeFile"        */
/*      Returns:      Nothing                                                                                   */
/*                                                                                                              */
/*      Side Effects: Lookahead token advanced.                                                                 */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/

PRIVATE void ParseExpression(void)
{
    int token;
    ParseCompoundTerm();
    while ((token = CurrentToken.code) == ADD || token == SUBTRACT)
    {

        switch (token)
        {
        case ADD:
            Accept(token);
            ParseCompoundTerm();
            _Emit(I_ADD);
            break;
        case SUBTRACT:
            Accept(token);
            ParseCompoundTerm();
            _Emit(I_SUB);
            break;
        }
    }
}

/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*  Parser routines: Recursive-descent implementaion of the grammar's                                           */
/*                   productions.                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*  ParseCompoundTerm implements:                                                                               */
/*                                                                                                              */
/*      <ParseCompoundTerm>        :==  <ParseTerm> {<ParseMultOp> <ParseTerm>}                                 */
/*                                                                                                              */
/*      Inputs:       None                                                                                      */
/*                                                                                                              */
/*      Outputs:      1) If a Multipicaltion is processed from the "InputFile" then "Mult" is added to          */
/*                    the "CodeFile"                                                                            */
/*                    2) If a Division is processed from the "InputFile" then "Div" is added tp the "CodeFile"  */
/*                                                                                                              */
/*      Returns:      Nothing                                                                                   */
/*                                                                                                              */
/*      Side Effects: Lookahead token advanced.                                                                 */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/

PRIVATE void ParseCompoundTerm(void)
{
    int token2;
    ParseTerm();
    while ((token2 = CurrentToken.code) == MULTIPLY || token2 == DIVIDE)
    {
        switch (token2)
        {
        case MULTIPLY:
            Accept(token2);
            ParseTerm();
            _Emit(I_MULT);
            break;
        case DIVIDE:
            Accept(token2);
            ParseTerm();
            _Emit(I_DIV);
            break;
        }
    }
}

/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*  Parser routines: Recursive-descent implementaion of the grammar's                                           */
/*                   productions.                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*  ParseTerm implements:                                                                                       */
/*                                                                                                              */
/*                                                                                                              */
/*      <ParseTerm>                :==  ["-"]  <ParseSubTerm>                                                   */
/*                                                                                                              */
/*                                                                                                              */
/*      Inputs:       None                                                                                      */
/*                                                                                                              */
/*      Outputs:      If a negitive value is processed from the "inputFile" Then a Neg is loaded to             */
/*                    the "CodeFile"                                                                            */
/*                                                                                                              */
/*      Returns:      Nothing                                                                                   */
/*                                                                                                              */
/*      Side Effects: Lookahead token advanced.                                                                 */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/

PRIVATE void ParseTerm(void)
{
    int TokenCheck = CurrentToken.code;
    if (CurrentToken.code == SUBTRACT)
        Accept(SUBTRACT);

    ParseSubTerm();
    if (TokenCheck == SUBTRACT)
    {
        _Emit(I_NEG);
        TokenCheck = 0;
    }
}

/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*  Parser routines: Recursive-descent implementaion of the grammar's                                           */
/*                   productions.                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*  ParseSubTerm implements:                                                                                    */
/*                                                                                                              */
/*                                                                                                              */
/*      <ParseSubTerm>             :==  <ParseVariable> | <ParseIntConst> | "(" <ParseExpression> ")"           */
/*                                                                                                              */
/*                                                                                                              */
/*      Inputs:       None                                                                                      */
/*                                                                                                              */
/*      Outputs:      1) if IDENTIFIER: Loads the  pre-declared identifier to "codeFile"                        */
/*                    2) if INCONST: Loads the constant to the "codeFile"                                       */
/*                                                                                                              */
/*      Returns:      Nothing                                                                                   */
/*                                                                                                              */
/*      Side Effects: Lookahead token advanced.                                                                 */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/

PRIVATE void ParseSubTerm(void)
{

    int i, dS;

    SYMBOL *var;

    switch (CurrentToken.code)
    {
    case INTCONST:
        Emit(I_LOADI, CurrentToken.value);
        Accept(INTCONST);
        break;
    case LEFTPARENTHESIS:
        Accept(LEFTPARENTHESIS);
        ParseExpression();
        Accept(RIGHTPARENTHESIS);
        break;
    case IDENTIFIER:
    default:
        var = LookupSymbol();
        if (var != NULL)
        {

            switch (var->type)
            {

            case STYPE_VARIABLE:
                Emit(I_LOADA, var->address);
                break;
            case STYPE_LOCALVAR:
                dS = scope - var->scope;
                if (dS == 0)
                    Emit(I_LOADFP, var->address);
                else
                {
                    _Emit(I_LOADFP);
                    for (i = 0; i < dS; i++)
                    {
                        _Emit(I_LOADSP);
                    }
                    Emit(I_LOADSP, var->address);
                }
                break;
            case STYPE_VALUEPAR:
                Emit(I_LOADA, var->address);
                break;
            case STYPE_REFPAR:
                dS = scope - var->scope;
                if (dS == 0)
                    Emit(I_LOADI, var->address);
                else
                {

                    _Emit(I_LOADFP);
                    for (i = 0; i < dS; i++)
                    {
                        _Emit(I_LOADSP);
                    }
                    Emit(I_LOADSP, var->address);
                }
                break;
            }
        }
        else
        {
            printf("Error: undeclared variable");
            KillCodeGeneration();
        }
        Accept(IDENTIFIER);

        break;
    }
}

/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*  Parser routines: Recursive-descent implementaion of the grammar's                                           */
/*                   productions.                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*  ParseBooleanExpression implements:                                                                          */
/*                                                                                                              */
/*                                                                                                              */
/*      <ParseBooleanExpression>   :==  <ParseExpression> <ParseRelOp> <ParseExpression>                        */
/*                                                                                                              */
/*                                                                                                              */
/*      Inputs:       None                                                                                      */
/*                                                                                                              */
/*      Outputs:      None                                                                                      */
/*                                                                                                              */
/*      Returns:      Nothing                                                                                   */
/*                                                                                                              */
/*      Side Effects: Lookahead token advanced.                                                                 */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/

PRIVATE void ParseBooleanExpression(void)
{
    ParseExpression();
    ParseRelOp();
    ParseExpression();
}

/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*  Parser routines: Recursive-descent implementaion of the grammar's                                           */
/*                   productions.                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*  ParseRelOp implements:                                                                                      */
/*                                                                                                              */
/*                                                                                                              */
/*      <ParseRelOp>               :==  "=" | "<=" | ">=" | "<" | ">"                                           */
/*                                                                                                              */
/*                                                                                                              */
/*      Inputs:       None                                                                                      */
/*                                                                                                              */
/*      Outputs:      None                                                                                      */
/*                                                                                                              */
/*      Returns:      Nothing                                                                                   */
/*                                                                                                              */
/*      Side Effects: Lookahead token advanced.                                                                 */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/

PRIVATE void ParseRelOp(void)
{
    switch (CurrentToken.code)
    {
    case EQUALITY:
        Accept(EQUALITY);
        break;
    case LESSEQUAL:
        Accept(LESSEQUAL);
        break;
    case GREATEREQUAL:
        Accept(GREATEREQUAL);
        break;
    case LESS:
        Accept(LESS);
        break;
    case GREATER:
        Accept(GREATER);
        break;
    default:
        break;
    }
}

/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*  Parser routines: Recursive-descent implementaion of the grammar's                                           */
/*                   productions.                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*  ParseVariable implements:                                                                                   */
/*                                                                                                              */
/*      <ParseVariable>            :==  <Identifier>                                                            */
/*                                                                                                              */
/*      Inputs:       None                                                                                      */
/*                                                                                                              */
/*      Outputs:      Nothing but Note side effect                                                              */
/*                                                                                                              */
/*      Returns:      Nothing                                                                                   */
/*                                                                                                              */
/*      Side Effects: Lookahead token advanced. Gives variable to the MakeSymbolTableEntry to be added to       */
/*                    "codeFile" if it is a new entry                                                           */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/

PRIVATE void ParseVariable(void)
{
    /* MakeSymbolTableEntry(STYPE_VARIABLE, &varaddress); */
    Accept(IDENTIFIER);
}

/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*  Parser routines: Recursive-descent implementaion of the grammar's                                           */
/*                   productions.                                                                               */
/*                                                                                                              */
/*                                                                                                              */
/*  ParseVarOrProcName implements:                                                                              */
/*                                                                                                              */
/*                                                                                                              */
/*      <ParseVarOrProcName>       :==  <Identifier>                                                            */
/*                                                                                                              */
/*                                                                                                              */
/*      Inputs:       None                                                                                      */
/*                                                                                                              */
/*      Outputs:      None                                                                                      */
/*                                                                                                              */
/*      Returns:      Nothing                                                                                   */
/*                                                                                                              */
/*      Side Effects: Lookahead token advanced.                                                                 */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/

PRIVATE void ParseVarOrProcName(void)
{

    Accept(IDENTIFIER);
}
/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  End of parser.  Support routines follow.                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Accept:  Takes an expected token name as argument, and if the current   */
/*           lookahead matches this, advances the lookahead and returns.    */
/*                                                                          */
/*           If the expected token fails to match the current lookahead,    */
/*           this routine reports a syntax error and exits ("crash & burn"  */
/*           parsing).  Note the use of routine "SyntaxError"               */
/*           (from "scanner.h") which puts the error message on the         */
/*           standard output and on the listing file, and the helper        */
/*           "ReadToEndOfFile" which just ensures that the listing file is  */
/*           completely generated.                                          */
/*                                                                          */
/*                                                                          */
/*    Inputs:       Integer code of expected token                          */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: If successful, advances the current lookahead token     */
/*                  "CurrentToken".                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void Accept(int ExpectedToken)
{
    static int recovering = 0;

    if (recovering)
    {
        while (CurrentToken.code != ExpectedToken && CurrentToken.code != ENDOFINPUT)
            CurrentToken = GetToken();
        recovering = 0;
    }

    if (CurrentToken.code != ExpectedToken)
    {

        SyntaxError(ExpectedToken, CurrentToken);
        recovering = 1;
        ErrorFlag = 1;
    }
    else
        CurrentToken = GetToken();
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  OpenFiles:  Reads strings from the command-line and opens the           */
/*              associated input and listing files.                         */
/*                                                                          */
/*    Note that this routine mmodifies the globals "InputFile" and          */
/*    "ListingFile".  It returns 1 ("true" in C-speak) if the input and     */
/*    listing files are successfully opened, 0 if not, allowing the caller  */
/*    to make a graceful exit if the opening process failed.                */
/*                                                                          */
/*                                                                          */
/*    Inputs:       1) Integer argument count (standard C "argc").          */
/*                  2) Array of pointers to C-strings containing arguments  */
/*                  (standard C "argv").                                    */
/*                                                                          */
/*    Outputs:      No direct outputs, but note side effects.               */
/*                                                                          */
/*    Returns:      Boolean success flag (i.e., an "int":  1 or 0)          */
/*                                                                          */
/*    Side Effects: If successful, modifies globals "InputFile" and         */
/*                  "ListingFile".                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE int OpenFiles(int argc, char *argv[])
{

    if (argc != 4)
    {
        fprintf(stderr, "%s <inputfile> <listfile> <CodeFile>\n", argv[0]);
        return 0;
    }

    if (NULL == (InputFile = fopen(argv[1], "r")))
    {
        fprintf(stderr, "cannot open \"%s\" for input\n", argv[1]);
        return 0;
    }

    if (NULL == (ListFile = fopen(argv[2], "w")))
    {
        fprintf(stderr, "cannot open \"%s\" for output\n", argv[2]);
        fclose(InputFile);
        return 0;
    }
    if (NULL == (CodeFile = fopen(argv[3], "w")))
    {
        fprintf(stderr, "cannot open \"%s\" for output\n", argv[2]);
        fclose(InputFile);
        return 0;
    }

    return 1;
}

/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*  SetupSets: This function is used to initialise the code necessary for augmented S-Algol error recovery      */
/*             Each of the global set variables are initialised using the initSet() function provided           */
/*             These sets are then used to synchronise an incorrect token and allow the parser to somewhat      */
/*             continue parsing.                                                                                */
/*                                                                                                              */
/*                                                                                                              */
/*  Setup Sets implements:                                                                                      */
/*                                                                                                              */
/*                                                                                                              */
/*      Inputs:       None                                                                                      */
/*                                                                                                              */
/*      Outputs:      None                                                                                      */
/*                                                                                                              */
/*      Returns:      Nothing                                                                                   */
/*                                                                                                              */
/*      Side Effects: Sets initialized                                                                          */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/

PRIVATE void SetupSets(void)
{
    /* init for Program */
    /* SET 1 */
    InitSet(&DeclarationFS_aug, 3, VAR, PROCEDURE, BEGIN);
    /* SET 2 */
    InitSet(&ProcDeclarationFS_aug, 2, PROCEDURE, BEGIN);

    InitSet(&ProcDeclarationFBS, 3, ENDOFINPUT, ENDOFPROGRAM, END);

    /* Init for statement found in block*/
    InitSet(&StatementFS_aug, 6, IDENTIFIER, WHILE, IF, READ, WRITE, END);

    InitSet(&StatementFBS, 4, SEMICOLON, ELSE, ENDOFPROGRAM, ENDOFINPUT);
}

/*--------------------------------------------------------------------------------------------------------------*/
/*                                                                                                              */
/*  Synchronise:  This function is called to check if the tokens are still synchronised with the current        */
/*                If a token in the set cannot be found it will send an error message.                          */
/*                                                                                                              */
/*                This is a powerful function as it allows you to skip over false tokens and resynchronise the  */
/*                parser on a token that is required by the syntax. This allows us to continue parsing and      */
/*                somewhat recover from the error.                                                              */
/*                                                                                                              */
/*  Synchronise implements:                                                                                     */
/*                                                                                                              */
/*                                                                                                              */
/*      Inputs:       Takes 2 sets declared in SetupSets                                                        */
/*                    1) Augmented first set                                                                    */
/*                    2) Follow U Beacons                                                                       */
/*                                                                                                              */
/*      Outputs:      None                                                                                      */
/*                                                                                                              */
/*      Returns:      Nothing                                                                                   */
/*                                                                                                              */
/*      Side Effects: Skips tokens until token within set is matched                                            */
/*                                                                                                              */
/*--------------------------------------------------------------------------------------------------------------*/

PRIVATE void Synchronise(SET *F, SET *FB)
{

    SET S;
    S = Union(2, F, FB);
    if (!InSet(F, CurrentToken.code))
    {
        SyntaxError2(*F, CurrentToken);
        while (!InSet(&S, CurrentToken.code))
        {
            CurrentToken = GetToken();
        }
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  MakeSymbolTableEntry:                                                   */
/*                                                                          */
/*                                                                          */
/*    Note that this routine mmodifies the globals "CodeFile"               */
/*    This function takes input of symbol type                              */
/*    The function checks the Current Token code if its an identifier       */
/*    Check are made to make sure the string                                */
/*    has not been added to the CodeFile before.                            */
/*    If it hasn't then this string is added as a new entry                 */
/*                                                                          */
/*                                                                          */
/*    Inputs:       1)  Symbol Type                                         */
/*                                                                          */
/*                                                                          */
/*    Outputs:      No direct outputs, but note side effects.               */
/*                                                                          */
/*    Returns:      Returns new symbol                                      */
/*                                                                          */
/*    Side Effects: If successful, modifies globals  "CodeFile"             */
/*                                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE SYMBOL *MakeSymbolTableEntry(int symtype, int *varaddress)
{

    SYMBOL *oldsptr, *newsptr = NULL;
    char *cptr;
    int hashindex;

    if (CurrentToken.code == IDENTIFIER)
    {
        if (NULL == (oldsptr = Probe(CurrentToken.s, &hashindex)) || oldsptr->scope < scope)
        {

            if (oldsptr == NULL)
                cptr = CurrentToken.s;
            else
                cptr = oldsptr->s;
            if (NULL == (newsptr = EnterSymbol(cptr, hashindex)))
            {
                printf("Error: SYMBOL ENTRY FAILED\n");
                KillCodeGeneration();
            }
            else
            {

                if (oldsptr == NULL)
                    PreserveString();
                newsptr->scope = scope;
                newsptr->type = symtype;
                if (symtype == STYPE_VARIABLE || symtype == STYPE_LOCALVAR || symtype == STYPE_REFPAR)
                {
                    newsptr->address = *varaddress;
                    (*varaddress)++;
                }
                else
                    newsptr->address = -1;
                hashindex++;
            }
        }
        else
        {

            Error("ERROR IN SYMBOL CREATION :::::", CurrentToken.pos);
            KillCodeGeneration();
        }
    }
    return newsptr;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  LookupSymbol:                                                           */
/*                                                                          */
/*                                                                          */
/*     The current token string is checked to make sure it is an identifier */
/*    The currentToken string is searched for on the "codeFile"             */
/*    if the Identifer is not found An error is thrown                      */
/*    Else the string is returned                                           */
/*                                                                          */
/*                                                                          */
/*    Inputs:       1)  Symbol Type                                         */
/*                                                                          */
/*                                                                          */
/*    Outputs:      If Error occurs ,"Identifier not declared"              */
/*                  and the code Generated is killed                        */
/*                                                                          */
/*    Returns:      Symbol found with matching identifer                    */
/*                                                                          */
/*    Side Effects: If successful, modifies globals  "CodeFile"             */
/*                                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE SYMBOL *LookupSymbol(void)
{
    SYMBOL *sptr;
    if (CurrentToken.code == IDENTIFIER)
    {
        sptr = Probe(CurrentToken.s, NULL);
        if (sptr == NULL)
        {
            Error("Identifier not declared", CurrentToken.pos);
            KillCodeGeneration();
        }
    }
    else
        sptr = NULL;
    return sptr;
}
