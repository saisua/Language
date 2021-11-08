#ifndef FRB_GEN_CPP
#define FRB_GEN_CPP

#define FRB_GENERATE true

#ifndef FRB_MATCH
#define FRB_MATCH true
#endif

// Local includes
#include "Mreg.cpp"


std::unordered_set<char> invert_chars(const char* chars){
	std::unordered_set<char> negative = std::unordered_set<char>();

	for(char inserted : reg_dot)
		negative.insert(inserted);

	while(*chars){
		if(negative.count(*chars))
			negative.erase(*chars);

		++chars;
	}

	return negative;
};
std::unordered_set<char> invert_chars(const char chars){
	std::unordered_set<char> negative = std::unordered_set<char>();

	for(char inserted : reg_dot)
		negative.insert(inserted);

	negative.erase(chars);

	return negative;
};

template <typename T>
class Mreg_gen : public Mreg<T>{
    private:
        typedef Mreg<T> super;

		bool analizer_generated = false;
		std::vector<std::pair<regex, std::unordered_map<char, uint>>> regex_analizer;
		std::vector<std::string> lookbehind;

		std::unordered_map<T, branch_t<T>> added_final_branches;

    public:
		std::unordered_map<std::string, T> added_ids;
		std::unordered_map<std::string, T> added_nicknames;

    Mreg_gen() : super(){
		this->deleted = std::unordered_set<T>();
		this->unknown_ptrs = std::list<capture_t<long unsigned int>*>();

		this->added_ids = std::unordered_map<std::string, T>();
		this->added_nicknames = std::unordered_map<std::string, T>();
		this->added_final_branches = std::unordered_map<T, branch_t<T>>();
    }

    ~Mreg_gen(){
        if(this->unknown_ptrs.size()){
			printf("unknown_ptrs\n");
			for(capture_t<long unsigned int>* ucap : this->unknown_ptrs)
				delete ucap;
		}
    }

    // This function analizes a regex expression, and divides it
	// in groups of characters, separating capture groups, expressions
	// and basic char strings.
	inline char** _analize(const char* c_raw_expr){
		#if FRB_VERBOSE
		printf("\nAnalizing pattern: \"%s\"\n", c_raw_expr);
		#endif
		if(! this->analizer_generated)
			this->generate_regex_analizer();

		// The list with the processed substrings
		std::list<std::string> storage = std::list<std::string>{c_raw_expr};

		{
		std::smatch match;
		uint last;
		std::string expr_part;

		std::vector<std::string>::const_iterator lb_iter = this->lookbehind .begin();

		// Iterate over the expression, looking for regex
		// patterns. If there is one, add it to expression
		// Get matcher (regex) and flags (map<char, id>)
		// in the entire regex analize and start matching
		// all split strings in storage.
		for(const auto& [matcher, flags] : this->regex_analizer){
			#if FRB_VERBOSE_ANALYSIS
			/*
			if(storage.size()){
				printf("[~] ");

				for(std::string &s : *storage){
					if(s.front() > 31)
						printf("%s, ", s.c_str());
					else
						printf("#, ");
				}

				printf("\n");
			}
			*/
			printf("%s\n  ", matcher.pattern.c_str());
			int found = 0;
			int parts = 0;
			#endif

			// Get an iterator of the list. 
			std::list<std::string>::iterator expr_iter = storage.begin(); 
			
			// While we are not at the end of the list.
			while(expr_iter != storage.end()){				
				if((*expr_iter)[0] < 32){
					#if FRB_VERBOSE_ANALYSIS
					printf("#");
					#endif

					++expr_iter;
					
					continue;
				}

				#if FRB_VERBOSE_ANALYSIS
				if(!found)
					printf("|");
				#endif

				// Keep this so that we can match the
				// regex again against the suffix of the
				// same loop
				expr_part = *expr_iter;

				bool matched = false;
				bool looped = false;
				while(matched = std::regex_search(expr_part, match, matcher)) {
					if(looped){
						storage.emplace(expr_iter, "");
						--expr_iter;
					}
					
					looped = true;

					#if FRB_VERBOSE_ANALYSIS
					if(! found)
						printf(" %s:", expr_part.c_str());
					else
						printf(",");
					#endif

					// New prefix string to be added in storage
					std::string prefix = match.prefix().str();
					// New matched string to be added in storage
					// where the first char will be changed to
					// the appropiate identifier
					std::string match_result = "0";
					if(prefix.empty() || !(*lb_iter).length() || !prefix.ends_with(*lb_iter))
						if(match.size() > 1){
							auto m = match.cbegin() + 1;
							match_result.append(*m);

							#if FRB_VERBOSE_ANALYSIS
							if(! found)
								printf(" [");

							printf(" M:%s ", (*m).str().c_str());
							#endif

							while(++m != match.cend()){
								match_result.append(1, ANALIZE_SEPARATE);

								match_result.append(*m);

								#if FRB_VERBOSE_ANALYSIS
								printf("M:%s ", (*m).str().c_str());
								#endif
							
							}
						} else {
							#if FRB_VERBOSE_ANALYSIS
							printf("[ ");
							#endif

							match_result.append((*(match.cbegin())).str());
						}

					// matched regex, but failed in negative lookbehind
					else {
						std::string suffix = match.suffix().str();

						#if FRB_VERBOSE_ANALYSIS
						// Add to match_result to print for debugging purposes
						auto m2 = match.cbegin();
						std::string full_match = std::string(*m2);

						match_result.assign(*m2);
						while(++m2 != match.cend()){
								match_result.append(1, ANALIZE_SEPARATE);
								match_result.append(*m2);
						}
						#endif

						// If there is something to look for, it is
						// rather messy, I need to add the prefix + matches,
						// and assign it to the previous searched string
						if(suffix.length()){
							for(auto m = match.cbegin() + 1; m != match.cend(); ++m)
								prefix.append(*m);

							storage.emplace(expr_iter, prefix);

							// In case the while does not loop, 
							// the program will think *expr_iter is
							// valid, and so it must be changed now.
							(*expr_iter).assign(suffix);
							expr_part.assign(suffix);

							#if FRB_VERBOSE_ANALYSIS
							printf(" [ M!(%s) S:%s ", match_result.c_str(), suffix.c_str());
							#endif

							// Just in case there is no more matches
							looped = false;

							continue;
						}

						(*expr_iter).assign(expr_part);

						#if FRB_VERBOSE_ANALYSIS
						printf(" [ M!(%s) ", match_result.c_str());
						#endif

						++expr_iter;

						break;
					}

					// New suffix string to be added in storage
					std::string suffix = match.suffix().str();

					// Detect the exact match found
					char search_char = (*(match_result.begin()+1));
					std::unordered_map<char, uint>::const_iterator find;

					if(search_char && (find = flags.find(search_char)) != flags.end()){
						#if FRB_VERBOSE_ANALYSIS
						++found;
						#endif

						// Since the match was exact,
						// we only need its identifier
						match_result.assign("0");
					} else {
						find = flags.find('_');

						if(find != flags.end()){
							#if FRB_VERBOSE_ANALYSIS
							++found;
							#endif
						} else {
							printf("\n[-] Wrong match Idk how\n");
							printf("\t{P:%s c:%c S:%s}\n", prefix.c_str(), search_char, suffix.c_str());
						}
					}
					
					// Change the initial char to the identifier
					match_result[0] = find->second;

					// Replace the matched string with the
					// prefix of the match
					if(prefix.length()){
						#if FRB_VERBOSE_ANALYSIS
						printf("P:%s ", prefix.c_str());
						#endif

						// Assign breaks iterators
						(*expr_iter).assign(prefix);
						++expr_iter;

						storage.emplace(expr_iter, match_result);
					} 
					// or if it does not exist, replace it
					// with the matched result instead.
					else {
						(*expr_iter).assign(match_result);
						++expr_iter;
					}

					if(suffix.length() != 0){
						#if FRB_VERBOSE_ANALYSIS
						printf("S:%s ", suffix.c_str());
						#endif

						expr_part.assign(suffix);
						//storage.emplace(expr_iter, suffix);

						looped = true;
					}
					// No need to continue the loop
					else {
						++expr_iter;

						break;
					}
				}

				if(! matched){
					if(expr_iter != storage.end()){
						// Sometimes after a match this is needed
						(*expr_iter).assign(expr_part);

						++expr_iter;
					} else
						storage.emplace(expr_iter, expr_part);
				}
				
				#if FRB_VERBOSE_ANALYSIS
				printf("N:%d ] ", storage.size());
				#endif
				
			}

			#if FRB_VERBOSE_ANALYSIS
			if(found)
				printf("\n");
			#endif

			++lb_iter;
		}
		}

		char** c_final = new char*[storage.size() + 1];
		char** final_iter = c_final;

		#if FRB_VERBOSE
		#if FRB_VERBOSE_ANALYSIS
		printf("\n");
		#endif
		printf("    FINAL EXPRESSION [%d]:\n  ", storage.size());
		#endif
		for(std::list<std::string>::const_iterator storage_iter = storage.cbegin();; ++final_iter){
			std::string stored_string = *storage_iter;

			// In case the string is explicit, 
			// we don't need no more backslashes.
			// It cannot be added before, in case it is rematched
			if(stored_string.front() > 31){
				#if FRB_VERBOSE
				std::string prev_string = std::string(stored_string);

				size_t s_size = stored_string.length();
				#endif
				stored_string.erase(std::remove(stored_string.begin(), stored_string.end(), '\\'), 
										stored_string.end());

				#if FRB_VERBOSE
				if(stored_string.length() != s_size)
					printf("%s->", prev_string.c_str());
				#endif
			}
			
			*final_iter = new char[stored_string.length()+1];
			strcpy(*final_iter, stored_string.c_str());

			#if FRB_VERBOSE
			if(stored_string.front() > 31)
				printf("\"%s\"", stored_string.c_str());
			else
				printf("#%d", stored_string.front());

			#endif

			// only for verbose
			if((++storage_iter) == storage.cend())
				break;
			else{
				#if FRB_VERBOSE
				printf(", ");
				#endif
			}

		}
		
		*(final_iter+1) = NULL;

		#if FRB_VERBOSE
		printf("\n\n");
		#endif


		return c_final;
	}

