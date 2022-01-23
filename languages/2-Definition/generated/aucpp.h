#ifndef LANG_DEFINITION_GENERATED_H
#define LANG_DEFINITION_GENERATED_H

#define definition_standalones_while_boolean 1
#define definition_standalones_while_struct 2
#define definition_standalones_varDefinition_CompilerTags 1
#define definition_standalones_varDefinition_data 4
#define definition_standalones_varDefinition_varType 2
#define definition_standalones_varDefinition_varname 3
#define definition_standalones_if_boolean 1
#define definition_standalones_if_struct 2
#define definition_standalones_print_data 1
#define definition_t_bool_data_1 1
#define definition_t_bool_data_2 3
#define definition_t_bool_operation 2
#define definition_t_bool_data_1 1
#define definition_t_bool_data_2 3
#define definition_t_bool_operation 2

#define definition_generated(data_struct) \
switch(data_struct[0]) { \
case _code_standalones_structStart: \
	translation_structStart; \
	break; \
case _code_standalones_if: \
	if(data_struct[data_struct[definition_standalones_if_boolean + pos]] & _code_primitive_bool_mask == _code_primitive_bool){ \
		translation_if_bool_strict__; \
	} \
	else if(data_struct[data_struct[definition_standalones_if_boolean + pos]] & _code_t_bool_mask == _code_t_bool){ \
		pos = data_struct[definition_standalones_if_boolean + pos]; \
		if(data_struct[data_struct[definition_t_bool_operation + pos]] == _code_t_bool_operation_greater_or_equal_than){ \
			translation_if_bool_greater_or_equal_than; \
		} \
		else if(data_struct[data_struct[definition_t_bool_operation + pos]] == _code_t_bool_operation_greater_than){ \
			translation_if_bool_greater_than; \
		} \
		else if(data_struct[data_struct[definition_t_bool_operation + pos]] == _code_t_bool_operation_less_or_equal_than){ \
			translation_if_bool_less_or_equal_than; \
		} \
		else if(data_struct[data_struct[definition_t_bool_operation + pos]] == _code_t_bool_operation_less_than){ \
			translation_if_bool_less_than; \
		} \
		else if(data_struct[data_struct[definition_t_bool_operation + pos]] == _code_t_bool_operation_strict_and){ \
			translation_if_bool_strict_and; \
		} \
		else if(data_struct[data_struct[definition_t_bool_operation + pos]] == _code_t_bool_operation_strict_not){ \
			translation_if_bool_strict_not; \
		} \
		else if(data_struct[data_struct[definition_t_bool_operation + pos]] == _code_t_bool_operation_strict_or){ \
			translation_if_bool_strict_or; \
		} \
		else if(data_struct[data_struct[definition_t_bool_operation + pos]] == _code_t_bool_operation_strict_xor){ \
			translation_if_bool_strict_xor; \
		} \
	} \
	break; \
case _code_standalones_print: \
	translation_print; \
	break; \
}

#endif

