# input - rdi: ptr to string, rsi: str len
# write() - rdi: fd, rsi: ptr to string, rdx: str len
mov rdx, rsi
mov rsi, rdi
mov edi, 0x1
syscall
ret
