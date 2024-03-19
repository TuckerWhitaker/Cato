section .data
section .text
global _start
_start:
;;NodeFunctionCall
;;NodeTermIntLit
  mov rax, 6
  push rax
;;/NodeTermIntLit
  pop rdi
;;NodeTermIntLit
  mov rax, 3
  push rax
;;/NodeTermIntLit
  pop rsi
  call test
  push rax
;;/NodeFunctionCall
  ;; Declaring int variable: c
  mov rdi, rax
  mov rax, 60
  syscall
;;functions
;; Function: test
global test
test:
  push rbp
  mov rbp, rsp
;;begin_scope
;;Return
;;add
  ;; Using variable: d
  mov rax, rdi
  push rax
;;add
  ;; Using variable: e
  mov rax, rsi
  push rax
;;add
  ;; Using variable: e
  mov rax, rsi
  push rax
  ;; Using variable: d
  mov rax, rdi
  push rax
  pop rax
  pop rbx
  add rax, rbx
  push rax
;;/add
  pop rax
  pop rbx
  add rax, rbx
  push rax
;;/add
  pop rax
  pop rbx
  add rax, rbx
  push rax
;;/add
  pop rax
  jmp label0_epilogue
;;/Return
  add rsp,0
;;endscope
  mov rsp, rbp
  pop rbp
  ret
label0_epilogue:
  mov rsp, rbp
  pop rbp
  ret
;; /Function: test
