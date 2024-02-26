#pragma once

#include <iostream>
#include <optional>
#include <vector>
#include "./tokenization.hpp"
#include <variant>
#include "./arena.hpp"



  

    struct NodeTermIntLit {
    Token int_lit;
    };

    struct NodeTermIdent {
        Token ident;
    };


    struct NodeTerm {
    std::variant<NodeTermIntLit*, NodeTermIdent*> var;
    };

    struct NodeExpr;


    struct NodeStatmentExit{
        NodeExpr* expr;
    };

      struct NodeStatmentLet{
        Token ident;
        NodeExpr* expr;
    };

    struct NodeStatment{
        std::variant<NodeStatmentExit*, NodeStatmentLet*> var;
    };

    struct NodeProg{
        std::vector<NodeStatment*> statements;
    };


    struct NodeBinExpressionAdd{
        NodeExpr* lhs; //left hand side
        NodeExpr* rhs; //right hand side

    };

    struct NodeBinExpressionMulti{
        NodeExpr* lhs; //left hand side
        NodeExpr* rhs; //right hand side
    };

    struct NodeBinExpression{
        NodeBinExpressionAdd* add;
    };

    struct NodeExpr {
    std::variant<NodeTerm*, NodeBinExpression*> var;
    };

    



class Parser {
  

    public: 

        const std::vector<Token> m_tokens;
        size_t m_index = 0;
        ArenaAllocator m_allocator;


        inline explicit Parser(std::vector<Token> tokens)
            : m_tokens(std::move(tokens)),
            m_allocator(1024 * 1024 * 4) //4mb

        {
        }

        std::optional<NodeTerm*> parse_term()
        {
            if (auto int_lit = try_consume(TokenType::int_lit)) {
                auto term_int_lit = m_allocator.alloc<NodeTermIntLit>();
                term_int_lit->int_lit = int_lit.value();
                auto term = m_allocator.alloc<NodeTerm>();
                term->var = term_int_lit;
                return term;
            }
            else if (auto ident = try_consume(TokenType::ident)) {
                auto expr_ident = m_allocator.alloc<NodeTermIdent>();
                expr_ident->ident = ident.value();
                auto term = m_allocator.alloc<NodeTerm>();
                term->var = expr_ident;
                return term;
            }
            else {
                return {};
            }
        }

        std::optional<NodeExpr*> parse_expr(){
        if (auto term = parse_term()) {
            if (try_consume(TokenType::plus).has_value()) {
                auto bin_expr = m_allocator.alloc<NodeBinExpression>();
                auto bin_expr_add = m_allocator.alloc<NodeBinExpressionAdd>();
                auto lhs_expr = m_allocator.alloc<NodeExpr>();
                lhs_expr->var = term.value();
                bin_expr_add->lhs = lhs_expr;
                if (auto rhs = parse_expr()) {
                    bin_expr_add->rhs = rhs.value();
                    bin_expr->add = bin_expr_add;
                    auto expr = m_allocator.alloc<NodeExpr>();
                    expr->var = bin_expr;
                    return expr;
                }
                else {
                    std::cerr << "Expected expression" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            else {
                auto expr = m_allocator.alloc<NodeExpr>();
                expr->var = term.value();
                return expr;
            }
        }
        else {
            return {};
        }
            
        }

        std::optional<NodeStatment*> parse_statment(){
             if (peek().value().type == TokenType::exit && peek(1).has_value() && peek(1).value().type == TokenType::open_paren){
                    consume();
                    consume();

                    auto stmt_exit = m_allocator.alloc<NodeStatmentExit>();

                    if(auto node_expr = parse_expr()){
                        stmt_exit->expr = node_expr.value();
                    }
                    else{
                        std::cerr << "Goof Invalid Expression 1 " << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    try_consume(TokenType::close_paren, "Expected `)`");
                    try_consume(TokenType::semi, "Expected `;`");
                    auto stmt = m_allocator.alloc<NodeStatment>();
                    stmt->var = stmt_exit;
                    return stmt;

                } else if (peek().has_value() && peek().value().type == TokenType::let && peek(1).has_value() 
                    && peek(1).value().type == TokenType::ident && peek(2).has_value() 
                    && peek(2).value().type == TokenType::eq)
                    {
                    
                    consume();
                    auto statment_let = m_allocator.alloc<NodeStatmentLet>();
                    statment_let->ident = consume();
                    consume();

                    if(auto expr = parse_expr()){
                        statment_let->expr = expr.value();
                    } else{
                        std::cerr << "Invalid Expression 1 " << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    try_consume(TokenType::semi, "Expected `;`");
                    auto stmt = m_allocator.alloc<NodeStatment>();
                    stmt->var = statment_let;
                    return stmt;

                }else{
                    return {};
                }
        }

        std::optional<NodeProg> parse_prog(){
            NodeProg prog;

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

            inline Token try_consume(TokenType type, const std::string& err_msg)
            {
                if (peek().has_value() && peek().value().type == type) {
                    return consume();
                }
                else {
                    std::cerr << err_msg << std::endl;
                    exit(EXIT_FAILURE);
                }
            }

            inline std::optional<Token> try_consume(TokenType type)
            {
                if (peek().has_value() && peek().value().type == type) {
                    return consume();
                }
                else {
                    return {};
                }
            }
};