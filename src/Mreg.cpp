// Include guard:
#ifndef FB_R_TREE
#define FB_R_TREE

#include <array>
#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <stack>
#include <ctype.h>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <utility>
#include <iterator>
#include <cmath>
#include <chrono>
#include <stdint.h>
#include <iostream>
#include <list>
#include <queue>
#include <fstream>
#include <sys/resource.h>
#include <cstring>
#include "utils/cllist.cpp"
#include <regex>
#include "full_regex.cpp"
#include <optional>
#include <limits>
#include <csignal>
#if FBR_USE_CONSTEXPR_DATA
#include "frb_data.h"
#endif

using regex=Fregex;

#ifndef FRB_VERBOSE
	#define FRB_VERBOSE false
#endif
#ifndef FRB_VERBOSE_ANALYSIS
	#define FRB_VERBOSE_ANALYSIS (FRB_VERBOSE && false)
#endif
#ifndef FRB_CLEAN
	#define FRB_CLEAN false
#endif
#ifndef FRB_PROFILE
	#define FRB_PROFILE false
#endif

#ifndef FRB_GENERATE
	#define FRB_GENERATE false
#endif
#ifndef FRB_STORE
	#define FRB_STORE true
#endif
#ifndef FRB_MATCH
	#define FRB_MATCH true
#endif
// Only match and do not process groups
#ifndef PLAIN_FRB_MATCH
	#define PLAIN_FRB_MATCH false
#endif


// First char is not 7, but this way
// we can save on some cycles,
// since we don't have to "remap"
// chars. First char is actually 32
// Defined both, so the first_char
// is for cache optimizations
#define first_char 32u
// Must never be 0
#define start_match_char  5u
#define last_char 127u
#define control_positions 5u
// Offset positions subtracts to the node first position
// so that when accessed we can just shave of
// some control positions unused for free
#define offset_positions (first_char - control_positions)
#define reserved_data_size 2^15

#define char_offset (control_positions - start_match_char)

#define node_length (last_char - offset_positions)



// The order is thought to improve on cache.
// Only useful flags during compilation-time
// must be placed at the end, so that cache
// starts there and more used nodes are
// in the cache.

// The id of the added match string, to not 
// mistake groups that are from previous compilations
#define GROUP_ID (offset_positions + 0)
// A pointer to a vector<uint> of nodes, from which 
// we were meant to merge, when we advance one char
#define GROUP (offset_positions + 1)
// The amount of nodes pointing to this node
#define LINKS (offset_positions + 2)
// The final id of the match. (offset_positions + 0) if the node is not final
#define FINAL (offset_positions + 3)
// A pointer to a list of captures to keep track what
// sub-strings to get and what not
// Also contains a flag, which will return 
// to the start of the tree
// when detected, and when it ends, returns
// to the node 
#define WARP_CAPTURES (offset_positions + 4)
#define SEMIWARP_MASK (std::numeric_limits<T>::max()>>1)
#define WARP_MASK (std::numeric_limits<T>::max()>>2 | ~SEMIWARP_MASK)

// Nodes in data [0 : node_length] used to aid
// in performance with pre-calculated statistics
//
// The number of added_id in the tree
#define NUM_ADDED 1
// The maximum length of a regex expression
#define MAX_REG_LENGTH 2
// The total length of the vector being used
#define SIZE 3

// The number to look for when looking for
// capture nodes
#define CAPTURE_NODE -1u

#define group_t std::unordered_set
#define capture_t std::list
#define branch_t std::unordered_set

// Regex to string translation
#define reg_d std::string("0123456789")
#define reg_w (reg_d+std::string("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"))
#define reg_n '\n'
#define reg_t '\t'
#define reg_r '\r'
#define reg_z '\0'
#define reg_s std::string({' ',reg_n,reg_t})
#define reg_dot (reg_w+reg_s+std::string("!@#$%^&*()_+{}[]'\\|\"/-=>?<ºª`~"))

