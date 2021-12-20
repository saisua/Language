#ifndef C_LINKED_LIST_CPP
#define C_LINKED_LIST_CPP

#include <vector>
#include <unordered_map>
#include <unistd.h>
#include <cstdio>

enum {
    NODE_S = 1,
    COLL_S = 1 << 1,
    MAX_S = 1 << 2
};

template <typename T>
class C_linked_list {
    private:
        std::vector<T*> _array_starts;
        std::vector<T*> _pointers;
        std::unordered_map<T, T*> _groups;

        // Positions per node
        uint _node_size = 0;
        // Positions per collection

        unsigned long _collection_size = sysconf (_SC_LEVEL1_DCACHE_LINESIZE) * 2;
        // Total positions reserved
        unsigned long _size = sysconf (_SC_LEVEL1_DCACHE_LINESIZE) * 4;
        // Total collections reserved
        unsigned long _size_coll = 2;
    
        // Wether the collection is a multiple of the total size
        bool _coll_m_size = true;
        // Wether the collection size has ever been modified
        bool _modified_coll_size = false;

        // Fixed flags not to modify any user-defined values
        u_int8_t _flags;

        // System cache line size
        #define _CACHE_SIZE sysconf (_SC_LEVEL1_DCACHE_LINESIZE)

        // For fibbonacci reserve
        unsigned long _added = sysconf(_SC_LEVEL1_DCACHE_LINESIZE) * 4;
        unsigned long _last_added = sysconf(_SC_LEVEL1_DCACHE_LINESIZE) * 4;

    public:
        T* actual_pointer = nullptr;
        uint actual_space = 0;

        T* actual_array = nullptr;
        uint actual_array_index = 0;

        T* next_array = nullptr;
        uint next_array_index = 0;

        // Used groups
        uint size = 0;
        // Used collections
        uint collections = 0;

        C_linked_list() {
            this->_array_starts = std::vector<T*>();
            this->_pointers = std::vector<T*>();
            this->_groups = std::unordered_map<T, T*>();

            this->_pointers.push_back(new T[this->_collection_size]);
            this->_array_starts.push_back(this->_pointers[0]);
            this->_pointers.push_back(new T[this->_collection_size]);
            this->_array_starts.push_back(this->_pointers[1]);

            this->actual_array = this->_array_starts.front();
            this->actual_pointer = this->actual_array;
            this->actual_array_index = 0;
            this->actual_space = this->_collection_size;    

            this->next_array = this->_array_starts.front()+1;
            this->next_array_index = 1;
        }

        // Would be useful to have a constructor that takes a vector of T
        // and a linked list of T*. However, I don't need it right now.

        ~C_linked_list() {
            //T** ptr = this->_array_starts.data();

            //for(uint pos = this->_array_starts.size(); pos; --pos, ++ptr) {
                //delete *ptr;
            //}
        }

        T operator[](size_t index){
            return *(this->_array_starts[index/this->_collection_size]+(index%this->_collection_size));
        }

        inline T* at(uint index){
            if(!this->_modified_coll_size){
                return this->_array_starts[index/this->_collection_size]+(index%this->_collection_size);
            }

            for(T* array : this->_array_starts){
                if(index < *array)
                    return array+index;

                index -= *array;
            }
        }

        inline T* at_node(uint index){
            return this->_pointers[index];
        }

        inline T* append_position(){
            if(! this->actual_space)
                this->next_collection();

            return this->actual_pointer++;
        }

        inline T* append_node(size_t positions=NULL, bool add_size=false)
            __attribute__((always_inline))
            __attribute__((hot))
            __attribute__((flatten))
        {
            if(!positions)
                if(this->_node_size)
                    positions = this->_node_size;
                else
                    return nullptr;

            //printf("Selected %d positions\n", positions);

            if(this->actual_space < positions){
                //printf("Allocated new collection\n");
                this->next_collection();
            }

            T* result = this->actual_pointer;

            this->_pointers.push_back(this->actual_pointer);
            this->actual_pointer += positions + add_size;

            if(add_size){
                //printf("Added size\n");
                *result = positions;

                return result + 1;
            }

            return result;
        }

        inline void next_collection()
            __attribute__((always_inline))
            __attribute__((hot))
            __attribute__((flatten))
        {
            if(this->collections == this->_size_coll || this->next_array == nullptr){
                this->reserve_fibb(true);
            }
            else{
                ++this->collections;

                this->actual_array = this->next_array;
                this->actual_pointer = this->actual_array+1;
                this->actual_array_index = this->next_array_index;

                this->actual_space = *this->actual_array;

                ++this->next_array;
                ++this->next_array_index;
            }
        }

