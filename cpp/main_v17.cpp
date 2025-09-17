//Debes usar un compilador Pre-C++17
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <regex>

// Definiciones de tipos de datos para el AST y la Tabla de Simbolos
using AstValue = std::string;
using AstList = std::vector<AstValue>;
using AstBlock = std::map<std::string, AstValue>;

// -------------------------------------------------------------
// ANALIZADOR LEXICO (LEXER) - CORREGIDO Y MEJORADO
// -------------------------------------------------------------
enum TokenType {
    IDENTIFIER,
    STRING,
    NUMBER,
    OPERATOR,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,
    COMMA,
    EQUALS
};

struct Token {
    TokenType type;
    std::string value;
};

class Lexer {
public:
    Lexer(const std::string& source) : source_(source), current_pos_(0) {}

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        while (current_pos_ < source_.length()) {
            char current_char = source_[current_pos_];
            
            if (isspace(current_char)) {
                current_pos_++;
                continue;
            }

            if (current_char == '#') {
                while (current_pos_ < source_.length() && source_[current_pos_] != '\n') {
                    current_pos_++;
                }
                current_pos_++;
                continue;
            }

            if (current_char == '"') {
                current_pos_++;
                size_t start = current_pos_;
                while (current_pos_ < source_.length() && source_[current_pos_] != '"') {
                    current_pos_++;
                }
                tokens.push_back({STRING, source_.substr(start, current_pos_ - start)});
                current_pos_++;
                continue;
            }

            if (isalpha(current_char) || current_char == '_') {
                size_t start = current_pos_;
                while (current_pos_ < source_.length() && (isalnum(source_[current_pos_]) || source_[current_pos_] == '_')) {
                    current_pos_++;
                }
                tokens.push_back({IDENTIFIER, source_.substr(start, current_pos_ - start)});
                continue;
            }

            if (isdigit(current_char)) {
                size_t start = current_pos_;
                while (current_pos_ < source_.length() && (isdigit(source_[current_pos_]) || source_[current_pos_] == '.')) {
                    current_pos_++;
                }
                tokens.push_back({NUMBER, source_.substr(start, current_pos_ - start)});
                continue;
            }
            
            if (current_char == '{') tokens.push_back({LBRACE, "{"});
            else if (current_char == '}') tokens.push_back({RBRACE, "}"});
            else if (current_char == '[') tokens.push_back({LBRACKET, "["});
            else if (current_char == ']') tokens.push_back({RBRACKET, "]"});
            else if (current_char == '=') tokens.push_back({EQUALS, "="});
            else if (current_char == ',') tokens.push_back({COMMA, ","});
            else {
                // Manejar cualquier caracter inesperado
                throw std::runtime_error("Caracter inesperado: " + std::string(1, current_char));
            }
            current_pos_++;
        }
        return tokens;
    }

private:
    std::string source_;
    size_t current_pos_;

    std::string trim(const std::string& s) {
        auto wsfront = std::find_if_not(s.begin(), s.end(), ::isspace);
        return std::string(wsfront, std::find_if_not(s.rbegin(), s.rend(), ::isspace).base());
    }
    
    Token create_token(const std::string& value) {
        if (value == "=") return {EQUALS, "="};
        if (value == "{") return {LBRACE, "{"};
        if (value == "}") return {RBRACE, "}"};
        if (value == "[") return {LBRACKET, "["};
        if (value == "]") return {RBRACKET, "]"};
        if (value == ",") return {COMMA, ","};
        
        if (value.front() == '-' || isdigit(value.front())) {
            bool is_number = true;
            for(size_t i = 1; i < value.length(); ++i) {
                if (!isdigit(value[i]) && value[i] != '.') {
                    is_number = false;
                    break;
                }
            }
            if (is_number) {
                return {NUMBER, value};
            }
        }
        return {IDENTIFIER, value};
    }
};

// -------------------------------------------------------------
// ANALIZADOR SINTACTICO (PARSER) - SIN CAMBIOS
// -------------------------------------------------------------
class Parser {
public:
    Parser(const std::vector<Token>& tokens) : tokens_(tokens), current_token_index_(0) {}

    std::map<std::string, AstValue> parse() {
        std::map<std::string, AstValue> ast;
        while (current_token_index_ < tokens_.size()) {
            if (peek_token().type != IDENTIFIER) {
                throw std::runtime_error("Error de sintaxis: Se esperaba un identificador.");
            }
            std::string key = get_token().value;

            if (peek_token().type != EQUALS) {
                throw std::runtime_error("Error de sintaxis: Se esperaba '='.");
            }
            get_token(); // Consume '='
            
            ast[key] = parse_value();
        }
        return ast;
    }

private:
    const std::vector<Token>& tokens_;
    size_t current_token_index_;

