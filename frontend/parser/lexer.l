%{
    #include <frontend/parser/yacc.h>
    #include <frontend/parser/scanner.h>
    #include <frontend/parser/location.hh>
    #include <frontend/parser/parser.h>
    #include <stdexcept>
    #include <string>
    #include <limits>      
    #include <cmath>       

    using namespace FE;

    static location loc;

    // YY_USER_ACTION 宏会自动插入到每个规则的代码块开头
    // 如下方的 "int"               { RETT(INT, loc) }
    // 实际上会被转换为
    // "int"               { loc.step(); loc.columns(yyleng); RETT(INT, loc) }
    // 这样就能自动更新 loc 的位置
    // yyleng 是当前匹配的字符串的长度
    #define YY_USER_ACTION      \
        loc.step();             \
        loc.columns(yyleng);

    #define yywrap() 1
    #define RETT(x, y) \
        return YaccParser::make_##x(y);

    #define TAB_WIDTH 4

    int handleTab();
    long long convertToInt(const char* str, const char end, bool& isLongLong);
    float convertToFloatDec(const char* str);
    float convertToFloatHex(const char* str);
%}

%option c++
%option noyywrap debug
%option yyclass="Scanner"
%option prefix="Yacc_"
%x COMMENT

%%

%{
/*
 * Lab1: 词法分析器实现
 * 
 * 本词法分析器实现了以下功能：
 * 1. 关键字识别: int, float, void, if, else, for, while, return, const, break, continue等
 * 2. 运算符识别: 算术、关系、逻辑运算符
 * 3. 注释处理: 单行注释和多行注释
 * 4. 数字识别: 十进制、八进制、十六进制、浮点数(十进制和十六进制)
 * 5. 字符串字面量: 支持转义字符处理
 * 6. 错误处理: 遇到未知字符时报告错误
 * 7. 位置信息: 自动计算行号和列号
 * 
 * 相对于原始代码的改进：
 * - 添加了完整的运算符集合
 * - 添加了类型关键字(int, float, void)
 * - 实现了浮点数识别(支持十进制和十六进制格式)
 * - 实现了多行注释处理(使用Flex状态机)
 * - 添加了字符串转义字符处理
 * - 添加了未知字符错误报告
 * - 改进的整数识别(支持八进制和十六进制)
 * - 添加了长整型支持(LL_CONST)
 *
 * 词法分析返回的四种主要类型：
 * - int_token: 整型常量值
 * - float_token: 浮点型常量值
 * - string_token: 字符串和标识符
 * - symbol_token: 运算符和关键字(无值)
 */
%}

\n              {
                    loc.lines(yyleng);
                    loc.step();
                }

[ \f\r\v]+    { /* empty */ }
[\t]    {
    loc.columns(handleTab());
}

"if"                { RETT(IF, loc) }    // 控制流关键字
"else"              { RETT(ELSE, loc) }
"for"               { RETT(FOR, loc) }
"while"             { RETT(WHILE, loc) }
"continue"          { RETT(CONTINUE, loc) }
"break"             { RETT(BREAK, loc) }
"switch"            { RETT(SWITCH, loc) }
"case"              { RETT(CASE, loc) }
"goto"              { RETT(GOTO, loc) }
"do"                { RETT(DO, loc) }
"return"            { RETT(RETURN, loc) }

"const"             { RETT(CONST, loc) }
"int"               { RETT(INT, loc) }        // 整型关键字
"float"             { RETT(FLOAT, loc) }      // 浮点型关键字
"void"              { RETT(VOID, loc) }       // 空类型关键字

";"                 { RETT(SEMICOLON, loc) }
","                 { RETT(COMMA, loc) }

"="                 { RETT(ASSIGN, loc) }      // 赋值运算符
"+"                 { RETT(PLUS, loc) }
"-"                 { RETT(MINUS, loc) }
"*"                 { RETT(STAR, loc) }
"/"                 { RETT(SLASH, loc) }
"%"                 { RETT(MOD, loc) }

"<"                 { RETT(LT, loc) }
">"                 { RETT(GT, loc) }
"<="                { RETT(LE, loc) }
">="                { RETT(GE, loc) }
"=="                { RETT(EQ, loc) }
"!="                { RETT(NE, loc) }

"&&"                { RETT(AND, loc) }
"||"                { RETT(OR, loc) }
"!"                 { RETT(NOT, loc) }

"//"[^\n]*              { }  // 单行注释，直接忽略

"/*"                    { BEGIN(COMMENT); }  // 多行注释状态机