#define ANALIZE_SEPARATE (first_char-1)
// Analizing the regex,
// the char* meaning of every regex
// instruction. Must be up to 32 max.
#define an_not_sq_brack 1
#define an_sq_brack 2
#define an_bb_brack_m_n 3
#define an_bb_brack__n 4
#define an_bb_brack_m_ 5
#define an_bb_brack_m 6
#define an_asterisk_question 7
#define an_plus_question 8
#define an_asterisk 9
#define an_plus 10
#define an_question 11
#define an_backw_slash 12
#define an_dot 13
#define an_start_paren 14
#define an_end_paren 15
#define an_or 16
#define an_not_capture 17
#define an_positive_lookahead 18
#define an_negative_lookahead 19
#define an_positive_lookbehind 20
#define an_negative_lookbehind 21
#define an_warp 22


constexpr inline bool fnull(uint p){ return p != 0; }
inline bool is_regex(const char* expr){
	return *(expr-1) == '\\' && !(*(expr-1) == '\\' && ! isalpha(*expr));
}


template <typename T>
class Mreg{
	// Not all should be public, but I have yet to look up
	// how to make some variables only accessable from child
	public:
	// Take note of which pointers have been deleted
	std::unordered_set<T> deleted;
	std::list<capture_t<T>*> unknown_ptrs;
	
	std::vector<bool> contains_captures;
	std::vector<T> nodes_captures;
	std::vector<const char*> str_captures;
	std::vector<const char*> str_starts;
	std::stack<T, std::vector<T>> warps;
	std::stack<const char*, std::vector<const char*>> str_warps;

	std::unordered_set<T> nodes_data_captures;

	std::queue<uint8_t> capture = std::queue<uint8_t>();
	// Also, I could turn WARP_CAPTURES to int and add the ids only.
	// capture end would be in last capture node + 1

	T max_str_size = 0;

	static constexpr T warp_mask = 1;
	static constexpr T captures_shift = 1;

	T added_id=1;

	// This will take the form of void* in positions 0..control_positions
	// and of int in control_positions+1..node_length
	#ifdef FRB_CONSTEXPR_DATA
	static constexpr std::array<T, FRB_CONSTEXPR_DATA_LEN> data{FRB_CONSTEXPR_DATA};
	#else
		std::vector<T> data;
	#endif
	std::vector<T> states;
	C_linked_list<uintptr_t> result_subgr;

	std::vector<uint_fast8_t> final_sizes;

	Mreg()
	{
#ifndef FRB_CONSTEXPR_DATA
		this->data = std::vector<T>(node_length * 2, 0);
		this->data.reserve(reserved_data_size);
#endif
		this->states = std::vector<T>();

		this->contains_captures = std::vector<bool>();
		this->nodes_captures = std::vector<T>();
		this->str_captures = std::vector<const char *>();
		this->str_starts = std::vector<const char *>();
		this->warps = std::stack<T, std::vector<T>>();
		this->str_warps = std::stack<const char *, std::vector<const char *>>();

		this->nodes_data_captures = std::unordered_set<T>();

		this->contains_captures.push_back(false);

		this->result_subgr = C_linked_list<uintptr_t>();
		this->result_subgr.reserve();
	}

	~Mreg(){
		#ifndef FRB_CONSTEXPR_DATA
		this->delete_pointers();
		#endif

		// TODO: Find where any of these are deleted 
		if(false && this->str_captures.size()){
			for(const char* scap: this->str_captures)
				delete[] scap;
		}
		if(this->str_starts.size()){
			printf("str_starts\n"); fflush(stdout);
			for(const char* ssta: this->str_starts)
				delete[] ssta;
		}
	}

