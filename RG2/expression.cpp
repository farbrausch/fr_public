// This code is in the public domain. See LICENSE for details.

// expression parser/evaluator source file

#include "stdafx.h"
#include "expression.h"
#include "types.h"
#include "tool.h"
#include <math.h>
#include <map>
#include "debug.h"

struct stringLessHelper
{
  bool operator()(const fr::string& str1, const fr::string& str2) const
  {
    return str1.compare(str2, sTRUE) < 0;
  }
};

namespace fr
{
  // ---- expressionParser

  struct expressionParser::privateData
  {
    typedef std::map<string, sF64, stringLessHelper>  varMap;
    typedef varMap::iterator											    varMapIt;

    varMap m_variables;
  };

  expressionParser::expressionParser()
  {
    m_priv = new privateData;

    clearVariables();
  }

  expressionParser::~expressionParser()
  {
    delete m_priv;
    m_priv = 0;
  }

  sInt expressionParser::evaluate(const sChar* expression, sF64& result)
  {
    m_expression = expression;
    m_expPtr = m_expression;
    m_endFlag = sFALSE;
    m_hasSavedToken = sFALSE;

    if (getToken() == _EOF)
      return noExpression;

    return evalInput(result);
  }

  void expressionParser::clearVariables()
	{
		m_priv->m_variables.clear();
    m_priv->m_variables["pi"] = 4 * atan(1.0);
    m_priv->m_variables["e"] = ::exp(1.0);
  }

  sChar expressionParser::getNextChar()
  {
    if (m_endFlag)
      return 0;

    sChar res = *m_expPtr++;
    m_endFlag = (res == 0);

    return res;
  }

  void expressionParser::putCharBack()
  {
    FRDASSERT(m_expPtr > m_expression);

    --m_expPtr;
  }

  void expressionParser::saveToken()
  {
    FRDASSERT(!m_hasSavedToken);

    m_hasSavedToken = sTRUE;
    m_savedToken.token = m_currentToken;
    m_savedToken.string_value = m_identifier;
    m_savedToken.number = m_value;
  }

  void expressionParser::processSavedToken(sBool seekBack)
  {
    FRDASSERT(m_hasSavedToken);

    m_hasSavedToken = sFALSE;

    if (seekBack)
    {
      m_currentToken = m_savedToken.token;
      m_identifier = m_savedToken.string_value;
      m_value = m_savedToken.number;
      m_expPtr = m_oldExpPtr;
    }
  }

  sInt expressionParser::getToken()
  {
    m_oldExpPtr = m_expPtr;

    while (1)
    {
      sChar ch = getNextChar();

      switch (ch)
      {
      case 0:
        return (m_currentToken = _EOF);

      case ' ': case '\t': case '\r': case '\n':
        break;

      case '*':
        if (getNextChar() == '*')
          return (m_currentToken = MULMUL);
        else
          putCharBack();

        return (m_currentToken = ch);

      case '+': case '-': case '/': case '(': case ')': case '=': case ',': case ';':
        return (m_currentToken = ch);
      }

      if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_')
      {
        sChar idBuf[513], *idPtr = idBuf;

        *idPtr++ = ch;

        while (1)
        {
          if (idPtr - idBuf > 512)
            return (m_currentToken = IDENTIFIER_TOO_LONG);

          ch = getNextChar();

          if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_')
            *idPtr++ = ch;
          else
          {
            putCharBack();
            break;
          }
        }

        *idPtr++ = 0;
        m_identifier = idBuf;

        return (m_currentToken = IDENTIFIER);
      }

      if ((ch >= '0' && ch <= '9') || ch == '.')
      {
        sChar numBuf[259], *numBufPtr = numBuf;
        sBool pointSeen = (ch == '.'), expSeen = sFALSE;

        *numBufPtr++ = ch;

        while (1)
        {
          if (numBufPtr - numBuf > 255)
            return (m_currentToken = NUMBER_TOO_LONG);

          ch = getNextChar();

          if ((ch == '.' && !pointSeen) || (ch >= '0' && ch <= '9') || ((ch == 'e' || ch == 'E') && !expSeen))
          {
            *numBufPtr++ = ch;
            pointSeen |= (ch == '.');

            if (ch == 'e' || ch == 'E')
            {
              expSeen = sTRUE;

              ch = getNextChar();
              if (ch == '+' || ch == '-')
                *numBufPtr++ = ch;
              else
                return (m_currentToken = SYNTAX_ERROR);

              ch = getNextChar();
              if (ch >= '0' && ch <= '9')
                *numBufPtr++ = ch;
              else
                return (m_currentToken = SYNTAX_ERROR);
            }
          }
          else
          {
            putCharBack();
            break;
          }
        }

        *numBufPtr++ = 0;
        m_value = atof(numBuf);

        return (m_currentToken = NUMBER);
      }

      return (m_currentToken = SYNTAX_ERROR);
    }
  }

