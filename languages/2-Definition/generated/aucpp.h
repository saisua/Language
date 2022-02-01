#ifndef LANG_DEFINITION_GENERATED_H
#define LANG_DEFINITION_GENERATED_H

#define next_group \
if(todo_jump.empty()) \
	goto end_definition_label; \
else { \
	void * next = todo_jump.top(); \
	todo_jump.pop(); \
	\
	mreg = todo_mreg.top(); \
	todo_mreg.pop(); \
	\
	goto *next; \
}

#define error printf("Error");

#define store_state(next_jump) todo_jump.push( && next_jump ); todo_mreg.push( mreg );

#define definition_standalones_print_data 1
#define definition_standalones_test_data 1
#define definition_standalones_test_operation 2


#define definition_generated(data_struct) \
switch(data_struct[0]){ \
	case _code_standalones_print: \
		switch(data_struct[data_struct[definition_standalones_print_data]]){ \
			case _code_primitive_char: \
				goto _label_standalones_print_final; \
			case _code_primitive_double: \
				goto _label_standalones_print_final; \
			case _code_primitive_float: \
				goto _label_standalones_print_final; \
			case _code_primitive_int: \
				goto _label_standalones_print_final; \
			case _code_primitive_str: \
				goto _label_standalones_print_final; \
			case _code_primitive_bool: \
				goto _label_standalones_print_final; \
			[[unlikely]] \
			default: error; \
		} \
			[[unlikely]] \
			default: error; \
		} \
		switch(data_struct[data_struct[definition_standalones_print_data]]){ \
			[[unlikely]] \
			default: error; \
		} \
			[[unlikely]] \
			default: error; \
		} \
		break; \
	\
	case _code_standalones_test: \
		switch(data_struct[data_struct[definition_standalones_test_operation]]){ \
			case _code_arithmetic_addition: \
				goto _label_standalones_test_final; \
			case _code_arithmetic_division: \
				goto _label_standalones_test_final; \
			case _code_arithmetic_exponentiation: \
				goto _label_standalones_test_final; \
			case _code_arithmetic_modulo: \
				goto _label_standalones_test_final; \
			case _code_arithmetic_multiplication: \
				goto _label_standalones_test_final; \
			case _code_arithmetic_subtraction: \
				goto _label_standalones_test_final; \
			[[unlikely]] \
			default: error; \
		} \
		switch(data_struct[data_struct[definition_standalones_test_data]]){ \
			[[unlikely]] \
			default: error; \
		} \
			case _code_test_test1: \
				store_state( _label_standalones_test_final ); \
				goto _label_standalones_test_test_test1; \
			case _code_test_test2: \
				store_state( _label_standalones_test_final ); \
				goto _label_standalones_test_test_test2; \
			[[unlikely]] \
			default: error; \
		} \
		break; \
	\
	\
	[[unlikely]] \
	default: error; \
} \
\
_label_standalones_test_test_test1: \
_label_standalones_test_test_test2: \
\
\
_label_standalones_print_final: \
	translation_print; \
	next_group; \
_label_standalones_test_final: \
	translation_test; \
	next_group; \
\
end_definition_label:


#endif

