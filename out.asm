global _start
_start:
  mov rax, 1
  push rax
  mov rax, 0
  push rax
  mov rax, 0
  push rax
  pop rax
  test rax, rax
  jz label0
  mov rax, 1
  push rax
  mov rax, 60
  pop rdi
  syscall
  add rsp,0
label0:
  mov rax, 0
  push rax
  pop rax
  test rax, rax
  jz label2
  mov rax, 2
  push rax
  mov rax, 60
  pop rdi
  syscall
  add rsp,0
  jmp label1
label2:
  mov rax, 3
  push rax
  mov rax, 60
  pop rdi
  syscall
  add rsp,0
label1:
  mov rax, 5
  push rax
  mov rax, 60
  pop rdi
  syscall
  mov rax, 60
  mov rdi, 0
  syscall