	inline void generate_regex_analizer(){
		// I will not use Mreg to analyze the regex, because
		// in case anything changes, I want to have a slower-safer
		// way to do rebuild the tree.

		#if FRB_VERBOSE
		printf("  Looks like the regex analizer was not generated.\n\tGenerating regex analizer\n");
		#endif

		using umap = std::unordered_map<char, uint>;

		this->lookbehind = std::vector<std::string>{
			"\\", // [^...]
			"\\", // [...]
			"\\", // {m,n}
			"\\", // {,n}
			"\\", // {m,}
			"\\", // {m}
			"\\", // *?, +?
			"\\", // *, +, ?
			"",   // \d ...
			"\\", // (?=, (?:, (?!
			"\\", // (?<=, (?<!
			"\\", // . ( ) |
			"\\", // @ @
		};
		this->regex_analizer = std::vector<std::pair<regex, umap>>{
				// Analizes warps
				{regex(
						(std::string()+static_cast<char>(an_warp)
							+"(.*?)"+static_cast<char>(an_warp)).c_str(), std::regex::ECMAScript), umap{
					{'_',an_warp}
				}},

				// Analizes [^...]
				{regex("\\[\\^(.*?[^\\\\])\\]"),umap{
					{'_',an_not_sq_brack}
				}},
				// Analizes [...]
				{regex("\\[(.*?[^\\\\])\\]"),umap{
					{'_',an_sq_brack}
				}},

				// Analizes {m,n}
				{regex("\\{(\\d+),(\\d+)\\}"),umap{
					{'_',an_bb_brack_m_n}
				}},
				// Analizes {,n}
				{regex("\\{,(\\d+)\\}"),umap{
					{'_',an_bb_brack__n}
				}},
				// Analizes {m,}
				{regex("\\{(\\d+),\\}"),umap{
					{'_',an_bb_brack_m_}
				}},
				// Analizes {m}
				{regex("\\{(\\d+)\\}"),umap{
					{'_',an_bb_brack_m}
				}},

				// Analizes a*? and a+?
				{regex("([*+])\\?"),umap{
					{'*',an_asterisk}, //an_asterisk_question
					{'+',an_plus} // an_plus_question
				}},
				// Analizes a*, a+ and a?
				{regex("([*+?])"),umap{
					{'*',an_asterisk},
					{'+',an_plus},
					{'?',an_question}
				}},

				// Analizes \d, \D, \s, \S, \w, \W, \n, \t, \0
				{regex("\\\\([dDsSwWrt0n])"),umap{
					{'_',an_backw_slash}
				}},

				// Analizes (?=, (?:, (?!
				{regex("\\(?([:!=])"),umap{
					{':',an_not_capture},
					{'=',an_positive_lookahead},
					{'!',an_negative_lookahead}
				}},

				// Analizes (?<=, (?<!
				{regex("\\(?<([!=])"),umap{
					{'=',an_positive_lookbehind},
					{'!',an_negative_lookbehind}
				}},

				// Analizes . ( ) |
				{regex("([.()|])", std::regex::ECMAScript), umap{
					{'.',an_dot},
					{'(',an_start_paren},
					{')',an_end_paren},
					{'|',an_or}
				}},
		};

		if(this->regex_analizer.size() != this->lookbehind.size())
			this->lookbehind.resize(this->regex_analizer.size());

		this->analizer_generated = true;
	}