	#ifndef FRB_CONSTEXPR_DATA
	void delete_pointers(bool all = false){
		#if !FRB_GENERATE
		this->deleted = std::unordered_set<T> {};
		#endif

		const size_t max_size = this->data.size();
	
		#if FRB_VERBOSE
		printf("\n\n### DELETING\n\t");
		
		constexpr uint max = 5;
		uint count = 0;
		#endif

		for(uint node = node_length; node != max_size; node += node_length){
			if(this->nodes_data_captures.count(node))
				continue;

			#if FRB_VERBOSE
			printf(" %u", node);
			#endif

			if(this->data[node+GROUP] && static_cast<size_t>(this->data[node+GROUP]) > this->data.size()){
				#if FRB_VERBOSE
				printf("-G", this->data[node+GROUP]);
				#endif
				if(! deleted.count(this->data[node+GROUP])){
					#if FRB_VERBOSE
					printf("#");
					
					#endif

					deleted.insert(this->data[node+GROUP]);

					delete reinterpret_cast<group_t<uint>*>(this->data[node+GROUP]);
				}
				this->data[node+GROUP] = 0;
			}
			
			#if !PLAIN_FRB_MATCH
			if(all)
			#endif
			if(this->data[node+WARP_CAPTURES]){
				#if FRB_VERBOSE
				printf("-C");
				#endif
				if(! deleted.count(this->data[node+WARP_CAPTURES])){
					#if FRB_VERBOSE
					printf("#");
					#endif
					deleted.insert(this->data[node+WARP_CAPTURES]);

					delete reinterpret_cast<capture_t<T>*>(this->data[node+WARP_CAPTURES]);
				}
				// Used in match to detect nodes with possible
				// captures
				#if PLAIN_FRB_MATCH
				this->data[node+WARP_CAPTURES] = 0;
				#endif
			}

			#if FRB_VERBOSE
			if(count != max)
				++count;
			else{
				count = 0;
				printf("\r%c[2K\r\t", 27);
			}
			#endif
		}

		;
		// For all pointers in this->unknown_ptrs, check if they are
		// in deleted. If they are not in deleted,
		// delete the pointer.
		for(typename std::list<capture_t<T>*>::iterator 
					it = this->unknown_ptrs.begin(); 
					it != this->unknown_ptrs.end(); ++it){
			T ptr = reinterpret_cast<T>(*it);
			if(deleted.count(ptr))
				continue;
			
			delete *it;

			deleted.insert(ptr);
		}
		this->unknown_ptrs.clear();

		#if FRB_VERBOSE
		printf("\n###\n\n");
		#endif
	}
	#endif

	std::string str(const uint node=node_length){
		std::string result = "[";

		#if FRB_VERBOSE
		printf("Seen ");
		bool seen = false;
		#endif
		for(uint lett = control_positions+offset_positions; lett < node_length; ++lett){
			if(this->data[node+lett]){
				#if FRB_VERBOSE
				printf("%c ", lett-char_offset);
				seen = true;
				#endif
				result.append(std::string({static_cast<char>(lett-char_offset), ',', ' '}));
			}
		}
		#if FRB_VERBOSE
		if(! seen)
			printf("nothing ");

		printf("in str()\n");
		#endif

		result = result.substr(0, result.length()-2);
		result.append(std::string("]"));
		return result;
	}
	
	const char* c_str(const uint node=node_length){
		return this->str(node).c_str();
	}

	// Generates a new node at the end of data and 
	// returns it.
	inline T new_node(){
		#if FRB_VERBOSE
		printf("   Generated new node: %u at %u[%u]\n", this->data.size() - offset_positions,
		 									this->data.size(), node_length);
		#endif

		T new_node = this->data.size() - offset_positions;
		this->data.insert(this->data.end(), node_length, 0);

		return new_node;
	}

	void clean(){
		// Breath, hash and remove 
		printf("\n\n### CLEANING\n");

		// This must be done properly
		// to ensure no capture vector
		// is placed in the wrong node
		this->_clear_capture_vectors();
	}
	
