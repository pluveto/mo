#include "gtest/gtest.h"
#include "src/lexer.h"

TEST(LexerTest, TestNumber) {
    Lexer lexer("123 45.67");
    Token token1 = lexer.nextToken();
    EXPECT_EQ(token1.type, TokenType::IntegerLiteral);
    EXPECT_EQ(token1.lexeme, "123");

    Token token2 = lexer.nextToken();
    EXPECT_EQ(token2.type, TokenType::FloatLiteral);
    EXPECT_EQ(token2.lexeme, "45.67");
}

TEST(LexerTest, TestString) {
    Lexer lexer("\"hello\"");
    Token token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::StringLiteral);
    EXPECT_EQ(token.lexeme, "hello");
}

TEST(LexerTest, TestKeyword) {
    Lexer lexer("let if else");
    Token token1 = lexer.nextToken();
    EXPECT_EQ(token1.type, TokenType::Let);
    EXPECT_EQ(token1.lexeme, "let");

    Token token2 = lexer.nextToken();
    EXPECT_EQ(token2.type, TokenType::If);
    EXPECT_EQ(token2.lexeme, "if");

    Token token3 = lexer.nextToken();
    EXPECT_EQ(token3.type, TokenType::Else);
    EXPECT_EQ(token3.lexeme, "else");
}

TEST(LexerTest, TestOperator) {
    Lexer lexer("== != ->");
    Token token1 = lexer.nextToken();
    EXPECT_EQ(token1.type, TokenType::OperatorEq);
    EXPECT_EQ(token1.lexeme, "==");

    Token token2 = lexer.nextToken();
    EXPECT_EQ(token2.type, TokenType::OperatorNe);
    EXPECT_EQ(token2.lexeme, "!=");

    Token token3 = lexer.nextToken();
    EXPECT_EQ(token3.type, TokenType::OperatorArrow);
    EXPECT_EQ(token3.lexeme, "->");
}

TEST(LexerTest, TestEOF) {
    Lexer lexer("");
    Token token = lexer.nextToken();
    EXPECT_EQ(token.type, TokenType::Eof);
}
