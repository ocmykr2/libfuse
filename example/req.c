/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  This program can be distributed under the terms of the GNU GPLv2.
  See the file COPYING.
*/

/** @file
 *
 * minimal example filesystem using high-level API
 *
 * Compile with:
 *
 *     gcc -Wall req.c `pkg-config fuse3 --cflags --libs` -o caesar
 *
 * ## Source code ##
 * \include caesar.c
 */


#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>

FILE *fp;

/*
 * Command line options
 *
 * We can't set default values for the char* fields here because
 * fuse_opt_parse would attempt to free() them when the user specifies
 * different values on the command line.
 */

struct Node {
    char* name;
    char* content;
    struct Node * father;
    struct Node * pre;
    struct Node * nxt;
    struct Node * list;
};

struct Node * root;

struct Node * New_Node() {
    void * ptr = malloc(sizeof(struct Node));
    memset(ptr, 0, sizeof(struct Node));
    return (struct Node*) ptr;
}

void Free_Node(struct Node * x) {
    if((x -> name) != NULL) free((x -> name));
    if((x -> content) != NULL) free((x -> content));
    if((x -> father) != NULL) free((x -> father));
    if((x -> pre) != NULL) free((x -> pre));
    if((x -> nxt) != NULL) {
        Free_Node((x -> nxt));
    }
    if((x -> list) != NULL) {
        Free_Node((x -> list));
    }
    free(x);
}

int Find_User(struct Node** x, char * who) {
    struct Node * _root  = (root -> list);
    int is_found = 0;
    while(_root != NULL) {
        if(!strcmp((_root) -> name, who)) {
            is_found = 1; break;
        }
        (_root) = (_root) -> nxt;
    }
    (*x) = _root;
    if(is_found) return 0;
    else return -1;
}

void Set_Node(struct Node* who, char * name, char * content, struct Node * father, struct Node *pre, struct Node * nxt, struct Node * list) {
    (who) -> name = name;
    (who) -> content = content;
    (who) -> father = father;
    (who) -> pre = pre;
    (who) -> nxt = nxt;
    (who) -> list = list;
}

void Add_User(struct Node* New, char *name) {
    if((root -> list) != NULL) {
        (root -> list) -> pre = New;
    }
    Set_Node(New, strdup(name), NULL, root, NULL, (root -> list), NULL);
    assert(root != NULL);
    assert(New != NULL);
    (root -> list) = New;
}

int Find_Diary(struct Node *x, struct Node** y, char *name) {
    x = (x -> list);
    while(x != NULL) {
        if(!strcmp((x -> name), name)) {
            (*y) = x;
            return 0;
        } 
        x = (x -> nxt);
    }
    *y = NULL;
    return -1;
}

void Add_Diary(struct Node *x, struct Node * y, char* name) {
    if(x -> list != NULL) {
        (x -> list) ->  pre = y;
    }
    Set_Node(y, strdup(name), strdup(""), x, NULL, (x -> list), NULL);
}

void Insert(const char * Path) {
    // struct Node * cur = root;
    int t = 0, Len = strlen(Path);
    t = Len;
    for(int i = 0; i < Len; ++ i)  {
        if(Path[i] == '/') {
            t = i; break;
        }
    }

    char * who = malloc(sizeof(char) * (t + 1));
    for(int i = 0; i < t; ++ i) who[i] = Path[i];
    who[t] = '\0';
    
    struct Node * x;

    if(Find_User(&x, who)) {
        x = New_Node();
        Add_User(x, who);
    }

    free(who);

    if(t < Len - 1) {
        int len2 = Len - t - 1;
	    char *w = malloc(sizeof(char) * (len2 + 1));
        for(int i = t + 1; i < Len; ++ i)
        w[i - t - 1] = Path[i];
        w[len2] = '\0';
        struct Node * y;
        if(Find_Diary(x, &y, w)) {
            y = New_Node();
            Add_Diary(x, y, w);
            x -> list = y;
        }
        free(w);
    }
}