    const Token& peek_token() {
        if (current_token_index_ >= tokens_.size()) {
            throw std::runtime_error("Error de sintaxis: Fin inesperado del archivo.");
        }
        return tokens_[current_token_index_];
    }
    
    const Token& get_token() {
        if (current_token_index_ >= tokens_.size()) {
            throw std::runtime_error("Error de sintaxis: Fin inesperado del archivo.");
        }
        return tokens_[current_token_index_++];
    }

    AstValue parse_value() {
        const Token& token = peek_token();
        if (token.type == IDENTIFIER || token.type == STRING || token.type == NUMBER) {
            return get_token().value;
        } else if (token.type == LBRACE) {
            return parse_block();
        } else if (token.type == LBRACKET) {
            return parse_list();
        }
        throw std::runtime_error("Error de sintaxis: Valor inesperado.");
    }

    AstValue parse_block() {
        get_token(); // Consume '{'
        std::map<std::string, AstValue> block_content;
        while (peek_token().type != RBRACE) {
            if (peek_token().type != IDENTIFIER) {
                throw std::runtime_error("Error de sintaxis en bloque: Se esperaba un identificador.");
            }
            std::string key = get_token().value;
            if (get_token().type != EQUALS) {
                throw std::runtime_error("Error de sintaxis en bloque: Se esperaba '='.");
            }
            block_content[key] = parse_value();
        }
        get_token(); // Consume '}'
        std::ostringstream oss;
        oss << "{";
        for (auto const& pair : block_content) {
            oss << pair.first << "=" << pair.second << ",";
        }
        oss << "}";
        return oss.str();
    }

    AstValue parse_list() {
        get_token(); // Consume '['
        std::vector<std::string> list_content;
        while (peek_token().type != RBRACKET) {
            const Token& item_token = peek_token();
            if (item_token.type == IDENTIFIER || item_token.type == NUMBER || item_token.type == STRING) {
                list_content.push_back(get_token().value);
            } else if (item_token.type == LBRACKET) {
                list_content.push_back(parse_list());
            } else {
                throw std::runtime_error("Error de sintaxis en lista: Se esperaba un valor.");
            }
            if (peek_token().type == COMMA) {
                get_token();
            }
        }
        get_token(); // Consume ']'
        std::ostringstream oss;
        oss << "[";
        for (const auto& item : list_content) {
            oss << item << ",";
        }
        oss << "]";
        return oss.str();
    }
};

// -------------------------------------------------------------
// FUNCIONES AUXILIARES Y MAIN
// -------------------------------------------------------------
std::string loadFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Error: No se pudo abrir el archivo " + filepath);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void printTokens(const std::vector<Token>& tokens) {
    std::cout << "Tokens reconocidos:" << std::endl;
    std::map<TokenType, std::string> tokenTypeMap = {
        {IDENTIFIER, "IDENTIFIER"}, {STRING, "STRING"}, {NUMBER, "NUMBER"},
        {OPERATOR, "OPERATOR"}, {LBRACE, "LBRACE"}, {RBRACE, "RBRACE"},
        {LBRACKET, "LBRACKET"}, {RBRACKET, "RBRACKET"}, {COMMA, "COMMA"},
        {EQUALS, "EQUALS"}
    };
    for (const auto& token : tokens) {
        std::cout << "(" << tokenTypeMap[token.type] << ", \"" << token.value << "\")" << std::endl;
    }
}

int main() {
    std::string file_path = "tetris.brik";

    try {
        std::string source_code = loadFile(file_path);

        std::cout << "--- Analisis Lexico (Lexer) ---" << std::endl;
        Lexer lexer(source_code);
        std::vector<Token> tokens = lexer.tokenize();
        printTokens(tokens);

        std::cout << "\n--- Analisis Sintactico (Parser) ---" << std::endl;
        Parser parser(tokens);
        std::map<std::string, AstValue> ast = parser.parse();

        std::cout << "Sintaxis correcta. AST (Tabla de Simbolos) construido." << std::endl;
        std::cout << "Contenido del AST:" << std::endl;
        for (auto const& pair : ast) {
            std::cout << "  " << pair.first << ": " << pair.second << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}