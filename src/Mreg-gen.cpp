#ifndef FRB_GEN_CPP
#define FRB_GEN_CPP

#define FRB_GENERATE true

#ifndef FRB_MATCH
#define FRB_MATCH true
#endif

// Local includes
#include <set>
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
inline bool ends_with(std::string const & value, std::string const & ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

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
		std::unordered_map<std::string, branch_t<T>> final_nicknames;

		std::set<T> all_states;
		std::unordered_map<T, void*> tmp_states;

    Mreg_gen() : super(){
		this->deleted = std::unordered_set<T>();
		this->unknown_ptrs = std::list<capture_t<long unsigned int>*>();

		this->added_ids = std::unordered_map<std::string, T>();
		this->added_nicknames = std::unordered_map<std::string, T>();
		this->added_final_branches = std::unordered_map<T, branch_t<T>>();
		this->final_nicknames = std::unordered_map<std::string, branch_t<T>>();

		this->tmp_states = std::unordered_map<T, void*>();
		this->all_states = std::set<T>();
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

		// If the analizer is not generated, we need to create it
		if(! this->analizer_generated)
			this->generate_regex_analizer();

		// The list with the processed substrings
		// To fill in what we have analized
		std::vector<std::string> storage = std::vector<std::string>{c_raw_expr};
		std::vector<std::string> tmp_storage = std::vector<std::string>();

		std::smatch match;
		uint last;
		std::string expr_part;

		std::vector<std::string>::const_iterator lb_iter = this->lookbehind.begin();

		// Iterate over the expression, looking for regex
		// patterns. If there is one, add it to expression
		// Get matcher (regex) and flags (map<char, id>)
		// in the entire regex analize and start matching
		// all split strings in storage.
		for(const auto& reg_tuple : this->regex_analizer){
			// match the matcher regex against all strings in storage
			for(std::string str : storage){
				while(std::regex_search(str, match, reg_tuple.first)){
					#if FRB_VERBOSE
					printf("Got analisis match \"%s\" ", match.str().c_str());
					#endif

					if(lb_iter->empty() || !ends_with(match.prefix(), *lb_iter)){
						#if FRB_VERBOSE
						printf("valid\n");
#endif
						// Since we know that the negative lookbehind is
						// not matched, we can apply it.
					
						// First, we add the unmatched prefix of str
						if(match.prefix().length())
							tmp_storage.emplace_back(match.prefix());
						
						// Then we need to check what match it really is.
						if(reg_tuple.second.size() == 1){
							// Since there is only one option, may as well
							// use it.
							// flags will return the int reference of the
							// match id.
							std::string match_part = std::string()+(char)reg_tuple.second.at('_');

							// And then add the matched part
							// if there is any
							for (uint i = 1; i != match.size(); ++i)
								match_part += match[i];

							tmp_storage.emplace_back(match_part);
						} else {
							// We now have to check what match it really is.
							// We do this by checking the flags map.
							//
							// It is known the match is only one char
							//
							// flags will return the int reference of the
							// match id.
							tmp_storage.emplace_back(std::string()+(char)reg_tuple.second.at(match[1].str().front()));
						}
					} else if(!lb_iter->empty()){
						#if FRB_VERBOSE
						printf("neglected\n");
						#endif
						// We got a match, but it was neglected by
						// the negative lookbehind.
						std::string prefix = match.prefix();

						for(uint i = 0; i != lb_iter->size(); ++i)
							prefix.pop_back();

						tmp_storage.emplace_back(prefix + match[0].str());
					}

					str.assign(match.suffix());
				}

				if(str.length())
					tmp_storage.emplace_back(str);
			}

			storage.assign(tmp_storage.begin(), tmp_storage.end());
			tmp_storage.clear();

			++lb_iter;
		}

		char** c_final = new char*[storage.size() + 1];
		char** final_iter = c_final;

		#if FRB_VERBOSE
		#if FRB_VERBOSE_ANALYSIS
		printf("\n");
		#endif
		printf("    FINAL EXPRESSION [%d]:\n  ", storage.size());
		#endif
		
		for(std::string & final_part : storage){
			if(final_part.front() > 31){
			final_part = std::regex_replace(final_part, std::regex("\\\\"), std::string(""));
				#if FRB_VERBOSE
				printf("\"%s\" ", final_part.c_str());
			} else {
				printf("#%d ", final_part.front());
			#endif
			}

			*final_iter = new char[final_part.size() + 1];
			strcpy(*final_iter, final_part.c_str());
			
			++final_iter;
		}
		*final_iter = nullptr;

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
			"",   // Warps
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
						(std::string()+static_cast<char>(an_warp)).c_str()), umap{
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

	void clean_gen(){
		this->clean();

		//this->setup_states();
	}

	void store_gen(){
		//this->store();

	}

	T append(const char* expression, const char* nickname, T id = 0){
		// Check we have not added the same expression before
		if(!this->added_ids.count(expression)){
			this->added_ids[expression] = this->added_id;

			T last_added_id = 0;
			// If we have already added the nickname, 
			// we will use the same id
			//
			// If we have not added the nickname
			if(! this->added_nicknames.count(nickname)){
				// If the id is given, we will use it
				if(id != 0){
					last_added_id = this->added_id;
					this->added_id = id;
				}
				printf(" New nickname [id:%lu]: %s \n  [%s]\n", this->added_id, nickname, expression);

				this->added_nicknames[nickname] = this->added_id;
				this->added_final_branches[this->added_id] = branch_t<T>();
			} else{
				// If the nickname was already used
				last_added_id = this->added_id;

				this->added_id = this->added_nicknames[nickname];

				printf(" Seen nickname [id:%lu] with last %d nodes:", this->added_id,
				 					this->added_final_branches[this->added_id].size());
				for(T node : this->added_final_branches[this->added_id])
					printf(" %d", node);
				printf("\n");
			}

			branch_t<T> branches = this->append(expression, false);


			//printf("Adding %zu final nodes to id:%lu\n", branches.size(), this->added_id-1);

			this->added_final_branches[this->added_id].insert(
														branches.begin(), branches.end());

			this->final_nicknames[std::string(nickname)] = this->added_final_branches[this->added_id];

			T aux = this->added_id;
			if(last_added_id)
				this->added_id = last_added_id;
			else {
				++this->added_id;
				++this->data[NUM_ADDED];
			}

			return aux;
		}

		return this->added_ids[expression];
	}

  	branch_t<T> append(const char* expression, bool update_id = true){
		// Delete any invalid pointers from
		// last append
		this->delete_pointers();

		T open_parenthesis = 0, close_parenthesis = 0,
		  open_warp = 0;

		// Calculate expression length
		uint expression_length = 100;
		if(expression_length > this->max_str_size){
			this->max_str_size = expression_length;

			//this->nodes_captures.reserve(expression_length*this->added_id*2);
			//this->str_captures.reserve(expression_length*this->added_id*2);
			//this->str_starts.reserve(expression_length*this->added_id*2);

			this->data[MAX_REG_LENGTH] = expression_length;
		}
		this->contains_captures[this->added_id] = false;

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


		std::stack<branch_t<T>> groups_start_branches {{{initial_position}}};
		std::stack<char*> groups_start_expr {{*analized}};
		std::stack<uint> parenthesis_type {{an_normal_parenthesis}};

		// Do we need last_last_branches? For *, +, ?
		branch_t<T> last_last_br {};
		branch_t<T> last_branches {};
		// It should not be possible, but just in case,
		// add it so that it will affect an useless possition
		char *last_expression = nullptr;
		branch_t<T> branches {initial_position};
		branch_t<T> or_br {};

		bool placed_literal = false;
		bool had_captures = false;

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
					had_captures = true;

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
					if (placed_literal)
					{
						printf("Detected warp\n");

						// Okay, here I'm going
						// to repeat myself, and in fact
						// the if inside the for is not even
						// necessary. However, this is just to
						// be future-proof, and it is not critical
						this->new_start_group(branches);
						had_captures = true;

						for(T node : branches){
							if(this->data[node + WARP_CAPTURES])
								[[likely]]
								reinterpret_cast<capture_t<T>*>(
									this->data[node + WARP_CAPTURES])->push_front(0);
							else
								this->data[node + WARP_CAPTURES] = reinterpret_cast<T>(new capture_t<T>{0});
						
							printf("WARP PTR %d: %d\n", node, this->data[node + WARP_CAPTURES]);
						}

						//branches = {node_length};
					}
					else{
						// No need for now, since it just ensures it concatenates
						/*
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
						*/
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
					had_captures = true;

					
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
			for (; open_warp; --open_warp)
				this->new_end_group(branches);


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

		if(branches.size() > 1){
			#if FRB_VERBOSE
			printf("Detected multiple final %u branches\n  ", branches.size());
			#endif

			for(T node : branches){
				this->data[node+FINAL] = this->added_id;

				#if FRB_VERBOSE
				printf("%u,",node); 
				#endif
			}
		} else{
			#if FRB_VERBOSE
			printf("Final node: %d\n", *branches.begin());
			#endif
			this->data[(*branches.begin())+FINAL] = this->added_id;
		}
		
testing_do_not_generate:


		#if FRB_VERBOSE
		printf("\nAdded new expression with id:%u\n%s###\n\n", this->added_id,
						had_captures ? "  which had captures\n" : "");
		#endif

		if(update_id){
			++this->data[NUM_ADDED];
			++this->added_id;
		}
		if(!this->contains_captures.count(this->added_id) || had_captures)
			this->contains_captures[this->added_id] = had_captures;

		this->data[SIZE] = this->data.size();

		return branches;
	} 

	std::unordered_map<T, void*> * append_state(T state){
		if(this->tmp_states.count(state)){
			return static_cast<std::unordered_map<T, void*>*>(this->tmp_states[state]);
		}
		else{
			std::unordered_map<T, void *> * new_state = new std::unordered_map<T, void *>();

			this->tmp_states[state] = static_cast<void *>(new_state);

			return new_state;
		}
	}

	std::unordered_map<T, void*> * append_state(std::unordered_map<T, void*> * node, T state){
		if(node->count(state)){
			return static_cast<std::unordered_map<T, void*>*>(node->at(state));
		}
		else{
			std::unordered_map<T, void *> * new_state = new std::unordered_map<T, void *>();

			node->at(state) = static_cast<void *>(new_state);

			return new_state;
		}
	}

	inline void new_append_letter(branch_t<T> &br, const char letter, const T to=0){
		// If there is an explicit node we should route nodes to,
		// define "merge" to it.
		T merge = 0;
		branch_t<T> new_branches {};

		std::unordered_map<T, T> same_node_counter = std::unordered_map<T, T>();
		// Count for each branch the amount of same nodes
		for(T node : br)
			if(same_node_counter.count(node))
				same_node_counter[node]++;
			else
				same_node_counter[node] = 1;

		

		for(T node : br){
			[[likely]];
			//printf("%d, %d, %d, %d\n", node, node + letter + char_offset, this->data.size(), this->data[node + letter + char_offset]);

			// If the letter in node does not route to
			// any other node (aka is empty):
			if(!this->data[static_cast<T>(node+letter+char_offset)]){
				// If "to" was 0, and exists any letter that is
				// empty, generate a new node to merge any other
				// empty letters to. If all were occupied,
				// there is no need to generate merge.
				if(!merge)
					new_branches.insert(merge = (to ? to : this->new_node()));

				#if FRB_VERBOSE
				printf("  %u - %c -> %d  (new)\n", node, letter, merge);
				#endif
				this->add(node, letter, merge);
			}
			// If the letter is actually occupied:
			else {
				T occupied_node = this->data[node+letter+char_offset];

				/*
				if(this->data[occupied_node+LINKS] > 1
							&& same_node_counter[occupied_node] == 1){
					// Use this version of copy to make sure the new node
					// is set in the place of the previous
					occupied_node = this->copy(node, letter);

					#if FRB_VERBOSE
					printf("  %u - %c -> %d  (copied)\n", node, letter, occupied_node);
					#endif
				}
				*/
				#if FRB_VERBOSE
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
				this->new_start_group(node);
			else
				this->new_end_group(node);

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

		if(this->data[copied+LINKS])
			--this->data[copied+LINKS];

		return new_arr;
	}
  
	inline void new_start_group(const branch_t<T> &br){
		#if FRB_VERBOSE
		printf("Opening %d captures\n", br.size()); fflush(stdout);
		#endif

		for(T node : br){
			this->new_start_group(node);
		}
	}

	inline void new_start_group(const T node){
		printf("%u", node); fflush(stdout);
		if(! this->data[node+WARP_CAPTURES])
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

	inline void new_end_group(const branch_t<T> &br){
		#if FRB_VERBOSE
		printf("Closing %d captures\n", br.size());
		#endif

		for(T node : br){
			this->new_end_group(node);
		}
	}

	inline void new_end_group(T node){
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

	void setup_states(){
		if(this->tmp_states.size() == 0)
			return;
		size_t n_states = this->tmp_states.size();
		this->states.insert(this->data.end(), n_states, 0);

		// We add all states
		std::stack<std::unordered_map<T, void *> *> search{{
			static_cast<std::unordered_map<T, void *> *>(this->tmp_states[0])
		}};
		std::stack<uint_fast16_t> seen {{0}};

		printf("Got :\n");
		T tmp_state = 0;
		T current_state = 0;
		while (!search.empty())
		{
			printf("%d remaining - ", search.size());
			fflush(stdout);
			if (search.top()->begin() != search.top()->end())
			{
				printf("useful\n");
				auto tmp_it = search.top()->begin();
				for (int i = 0; i != seen.top(); ++i, tmp_it++)
					;
				auto it = *(tmp_it);
				++seen.top();
				tmp_state = it.first;

				// Should be an if, but since time is not a problem, just in case
				// Actually I think this will never go in
				while(current_state + tmp_state > this->states.size() - 1)
					[[unlikely]]
					this->states.insert(this->data.end(), n_states, 0);

				// if exists
				if(! this->states[current_state + tmp_state]){
					this->states[current_state + tmp_state] = this->states.size();
					this->states.insert(this->data.end(), n_states, 0);
				}

				current_state = this->states[current_state + tmp_state];

				seen.emplace(0);
				search.emplace(static_cast<std::unordered_map<T, void *> *>(it.second));

				if (search.top()->size() >= seen.top())
					search.pop();
			}
			else
			{
				printf("empty\n");
				search.pop();
			}
		}
	}
};

#endif