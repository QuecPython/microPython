/*
 * Copyright (c) Quectel Wireless Solution, Co., Ltd.All Rights Reserved.
 *  
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *  
 *     http://www.apache.org/licenses/LICENSE-2.0
 *  
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#if defined(PLAT_Qualcomm)

#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/mpthread.h"
#include "py/objstr.h"
#include "extmod/vfs.h"
#include "extmod/vfs_efs.h"

#include <stdio.h>
#include <string.h>

#include "helios_fs.h"


#define EFS_NAME_MAX   (768)   /* Maximum length of a file name. */
#define EFS_IFDIR      (0040000) 	/**< Directory */
#define EFS_IFMT       (0170000)		/**< Mask of all values */

 struct efs_stat_type_s {
  uint16_t          st_dev;       /**< Unique device ID among the mounted file
                                     systems. */
  uint32_t          st_ino;       /**< INode number associated with the file. */
  uint16_t          st_Mode;      /**< Mode associated with the file. */
  uint8_t           st_nlink;     /**< Number of active links that are
                                     referencing this file. The	space occupied
                                     by the file is released after its
                                     references are reduced to 0. */
  uint32_t          st_size;      /**< File size in bytes. */
  unsigned long   st_blksize;   /**< Block size; smallest allocation unit of
                                     the file system. The unit in which
                                     the block Count is represented. */
  unsigned long   st_blocks;    /**< Number of blocks allocated for this file
                                     in st_blksize units. */
  uint32_t          st_atime;     /**< Last access time. This is not updated, so 
                                     it might have an incorrect value. */
  uint32_t          st_mtime;     /**< Last modification time. Currently, this 
                                     indicates the time when the file was
                                     created. */
  uint32_t          st_ctime;     /**< Last status change time. Currently, this
                                     indicates the time when the file was
                                     created. */
  uint32_t          st_rdev;      /**< Major and minor device number for special
                                     device files. */
  uint16_t          st_uid;       /**< Owner or user ID of the file. */
  uint16_t          st_gid;       /**< Group ID of the file. The stored file data
                                     blocks are charged to the quota of this
                                     group ID. */
};
									 
enum efs_filename_rule_e{
    EFS_FILENAME_RULE_8BIT_RELAXED = 1,  /**< 8-bit relaxed rule. */
    EFS_FILENAME_RULE_FAT          = 2,  /**< FAT rule. */
    EFS_FILENAME_RULE_FDI          = 3   /**< FDI rule. */
};

enum efs_filename_encoding_e{
    EFS_FILENAME_ENCODING_8BIT = 1,  /**< 8-bit encoding. */
    EFS_FILENAME_ENCODING_UTF8 = 2 	 /**< UTF8 encoding. */
};
struct efs_statvfs_type_s {
  unsigned long                        f_bsize;
  /**< Fundamental file system block size. Minimum allocations in
       the file system happen at this size. */
  uint32_t                               f_blocks;
  /**< Maximum possible number of blocks available in the entire
       file system. */
  uint32_t                               f_bfree;
  /**< Total number of free blocks. */
  uint32_t                               f_bavail;
  /**< Number of free blocks currently available. */
  uint32_t                               f_files;
  /**< Total number of file serial numbers. */
  uint32_t                               f_ffree;
  /**< Total number of free file serial numbers. */
  uint32_t                               f_favail;
  /**< Number of file serial numbers available. */
  unsigned long                        f_fsid;
  /**< File system ID; this varies depending on the implementation
       of the file system. */
  unsigned long                        f_flag;
  /**< Bitmask of f_flag values. */
  unsigned long                        f_namemax;
  /**< Maximum length of the name part of the string for a file,
       directory, or symlink. */
  unsigned long                        f_maxwrite;
  /**< Maximum number of bytes that can be written in a single
       write call. */
  uint32_t                               f_balloc;
  /**< Blocks allocated in the general pool. */
  uint32_t                               f_hardalloc;
  /**< Hard allocation count. Resource intensive, so this is not
       usually computed. */
  unsigned long                        f_pathmax;
  /**< Maximum path length, excluding the trailing NULL. The unit here
       is in terms of character symbols. The number of bytes needed 
       to represent a character will vary depending on the 
       file name encoding scheme. For example, in a UTF8 encoding scheme,
       representing a single character could take anywhere between 
       1 to 4 bytes. */
  unsigned long                        f_is_case_sensitive;
  /**< Set to 1 if Path is case sensitive. */
  unsigned long                        f_is_case_preserving;
  /**< Set to 1 if Path is case preserved. */
  unsigned long                        f_max_file_size;
  /**< Maximum file size in the units determined by the member
       f_max_file_size_unit. */
  unsigned long                        f_max_file_size_unit;
  /**< Unit type for f_max_file_size. */
  unsigned long                        f_max_open_files;
  /**< This member tells how many files can be kept opened for one 
       particular file system. However, there is a global limit on how 
	   many files can be kept opened simultaneously across all 
	   file systems, which is configured by EFS_MAX_DESCRIPTORS. */
  enum efs_filename_rule_e         f_name_rule;
  /**< File naming rule. */
  enum efs_filename_encoding_e     f_name_encoding;
  /**< Encoding scheme. */
};