<COMMENT>"*/"           { BEGIN(INITIAL); }
<COMMENT>\n             { loc.lines(1); loc.step(); }
<COMMENT>.              { }
<COMMENT><<EOF>>        { 
    _parser.reportError(loc, "Unterminated comment");
    return YaccParser::make_END(loc);
}

[A-Za-z_]+[A-Za-z0-9_]* {  // 标识符识别
    auto str = std::string(yytext);
    return YaccParser::make_IDENT(str, loc); 
}

\"[^\"]*\"          {  // 字符串字面量识别 
    std::string str(yytext);
    if (!str.empty()) str = str.substr(1, str.size() - 2);
    
    // 词法分析器增强：处理转义字符，增强字符串处理能力
    // 支持常见转义序列：\n(换行)、\t(制表)、\r(回车)、\0(空)、\\(反斜杠)、\'(单引号)、\"(双引号)、\?(问号)
    std::string result;
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '\\' && i + 1 < str.size()) {
            switch (str[i+1]) {
                case 'n': result += '\n'; i++; break;
                case 't': result += '\t'; i++; break;
                case 'r': result += '\r'; i++; break;
                case '0': result += '\0'; i++; break;
                case '\\': result += '\\'; i++; break;
                case '\'': result += '\''; i++; break;
                case '"': result += '"'; i++; break;
                case '?': result += '?'; i++; break;
                default: result += str[i]; break;
            }
        } else {
            result += str[i];
        }
    }
    
    return YaccParser::make_STR_CONST(result, loc); 
}

0[xX][0-9a-fA-F]+ {  // 十六进制整数识别
    try {
        bool isLL = false;
        long long result = convertToInt(yytext, '\0', isLL);
        if (isLL) return YaccParser::make_LL_CONST(result, loc);
        return YaccParser::make_INT_CONST(static_cast<int>(result), loc);
    } catch (const std::exception& e) {
        _parser.reportError(loc, std::string("Error parsing hexadecimal int: ") + e.what());
        auto str = std::string(yytext);
        return YaccParser::make_ERR_TOKEN(str, loc);
    }
}

0[0-7]+ {  // 八进制整数识别
    try {
        bool isLL = false;
        long long result = convertToInt(yytext, '\0', isLL);
        if (isLL) return YaccParser::make_LL_CONST(result, loc);
        return YaccParser::make_INT_CONST(static_cast<int>(result), loc);
    } catch (const std::exception& e) {
        _parser.reportError(loc, std::string("Error parsing octal int: ") + e.what());
        auto str = std::string(yytext);
        return YaccParser::make_ERR_TOKEN(str, loc);
    }
}

0[xX][0-9a-fA-F]*\.[0-9a-fA-F]*[pP][+-]?[0-9]+ {  // 十六进制浮点数(带小数点和二进制指数)
    try {
        float result = convertToFloatHex(yytext);
        return YaccParser::make_FLOAT_CONST(result, loc);
    } catch (const std::exception& e) {
        _parser.reportError(loc, std::string("Error parsing hex float: ") + e.what());
        auto str = std::string(yytext);
        return YaccParser::make_ERR_TOKEN(str, loc);
    }
}

0[xX][0-9a-fA-F]+[pP][+-]?[0-9]+ {  // 十六进制浮点数(不带小数部分)
    try {
        float result = convertToFloatHex(yytext);
        return YaccParser::make_FLOAT_CONST(result, loc);
    } catch (const std::exception& e) {
        _parser.reportError(loc, std::string("Error parsing hex float: ") + e.what());
        auto str = std::string(yytext);
        return YaccParser::make_ERR_TOKEN(str, loc);
    }
}

[0-9]+\.[0-9]*([eE][+-]?[0-9]+)? {  // 十进制浮点数(标准形式)
    try {
        float result = convertToFloatDec(yytext);
        return YaccParser::make_FLOAT_CONST(result, loc);
    } catch (const std::exception& e) {
        _parser.reportError(loc, std::string("Error parsing decimal float: ") + e.what());
        auto str = std::string(yytext);
        return YaccParser::make_ERR_TOKEN(str, loc);
    }
}

\.[0-9]+([eE][+-]?[0-9]+)? {  // 十进制浮点数(省略整数部分)
    try {
        float result = convertToFloatDec(yytext);
        return YaccParser::make_FLOAT_CONST(result, loc);
    } catch (const std::exception& e) {
        _parser.reportError(loc, std::string("Error parsing decimal float: ") + e.what());
        auto str = std::string(yytext);
        return YaccParser::make_ERR_TOKEN(str, loc);
    }
}