	void _clear_capture_vectors(){
		std::unordered_map<T, uint> capture_data_start = std::unordered_map<T, uint>();
		std::unordered_map<uint, uint> capture_data_space = std::unordered_map<uint, uint>();
		const size_t max_size = this->data.size()-offset_positions;
	
		uint capture_cell;

		for(uint node = node_length-offset_positions; node != max_size; node += node_length){
			// If the node is a reallocated capture node, no need 
			// to iterate over it

			printf("%d %d", node, this->data[node+WARP_CAPTURES]);
			if (this->data[node] == CAPTURE_NODE){
				printf("c\n");
				continue;
			}

			printf("-\n");

			// If the node has a capture vector
			// and it is a pointer (gt data.size())
			if(this->data[node+WARP_CAPTURES]){
				// If it is a pointer, it must be
				// greater than data.size()
				if (this->data[node+WARP_CAPTURES] >> this->captures_shift > this->data.size())
				{
					// If we have not deleted this ptr yet
					if(! deleted.count(this->data[node+WARP_CAPTURES])){

						capture_t<T>* captures = reinterpret_cast<capture_t<T>*>(this->data[node+WARP_CAPTURES]);

						//printf("D TEST %d < %d\n", this->data[node+WARP_CAPTURES], static_cast<T>(-this->added_id)); fflush(stdout);

						uint prev_size = captures->size();
						captures->remove(0);
						bool has_warp = captures->size() != prev_size;

						captures->sort();
						captures->reverse();

						deleted.insert(this->data[node+WARP_CAPTURES]);

						std::unordered_map<uint, uint>::iterator spaces_iter = capture_data_space.begin();

						for(; spaces_iter != capture_data_space.end(); ++spaces_iter){
							// Space_left + captures.size + 1 <= node_length
							if(spaces_iter->second+captures->size() < node_length)
								break;
						} 


						#if FRB_VERBOSE
						printf("\tNew ptr 0x%X in node %u\n", this->data[node+WARP_CAPTURES], node);
						#endif

						//printf("D2 TEST\n"); fflush(stdout);

						if(spaces_iter == capture_data_space.end()){
							capture_cell = this->new_node();
							this->nodes_data_captures.insert(capture_cell);

							// Take note of the size of the vector
							// we are writing down.
							capture_data_space[capture_cell] = captures->size()+2;

							// Mark as a node that holds only captures
							// Start in cell 1 (zero ocupied)
							this->data[capture_cell] = CAPTURE_NODE;
							++capture_cell;
						} else {
							capture_cell = spaces_iter->first + spaces_iter->second;
							spaces_iter->second += captures->size()+1;
						}

						#if FRB_VERBOSE
						if(has_warp)
							printf("\t\t Contains warp\n");
						printf("\t\t Contains %zu capture nodes\n\t  [", capture_cell, captures->size());
						#endif
						
						capture_data_start[this->data[node+WARP_CAPTURES]] = capture_cell << this->captures_shift;
						this->data[capture_cell] = captures->size();

						// Take note the new direction assigned
						this->data[node+WARP_CAPTURES] = capture_cell << this->captures_shift;
						if(has_warp){
							this->data[node + WARP_CAPTURES] |= this->warp_mask;
							capture_data_start[this->data[node + WARP_CAPTURES]] |= this->warp_mask;
						}

						for(long int captured_node : *captures){
							++capture_cell;
							this->data[capture_cell] = captured_node;
							#if FRB_VERBOSE
							printf("  %d[%u]", captured_node, capture_cell);
							#endif
						}

						#if FRB_VERBOSE
						printf(" ]\n");
						#endif

						delete captures;
					}
					// If we deleted it, access to the capture cell in the umap
					else {
						//printf("TEST\n"); fflush(stdout);

						typename
						std::unordered_map<T, uint>::const_iterator seen_cell = 
								capture_data_start.find(this->data[node+WARP_CAPTURES]);

						if(seen_cell != capture_data_start.cend()){
							this->data[node+WARP_CAPTURES] = seen_cell->second;

							#if FRB_VERBOSE
							printf("\tSeen ptr 0x%X in node %u\n\t Redirected to %u\n", 
										this->data[node+WARP_CAPTURES], node, seen_cell->second);
							#endif
						} else {
							printf("[!]\n[!] ERROR: Missing ptr in %u (0x%x)\n[!]\n\n\n", node, this->data[node+WARP_CAPTURES]);
							this->data[node+WARP_CAPTURES] = 0;
						}
					}
				}
			}
		}
		
		printf("### Size %zu -> %zu\n\n", max_size, this->data.size());
	}

	void _move_captures(){
		T first_valid = this->data.size();
		const T first_invalid = (this->warp_mask & this->semiwarp_mask) - node_length;

		#if FRB_VERBOSE
		printf("first invalid node : %d\n", first_invalid);
		#endif

		while(first_valid >= first_invalid)
			first_valid -= node_length;

		#if FRB_VERBOSE
		printf("first valid node : %zu\n\n", first_valid);
		#endif

		std::vector<T> cap_to_move {};

		// Copy invalid captures nodes to cap_to_move,
		// and lower the first valid, since there
		// needs to fit the invalid nodes in a valid zone
		for(T node : this->nodes_captures)
			if(node >= first_invalid){
				#if FRB_VERBOSE
				printf("  [-] Detected invalid node %zu\n", node);
				#endif
				cap_to_move.push_back(node);

				first_valid -= node_length;
			}

		// First valid is unchanged because every time we
		// move a node, the previous is pushed forward
		// (can't be pushed backwards, since)
		// these nodes will always be in an invalid
		// position, which is greater than the first valid
		for(T node : cap_to_move)
			this->move_node(node, first_valid);
	}

