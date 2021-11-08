#define translation_structStart \
	add_line({ \
	"(new_struct_name)"})
#define translation_if_bool_strict__ \
	add_line({ \
	"bnez ", \
		"\4""1", \
	", {struct}"})
#define translation_if_bool_greater_or_equal_than \
	add_line({ \
	"bge ", \
		"\4""1", \
	", ", \
		"\4""2", \
	", {struct}"})
#define translation_if_bool_greater_than \
	add_line({ \
	"bgt ", \
		"\4""1", \
	", ", \
		"\4""2", \
	", {struct}"})
#define translation_if_bool_less_or_equal_than \
	add_line({ \
	"ble ", \
		"\4""1", \
	", ", \
		"\4""2", \
	", {struct}"})
#define translation_if_bool_less_than \
	add_line({ \
	"blt ", \
		"\4""1", \
	", ", \
		"\4""2", \
	", {struct}"})
#define translation_if_bool_strict_and \
	add_line({ \
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
	add_line({ \
	"beq $zero, ", \
		"\4""1", \
	", {struct}"})
#define translation_if_bool_strict_or \
	add_line({ \
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
	add_line({ \
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
	add_line({ \
	"lui $a0, ", \
		"\4""1", \
	"addi $v0, $0, 4", \
	"syscall"})