[0-9]+[eE][+-]?[0-9]+ {  // 十进制浮点数(纯科学计数法)
    try {
        float result = convertToFloatDec(yytext);
        return YaccParser::make_FLOAT_CONST(result, loc);
    } catch (const std::exception& e) {
        _parser.reportError(loc, std::string("Error parsing decimal float: ") + e.what());
        auto str = std::string(yytext);
        return YaccParser::make_ERR_TOKEN(str, loc);
    }
}

[1-9][0-9]* {  // 十进制整数识别
    try {
        bool isLL = false;
        long long result = convertToInt(yytext, '\0', isLL);
        if (isLL) return YaccParser::make_LL_CONST(result, loc);
        return YaccParser::make_INT_CONST(static_cast<int>(result), loc);
    } catch (const std::exception& e) {
        _parser.reportError(loc, std::string("Error parsing int: ") + e.what());
        auto str = std::string(yytext);
        return YaccParser::make_ERR_TOKEN(str, loc);
    }
}

0 {
    return YaccParser::make_INT_CONST(0, loc);
}

"("                 { RETT(LPAREN, loc) }
")"                 { RETT(RPAREN, loc) }
"["                 { RETT(LBRACKET, loc) }
"]"                 { RETT(RBRACKET, loc) }
"{"                 { RETT(LBRACE, loc) }
"}"                 { RETT(RBRACE, loc) }

.                  {
    _parser.reportError(loc, std::string("Unexpected character: ") + yytext);
    return YaccParser::make_ERR_TOKEN(std::string(yytext), loc);
}

<<EOF>>         { RETT(END, loc); }

%%

/**
 * 处理制表符(TAB)
 * 根据当前位置计算制表符应占用的列数
 * 
 * @return 制表符占用的列数
 * 
 * 示例(假设TAB_WIDTH=4)：
 * - " \t"   -> 返回3 (当前位置1，需要到4，占用3列)
 * - "   \t"  -> 返回1 (当前位置3，需要到4，占用1列)
 * - "    \t" -> 返回4 (当前位置4，需要到8，占用4列)
 */
int handleTab()
{
    int& c = loc.begin.column;
    return TAB_WIDTH - ((c - 1) % TAB_WIDTH) - 1;
}

/**
 * Lab1：整数转换函数
 * 将字符串转换为整数，支持十进制、十六进制和八进制整数
 * 
 * @param str 要转换的字符串
 * @param end 字符串结束标志(当前传入'\0'表示null结尾)
 * @param isLongLong 输出参数，指示结果是否超出int范围
 * @return 转换后的long long整数值
 * 
 * 支持的格式：
 * - 十进制: 123, 4567
 * - 十六进制: 0x1A3F, 0XFF
 * - 八进制: 0123, 0777
 */
long long convertToInt(const char* str, const char end, bool& isLongLong)
{
    int         base       = 10;
    long long   result     = 0;
    int         isNegative = 0;
    const char* ptr        = str;
    static int  zeroOffset = '0';
    static int  aOffset    = 'a' - 10;
    static int  AOffset    = 'A' - 10;
    int*        offset     = NULL;
    isLongLong             = false;

    if (*ptr == '-')
    {
        isNegative = 1;
        ++ptr;
    }
    else if (*ptr == '+') { ++ptr; }

    if (ptr[0] == '0' && (ptr[1] == 'x' || ptr[1] == 'X'))
    {
        base = 16;
        ptr += 2;
    }
    else if (ptr[0] == '0')
    {
        base = 8;
        ptr += 1;
    }

    while (*ptr != end)
    {
        int value = 0;

        if (base == 16)
        {
            if (*ptr >= '0' && *ptr <= '9')
                offset = &zeroOffset;
            else if (*ptr >= 'a' && *ptr <= 'f')
                offset = &aOffset;
            else if (*ptr >= 'A' && *ptr <= 'F')
                offset = &AOffset;
        }
        else { offset = &zeroOffset; }

        value  = *ptr - *offset;
        result = result * base + value;
        ++ptr;
    }

    if (isNegative) result = -result;

    if (result > std::numeric_limits<int>::max() || result < std::numeric_limits<int>::min())
    {
        isLongLong = true;
        if (result > std::numeric_limits<long long>::max() || result < std::numeric_limits<long long>::min())
        {
            throw std::out_of_range(str + std::string(" overflow or underflow for long long"));
        }
    }

    return result;
}

/**
 * Lab1：十进制浮点数转换函数
 * 将字符串转换为浮点数，支持多种十进制浮点数格式
 * 
 * @param str 要转换的字符串
 * @return 转换后的float浮点数值
 * 
 * 支持的格式：
 * - 标准形式: 3.14, 123.456
 * - 省略小数部分: 5., 123.
 * - 省略整数部分: .5, .123
 * - 科学计数法: 1e10, 1.5e10, 2.5e-3
 * - 负数支持: -3.14, -1e10
 * 
 * 可处理溢出和非法值，并抛出异常
 */
