// c 2018,2020



#include "ramdiskro.h"



const unsigned char rdro_magic[RDRO_MAGIC_SIZE] = {0xdc,0xf3,0x18,0x17,0xaa,0x76,0xc4,0x80};
const unsigned short end = 1;
const unsigned char *endp = (const unsigned char *)&end;



rdro_t rdro_swap_if(rdro_t r)
{
#if RDRO_ENDIAN == 0
	return r;
#elif RDRO_ENDIAN == 1
	if (*endp != 0)
		return r;
#elif RDRO_ENDIAN == 2
	if (*endp == 0)
		return r;
#else
#error wrong RDRO_ENDIAN
#endif
#if RDRO_ENDIAN != 0
	rdro_t t = 0;
	for (num_t b = 0 ; b != sizeof(rdro_t) ; ++b)
		t |= ((r >> (b * 8)) & 0xff) << ((sizeof(rdro_t) - 1 - b) * 8);
	return t;
#endif
}



int rdro_strcmp_n(const char *a, num_t a_size, const char *b, num_t b_size)
{
	num_t size_min = a_size;
	if (b_size < size_min)
		size_min = b_size;
	for (num_t p = 0 ; p != size_min ; ++p)
	{
		if ((unsigned char)(a[p]) < (unsigned char)(b[p]))
			return -1;
		if ((unsigned char)(a[p]) > (unsigned char)(b[p]))
			return 1;
	}
	if (a_size < b_size)
		return -1;
	if (a_size > b_size)
		return 1;
	return 0;
}



// non-recursive
// first and last are included
rdro_t rdro_search_bin_3(const rdro_diri *diri, rdro_t size,
				const char *name, num_t name_size,
				rdro_t first, rdro_t last)
{
	rdro_t entries = RDRO_XTOH(diri->entries);
	while (1)
	{
		if (last >= entries)
			return RDRO_NONE;
		if (first > last)
			return RDRO_NONE;
		rdro_t half = (first + last) / 2;
		const rdro_dire *h = (const rdro_dire*)(diri+1) + half;
		// test for overflow
		if (RDRO_XTOH(h->name) >= size)
			return RDRO_NONE;
		const unsigned char *hn = ((const unsigned char*)diri) + RDRO_XTOH(h->name);
		// test for integer overflow
		if (RDRO_XTOH(h->name) + (rdro_t)1 + (rdro_t)(hn[0]) < RDRO_XTOH(h->name))
			return RDRO_NONE;
		// test for overflow
		if (RDRO_XTOH(h->name) + (rdro_t)1 + (rdro_t)(hn[0]) >= size)
			return RDRO_NONE;
		int ret = rdro_strcmp_n(name, name_size, (const char*)(hn+1), hn[0]);
		if (ret < 0)
			last = half-1;
		else if (ret > 0)
			first = half+1;
		else
			return RDRO_XTOH(((rdro_dire*)(diri+1))[half].inode);
	}
}



rdro_t rdro_search_bin(const rdro_diri *dir, rdro_t size,
			const char *name, num_t name_size)
{
	// prev: rdro_diri
	//    0: rdro_dire: .
	//    1: rdro_dire: ..
	//  ...: ...
	if (sizeof(rdro_diri) > size)
		return RDRO_NONE;
	if (RDRO_XTOH(dir->entries) < 2)
		return RDRO_NONE;
	// test for overflow
	if (RDRO_XTOH(dir->entries) > (size - sizeof(rdro_diri)) / sizeof(rdro_dire))
		return RDRO_NONE;
	if (name_size == 1 && name[0] == '.')
		return RDRO_XTOH(((rdro_dire*)(dir+1))[0].inode);
	if (name_size == 2 && name[0] == '.' && name[1] == '.')
		return RDRO_XTOH(((rdro_dire*)(dir+1))[1].inode);
	return rdro_search_bin_3(dir, size, name, name_size, 2, RDRO_XTOH(dir->entries)-1);
}



