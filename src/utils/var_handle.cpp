#ifndef VAR_HANDLE_LPPL_CPP
#define VAR_HANDLE_LPPL_CPP

#include <vector>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <list>
#include <cmath>

#include "var_hash.cpp"

/*
auto x = "yo";

printf(x);

*/
using fnv1a_t = basic_fnv1a<uint_fast64_t,
                            uint_fast64_t(14695981039346656037),
                            uint_fast64_t(1099511628211)>;
                             

#define stack_id_t std::vector<var_handle>*
#define vec_stack(type) std::stack<type, std::vector<type>>
#define var_map std::unordered_map<uint_fast64_t, vec_stack(var_handle*)>


struct var_handle {
    stack_id_t stack_id;
    uint_fast32_t type_id;
    std::list<uintptr_t> * value;
};

#ifndef CALLABLE
#define CALLABLE  ~(((uint_fast32_t)pow(2, sizeof(uint_fast32_t)*8))>>1)
#define KNOWN (CALLABLE >> 1)
#define DEFINED (CALLABLE >> 2)
#define NAMED (CALLABLE >> 3)
#endif

#define RESERVED_WORDS {\
    "if", "else", "for", "while", \
    "break", "continue", "return", \
    "int", "float", "double", "char", \
    "string", "void", "true", \
    "false", "null", "new", "delete" \
}
#define RESERVED_FUNCTIONS { \
    "printf", "scanf", "getchar", "putchar", \
    "strlen", "strcpy", "strcmp", "strcat", \
    "print", "read" \
}

#define PAR_IZQ_ 1
#define PAR_DER_ 2
#define MAS_ 3
#define MENOS_ 4
#define POR_ 5
#define DIV_ 6
#define MOD_ 7
#define MAS_IGUAL_ 8
#define MENOS_IGUAL_ 9
#define POR_IGUAL_ 10
#define DIV_IGUAL_ 11
#define MOD_IGUAL_ 12
#define MASMAS_ 13
#define MENOSMENOS_ 14
#define LT_ 15
#define GT_ 16
#define LE_ 17
#define GE_ 18
#define EQ_ 19
#define NE_ 20
#define STRUCT_IZQ_ 21
#define STRUCT_DER_ 22
#define STRUCT_ 23
#define ACCESS_IZQ_ 24
#define ACCESS_DER_ 25
#define AND_ 26
#define OR_ 27
#define NOT_ 28
#define BIN_AND_ 29
#define BIN_OR_ 30
#define BIN_XOR_ 31
#define BIN_NOT_ 129
#define SHIFT_IZQ_ 130
#define SHIFT_DER_ 131
#define VAR_ 132
#define ACC_ 133
#define RETURN_ 134
#define SEMICOLON_ 135
#define COMA_ 136
#define DOT_ 137
#define TYPE_ 138
#define CONSTANT_ 139
#define IF_ 140
#define ELSE_ 141
#define WHILE_ 142
#define FOR_ 143
#define ASSIGN_ 144
#define ASSIGN_MODIF_ 145
#define CALL_ 146
#define INT_ 147
#define FLOAT_ 148
#define CHAR_ 149
#define STRING_ 150
#define BOOL_ 151
#define NULL_ 152
#define INT_T_ 153
#define FLOAT_T_ 154
#define CHAR_T_ 155
#define STRING_T_ 156
#define BOOL_T_ 157
#define VOID_T_ 158


class Handler{
    private:
        vec_stack(std::vector<var_handle>) scopes;
        vec_stack(stack_id_t) scope_stack;
        var_map known_vars;

        fnv1a_t hasher;

        // Must be 1 for the handler to initialize the global
        // variables properly.
        uint_fast16_t new_scope = 1;

        /*
            The scopes vector contains a vector of var_handles for each scope
            *level*. If no variable is defined in a scope, no vector is
            created.
            per example: scopes[1] contains all variables defined in the first
            scope, not globals.

            The scope_stack contains the latter scope_id for each scope.
            *level*. This ensures we don't have to remove all latter known
            vars in "known vars" when a scope is popped, but just check the
            stack_id.

            The known_vars map contains all known variables, referenced by
            their name, and pointing to a stack of nest-defined variable names.

            scope_id is an accumulator for the current scope. It is incremented
            when a new scope is pushed, so that each scope has a unique id.
        */
    public:
    
