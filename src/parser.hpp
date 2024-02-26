#pragma once

#include <iostream>
#include <optional>
#include <vector>
#include "./tokenization.hpp"
#include <variant>

namespace node { 

    struct NodeExprIntLit {
        Token int_lit;
    };
    struct NodeExprIdent {
        Token ident;
    };

    struct NodeExpr{
        std::variant<NodeExprIntLit, NodeExprIdent> var;
    };

    struct NodeStatmentExit{
        NodeExpr expr;
    };

      struct NodeStatmentLet{
        Token ident;
        NodeExpr expr;
    };

    struct NodeStatment{
        std::variant<NodeStatmentExit, NodeStatmentLet> var;
    };

    struct NodeProg{
        std::vector<NodeStatment> statements;
    };

};

class Parser {
  

    public: 

        const std::vector<Token> m_tokens;
        size_t m_index = 0;


        inline explicit Parser(std::vector<Token> tokens)
            : m_tokens(std::move(tokens))
        {
        }

        std::optional<node::NodeExpr> parse_expr(){
            if(peek().has_value() && peek().value().type == TokenType::int_lit){

                return node::NodeExpr{.var = node::NodeExprIntLit {.int_lit = consume()}};
            }
            else if(peek().has_value() && peek().value().type == TokenType::ident)
            {
                return node::NodeExpr{.var = node::NodeExprIdent{.ident = consume()}};
            }
            else{
                return {};
            }
        }

        std::optional<node::NodeStatment> parse_statment(){
             if (peek().value().type == TokenType::exit && peek(1).has_value() && peek(1).value().type == TokenType::open_paren){
                    consume();
                    consume();

                     node::NodeStatmentExit statment_exit;
                    if(auto node_expr = parse_expr()){
                       statment_exit = {.expr = node_expr.value()};
                    }
                    else{
                        std::cerr << "Goof Invalid Expression 1 " << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    if(peek().has_value() && peek().value().type == TokenType::close_paren){
                        consume();
                    }
                    else{
                         std::cerr << "Expected `)` " << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    if(peek().has_value() && peek().value().type == TokenType::semi){
                       consume();
                    }
                    else{
                        std::cerr << "Expected ; " << std::endl;
                        exit(EXIT_FAILURE);
                    }

                    return node::NodeStatment{.var = statment_exit};

                } else if (peek().has_value() && peek().value().type == TokenType::let && peek(1).has_value() 
                    && peek(1).value().type == TokenType::ident && peek(2).has_value() 
                    && peek(2).value().type == TokenType::eq)
                    {
                    
                    consume();
                    auto statment_let = node::NodeStatmentLet {.ident = consume()};
                    consume();

                    if(auto expr = parse_expr()){
                        statment_let.expr = expr.value();
                    } else{
                        std::cerr << "Invalid Expression 1 " << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    if(peek().has_value() && peek().value().type == TokenType::semi){
                        consume();
                    } else{
                        std::cerr << "Expected `;` " << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    return node::NodeStatment {.var = statment_let};

                }else{
                    return {};
                }
        }

        std::optional<node::NodeProg> parse_prog(){
            node::NodeProg prog;

            while(peek().has_value()){
                if(auto statment = parse_statment()){
                    prog.statements.push_back(statment.value());
                }
                else {
                    std::cerr << "Invalid Expression 2 " << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            return prog;
        }

    private:
       

         [[nodiscard]] inline std::optional<Token> peek(int offset = 0) const {
                if(m_index + offset >= m_tokens.size()){
                    return {};
                } else {
                    return m_tokens.at(m_index + offset);
                }

            }

            inline Token consume() {
                return m_tokens.at(m_index++);
            }

};