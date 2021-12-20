#define COMMA ,

#define translation_print \
	add_line<4>({ \
	"lui $a0, " COMMA \
		"\4""1" COMMA \
	"addi $v0, $0, 4" COMMA \
	"syscall"})
#define translation_structStart \
	add_line<1>({ \
	"(new_struct_name)"})
#define translation_if_bool_strict__ \
	add_line<3>({ \
	"bnez " COMMA \
		"\4""1" COMMA \
	", {struct}"})
#define translation_if_bool_greater_or_equal_than \
	add_line<5>({ \
	"bge " COMMA \
		"\4""1" COMMA \
	", " COMMA \
		"\4""2" COMMA \
	", {struct}"})
#define translation_if_bool_greater_than \
	add_line<5>({ \
	"bgt " COMMA \
		"\4""1" COMMA \
	", " COMMA \
		"\4""2" COMMA \
	", {struct}"})
#define translation_if_bool_less_or_equal_than \
	add_line<5>({ \
	"ble " COMMA \
		"\4""1" COMMA \
	", " COMMA \
		"\4""2" COMMA \
	", {struct}"})
#define translation_if_bool_less_than \
	add_line<5>({ \
	"blt " COMMA \
		"\4""1" COMMA \
	", " COMMA \
		"\4""2" COMMA \
	", {struct}"})
