#define translation_structStart \
	add_line<1>({ \
	"(new_struct_name)"})
#define translation_if_bool_strict__ \
	add_line<3>({ \
	"bnez ", \
		"\4""1", \
	", {struct}"})
#define translation_if_bool_greater_or_equal_than \
	add_line<5>({ \
	"bge ", \
		"\4""1", \
	", ", \
		"\4""2", \
	", {struct}"})
#define translation_if_bool_greater_than \
	add_line<5>({ \
	"bgt ", \
		"\4""1", \
	", ", \
		"\4""2", \
	", {struct}"})
#define translation_if_bool_less_or_equal_than \
	add_line<5>({ \
	"ble ", \
		"\4""1", \
	", ", \
		"\4""2", \
	", {struct}"})
#define translation_if_bool_less_than \
	add_line<5>({ \
	"blt ", \
		"\4""1", \
	", ", \
		"\4""2", \
	", {struct}"})
#define translation_if_bool_strict_and \
	add_line<10>({ \
	"[bool_strict_and|", \
		"\3""0", \
	"|", \
		"\4""1", \
	", ", \
		"\4""2", \
	"]", \
	"bnez ", \
		"\3""0", \
	", {struct}"})
#define translation_if_bool_strict_not \
	add_line<3>({ \
	"beq $zero, ", \
		"\4""1", \
	", {struct}"})
#define translation_if_bool_strict_or \
	add_line<10>({ \
	"[bool_strict_or|", \
		"\3""0", \
	"|", \
		"\4""1", \
	", ", \
		"\4""2", \
	"]", \
	"bnez ", \
		"\3""0", \
	", {struct}"})
#define translation_if_bool_strict_xor \
	add_line<10>({ \
	"[bool_strict_xor|", \
		"\3""0", \
	"|", \
		"\4""1", \
	", ", \
		"\4""2", \
	"]", \
	"bnez ", \
		"\3""0", \
	", {struct}"})
#define translation_print \
	add_line<4>({ \
	"lui $a0, ", \
		"\4""1", \
	"addi $v0, $0, 4", \
	"syscall"})
