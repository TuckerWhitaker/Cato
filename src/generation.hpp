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
                    gen.m_output << ";;NodeTermIntLit" << "\n";
                    gen.m_output << "  mov rax, " << term_int_lit->int_lit.value.value() << "\n";
                    gen.push("rax");
                    gen.m_output << ";;/NodeTermIntLit" << "\n";
                }
                void operator()(const NodeTermIdent* term_ident) const {

                    std::cout << "m_vars: " << gen.m_vars.size() << std::endl;
                    
                    auto it = std::find_if(gen.m_vars.cbegin(), gen.m_vars.cend(), [&](const Var& var) {
                        return var.name == term_ident->ident.value.value();
                    });

                    if(it == gen.m_vars.cend()){
                        std::cerr << "Undeclared identifier 1: " << term_ident->ident.value.value() << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    
                    std::stringstream offset;
                    std::cout << "stack size:" << gen.m_stack_size << std::endl;
                    std::cout << "stack loc:" <<(*it).stack_loc << std::endl;
                    std::cout << "QWORD [rsp + : " << (gen.m_stack_size - (*it).stack_loc) * 8 << std::endl;
                    offset << "QWORD [rsp + " << (gen.m_stack_size - (*it).stack_loc) * 8 << "]";
                    
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
                        gen.m_data << label << ": db '" << term_string_lit->value << "', 0\n"; 
                    }

                    // Load the address of the string into rax
                    gen.m_output << "  lea rax, [" << gen.m_string_literals[term_string_lit->value] << "]\n";
                    gen.push("rax");
                }
                void operator()(const NodeFunctionCall* func_call) const {
                    gen.m_output << ";;NodeFunctionCall" << "\n";
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
                gen.m_output << ";;sub\n";
                gen.generate_expression(sub->rhs);
                gen.generate_expression(sub->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "  sub rax, rbx\n";
                gen.push("rax");
                gen.m_output << ";;/sub\n";

            }
            void operator()(const NodeBinExpressionAdd* add) const 
            {
                gen.m_output << ";;add\n";
                gen.generate_expression(add->rhs);
                gen.generate_expression(add->lhs);
                
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "  add rax, rbx\n";
                gen.push("rax");
                gen.m_output << ";;/add\n";
            }
            void operator()(const NodeBinExpressionMulti* multi) const 
            {
                gen.m_output << ";;multi\n";
                gen.generate_expression(multi->rhs);
                gen.generate_expression(multi->lhs);
                
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "  mul rbx\n";
                gen.push("rax");
                gen.m_output << ";;/multi\n";

            }
            void operator()(const NodeBinExpressionDiv* div) const 
            {
                gen.m_output << ";;div\n";
                gen.generate_expression(div->rhs);
                gen.generate_expression(div->lhs);
               
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "  div rbx\n";
                gen.push("rax");
                gen.m_output << ";;/div\n";

            }
            void operator()(const NodeBinExpressionEquals* equals) const {
                gen.m_output << ";;equals\n";
                gen.generate_expression(equals->lhs);
                gen.generate_expression(equals->rhs);

                gen.pop("rbx");  
                gen.pop("rax");  

                gen.m_output << "  cmp rax, rbx\n"; 
                gen.m_output << "  mov rax, 0\n";    
                gen.m_output << "  sete al\n";       
                gen.push("rax");
                gen.m_output << ";;/equals\n";
            }
            void operator()(const NodeBinExpressionNotEquals* not_equals) const {
                gen.m_output << ";;not_equals\n";
                gen.generate_expression(not_equals->lhs);
                gen.generate_expression(not_equals->rhs);

                gen.pop("rbx");  
                gen.pop("rax");  

                gen.m_output << "  cmp rax, rbx\n"; 
                gen.m_output << "  mov rax, 0\n";    
                gen.m_output << "  setne al\n";       
                gen.push("rax");
                gen.m_output << ";;/not_equals\n";
            }
            void operator()(const NodeBinExpressionLess* less) const {
                gen.m_output << ";;less_than\n";
                gen.generate_expression(less->lhs);
                gen.generate_expression(less->rhs);

                gen.pop("rbx");  
                gen.pop("rax");  

                gen.m_output << "  cmp rax, rbx\n"; 
                gen.m_output << "  mov rax, 0\n";    
                gen.m_output << "  setl al\n";       
                gen.push("rax");
                gen.m_output << ";;/less_than\n";
            }
            void operator()(const NodeBinExpressionGreater* greater_than) const {
                gen.m_output << ";;greater_than\n";
                gen.generate_expression(greater_than->lhs);
                gen.generate_expression(greater_than->rhs);

                gen.pop("rbx");  
                gen.pop("rax");  

                gen.m_output << "  cmp rax, rbx\n"; 
                gen.m_output << "  mov rax, 0\n";    
                gen.m_output << "  setg al\n";       
                gen.push("rax");
                gen.m_output << ";;/greater_than\n";
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


            void operator()(const NodeStatementReturn* statement_return) const {
                if (!functionPass) {
                    gen.m_output << ";;Return\n";

                    // If the return statement has an expression, evaluate it
                    if (statement_return->expr) {
                        gen.generate_expression(statement_return->expr);
                        // Assume the result is in rax after expression evaluation
                        gen.pop("rax");
                    }

                    // Jump to the function's epilogue
                    // This assumes you have a label at the end of the function for the epilogue
                    // The label should be unique per function; you can generate it when you start processing the function
                    gen.m_output << "  jmp " << gen.currentFunctionEpilogueLabel() << "\n";
                    gen.m_output << ";;/Return\n";
                }
            }

            
            void operator()(const NodeFunctionDecl* func_decl) const 
            {
                if (functionPass) {
                    if (!func_decl->ident.value.has_value()) {
                        std::cerr << "Function identifier is missing." << std::endl;
                        exit(EXIT_FAILURE);
                    }

                    // Generate and store the unique epilogue label for the current function
                    gen.m_currentFunctionEpilogueLabel = gen.create_label() + "_epilogue";

                    std::string funcName = func_decl->ident.value.value();
                    gen.m_output << ";; Function: " << funcName << "\n";
                    gen.m_output << "global " << funcName << "\n";
                    gen.m_output << funcName << ":\n";

                    // Function prologue...
                    gen.m_output << "  push rbp" << "\n";
                    gen.m_output << "  mov rbp, rsp" << "\n";

                    gen.generate_scope(func_decl->body);
                    // Function epilogue...
                    gen.m_output << "  mov rsp, rbp" << "\n";
                    gen.m_output << "  pop rbp" << "\n";
                    gen.m_output << "  ret" << "\n";


                    // Mark the epilogue label position
                    gen.m_output << gen.m_currentFunctionEpilogueLabel << ":\n";
                    gen.m_output << "  mov rsp, rbp\n";
                    gen.m_output << "  pop rbp\n";
                    gen.m_output << "  ret\n";
                    gen.m_output << ";; /Function: " << funcName << "\n";
                }
            }

            void operator()(const NodeStatementExit* stmt_exit) const
            {
                if(!functionPass){
                gen.m_output << ";;Exit\n";
                gen.generate_expression(stmt_exit->expr);
                gen.m_output << "  mov rax, 60\n";
                gen.pop("rdi");
                gen.m_output << "  syscall\n";
                gen.m_output << ";;/Exit\n";
                }
            }
            void operator()(const NodeStatementInt* stmt_int) const {
                if (!functionPass) {
                    auto it = std::find_if(gen.m_vars.cbegin(), gen.m_vars.cend(), [&](const Var& var) {
                        return var.name == stmt_int->ident.value.value();
                    });

                    if (it != gen.m_vars.cend()) {
                        std::cerr << "Identifier already used: " << stmt_int->ident.value.value() << std::endl;
                        exit(EXIT_FAILURE);
                    }

                    // Add the variable to the stack and increment the stack size.
                    gen.generate_expression(stmt_int->expr);
                    gen.m_vars.push_back({.name = stmt_int->ident.value.value(), .stack_loc = gen.m_stack_size });
                    //gen.m_stack_size++;
                }
            }
            void operator()(const NodeStatementIf* statement_if) const {
                if(!functionPass){
                gen.m_output << ";;If\n";
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
                
                }
                gen.m_output << ";;/If\n";
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
                    std::cerr << "Undeclared identifier 2: " << stmt_assign->ident.value.value() << std::endl;
                }
                gen.generate_expression(stmt_assign->expr);
                gen.pop("rax");

                std::cout << "NodeStatementAssign:" << std::endl;
                std::cout << "stack size:" << gen.m_stack_size << std::endl;
                std::cout << "stack loc:" <<(*it).stack_loc << std::endl;
                std::cout << "QWORD [rsp + : " << (gen.m_stack_size - (*it).stack_loc) * 8 << std::endl;
                gen.m_output << "  mov [rsp + " << (gen.m_stack_size - (*it).stack_loc) * 8 << "], rax\n";
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

    

    m_output << "section .data\n";
    m_output << m_data.str();
    m_output << "section .text\n";
    m_output << "global _start\n";
    m_output << "_start:\n";

    

    for(const NodeStatement* statement : m_program.statements) {
        generate_statement(statement, false);
    }


    m_output << "  mov rdi, rax\n";
    m_output << "  mov rax, 60\n";
    m_output << "  syscall\n";
    
    m_output << ";;functions\n";

    for(const NodeStatement* statement : m_program.statements) {
        generate_statement(statement, true);
    }

    //m_output << "  mov rax, 60\n";
    //m_output << "  mov rdi, 12\n";
    //m_output << "  syscall\n";

    // Append function definitions at the end
    //full_output << m_function_defs.str();

    return m_output.str();
    }

    std::string currentFunctionEpilogueLabel() const {
        return m_currentFunctionEpilogueLabel;
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
            m_output << ";;begin_scope" << "\n";
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

        std::string m_currentFunctionEpilogueLabel;
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