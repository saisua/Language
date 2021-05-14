#include <array>
#include <string>
#include <memory>
#include <unordered_map>
#include <stack>
#include <ctype.h>
#include <stdexcept>

#define verbose 1

#define ID 0
#define FINAL 1


#define first_char  2
#define last_char 127
#define control_positions 2

#define char_offset control_positions - first_char

#define char_length last_char - first_char + control_positions

class Mreg{
	private:
		uint* expr_id;
		uint* added_id;

		std::unique_ptr<uint> add_ptr;




	public:
		// In a future, they will be private
		std::array<uint, char_length>* count = new std::array<uint, char_length>();
		std::array<uint, char_length>* storage = new std::array<uint, char_length>();
		
  
  //uint expr;
//  uint added;
 
		std::array<void*,char_length> data;
	
		uint length;
		uint id;
  
  
	Mreg(uint* expr_id=nullptr, uint* added_id=nullptr) {
		#if VERBOSE
			printf("0const\n");
		#endif
		this->data = std::array<void*, char_length>();
		this->length = 0;
		
		#if VERBOSE
			printf("1const\n");
		#endif
		
		this->data.fill(nullptr);
		
		//if(expr_id)
		this->expr_id = expr_id;
		//  else{
		//   this->expr = 1;
		//   this->expr_id = &this->expr;
		//  }
		#if VERBOSE
			printf("2const\n");
		#endif
		//  if(added_id)
		this->added_id = added_id;
		//  else{
		//   this->added = 1;
		//   this->added_id = &this->added;
		//  }  
		#if VERBOSE
			printf("3const\n");
		#endif
		
		this->id = ++(*expr_id);
		this->data[ID] = &this->id;
		#if VERBOSE
			printf("4const\n");
		#endif
	}
  
  	inline void append(const char* expression){
		// Setup of vars
		// Starting regex_object
		Mreg* reg_obj = this;
		// Clear any counter of chars
		this->count.fill(0);
		// and clear any stored counter
		this->store.fill(0);

		// Any char after a backslash
		bool backslash = false;

		Mreg* lett;
		std::stack<Mreg*> depth;

		while(*(++expression)){
			switch ((*expression)+char_offset){
				
				// char '('
				case 40:
				// char ')'
				case 41:
				// char '[' to ']'
				case 91:
					if(! backslash){
						charset = true;
						this->append_process_sq();
						continue;
					}
				// char ']' has no special behaviour
				// char '{' to '}'
				case 123:
					if(! backslash){
						bubbled = true;
						this->append_process_bbl();
						continue;
					}
				// char '}'  has no special behaviour


				// char '*'
				case 42:
				// char '+'
				case 43:
				// char '?'

				// char '\'
				case 47:
					backslash = !backslash;

					// if backslash was false, it means
					// next char is special, so no need
					// to add it, get next char
					if(backslash) 
						continue;
				// char '|'
				case 124:


				default:		
					#if VERBOSE
						printf("%u -> ", reg_obj->id);
					#endif
					lett->append(++expression);

					if(backslash && isalpha(*expression))
						reg_obj = this->append_backslash(reg_obj, *expression);
					else{
						reg_obj = this->append_letter(reg_obj, *expression)
					}
					#if VERBOSE
						printf("%u\n", reg_obj->id);
					#endif

					backslash = false;
			}
		}	
		
		// End of string
		this->add_ptr = std::unique_ptr<uint>{new uint()};
		*add_ptr = ++(*this->added_id);
		
		uint* added = add_ptr.get();

		this->data[FINAL] = added;
		#if VERBOSE
			printf("Added new expression {%u}\n", added);
		#endif
	} 

	// Append inline sub-functions
		inline Mreg* append_letter(Mreg* reg_obj, const char expr){
			this->count[expr]++;

			Mreg* lett = (Mreg*) reg_obj->data[expr];

			// Si ya existe dicha letra, avanza con el
			// objeto al siguiente array.
			if(lett){
				return lett;
			}
			else {
				return reg_obj->generate(expr);
			}
		}

		inline Mreg* append_letters(Mreg* reg_obj, const std::string expr){

		}

		inline Mreg* append_backslash(Mreg* reg_obj, const char expr){

		}

		inline Mreg* append_process_sq(Mreg* reg_obj, const char** expr){

		}

		inline void append_process_bbl(Mreg* reg_obj, const char** expr){
			bool both = false;
			uint min = 0, max = 0;

			std::string buffer = "";
			
			while(*(expr++) != '}'){
				if(*expr != ',')
					if(isdigit(*expr))
						buffer += *expr;
					else
						throw std::invalid_argument("wrong values passed between squared brackets {}");
				else{
					min = std::stoi(buffer);
					both = true;
					buffer.erase();
				}
			}

			max = std::stoi(buffer);
		}
  
  	inline Mreg* generate(const char pos){
		Mreg* new_arr = new Mreg(this->expr_id, this->added_id);
		#if VERBOSE
			printf("Generated array in %u[%c %i] {%u}\n", this->id, pos, pos, new_arr->id);
		#endif
		
		this->length++;
		
		this->data[pos] = new_arr;
		return new_arr;
	}

	inline Mreg* copy(const char pos){
		Mreg* copied = this->data[pos];
		Mreg* new_arr = new Mreg(this->expr_id, this->added_id);
		#if VERBOSE
			printf("Copied array in %u[%c %i] {%u from %u}\n", this->id, pos, pos, new_arr->id, copied->id);
		#endif

		for(uint pos_val = first_char, pos_val < last_char; pos_val++){
			new_arr->data[pos_val] = copied->data[pos_val];
		}

		return new_arr;
	}
  
	void clean(){
		// Breath, hash and remove 
		std::hash<std::string> a;
		return;
	}
  
	bool match(const char * str, uint* ret){
		#if VERBOSE
			printf("mat0\n");
		#endif
		Mreg* mreg = this;
		
		#if VERBOSE
			printf("mat1\n");
		#endif
		while(*str && mreg->data[*str]){
			#if VERBOSE
				printf("mat %c %i\n %u ->", *str, *str, mreg->id);
			#endif
			mreg = (Mreg*) mreg->data[*str];
			#if VERBOSE
				printf("%u\n", mreg->id);
			#endif
			str++;
		}
		#if VERBOSE
			printf("mat2 - %i", !*str, mreg);
		#endif
		
		if(!*str){
			uint a = 1;
			#if VERBOSE
				printf(" - 0x%x\n", mreg->data[FINAL]);
			#endif
			
			if(mreg->data[FINAL]){
				*ret = *(uint*) mreg->data[FINAL];
				return true;
			}
		}
		#if VERBOSE
			printf("\n");
		#endif
		return false;
	}
};




int main(int argc, char *argv[])
{
	std::string a = "hola";
	std::string b = "hombre";
	std::string c = "adios";
	std::string m[] = {"hombro","adios","hombree", ""};
	
	uint ua = 1;
	uint ub = 1;
	
	#if VERBOSE
		printf("#### Start ####\n");
	#endif
	
	Mreg r = Mreg(&ua, &ub);
	r.append(a.c_str());
	r.append(b.c_str());
	r.append(c.c_str());
	
	uint* ma = new uint(0); 
	
	bool found;
	
	for(std::string check : m){
		found = r.match(check.c_str(), ma);
		
		#if VERBOSE
			printf("%i\n", r.length);
		#endif
		if(found)
			printf("Match of %s = %u\n",check.c_str(),*ma);
		else
			printf("Not match of %s\n", check.c_str());
	}
}