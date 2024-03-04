global _start
_start:
  mov rax, 1
  push rax
  mov rax, 3
  push rax
  mov rax, 0
  push rax
label0:
  push QWORD [rsp + 0]
  mov rax, 10
  push rax
  pop rbx
  pop rax
  cmp rax, rbx
  mov rax, 0
  setl al
  push rax
  pop rax
  test rax, rax
  jz label1
  mov rax, 3
  push rax
  push QWORD [rsp + 24]
  pop rax
  pop rbx
  add rax, rbx
  push rax
  pop rax
  mov [rsp + 16], rax
  add rsp,0
  mov rax, 1
  push rax
  push QWORD [rsp + 8]
  pop rax
  pop rbx
  add rax, rbx
  push rax
  pop rax
  mov [rsp + 0], rax
  jmp label0
label1:
  push QWORD [rsp + 16]
  mov rax, 60
  pop rdi
  syscall
  mov rax, 60
  mov rdi, 0
  syscall