        inline void set_current_collection(T* array)
            __attribute__((always_inline))
            __attribute__((hot))
            __attribute__((flatten))
        {
            ++this->collections;

            this->actual_pointer = array+1;
            this->actual_array = array;

            this->actual_space = *array;

            if(this->collections == this->_size_coll){
                this->next_array = nullptr;
                this->next_array_index = 0;
            }
        }

        inline void set_current_collection(T* array, uint index)
            __attribute__((always_inline))
            __attribute__((hot))
            __attribute__((flatten))
        {
            ++this->collections;

            this->actual_array = array;
            this->actual_array_index = index;   

            this->actual_space = *array;
        }

        inline void set_current_collection(T* array, uint index, T* next_array_var, uint next_index)
            __attribute__((always_inline))
            __attribute__((hot))
            __attribute__((flatten))
        {
            ++this->collections;

            this->actual_array = array;
            this->actual_array_index = index;  

            this->actual_space = *array; 

            this->next_array = next_array_var;
            this->next_array_index = next_index;
        }

        inline void set_current_collection(std::vector<T*>::iterator & array, long unsigned int index, 
                                        std::vector<T*>::iterator & next_array_iter, long unsigned int next_index)
            __attribute__((always_inline))
            __attribute__((hot))
            __attribute__((flatten))
        {
            ++this->collections;

            this->actual_array = *array;
            this->actual_array_index = index;  

            this->actual_space = **array; 

            this->next_array = *next_array_iter;
            this->next_array_index = next_index;
        }

        inline T* reserve_group(T& group_index) {
            // If already exists, check if it is the last in
            // array_starts. If it is not, move all starts one
            // position forward.

            auto last_group = this->_groups.find(group_index);

            // If the collection doesn't exist, or it is the last one,
            // reserve two new collections, assign to the first free collection, 
            // and move next_array to the second.
            if(last_group > this->_groups.end()-1) {
                // If there are no free collections, allocate a new one
                if(this->collections > this->_size_coll - 2){
                    // This will reserve at least 2 more collections
                    this->reserve_fibb();
                    
                    this->next_array = this->_array_starts.back();
                    this->next_array_index = this->_array_starts.size()-1;
                
                    this->_groups[group_index] = this->next_array+1;
                } else {
                    this->_groups[group_index] = this->next_array+1;

                    this->next_array = this->_array_starts[++this->next_array_index];
                }
            } 
            // Otherwise, insert a new one in the position of the previous + 1
            else {
                const uint pos = std::distance(this->_groups.begin(), last_group) + 1;
                
                this->_groups[group_index] = this->insert_new(pos);

                // Since we did not use the new array, we don't need to change
                // but because we did add a new entry in _array_starts, we need to
                // add 1 to next_array_index
                if(this->next_array_index > pos)
                    ++this->next_array_index;
                if(this->current_array_index > pos)
                    ++this->current_array_index;
            }

            ++this->collections;
        }

        void set_node_size(const uint nsize, uint nodes_per_coll=10, uint at_least_nodes=20 ) {
            this->_flags = (this->_flags | NODE_S);

            uint mod_cache_size = _CACHE_SIZE % nsize;

            // Make node size a divisor of the cache line size
            if(mod_cache_size)
                this->_node_size = (nsize - mod_cache_size) * 2;
            
            // Make collection size a multiple of the cache line size
            if(! this->_flags & COLL_S) {
                uint prev_coll_size = this->_collection_size;

                if(this->_node_size > _CACHE_SIZE)
                    this->_collection_size = this->_node_size * _CACHE_SIZE;
                else
                    if(_CACHE_SIZE * 2 >= this->_node_size * nodes_per_coll)
                        this->_collection_size = _CACHE_SIZE * 2;
                    else{
                        uint mod_cache_coll;
                        
                        this->_collection_size = this->_node_size * nodes_per_coll;
                        
                        // Make it (near) multiple of the cache line size and node_size
                        // Tl;dr: add the node size to the collection size until it
                        // surpasses the next cache line size multiple.
                        if(mod_cache_coll = (_CACHE_SIZE % this->_collection_size))
                            this->_collection_size += (this->_node_size * 
                                    (((_CACHE_SIZE - (mod_cache_coll)) / this->_node_size)+1));
                    }

                this->_modified_coll_size = prev_coll_size == this->_collection_size;
            }

            // Make reserved_size a multiple of the collection size
            if(! this->_flags & MAX_S){
                this->_size = at_least_nodes * this->_node_size;

                uint mod_size = (at_least_nodes * this->_node_size) % this->_collection_size;

                if(mod_size){
                    this->_size += this->_collection_size - mod_size;
                }
            }

            this->reserve();
        }

