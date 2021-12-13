#ifndef LANG_COMPILE_H
#define LANG_COMPILE_H

#define LANG_LANGUAGES_FOLDER "languages//"

#define LANG_SYNTAX_FOLDER "1-Syntax//"
#define LANG_DEFINITION_FOLDER "2-Definition//"
#define LANG_TRANSLATION_FOLDER "3-Translation//"
#define LANG_LANGUAGE_FOLDER "4-Language//"
#define LANG_UTILS_FOLDER "utils//"
#define LANG_OPTIMIZATIONS_FOLDER "Optimizations//"

#define LANG_SYNTAX_TREES_FOLDER "trees//"
#define LANG_DEFINITION_CODES_FOLDER "codes//"
#define LANG_DEFINITION_GENERATED_FOLDER "generated//"
#define LANG_TRANSLATION_CODE_GROUPS_FOLDER "code_groups//"

#define LANG_SYNTAX_STATE_CODES_FOLDER "state_codes//"

#define LANG_SYNTAX_PATH LANG_LANGUAGES_FOLDER LANG_SYNTAX_FOLDER
#define LANG_DEFINITION_PATH LANG_LANGUAGES_FOLDER LANG_DEFINITION_FOLDER
#define LANG_TRANSLATION_PATH LANG_LANGUAGES_FOLDER LANG_TRANSLATION_FOLDER
#define LANG_LANGUAGE_PATH LANG_LANGUAGES_FOLDER LANG_LANGUAGE_FOLDER

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