	T append(const char* expression, const char* nickname){
		if(!this->added_ids.count(expression)){
			this->added_ids[expression] = this->added_id;

			T last_added_id = 0;
			if(! this->added_nicknames.count(nickname)){
				printf(" New nickname [id:%lu]: %s\n", this->added_id, nickname);

				this->added_nicknames[nickname] = this->added_id;
				this->added_final_branches[this->added_id] = branch_t<T>();
			} else{
				last_added_id = this->added_id;

				this->added_id = this->added_nicknames[nickname];

				printf(" Seen nickname [id:%lu] with last %d nodes:", this->added_id,
				 					this->added_final_branches[this->added_id].size());
				for(T node : this->added_final_branches[this->added_id])
					printf(" %d", node);
				printf("\n");
			}

			branch_t<T> branches = this->append(expression);


			printf("Adding %zu final nodes to id:%lu\n", branches.size(), this->added_id-1);

			this->added_final_branches[this->added_id-1].insert(
														branches.begin(), branches.end());

			T aux = this->added_id;
			if(last_added_id)
				this->added_id = last_added_id;

			return aux;
		}

		return this->added_ids[expression];
	}

  	branch_t<T> append(const char* expression){
		// Delete any invalid pointers from
		// last append
		this->delete_pointers();

		T open_parenthesis = 0, close_parenthesis = 0;

		// Calculate expression length
		uint expression_length = 100;
		if(expression_length > this->max_str_size){
			this->max_str_size = expression_length;

			//this->nodes_captures.reserve(expression_length*this->added_id*2);
			//this->str_captures.reserve(expression_length*this->added_id*2);
			//this->str_starts.reserve(expression_length*this->added_id*2);

			this->data[MAX_REG_LENGTH] = expression_length;
		}
		this->contains_captures.push_back(false);

		// Analize the expression
		char **analized_ptr = this->_analize(expression);

		char **analized = analized_ptr;
		char *split;

		#define an_normal_parenthesis 1u
		#define an_no_capture_parenthesis 2u
		#define an_pla_parenthesis 3u
		#define an_nla_parenthesis 4u
		#define an_plb_parenthesis 5u
		#define an_nlb_parenthesis 6u


		std::stack<branch_t<T>> groups_start_branches {{{node_length}}};
		std::stack<char*> groups_start_expr {{*analized}};
		std::stack<uint> parenthesis_type {{an_normal_parenthesis}};

		// Do we need last_last_branches? For *, +, ?
		branch_t<T> last_last_br {};
		branch_t<T> last_branches {};
		// It should not be possible, but just in case,
		// add it so that it will affect an useless possition
		char *last_expression = nullptr;
		branch_t<T> branches {node_length};
		branch_t<T> or_br {};

		bool placed_literal = false;

		while(split = *analized){
			[[likely]];
			switch (*split){
				case an_not_sq_brack:
					// TODO: Add #if FRB_VERBOSE
					printf("Detected: [^%s]\n", split+1);

					{
					std::unordered_set<char> negative = std::unordered_set<char>();

					for(char inserted : reg_dot)
						negative.insert(inserted);

					char first, last;
					while(*(++split)){
						// If exists a '-' in the middle, which is not next
						// to a '\', then it's a range
						if(*split == '-' && *(split-1) != '\\' && 
									*(split-1) > 31 && *(split+1) != '\0'){
							// Add all the chars between the '-'
							if(*(split-1) < *(split+1)){
								first = *(split-1);
								last = *(split+1);
							}
							else{
								first = *(split+1);
								last = *(split-1);
							}

							while(first <= last){
								// If the char is in the negative set,
								// then remove it
								if(negative.count(first))
									negative.erase(first);
								
								++first;
							}
						}
						else
							negative.erase(*split);
					}

					this->new_append_letters<std::unordered_set<char>::const_iterator>
											(branches, negative.cbegin(), negative.cend());
					}

					break;
				
				case an_sq_brack:
					printf("Detected: [%s]\n", split+1);

					// look for '-' ranges in the brackets
					{
					std::unordered_set<char> bracket_chars {};

					char first, last;
					while(*(++split)){
						// If exists a '-' in the middle, which is not next
						// to a '\', then it's a range
						if(*split == '-' && *(split-1) != '\\' && 
									*(split-1) > 31 && *(split+1) != '\0'){
							// Add all the chars between the '-'
							if(*(split-1) < *(split+1)){
								first = *(split-1);
								last = *(split+1);
							}
							else{
								first = *(split+1);
								last = *(split-1);
							}

							while(first <= last)
								bracket_chars.insert(first++);
						}
						else
							bracket_chars.insert(*split);
					}
				
					// TODO: Remove "", since it is no longer necessary
					this->new_append_letters<std::unordered_set<char>::const_iterator>
											(branches, bracket_chars.cbegin(), bracket_chars.cend());

					}
					break;

				case an_bb_brack_m_n:
					printf("Detected: {");
					while(*(++split) != ANALIZE_SEPARATE)
						printf("%c", *split);

					printf(",%s}\n", split);
					break;

				case an_bb_brack_m_:
					printf("Detected: {%s,}\n", split+1);
					break;

				case an_bb_brack_m:
					printf("Detected: {%s}\n", split+1);
					break;

				case an_bb_brack__n:
					printf("Detected: {,%s}\n", split+1);
					break;
				
				case an_asterisk_question:
					printf("Detected: *?\n");
					break;

				case an_plus_question:
					printf("Detected: +?\n");
					break;

				case an_asterisk:
					printf("Detected: *\n");

					break;
				
				case an_plus:
					printf("Detected: +\n");
					break;

				case an_question:
					printf("Detected: ?\n");

					// We need the flow to resume in both the current and
					// previous branches, so we insert those branches.
					branches.insert(last_last_br.begin(), last_last_br.end());
					break;

				case an_backw_slash:
					printf("Detected: \\%s\n", split+1);

					this->new_append_backslash(branches, *(split+1));
					break;

				case an_dot:
					printf("Detected: .\n");

					this->new_append_letters(branches, reg_dot.c_str());
					break;

				case an_start_paren:
					printf("Detected: (\n");

					++open_parenthesis;

					// Add a copy of branches in case we need to use it
					// later in a optional loop (*, ?), where we have to
					// continue and merge both branches.
					groups_start_branches.emplace(branches);
					groups_start_expr.push(split);
					parenthesis_type.push(an_normal_parenthesis);

					++analized;

					// Continue so that last_branches do not get
					// updated.
					continue;

				case an_end_paren:
					printf("Detected: )\n");

					// No use if the capture has just started.
					if(open_parenthesis){
						--open_parenthesis;

						parenthesis_type.pop();
					}
					else {
						++close_parenthesis;

						if(groups_start_branches.size() == 1){
							printf("[-] Detected close parenthesis before open\n");
							throw std::exception();
						}

						{
						branch_t<T> group_capture = groups_start_branches.top();
						groups_start_branches.pop();

						// Add the capture group to last_last_br, in case
						// any loop (*, ?) is applied after the parenthesis
						last_last_br.clear();
						last_last_br.insert(group_capture.begin(), group_capture.end());

						// If there were any or inside the capture group, 
						// we need to add it as branches.
						branches.insert(or_br.begin(), or_br.end());

						// If I need to repeat the first expression in the group
						last_expression = groups_start_expr.top();
						groups_start_expr.pop();
						}
					}

					++analized;

					// Continue so that last_branches do not get
					// updated.
					continue;

				case an_not_capture:
					printf("Detected non-capturing group\n");

					parenthesis_type.push(an_no_capture_parenthesis);
					++open_parenthesis;

					++analized;
					continue;

				case an_positive_lookahead:
					printf("Detected positive lookahead\n");

					parenthesis_type.push(an_pla_parenthesis);
					++open_parenthesis;

					++analized;
					continue;

				case an_negative_lookahead:
					printf("Detected negative lookahead\n");

					parenthesis_type.push(an_nla_parenthesis);
					++open_parenthesis;
					
					++analized;
					continue;

				case an_positive_lookbehind:
					printf("Detected positive lookbehind\n");

					parenthesis_type.push(an_plb_parenthesis);
					++open_parenthesis;
					
					++analized;
					continue;

				case an_negative_lookbehind:
					printf("Detected negative lookbehind\n");

					parenthesis_type.push(an_nlb_parenthesis);
					++open_parenthesis;
					
					++analized;
					continue;

				case an_or:
					printf("Detected: |\n");

					// Add actual final branches to or_br
					or_br.insert(branches.begin(), branches.end());
					
					// Update branches to the previous point
					// where to continue from
					branches.clear();
					{
					branch_t<T> group_start_br = groups_start_branches.top(); 
					
					branches.insert(group_start_br.begin(), group_start_br.end());
					}

					break;

				case an_warp:
					if(placed_literal){
						printf("Detected warp\n", split+1);

						// This case is when we need to match a given sub-group.
						// There will be added to the node+WARP a new node to point
						// to when the matched group is valid.
						for(T node : branches)
							this->data[node+WARP_CAPTURES] |= ~this->warp_mask;
					}
					else{
						goto testing_do_not_generate;
						
						printf("Detected warp of");

						branches.clear();

						while(*++split){			
							printf(" %d", *split);

							branch_t<T> final_nodes = this->added_final_branches[*split];
								
							branches.insert(final_nodes.begin(), final_nodes.end());
						}
						printf("\n  (%d set branches)\n", branches.size());

						for(T node : branches){
							this->data[node+WARP_CAPTURES] |= ~this->semiwarp_mask;
						}
					}

					++analized;
					
					continue;
					
				[[likely]] 
				default: 
					printf("Detected: \"%s\"\n", split);
					//node = this->append_word(node, split);

					placed_literal = true;

					// Cannot use append_letters, since it would add them
					// in the same node
					while(*split)
						this->new_append_letter(branches, *(split++));

					break;
			}
		
			{
			std::stack<uint> aux {};
			// This is done in the character after the parenthesis because it
			// has to be marked in the next node.
			for(; open_parenthesis; --open_parenthesis){
				switch (parenthesis_type.top())
				{
				case an_normal_parenthesis:
					
					this->new_start_group(branches);
					break;
				
				default:
					break;
				}

				aux.push(parenthesis_type.top());
				parenthesis_type.pop();
			}

			while(! aux.empty()){
				parenthesis_type.push(aux.top());
				aux.pop();
			}

			}
			
			// There may not be any use to this, but I think scallability
			// may play an important role in the future.
			for(;close_parenthesis; --close_parenthesis){
				switch(parenthesis_type.top()){
					case an_normal_parenthesis:
						this->new_end_group(branches);
						break;

					default:
						break;
				}

				parenthesis_type.pop();
			}

			// In case we need to loop back or merge optional
			// expressions (?, *), we need to keep track of 
			// what nodes were last time.
			last_last_br.clear();
			last_last_br.insert(last_branches.begin(), last_branches.end());
			last_branches.clear();
			last_branches.insert(branches.begin(), branches.end());
			last_expression = split;

			++analized;
		}

		// At the end, the entire expression is counted as a group.
		// As so, any or stated in a global context is a end point
		// for the expression.
		branches.insert(or_br.begin(), or_br.end());

		analized = analized_ptr;
		while(*analized){
			delete[] *analized;
			
			++analized;
		}
		delete[] analized_ptr;

		printf("\n");

		#if FRB_VERBOSE
		if(branches.size() > 1)
			printf("Detected multiple final %u branches\n  ", branches.size());
		else
			printf("  ");
		#endif

testing_do_not_generate:

		for(T node : branches){
			this->data[node+FINAL] = this->added_id;

			#if FRB_VERBOSE
			printf("%u,",node); 
			#endif
		}


		#if FRB_VERBOSE
		printf("\nAdded new expression with id:%u\n", this->added_id);
		#endif

		++this->data[NUM_ADDED];
		++this->added_id;

		this->data[SIZE] = this->data.size();

		return branches;
	} 

