/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/intl.cpp
// Purpose:     Internationalization and localisation for wxWidgets
// Author:      Vadim Zeitlin
// Modified by: Michael N. Filippov <michael@idisys.iae.nsk.su>
//              (2003/09/30 - PluralForms support)
// Created:     29/01/98
// RCS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include <ctype.h>

#include "pl_evaluate.h"

// ----------------------------------------------------------------------------
// Plural forms parser
// ----------------------------------------------------------------------------


PluralFormsScanner::PluralFormsScanner(const char* s) : m_s(s)
{
    nextToken();
}

bool PluralFormsScanner::nextToken()
{
    PluralFormsToken::Type type = PluralFormsToken::T_ERROR;
    while (isspace(*m_s))
    {
        ++m_s;
    }
    if (*m_s == 0)
    {
        type = PluralFormsToken::T_EOF;
    }
    else if (isdigit(*m_s))
    {
        PluralFormsToken::Number number = *m_s++ - '0';
        while (isdigit(*m_s))
        {
            number = number * 10 + (*m_s++ - '0');
        }
        m_token.setNumber(number);
        type = PluralFormsToken::T_NUMBER;
    }
    else if (isalpha(*m_s))
    {
        const char* begin = m_s++;
        while (isalnum(*m_s))
        {
            ++m_s;
        }
        size_t size = m_s - begin;
        if (size == 1 && memcmp(begin, "n", size) == 0)
        {
            type = PluralFormsToken::T_N;
        }
        else if (size == 6 && memcmp(begin, "plural", size) == 0)
        {
            type = PluralFormsToken::T_PLURAL;
        }
        else if (size == 8 && memcmp(begin, "nplurals", size) == 0)
        {
            type = PluralFormsToken::T_NPLURALS;
        }
    }
    else if (*m_s == '=')
    {
        ++m_s;
        if (*m_s == '=')
        {
            ++m_s;
            type = PluralFormsToken::T_EQUAL;
        }
        else
        {
            type = PluralFormsToken::T_ASSIGN;
        }
    }
    else if (*m_s == '>')
    {
        ++m_s;
        if (*m_s == '=')
        {
            ++m_s;
            type = PluralFormsToken::T_GREATER_OR_EQUAL;
        }
        else
        {
            type = PluralFormsToken::T_GREATER;
        }
    }
    else if (*m_s == '<')
    {
        ++m_s;
        if (*m_s == '=')
        {
            ++m_s;
            type = PluralFormsToken::T_LESS_OR_EQUAL;
        }
        else
        {
            type = PluralFormsToken::T_LESS;
        }
    }
    else if (*m_s == '%')
    {
        ++m_s;
        type = PluralFormsToken::T_REMINDER;
    }
    else if (*m_s == '!' && m_s[1] == '=')
    {
        m_s += 2;
        type = PluralFormsToken::T_NOT_EQUAL;
    }
    else if (*m_s == '&' && m_s[1] == '&')
    {
        m_s += 2;
        type = PluralFormsToken::T_LOGICAL_AND;
    }
    else if (*m_s == '|' && m_s[1] == '|')
    {
        m_s += 2;
        type = PluralFormsToken::T_LOGICAL_OR;
    }
    else if (*m_s == '?')
    {
        ++m_s;
        type = PluralFormsToken::T_QUESTION;
    }
    else if (*m_s == ':')
    {
        ++m_s;
        type = PluralFormsToken::T_COLON;
    } else if (*m_s == ';') {
        ++m_s;
        type = PluralFormsToken::T_SEMICOLON;
    }
    else if (*m_s == '(')
    {
        ++m_s;
        type = PluralFormsToken::T_LEFT_BRACKET;
    }
    else if (*m_s == ')')
    {
        ++m_s;
        type = PluralFormsToken::T_RIGHT_BRACKET;
    }
    m_token.setType(type);
    return type != PluralFormsToken::T_ERROR;
}