	// This function moves a given node to a position,
	// displacing all other nodes from behind one position.
	// This is useful to optimize cache loading
	// and it is necessary to join the warp bit to
	// the captures position, since T has a specific
	// length (in bits) to optimize it, to being
	// able to optimize it, I need to check no 
	// captures node number uses all bits, so that
	// the last one can become the warp flag.
	// Since captures and warp is checked every
	// char in match(), not accessing the array
	// again should free some clock cycles.
	void move_node(T node_from, T node_to){
		#if FRB_VERBOSE
		printf("Moving node %d -> %d\n", node_from, node_to);
		#endif
		T* aux = new T[node_length];

		__builtin_prefetch(&this->data[node_from]);

		// copy node_from to aux
		T positions = node_length+1;
		while(--positions)
			*(aux + positions) = this->data[node_from+positions];
		*aux = this->data[node_from];

		// copy (node_from + n <- node_from + n+1)
		if(node_from < node_to){
			// Since we start moving data to node_from, 
			// to_iter points to node_from.
			T* to_iter = this->data.data() + node_from;
			T* from_iter = to_iter + node_length;
			T* goal = this->data.data() + node_to + node_length;

			while(from_iter != goal)
				*(to_iter++) = *(from_iter++);

		// node_from > node_to
		// copy (node_from - n-1 -> node_from - n)
		} else {
			T* from_iter = this->data.data() + node_from - 1;
			// Since we start moving data to node_from, 
			// to_iter points to node_from.
			T* to_iter = from_iter + node_length;
			
			T* goal = this->data.data() + node_to;

			while(from_iter != goal)
				*(to_iter--) = *(from_iter--);
		}

		// Copy back node_from (in aux) to node_to
		positions = node_length+1;
		while(--positions)
			this->data[node_to+positions] = *(aux + positions);
		this->data[node_to] = *aux;

		delete[] aux;
	}

	inline std::ostream& store(std::ostream& out_stream){
		#if FRB_VERBOSE
		printf("Serializing all %zu data pos... \n", this->data.size());
		#endif
		
		uintptr_t data_size = this->data.size();

		out_stream.write(reinterpret_cast<char*>(&data_size), sizeof(data_size));
		out_stream.write(reinterpret_cast<const char*>(this->data.data()), data_size*sizeof(T));

		#if !FRB_CLEAN || FRB_VERBOSE
		printf("[#] Data serialized.\n\n");
		#endif

		return out_stream;
	}

	inline std::istream& load(std::istream& in_stream){
		#if FRB_VERBOSE
		printf("Loading all ");
		#endif
		uintptr_t size = 0;
		in_stream.read(reinterpret_cast<char *>(&size), sizeof(size));
		
		#if FRB_VERBOSE
		printf("%zu data pos...\n ", size);
		#endif

		this->data.resize(size);
		{
		std::vector<uintptr_t> tmp_data(size);

		in_stream.read(reinterpret_cast<char *>(tmp_data.data()), tmp_data.size() * sizeof(uintptr_t));
		
		for(uintptr_t tmp : tmp_data)
			this->data.push_back(static_cast<T>(tmp));
		}

		#if !FRB_CLEAN || FRB_VERBOSE
		printf("[#] Data deserialized.\n\n");
		#endif

		this->nodes_captures.reserve(this->data[MAX_REG_LENGTH]);
		this->str_captures.reserve(this->data[MAX_REG_LENGTH]);
		this->str_starts.reserve(this->data[MAX_REG_LENGTH]);
		this->added_id = this->data[NUM_ADDED];

		this->restore_captures();

		return in_stream;
	}

	inline std::ostream& store_states(std::ostream& out_stream){
		#if FRB_VERBOSE
		printf("Serializing all %zu states... \n", this->states.size());
		#endif
		
		uintptr_t states_size = this->states.size();

		out_stream.write(reinterpret_cast<char*>(&states_size), sizeof(states_size));
		out_stream.write(reinterpret_cast<const char*>(this->states.data()), states_size*sizeof(T));

		#if !FRB_CLEAN || FRB_VERBOSE
		printf("[#] States serialized.\n\n");
		#endif

		return out_stream;
	}