int Find_Path(struct Node** which, const char *Path) {
    // struct Node * cur = root;
    int t = 0, Len = strlen(Path);
    t = Len;
    for(int i = 0; i < Len; ++ i)  {
        if(Path[i] == '/') {
            t = i; break;
        }
    }

    char * who = malloc(sizeof(char) * (t + 1));
    for(int i = 0; i < t; ++ i) who[i] = Path[i];
    who[t] = '\0';
    
    struct Node * x;

    if(Find_User(&x, who)) {free(who); return -1;}
    (*which) = x;

    if(t < Len - 1) {
        int len2 = Len - t - 1;
	    free(who);
	    who = malloc(sizeof(char) * (len2 + 1));
        for(int i = t + 1; i < Len; ++ i)
        who[i - t - 1] = Path[i];
        who[len2] = '\0';
        struct Node * y;
        if(Find_Diary(x, &y, who)) {
		    free(who);
		    return -1;
        }
	    (*which) = y;
    }
    free(who); return 0;
}


static void *caesar_init(struct fuse_conn_info *conn,
			struct fuse_config *cfg)
{
	(void) conn;
	cfg->kernel_cache = 0;
	return NULL;
}

static int caesar_getattr(const char *Path, struct stat *stbuf,
			 struct fuse_file_info *fi)
{

(void) fi;
    memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(Path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
        return 0;
	}
    char *path = malloc(strlen(Path));
    int Len = strlen(Path);
    for(int i = 1; i < Len; ++ i)
    path[i - 1] = Path[i];
    path[Len - 1] = '\0';
    -- Len;
    struct Node * x;
    int sta = Find_Path(&x, path);
    free(path);
    if(sta) {return -ENOENT;}
	if ((x -> father) == root) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
        return 0;
	}
	stbuf->st_mode = S_IFREG | 0770;
	stbuf->st_nlink = 1;
    if((x -> content) != NULL)
    stbuf->st_size = strlen(x->content);
    else stbuf -> st_size = 0;
	return 0;
}

static int caesar_readdir(const char *Path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi,
			 enum fuse_readdir_flags flags)
{
	(void) offset;
	(void) fi;
	(void) flags;
    if(strlen(Path) == 0) return -ENOENT;
    char *path = malloc(strlen(Path));
    int Len = strlen(Path);
    for(int i = 1; i < Len; ++ i)
    path[i - 1] = Path[i];
    path[Len - 1] = '\0';
    -- Len;   

    struct Node * x;
    if(Len > 0) {
        int sta = Find_Path(&x, path);
        free(path);
        if(sta != 0) return -ENOENT;
	     if((x -> father) != root) return -ENOENT;
    } else x = root;
	filler(buf, ".", NULL, 0, 0);
	filler(buf, "..", NULL, 0, 0);
	x = x -> list;
	while(x != NULL) {
        	filler(buf, (x -> name), NULL, 0, 0);
        	x = x -> nxt;
    }

	return 0;
}

static int caesar_open(const char *Path, struct fuse_file_info *fi)
{
    return 0;
    if(strlen(Path) == 1) return 0;
    char *path = malloc(strlen(Path));
    int Len = strlen(Path);
    for(int i = 1; i < Len; ++ i)
    path[i - 1] = Path[i];
    path[Len - 1] = '\0';
    -- Len;   
    struct Node * x;
    if(Len > 0) {
        int sta = Find_Path(&x, path);
        free(path);
        if(sta != 0) return -ENOENT;
	    //if((x -> father) == root) return -ENOENT;
	    return 0;
    }
    return -ENOENT;
}

static int caesar_read(const char *Path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
size_t len;
(void) fi;
    if(strlen(Path) <= 0) return -ENOENT;

    char *path = malloc(strlen(Path));
    int Len = strlen(Path);
    for(int i = 1; i < Len; ++ i)
    path[i - 1] = Path[i];
    path[Len - 1] = '\0';
    -- Len;
    struct Node * x;
    if(Len > 0) {
        int sta = Find_Path(&x, path);
        free(path);
        if(sta != 0) return -ENOENT;
        if((x -> father) == root) return -ENOENT;
    } else return -ENOENT;

	len = strlen(x -> content);
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, (x -> content) + offset, size);
	} else
		size = 0;
	return size;
}

