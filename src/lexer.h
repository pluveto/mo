#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <cctype>
#include <stdexcept>

struct Token {
    enum class Type {
        // Keywords
        Let,
        Struct,
        Impl,
        Fn,
        Return,
        Int,
        Float,
        Const,
        Sizeof,
        Cast,
        If,
        Else,
        While,
        For,
        // Identifiers
        Identifier,
        // Literals
        IntegerLiteral,
        FloatLiteral,
        StringLiteral,
        // Operators
        OperatorDot,         // .
        OperatorArrow,       // ->
        OperatorDoubleColon, // ::
        OperatorAssign,      // =
        OperatorColon,       // :
        OperatorSemicolon,   // ;
        OperatorComma,       // ,
        OperatorStar,        // *
        OperatorAmpersand,   // &
        OperatorLParen,      // (
        OperatorRParen,      // )
        OperatorLBrace,      // {
        OperatorRBrace,     // }
        OperatorLBracket,    // [
        OperatorRBracket,    // ]
        OperatorPlus,        // +
        OperatorMinus,       // -
        OperatorDivide,      // /
        OperatorModulo,      // %
        OperatorEq,          // ==
        OperatorNe,          // !=
        OperatorLt,          // <
        OperatorLe,          // <=
        OperatorGt,          // >
        OperatorGe,          // >=
        OperatorAnd,         // &&
        OperatorOr,          // ||
        // End of File
        Eof,
    };

    Type type;
    int startLine;
    int startCol;
    int endLine;
    int endCol;
    std::string lexeme;

    Token(Type type, int startLine, int startCol, int endLine, int endCol, const std::string& lexeme);
};

class Lexer {
public:
    Lexer(const std::string& input);
    Token nextToken();

private:
    std::string input;
    size_t pos;
    int currentLine;
    int currentCol;

    void advance();
    char peek(size_t offset = 0) const;
    void skipWhitespaceAndComments();
    void skipLineComment();
    void skipBlockComment();
    Token parseNumber();
    Token parseString();
    Token parseIdentifierOrKeyword();
    Token parseArrowOrMinus();
    Token parseDoubleColonOrColon();
    Token parseDot();
    Token parseEqualOrEq();
    Token parseNe();
    Token parseLeOrLt();
    Token parseGeOrGt();
    Token parseAnd();
    Token parseOr();
    Token parseSingleChar(Token::Type type, const std::string& lexeme);
};

#endif // LEXER_H
