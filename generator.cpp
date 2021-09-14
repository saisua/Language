// Load the corresponding tree
// Load the file passed in the args
// Match the file (as const char *)
// For every match, pass its match_id
//   to a switch containing all definition
//   instructions
// call def_trans_optimize()
// Pass the match to the translator
// (which probably will be another switch)
// Add all lines "unfolded" and pass them
//   to the language
// call trans_lang_optimize()
// Apply all lines and "sew" them correctly
// call language_optimize()