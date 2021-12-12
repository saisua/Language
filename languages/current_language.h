#ifndef LANG_COMPILE_H
#define LANG_COMPILE_H

#define LANG_LANGUAGES_FOLDER "languages//"

#define LANG_SYNTAX_PATH LANG_LANGUAGES_FOLDER "1-Syntax//"
#define LANG_DEFINITION_PATH LANG_LANGUAGES_FOLDER "2-Definition//"
#define LANG_TRANSLATION_PATH LANG_LANGUAGES_FOLDER "3-Translation//"
#define LANG_LANGUAGE_PATH LANG_LANGUAGES_FOLDER "4-Language//"
#define LANG_UTILS_FOLDER "utils//"
#define LANG_OPTIMIZATIONS_FOLDER "Optimizations//"

#define LANG_SYNTAX_TREES_FOLDER "trees//"
#define LANG_DEFINITION_CODES_FOLDER "codes//"
#define LANG_DEFINITION_GENERATED_FOLDER "generated//"
#define LANG_TRANSLATION_CODE_GROUPS_FOLDER "code_groups//"

#define LANG_SYNTAX_STATE_CODES_FOLDER "state_codes//"

// This should not be redefined.
// However, just in case there is one case
// that needs another compilation step, I'll
// semi-hardcode it here.
#ifndef LANG_GEN_RENAME_FROM
#define LANG_GEN_RENAME_FROM \
    {LANG_SYNTAX_PATH "%s.json"}, \
    {LANG_DEFINITION_PATH "%s.json"}
#endif
#ifndef LANG_GEN_RENAME_TO
#define LANG_GEN_RENAME_TO \
    {LANG_TRANSLATION_PATH "%s.json"}, \
    {LANG_LANGUAGE_PATH "%s.json"}
#endif
#ifndef LANG_GEN_RENAME_OTHER
#define LANG_GEN_RENAME_OTHER {}
#endif

#define max_flag(size) (pow(2,(size-1))-1)

#endif

#ifndef GENERATED_TAG_LIST_H
#define GENERATED_TAG_LIST_H

#define Syntax_match_symbol "%"
#define Syntax_id 1
#define Definition_match_symbol "#"
#define Definition_id 2
#define Translation_match_symbol "::"
#define PATH_PWD '.'
#define PATH_ROOT '/'
#define PATH_SET '!'
#define Translation_id 3
#define Language_match_symbol "~"
#define Language_id 4


#define LANG_FROM "aucpp"
#define LANG_TO "mips"

#endif

#include  "languages//1-Syntax//trees//aucpp.tree.h"

#include "languages//2-Definition//codes//aucpp.h"
#include "languages//2-Definition//generated//aucpp.h"
#include "languages//3-Translation//code_groups//mips.h"

#include "utils//var_handler//var_handler.h"
#include "utils//line_generation//line_gen_concat.h"

#define VAR_GEN \n