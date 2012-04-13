// This code is in the public domain. See LICENSE for details.

// expression parser/evaluator

#ifndef __fr_expression_h_
#define __fr_expression_h_

#include "types.h"
#include "tool.h"

namespace fr
{
	class expressionParser: public noncopyable
	{
	public:
		enum
		{
			ok=0,
			syntaxError=-1,
			noExpression=-2,
			junkAfterEnd=-3,
			divisionByZero=-4,
			unknownIdentifier=-5,
		};

		expressionParser();
		~expressionParser();

		sInt evaluate(const sChar* expression, sF64& result);
		void clearVariables();

	private:
		struct savedToken
		{
			sInt		token;
			string	string_value;
			sF64    number;
		};

    struct privateData;

		enum
		{
			_EOF=256,
			MULMUL=257,
			NUMBER=258,
			IDENTIFIER=259,
			SYNTAX_ERROR=512,
			NUMBER_TOO_LONG=513,
			IDENTIFIER_TOO_LONG=514,
		};

    privateData*    m_priv;
		const sChar*		m_expression, *m_expPtr, *m_oldExpPtr;
		sBool      			m_endFlag;
		sInt						m_currentToken;
		string					m_identifier;
		savedToken			m_savedToken;
		sBool						m_hasSavedToken;
		sF64						m_value;

		sChar getNextChar();
		void putCharBack();
		void saveToken();
		void processSavedToken(sBool seekBack);
		sInt getToken();
		sInt evalInput(sF64& result);
		sInt evalAssignExpression(sF64& result);
		sInt evalExpression(sF64& result);
		sInt evalTerm(sF64& result);
		sInt evalFactor(sF64& result);
		sInt evalValue(sF64& result);
		sInt evalAtom(sF64& result);
	};
}

#endif
