#ifndef LANG_DEFINITION_GENERATED_H
#define LANG_DEFINITION_GENERATED_H

#define definition_standalones_varDefinition_CompilerTags 0
#define definition_standalones_varDefinition_data 3
#define definition_standalones_varDefinition_varType 1
#define definition_standalones_varDefinition_varname 2
#define definition_standalones_if_boolean 0
#define definition_standalones_if_struct 1
#define definition_standalones_print_data 0
#define definition_standalones_while_boolean 0
#define definition_standalones_while_struct 1

#define definition_generated \
case standalones_if: \
	if(data[definition_standalones_if_boolean + pos] == primitive_bool){ \
		"bnez {arg1}, {struct}"\
	}\
	if(data[definition_standalones_if_operation + data[definition_standalones_if_boolean + pos]] == bool_operation_greaterOrEqualThan){ \
		"bge {arg1}, {arg2}, {struct}"\
	}\
	if(data[definition_standalones_if_operation + data[definition_standalones_if_boolean + pos]] == bool_operation_greaterThan){ \
		"bgt {arg1}, {arg2}, {struct}"\
	}\
	if(data[definition_standalones_if_operation + data[definition_standalones_if_boolean + pos]] == bool_operation_lessOrEqualThan){ \
		"ble {arg1}, {arg2}, {struct}"\
	}\
	if(data[definition_standalones_if_operation + data[definition_standalones_if_boolean + pos]] == bool_operation_lessThan){ \
		"blt {arg1}, {arg2}, {struct}"\
	}\
	if(data[definition_standalones_if_operation + data[definition_standalones_if_boolean + pos]] == bool_operation_strict_and){ \
		"[bool_strict_and|@reg[0]@|{arg1}, {arg2}]"\
		"bnez @reg[0]@, {struct}"\
	}\
	if(data[definition_standalones_if_operation + data[definition_standalones_if_boolean + pos]] == bool_operation_strict_not){ \
		"beq $zero, {arg1}, {struct}"\
	}\
	if(data[definition_standalones_if_operation + data[definition_standalones_if_boolean + pos]] == bool_operation_strict_or){ \
		"[bool_strict_or|@reg[0]@|{arg1}, {arg2}]"\
		"bnez @reg[0]@, {struct}"\
	}\
	if(data[definition_standalones_if_operation + data[definition_standalones_if_boolean + pos]] == bool_operation_strict_xor){ \
		"[bool_strict_xor|@reg[0]@|{arg1}, {arg2}]"\
		"bnez @reg[0]@, {struct}"\
	}\
	break;\
case standalones_print: \
	"lui $a0, {arg1}"\
	"addi $v0, $0, 4"\
	"syscall"\
	break;\


#endif

