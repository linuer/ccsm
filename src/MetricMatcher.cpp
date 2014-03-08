/*
   Copyright 2014 John Bailey

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <sstream>
#include <iostream>

#include "MetricMatcher.hpp"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"
#include "clang/AST/ASTContext.h"

using namespace clang;
using namespace std;

//#define DEBUG_FN_TRACE_OUTOUT 1

MetricVisitor::MetricVisitor(clang::ASTContext* p_Context, MetricUnit* p_topUnit, Options* p_options) : astContext(p_Context), m_topUnit( p_topUnit ), m_currentUnit( NULL ), m_options( p_options )
{
}

MetricVisitor::~MetricVisitor(void)
{
}

bool MetricVisitor::ShouldIncludeFile( const std::string& p_fn ) const
{
	bool ret_val = true;
	if (( m_options != NULL ) &&
		( m_options->ExcludeFiles != NULL ))
	{	
		for( std::vector<std::string>::const_iterator it = m_options->ExcludeFiles->begin();
			 it != m_options->ExcludeFiles->end();
			 it++ )
		{
			if( p_fn.find( *it ) !=  string::npos )
			{
				ret_val = false;
				break;
			}
		}
	}
	return ret_val;
}

bool MetricVisitor::ShouldIncludeFunction( const std::string& p_fn ) const
{
	return (( m_options == NULL ) ||
			( m_options->ExcludeFunctions == NULL ) ||
			( std::find( m_options->ExcludeFunctions->begin(), m_options->ExcludeFunctions->end(), p_fn ) == m_options->ExcludeFunctions->end() ));
}

bool MetricVisitor::VisitFunctionDecl(clang::FunctionDecl *func) {
    
#if defined( DEBUG_FN_TRACE_OUTOUT )
	std::cout << "VisitFunctionDecl" << std::endl;
#endif
	if( func->doesThisDeclarationHaveABody() )
	{
		m_currentFileName = astContext->getSourceManager().getFilename( func->getLocation() ).str();
		m_currentFunctionName = func->getQualifiedNameAsString();

#if defined( DEBUG_FN_TRACE_OUTOUT )
		std::cout << "VisitFunctionDecl - " << m_currentFileName << "::"<< m_currentFunctionName << std::endl;
#endif

		if( ShouldIncludeFile( m_currentFileName ) && ShouldIncludeFunction( m_currentFunctionName ))
		{
#if defined( DEBUG_FN_TRACE_OUTOUT )
			std::cout << "VisitFunctionDecl - Should be included" << std::endl;
#endif
			MetricUnit* fileUnit = m_topUnit->getSubUnit(m_currentFileName, METRIC_UNIT_FILE);

			MetricUnitType_e type = METRIC_UNIT_FUNCTION;

			if( func->isCXXClassMember() )
			{
				type = METRIC_UNIT_METHOD;
			}
		
			m_currentUnit = fileUnit->getSubUnit(m_currentFunctionName, type);

			if( func->getLinkageAndVisibility().getLinkage() == InternalLinkage )
			{
				fileUnit->increment( METRIC_TYPE_LOCAL_FUNCTIONS );
			}
			fileUnit->increment( METRIC_TYPE_FUNCTIONS );
		}
		else
		{
			m_currentUnit = NULL;
		}
	}
	return true;     
}     

bool MetricVisitor::VisitVarDecl(clang::VarDecl *p_varDec) {

#if defined( DEBUG_FN_TRACE_OUTOUT )
	std::cout << "VisitVarDecl : CONTEXT " << m_currentFileName << "::" << m_currentFunctionName << std::endl;
#endif

	/* Check it's not something like a function parameter */
	if( p_varDec->getKind() == clang::Decl::Var )
	{
		/* Check to see if this variable does not have a parent function/method */
		if( !p_varDec->getParentFunctionOrMethod() )
		{
			/* Don't count external references */
			if ( !p_varDec->hasExternalStorage() )
			{

				SourceLocation loc = p_varDec->getLocation();
				if( loc.isMacroID() )
				{
					loc = astContext->getSourceManager().getFileLoc(loc);
				}

				/* File-scope variable */
				m_currentFileName = astContext->getSourceManager().getFilename( loc ).str();
				m_currentFunctionName = "";
				if( ShouldIncludeFile( m_currentFileName ))
				{
#if defined( DEBUG_FN_TRACE_OUTOUT )
				std::cout << "VisitVarDecl : Processing " << p_varDec->getNameAsString() << std::endl;
#endif
					m_currentUnit = m_topUnit->getSubUnit(m_currentFileName, METRIC_UNIT_FILE);
					m_currentUnit->increment( METRIC_TYPE_VARIABLE );
				}
				else
				{
					m_currentUnit = NULL;
				}
			}
		}

	}
	return true;	
}

// TODO: Lots of repetition here - template it?
bool MetricVisitor::VisitForStmt(clang::ForStmt *p_forSt) {
	if( m_currentUnit )
	{
		m_currentUnit->increment( METRIC_TYPE_FORLOOP );
	}
    return true;
}

