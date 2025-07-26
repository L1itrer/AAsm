# input - rdi: ptr to string, rsi: str len
# write() - rdi: fd, rsi: ptr to string, rdx: str len
mov rax, 0x1
mov rdx, rsi
mov rsi, rdi
mov edi, 0x1
syscall
ret