	// Append inline sub-functions
	inline uint append_letter(const uint node, const char* expr, bool regexpr=false, const uint to=0){
		// If not regexpr we can be sure it has been part of a loop or branching
		// And so, right now, it has just ended
		if(!regexpr){ [[likely]]
			// Count the amount of letters seen
			//(*this->count)[*expr]++;

			// If exists a GROUP_ID and it is the same as this append (every expr append has
			// a unique ID) it means last letter append was actually a charset, and we must
			// converge them all to one
			if(this->data[node+GROUP_ID] == this->added_id){
				uint result = this->data[node+NEXT];
				
				group_t<uint>* group = reinterpret_cast<group_t<uint>*>(this->data[node+GROUP]);

				#if FRB_VERBOSE
					printf("Detected post multi-regex node %u of length %i\n", node, group->size());
				#endif
				for(group_t<uint>::const_iterator n = group->cbegin(); n != group->end(); ++n){
					this->data[*n+*expr+char_offset] = result;
					#if FRB_VERBOSE
					printf(" %u -> %u\n", (*n), result);
					#endif
				}

				return result;
			}
		}
		# if FRB_VERBOSE
		else
			printf("Found clean append_letter (prolly after charset)\n");
		#endif

		// Si ya existe dicha letra, avanza con el
		// objeto al siguiente array.
		if(this->data[node+*expr+char_offset]){
			return append_letter_exist(node, expr);
		}
		else {
			return append_letter_no_exist(node, expr, to);
		}
	}

