section .data
section .text
global _start
_start:
;;NodeTermIntLit
  mov rax, 9
  push rax
;;/NodeTermIntLit
;;NodeTermIntLit
  mov rax, 5
  push rax
;;/NodeTermIntLit
;;NodeTermIntLit
  mov rax, 10
  push rax
;;/NodeTermIntLit
;;Exit
;;add
;;multi
;;NodeTermIntLit
  mov rax, 2
  push rax
;;/NodeTermIntLit
  push QWORD [rsp + 16]
  pop rax
  pop rbx
  mul rbx
  push rax
;;/multi
;;add
  push QWORD [rsp + 8]
  push QWORD [rsp + 32]
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
  mov rax, 60
  pop rdi
  syscall
;;/Exit
  mov rdi, rax
  mov rax, 60
  syscall
;;functions