	inline std::istream& load_states(std::istream& in_stream){
		#if FRB_VERBOSE
		printf("Loading all ");
		#endif
		uintptr_t size = 0;
		in_stream.read(reinterpret_cast<char *>(&size), sizeof(size));
		
		#if FRB_VERBOSE
		printf("%zu states...\n ", size);
		#endif

		this->states.resize(size);
		in_stream.read(reinterpret_cast<char *>(this->states.data()), this->states.size() * sizeof(T));
		//this->data.resize(this->data.size());

		#if !FRB_CLEAN || FRB_VERBOSE
		printf("[#] States deserialized.\n\n");
		#endif

		return in_stream;
	}

	static inline T load_position(std::istream& in_stream, uint pos){
		#if FRB_VERBOSE
		printf("Loading position %i of data\n", pos);
		#endif

		std::vector<T> positions = std::vector<T>();

		in_stream.read(reinterpret_cast<char *>(positions.data()), pos * sizeof(uintptr_t));

		return positions.back();
	}

	inline void generate_constexpr(std::ostream & out_stream){
		#if FRB_VERBOSE
		printf("Generating constexpr...\n");
		#endif

		std::string file = "#ifndef FRB_CONSTEXPR_DATA_H\n#define FRB_CONSTEXPR_DATA_H\n\n"
						   "#define FRB_CONSTEXPR_DATA_LEN " +
						   std::to_string(this->data.size()) + "\n\n";

		file += "#define FRB_CONSTEXPR_DATA \\\n";

		#define values_nl 5
		for(uintptr_t i = 0; i != this->data.size(); ++i){
			file += std::to_string(this->data[i]) + ", ";
			if(i % values_nl == values_nl - 1)
				file += "\\\n";
		}

		file += "\n\n#endif\n";

		out_stream << file;
	}

	template<typename out_t>
	inline void generate_constexpr(std::ostream & out_stream){
		#if FRB_VERBOSE
		printf("Generating constexpr...\n");
		#endif

		std::string file = "#ifndef FRB_CONSTEXPR_DATA_H\n#define FRB_CONSTEXPR_DATA_H\n\n"
						   "#define FRB_CONSTEXPR_DATA_LEN " +
						   std::to_string(this->data.size()) + "\n\n";

		file += "#define FRB_CONSTEXPR_DATA \\\n";

		file += std::to_string(static_cast<out_t>(this->data[0]));

		#define values_nl 5
		for(uintptr_t i = 1; i != this->data.size(); ++i){
			file += ", " + std::to_string(static_cast<out_t>(this->data[i]));
			if(i % values_nl == values_nl - 1)
				file += "\\\n";
		}

		file += "\n\n#endif\n";

		out_stream << file;
	}

	void restore_captures(){
		this->contains_captures.insert(
				this->contains_captures.cend(), this->added_id+1, false);
		const size_t max_size = this->data.size();

		#if FRB_VERBOSE
		printf("Restoring captures in %d positions:\n", max_size);

		std::unordered_set<uint> seen = std::unordered_set<uint>();
		#endif

		for(uint node = node_length; node != max_size; node += node_length){
			if (this->data[node] == CAPTURE_NODE)
			{
				#if FRB_VERBOSE
				printf("Node %u is a capture node\n", node);
				#endif

				++node;
				while(this->data[node] != 0){
					//printf("node %u of length: %u\n", node, this->data[node]);
					for(uint captured = this->data[node++]; captured != 0; --captured, ++node){
						//printf("%u %u %u\n", node, this->data[node], captured);
						this->contains_captures[abs(static_cast<int>(this->data[node]))] = true;

						#if FRB_VERBOSE
						int captured_id = abs(static_cast<int>(this->data[node]));
						if(! seen.count(captured_id)){
							printf("Detected captures in id: %u\n", captured_id);

							seen.insert(captured_id);
						}
						#endif
					}
				}

				#if FRB_VERBOSE
				printf("Return to initial node cell from %u -> ", node);
				#endif
				// Return to node % node_length = 0
				node -= node % node_length;
				#if FRB_VERBOSE
				printf("%u\n", node);
				#endif
			}
		}

		#if FRB_VERBOSE
		printf("\n");
		#endif
		#if !FRB_CLEAN || FRB_VERBOSE
		printf("Ids with captures are:\n ");
		
		for(uint capture_id = 1; capture_id != this->contains_captures.size(); ++capture_id)
			if(this->contains_captures[capture_id])
				printf("%u ", capture_id);

		printf("\n\n");
		#endif

		this->data[SIZE] = this->data.size(); 
	}