PluralFormsNodePtr::~PluralFormsNodePtr()
{
    delete m_p;
}
PluralFormsNode* PluralFormsNodePtr::release()
{
    PluralFormsNode *p = m_p;
    m_p = NULL;
    return p;
}
void PluralFormsNodePtr::reset(PluralFormsNode *p)
{
    if (p != m_p)
    {
        delete m_p;
        m_p = p;
    }
}


void PluralFormsNode::setNode(size_t i, PluralFormsNode* n)
{
    m_nodes[i].reset(n);
}

PluralFormsNode*  PluralFormsNode::releaseNode(size_t i)
{
    return m_nodes[i].release();
}

PluralFormsToken::Number
PluralFormsNode::evaluate(PluralFormsToken::Number n) const
{
    switch (token().type())
    {
        // leaf
        case PluralFormsToken::T_NUMBER:
            return token().number();
        case PluralFormsToken::T_N:
            return n;
        // 2 args
        case PluralFormsToken::T_EQUAL:
            return node(0)->evaluate(n) == node(1)->evaluate(n);
        case PluralFormsToken::T_NOT_EQUAL:
            return node(0)->evaluate(n) != node(1)->evaluate(n);
        case PluralFormsToken::T_GREATER:
            return node(0)->evaluate(n) > node(1)->evaluate(n);
        case PluralFormsToken::T_GREATER_OR_EQUAL:
            return node(0)->evaluate(n) >= node(1)->evaluate(n);
        case PluralFormsToken::T_LESS:
            return node(0)->evaluate(n) < node(1)->evaluate(n);
        case PluralFormsToken::T_LESS_OR_EQUAL:
            return node(0)->evaluate(n) <= node(1)->evaluate(n);
        case PluralFormsToken::T_REMINDER:
            {
                PluralFormsToken::Number number = node(1)->evaluate(n);
                if (number != 0)
                {
                    return node(0)->evaluate(n) % number;
                }
                else
                {
                    return 0;
                }
            }
        case PluralFormsToken::T_LOGICAL_AND:
            return node(0)->evaluate(n) && node(1)->evaluate(n);
        case PluralFormsToken::T_LOGICAL_OR:
            return node(0)->evaluate(n) || node(1)->evaluate(n);
        // 3 args
        case PluralFormsToken::T_QUESTION:
            return node(0)->evaluate(n)
                ? node(1)->evaluate(n)
                : node(2)->evaluate(n);
        default:
            return 0;
    }
}


void PluralFormsCalculator::init(PluralFormsToken::Number nplurals,
                                   PluralFormsNode* plural)
{
    m_nplurals = nplurals;
    m_plural.reset(plural);
}

int PluralFormsCalculator::evaluate(int n) const
{
    if (m_plural.get() == 0)
    {
        return 0;
    }
    PluralFormsToken::Number number = m_plural->evaluate(n);
    if (number < 0 || number > m_nplurals)
    {
        return 0;
    }
    return number;
}


class PluralFormsParser
{
public:
    PluralFormsParser(PluralFormsScanner& scanner) : m_scanner(scanner) {}
    bool parse(PluralFormsCalculator& rCalculator);

private:
    PluralFormsNode* parsePlural();
    // stops at T_SEMICOLON, returns 0 if error
    PluralFormsScanner& m_scanner;
    const PluralFormsToken& token() const;
    bool nextToken();

    PluralFormsNode* expression();
    PluralFormsNode* logicalOrExpression();
    PluralFormsNode* logicalAndExpression();
    PluralFormsNode* equalityExpression();
    PluralFormsNode* multiplicativeExpression();
    PluralFormsNode* relationalExpression();
    PluralFormsNode* pmExpression();
};