static int caesar_write(const char *Path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi) {
    if(strlen(Path) <= 0) return -ENOENT;
    (void) fi;
    char *path = malloc(strlen(Path));
    int Len = strlen(Path);
    for(int i = 1; i < Len; ++ i)
    path[i - 1] = Path[i];
    path[Len - 1] = '\0';
    -- Len;   
    struct Node * x;
    if(Len > 0) {
        int sta = Find_Path(&x, path);
        if(sta != 0) return -ENOENT;
    }

    if(x -> father == root || x == root) {
        free(path);
        return -ENOENT;
    }

    char *User = (char*)malloc((strlen((x -> father) -> name) + 3) * sizeof(char)); 
    memcpy(User, (x -> father) -> name, strlen((x -> father) -> name));
    int lenUser = strlen((x -> father) -> name) + 2;
    User[lenUser - 2] = ':';
    User[lenUser - 1] = '\n';
    User[lenUser]  = '\0';

    Len = offset + lenUser + size + 1;
    
    char * NewContent = (char*) malloc(Len + 1);
    memcpy(NewContent, (x -> content), offset);
    memcpy(NewContent + offset, User, lenUser);
    memcpy(NewContent + offset + lenUser, buf, size);
    NewContent[Len] = '\0';

    free(x -> content);
    (x -> content) = NewContent;

    char *new_path = (char*)malloc(strlen(path) + 1);

    memcpy(new_path, x -> name, strlen(x -> name));
    new_path[strlen(x -> name)] = '/';
    memcpy(new_path + strlen(x -> name) + 1, (x -> father) -> name, strlen((x -> father) -> name));
    new_path[strlen(path)] = '\0';

    free(path);

    int sta = Find_Path(&x, new_path);
    if(sta) {
        return -ENOENT;
    }

    if(x -> father == root) {
        return -ENOENT;
    }

    free(x -> content);
    (x -> content) = strdup(NewContent);
    
    return size;
}

static int caesar_mkdir(const char *Path, mode_t mode) {
    if(strlen(Path) == 0) return -ENOENT;
    char *path = malloc(strlen(Path));
    int Len = strlen(Path);
    for(int i = 1; i < Len; ++ i)
    path[i - 1] = Path[i];
    path[Len - 1] = '\0';
    -- Len;
    int t = Len;
    for(int i = 0; i < Len; ++ i) {
        if(path[i] == '/') {
            t = i; break;
        }
    }
    if(t != Len) {
        return -ENOENT;
    }

    Insert(path);
    free(path);
	return 0;
}

static int caesar_mknod(const char * Path, mode_t mode, dev_t di) {
    (void) di;
    (void) mode;
    char *path = malloc(strlen(Path));
    int Len = strlen(Path);
    for(int i = 1; i < Len; ++ i)
    path[i - 1] = Path[i];
    path[Len - 1] = '\0';
    -- Len;
    int t = Len;
    for(int i = 0; i < Len; ++ i) {
        if(path[i] == '/') {
            t = i; break;
        }
    }
    if(t == Len) return -EPIPE;
    struct Node * x; int sta = 0;
    sta = Find_Path(&x, path);
    //if(sta != 0) return -ENOENT;
    // assert(sta != 0);
    if(!sta) {
        free(path);
        return 0;
    }
    else Insert(path);
    Find_Path(&x, path);
    (x -> content) = strdup("You have not set up your username,you must restart your computer to set it up.\n");

    char *new_path = (char*)malloc(strlen(path) + 1);
    memcpy(new_path, x -> name, strlen(x -> name));
    new_path[strlen(x -> name)] = '/';
    memcpy(new_path + strlen(x -> name) + 1, (x -> father) -> name, strlen((x -> father) -> name));
    new_path[strlen(path)] = '\0';   
    //exit(0);
    if(Find_Path(&x, new_path)) {
        Insert(new_path);
        Find_Path(&x, new_path);
        (x -> content) = strdup("You have not set up your username,you must restart your computer to set it up.\n");
    }

    free(path);
    free(new_path);


    return 0;
}

static const struct fuse_operations caesar_oper = {
    .init       =   caesar_init,
    .getattr    =   caesar_getattr,
    .readdir    =   caesar_readdir,
    .open       =   caesar_open,
    .read       =   caesar_read,
    .mkdir      =   caesar_mkdir,
    .write      =   caesar_write,
    .mknod      =   caesar_mknod,
};

int main(int argc, char *argv[])
{
    //fp = fopen("log.c", "a");

	/*if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
		return 1;*/
	int sta;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    root = New_Node(); Set_Node(root, NULL, NULL, NULL, NULL, NULL, NULL);

	sta = fuse_main(args.argc, args.argv, &caesar_oper, NULL);
	fuse_opt_free_args(&args);
    
    Free_Node(root);
    //fclose(fp);
	return sta;
}
