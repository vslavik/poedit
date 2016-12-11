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

#include <wx/string.h>

#include <memory>

// ----------------------------------------------------------------------------
// Plural forms parser
// ----------------------------------------------------------------------------

/*
                                Simplified Grammar

Expression:
    LogicalOrExpression '?' Expression ':' Expression
    LogicalOrExpression

LogicalOrExpression:
    LogicalAndExpression "||" LogicalOrExpression   // to (a || b) || c
    LogicalAndExpression

LogicalAndExpression:
    EqualityExpression "&&" LogicalAndExpression    // to (a && b) && c
    EqualityExpression

EqualityExpression:
    RelationalExpression "==" RelationalExperession
    RelationalExpression "!=" RelationalExperession
    RelationalExpression

RelationalExpression:
    MultiplicativeExpression '>' MultiplicativeExpression
    MultiplicativeExpression '<' MultiplicativeExpression
    MultiplicativeExpression ">=" MultiplicativeExpression
    MultiplicativeExpression "<=" MultiplicativeExpression
    MultiplicativeExpression

MultiplicativeExpression:
    PmExpression '%' PmExpression
    PmExpression

PmExpression:
    N
    Number
    '(' Expression ')'
*/

class PluralFormsToken
{
public:
    enum Type
    {
        T_ERROR, T_EOF, T_NUMBER, T_N, T_PLURAL, T_NPLURALS, T_EQUAL, T_ASSIGN,
        T_GREATER, T_GREATER_OR_EQUAL, T_LESS, T_LESS_OR_EQUAL,
        T_REMINDER, T_NOT_EQUAL,
        T_LOGICAL_AND, T_LOGICAL_OR, T_QUESTION, T_COLON, T_SEMICOLON,
        T_LEFT_BRACKET, T_RIGHT_BRACKET
    };
    Type type() const { return m_type; }
    void setType(Type type) { m_type = type; }
    // for T_NUMBER only
    typedef int Number;
    Number number() const { return m_number; }
    void setNumber(Number num) { m_number = num; }
private:
    Type m_type;
    Number m_number;
};


class PluralFormsScanner
{
public:
    PluralFormsScanner(const char* s);
    const PluralFormsToken& token() const { return m_token; }
    bool nextToken();  // returns false if error
private:
    const char* m_s;
    PluralFormsToken m_token;
};

class PluralFormsNode;

// NB: Can't use wxDEFINE_SCOPED_PTR_TYPE because PluralFormsNode is not
//     fully defined yet:
class PluralFormsNodePtr
{
public:
    PluralFormsNodePtr(PluralFormsNode *p = NULL) : m_p(p) {}
    ~PluralFormsNodePtr();
    PluralFormsNode& operator*() const { return *m_p; }
    PluralFormsNode* operator->() const { return m_p; }
    PluralFormsNode* get() const { return m_p; }
    PluralFormsNode* release();
    void reset(PluralFormsNode *p);

private:
    PluralFormsNode *m_p;
};

class PluralFormsNode
{
public:
    PluralFormsNode(const PluralFormsToken& token) : m_token(token) {}
    const PluralFormsToken& token() const { return m_token; }
    const PluralFormsNode* node(size_t i) const
        { return m_nodes[i].get(); }
    void setNode(size_t i, PluralFormsNode* n);
    PluralFormsNode* releaseNode(size_t i);
    PluralFormsToken::Number evaluate(PluralFormsToken::Number n) const;

private:
    PluralFormsToken m_token;
    PluralFormsNodePtr m_nodes[3];
};


class PluralFormsCalculator
{
public:
    PluralFormsCalculator() : m_nplurals(0), m_plural(0) {}

    // input: number, returns msgstr index
    int evaluate(int n) const;

    // input: text after "Plural-Forms:" (e.g. "nplurals=2; plural=(n != 1);"),
    // if s == 0, creates default handler
    // returns 0 if error
    static std::unique_ptr<PluralFormsCalculator> make(const char* s = 0);

    ~PluralFormsCalculator() {}

    void  init(PluralFormsToken::Number nplurals, PluralFormsNode* plural);
    wxString getString() const;

private:
    PluralFormsToken::Number m_nplurals;
    PluralFormsNodePtr m_plural;
};