        Handler(){
            this->scopes = vec_stack(std::vector<var_handle>)();
            // Using a ptr since they are unique and can be used as id
            this->scope_stack = vec_stack(stack_id_t)();
            this->known_vars = var_map();

            this->hasher = fnv1a_t();

            this->init();

            printf("Handler correctly initialized\n");
        }

        ~Handler(){
            std::unordered_set<var_handle *> deleted_vars {};

            for(auto& scope : this->known_vars){
                while(! scope.second.empty()){
                    // If a type has a char*, delete scope.second.top().value
                    /*if(! deleted_vars.count(scope.second.top())){
                        deleted_vars.insert(scope.second.top());
                        delete scope.second.top();  
                    }*/
                    scope.second.pop();
                }
            }
        }

        inline void check_scope(){
            if (this->new_scope)
            {
                printf(" in new scope %zu", this->scopes.size());
                this->scopes.push(std::vector<var_handle>());
                this->scope_stack.push(&this->scopes.top());
                //++this->scope_level;
                --this->new_scope;
            }
        }

        #define VAR_EXISTS 1
        #define VAR_NOT_EXISTS 0
        #define VAR_UNKNOWN 2
        void add_var(const char* name, uint_fast8_t type_id, std::list<uintptr_t> & value, uint_fast8_t var_exists = VAR_UNKNOWN)
            __attribute__((always_inline))
            __attribute__((flatten))
            __attribute__((hot))
            __attribute__((optimize("-Ofast")))
        {
            printf("Adding var %s of type %d with %d values", name, type_id, value.size());
            this->check_scope();

            uint_fast64_t hash = this->hasher.hash(name);

            // If the value is known, flag it
            if(this->operate(value, name))
                type_id |= KNOWN;


            this->scopes.top().push_back(var_handle{
                                            this->scope_stack.top(), 
                                            type_id, 
                                            new std::list<uintptr_t>(value)});

            switch (var_exists)
            {
                case VAR_EXISTS:
                    this->known_vars[hash].push(& this->scopes.top().back());
                    break;
                
                case VAR_NOT_EXISTS:
                    this->known_vars[hash].emplace();
                    this->known_vars[hash].push(& this->scopes.top().back());
                    printf(" SIZE: %d", this->known_vars[hash].size());
                    break;

                // Prolly won't use this, but just in case
                case VAR_UNKNOWN:
                    var_map::iterator var_iter = this->known_vars.find(hash);

                    if(var_iter == this->known_vars.end()){
                        this->known_vars[hash].emplace();
                        this->known_vars[hash].push(& this->scopes.top().back());
                    } else {
                        var_iter->second.push(& (this->scopes.top().back()));
                    }
                    break;
            }    

            printf(" in 0x%x\n", this->scope_stack.top());
        }

        void add_var(const char *name, uint_fast8_t type_id)
            __attribute__((always_inline))
            __attribute__((flatten))
            __attribute__((hot))
            __attribute__((optimize("-Ofast")))
        {
            printf("Adding var %s of type %d", name, type_id);
            this->check_scope();

            uint_fast64_t hash = this->hasher.hash(name);

            this->scopes.top().push_back(var_handle{this->scope_stack.top(), type_id, nullptr});

            var_map::iterator var_iter = this->known_vars.find(hash);

            if(var_iter == this->known_vars.end()){
                this->known_vars[hash].emplace();
                this->known_vars[hash].top() = & this->scopes.top().back();
                printf(" %d", this->known_vars[hash].size());
            } else {
                var_iter->second.push(& (this->scopes.top().back()));
            }
            printf(" in 0x%x\n", this->scope_stack.top());
        }

