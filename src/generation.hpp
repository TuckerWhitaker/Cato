#pragma once
#include <sstream>
#include <vector>
#include "./tokenization.hpp"
#include "./parser.hpp"
#include <unordered_map>



class Generator{
    public:
        inline Generator(node::NodeProg prog)
        : m_program(std::move(prog))
        {
        }

        void generate_expression(const node::NodeExpr& expr) {

            struct ExpressionVisitor {

                Generator* gen;
               

                void operator()(const node::NodeExprIntLit& expression_int_lit)
                {
                    gen->m_output << "  mov rax, " << expression_int_lit.int_lit.value.value() << "\n";
                    gen->push("rax");
                   
                }
                 void operator()(const node::NodeExprIdent& expression_ident)
                {
                    if(!gen->m_vars.contains(expression_ident.ident.value.value())) {
                        std::cerr << "Undeclared Identifier " << expression_ident.ident.value.value() << std::endl;
                        exit(EXIT_FAILURE);
                    };
                    const auto& var = gen->m_vars.at(expression_ident.ident.value.value());
                    std::stringstream offset;
                    offset << "QWORD [rsp +" << (gen->m_stack_size - var.stack_loc - 1) * 8 << "]\n";
                    gen->push(offset.str());
                 
                }
            };

            ExpressionVisitor visitor({.gen = this});
            std::visit(visitor, expr.var);

        };

        void generate_statment(const node::NodeStatment& statment) {

            struct StatmentVisitor {
                Generator* gen;
                void operator()(const node::NodeStatmentExit& statment_exit)
                {
                    gen->generate_expression(statment_exit.expr);
                    gen->m_output << "  mov rax, 60\n";
                    gen->pop("rdi");
                    gen->m_output << "  syscall\n";
                   
                }
                
                void operator()(const node::NodeStatmentLet& statment_let)
                {
                   
                    if (gen->m_vars.contains(statment_let.ident.value.value())){
                        std::cerr << "Identifier already defined\n" << statment_let.ident.value.value() << std::endl;
                        exit(EXIT_FAILURE);
                    }

                    gen->m_vars.insert({statment_let.ident.value.value(), Var {.stack_loc = gen->m_stack_size}});
                    gen->generate_expression(statment_let.expr);
                }
            };
            StatmentVisitor visitor{.gen = this};
            std::visit(visitor, statment.var);
        };

        [[nodiscard]] std::string generate_program()
        {
            m_output << "global _start\n_start:\n";

            for(const node::NodeStatment& statment : m_program.statements){
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


        const node::NodeProg m_program;
        std::stringstream m_output;
        size_t m_stack_size = 0;
        std::unordered_map<std::string, Var> m_vars {};
};