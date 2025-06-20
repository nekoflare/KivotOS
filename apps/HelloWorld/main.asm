
main:     file format elf64-x86-64


Disassembly of section .text:

0000000000401000 <_start>:
  401000:	f3 0f 1e fa          	endbr64
  401004:	55                   	push   %rbp
  401005:	48 89 e5             	mov    %rsp,%rbp
  401008:	48 83 ec 10          	sub    $0x10,%rsp
  40100c:	b8 00 00 00 00       	mov    $0x0,%eax
  401011:	ba 00 00 00 00       	mov    $0x0,%edx
  401016:	48 89 d7             	mov    %rdx,%rdi
  401019:	0f 05                	syscall
  40101b:	48 89 45 f8          	mov    %rax,-0x8(%rbp)
  40101f:	90                   	nop
  401020:	c9                   	leave
  401021:	c3                   	ret
