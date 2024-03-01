#pragma once
#include <sstream>
#include <vector>
#include "./tokenization.hpp"
#include "./parser.hpp"
#include <map>
#include <assert.h>
#include <algorithm>


class Generator{
    public:
        inline explicit Generator(NodeProg prog)
        : m_program(std::move(prog))
        {
        }

    void gen_term(const NodeTerm* term) {
            struct TermVisitor {
                Generator& gen;
                void operator()(const NodeTermIntLit* term_int_lit) const {
                    gen.m_output << "  mov rax, " << term_int_lit->int_lit.value.value() << "\n";
                    gen.push("rax");
                }
                void operator()(const NodeTermIdent* term_ident) const {
                    auto it = std::find_if(gen.m_vars.cbegin(), gen.m_vars.cend(), [&](const Var& var){
                        return var.name == term_ident->ident.value.value();
                    });

                    if(it == gen.m_vars.cend()){
                        std::cerr << "Undeclared identifier: " << term_ident->ident.value.value() << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    
                    std::stringstream offset;
                    offset << "QWORD [rsp + " << (gen.m_stack_size - (*it).stack_loc - 1) * 8 << "]";
                    gen.push(offset.str());
                }
                void operator()(const NodeTermParen* term_paren) const
                {
                    gen.generate_expression(term_paren->expr);
                }
            };

        TermVisitor visitor({.gen = *this});
        std::visit(visitor, term->var);
    }

    void generate_bin_expression(const NodeBinExpression* bin_expr){
        struct BinExpressionVisitor {
            Generator& gen;
            void operator()(const NodeBinExpressionSub* sub) const 
            {
                gen.generate_expression(sub->rhs);
                gen.generate_expression(sub->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "  sub rax, rbx\n";
                gen.push("rax");

            }
            void operator()(const NodeBinExpressionAdd* add) const 
            {
                gen.generate_expression(add->rhs);
                gen.generate_expression(add->lhs);
                
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "  add rax, rbx\n";
                gen.push("rax");

            }
            void operator()(const NodeBinExpressionMulti* multi) const 
            {
                gen.generate_expression(multi->rhs);
                gen.generate_expression(multi->lhs);
                
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "  mul rbx\n";
                gen.push("rax");

            }
            void operator()(const NodeBinExpressionDiv* div) const 
            {
                gen.generate_expression(div->rhs);
                gen.generate_expression(div->lhs);
               
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "  div rbx\n";
                gen.push("rax");

            }
            void operator()(const NodeTermParen* term_paren) const 
            {
                gen.generate_expression(term_paren->expr);
            }
        };

        BinExpressionVisitor visitor { .gen = *this };
        std::visit(visitor, bin_expr->var);
    }

    void generate_expression(const NodeExpr* expr)
    {
        struct ExprVisitor {
            Generator& gen;
            void operator()(const NodeTerm* term) const
            {
                gen.gen_term(term);
            }
            void operator()(const NodeBinExpression* bin_expr) const
            {
                gen.generate_bin_expression(bin_expr);
            }
        };

        ExprVisitor visitor { .gen = *this };
        std::visit(visitor, expr->var);
    }

    void generate_scope(const NodeScope* scope){
        begin_scope();
        for(const NodeStatment* statment: scope->statements){
            generate_statment(statment);
        }
        end_scope();
    }

    void generate_if_predicate(const NodeIfPred* pred, const std::string& end_label){
        struct PredVisitor {
            Generator& gen;
            const std::string& end_label;

            void operator()(const NodeIfPredicateElif* elif) const{
                gen.generate_expression(elif->expr);
                gen.pop("rax");
                const std::string label = gen.create_label();
                gen.m_output << "  test rax, rax\n";
                gen.m_output << "  jz " << label << "\n";
                gen.generate_scope(elif->scope);
                gen.m_output << "  jmp "<< end_label <<  "\n";
                
                if(elif->pred.has_value()){
                    gen.m_output << label << ":\n";
                    gen.generate_if_predicate(elif->pred.value(), end_label);
                }
                
            }
            void operator()(const NodeIfPredicateElse* else_) const{
                gen.generate_scope(else_->scope);

            }
        };

        PredVisitor visitor{.gen = *this, .end_label = end_label};
        std::visit(visitor, pred->var);


    }


    void generate_statment(const NodeStatment* stmt)
    {
        struct StmtVisitor {
            Generator& gen;
            void operator()(const NodeStatmentExit* stmt_exit) const
            {
                gen.generate_expression(stmt_exit->expr);
                gen.m_output << "  mov rax, 60\n";
                gen.pop("rdi");
                gen.m_output << "  syscall\n";
            }
            void operator()(const NodeStatmentLet* stmt_let) const
            {

                auto it = std::find_if(gen.m_vars.cbegin(), gen.m_vars.cend(), [&](const Var& var){
                    return var.name == stmt_let->ident.value.value();
                });

                if (it != gen.m_vars.cend()) {
                    std::cerr << "Identifier already used: " << stmt_let->ident.value.value() << std::endl;
                    exit(EXIT_FAILURE);
                }
                gen.m_vars.push_back({.name = stmt_let->ident.value.value(), .stack_loc = gen.m_stack_size });
                gen.generate_expression(stmt_let->expr);
            }
            void operator()(const NodeStatmentIf* statment_if) const
            {
                gen.generate_expression(statment_if->expr);
                gen.pop("rax");
                std::string label = gen.create_label();
                gen.m_output << "  test rax, rax\n";
                gen.m_output << "  jz " << label << "\n";
                gen.generate_scope(statment_if->scope);
                if (statment_if->pred.has_value()) {
                    const std::string end_label = gen.create_label();
                    gen.m_output << "    jmp " << end_label << "\n";
                    gen.m_output << label << ":\n";
                    gen.generate_if_predicate(statment_if->pred.value(), end_label);
                    gen.m_output << end_label << ":\n";
                }
                else {
                    gen.m_output << label << ":\n";
                }
                gen.m_output << "    ;; /if\n";
                

            }
            void operator()(const NodeScope* scope) const
            {
                gen.generate_scope(scope);
            }
            void operator()(const NodeStatmentAssign* stmt_assign) const
            {
                auto it = std::find_if(gen.m_vars.cbegin(), gen.m_vars.cend(), [&](const Var& var){
                    return var.name == stmt_assign->ident.value.value();
                });
                
                if(it == gen.m_vars.end()){
                    std::cerr << "Undeclared identifier: " << stmt_assign->ident.value.value() << std::endl;
                }
                gen.generate_expression(stmt_assign->expr);
                gen.pop("rax");
                gen.m_output << "  mov [rsp + " << (gen.m_stack_size - (*it).stack_loc - 1) * 8 << "], rax\n";

            }
            
        };

        StmtVisitor visitor { .gen = *this };
        std::visit(visitor, stmt->var);
    }

        [[nodiscard]] std::string generate_program()
        {
            m_output << "global _start\n_start:\n";

            for(const NodeStatment* statment : m_program.statements){
               generate_statment(statment);
             
            }
            m_output << "  mov rax, 60\n";
            m_output << "  mov rdi, 0\n";
            m_output << "  syscall\n";
            return m_output.str();
        }



    private:

        void push(const std::string& reg) {
            m_output << "  push " << reg << "\n";
            m_stack_size++;
        }

        void pop(const std::string& reg) {
            m_output << "  pop " << reg << "\n";
            m_stack_size--;
        }

        void begin_scope(){
            m_scopes.push_back(m_vars.size());
        }

        void end_scope(){
            size_t pop_count = m_vars.size() - m_scopes.back();
            m_output << "  add rsp," << pop_count * 8 << "\n";
            m_stack_size -= pop_count;
            for(int i = 0; i < pop_count; i++){
                m_vars.pop_back();
            }
            m_scopes.pop_back();
        }

        std::string create_label(){
            std::stringstream ss;
            ss << "label" << m_label_count++;
            return ss.str();
        }



        struct Var {
            std::string name;
            size_t stack_loc;
        };


        const NodeProg m_program;
        std::stringstream m_output;
        size_t m_stack_size = 0;
        std::vector<Var> m_vars {};
        std::vector<size_t> m_scopes {};
        int m_label_count = 0;
};