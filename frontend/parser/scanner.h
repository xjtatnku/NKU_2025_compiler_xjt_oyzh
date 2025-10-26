#ifndef __FRONTEND_PARSER_SCANNER_H__
#define __FRONTEND_PARSER_SCANNER_H__

#ifndef yyFlexLexerOnce
#undef yyFlexLexer
#define yyFlexLexer Yacc_FlexLexer
#include <FlexLexer.h>
#endif

#undef YY_DECL
#define YY_DECL FE::YaccParser::symbol_type FE::Scanner::nextToken()

#include <frontend/parser/yacc.h>

namespace FE
{
    class Parser;

    class Scanner : public yyFlexLexer
    {
      private:
        Parser& _parser;

      public:
        Scanner(Parser& parser) : _parser(parser) {}
        virtual ~Scanner() {}

        virtual YaccParser::symbol_type nextToken();
    };
}  // namespace FE

#endif  // __FRONTEND_PARSER_SCANNER_H__