bool PluralFormsParser::parse(PluralFormsCalculator& rCalculator)
{
    if (token().type() != PluralFormsToken::T_NPLURALS)
        return false;
    if (!nextToken())
        return false;
    if (token().type() != PluralFormsToken::T_ASSIGN)
        return false;
    if (!nextToken())
        return false;
    if (token().type() != PluralFormsToken::T_NUMBER)
        return false;
    PluralFormsToken::Number nplurals = token().number();
    if (!nextToken())
        return false;
    if (token().type() != PluralFormsToken::T_SEMICOLON)
        return false;
    if (!nextToken())
        return false;
    if (token().type() != PluralFormsToken::T_PLURAL)
        return false;
    if (!nextToken())
        return false;
    if (token().type() != PluralFormsToken::T_ASSIGN)
        return false;
    if (!nextToken())
        return false;
    PluralFormsNode* plural = parsePlural();
    if (plural == 0)
        return false;
    if (token().type() != PluralFormsToken::T_SEMICOLON)
        return false;
    if (!nextToken())
        return false;
    if (token().type() != PluralFormsToken::T_EOF)
        return false;
    rCalculator.init(nplurals, plural);
    return true;
}

PluralFormsNode* PluralFormsParser::parsePlural()
{
    PluralFormsNode* p = expression();
    if (p == NULL)
    {
        return NULL;
    }
    PluralFormsNodePtr n(p);
    if (token().type() != PluralFormsToken::T_SEMICOLON)
    {
        return NULL;
    }
    return n.release();
}

const PluralFormsToken& PluralFormsParser::token() const
{
    return m_scanner.token();
}

bool PluralFormsParser::nextToken()
{
    if (!m_scanner.nextToken())
        return false;
    return true;
}

PluralFormsNode* PluralFormsParser::expression()
{
    PluralFormsNode* p = logicalOrExpression();
    if (p == NULL)
        return NULL;
    PluralFormsNodePtr n(p);
    if (token().type() == PluralFormsToken::T_QUESTION)
    {
        PluralFormsNodePtr qn(new PluralFormsNode(token()));
        if (!nextToken())
        {
            return 0;
        }
        p = expression();
        if (p == 0)
        {
            return 0;
        }
        qn->setNode(1, p);
        if (token().type() != PluralFormsToken::T_COLON)
        {
            return 0;
        }
        if (!nextToken())
        {
            return 0;
        }
        p = expression();
        if (p == 0)
        {
            return 0;
        }
        qn->setNode(2, p);
        qn->setNode(0, n.release());
        return qn.release();
    }
    return n.release();
}

PluralFormsNode*PluralFormsParser::logicalOrExpression()
{
    PluralFormsNode* p = logicalAndExpression();
    if (p == NULL)
        return NULL;
    PluralFormsNodePtr ln(p);
    if (token().type() == PluralFormsToken::T_LOGICAL_OR)
    {
        PluralFormsNodePtr un(new PluralFormsNode(token()));
        if (!nextToken())
        {
            return 0;
        }
        p = logicalOrExpression();
        if (p == 0)
        {
            return 0;
        }
        PluralFormsNodePtr rn(p);    // right
        if (rn->token().type() == PluralFormsToken::T_LOGICAL_OR)
        {
            // see logicalAndExpression comment
            un->setNode(0, ln.release());
            un->setNode(1, rn->releaseNode(0));
            rn->setNode(0, un.release());
            return rn.release();
        }


        un->setNode(0, ln.release());
        un->setNode(1, rn.release());
        return un.release();
    }
    return ln.release();
}

PluralFormsNode* PluralFormsParser::logicalAndExpression()
{
    PluralFormsNode* p = equalityExpression();
    if (p == NULL)
        return NULL;
    PluralFormsNodePtr ln(p);   // left
    if (token().type() == PluralFormsToken::T_LOGICAL_AND)
    {
        PluralFormsNodePtr un(new PluralFormsNode(token()));  // up
        if (!nextToken())
        {
            return NULL;
        }
        p = logicalAndExpression();
        if (p == 0)
        {
            return NULL;
        }
        PluralFormsNodePtr rn(p);    // right
        if (rn->token().type() == PluralFormsToken::T_LOGICAL_AND)
        {
// transform 1 && (2 && 3) -> (1 && 2) && 3
//     u                  r
// l       r     ->   u      3
//       2   3      l   2
            un->setNode(0, ln.release());
            un->setNode(1, rn->releaseNode(0));
            rn->setNode(0, un.release());
            return rn.release();
        }

        un->setNode(0, ln.release());
        un->setNode(1, rn.release());
        return un.release();
    }
    return ln.release();
}

