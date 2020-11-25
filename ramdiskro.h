// c 2018,2020



#ifndef _RAMDISKRO_H
#define _RAMDISKRO_H



//#define RDRO_INCLUDE 0 // do not include
#define RDRO_INCLUDE 1 // include stdint.h and stddef.h

#define RDRO_ENDIAN 0 // native endian
//#define RDRO_ENDIAN 1 // little endian
//#define RDRO_ENDIAN 2 // big endian



#if RDRO_INCLUDE
#include <stdint.h>
#include <stddef.h>
	// type for filesystem data
//typedef uint16_t rdro_t; // 16 bit: sizes up to ~64kiB
//typedef uint32_t rdro_t; // 32 bit: sizes up to ~4GiB
typedef uint64_t rdro_t; // 64 bit: sizes up to ~16EiB
	// type for sizes
typedef size_t num_t;
#else
	// type for filesystem data
//typedef unsigned short rdro_t;
//typedef unsigned int rdro_t;
typedef unsigned long long rdro_t;
	// type for sizes
typedef unsigned long num_t;
#endif



#if RDRO_ENDIAN == 0
#define RDRO_HTOX(r) (r)
#define RDRO_XTOH(r) (r)
#else
#define RDRO_HTOX(r) rdro_swap_if(r)
#define RDRO_XTOH(r) rdro_swap_if(r)
#endif



#ifdef __cplusplus
extern "C" {
#endif



// magic value
#define RDRO_MAGIC_SIZE (8)
extern const unsigned char rdro_magic[RDRO_MAGIC_SIZE];



// constants and macros
#define RDRO_NONE ((rdro_t)-1)
#define RDRO_MAX ((rdro_t)-2)
#define RDRO_DIR (((rdro_t)1)<<(sizeof(rdro_t)*8-1))
#define RDRO_IS_DIR(f) (((f) & RDRO_DIR) != 0)
#define RDRO_ROOT_INODE (0)
// for rdro_check
#define RDRO_SIZE_ANY (0)
#define RDRO_SIZE_NOMORE (1)
#define RDRO_SIZE_NOLESS (2)
#define RDRO_SIZE_EXACT (RDRO_SIZE_NOLESS|RDRO_SIZE_NOMORE)



// superblock
typedef struct rdro_super
{
	unsigned char magic[RDRO_MAGIC_SIZE]; // magic number
	unsigned char rdro_size; // size of rdro_t
	unsigned char version; // rdro format version
	unsigned char pad[6]; // pad
	rdro_t endian; // endianness check
	rdro_t inodes; // number of inodes
	rdro_t size; // image size in bytes
} rdro_super;



// inode
typedef struct rdro_inode
{
	rdro_t start; // offset from the beginning of the image in bytes
	rdro_t size; // size in bytes
	rdro_t type; // type of inode (dir/file) and attributes
} rdro_inode;



// header of directory
typedef struct rdro_diri
{
	rdro_t entries; // number of entries of the directory, counting . and ..
} rdro_diri;



// entry of directory
typedef struct rdro_dire
{
	rdro_t inode; // number of inode of the entry
	rdro_t name; // offset of inode data where the name starts
} rdro_dire;



// returned data of rdro_get_data_from_inode and rdro_get_data_from_path
// native endianness
typedef struct rdro_data
{
	void *data; // pointer to the contents
	rdro_t size; // size in bytes of the contents
	rdro_t type; // type of inode (dir/file) and attributes
	rdro_t inode; // inode number
} rdro_data;



// swap endianness if needed
rdro_t rdro_swap_if(rdro_t r);



// compare strings a and b
// in: a: first string
// in: a_size: size of first string in bytes
// in: b: second string
// in: b_size: size of second string in bytes
// return value:
// -1: a < b
//  0: a = b
//  1: a > b
int rdro_strcmp_n(const char *a, num_t a_size, const char *b, num_t b_size);



// check validity of image
// it is recommended to call this once before using the image
// in: super: pointer to the start of the image
// in: size: (optional) size allocated to the image in bytes
// in: flags:
// RDRO_SIZE_ANY -> do not check image size
// RDRO_SIZE_NOMORE -> chech that image size in superblock isn't greater than given size
// RDRO_SIZE_NOLESS -> chech that image size in superblock isn't smaller then given size
// RDRO_SIZE_EXACT -> check that image size in superblock is exactly equal to given size
// return value: 0 if ok or pointer to null-terminated error string if error
const char *rdro_check(const void *super, num_t size, num_t flags);



// get number of inodes
// in: super: pointer to the start of the image
// out: inodes: pointer to number of inodes
// return value: 0 if ok or pointer to null-terminated error string if error
const char *rdro_get_inode_count(const void *super, rdro_t *inodes);



// get pointer to inode
// in: super: pointer to the start of the image
// in: inode: number of inode to get
// out: ino: pointer to pointer to inode
// return value: 0 if ok or pointer to null-terminated error string if error
const char *rdro_get_inode(const void *super, rdro_t inode, const rdro_inode **ino);



// get number of entries if inode is a directory
// directories do always have 2 or more entries
// first is "."; second is ".."; next ones, if any, are sorted
// in: super: pointer to the start of the image
// in: ino: pointer to inode
// out: entries: pointer to number of entries
// return value: 0 if ok or pointer to null-terminated error string if error
const char *rdro_get_dir_entry_count(const void *super, const rdro_inode *ino, rdro_t *entries);



// get directory entry
// in: super: pointer to the start of the image
// in: ino: pointer to inode
// in: entry: number of entry
// out: inode: pointer to number of inode of the entry
// out: name: pointer to pointer to the name of the entry
// out: name_size: pointer to number of bytes of the name of the entry (only filled if it isn't nullptr)
// return value: 0 if ok or pointer to null-terminated error string if error
const char *rdro_get_dir_entry(const void *super, const rdro_inode *ino, rdro_t entry, rdro_t *inode, const char **name, num_t *name_size);



// get inode number from path
// in: super: pointer to the start of the image
// in: path: null-terminated path
// out: inode: pointer to inode number
// return value: 0 if ok or pointer to null-terminated error string if error
const char *rdro_get_inode_from_path(const void *super, const char *path, rdro_t *inode);



// get data from inode pointer
// in: super: pointer to the start of the image
// in: ino: pointer to inode
// out: data: pointer to data
// return value: 0 if ok or pointer to null-terminated error string if error
const char *rdro_get_data_from_ino(const void *super, const rdro_inode *ino, rdro_data *data);



// get data from inode number
// in: super: pointer to the start of the image
// in: inode: inode number
// out: data: pointer to data
// return value: 0 if ok or pointer to null-terminated error string if error
const char *rdro_get_data_from_inode(const void *super, rdro_t inode, rdro_data *data);



// get data from path
// in: super: pointer to the start of the image
// in: path: null-terminated path
// out: data: pointer to data
// return value: 0 if ok or pointer to null-terminated error string if error
const char *rdro_get_data_from_path(const void *super, const char *path, rdro_data *data);



#ifdef __cplusplus
}
#endif



#endif