struct efs_iter_entry_s {
  char file_Path[EFS_NAME_MAX+1];
  /**< Name of the directory component. */
  struct efs_stat_type_s SBuf;
  /**< See qapi_FS_Stat_Type_s for information on this structure. */
  uint32_t EFS_D_Stats_Present; 
  /**< Bitmask for the EFS_Stat_Type_s structure that defines
       which fields are filled when the Helios_fdirread() API is called. */
};

enum { EFS_MAKE_ARG_pname };

const mp_arg_t efs_make_allowed_args[] = {   
    { MP_QSTR_pname, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
};

extern int Helios_frename(const char *old_path, const char *new_path);
extern int Helios_frmdir(const char *path);
extern int Helios_fmkdir(const char *path, uint16_t mode);
extern int Helios_fstat(const char *path, struct efs_stat_type_s *sb);
extern int Helios_fstatvfs(const char *path, struct efs_statvfs_type_s *sb);
extern void* Helios_fdiropen(const char *path);
extern int Helios_fdirread(void* iter_handle, struct efs_iter_entry_s  *iter_entry);
extern int Helios_fdirclose(void* iter_handle);

const char *mp_vfs_efs_make_path(mp_obj_vfs_efs_t *self, mp_obj_t path_in) {
	const char *path = mp_obj_str_get_str(path_in);
    if (path[0] != '/') {
        size_t l = vstr_len(&self->cur_dir);
        if (l > 0) {
            vstr_add_str(&self->cur_dir, path);
            path = vstr_null_terminated_str(&self->cur_dir);
            self->cur_dir.len = l;
        }
    }
    return path;
}

STATIC mp_import_stat_t mp_vfs_efs_import_stat(void *self_in, const char *path) {
	mp_obj_vfs_efs_t *self = self_in;
	struct efs_stat_type_s sb = {0};
	
	//compatible for "from usr import module" function
	if(0 == strncmp(path, "usr", 3))
	{
		path = &path[3];
	}

	mp_obj_str_t path_obj = { { &mp_type_str }, 0, 0, (const byte *)path };
    path = mp_vfs_efs_make_path(self, MP_OBJ_FROM_PTR(&path_obj));
	char f_path[64] = "/usr";
	sprintf(f_path, "%s%s", "/usr", path);
	
	int ret = Helios_fstat(f_path, &sb);
	if (ret == 0) 
	{
	    if (((sb.st_Mode) & EFS_IFMT) == EFS_IFDIR) 
		{
			return MP_IMPORT_STAT_DIR;
	    } 
		else 
		{
	        return MP_IMPORT_STAT_FILE;
	    }
	}

    return MP_IMPORT_STAT_NO_EXIST;
}

STATIC mp_obj_t vfs_efs_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
	mp_arg_val_t args_parse[MP_ARRAY_SIZE(efs_make_allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(efs_make_allowed_args), efs_make_allowed_args, args_parse);
	char *partition_name = (char *)mp_obj_str_get_str(args_parse[EFS_MAKE_ARG_pname].u_obj);

    mp_obj_vfs_efs_t *vfs = m_new_obj(mp_obj_vfs_efs_t);
    vfs->base.type = type;
	vstr_init(&vfs->cur_dir, 16);
    vstr_add_byte(&vfs->cur_dir, '/');
	
	if(strcmp(partition_name, "customer_fs") == 0)
	{
		vfs->readonly = false;
		//create dir for micropython dir "/usr"
		Helios_fmkdir("/usr", 0777);
	}
	else
	{
		vfs->readonly = true;
		//create dir for micropython dir "/bak"
		Helios_fmkdir("/bak", 0777);
	}
	
    return MP_OBJ_FROM_PTR(vfs);
}

STATIC mp_obj_t vfs_efs_mount(mp_obj_t self_in, mp_obj_t readonly, mp_obj_t mkfs) {
    /*mp_obj_vfs_posix_t *self = MP_OBJ_TO_PTR(self_in);
    if (mp_obj_is_true(readonly)) {
        self->readonly = true;
    }
    if (mp_obj_is_true(mkfs)) {
        mp_raise_OSError(MP_EPERM);
    }
    return mp_const_none;*/
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(vfs_efs_mount_obj, vfs_efs_mount);

STATIC mp_obj_t vfs_efs_umount(mp_obj_t self_in) {
    (void)self_in;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(vfs_efs_umount_obj, vfs_efs_umount);

STATIC mp_obj_t vfs_efs_open(mp_obj_t self_in, mp_obj_t path_in, mp_obj_t mode_in) {
    mp_obj_vfs_efs_t *self = MP_OBJ_TO_PTR(self_in);
    const char *mode = mp_obj_str_get_str(mode_in);
	const char *path = mp_obj_str_get_str(path_in);

	//compatible for "from usr import module" function
	if(0 == strncmp(path, "usr", 3))
	{
		path = &path[3];
	}
	
	mp_obj_str_t path_obj = { { &mp_type_str }, 0, 0, (const byte *)path };
    path = mp_vfs_efs_make_path(self, MP_OBJ_FROM_PTR(&path_obj));
	path_obj.data = (const byte *)path;

    return mp_vfs_efs_file_open(self_in, &mp_type_vfs_efs_textio, MP_OBJ_FROM_PTR(&path_obj), mode_in);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(vfs_efs_open_obj, vfs_efs_open);

STATIC mp_obj_t vfs_efs_chdir(mp_obj_t self_in, mp_obj_t path_in) {
    mp_obj_vfs_efs_t *self = MP_OBJ_TO_PTR(self_in);

    // Check path exists
    const char *path = mp_vfs_efs_make_path(self, path_in);
    if (path[1] != '\0') {
        // Not at root, check it exists
		struct efs_stat_type_s sb = {0};
		char f_path[64] = "";
		if(self->readonly)
		{
			sprintf(f_path, "%s%s", "/bak", path);
		}
		else
		{
			sprintf(f_path, "%s%s", "/usr", path);
		}
		
		int ret = Helios_fstat(f_path, &sb);
		if ((ret != 0) || ((sb.st_Mode & EFS_IFMT) != EFS_IFDIR)) {
	        mp_raise_OSError(-MP_ENOENT);
	    }
    }

    // Update cur_dir with new path
    if (path == vstr_str(&self->cur_dir)) {
        self->cur_dir.len = strlen(path);
    } else {
        vstr_reset(&self->cur_dir);
        vstr_add_str(&self->cur_dir, path);
    }

    // If not at root add trailing / to make it easy to build paths
    // and then normalise the path
    if (vstr_len(&self->cur_dir) != 1) {
        vstr_add_byte(&self->cur_dir, '/');

	#define CWD_LEN (vstr_len(&self->cur_dir))
        size_t to = 1;
        size_t from = 1;
        char *cwd = vstr_str(&self->cur_dir);
        while (from < CWD_LEN) {
            for (; cwd[from] == '/' && from < CWD_LEN; ++from) {
                // Scan for the start
            }
            if (from > to) {
                // Found excessive slash chars, squeeze them out
                vstr_cut_out_bytes(&self->cur_dir, to, from - to);
                from = to;
            }
            for (; cwd[from] != '/' && from < CWD_LEN; ++from) {
                // Scan for the next /
            }
            if ((from - to) == 1 && cwd[to] == '.') {
                // './', ignore
                vstr_cut_out_bytes(&self->cur_dir, to, ++from - to);
                from = to;
            } else if ((from - to) == 2 && cwd[to] == '.' && cwd[to + 1] == '.') {
                // '../', skip back
                if (to > 1) {
                    // Only skip back if not at the tip
                    for (--to; to > 1 && cwd[to - 1] != '/'; --to) {
                        // Skip back
                    }
                }
                vstr_cut_out_bytes(&self->cur_dir, to, ++from - to);
                from = to;
            } else {
                // Normal element, keep it and just move the offset
                to = ++from;
            }
        }
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(vfs_efs_chdir_obj, vfs_efs_chdir);

STATIC mp_obj_t vfs_efs_getcwd(mp_obj_t self_in) {
    mp_obj_vfs_efs_t *self = MP_OBJ_TO_PTR(self_in);
    if (vstr_len(&self->cur_dir) == 1) {
        return MP_OBJ_NEW_QSTR(MP_QSTR__slash_);
    } else {
        // don't include trailing /
        return mp_obj_new_str(self->cur_dir.buf, self->cur_dir.len - 1);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(vfs_efs_getcwd_obj, vfs_efs_getcwd);

typedef struct _vfs_efs_ilistdir_it_t {
    mp_obj_base_t base;
    mp_fun_1_t iternext;
    bool is_str;
    void *dir;
	char ilist_path[64];
	bool readonly;
} vfs_efs_ilistdir_it_t;

STATIC mp_obj_t vfs_efs_ilistdir_it_iternext(mp_obj_t self_in) {
    vfs_efs_ilistdir_it_t *self = MP_OBJ_TO_PTR(self_in);
	char real_file_path[128] = "";
	struct efs_stat_type_s sb = {0};

    if (self->dir == NULL) 
	{
        return MP_OBJ_STOP_ITERATION;
    }
	struct efs_iter_entry_s iter_entry;
	
	for (;;) 
	{
		int ret = Helios_fdirread(self->dir, &iter_entry);
		
        if (ret != 0 || strlen(iter_entry.file_Path) == 0) 
		{
			// stop on error or end of dir
			break;
        }

		if (iter_entry.file_Path[0] == '.' && (iter_entry.file_Path[1] == 0 || iter_entry.file_Path[1] == '.')) 
		{
            // skip . and ..
            continue;
        }	   

		// make 4-tuple with info about this entry
	    mp_obj_tuple_t *t = MP_OBJ_TO_PTR(mp_obj_new_tuple(4, NULL));
	    if (self->is_str) 
		{
	        t->items[0] = mp_obj_new_str(iter_entry.file_Path, strlen(iter_entry.file_Path));
	    } 
		else 
		{
	        t->items[0] = mp_obj_new_bytes((const byte *)iter_entry.file_Path, strlen(iter_entry.file_Path));
	    }

		sprintf(real_file_path, "%s%s", self->ilist_path, iter_entry.file_Path);
		Helios_fstat(real_file_path, &sb);
		memset (real_file_path, 0, sizeof(real_file_path));
	    t->items[1] = MP_OBJ_NEW_SMALL_INT(((sb.st_Mode) & EFS_IFMT) == EFS_IFDIR ? MP_S_IFDIR : MP_S_IFREG);
	    t->items[2] = MP_OBJ_NEW_SMALL_INT(0); // no inode number
	    t->items[3] = MP_OBJ_NEW_SMALL_INT(sb.st_size);

		return MP_OBJ_FROM_PTR(t);   
    }

	Helios_fdirclose(self->dir);
	self->dir = NULL;
	
	return MP_OBJ_STOP_ITERATION;
}

STATIC mp_obj_t vfs_efs_ilistdir(mp_obj_t self_in, mp_obj_t path_in) {
	mp_obj_vfs_efs_t *self = MP_OBJ_TO_PTR(self_in);
    vfs_efs_ilistdir_it_t *iter = m_new_obj(vfs_efs_ilistdir_it_t);
    iter->base.type = &mp_type_polymorph_iter;
    iter->iternext = vfs_efs_ilistdir_it_iternext;
    iter->is_str = mp_obj_get_type(path_in) == &mp_type_str;
    const char *path = mp_obj_str_get_str(path_in);
	char f_path[64] = "";
	int len = 0;
	
    if (path[0] == '\0') {
        path = ".";
    }
	
	if(self->readonly)
	{
		sprintf(f_path, "%s%s", "/bak", path);
		iter->readonly = true;
	}
	else
	{
		sprintf(f_path, "%s%s", "/usr", path);
		iter->readonly = false;
	}
	len = (strlen(f_path) < 60) ? strlen(f_path) : 60;
	memset (iter->ilist_path, 0, sizeof(iter->ilist_path));
	strncpy(iter->ilist_path, f_path, len); 
	
    iter->dir = Helios_fdiropen(f_path);
    if (iter->dir == NULL) {
        mp_raise_OSError(-1);
    }
    return MP_OBJ_FROM_PTR(iter);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(vfs_efs_ilistdir_obj, vfs_efs_ilistdir);

STATIC mp_obj_t vfs_efs_mkdir(mp_obj_t self_in, mp_obj_t path_in) {
	mp_obj_vfs_efs_t *self = MP_OBJ_TO_PTR(self_in);
    const char *path = mp_vfs_efs_make_path(self, path_in);
    char f_path[64] = "";

	if(self->readonly)
	{
		sprintf(f_path, "%s%s", "/bak", path);
	}
	else
	{
		sprintf(f_path, "%s%s", "/usr", path);
	}
	
	int ret = Helios_fmkdir(f_path, 0777);
	if (ret != 0) {
        mp_raise_OSError(ret);
    }
	
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(vfs_efs_mkdir_obj, vfs_efs_mkdir);

STATIC mp_obj_t vfs_efs_remove(mp_obj_t self_in, mp_obj_t path_in) {
    mp_obj_vfs_efs_t *self = MP_OBJ_TO_PTR(self_in);
    const char *path = mp_vfs_efs_make_path(self, path_in);
	char f_path[64] = "";

	if(self->readonly)
	{
		sprintf(f_path, "%s%s", "/bak", path);
	}
	else
	{
		sprintf(f_path, "%s%s", "/usr", path);
	}
	
    int ret = Helios_remove(f_path);
	if (ret != 0) {
        mp_raise_OSError(ret);
    }
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(vfs_efs_remove_obj, vfs_efs_remove);

STATIC mp_obj_t vfs_efs_rename(mp_obj_t self_in, mp_obj_t old_path_in, mp_obj_t new_path_in) {
    mp_obj_vfs_efs_t *self = MP_OBJ_TO_PTR(self_in);
    const char *old_path = mp_vfs_efs_make_path(self, old_path_in);
    const char *new_path = mp_obj_str_get_str(new_path_in);
    vstr_t path_new;
    vstr_init(&path_new, vstr_len(&self->cur_dir));
    if (new_path[0] != '/') {
        vstr_add_strn(&path_new, vstr_str(&self->cur_dir), vstr_len(&self->cur_dir));
    }
    vstr_add_str(&path_new, new_path);
	char o_path[64] = "";
	char n_path[64] = "";

	if(self->readonly)
	{
		sprintf(o_path, "%s%s", "/bak", old_path);
		sprintf(n_path, "%s%s", "/bak", vstr_null_terminated_str(&path_new));
	}
	else
	{
		sprintf(o_path, "%s%s", "/usr", old_path);
		sprintf(n_path, "%s%s", "/usr", vstr_null_terminated_str(&path_new));
	}

	
    int ret = Helios_frename(o_path, n_path);
    vstr_clear(&path_new);
    if (ret != 0) {
        mp_raise_OSError(ret);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(vfs_efs_rename_obj, vfs_efs_rename);

STATIC mp_obj_t vfs_efs_rmdir(mp_obj_t self_in, mp_obj_t path_in) {
    mp_obj_vfs_efs_t *self = MP_OBJ_TO_PTR(self_in);
    const char *path = mp_vfs_efs_make_path(self, path_in);
    char f_path[64] = "";

	if(self->readonly)
	{
		sprintf(f_path, "%s%s", "/bak", path);
	}
	else
	{
		sprintf(f_path, "%s%s", "/usr", path);
	}
	
	int ret = Helios_frmdir(f_path);
	if (ret != 0) {
        mp_raise_OSError(ret);
    }
	
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(vfs_efs_rmdir_obj, vfs_efs_rmdir);

STATIC mp_obj_t vfs_efs_stat(mp_obj_t self_in, mp_obj_t path_in) {
    int ret;
	struct efs_stat_type_s sb = {0};
	mp_obj_vfs_efs_t *self = MP_OBJ_TO_PTR(self_in);
    const char *path = mp_vfs_efs_make_path(self, path_in);
	char f_path[64] = "";
	
	if(self->readonly)
	{
		sprintf(f_path, "%s%s", "/bak", path);
	}
	else
	{
		sprintf(f_path, "%s%s", "/usr", path);
	}
	
	ret = Helios_fstat(f_path, &sb);
	if (ret != 0) {
        mp_raise_OSError(ret);
    }
	
    mp_obj_tuple_t *t = MP_OBJ_TO_PTR(mp_obj_new_tuple(10, NULL));
    t->items[0] = MP_OBJ_NEW_SMALL_INT(((sb.st_Mode) & EFS_IFMT) == EFS_IFDIR ? MP_S_IFDIR : MP_S_IFREG);// st_mode
    t->items[1] = MP_OBJ_NEW_SMALL_INT(0);//mp_obj_new_int_from_uint(sb.st_ino);// st_ino
    t->items[2] = MP_OBJ_NEW_SMALL_INT(0);//mp_obj_new_int_from_uint(sb.st_dev);// st_dev
    t->items[3] = MP_OBJ_NEW_SMALL_INT(0);//mp_obj_new_int_from_uint(sb.st_nlink);// st_nlink
    t->items[4] = MP_OBJ_NEW_SMALL_INT(0);//mp_obj_new_int_from_uint(sb.st_uid);// st_uid
    t->items[5] = MP_OBJ_NEW_SMALL_INT(0);//mp_obj_new_int_from_uint(sb.st_gid);// st_gid
    t->items[6] = mp_obj_new_int_from_uint(sb.st_size);// st_size
    t->items[7] = MP_OBJ_NEW_SMALL_INT(0);//mp_obj_new_int_from_uint(sb.st_atime);// st_atime
    t->items[8] = MP_OBJ_NEW_SMALL_INT(0);//mp_obj_new_int_from_uint(sb.st_mtime);// st_mtime
    t->items[9] = MP_OBJ_NEW_SMALL_INT(0);//mp_obj_new_int_from_uint(sb.st_ctime);// st_ctime
    return MP_OBJ_FROM_PTR(t);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(vfs_efs_stat_obj, vfs_efs_stat);


STATIC mp_obj_t vfs_efs_statvfs(mp_obj_t self_in, mp_obj_t path_in) {
	mp_obj_vfs_efs_t *self = MP_OBJ_TO_PTR(self_in);
	int ret;
	struct efs_statvfs_type_s sb = {0};
    const char *path = mp_obj_str_get_str(path_in);
	char f_path[64] = "";
	
	if(self->readonly)
	{
		sprintf(f_path, "%s%s", "/bak", path);
	}
	else
	{
		sprintf(f_path, "%s%s", "/usr", path);
	}

	ret = Helios_fstatvfs(f_path, &sb);
	if (ret != 0) {
        mp_raise_OSError(ret);
    }
	
    mp_obj_tuple_t *t = MP_OBJ_TO_PTR(mp_obj_new_tuple(10, NULL));
    t->items[0] = MP_OBJ_NEW_SMALL_INT(sb.f_bsize);// f_bsize
    t->items[1] = t->items[0];// f_frsize
    t->items[2] = MP_OBJ_NEW_SMALL_INT(sb.f_blocks);//MP_OBJ_NEW_SMALL_INT(256);// f_blocks
    t->items[3] = MP_OBJ_NEW_SMALL_INT(sb.f_bfree);// f_bfree
    t->items[4] = t->items[3];// f_bavail
    t->items[5] = MP_OBJ_NEW_SMALL_INT(0); // f_files
    t->items[6] = MP_OBJ_NEW_SMALL_INT(0); // f_ffree
    t->items[7] = MP_OBJ_NEW_SMALL_INT(0); // f_favail
    t->items[8] = MP_OBJ_NEW_SMALL_INT(0); // f_flags
    t->items[9] = MP_OBJ_NEW_SMALL_INT(sb.f_namemax);
    return MP_OBJ_FROM_PTR(t);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(vfs_efs_statvfs_obj, vfs_efs_statvfs);

STATIC const mp_rom_map_elem_t vfs_efs_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_mount), MP_ROM_PTR(&vfs_efs_mount_obj) },
    { MP_ROM_QSTR(MP_QSTR_umount), MP_ROM_PTR(&vfs_efs_umount_obj) },
    { MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&vfs_efs_open_obj) },

    { MP_ROM_QSTR(MP_QSTR_chdir), MP_ROM_PTR(&vfs_efs_chdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_getcwd), MP_ROM_PTR(&vfs_efs_getcwd_obj) },
    { MP_ROM_QSTR(MP_QSTR_ilistdir), MP_ROM_PTR(&vfs_efs_ilistdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_mkdir), MP_ROM_PTR(&vfs_efs_mkdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_remove), MP_ROM_PTR(&vfs_efs_remove_obj) },
    { MP_ROM_QSTR(MP_QSTR_rename), MP_ROM_PTR(&vfs_efs_rename_obj) },
    { MP_ROM_QSTR(MP_QSTR_rmdir), MP_ROM_PTR(&vfs_efs_rmdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_stat), MP_ROM_PTR(&vfs_efs_stat_obj) },
    { MP_ROM_QSTR(MP_QSTR_statvfs), MP_ROM_PTR(&vfs_efs_statvfs_obj) },
};
STATIC MP_DEFINE_CONST_DICT(vfs_efs_locals_dict, vfs_efs_locals_dict_table);

STATIC const mp_vfs_proto_t vfs_efs_proto = {
    .import_stat = mp_vfs_efs_import_stat,
};

const mp_obj_type_t mp_type_vfs_efs = {
    { &mp_type_type },
    .name = MP_QSTR_VfsEfs,
    .make_new = vfs_efs_make_new,
    .protocol = &vfs_efs_proto,
    .locals_dict = (mp_obj_dict_t *)&vfs_efs_locals_dict,
};

#endif // PLAT_Qualcomm
