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

    struct NodeTermStringLit {
        std::string value;
    };

    struct NodeStatementExit{
        NodeExpr* expr;
    };

    struct NodeStatementInt{
        Token ident;
        NodeExpr* expr;
    };

    struct NodeStatementAssign{
        Token ident;
        NodeExpr* expr;
    };

    struct NodeStatement;
    struct NodeIfPred;
    struct NodeFunctionDecl;


    struct NodeScope{
        std::vector<NodeStatement*> statements;
    };

    struct NodeIfPredicateElif{
        NodeExpr* expr;
        NodeScope* scope;
        std::optional<NodeIfPred*> pred;
    };

    struct NodeIfPredicateElse{
        NodeScope* scope;
    };

    struct NodeIfPred{
        std::variant<NodeIfPredicateElif*, NodeIfPredicateElse*> var;
    };


    struct NodeStatementIf{
        NodeExpr* expr;
        NodeScope* scope;
        std::optional<NodeIfPred*> pred;
    };

    struct NodeStatementFor{
    NodeStatement* init;
    NodeExpr* condition;
    NodeStatement* iteration;
    NodeScope* scope;

    NodeStatementFor(NodeStatement* init, NodeExpr* condition, NodeStatement* iteration, NodeScope* scope)
        : init(init), condition(condition), iteration(iteration), scope(scope) {}
    };

    struct NodeStatement{
        std::variant<NodeStatementExit*, NodeStatementInt*, NodeScope*, NodeStatementIf*, NodeStatementAssign*, NodeStatementFor*, NodeFunctionDecl*> var;
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

    struct NodeBinExpressionNotEquals{
        NodeExpr* lhs;
        NodeExpr* rhs;
    };
    struct NodeBinExpressionEquals{
        NodeExpr* lhs;
        NodeExpr* rhs;
    };
    struct NodeBinExpressionLess{
        NodeExpr* lhs;
        NodeExpr* rhs;
    };
    struct NodeBinExpressionGreater{
        NodeExpr* lhs;
        NodeExpr* rhs;
    };

    struct NodeBinExpression{
        std::variant<NodeBinExpressionAdd*, NodeBinExpressionSub*,
        NodeBinExpressionMulti*, NodeBinExpressionDiv*,
        NodeBinExpressionEquals*, NodeBinExpressionNotEquals*,
        NodeBinExpressionLess*, NodeBinExpressionGreater*> var;
    };

    

    struct NodeFunctionDecl {
        Token ident;
        std::vector<Token> params;
        NodeScope* body;
    };

    struct NodeFunctionCall {
        Token ident;
        std::vector<NodeExpr*> args;
    };

    struct NodeTerm {
        std::variant<NodeTermIntLit*, NodeTermIdent*, NodeTermParen*, NodeTermStringLit*, NodeFunctionCall*> var;
    };

    struct NodeExpr {
        std::variant<NodeTerm*, NodeBinExpression*> var;
    };

    struct NodeProg{
        std::vector<NodeStatement*> statements;
        std::vector<NodeFunctionDecl*> functions;
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
                if (try_consume(TokenType::open_paren)) { // Function call
                    auto func_call = m_allocator.alloc<NodeFunctionCall>();
                    func_call->ident = ident.value();
                    // Parse arguments
                    while (!try_consume(TokenType::close_paren)) {
                        if (auto arg = parse_expr()) {
                            func_call->args.push_back(arg.value());
                        } else {
                            std::cerr << "Expected argument expression" << std::endl;
                            exit(EXIT_FAILURE);
                        }
                        try_consume(TokenType::comma); // Optional comma
                    }

                    auto term = m_allocator.alloc<NodeTerm>();
                    term->var = func_call;
                    return term;
                } else {
                    // Handle identifier (variable)
                    auto expr_ident = m_allocator.alloc<NodeTermIdent>();
                    expr_ident->ident = ident.value();
                    auto term = m_allocator.alloc<NodeTerm>();
                    term->var = expr_ident;
                    return term;
                }
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
                try_consume(TokenType::close_paren, "Expected `)` 1");
                auto term_paren = m_allocator.alloc<NodeTermParen>();
                term_paren->expr = expr.value();
                auto term = m_allocator.alloc<NodeTerm>();
                term->var = term_paren;
                return term;
            } else if (auto string_lit = try_consume(TokenType::string_lit)) {
                auto term_string_lit = m_allocator.alloc<NodeTermStringLit>();
                if (string_lit.has_value() && string_lit.value().value.has_value()) {
                term_string_lit->value = string_lit.value().value.value();
                } else {
                    std::cerr << "String literal without a value." << std::endl;
                    exit(EXIT_FAILURE);
                }
                auto term = m_allocator.alloc<NodeTerm>();
                term->var = term_string_lit;
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
                std::cout << "Created NodeBinExpressionAdd" << std::endl;
                
            } else if (op.type == TokenType::sub) {
                auto sub = m_allocator.alloc<NodeBinExpressionSub>();
                expr_lhs2->var = expr_lhs->var;
                sub->lhs = expr_lhs2;
                sub->rhs = expr_rhs.value();
                expr->var = sub;
                std::cout << "Created NodeBinExpressionSub" << std::endl;

            } else if (op.type == TokenType::star) {
                auto multi = m_allocator.alloc<NodeBinExpressionMulti>();
                expr_lhs2->var = expr_lhs->var;
                multi->lhs = expr_lhs2;
                multi->rhs = expr_rhs.value();
                expr->var = multi;
                std::cout << "Created NodeBinExpressionMulti" << std::endl;
            } else if (op.type == TokenType::div) {
                auto div = m_allocator.alloc<NodeBinExpressionDiv>();
                expr_lhs2->var = expr_lhs->var;
                div->lhs = expr_lhs2;
                div->rhs = expr_rhs.value();
                expr->var = div;
                std::cout << "Created NodeBinExpressionDiv" << std::endl;
            } else if (op.type == TokenType::equality) {
                auto condition = m_allocator.alloc<NodeBinExpressionEquals>();
                expr_lhs2->var = expr_lhs->var;
                condition->lhs = expr_lhs2;
                condition->rhs = expr_rhs.value();
                expr->var = condition;
                std::cout << "Created NodeBinExpressionEquals" << std::endl;
            } else if (op.type == TokenType::not_equal) {
                auto condition = m_allocator.alloc<NodeBinExpressionNotEquals>();
                expr_lhs2->var = expr_lhs->var;
                condition->lhs = expr_lhs2;
                condition->rhs = expr_rhs.value();
                expr->var = condition;
                std::cout << "Created NodeBinExpressionNotEquals" << std::endl;
            } else if (op.type == TokenType::greater_than) {
                auto condition = m_allocator.alloc<NodeBinExpressionGreater>();
                expr_lhs2->var = expr_lhs->var;
                condition->lhs = expr_lhs2;
                condition->rhs = expr_rhs.value();
                expr->var = condition;
                std::cout << "Created NodeBinExpressionGreater" << std::endl;
            } else if (op.type == TokenType::less_than) {
                auto condition = m_allocator.alloc<NodeBinExpressionLess>();
                expr_lhs2->var = expr_lhs->var;
                condition->lhs = expr_lhs2;
                condition->rhs = expr_rhs.value();
                expr->var = condition;
                std::cout << "Created NodeBinExpressionLess" << std::endl;
            }

            expr_lhs->var = expr;
            
        }

        return expr_lhs;
    }

        std::optional<NodeScope*> parse_scope(){

            if(!try_consume(TokenType::open_curly).has_value()) {
                return {};
            }

            auto scope = m_allocator.alloc<NodeScope>();
            while(auto statment = parse_statement()){
                scope->statements.push_back(statment.value());
            }
            try_consume(TokenType::close_curly, "Expected `}`");
            return scope;
    
        }

        std::optional<NodeIfPred*> parse_if_predicate()
        {
            if(try_consume(TokenType::elif_)){
                try_consume(TokenType::open_paren, "Expected `(`");
                auto elif = m_allocator.alloc<NodeIfPredicateElif>();
                if(auto expr = parse_expr()){
                    elif->expr = expr.value();
                }
                else{
                    std::cerr << "Expected Expression" << std::endl;
                    exit(EXIT_FAILURE);
                }
                try_consume(TokenType::close_paren, "Expected `)` 2");
                if(auto scope = parse_scope()){
                    elif->scope = scope.value();
                }else{
                    std::cerr << "Expected scope" << std::endl;
                    exit(EXIT_FAILURE);
                }
                elif->pred = parse_if_predicate();
                auto pred = m_allocator.emplace<NodeIfPred>(elif);
                return pred;
            }
            if(try_consume(TokenType::else_)){
                auto else_ = m_allocator.alloc<NodeIfPredicateElse>();
                if(auto scope = parse_scope()){
                    else_->scope = scope.value();
                }else{
                    std::cerr << "Expected scope" << std::endl;
                    exit(EXIT_FAILURE);
                }
                auto pred = m_allocator.emplace<NodeIfPred>(else_);
                return pred;
            }
            return {};
        };

        std::optional<NodeStatement*> parse_for_statement() {
            if (!try_consume(TokenType::for_)) {
                return {};
            }

            try_consume(TokenType::open_paren, "Expected `(` in for loop");

            auto init = parse_statement();
            if (!init.has_value()) {
                std::cerr << "Expected initialization in for loop" << std::endl;
                exit(EXIT_FAILURE);
            }


            auto condition = parse_expr();
            if (!condition.has_value()) {
                std::cerr << "Expected condition in for loop" << std::endl;
                exit(EXIT_FAILURE);
            }
            try_consume(TokenType::semi, "Expected `;` after condition in for loop");


            auto iteration = parse_statement(false);
            if (!iteration.has_value()) {
                std::cerr << "Expected iteration in for loop" << std::endl;
                exit(EXIT_FAILURE);
            }
            try_consume(TokenType::close_paren, "Expected `)` after iteration in for loop");

            auto scope = parse_scope();
            if (!scope.has_value()) {
                std::cerr << "Expected scope in for loop" << std::endl;
                exit(EXIT_FAILURE);
            }

            auto stmt_for = m_allocator.alloc<NodeStatementFor>();
            stmt_for->init = init.value();
            stmt_for->condition = condition.value();
            stmt_for->iteration = iteration.value();
            stmt_for->scope = scope.value();

            auto stmt = m_allocator.alloc<NodeStatement>();
            stmt->var = stmt_for;
            return stmt;
        }


        std::optional<NodeStatement*> parse_statement(bool expect_semicolon = true){
             if (peek().value().type == TokenType::exit && peek(1).has_value() && peek(1).value().type == TokenType::open_paren){
                    consume();
                    consume();

                    auto stmt_exit = m_allocator.alloc<NodeStatementExit>();

                    if(auto node_expr = parse_expr()){
                        stmt_exit->expr = node_expr.value();
                    }
                    else{
                        std::cerr << "Goof Invalid Expression 1 " << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    try_consume(TokenType::close_paren, "Expected `)` 3");
                    if (expect_semicolon) {
                        try_consume(TokenType::semi, "Expected `;`");
                    }
                    auto stmt = m_allocator.alloc<NodeStatement>();
                    stmt->var = stmt_exit;
                    return stmt;

                } else if (peek().has_value() && peek().value().type == TokenType::int_ && peek(1).has_value() 
                    && peek(1).value().type == TokenType::ident && peek(2).has_value() 
                    && peek(2).value().type == TokenType::eq)
                    {
                    
                    consume();
                    auto statment_int = m_allocator.alloc<NodeStatementInt>();
                    statment_int->ident = consume();
                    consume();

                    if(auto expr = parse_expr()){
                        statment_int->expr = expr.value();
                    } else{
                        std::cerr << "Invalid Expression 1 " << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    if (expect_semicolon) {
                        try_consume(TokenType::semi, "Expected `;`");
                    }
                    auto stmt = m_allocator.alloc<NodeStatement>();
                    stmt->var = statment_int;
                    return stmt;

                }
                else if(peek().has_value() && peek().value().type == TokenType::ident 
                && peek(1).has_value() 
                && peek(1).value().type == TokenType::eq)
                {
                    const auto assign = m_allocator.alloc<NodeStatementAssign>();
                    assign->ident = consume();
                    consume();
                    if(const auto expr = parse_expr()){
                        assign->expr = expr.value();
                    } else {
                        std::cerr << "Expected Expression" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    if (expect_semicolon) {
                        try_consume(TokenType::semi, "Expected `;`");
                    }
                    auto stmt = m_allocator.emplace<NodeStatement>(assign);
                    return stmt;
                }
                else if (auto func_decl = parse_function_decl()) {
                    auto stmt = m_allocator.alloc<NodeStatement>();
                    stmt->var = func_decl.value();
                    return stmt;
                }
                else if(peek().has_value() && peek().value().type == TokenType::open_curly){
                    if(auto scope = parse_scope()){
                    auto statment = m_allocator.alloc<NodeStatement>();
                    statment->var = scope.value();
                    return statment;
                    }else{
                        std::cerr << "Invalid Scope" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                        
                }
                else if(auto if_ = try_consume(TokenType::if_)){
                    try_consume(TokenType::open_paren, "Expected `(`");
                    auto statment_if = m_allocator.alloc<NodeStatementIf>();
                    if(auto expr = parse_expr()){
                        statment_if->expr = expr.value();
                    }
                    else{
                        std::cerr << "Invalid Expression" << std::endl;
                        exit(EXIT_FAILURE);
                    }

                    try_consume(TokenType::close_paren, "Expected `)` 4");
                    if(auto scope = parse_scope()){
                        statment_if->scope = scope.value();
                    }
                    else{
                        std::cerr << "Invalid Scope" << std::endl;
                        exit(EXIT_FAILURE);
                    }

                    statment_if->pred = parse_if_predicate();
                    auto statment = m_allocator.alloc<NodeStatement>();
                    statment->var = statment_if;
                    return statment;

                }else if (auto for_stmt = parse_for_statement()) {
                    return for_stmt;
                }
                
                else{
                    return {};
                }
        }

        std::optional<NodeFunctionDecl*> parse_function_decl() {
            if (!try_consume(TokenType::function)) {
                return {};
            }

            auto func_decl = m_allocator.alloc<NodeFunctionDecl>();
            func_decl->ident = try_consume(TokenType::ident, "Expected function name");

            try_consume(TokenType::open_paren, "Expected `(` after function name");

            while (!try_consume(TokenType::close_paren)) {
                func_decl->params.push_back(try_consume(TokenType::ident, "Expected parameter name"));
                try_consume(TokenType::comma);
            }
            if (auto scope = parse_scope()) {
            func_decl->body = scope.value();
            } else {
                std::cerr << "Expected function body" << std::endl;
                exit(EXIT_FAILURE);
            }
            return func_decl;
        }

        std::optional<NodeProg> parse_prog(){
            NodeProg prog;

            while(peek().has_value()){
                if(auto statment = parse_statement()){
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