	inline uint append_letter_exist(const uint node, const char* expr){
		#if FRB_VERBOSE
		printf("Letter exists\n");
		#endif
		// Next node to go
		uint lett = this->data[node+*expr+char_offset];
		
		#if FRB_VERBOSE
		if(lett)
			printf("Next node has %li links\n", this->data[lett+LINKS]);
		#endif

		// Si el objeto estaba apuntado por un charset,
		// duplica el nodo, pues los charset no van a seguir
		// el mismo camino
		if((*(expr+1) != '\\' || this->data[lett+*(expr+1)+char_offset])
					&& this->data[lett+LINKS] > 1){
			return this->copy(node, *expr);
		}
		else{
			capture_t<long unsigned int>* capture_merge = new capture_t<long unsigned int>();

			// Cast lett+CAPTURE and node+CAPTURE to capture_t<long unsigned int>*, and merge
			// them into capture_merge.
			capture_t<long unsigned int>* merged_capture;
			
			if(this->data[node+WARP_CAPTURES]){
				merged_capture = reinterpret_cast<capture_t<long unsigned int>*>(this->data[node+WARP_CAPTURES]);

				if(!this->deleted.count(this->data[node+WARP_CAPTURES]))
					capture_merge->insert(capture_merge->end(), merged_capture->begin(), merged_capture->end());
			}
			if(this->data[node+WARP_CAPTURES]){
				merged_capture = reinterpret_cast<capture_t<long unsigned int>*>(this->data[lett+WARP_CAPTURES]);

				if(!this->deleted.count(this->data[node+WARP_CAPTURES]))
					capture_merge->insert(capture_merge->end(), merged_capture->begin(), merged_capture->end());
			
				if(this->data[lett+LINKS] == 1)
					delete merged_capture;
				else
					// Add capture to unknown_ptrs
					this->unknown_ptrs.push_back(merged_capture);
			}

			if(capture_merge->size())
				this->data[lett+WARP_CAPTURES] = reinterpret_cast<T>(capture_merge);
			else
				delete capture_merge;
		}

		#if FRB_VERBOSE
		printf("%u -> %u\n", node, lett);
		#endif
		return lett;
	}

	inline uint append_letter_no_exist(const uint node, const char* expr, const uint to=0){
		#if FRB_VERBOSE
		printf("Letter does not exist");
		#endif
		if(to){
			#if FRB_VERBOSE
			printf(" (shared)\n%u -> %u\n", node, to);
			#endif
			this->add(node, *expr, to);

			return to;
		}
		#if FRB_VERBOSE
		printf("\n");
		#endif
		return this->generate(node,*expr);
	}

	inline void new_append_letter(branch_t<T> &br, const char letter, const T to=0){
		// If there is an explicit node we should route nodes to,
		// define "merge" to it.
		T merge = 0;
		branch_t<T> new_branches {};

		for(T node : br){
			[[likely]];

			// If the letter in node does not route to
			// any other node (aka is empty):
			if(!this->data[node+letter+char_offset]){
				// If "to" was 0, and exists any letter that is
				// empty, generate a new node to merge any other
				// empty letters to. If all were occupied,
				// there is no need to generate merge.
				if(!merge)
					new_branches.insert(merge = (to ? to : this->new_node()));

				#if FRB_VERBOSE
				printf("  %u - %c -> %d  (merged)\n", node, letter, merge);
				#endif
				this->add(node, letter, merge);
			}
			// If the letter is actually occupied:
			else {
				T occupied_node = this->data[node+letter+char_offset];

				// If the node is linked from some other nodes, 
				// we need to duplicate it, because otherwise
				// we could end up in a "final" node we don't want
				// to.
				if(this->data[occupied_node+LINKS] > 1){
					// Use this version of copy to make sure the new node
					// is set in the place of the previous
					occupied_node = this->copy(node, letter);

					#if FRB_VERBOSE
					printf("  %u - %c -> %d  (copied)\n", node, letter, occupied_node);
					#endif
				}
				#if FRB_VERBOSE
				else
					printf("  %u - %c -> %d  (advanced)\n", node, letter, occupied_node);
				#endif

				new_branches.insert(occupied_node);
			}
		}

		// Swap all the old branches for new ones
		br.clear();
		br.insert(new_branches.begin(), new_branches.end());
	}

	template<typename iter_t>
	inline void new_append_letters(branch_t<T> &br, iter_t letters, iter_t l_end, const T to=0){
		// If there is an explicit node we should route nodes to,
		// define "merge" to it.
		T merge = 0;
		branch_t<T> new_branches {};

		// This is a duplicate of new_append_letter. However,
		// I could not find an intuitive way of keeping a merge
		// without generating it before knowing if it was necessary
		// and shared between all loops of nodes & letters
		while(letters != l_end){
			[[likely]];
			char letter = *letters;

			for(T node : br){
				[[likely]];

				// If the letter in node does not route to
				// any other node (aka is empty):
				if(!this->data[node+letter+char_offset]){
					// If "to" was 0, and exists any letter that is
					// empty, generate a new node to merge any other
					// empty letters to. If all were occupied,
					// there is no need to generate merge.
					// Add it to new_branches, to continue working with it
					if(!merge)
						new_branches.insert(merge = (to ? to : this->new_node()));
					

					#if FRB_VERBOSE
					printf("  %u - %c -> %d  (merged)\n", node, letter, merge);
					#endif
					this->add(node, letter, merge);
				}
				// If the letter is actually occupied:
				else {
					T occupied_node = this->data[node+letter+char_offset];

					// If the node is linked from some other nodes, 
					// we need to duplicate it, because otherwise
					// we could end up in a "final" node we don't want
					// to.
					if(this->data[occupied_node+LINKS] > 1){
						// Use this version of copy to make sure the new node
						// is set in the place of the previous
						occupied_node = this->copy(node, letter);

						#if FRB_VERBOSE
						printf("  %u - %c -> %d  (copied)\n", node, letter, occupied_node);
						#endif
					}
					#if FRB_VERBOSE
					else
						printf("  %u - %c -> %d  (advanced)\n", node, letter, occupied_node);
					#endif

					new_branches.insert(occupied_node);
				}
			}

			++letters;
		}

		// Swap all the old branches for new ones
		br.clear();
		br.insert(new_branches.begin(), new_branches.end());
	}

	inline void new_append_letters(branch_t<T> &br, const char* letters, char l_end='\0', const T to=0){
		// If there is an explicit node we should route nodes to,
		// define "merge" to it.
		T merge = 0;
		branch_t<T> new_branches {};

		char letter;

		// This is a duplicate of new_append_letter. However,
		// I could not find an intuitive way of keeping a merge
		// without generating it before knowing if it was necessary
		// and shared between all loops of nodes & letters
		while((letter = *letters) != l_end){
			[[likely]];

			for(T node : br){
				[[likely]];

				// If the letter in node does not route to
				// any other node (aka is empty):
				if(!this->data[node+letter+char_offset]){
					// If "to" was 0, and exists any letter that is
					// empty, generate a new node to merge any other
					// empty letters to. If all were occupied,
					// there is no need to generate merge.
					// Add it to new_branches, to continue working with it
					if(!merge)
						new_branches.insert(merge = (to ? to : this->new_node()));
					

					#if FRB_VERBOSE
					printf("  %u - %c -> %d  (merged)\n", node, letter, merge);
					#endif
					this->add(node, letter, merge);
				}
				// If the letter is actually occupied:
				else {
					T occupied_node = this->data[node+letter+char_offset];

					// If the node is linked from some other nodes, 
					// we need to duplicate it, because otherwise
					// we could end up in a "final" node we don't want
					// to.
					if(this->data[occupied_node+LINKS] > 1){
						// Use this version of copy to make sure the new node
						// is set in the place of the previous
						occupied_node = this->copy(node, letter);

						#if FRB_VERBOSE
						printf("  %u - %c -> %d  (copied)\n", node, letter, occupied_node);
						#endif
					}
					#if FRB_VERBOSE
					else
						printf("  %u - %c -> %d  (advanced)\n", node, letter, occupied_node);
					#endif

					new_branches.insert(occupied_node);
				}
			}

			++letters;
		}

		// Swap all the old branches for new ones
		br.clear();
		br.insert(new_branches.begin(), new_branches.end());
	}

