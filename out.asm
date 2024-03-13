section .data
section .text
global test
global _start
_start:
;;NodeTermIntLit
  mov rax, 7
  push rax
;;/NodeTermIntLit
;;NodeFunctionCall
;;Exit
  push QWORD [rsp + 8]
  mov rax, 60
  pop rdi
  syscall
;;/Exit
  call test
  mov rdi, 21
  mov rax, 60
  syscall
;;functions
;; Function: test
global test
test:
  push rbp
  mov rbp, rsp
  sub rsp, 8 ; make space for parameters
  mov rdi, 5
  mov [rbp - 8], rdi
;;begin_scope
;;Return
  push QWORD [rsp + 16]
  pop rax
  jmp label0_epilogue
;;/Return
  add rsp,0
;;endscope
label0_epilogue:
  add rsp, 8
  mov rsp, rbp
  pop rbp
  ret
;; /Function: test