bool MetricVisitor::VisitGotoStmt(clang::GotoStmt *p_gotoSt) {
	if( m_currentUnit )
	{
		m_currentUnit->increment( METRIC_TYPE_GOTO );
	}

    return true;
}

bool MetricVisitor::VisitLabelStmt(clang::LabelStmt *p_LabelSt) {
	if( m_currentUnit )
	{
		m_currentUnit->increment( METRIC_TYPE_LABEL );
	}
    return true;
}

bool MetricVisitor::VisitWhileStmt(clang::WhileStmt *p_whileSt) {
	if( m_currentUnit )
	{
		m_currentUnit->increment( METRIC_TYPE_WHILELOOP );
	}
    return true;
}

bool MetricVisitor::VisitReturnStmt(clang::ReturnStmt *p_returnSt) {
	if( m_currentUnit )
	{
		m_currentUnit->increment( METRIC_TYPE_RETURN );

		if( !isLastExecutableStmtInFn( p_returnSt, astContext ) )
		{
			m_currentUnit->increment( METRIC_TYPE_RETURNPOINTS );
		}
	}
    return true;
}

bool MetricVisitor::VisitCallExpr(clang::CallExpr *p_callExpr)
{
	if( m_currentUnit )
	{
		if( p_callExpr->getCalleeDecl()->getAsFunction()->getBody() != NULL )
		{
			m_currentUnit->increment( METRIC_TYPE_LOCAL_FUNCTION_CALLS );
		}
		m_currentUnit->increment( METRIC_TYPE_FUNCTION_CALLS );
	}
    return true;
}

bool MetricVisitor::VisitSwitchStmt(clang::SwitchStmt *p_switchSt) {
	if( m_currentUnit )
	{
		m_currentUnit->increment( METRIC_TYPE_SWITCH );
	}
    return true;
}

bool MetricVisitor::VisitConditionalOperator(clang::ConditionalOperator *p_condOp) {
#if defined( DEBUG_FN_TRACE_OUTOUT )
	std::cout << "VisitConditionalOperator : " << m_currentFileName << "::" << m_currentFunctionName << std::endl;
#endif
	if( m_currentUnit )
	{
		m_currentUnit->increment( METRIC_TYPE_TERNARY );
	}
    return true;
}

bool MetricVisitor::VisitDefaultStmt(clang::DefaultStmt *p_defaultSt) {
	if( m_currentUnit )
	{
		m_currentUnit->increment( METRIC_TYPE_DEFAULT );
	}
    return true;
}

bool MetricVisitor::VisitCaseStmt(clang::CaseStmt *p_caseSt) {
	if( m_currentUnit )
	{
		m_currentUnit->increment( METRIC_TYPE_CASE );
	}
    return true;
}

bool MetricVisitor::VisitBinaryOperator(clang::BinaryOperator *p_binOp) {
	if( m_currentUnit )
	{
		switch( p_binOp->getOpcode() )
		{
			case clang::BO_LAnd:
				m_currentUnit->increment( METRIC_TYPE_LOGICAL_AND );
				break;
			case clang::BO_LOr:
				m_currentUnit->increment( METRIC_TYPE_LOGICAL_OR );
				break;
			default:
				break;
		}
	}
    return true;
}


bool MetricVisitor::VisitStmt(clang::Stmt *p_statement) {

	if( m_currentUnit )
	{
		m_currentUnit->increment( METRIC_TYPE_STATEMENTS );
	}
	return true;
}


bool MetricVisitor::VisitIfStmt(clang::IfStmt *p_ifSt) {
#if defined( DEBUG_FN_TRACE_OUTOUT )
	std::cout << "VisitIfStmt : CONTEXT " << m_currentFileName << "::" << m_currentFunctionName << std::endl;
#endif

	if( m_currentUnit )
	{
#if defined( DEBUG_FN_TRACE_OUTOUT )
	std::cout << "VisitIfStmt - Recorded" << std::endl;
#endif
		m_currentUnit->increment( METRIC_TYPE_IF );

		if( p_ifSt->getElse() )
		{
			// TODO: This means that "else if" statements get counted as both an IF and an ELSE, which may not be what everyone wants
			m_currentUnit->increment( METRIC_TYPE_ELSE );
		}
	} else {
		/* TODO */
	}

    return true;
}

void MetricVisitor::dump( std::ostream& out, const bool p_output[ METRIC_UNIT_MAX ], const MetricDumpFormat_e p_fmt )
{
	m_topUnit->dump( out, p_output, p_fmt );
}

MetricASTConsumer::MetricASTConsumer(clang::ASTContext *CI, MetricUnit* p_topUnit, MetricVisitor::Options* p_options ) : visitor(new MetricVisitor(CI, p_topUnit, p_options)) 
{ 
}

MetricASTConsumer::~MetricASTConsumer(void) 
{
};

void MetricASTConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
	/* we can use ASTContext to get the TranslationUnitDecl, which is
		a single Decl that collectively represents the entire source file */
	visitor->TraverseDecl(Context.getTranslationUnitDecl());
}

void MetricASTConsumer::dump( std::ostream& out, const bool p_output[ METRIC_UNIT_MAX ], const MetricDumpFormat_e p_fmt )
{
	visitor->dump( out, p_output, p_fmt );
}