	inline uint append_letters(const uint node, const std::string expr, const char* next, const uint to=0){
		uint new_reg, seen_reg, new_arr, existing_node;
		group_t<uint>* group;

		if(to && this->data[to+GROUP])
			group = reinterpret_cast<group_t<uint>*>(this->data[to+GROUP]);
		else		 
		 	group = new group_t<uint>();

		group->reserve(group->size()+expr.length());

		// Common node for all letters that do not exist
		if(to){
			seen_reg = new_arr = to;
		} else if(this->data[node+NEXT])
			seen_reg = new_arr = this->data[node+NEXT];
		else {
			seen_reg = this->data.size();
			this->new_node();
			new_arr = this->data.size();
			this->new_node();
		}

		#if FRB_VERBOSE
		printf("Append of letters \"%s\"\n", expr.c_str());
		#endif

		std::unordered_map<uint, uint> exist_common {};
		std::unordered_map<uint, uint>::const_iterator exists;
		exist_common.reserve(expr.length());

		for(const char* letts = expr.c_str(); *letts; ++letts){
			// If node+letts has no existing node, add to the new node
			if(!(existing_node = this->data[node+*letts+char_offset])){
				#if FRB_VERBOSE
				printf(" - '%c' not exist\n %i -> %i\n", *letts, node, seen_reg);
				#endif
				this->add(node, *letts, seen_reg);
			} else {
				#if FRB_VERBOSE
				printf(" - '%c' exist\n", *letts);
				#endif
				// Find if any other lett has the same node pointing to it
				exists = exist_common.find(existing_node);
				
				// If there isn't (no shared node), duplicate the node
				if(exists == exist_common.end()){
					new_reg = this->append_letter_exist(node, std::string({*letts, *next}).c_str());
					this->data[new_reg+GROUP_ID] = this->added_id;
					this->data[new_reg+GROUP] = reinterpret_cast<T>(group);
					this->data[new_reg+NEXT] = new_arr;
					
					group->insert(new_reg);
					exist_common[existing_node] = new_reg;
				}
				else {
					// If any letter has been seen to go to the same node,
					// go to it too. Note: any duplication has already
					// been done.
					#if FRB_VERBOSE
					printf("Letter exists (shared)\n");
					#endif
					this->add(node, *letts, exists->second);
				}
				#if FRB_VERBOSE
				printf("%i -> %i\n", node, new_reg);
				#endif
			}
		}

		// If any of the letters did not exist, we add the common node
		if(this->data[seen_reg+LINKS]){
			this->data[seen_reg+GROUP_ID] = this->added_id;
			this->data[seen_reg+GROUP] = reinterpret_cast<T>(group);
			this->data[seen_reg+NEXT] = new_arr;
			this->data[seen_reg+WARP_CAPTURES] = this->data[node+WARP_CAPTURES];
			
			group->insert(seen_reg);

			this->check_capture(seen_reg);

			#if FRB_VERBOSE
			printf("Shared node has %i links\n", this->data[seen_reg+LINKS]);
			#endif

			return seen_reg;
		}

		this->check_capture(new_reg);

		return new_reg;
	}

	inline void new_append_backslash(branch_t<T> &br, const char backslash_letter, T to=0){
		std::unordered_set<char> neg;

		switch(backslash_letter){
			case 'd':
				this->new_append_letters(br, reg_d.c_str(), '\0', to);
				break;
			case 'w':
				this->new_append_letters(br, reg_w.c_str(), '\0', to);
				break;
			case 'n':
				this->new_append_letter(br, reg_n, to);
				break;
			case 't':
				this->new_append_letter(br, reg_t, to);
				break;
			case 'r':
				this->new_append_letter(br, reg_r, to);
				break;
			case '0':
				this->new_append_letter(br, reg_z, to);
				break;
			case 's':
				this->new_append_letters(br, reg_s.c_str(), '\0', to);
				break;
			case 'D':
				neg = invert_chars(reg_d.c_str());

				this->new_append_letters<std::unordered_set<char>::const_iterator>
												(br, neg.cbegin(), neg.cend(), to);
				break;
			case 'W':
				neg = invert_chars(reg_w.c_str());

				this->new_append_letters<std::unordered_set<char>::iterator>
												(br, neg.begin(), neg.end(), to);
				break;
			case 'N':
				neg = invert_chars(reg_n);

				this->new_append_letters<std::unordered_set<char>::iterator>
												(br, neg.begin(), neg.end(), to);
				break;
			case 'T':
				neg = invert_chars(reg_t);

				this->new_append_letters<std::unordered_set<char>::iterator>
												(br, neg.begin(), neg.end(), to);
				break;
			case 'R':
				neg = invert_chars(reg_r);

				this->new_append_letters<std::unordered_set<char>::iterator>
												(br, neg.begin(), neg.end(), to);
				break;
			case 'S':
				neg = invert_chars(reg_s.c_str());

				this->new_append_letters<std::unordered_set<char>::iterator>
												(br, neg.begin(), neg.end(), to);
				break;
			default:
				throw std::invalid_argument("Wrong regex construction \\"+backslash_letter);
		}
	}

	inline uint append_backslash(const uint node, const char* expr, T to=0){
		if(this->data[node+NEXT])
			to = this->data[node+NEXT];

		switch(*expr){
			case 'd':
				return this->append_letters(node, reg_d, expr+1, to);
			case 'w':
				return this->append_letters(node, reg_w, expr+1, to);
			case 'n':
				return this->append_letter(node, std::string({reg_n, *(expr+1)}).c_str(), false, to);
			case 't':
				return this->append_letter(node, std::string({reg_t, *(expr+1)}).c_str(), false, to);
			case 'r':
				return this->append_letter(node, std::string({reg_r, *(expr+1)}).c_str(), false, to);
			case '0':
				return this->append_letter(node, std::string({reg_z, *(expr+1)}).c_str(), false, to);
			case 's':
				return this->append_letters(node, reg_s, expr+1, to);

			// cases on caps, inverse

			default:
				printf("\n### Wrong regex construction \\%c in \"%s\"\n\n", *expr, expr);
				throw std::invalid_argument("Wrong regex construction \\"+*expr);
		}
	}