	template<T node>
	inline T count_sorted(const T value) noexcept
		__attribute__ ((const))
		#if !FRB_PROFILE
		__attribute__ ((always_inline))
		#endif
		__attribute__ ((hot))
	{
		[[ assert : (node < this->data.size()) ]]

		#if FRB_VERBOSE
		printf("CS[%u,%d|%u]:", node, this->data[node], value);
		#endif
		const T end = node + this->data[node] + 1;

		++node;
		if(this->data[node] > value){
			#if FRB_VERBOSE
			printf(" - None\n");
			#endif
			return 0;
		}
		++node;

		T result = 0;
		while(this->data[node] < value){
			if(node == end){
				#if FRB_VERBOSE
				printf(" - None\n");
				#endif

				return 0;
			}

			#if FRB_VERBOSE
			printf(" %u", this->data[node]);
			#endif
			++node;
		}

		while(node != end && this->data[node] == value){
			++result;
			#if FRB_VERBOSE
			printf(" N%u", node);
			#endif
			++node;
		}
			
		#if FRB_VERBOSE
		printf("\n", node);
		#endif
		return result;
	}

	template<T node>
	inline T count_sorted_backw(const T value) noexcept
		__attribute__ ((const))
		#if !FRB_PROFILE
		__attribute__ ((always_inline))
		#endif
		__attribute__ ((hot))
	{
		[[ assert : (node < this->data.size()) ]]

		#if FRB_VERBOSE
		printf("CSB[%u,%d|%u]:", node, this->data[node], value);
		#endif
		const T end = node;
		const T max = this->added_id + 1;
		node += this->data[node];

		#if FRB_VERBOSE
		printf(" %u", this->data[node]);
		
		#endif

		if(this->data[node] < value || this->data[node] > max){
			#if FRB_VERBOSE
			printf(" - None\n");
			#endif
			return 0;
		}

		T result = 0;
		while(this->data[node] < max && this->data[node] > value){
			if(node == end){
				#if FRB_VERBOSE
				printf(" - None\n");
				#endif

				return 0;
			}

			#if FRB_VERBOSE
			printf(" %u", this->data[node]);
			#endif
			--node;
		}

		while(node != end && this->data[node] == value){
			++result;
			#if FRB_VERBOSE
			printf(" N%u", node);
			#endif
			--node;
		}

		#if FRB_VERBOSE
		printf("\n");
		#endif

		return result;
	}

	T match(const char * str) noexcept
		# if !FRB_PROFILE
		__attribute__ ((always_inline))
		__attribute__ ((flatten))
		#endif
		__attribute__ ((hot))
		// Should it have a const attribute?
		
