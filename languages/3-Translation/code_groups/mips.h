#define translation_test \
	add_line<1>({ \
	"Just a test"})
#define translation_print \
	add_line<4>({ \
	"lui $a0, ", \
		"\4""1", \
	"addi $v0, $0, 4", \
	"syscall"})