	uint append_process_sq(const uint node, const char** expr){
		std::string letts = "";
		const char* expression = *expr;
		++expression;
		uint dist = 0;

		bool backslash = false;
		for(; *expression && *expression != ']'; ++expression){
			if(!backslash){
				if(*expression == '\\'){
					backslash = true;
					continue;
				}
				if(*expression == '-'){
					++expression;
					
					if(*expression == '\\')
						++expression;
					else if(*expression == ']')
						break;

					for(int let = letts.back()+1; let <= *expression; ++let)
						letts.push_back(let);
					continue;
				}
			} else 
				backslash = false;

			letts.push_back(*expression);
		}

		*expr = *expr+std::distance(*expr, expression);

		std::unordered_set<char> uniq_letts(letts.begin(), letts.end());

		return this->append_letters(node, 
					std::string(uniq_letts.begin(), uniq_letts.end()), 
					expression+1);
	}

	inline uint append_process_bbl(uint node, const char** expr){
			bool both = false;
			uint min = 0, max = 0;

			std::string buffer = "";
			const char* iter = *expr;
			char it_char, letter = *((*expr)-1);
			
			#if FRB_VERBOSE
			printf("expr: %c{", *((*expr)-1));
			#endif

			while((it_char=*(++iter)) != '}'){
				#if FRB_VERBOSE
				printf("%c", it_char);
				#endif
				if(it_char != ',') [[likely]]
					if(isdigit(it_char)) [[likely]]
						buffer += it_char;
					else
						throw std::invalid_argument("wrong values passed between squared brackets {}");
				else{
					min = std::stoi(buffer);
					both = true;
					buffer.erase();
				}
			}

			*expr = (*expr)+std::distance(*expr, iter);
			max = buffer!="" ? std::stoi(buffer) : 0;

			/* Result:
				· max, min > 0:
					from min to max
				· min = 0:
					max times
				· max = 0
					from min to infinity
			*/

			#if FRB_VERBOSE
			printf("}\n");
			#endif

			if (max == 0){
				#if FRB_VERBOSE
				printf(" %u to infinity the letter %c\n", min, letter);
				#endif

				// Add the minimum letters to start the loop
				for(; min > 1; --min)
					node = this->append_letter(node, (std::string(min, letter)+**expr).c_str());

				// Loop forever if the given letter is found
				this->append_letter(node, std::string(3, letter).c_str(), false, node);

			} 
			else if(min != 0){
				#if FRB_VERBOSE
				printf(" %u to %u times the letter %c\n", min, max, letter);
				#endif

				// Removee one from min, since the first node has already
				// been added
				min -= 1;
				uint diff = max-min;

				uint new_arr = this->data.size();
				this->new_node();

				group_t<uint>* group = new group_t<uint>();
				group->reserve(diff);

				// Get minimum letters found to allow continue matching
				for(; min > 1; --min)
					node = this->append_letter(node, (std::string(min, letter)+**expr).c_str());

				// Add the max-min remaining nodes with exits at any point to the same node
				for(; diff; --diff){
					node = this->append_letter(node, (std::string(diff, letter)+**expr).c_str());
					
					group->insert(node);
				}


				#if FRB_VERBOSE
				printf("Added to group nodes: {");
				#endif
				for(uint reg : *group){
					this->data[reg+GROUP_ID] = this->added_id;
					this->data[reg+GROUP] = reinterpret_cast<T>(group);
					this->data[reg+NEXT] = new_arr;

					#if FRB_VERBOSE
					printf("%u, ", reg);
					#endif
				}
				#if FRB_VERBOSE
				printf("}\n");
				#endif

			} 
			else{
				#if FRB_VERBOSE
				printf(" %u times the letter %c\n", max, letter);
				#endif

				// Removee one from max, since the first node has already
				// been added
				max -= 1;

				for(; max; --max)
					node = this->append_letter(node, (std::string(max, letter)+**expr).c_str());
			}

			return node;
		}
	
	inline uint append_question_mark(uint node, uint last){
		uint next = this->data.size();
		this->new_node();

		group_t<uint>* group, * last_group;
		
		if(this->data[node+GROUP_ID] != this->added_id || !this->data[node+GROUP]){
			group = new group_t<uint> {node};

			this->data[node+NEXT] = next;
			this->data[node+GROUP_ID] = this->added_id;
			this->data[node+GROUP] = reinterpret_cast<T>(group);
		}
		else {
			group = reinterpret_cast<group_t<uint>*>(this->data[node+GROUP]);

			for(group_t<uint>::const_iterator n = group->cbegin(); n != group->end(); ++n){
				this->data[*n+NEXT] = next;
			}
		}

		// Add the last non-optional to the group
		if (this->data[last+GROUP_ID] == this->added_id && this->data[last+GROUP]){
			last_group = reinterpret_cast<group_t<uint>*>(this->data[last+GROUP]);
			this->deleted.insert(this->data[last+GROUP]);

			for(group_t<uint>::const_iterator n = last_group->cbegin(); n != last_group->end(); ++n){
				group->insert(*n);

				this->data[*n+NEXT] = next;
				this->data[*n+GROUP_ID] = this->added_id;
				this->data[*n+GROUP] = reinterpret_cast<T>(group);
			}

			delete last_group;
		} else {
			group->insert(last);
			
			this->data[last+NEXT] = next;
			this->data[last+GROUP_ID] = this->added_id;
			this->data[last+GROUP] = reinterpret_cast<T>(group);
		}
		
		
		return last;
	}
	
	inline uint append_asterisk(uint node, uint last, const char* expr){
		this->append_question_mark(node, last);

		uint new_node;

		if(is_regex(expr-1)){
			new_node = this->append_backslash(node, expr-1, node);
		}
		else{
			new_node = this->append_letter_no_exist(node, expr-1, node);
		}

		return last;
	}
	
	inline uint append_plus(uint node, const char* expr){
		uint plus_1 = this->data.size();
		this->new_node();

		group_t<uint>* g_plus;

		if(is_regex(expr)){
			plus_1 = this->append_backslash(node, expr, plus_1);
			this->append_backslash(plus_1, expr, plus_1);

			g_plus = reinterpret_cast<group_t<uint>*>(this->data[plus_1+GROUP]);
		} else {
			plus_1 = this->append_letter(node, expr, plus_1);
			this->append_letter_no_exist(plus_1, expr, plus_1);

			g_plus = new group_t<uint>{plus_1};
			this->data[plus_1+GROUP] = reinterpret_cast<T>(g_plus);
			this->data[plus_1+GROUP_ID] = this->added_id;
		}

		if(this->data[node+GROUP_ID] == this->added_id && this->data[node+GROUP]){
			group_t<uint>* g_node = reinterpret_cast<group_t<uint>*>(this->data[node+GROUP]);
			this->deleted.insert(this->data[node+GROUP]);
		
			for(group_t<uint>::const_iterator n = g_node->cbegin(); n != g_node->end(); ++n){
				g_plus->insert(*n);

				this->data[*n+GROUP] = reinterpret_cast<T>(g_plus);
				this->data[*n+GROUP_ID] = this->added_id;
			}

			delete g_node;
		} else {
			g_plus->insert(node);
			this->data[node+GROUP] = reinterpret_cast<T>(g_plus);
			this->data[node+GROUP_ID] = this->added_id;
		}

		return plus_1;
	}
	// Append inline sub-functions

