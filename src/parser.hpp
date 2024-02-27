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

    struct NodeExpr;

    struct NodeTermParen {
        NodeExpr* expr;
    };

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
        NodeExpr* lhs;
        NodeExpr* rhs;

    };

    struct NodeBinExpressionMulti{
        NodeExpr* lhs;
        NodeExpr* rhs;
    };

    struct NodeBinExpressionSub{
        NodeExpr* lhs;
        NodeExpr* rhs;
    };

    struct NodeBinExpressionDiv{
        NodeExpr* lhs;
        NodeExpr* rhs;
    };

    struct NodeBinExpression{
        std::variant<NodeBinExpressionAdd*, NodeBinExpressionSub*, NodeBinExpressionMulti*, NodeBinExpressionDiv*> var;
    };

    struct NodeTerm {
        std::variant<NodeTermIntLit*, NodeTermIdent*, NodeTermParen*> var;
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
            else if (auto open_paren = try_consume(TokenType::open_paren)) {
                auto expr = parse_expr();
                if(!expr.has_value()){
                    std::cerr << "Expected Expression" << std::endl;
                    exit(EXIT_FAILURE);
                }
                try_consume(TokenType::close_paren, "Expected `)`");
                auto term_paren = m_allocator.alloc<NodeTermParen>();
                term_paren->expr = expr.value();
                auto term = m_allocator.alloc<NodeTerm>();
                term->var = term_paren;
                return term;
            }
            else {
                return {};
            }
        }

        std::optional<NodeExpr*> parse_expr(int min_prec = 0)
    {

        std::optional<NodeTerm*> term_lhs = parse_term();
        if(!term_lhs.has_value()) {
            return {};
        }

        auto expr_lhs = m_allocator.alloc<NodeExpr>();
        expr_lhs->var = term_lhs.value();

        while(true) {
            std::optional<Token> current_token = peek();
            std::optional<int> prec;

            if(current_token.has_value()) {
                prec = bin_prec(current_token->type);
                if(!prec.has_value() || prec < min_prec){
                    break;
                }

            }
            else{
                break;
            }

            Token op = consume();
            int next_min_prec = prec.value() + 1;
            auto expr_rhs = parse_expr(next_min_prec);

            if(!expr_rhs.has_value()) {
                std::cerr << "Error parsing expression " << std::endl;
                exit(EXIT_FAILURE);
            }

            auto expr = m_allocator.alloc<NodeBinExpression>();
            auto expr_lhs2 = m_allocator.alloc<NodeExpr>();
            

            if(op.type == TokenType::plus) {
                auto add = m_allocator.alloc<NodeBinExpressionAdd>();
                expr_lhs2->var = expr_lhs->var;
                add->lhs = expr_lhs2;
                add->rhs = expr_rhs.value();
                expr->var = add;
                
            } else if (op.type == TokenType::sub) {
                auto sub = m_allocator.alloc<NodeBinExpressionSub>();
                expr_lhs2->var = expr_lhs->var;
                sub->lhs = expr_lhs2;
                sub->rhs = expr_rhs.value();
                expr->var = sub;
            } else if (op.type == TokenType::star) {
                auto multi = m_allocator.alloc<NodeBinExpressionMulti>();
                expr_lhs2->var = expr_lhs->var;
                multi->lhs = expr_lhs2;
                multi->rhs = expr_rhs.value();
                expr->var = multi;
            } else if (op.type == TokenType::div) {
                auto div = m_allocator.alloc<NodeBinExpressionDiv>();
                expr_lhs2->var = expr_lhs->var;
                div->lhs = expr_lhs2;
                div->rhs = expr_rhs.value();
                expr->var = div;
            }

            expr_lhs->var = expr;
            
        }

        return expr_lhs;
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