PluralFormsNode* PluralFormsParser::equalityExpression()
{
    PluralFormsNode* p = relationalExpression();
    if (p == NULL)
        return NULL;
    PluralFormsNodePtr n(p);
    if (token().type() == PluralFormsToken::T_EQUAL
        || token().type() == PluralFormsToken::T_NOT_EQUAL)
    {
        PluralFormsNodePtr qn(new PluralFormsNode(token()));
        if (!nextToken())
        {
            return NULL;
        }
        p = relationalExpression();
        if (p == NULL)
        {
            return NULL;
        }
        qn->setNode(1, p);
        qn->setNode(0, n.release());
        return qn.release();
    }
    return n.release();
}

PluralFormsNode* PluralFormsParser::relationalExpression()
{
    PluralFormsNode* p = multiplicativeExpression();
    if (p == NULL)
        return NULL;
    PluralFormsNodePtr n(p);
    if (token().type() == PluralFormsToken::T_GREATER
            || token().type() == PluralFormsToken::T_LESS
            || token().type() == PluralFormsToken::T_GREATER_OR_EQUAL
            || token().type() == PluralFormsToken::T_LESS_OR_EQUAL)
    {
        PluralFormsNodePtr qn(new PluralFormsNode(token()));
        if (!nextToken())
        {
            return NULL;
        }
        p = multiplicativeExpression();
        if (p == NULL)
        {
            return NULL;
        }
        qn->setNode(1, p);
        qn->setNode(0, n.release());
        return qn.release();
    }
    return n.release();
}

PluralFormsNode* PluralFormsParser::multiplicativeExpression()
{
    PluralFormsNode* p = pmExpression();
    if (p == NULL)
        return NULL;
    PluralFormsNodePtr n(p);
    if (token().type() == PluralFormsToken::T_REMINDER)
    {
        PluralFormsNodePtr qn(new PluralFormsNode(token()));
        if (!nextToken())
        {
            return NULL;
        }
        p = pmExpression();
        if (p == NULL)
        {
            return NULL;
        }
        qn->setNode(1, p);
        qn->setNode(0, n.release());
        return qn.release();
    }
    return n.release();
}

PluralFormsNode* PluralFormsParser::pmExpression()
{
    PluralFormsNodePtr n;
    if (token().type() == PluralFormsToken::T_N
        || token().type() == PluralFormsToken::T_NUMBER)
    {
        n.reset(new PluralFormsNode(token()));
        if (!nextToken())
        {
            return NULL;
        }
    }
    else if (token().type() == PluralFormsToken::T_LEFT_BRACKET) {
        if (!nextToken())
        {
            return NULL;
        }
        PluralFormsNode* p = expression();
        if (p == NULL)
        {
            return NULL;
        }
        n.reset(p);
        if (token().type() != PluralFormsToken::T_RIGHT_BRACKET)
        {
            return NULL;
        }
        if (!nextToken())
        {
            return NULL;
        }
    }
    else
    {
        return NULL;
    }
    return n.release();
}

std::unique_ptr<PluralFormsCalculator> PluralFormsCalculator::make(const char* s)
{
    auto calculator = std::make_unique<PluralFormsCalculator>();
    if (s != NULL)
    {
        PluralFormsScanner scanner(s);
        PluralFormsParser p(scanner);
        if (!p.parse(*calculator))
        {
            return NULL;
        }
    }
    return calculator;
}