        var_handle * get_var(const char* name)
            __attribute__((always_inline))
            __attribute__((flatten))
            __attribute__((hot))
            __attribute__((optimize("-Ofast")))
        {
            printf("Getting var %s\n", name);
            var_map::iterator var_iter = this->known_vars.find(this->hasher.hash(name));

            if(var_iter == this->known_vars.end())
                return nullptr;

            vec_stack(var_handle *) & var_stack = var_iter->second;
            
            printf(" %s is in 0x%x (%d) | %d scopes\n", name, var_stack, var_stack.size(), this->scopes.size());

            vec_stack(stack_id_t) existing_vars = vec_stack(stack_id_t)();

            // Var stack is stack of var_handle*
            // Scope stack is stack of vector<>*
            // Check if any of the var_handle* in var_stack is in the current scope
            // stack. If so, return it.
            // Otherwise, pop var_stack.top() and check again.
            while(! var_stack.empty()){
               
                while (! this->scope_stack.empty())
                {
                    printf("0x%x == 0x%x\n", var_stack.top()->stack_id, this->scope_stack.top());
                    if(var_stack.top()->stack_id == this->scope_stack.top()){
                        printf("FOUND\n");
                        while(! existing_vars.empty()){
                            this->scope_stack.push(existing_vars.top());
                            existing_vars.pop();
                        }
                        return var_stack.top();
                    }
                    existing_vars.push(this->scope_stack.top());
                    this->scope_stack.pop();
                }
                printf("-\n");

                while(! existing_vars.empty()){
                    this->scope_stack.push(existing_vars.top());
                    existing_vars.pop();
                }

                var_stack.pop();
            }

            printf("NOT FOUND\n");
            // Just as a fallback
            return nullptr;
        }

        inline void add_scope()
            __attribute__((always_inline))
        {
            //printf("Added new scope\n");
            ++this->new_scope;
        }

        inline void pop_scope()
            __attribute__((always_inline))
            __attribute__((flatten))
        {
            if(this->new_scope && this->scope_stack.top() == & this->scopes.top()){
                --this->new_scope;
            } else {
                this->scope_stack.pop();
                this->scopes.pop();
                printf("Popped scope %d\n", this->scopes.size());
            }
        }

        inline stack_id_t get_current_scope_id()
            __attribute__((always_inline))
            __attribute__((flatten))
        {
            return this->scope_stack.top();
        }

        inline bool check_scope(stack_id_t scope_id)
            __attribute__((always_inline))
            __attribute__((flatten))
        {
            return !this->new_scope && this->scope_stack.top() == scope_id;
        }

        inline void raise_undeclared_variable(const char* name, uint line_number = -1u)
        {
            if(line_number != -1u)
                printf("[-] Error in line %d: ", line_number);
            
            printf("Variable '%s' not declared\n ", name);
            exit(1);
        }

        inline void raise_redefined_variable(const char* name, uint line_number = -1u)
        {
            if(line_number != -1u)
                printf("[-] Error in line %d: ", line_number);

            printf("Variable '%s' already defined\n", name);
            exit(1);
        }

        inline void raise_undefined_variable(const char* name, uint line_number = -1u)
        {
            if(line_number != -1u)
                printf("[-] Error in line %d: ", line_number);

            printf("Variable '%s' has been used, but has not been defined yet\n", name);
            exit(1);
        }

        inline void raise_wrong_call(const char* name, uint line_number = -1u)
        {
            if(line_number != -1u)
                printf("[-] Error in line %d: ", line_number);

            printf("Function '%s' has been called, but it is not callable\n", name);
            exit(1);
        }

    private:

        void init(){
            // RESERVED_WORDS defined in alex.l
            for(std::string reserved_word : RESERVED_WORDS){
                this->add_var(strcpy(
                                    (char*)malloc(reserved_word.length()+1), 
                                    reserved_word.c_str())
                                , 0);
            }
            for(std::string reserved_func : RESERVED_FUNCTIONS){
                this->add_var(strcpy(
                                    (char*)malloc(reserved_func.length()+1), 
                                    reserved_func.c_str())
                                , VOID_T_ | CALLABLE | DEFINED);
            }
        }

