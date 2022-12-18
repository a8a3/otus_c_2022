    bits 64
    extern malloc, free, puts, printf, fflush, abort
    global main

    section   .data
empty_str: db 0x0
int_format: db "%ld ", 0x0
data: dq 4, 8, 15, 16, 23, 42
data_length: equ ($-data) / 8

    section   .text
;;; print_int proc
print_int:
    push rbp
    mov rbp, rsp ;save original rsp
    and rsp, -16 ;align stack to 16 bytes boundary
    mov rsi, rdi
    mov rdi, int_format
    xor rax, rax
    call printf

    xor rdi, rdi
    call fflush
    mov rsp, rbp ;restore original rsp
    pop rbp
    ret

;;; p proc
p:
    mov rax, rdi
    and rax, 1
    ret

;;; add_element proc
add_element:
    push rbp
    push rbx
    push r14
    mov rbp, rsp ;save original rsp
    and rsp, -16 ;align stack to 16 bytes boundary

    mov r14, rdi
    mov rbx, rsi

    mov rdi, 16
    call malloc
    test rax, rax
    jz abort

    mov [rax], r14
    mov [rax + 8], rbx

    mov rsp, rbp ;restore original rsp
    pop r14
    pop rbx
    pop rbp

    ret

;;; remove_list proc
remove_list:
    test rdi, rdi
    jz out_remove_list

    push rbp
    push r14
    mov rbp, rsp
    and rsp, -16 ;align stack to 16 bytes boundary
   
    remove_loop:
        mov r14, [rdi + 8]
        call free ;rdi already contains node ptr
        mov rdi, r14
        test rdi, rdi
        jnz remove_loop

    mov rsp, rbp ;restore original rsp
    pop r14
    pop rbp
    out_remove_list:
    ret

;;; m proc
m:
    test rdi, rdi
    jz outm

    push rbp
    push rbx
    mov rbp, rsi
    m_loop:
        mov rbx, rdi

        mov rdi, [rdi]
        call rbp

        mov rdi, [rbx + 8]
        test rdi, rdi
        jnz m_loop

    pop rbx
    pop rbp

    outm:
    ret

;;; f proc
f:
    test rdi, rdi
    jz outf

    push rbx
    push r12
    push r13

    mov r13, rdx

    f_loop:
        mov rbx, rdi
        mov r12, rsi

        mov rdi, [rdi]
        call r13 ;check if odd
        test rax, rax
        jz z

        mov rdi, [rbx]
        mov rsi, r12
        call add_element
        mov rsi, rax
        jmp ff

        z:
        mov rsi, r12

        ff:
        mov rdi, [rbx + 8]
        test rdi, rdi
        jnz f_loop
   
    mov rax, rsi

    pop r13
    pop r12
    pop rbx
outf:
ret

;;; main proc
main:
    push rbx

    xor rax, rax
    mov rbx, data_length
adding_loop:
    mov rdi, [data - 8 + rbx * 8]
    mov rsi, rax
    call add_element
    dec rbx
    jnz adding_loop

    mov rbx, rax

    mov rdi, rax
    mov rsi, print_int
    call m

    mov rdi, empty_str
    call puts

    mov rdx, p
    xor rsi, rsi
    mov rdi, rbx
    call f

    mov r14, rax
    mov rdi, rax
    mov rsi, print_int
    call m

    mov rdi, empty_str
    call puts

    mov rdi, rbx
    call remove_list

    mov rdi, r14
    call remove_list

    pop rbx

    xor rax, rax
    ret