  sInt expressionParser::evalInput(sF64& result)
  {
    while (1)
    {
      if (m_currentToken == _EOF)
        break;

      result = 0;
      sInt res = evalAssignExpression(result);
      if (res != ok)
        return res;

      if (m_currentToken != ';')
        break;
      else
        getToken();
    }

    if (m_currentToken != _EOF)
      return junkAfterEnd;

    return ok;
  }

  sInt expressionParser::evalAssignExpression(sF64& result)
  {
    if (m_currentToken == IDENTIFIER)
    {
      string varName = m_identifier;

      saveToken();

      if (getToken() == '=')
      {
        processSavedToken(sFALSE);

        getToken();
        sInt res = evalAssignExpression(result);
        if (res != ok)
          return res;

        m_priv->m_variables[varName] = result;
        return ok;
      }
      else
        processSavedToken(sTRUE);
    }

    return evalExpression(result);
  }

  sInt expressionParser::evalExpression(sF64& result)
  {
    sInt res = evalTerm(result);
    if (res != ok)
      return res;

    sInt op;

    while ((op = m_currentToken) == '+' || op == '-')
    {
      getToken();

      sF64 temp;
      res = evalTerm(temp);
      if (res != ok)
        return res;

      if (op == '+')
        result += temp;
      else
        result -= temp;
    }

    return ok;
  }

  sInt expressionParser::evalTerm(sF64& result)
  {
    sInt res = evalFactor(result);
    if (res != ok)
      return res;

    sInt op;

    while ((op = m_currentToken) == '*' || op == '/')
    {
      getToken();

      sF64 temp;
      res = evalFactor(temp);
      if (res != ok)
        return res;

      if (op == '*')
        result *= temp;
      else if (op == '/')
      {
        if (temp != 0)
          result /= temp;
        else
        {
          result = 0;
          return divisionByZero;
        }
      }
    }

    return ok;
  }

  sInt expressionParser::evalFactor(sF64& result)
  {
    sInt res = evalValue(result);
    if (res != ok)
      return res;

    while (m_currentToken == MULMUL)
    {
      getToken();

      sF64 temp;
      res = evalFactor(temp);
      if (res != ok)
        return res;

      if (temp < 0 && result == 0.0f)
        return divisionByZero;
      else
        result = pow(result, temp);
    }

    return ok;
  }

  sInt expressionParser::evalValue(sF64& result)
  {
    sF64 sign = 1;

    if (m_currentToken == '+' || m_currentToken == '-')
    {
      if (m_currentToken == '-')
        sign = -1;

      getToken();
    }

    sInt res = evalAtom(result);
    if (res != ok)
      return res;

    result *= sign;
    return ok;
  }

  sInt expressionParser::evalAtom(sF64& result)
  {
    switch (m_currentToken)
    {
    case IDENTIFIER:
      {
        privateData::varMapIt it = m_priv->m_variables.find(m_identifier);

        if (it != m_priv->m_variables.end())
          result = it->second;
        else
          return unknownIdentifier;
      }
      break;

    case NUMBER:
      result = m_value;
      break;

    case '(':
      {
        getToken();

        sInt res = evalAssignExpression(result);
        if (res != ok)
          return res;

        if (m_currentToken != ')')
          return syntaxError;
      }
      break;

    default:
      return syntaxError;
    }

    getToken();
    return ok;
  }
}