        #define NUMBER_TOKENS (VOID_T_ + 127)
        bool operate(std::list<uintptr_t> & value, const char * name)
        {
            // initial implementation, only checks variable validity
            printf("Operating on %s\n", name);

            bool known = true;

            std::list<uintptr_t>::iterator iter = value.begin();
            var_map::iterator var_iter;
            var_handle * var;

            intptr_t left_type;
            intptr_t left_value;
            intptr_t operation;
            intptr_t right_value;

            // Iter must contain structured data
            // After a var_type must be a var
            bool is_var = false;
            while (iter != value.end())
            {
                if(is_var){
                    if(*iter > NUMBER_TOKENS){
                        // iter contains a pointer to a char *
                        char * var_name = (char *) *iter;

                        // Check if the variable is known
                        var = this->get_var(var_name);
                        if(var == nullptr){
                            this->raise_undeclared_variable(var_name);
                        }
                        if(! var->type_id & DEFINED){
                            this->raise_undefined_variable(var_name);
                        }

                        // Variable exists

                        // Check if the variable is known
                        /*
                        if(var->type & KNOWN){
                            // If we know the final value of the variable,
                            // we can just use it
                            if(left_value == 0){
                                left_type = var->type;
                                left_value = var->value;

                                goto continue_var;
                            } else {
                                right_value = var->value;
                            }
                        } else {
                            goto continue_var;
                        }
                        */
                    } else{
                        /*
                        if(left_value == 0){
                            left_value = *(iter - 1);
                            left_value = *iter;

                            goto continue_var;
                        } else {
                            right_value = *iter;
                        }
                        */
                    }

                    /*
                    switch (operation)
                    {
                        case MAS_:
                            left_value += right_value;
                            break;
                        case MENOS_:
                            left_value -= right_value;
                            break;
                        case POR_:
                            left_value *= right_value;
                            break;
                        case DIV_:
                            left_value /= right_value;
                            break;
                        case MOD_:
                            left_value %= right_value;
                            break;
                        case BIN_AND_:
                            left_value &= right_value;
                            break;
                        case BIN_OR_:
                            left_value |= right_value;
                            break;
                        case BIN_XOR_:
                            left_value ^= right_value;
                            break;
                        case BIN_NOT_:
                            left_value = ~left_value;
                            break;
                        case AND_:
                            left_value = left_value && right_value;
                            break;
                        case OR_:
                            left_value = left_value || right_value;
                            break;
                        case LT_:
                            left_value = left_value < right_value;
                            break;
                        case GT_:
                            left_value = left_value > right_value;
                            break;
                        case LE_:
                            left_value = left_value <= right_value;
                            break;
                        case GE_:
                            left_value = left_value >= right_value;
                            break;
                        case EQ_:
                            left_value = left_value == right_value;
                            break;
                        case NE_:
                            left_value = left_value != right_value;
                            break;
                        case SHIFT_IZQ_:
                            left_value <<= right_value;
                            break;
                        case SHIFT_DER_:
                            left_value >>= right_value;
                            break;
                        
                        
                        default:
                            break;
                    }
                    */

                    continue_var:
                    is_var = false;
                } else {
                    // iter contains a var_type
                    switch(*iter){
                        case INT_:
                            //[[fallthrough]];
                        case FLOAT_:
                            //[[fallthrough]];
                        case CHAR_:
                            //[[fallthrough]];
                        case STRING_:
                            //[[fallthrough]];
                        case BOOL_:
                            is_var = true;
                            break;
                        case MAS_:
                            //[[fallthrough]];
                        case MENOS_:
                            //[[fallthrough]];
                        case POR_:
                            //[[fallthrough]];
                        case DIV_:
                            //[[fallthrough]];
                        case MOD_:
                            //[[fallthrough]];
                        case AND_:
                            //[[fallthrough]];
                        case OR_:
                            //[[fallthrough]];
                        case BIN_XOR_:
                            //[[fallthrough]];
                        case BIN_AND_:
                            //[[fallthrough]];
                        case BIN_OR_:
                            //[[fallthrough]];
                        case BIN_NOT_:
                            //[[fallthrough]];
                        case NOT_:
                            //[[fallthrough]];
                        case SHIFT_IZQ_:
                            //[[fallthrough]];
                        case SHIFT_DER_:
                            //[[fallthrough]];
                        case LT_:
                            //[[fallthrough]];
                        case GT_:
                            //[[fallthrough]];
                        case LE_:
                            //[[fallthrough]];
                        case GE_:
                            //[[fallthrough]];
                        case EQ_:
                            //[[fallthrough]];
                        case NE_:
                            operation = *iter;
                            break;

                        case MASMAS_:
                        case MENOSMENOS_:
                            // Could assume that the next *iter is a var
                            // yet, I won't implement anything because
                            // looks rather ugly.
                            break;

                        default:
                            break;
                        }
                }

                ++iter;
            }
        }
};


#endif