	{
		[[ expects : str != nullptr ]]
		[[ ensures mreg : mreg < this->data.size()]]

		T mreg = node_length;

		this->nodes_captures.clear();
		this->str_captures.clear();

		#if FRB_VERBOSE
		printf("size %d\n", this->data.size());
		printf("match start %c, 0x%x\n", *str, this->data[mreg + *str + char_offset]);

		uint last_id = mreg;

		std::list<char> restart_str = std::list<char>();
		#endif

		// Profiler: this is match()

		// Not necessary, but tell the compiler that data
		// is important and must be cached from that address
		//__builtin_prefetch(&this->data[node_length+WARP_CAPTURES]);

		#if !PLAIN_FRB_MATCH
			// If contains any captures in the initial node
			if (this->data[mreg + WARP_CAPTURES] >> this->captures_shift){
			#if FRB_VERBOSE
			printf("Detected captures in node %u [%X]\n", mreg, this->data[mreg+WARP_CAPTURES]);
			
			restart_str.push_back(*str);
			#endif	
			this->nodes_captures.push_back(this->data[mreg+WARP_CAPTURES] >> this->captures_shift);
			this->str_captures.push_back(str);
		}
		#endif

		do{
			//printf("start %c %d\n", *str, *(str+1));
		while(this->data[mreg+*str+char_offset] && (mreg = this->data[mreg+*str+char_offset])){
			[[likely]];
// This is in case there
// is a warp
in_loop:
			#if FRB_VERBOSE
			printf("  match %c (%i)\n", *str, *str);
			
			printf("   %u -> %u\n", last_id, mreg);
			last_id = mreg;
			#endif
			
			#if !PLAIN_FRB_MATCH
			if(this->data[mreg+WARP_CAPTURES]){
				if(this->data[mreg+WARP_CAPTURES] & this->warp_mask){
					#if FRB_VERBOSE
					printf("[?] Detected warp in node %u\n", mreg);
					#endif

					this->warps.push(mreg);
					this->str_warps.push(str);
					mreg = node_length;

					if(this->data[mreg+WARP_CAPTURES] >> 1){
						#if FRB_VERBOSE
						printf("[?] Detected captures in node %u [%u]\n", mreg, this->data[mreg+WARP_CAPTURES] >> this->captures_shift);
					
						restart_str.push_back(*str);
						#endif
						this->nodes_captures.push_back(this->data[mreg+WARP_CAPTURES] >> this->captures_shift);
						this->str_captures.push_back(str);																										
					}
				} else {
					#if FRB_VERBOSE
					printf("[?] Detected captures in node %u [%u]\n", mreg, this->data[mreg+WARP_CAPTURES] >> this->captures_shift);
				
					restart_str.push_back(*str);
					#endif
					this->nodes_captures.push_back(this->data[mreg+WARP_CAPTURES] >> this->captures_shift);
					this->str_captures.push_back(str);
				}
			}
			#endif

			#define WARP false
			
			#if WARP
			// Warps go back at the start of the tree to look for 
			// sub-groups.
			if(this->data[mreg+WARP_CAPTURES] & this->warp_mask){
				#if FRB_VERBOSE
				printf("[?] Detected warp node %u\n", mreg);
				#endif

				this->warps.push(mreg);
				this->str_warps.push(str);
			}
			#endif

			if(!*++str)
				goto end_loop;
		}
		
		if(! this->warps.empty()){
			#if FRB_VERBOSE
			printf("[?] End of tree in %u\n  Match continued in node ", mreg);
			#endif
			
			if(! this->data[mreg+FINAL])
				str = this->str_warps.top();
			mreg = this->warps.top();

			#if FRB_VERBOSE
			printf("%u\n", mreg);
			#endif

			this->warps.pop();
			this->str_warps.pop();
		} else {
			#if FRB_VERBOSE
			printf("[-] End of tree in %u\n", mreg);
			#endif
			
			return 0; 
		}

		}while(1);

end_loop:
		#if FRB_VERBOSE
		printf("mat2 - id: %u\nEnd: %s\n", this->data[mreg+FINAL], this->c_str(mreg));
		#endif

		return this->data[mreg+FINAL];
	}

	uint match_and_subgroups(const char * str)
		# if !FRB_PROFILE
		//__attribute__ ((always_inline))
		//__attribute__ ((flatten))
		#endif
		//__attribute__ ((hot))
	{ 
		printf("Warning: match_and_subgroups is not implemented\n");
	}

	void test(){
		T mreg = node_length;
		std::string order = std::string();

		printf("order: \n");
		std::cin >> order;
		printf("\n");
		while(! order.empty()){
			const char * o = order.c_str();
			
			while(*o && mreg){
				printf("%d - ", mreg);
				mreg = this->data[mreg + *o];
				printf("%c -> %d\n", *o, mreg);
				if(data[mreg + WARP_CAPTURES])
					printf("   Contains warps or captures: %d\n", data[mreg + WARP_CAPTURES]);
				if(data[mreg + FINAL])
					printf("   Contains final: %d\n", data[mreg + FINAL]);
				++o;
			}
			if(*o){
				printf("Got to end. Resetting node");
				mreg = node_length;
			}

				printf("\norder: ");
				std::cin >> order;
				printf("\n");
		}
		printf("%s\n", this->str().c_str());
	}
	/* 
		Could be done a find/findall algorithm optimization,
		where we copy all prefixes from the tree to the subsequent
		nodes, with no performance nor spatial regression.
	*/
	
};

#endif
