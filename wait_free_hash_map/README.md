# wait free hash map implementation

### interface
TBD: 
- wtf_table_count();
- wtf_table_print(); or wtf_table_apply()
- wtf_table_remove();

TODO: update interface
```
// wtf stands for WaiT Free
typedef struct wtf_table* wtf_table_t

// Creates table instance and returns pointer on it
wtf_table_t wtf_table_create();

// Inserts key-value pair it table. If the table holds the key, value overwrites the previous
// value, and wtf_table_insert returns the previous value. Otherwise, key value pair is added to the table, 
// and wtf_table_insert returns NULL
void* wtf_table_insert(wtf_table_t t, const void* key, const void* value);

// Returns value associated with the key, if the table doesn't hold the key, returns NULL
void* wtf_table_find(wtf_table_t t, const void* key);

// Deallocates table content and the table itself, sets t to NULL
void wtf_table_destroy(wtf_table_t* t);
```

### hash func
Assumed the table receives precalculated hashed keys

### memory management
As the standard memory allocator is blocking, special provisions must be made for lock-free and
wait-free programs. In order for the hash map to behave in a wait-free manner, the user must choose 
a memory allocator that can manage memory in a wait-free manner

- tcmalloc(https://goog-perftools.sourceforge.net/doc/tcmalloc.html);
- lockless allocator(http://locklessinc.com/products/allocvl.shtml);
- custom allocator;
- ???

### tests
cmocka tests on board
TODO: create fixture

### benchmarks
TODO: gbench?

### sanitizing, formatting, linting
TODO: add clang-tidy call on each build call
TODO: add formatting on each call