const char *rdro_check(const void *super_void, num_t size, num_t flags)
{
	const rdro_super *super = (const rdro_super *)super_void;
	if (super == 0)
		return "rdro_check: passed null pointer as ramdisk";
	if ((flags & RDRO_SIZE_NOMORE) && (sizeof(super) > size))
		return "rdro_check: size of superblock too big";
	for (num_t l = 0 ; l != RDRO_MAGIC_SIZE ; ++l)
		if (super->magic[l] != rdro_magic[l])
			return "rdro_check: different magic value in superblock";
	if (super->rdro_size != sizeof(rdro_t))
		return "rdro_check: different rdro_t size in superblock";
	if (super->version != 1)
		return "rdro_check: different version value in superblock";
	if (RDRO_XTOH(super->endian) != 1)
		return "rdro_check: different endianness in superblock";
	if (RDRO_XTOH(super->size) < sizeof(super))
		return "rdro_check: bad size in superblock";
	if ((flags & RDRO_SIZE_NOMORE) && (RDRO_XTOH(super->size) > size))
		return "rdro_check: size in superblock too big";
	if ((flags & RDRO_SIZE_NOLESS) && (RDRO_XTOH(super->size) < size))
		return "rdro_check: size in superblock too small";
	if (RDRO_XTOH(super->inodes) == 0)
		return "rdro_check: inode count is 0";
	// test for overflow
	if (RDRO_XTOH(super->inodes) > (RDRO_XTOH(super->size) - sizeof(super)) / sizeof(rdro_inode))
		return "rdro_check: inode count too high";
	return 0;
}



const char *rdro_get_inode_count(const void *super_void, rdro_t *inodes)
{
	const rdro_super *super = (const rdro_super *)super_void;
	*inodes = RDRO_XTOH(super->inodes);
	return 0;
}



const char *rdro_get_inode(const void *super_void, rdro_t inode, const rdro_inode **ino)
{
	const rdro_super *super = (const rdro_super *)super_void;
	if (inode >= RDRO_XTOH(super->inodes))
		return "rdro_get_inode: inode too high";
	const rdro_inode *inode_p = ((const rdro_inode*)(super + 1)) + inode;
	rdro_t data_start = sizeof(super) + RDRO_XTOH(super->inodes) * sizeof(rdro_inode);
	rdro_t data_end = RDRO_XTOH(super->size);
	// test for integer overflow
	if ((RDRO_XTOH(inode_p->start)+RDRO_XTOH(inode_p->size)) < RDRO_XTOH(inode_p->start))
		return "rdro_get_inode: inode size overflow";
	if (RDRO_XTOH(inode_p->start) < data_start)
		return "rdro_get_inode: inode starts before limit";
	if ((RDRO_XTOH(inode_p->start)+RDRO_XTOH(inode_p->size)) > data_end)
		return "rdro_get_inode: inode ends after limit";
	*ino = inode_p;
	return 0;
}



const char *rdro_get_dir_entry_count(const void *super_void, const rdro_inode *ino, rdro_t *entries)
{
	if (!RDRO_IS_DIR(RDRO_XTOH(ino->type)))
		return "rdro_get_dir_entry_count: not a directory";
	const rdro_diri *diri = (const rdro_diri*)((const char*)super_void + RDRO_XTOH(ino->start));
	if (RDRO_XTOH(ino->size) < sizeof(rdro_diri))
		return "rdro_get_dir_entry_count: inode size too small";
	*entries = RDRO_XTOH(diri->entries);
	return 0;
}



