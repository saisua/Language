#ifndef LANG_COMPILE_H
#define LANG_COMPILE_H

#define LANG_SYNTAX_PATH "languages//1-Syntax//"
#define LANG_DEFINITION_PATH "languages//2-Definition//"
#define LANG_TRANSLATION_PATH "languages//3-Translation//"
#define LANG_LANGUAGE_PATH "languages//4-Language//"

#define current_compilation "MAIN"


// This should not be redefined.
// However, just in case there is one case
// that needs another compilation step, I'll
// semi-hardcode it here.
#ifndef LANG_GEN_RENAME_FROM
#define LANG_GEN_RENAME_FROM \
    LANG_SYNTAX_PATH "%s.json", \
    LANG_DEFINITION_PATH "%s.json"
#endif
#ifndef LANG_GEN_RENAME_TO
#define LANG_GEN_RENAME_TO \
    LANG_TRANSLATION_PATH "%s.json", \
    LANG_LANGUAGE_PATH "%s.json"
#endif
#ifndef LANG_GEN_RENAME_OTHER
#define LANG_GEN_RENAME_OTHER {}
#endif

#endif