# -*- coding: utf-8 -*-
"""
Implementado en Python 3.0
@author: Nadie Team
"""
import json
import re
import os

class Tokenizer:
    # ... (El código de la clase Tokenizer no cambia) ...
    def __init__(self, source_code):
        self.source = source_code
        self.tokens = []

    def tokenize(self):
        lines = self.source.splitlines()
        for line in lines:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            
            regex_tokens = re.findall(r'"([^"]*)"|(\d+\.?\d*)|({|}|\[|\]|=|,)|(\w+)', line)
            
            for group in regex_tokens:
                if group[0]:  # Es una cadena de texto
                    self.tokens.append(('STRING', group[0]))
                elif group[1]:  # Es un numero
                    if '.' in group[1]:
                        self.tokens.append(('NUMBER', float(group[1])))
                    else:
                        self.tokens.append(('NUMBER', int(group[1])))
                elif group[2]:  # Es un operador o delimitador
                    self.tokens.append(('OPERATOR', group[2]))
                elif group[3]:  # Es un identificador
                    self.tokens.append(('IDENTIFIER', group[3]))
        
        return self.tokens

class Parser:
    # ... (El código de la clase Parser no cambia) ...
    def __init__(self, tokens):
        self.tokens = tokens
        self.current_token_index = 0
        self.symbol_table = {}

    def parse(self):
        while self.current_token_index < len(self.tokens):
            if self.peek_token() is None:
                break
            
            key_token = self.get_token()
            if key_token[0] != 'IDENTIFIER':
                raise SyntaxError(f"Error de sintaxis: Se esperaba un identificador, se encontro {key_token[1]}")
            
            eq_token = self.get_token()
            if eq_token[1] != '=':
                raise SyntaxError(f"Error de sintaxis: Se esperaba '=', se encontro {eq_token[1]}")
            
            value = self.parse_value()
            self.symbol_table[key_token[1]] = value
            
        return self.symbol_table

    def get_token(self):
        if self.current_token_index < len(self.tokens):
            token = self.tokens[self.current_token_index]
            self.current_token_index += 1
            return token
        return None

    def peek_token(self):
        if self.current_token_index < len(self.tokens):
            return self.tokens[self.current_token_index]
        return None

    def parse_value(self):
        token = self.peek_token()
        if token is None:
            raise SyntaxError("Error de sintaxis: Se esperaba un valor despues de '='")

        token_type, token_value = token
        
        if token_type == 'STRING' or token_type == 'NUMBER':
            self.current_token_index += 1
            return token_value
        elif token_type == 'OPERATOR' and token_value == '{':
            return self.parse_block()
        elif token_type == 'OPERATOR' and token_value == '[':
            return self.parse_list()
        
        raise SyntaxError(f"Error de sintaxis: Valor inesperado '{token_value}'")

    def parse_block(self):
        self.get_token() # Consume '{'
        block_content = {}
        
        while self.peek_token() and self.peek_token()[1] != '}':
            key_token = self.get_token()
            if key_token[0] != 'IDENTIFIER':
                raise SyntaxError(f"Error de sintaxis en bloque: Se esperaba un identificador, se encontro {key_token[1]}")
            
            eq_token = self.get_token()
            if eq_token[1] != '=':
                raise SyntaxError(f"Error de sintaxis en bloque: Se esperaba '=', se encontro {eq_token[1]}")
            
            value = self.parse_value()
            block_content[key_token[1]] = value

        self.get_token() # Consume '}'
        return block_content
    
    def parse_list(self):
        self.get_token() # Consume '['
        list_content = []
        
        while self.peek_token() and self.peek_token()[1] != ']':
            token_type, token_value = self.peek_token()
            
            if token_type == 'IDENTIFIER':
                # Nuevo: Busca el identificador en la tabla de simbolos
                self.get_token()
                if token_value not in self.symbol_table:
                    raise NameError(f"Error semantico: El identificador '{token_value}' no ha sido definido.")
                list_content.append(self.symbol_table[token_value])
            else:
                item = self.parse_value()
                list_content.append(item)
            
            if self.peek_token() and self.peek_token()[1] == ',':
                self.get_token() # Consume ','
                
        self.get_token() # Consume ']'
        return list_content


def load_file_content(filepath):
    """
    Carga el contenido de un archivo de texto.
    Maneja el error si el archivo no existe.
    """
    if not os.path.exists(filepath):
        print(f"Error: El archivo '{filepath}' no se encontro. Asegurate de que el archivo exista en la misma carpeta que el script.")
        return None
    
    with open(filepath, 'r', encoding='utf-8') as file:
        return file.read()

def save_ast_to_file(ast, filepath):
    """
    Guarda el AST en un archivo de texto en formato JSON.
    """
    try:
        with open(filepath, 'w', encoding='utf-8') as file:
            json.dump(ast, file, indent=4)
        print(f"AST guardado exitosamente en '{filepath}'")
    except Exception as e:
        print(f"Error al guardar el archivo: {e}")

# --- Zona de ejecucion ---
# 1. Especifica la ruta del archivo a procesar
file_path = "tetris.brik"
ast_file_path = "arbol.ast"

# 2. Carga el contenido del archivo
source_code = load_file_content(file_path)

if source_code:
    # 3. Analisis Lexico
    print("--- Analisis Lexico (Lexer) ---")
    tokenizer = Tokenizer(source_code)
    tokens = tokenizer.tokenize()
    print("Tokens reconocidos:")
    for token in tokens:
        print(token)
    
    # 4. Analisis Sintactico y gestion de Tabla de Simbolos
    print("\n--- Analisis Sintactico (Parser) ---")
    parser = Parser(tokens)
    try:
        ast_and_symbol_table = parser.parse()
        print("Sintaxis correcta. Se ha construido el Arbol de Sintaxis Abstracta (AST) / Tabla de Simbolos.")
        print("Contenido del AST:")
        print(json.dumps(ast_and_symbol_table, indent=4))
        
        # 5. Guardar el AST en el archivo
        save_ast_to_file(ast_and_symbol_table, ast_file_path)
        
    except (SyntaxError, NameError) as e:
        print(f"Error en la sintaxis: {e}")