  	inline uint generate(const uint node, const char pos){
		uint new_arr = this->data.size();
		this->new_node();
		#if FRB_VERBOSE
			printf("Generated array [%c %i] {%u -> %u}\n %i -> %i\n", pos, pos, node+pos+char_offset, new_arr, node, new_arr);
		#endif

		add(node, pos, new_arr);
		return new_arr;
	}

	inline void add(const T node, const char pos, const T new_arr){
		++this->data[new_arr+LINKS];
		
		this->data[node+pos+char_offset] = new_arr;

		this->check_capture(new_arr);
	}

	void check_capture(const uint node){
		for(size_t cap = this->capture.size(); cap; --cap){
			if(this->capture.back() == 1)
				this->start_group(node);
			else
				this->end_group(node);

			this->capture.pop();
		}
	}

	inline T copy(const T node, const char pos){
		T copied = this->copy(this->data[node+pos+char_offset]);
		#if FRB_VERBOSE
		printf("Copied array in %u[%c %i] {new: %u}\n", node, pos, pos, copied);
		#endif

		this->add(node, pos, copied);

		return copied;
	}

	inline T copy(const T copied){
		T new_arr = this->new_node();
		
		// Add the following letters(nodes) from the original
		auto data_ptr = this->data.begin()+copied+control_positions;
		
		for(auto pos_val = control_positions; pos_val != node_length; 
									++pos_val, ++data_ptr){
			if(*data_ptr){
				this->data[new_arr+pos_val] = *data_ptr;
			}
		}
		#if FRB_VERBOSE
		bool copied_data[control_positions];
		#endif

		// Add the data from the original
		T pos_val = 0;
		for(; pos_val < control_positions; ++pos_val){
			#if FRB_VERBOSE
				if(this->data[new_arr+pos_val] = this->data[copied+pos_val])
					copied_data[pos_val] = true;
			#else
				this->data[new_arr+pos_val] = this->data[copied+pos_val];
			#endif
		}
		#if FRB_VERBOSE
		printf("\n Data: ");
		for(uint c_data_pos = 0; c_data_pos < control_positions; ++c_data_pos)
			if(copied_data[c_data_pos])
				printf("%u, ", c_data_pos);
		printf("\n");
		#endif

		this->data[copied+LINKS] = 1;

		return new_arr;
	}
  
	inline void new_start_group(const branch_t<T> &br){
		#if FRB_VERBOSE
		printf("Opening %d captures\n", br.size()); fflush(stdout);
		#endif

		for(T node : br){
			printf("%u", node); fflush(stdout);
			if(!this->data[node+WARP_CAPTURES])
				this->data[node+WARP_CAPTURES] = reinterpret_cast<T>(
												new capture_t<T>{this->added_id}
											);
			else
				reinterpret_cast<capture_t<T>*>(this->data[node+WARP_CAPTURES])
									->push_back(this->added_id);

			#if FRB_VERBOSE
			printf("\tWARP_CAPTURES %u : 0x%x\n", node, this->data[node+WARP_CAPTURES]);
			#endif
		}
	}

	inline void start_group(const uint node){
		if(!this->data[node+WARP_CAPTURES]){
				this->data[node+WARP_CAPTURES] = reinterpret_cast<T>(
												new capture_t<long unsigned int>()
											);
		}

		if(this->data[node+GROUP_ID] == this->added_id && this->data[node+GROUP]){
			// Open the capture in the group of generated nodes
			group_t<uint>* group = reinterpret_cast<group_t<uint>*>(this->data[node+GROUP]);

			#if FRB_VERBOSE
			printf("Opening %d captures\n", group->size());
			#endif
			for(group_t<uint>::const_iterator n = group->cbegin(); n != group->end(); ++n)
				if(!this->data[(*n)+WARP_CAPTURES]){
					this->data[(*n)+WARP_CAPTURES] = reinterpret_cast<T>(
												new capture_t<long unsigned int> {this->added_id}
												);
					#if FRB_VERBOSE
					printf("\tWARP_CAPTURES %u : 0x%x\n", (*n), this->data[(*n)+WARP_CAPTURES]);
					#endif
				}
				else
					reinterpret_cast<capture_t<long unsigned int>*>(this->data[(*n)+WARP_CAPTURES])
									->push_back(this->added_id);
		}
		else{
			reinterpret_cast<capture_t<long unsigned int>*>(this->data[node+WARP_CAPTURES])
							->push_back(this->added_id);
			#if FRB_VERBOSE
			printf("\tCAPTURE START %u : 0x%x\n", node, this->data[node+WARP_CAPTURES]);
			#endif
		}

		this->contains_captures[this->added_id] = true;
	}

	inline void new_end_group(const branch_t<T> &br){
		#if FRB_VERBOSE
		printf("Closing %d captures\n", br.size());
		#endif

		for(T node : br){
			if(!this->data[node+WARP_CAPTURES])
				this->data[node+WARP_CAPTURES] = reinterpret_cast<T>(
												new capture_t<T>{-this->added_id}
											);
			else
				reinterpret_cast<capture_t<T>*>(this->data[node+WARP_CAPTURES])
									->push_front(-this->added_id);

			#if FRB_VERBOSE
			printf("\tCAPTURES %u : 0x%x\n", node, this->data[node+WARP_CAPTURES]);
			#endif
		}
	}

	inline void end_group(const uint node){
		if(!this->data[node+WARP_CAPTURES]){
			this->data[node+WARP_CAPTURES] = reinterpret_cast<T>(
											new capture_t<long unsigned int>()
										);
			#if FRB_VERBOSE
			printf("\tWARP_CAPTURES %u : 0x%x\n", node, this->data[node+WARP_CAPTURES]);
			#endif
		}


		if(this->data[node+GROUP_ID] == this->added_id && this->data[node+GROUP]){
			// Close the capture in the group of generated nodes
			group_t<uint>* group = reinterpret_cast<group_t<uint>*>(this->data[node+GROUP]);
		
			#if FRB_VERBOSE
			printf("Closing %d captures\n", group->size());
			#endif
			for(group_t<uint>::iterator n = group->begin(); n != group->end(); ++n){
				reinterpret_cast<capture_t<long unsigned int>*>(this->data[node+WARP_CAPTURES])
							->push_front(-this->added_id);
							
				#if FRB_VERBOSE
				printf("\tWARP_CAPTURES %u : 0x%x\n", (*n), this->data[(*n)+WARP_CAPTURES]);
				#endif
			}
		}
		else{
			reinterpret_cast<capture_t<long unsigned int>*>(this->data[node+WARP_CAPTURES])
							->push_front(-this->added_id);
			#if FRB_VERBOSE
			printf("\tCAPTURE END %u : 0x%x\n", node, this->data[node+WARP_CAPTURES]);
			#endif
		}

	}
};

#endif