        void set_collection_size(const uint csize) {
            this->_flags = (this->_flags | COLL_S);

            uint prev_coll_size = this->_collection_size;

            // Don't set node size, since it may be variable

            // Make collection (near) size a multiple of the cache line size
            // Tl;dr: add the node size to the collection size until it
            // surpasses the next cache line size multiple.
            if(this->mod_cache_coll = _CACHE_SIZE % this->_collection_size)
                this->_collection_size = csize + (this->_node_size * 
                        (((_CACHE_SIZE - this->mod_cache_coll) / this->_node_size)+1));

            this->_modified_coll_size = prev_coll_size == this->_collection_size;

            if(! this->flags & MAX_S){
                this->_size = this->_collection_size * 2;
            }
            
            this->reserve();
        }

        void reserve()
            __attribute__((always_inline))
            __attribute__((hot))
            __attribute__((flatten))
        {
            this->reserve(this->_size - (this->collections * this->_collection_size));
        }

        void reserve(const uint rsize, bool set_actual=true)
                    __attribute__((always_inline))
                    __attribute__((hot))
                    __attribute__((flatten)) 
        {
            this->_flags = (this->_flags | MAX_S);

            if(rsize > this->_size){
                reserve_new(this->_size / this->_collection_size + 
                                (! rsize % this->_collection_size),
                                set_actual);
            }
        }

        void reserve_fibb(bool set_actual=true)
                    __attribute__((always_inline))
                    __attribute__((hot))
                    __attribute__((flatten))
        {
            uint aux = this->_last_added;

            this->_last_added = this->_added;

            this->reserve(this->_added += aux, set_actual);
        }

        void reserve_new(bool set_actual=true)
            __attribute__((always_inline))
            __attribute__((hot))
            __attribute__((flatten))
        {
            this->_size += collections * this->_collection_size;
            ++this->size_coll;

            T* new_array = new T[this->_collection_size+1];
            *new_array = this->_collection_size;

            this->_array_start.push_back(new_array);

            if(set_actual){
                this->set_current_collection(new_array);
            }
        }

        void reserve_new(int ncollections, bool set_actual = false)
            __attribute__((always_inline))
            __attribute__((hot))
            __attribute__((flatten))
        {
            T* new_array;

            this->_size += ncollections * this->_collection_size;

            ncollections -= this->collections;
            this->_size_coll += ncollections;

            int new_collections = ncollections;

            for(; ncollections; --ncollections) {
                new_array = new T[this->_collection_size+1];
                *new_array = this->_collection_size;

                this->_array_starts.push_back(new_array);
            }

            typename
            std::vector<T*>::iterator last = this->_array_starts.end()-new_collections;

            if(set_actual) 
                this->set_current_collection(last, 
                                            this->_size_coll - new_collections + 1,
                                            last,
                                            this->_size_coll - new_collections + 2);
        }

        // Use the node_size and make sure the collection size
        // can fit the nodes.
        void reserve_nodes(const uint nodes, const uint node_size=4) {
            this->reserve(nodes * node_size);
        }

        void reserve_collections(uint ncollections) {
            this->reserve(ncollections * this->_collection_size);
        }

        T* insert_new(const uint index){
            this->_size += collections * this->_collection_size;
            ++this->size_coll;

            T* new_array = new T[this->_collection_size+1];
            *new_array = this->_collection_size;

            this->_array_start.insert(this->_array_start.begin() + index, new_array);

            return new_array;
        }

        void resize(const uint rsize) {
            this->_flags = (this->_flags | MAX_S);


        }

};

/*
#ifndef MAIN
#define MAIN
int main(){
    C_linked_list<uint> list = C_linked_list<uint>();
    list.set_node_size(2);

    uint* a = list.append_node(2, true);

    *a = 3;
    *(a+1) = 4;

    printf("%u > %u > (%u, %u)\n", *(a-2), *(a-1), *a, *(a+1));

    return 0;
}
#endif
*/  
#endif