

.align 5
 .global sha1_begin
 .global sha1_end
 sha1_begin:
 .incbin "/home/lifeng/Development/cir-record/application/record4cir/.git/refs/heads/master"
	.word 0
 sha1_end:

