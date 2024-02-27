#pragma once
#include <sstream>
#include <vector>
#include "./tokenization.hpp"
#include "./parser.hpp"
#include <unordered_map>
#include <assert.h>


class Generator{
    public:
        inline explicit Generator(NodeProg prog)
        : m_program(std::move(prog))
        {
        }

    void gen_term(const NodeTerm* term) {
            struct TermVisitor {
                Generator* gen;
                void operator()(const NodeTermIntLit* term_int_lit) const {
                    gen->m_output << "  mov rax, " << term_int_lit->int_lit.value.value() << "\n";
                    gen->push("rax");
                }
                void operator()(const NodeTermIdent* term_ident) const {
                    if (!gen->m_vars.contains(term_ident->ident.value.value())) {
                        std::cerr << "Undeclared identifier: " << term_ident->ident.value.value() << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    const auto& var = gen->m_vars.at(term_ident->ident.value.value());
                    std::stringstream offset;
                    offset << "QWORD [rsp + " << (gen->m_stack_size - var.stack_loc - 1) * 8 << "]\n";
                    gen->push(offset.str());
                }
                void operator()(const NodeTermParen* term_paren) const
                {
                    gen->generate_expression(term_paren->expr);
                }
            };

        TermVisitor visitor({.gen = this});
        std::visit(visitor, term->var);
    }

    void generate_bin_expression(const NodeBinExpression* bin_expr){
        struct BinExpressionVisitor {
            Generator* gen;
            void operator()(const NodeBinExpressionSub* sub) const 
            {
                gen->generate_expression(sub->rhs);
                gen->generate_expression(sub->lhs);
                gen->pop("rax");
                gen->pop("rbx");
                gen->m_output << "  sub rax, rbx\n";
                gen->push("rax");

            }
            void operator()(const NodeBinExpressionAdd* add) const 
            {
                gen->generate_expression(add->rhs);
                gen->generate_expression(add->lhs);
                
                gen->pop("rax");
                gen->pop("rbx");
                gen->m_output << "  add rax, rbx\n";
                gen->push("rax");

            }
            void operator()(const NodeBinExpressionMulti* multi) const 
            {
                gen->generate_expression(multi->rhs);
                gen->generate_expression(multi->lhs);
                
                gen->pop("rax");
                gen->pop("rbx");
                gen->m_output << "  mul rbx\n";
                gen->push("rax");

            }
            void operator()(const NodeBinExpressionDiv* div) const 
            {
                gen->generate_expression(div->rhs);
                gen->generate_expression(div->lhs);
               
                gen->pop("rax");
                gen->pop("rbx");
                gen->m_output << "  div rbx\n";
                gen->push("rax");

            }
            void operator()(const NodeTermParen* term_paren) const 
            {
                gen->generate_expression(term_paren->expr);
            }
        };

        BinExpressionVisitor visitor { .gen = this };
        std::visit(visitor, bin_expr->var);
    }

    void generate_expression(const NodeExpr* expr)
    {
        struct ExprVisitor {
            Generator* gen;
            void operator()(const NodeTerm* term) const
            {
                gen->gen_term(term);
            }
            void operator()(const NodeBinExpression* bin_expr) const
            {
                gen->generate_bin_expression(bin_expr);
            }
        };

        ExprVisitor visitor { .gen = this };
        std::visit(visitor, expr->var);
    }

    void generate_statment(const NodeStatment* stmt)
    {
        struct StmtVisitor {
            Generator* gen;
            void operator()(const NodeStatmentExit* stmt_exit) const
            {
                gen->generate_expression(stmt_exit->expr);
                gen->m_output << "  mov rax, 60\n";
                gen->pop("rdi");
                gen->m_output << "  syscall\n";
            }
            void operator()(const NodeStatmentLet* stmt_let) const
            {
                if (gen->m_vars.contains(stmt_let->ident.value.value())) {
                    std::cerr << "Identifier already used: " << stmt_let->ident.value.value() << std::endl;
                    exit(EXIT_FAILURE);
                }
                gen->m_vars.insert({ stmt_let->ident.value.value(), Var { .stack_loc = gen->m_stack_size } });
                gen->generate_expression(stmt_let->expr);
            }
        };

        StmtVisitor visitor { .gen = this };
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


        struct Var {
            size_t stack_loc;
        };


        const NodeProg m_program;
        std::stringstream m_output;
        size_t m_stack_size = 0;
        std::unordered_map<std::string, Var> m_vars {};
};