const char *rdro_get_dir_entry(const void *super_void, const rdro_inode *ino, rdro_t entry, rdro_t *inode, const char **name, num_t *name_size)
{
	if (!RDRO_IS_DIR(RDRO_XTOH(ino->type)))
		return "rdro_get_dir_entry: not a directory";
	const rdro_diri *diri = (const rdro_diri*)((const char*)super_void + RDRO_XTOH(ino->start));
	if (RDRO_XTOH(ino->size) < sizeof(rdro_diri))
		return "rdro_get_dir_entry: inode size too small 1";
	// test for overflow
	if (RDRO_XTOH(diri->entries) > (RDRO_XTOH(ino->size) - sizeof(rdro_diri)) / sizeof(rdro_dire))
		return "rdro_get_dir_entry: number of entries too high";
	if (entry >= RDRO_XTOH(diri->entries))
		return "rdro_get_dir_entry: entry too high";
	const rdro_dire *h = ((const rdro_dire*)(diri+1)) + entry;
	if (RDRO_XTOH(ino->size) < sizeof(rdro_diri) + sizeof(rdro_dire) * RDRO_XTOH(diri->entries))
		return "rdro_get_dir_entry: inode size too small 2";
	// test for overflow
	if (RDRO_XTOH(h->name) >= RDRO_XTOH(ino->size))
		return "rdro_get_dir_entry: name overflow 1";
	const unsigned char *hn = ((const unsigned char*)diri) + RDRO_XTOH(h->name);
	// test for integer overflow
	if (RDRO_XTOH(h->name) + (rdro_t)1 + (rdro_t)(hn[0]) < RDRO_XTOH(h->name))
		return "rdro_get_dir_entry: name overflow 2";
	// test for overflow
	if (RDRO_XTOH(h->name) + (rdro_t)1 + (rdro_t)(hn[0]) >= RDRO_XTOH(ino->size))
		return "rdro_get_dir_entry: name overflow 3";
	if (hn[1+hn[0]] != 0)
		return "rdro_get_dir_entry: name isn't null-terminated";
	*inode = RDRO_XTOH(h->inode);
	*name = (const char*)(hn+1);
	if (name_size != 0)
		*name_size = hn[0];
	return 0;
}



const char *rdro_get_inode_from_path(const void *super_void, const char *path, rdro_t *inode)
{
	if (path == 0)
		return "rdro_get_inode_from_path: passed null pointer as path";
	const rdro_super *super = (const rdro_super *)super_void;
	rdro_t inode_num = RDRO_ROOT_INODE;
	num_t path_dir = 1;
	while (1)
	{
		// discard
		while (*path == '/')
			++path;

		// get pointer to inode
		const rdro_inode *inode_p = 0;
		const char *err = rdro_get_inode(super_void, inode_num, &inode_p);
		if (err)
			return err;
		if (inode_p == 0)
			return "rdro_get_inode_from_path: inconsistency: cannot get inode";
		num_t inode_dir = RDRO_IS_DIR(RDRO_XTOH(inode_p->type));

		// if end of string: ok
		if (*path == 0)
		{
			if (path_dir && !inode_dir)
			{
				return "rdro_get_inode_from_path: last path component is a directory and found a file";
			}
			else
			{
				*inode = inode_num;
				return 0;
			}
		}

		// continue
		const char *pathini = path;
		while (*path != '/' && *path != 0)
		{
			++path;
			if ((path - pathini) > 255)
				return "rdro_get_inode_from_path: path component is longer than 255 bytes";
		}
		path_dir = *path == '/';

		// if can deepen
		if (inode_dir)
		{
			const rdro_diri *diri = (const rdro_diri*)
				((const char*)super + RDRO_XTOH(inode_p->start));
			rdro_t num_next = rdro_search_bin(diri, RDRO_XTOH(inode_p->size),
				pathini, path - pathini);
			if (num_next == RDRO_NONE)
				return "rdro_get_inode_from_path: path component not found";
			inode_num = num_next;
		}
		else
			return "rdro_get_inode_from_path: path component is a directory and found a file";
	}
}



const char *rdro_get_data_from_ino(const void *super, const rdro_inode *ino, rdro_data *data)
{
	data->data = ((unsigned char*)super) + RDRO_XTOH(ino->start);
	data->size = RDRO_XTOH(ino->size);
	data->type = RDRO_XTOH(ino->type);
	data->inode = -1;
	return 0;
}



const char *rdro_get_data_from_inode(const void *super, rdro_t inode, rdro_data *data)
{
	const rdro_inode *ino = 0;
	const char *err = rdro_get_inode(super, inode, &ino);
	if (err != 0)
		return err;
	if (ino == 0)
		return "rdro_get_data_from_inode: cannot get inode";
	err = rdro_get_data_from_ino(super, ino, data);
	if (err != 0)
		return err;
	data->inode = inode;
	return 0;
}



const char *rdro_get_data_from_path(const void *super, const char *path, rdro_data *data)
{
	rdro_t inode = -1;
	const char *err = rdro_get_inode_from_path(super, path, &inode);
	if (err != 0)
		return err;
	return rdro_get_data_from_inode(super, inode, data);
}