float convertToFloatDec(const char* str)
{
    const char* head = str;
    if (str == NULL) { return 0.0f; }

    int sign = 1;
    if (*str == '-')
    {
        sign = -1;
        str++;
    }
    else if (*str == '+') { str++; }

    double integerPart = 0.0;
    while (isdigit(*str))
    {
        integerPart = integerPart * 10 + (*str - '0');
        str++;
    }

    double fractionPart = 0.0;
    if (*str == '.')
    {
        str++;
        double divisor = 10.0;
        while (isdigit(*str))
        {
            fractionPart += (*str - '0') / divisor;
            divisor *= 10.0;
            str++;
        }
    }

    double value = integerPart + fractionPart;

    if (*str == 'e' || *str == 'E')
    {
        str++;
        int expSign = 1;
        if (*str == '-')
        {
            expSign = -1;
            str++;
        }
        else if (*str == '+') { str++; }

        int exponent = 0;
        while (isdigit(*str))
        {
            exponent = exponent * 10 + (*str - '0');
            str++;
        }

        value *= pow(10, expSign * exponent);
    }

    value = sign * value;

    if (value > std::numeric_limits<float>::max() || value < -std::numeric_limits<float>::max())
    {
        throw std::out_of_range(
            head + ((value > std::numeric_limits<float>::max()) ? std::string(" overflow") : std::string(" underflow")));
    }

    return static_cast<float>(value);
}

/**
 * Lab1：十六进制浮点数转换函数
 * 将字符串转换为浮点数，支持十六进制浮点数格式
 * 
 * @param str 要转换的字符串
 * @return 转换后的float浮点数值
 * 
 * 支持的格式：
 * - 带小数点: 0x1.8p0, 0xA.Bp2, 0x1.0p-3
 * - 不带小数点: 0x1p5, 0xAp-2
 * - 二进制指数: 使用p或P表示（非e，因为十六进制）
 * 
 * 工作原理：
 * 1. 将十六进制的整数和小数部分转换为十进制
 * 2. 使用二进制指数(p/P)进行幂运算（而非十进制指数e）
 * 3. 可处理溢出并抛出异常
 */
float convertToFloatHex(const char* str)
{
    const char* head = str;
    if (str == NULL) { return 0.0f; }

    int sign = 1;
    if (*str == '-')
    {
        sign = -1;
        str++;
    }
    else if (*str == '+') { str++; }

    if (*str == '0' && (*(str + 1) == 'x' || *(str + 1) == 'X')) { str += 2; }

    unsigned long long integerPart = 0;
    while (isxdigit(*str))
    {
        int digit = 0;
        if (isdigit(*str)) { digit = (*str - '0'); }
        else if (*str >= 'a' && *str <= 'f') { digit = (*str - 'a' + 10); }
        else if (*str >= 'A' && *str <= 'F') { digit = (*str - 'A' + 10); }
        integerPart = integerPart * 16 + digit;
        str++;
    }

    double fractionPart = 0.0;
    if (*str == '.')
    {
        str++;
        unsigned long long numerator   = 0;
        unsigned long long denominator = 1;
        while (isxdigit(*str))
        {
            int digit = 0;
            if (isdigit(*str)) { digit = (*str - '0'); }
            else if (*str >= 'a' && *str <= 'f') { digit = (*str - 'a' + 10); }
            else if (*str >= 'A' && *str <= 'F') { digit = (*str - 'A' + 10); }
            numerator = numerator * 16 + digit;
            denominator *= 16;
            str++;
        }
        fractionPart = (double)numerator / (double)denominator;
    }

    double value = (double)integerPart + fractionPart;

    if (*str == 'p' || *str == 'P')
    {
        str++;
        int expSign = 1;
        if (*str == '-')
        {
            expSign = -1;
            str++;
        }
        else if (*str == '+') { str++; }

        int exponent = 0;
        while (isdigit(*str))
        {
            exponent = exponent * 10 + (*str - '0');
            str++;
        }

        value *= pow(2, expSign * exponent);
    }

    value = sign * value;

    if (value > std::numeric_limits<float>::max() || value < -std::numeric_limits<float>::max())
    {
        throw std::out_of_range(
            head + ((value > std::numeric_limits<float>::max()) ? std::string(" overflow") : std::string(" underflow")));
    }

    return static_cast<float>(value);
}
