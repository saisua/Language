#define translation_print \
	add_line({ \
	"lui $a0, ", \
		"\4""1", \
	"addi $v0, $0, 4", \
	"syscall"})
