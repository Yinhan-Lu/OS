#ifndef SHELLMEMORY_H
#   define SHELLMEMORY_H
#   define PAGE_SIZE 3

// Define default values for FRAMESIZE and VARMEMSIZE if not provided at compile time
#   ifndef FRAMESIZE
#      define FRAMESIZE 3
#   endif

#   ifndef VARMEMSIZE
#      define VARMEMSIZE 10
#   endif

#   define MEM_SIZE 1000
#   define MAX_TOKEN_SIZE 100
#   define MAX_USER_INPUT 1000

// Memory structure
struct memory_struct {
    char *var;
    char *value;
};

// Frame owner structure
struct FrameOwner {
    char *scriptName;
    int page;
};

// Declare memory arrays
extern struct memory_struct frame_store[FRAMESIZE];
extern struct memory_struct var_store[VARMEMSIZE];
extern struct FrameOwner frameOwners[FRAMESIZE / PAGE_SIZE];
extern int frame_access_time[FRAMESIZE / PAGE_SIZE];

// 特殊的空标记字符串，使用字符串常量而不是NULL
extern const char *EMPTY_SLOT;

// Interface functions
void mem_init ();
char *mem_get_value (char *var);
void mem_set_value (char *var, char *value);
void mem_remove_value (char *script, int length);
int find_free_frame ();
// Change the function declaration
void load_line_to_frame (int frame, int offset, char *line,
                         const char *scriptName, int page);
void update_frame_access_time (int frame);
int find_lru_frame ();

// Frame owner management
void initFrameOwners ();
void updateFrameOwner (int frame, char *scriptName, int page);

// 辅助函数
int is_empty_slot (const char *str);
void safe_free (char **ptr);

#endif // End of SHELLMEMORY_H
