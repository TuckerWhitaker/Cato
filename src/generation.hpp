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
                void operator()(const NodeTermStringLit* term_string_lit) const {
                    auto it = gen.m_string_literals.find(term_string_lit->value);
                    if (it == gen.m_string_literals.end()) {
                        // Generate a new label for the string
                        std::string label = gen.create_label();
                        gen.m_string_literals[term_string_lit->value] = label;
                        // Store the string in the data section
                        gen.m_data << label << ": db '" << term_string_lit->value << "', 0\n"; // Null-terminated string
                    }

                    // Load the address of the string into rax
                    gen.m_output << "  lea rax, [" << gen.m_string_literals[term_string_lit->value] << "]\n";
                    gen.push("rax");
                }
                void operator()(const NodeFunctionCall* func_call) const {
                    for (auto it = func_call->args.rbegin(); it != func_call->args.rend(); ++it) {
                        gen.generate_expression(*it);
                    }
                    // Call the function
                    gen.m_output << "  call " << func_call->ident.value.value() << "\n";

                    // Adjust stack pointer after the call if arguments were pushed
                    if (!func_call->args.empty()) {
                        gen.m_output << "  add rsp, " << func_call->args.size() * 8 << "\n";
                    }

                    // Result of the function call is assumed to be in RAX, push it onto the stack
                    gen.push("rax");
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
            void operator()(const NodeBinExpressionEquals* equals) const {
                gen.generate_expression(equals->lhs);
                gen.generate_expression(equals->rhs);

                gen.pop("rbx");  
                gen.pop("rax");  

                gen.m_output << "  cmp rax, rbx\n"; 
                gen.m_output << "  mov rax, 0\n";    
                gen.m_output << "  sete al\n";       
                gen.push("rax");
            }
            void operator()(const NodeBinExpressionNotEquals* not_equals) const {
                gen.generate_expression(not_equals->lhs);
                gen.generate_expression(not_equals->rhs);

                gen.pop("rbx");  
                gen.pop("rax");  

                gen.m_output << "  cmp rax, rbx\n"; 
                gen.m_output << "  mov rax, 0\n";    
                gen.m_output << "  setne al\n";       
                gen.push("rax");
            }
            void operator()(const NodeBinExpressionLess* less) const {
                gen.generate_expression(less->lhs);
                gen.generate_expression(less->rhs);

                gen.pop("rbx");  
                gen.pop("rax");  

                gen.m_output << "  cmp rax, rbx\n"; 
                gen.m_output << "  mov rax, 0\n";    
                gen.m_output << "  setl al\n";       
                gen.push("rax");
            }
            void operator()(const NodeBinExpressionGreater* greater_than) const {
                gen.generate_expression(greater_than->lhs);
                gen.generate_expression(greater_than->rhs);

                gen.pop("rbx");  
                gen.pop("rax");  

                gen.m_output << "  cmp rax, rbx\n"; 
                gen.m_output << "  mov rax, 0\n";    
                gen.m_output << "  setg al\n";       
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
        for(const NodeStatement* statement: scope->statements){
            generate_statement(statement);
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


    void generate_statement(const NodeStatement* stmt, bool functionPass = false)
    {
        struct StmtVisitor {
            Generator& gen;
            bool& functionPass;
            
            void operator()(const NodeFunctionDecl* func_decl) const {
                if(functionPass){
                    if (!func_decl->ident.value.has_value()) {
                        std::cerr << "Function identifier is missing." << std::endl;
                        exit(EXIT_FAILURE);
                    }

                    std::cout << "Generating function " << func_decl->ident.value.value() << std::endl;
                    gen.m_output << func_decl->ident.value.value() << ":\n";
                    gen.m_output << "  push rbp\n";
                    gen.m_output << "  mov rbp, rsp\n";

                    // Reserve space for local variables if needed
                    gen.m_output << "  sub rsp, " << (func_decl->params.size() * 8) << "\n";

                    // Store the function parameters as variables
                    size_t param_offset = 16; // Start offset for the first parameter (previous rbp + return address)
                    for (const auto& param : func_decl->params) {
                        if (!param.value.has_value()) {
                            std::cerr << "Function parameter identifier is missing." << std::endl;
                            exit(EXIT_FAILURE);
                        }
                        gen.m_vars.push_back({.name = param.value.value(), .stack_loc = param_offset});
                        //gen.m_vars.push_back({.name = param.value.value(), .stack_loc = func_decl->params.size() - offset - 1});
                        param_offset += 8;
                    }

                    gen.generate_scope(func_decl->body);
                        
                    // Cleanup and return
                    gen.m_output << "  mov rsp, rbp\n";
                    gen.m_output << "  pop rbp\n";
                    gen.m_output << "  ret\n";

                    // Clear function parameters from the variables list after function is generated.
                    gen.m_vars.clear();

                }
            }

            void operator()(const NodeStatementExit* stmt_exit) const
            {
                if(!functionPass){
                gen.generate_expression(stmt_exit->expr);
                gen.m_output << "  mov rax, 60\n";
                gen.pop("rdi");
                gen.m_output << "  syscall\n";
                }
            }
            void operator()(const NodeStatementInt* stmt_int) const
            {
                if(!functionPass){
                auto it = std::find_if(gen.m_vars.cbegin(), gen.m_vars.cend(), [&](const Var& var){
                    return var.name == stmt_int->ident.value.value();
                });

                if (it != gen.m_vars.cend()) {
                    std::cerr << "Identifier already used: " << stmt_int->ident.value.value() << std::endl;
                    exit(EXIT_FAILURE);
                }
                gen.m_vars.push_back({.name = stmt_int->ident.value.value(), .stack_loc = gen.m_stack_size });
                gen.generate_expression(stmt_int->expr);
                }
            }
            void operator()(const NodeStatementIf* statement_if) const {
                if(!functionPass){
                gen.generate_expression(statement_if->expr); 
                gen.pop("rax");  
                gen.m_output << "  test rax, rax\n";
                std::string label = gen.create_label();
                gen.m_output << "  jz " << label << "\n";
                gen.generate_scope(statement_if->scope);

                if (statement_if->pred.has_value()) {
                    const std::string end_label = gen.create_label();
                    gen.m_output << "  jmp " << end_label << "\n"; 
                    gen.m_output << label << ":\n";  
                    gen.generate_if_predicate(statement_if->pred.value(), end_label);
                    gen.m_output << end_label << ":\n";
                } else {
                    gen.m_output << label << ":\n";
                }
                gen.m_output << "  ;; /if\n";
                }
            }
            void operator()(const NodeScope* scope) const
            {
                if(!functionPass){
                gen.generate_scope(scope);
                }
            }
            void operator()(const NodeStatementAssign* stmt_assign) const
            {
                if(!functionPass){
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
            }
            void operator()(const NodeStatementFor* stmt_for) const {
                if(!functionPass){
                if (stmt_for->init) {
                    gen.generate_statement(stmt_for->init);
                }
                std::string start_label = gen.create_label();
                std::string end_label = gen.create_label();

                gen.m_output << start_label << ":\n";

                if (stmt_for->condition) {
                    gen.generate_expression(stmt_for->condition);
                    gen.pop("rax");
                    gen.m_output << "  test rax, rax\n";
                    gen.m_output << "  jz " << end_label << "\n";
                }

                if (stmt_for->scope) {
                    gen.generate_scope(stmt_for->scope);
                }

                if (stmt_for->iteration) {
                    gen.generate_statement(stmt_for->iteration);
                }
                gen.m_output << "  jmp " << start_label << "\n";
                gen.m_output << end_label << ":\n";
                }
            }

            
        };

        StmtVisitor visitor { .gen = *this, .functionPass = functionPass};
        std::visit(visitor, stmt->var);
    }

    std::string generate_program() {
    std::cout << "generating program" << std::endl;
    std::stringstream full_output;



    for(const NodeStatement* statement : m_program.statements) {
        generate_statement(statement, true);
    }

    m_output << "section .data\n";
    m_output << m_data.str();
    m_output << "section .text\nglobal _start\n";
    m_output << "_start:\n";

    // Generate the main program code
    for(const NodeStatement* statement : m_program.statements) {
        generate_statement(statement, false);
    }

    m_output << "  mov rax, 60\n";
    m_output << "  mov rdi, 0\n";
    m_output << "  syscall\n";

    // Append function definitions at the end
    //full_output << m_function_defs.str();

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
            m_output << ";;endscope" << "\n";
            m_stack_size -= pop_count;
            for(int i = 0; i < pop_count; i++){
                m_vars.pop_back();
            }
            m_scopes.pop_back();
        }

        std::string create_label(){
            std::stringstream ss;
            ss << "label" << m_label_count++;
            std::cout << ss.str() << "\n";
            return ss.str();
        }



        struct Var {
            std::string name;
            size_t stack_loc;
        };

        std::stringstream m_function_defs;
        std::stringstream m_data; // For storing data section
        std::map<std::string, std::string> m_string_literals; // Map from string literal to its label
        const NodeProg m_program;
        std::stringstream m_output;
        size_t m_stack_size = 0;
        std::vector<Var> m_vars {};
        std::vector<size_t> m_scopes {};
        int m_label_count = 0;
};