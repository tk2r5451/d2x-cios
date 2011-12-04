/*
 * FFS plugin for Custom IOS.
 *
 * Copyright (C) 2009-2010 Waninkoko.
 * Copyright (C) 2011 davebaol.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>

#include "fat.h"
#include "fs_calls.h"
#include "fs_tools.h"
#include "ioctl.h"
#include "ipc.h"
#include "isfs.h"
#include "plugin.h"
#include "syscalls.h"
#include "types.h"

/* Global config */
struct fsConfig config = { 0, {'\0'}, 0 };


void __FS_PrepareFolders(void)
{
	static const char dirs[10][17] = {
		"/tmp",
		"/sys",
		"/ticket",
		"/ticket/00010001",
		"/ticket/00010005",
		"/title",
		"/title/00010000",
		"/title/00010001",
		"/title/00010004",
		"/title/00010005"
	};

	char fatpath[FAT_MAXPATH];
	s32 cnt;

	/* Create directories */
	for (cnt = 0; cnt < 10; cnt++) {
		/* Generate path */
		FS_GeneratePath(&dirs[cnt][0], fatpath);

		/* Delete "/tmp" */
		if (cnt == 0)
			FAT_DeleteDir(fatpath);

		/* Create directory */
		FAT_CreateDir(fatpath);
	}
}

s32 __FS_SetMode(u32 mode, char *path)
{
	/* FAT mode enabled */
	if (mode & (MODE_SDHC | MODE_USB)) {
		u32 ret;

		/* Initialize FAT */
		ret = FAT_Init();
		if (ret < 0)
			return ret;

		/* Set FS mode */
		config.mode = mode;

		/* Set nand path */
		strcpy(config.path, path);

		/* Set nand path length */
		config.pathlen = strlen(path);

		/* Prepare folders */
		__FS_PrepareFolders();
	}
	else {
		/* Set FS mode */
		config.mode = 0;

		/* Set nand path */
		config.path[0] = '\0';

		/* Set nand path length */
		config.pathlen = 0;
	}

	return 0;
}

s32 FS_Open(ipcmessage *message)
{
#ifdef DEBUG
	char *path = message->open.device;
	u32 mode = message->open.mode;

	FS_printf("FS_Open(): (path: %s, mode: %d)\n", path, mode);
#endif

	return -6;
}

s32 FS_Close(ipcmessage *message)
{
#ifdef DEBUG
	s32 fd = message->fd;

	FS_printf("FS_Close(): %d\n", fd);
#endif

	return -6;
}

s32 FS_Read(ipcmessage *message)
{
#ifdef DEBUG
	char *buffer = message->read.data;
	u32   len    = message->read.length;
	s32   fd     = message->fd;

	FS_printf("FS_Read(): %d (buffer: 0x%08x, len: %d\n", fd, (u32)buffer, len);
#endif

	return -6;
}

s32 FS_Write(ipcmessage *message)
{
#ifdef DEBUG
	char *buffer = message->write.data;
	u32   len    = message->write.length;
	s32   fd     = message->fd;

	FS_printf("FS_Write(): %d (buffer: 0x%08x, len: %d)\n", fd, (u32)buffer, len);
#endif

	return -6;
}

s32 FS_Seek(ipcmessage *message)
{
#ifdef DEBUG
	s32 fd     = message->fd;
	s32 where  = message->seek.offset;
	s32 whence = message->seek.origin;

	FS_printf("FS_Seek(): %d (where: %d, whence: %d)\n", fd, where, whence);
#endif
	
	return -6;
}

/*
 * NOTE: 
 * The 2nd parameter is used to determine if call the original handler or not. 
 */
s32 FS_Ioctl(ipcmessage *message, u32 *performed)
{
	static struct stats stats ATTRIBUTE_ALIGN(32);

	u32 *inbuf = message->ioctl.buffer_in;
	u32 *iobuf = message->ioctl.buffer_io;
	u32  iolen = message->ioctl.length_io;
	u32  cmd   = message->ioctl.command;

	s32 ret;

	/* Set flag */
	*performed = config.mode;

	/* Parse command */
	switch (cmd) {
	/** Create directory **/
	case IOCTL_ISFS_CREATEDIR: {
		fsattr *attr = (fsattr *)inbuf;

		FS_printf("FS_CreateDir(): %s\n", attr->filepath);

		/* Check path */
		ret = FS_CheckRealPath(attr->filepath);
		if (ret) {
			*performed = 0;
			break;
		}

		/* FAT mode */
		if (config.mode) {
			char fatpath[FAT_MAXPATH];

			/* Generate path */
			FS_GeneratePath(attr->filepath, fatpath);

			/* Create directory */
			return FAT_CreateDir(fatpath);
		}

		break;
	}

	/** Create file **/
	case IOCTL_ISFS_CREATEFILE: {
		fsattr *attr = (fsattr *)inbuf;

		FS_printf("FS_CreateFile(): %s\n", attr->filepath);

		/* Check path */
		ret = FS_CheckRealPath(attr->filepath);
		if (ret) {
			*performed = 0;
			break;
		}

		/* FAT mode */
		if (config.mode) {
			char fatpath[FAT_MAXPATH];

			/* Generate path */
			FS_GeneratePath(attr->filepath, fatpath);

			/* Create file */
			return FAT_CreateFile(fatpath); 
		}

		break;
	}

	/** Delete object **/
	case IOCTL_ISFS_DELETE: {
		char *filepath = (char *)inbuf;

		FS_printf("FS_Delete(): %s\n", filepath);

		/* Check path */
		ret = FS_CheckRealPath(filepath);
		if (ret) {
			*performed = 0;
			break;
		}

		/* FAT mode */
		if (config.mode) {
			char fatpath[FAT_MAXPATH];

			/* Generate path */
			FS_GeneratePath(filepath, fatpath);

			/* Delete */
			return FAT_Delete(fatpath); 
		}

		break;
	}

	/** Rename object **/
	case IOCTL_ISFS_RENAME: {
		fsrename *rename = (fsrename *)inbuf;

		FS_printf("FS_Rename(): %s -> %s\n", rename->filepathOld, rename->filepathNew);

		/* Check paths */
		ret  = FS_CheckRealPath(rename->filepathOld);
		ret |= FS_CheckRealPath(rename->filepathNew);

		if (ret) {
			*performed = 0;
			break;
		}

		/* FAT mode */
		if (config.mode) {
			char oldpath[FAT_MAXPATH];
			char newpath[FAT_MAXPATH];

//			struct stats stats;

			/* Generate paths */
			FS_GeneratePath(rename->filepathOld, oldpath);
			FS_GeneratePath(rename->filepathNew, newpath);

			/* Compare paths */
			if (strcmp(oldpath, newpath)) {
				/* Check new path */
				ret = FAT_GetStats(newpath, &stats);

				/* New path exists */
				if (ret >= 0) {
					/* Delete directory */
					if (stats.attrib & AM_DIR)
						FAT_DeleteDir(newpath);

					/* Delete */
					FAT_Delete(newpath);
				}

				/* Rename */
				return FAT_Rename(oldpath, newpath); 
			}

			/* Check path exists */
			return FAT_GetStats(oldpath, NULL);
		}

		break;
	}

	/** Get device stats **/
	case IOCTL_ISFS_GETSTATS: {
		FS_printf("FS_GetStats():\n");

		/* FAT mode */
		if (config.mode) {
			fsstats *stats = (fsstats *)iobuf;

			/* Check buffer length */
			if (iolen < 0x1C)
				return -1017;

			/* Clear buffer */
			memset(iobuf, 0, iolen);

			/* Fill stats */
			stats->block_size  = 0x4000;
			stats->free_blocks = 0x5DEC;
			stats->used_blocks = 0x1DD4;
			stats->unk3        = 0x10;
			stats->unk4        = 0x02F0;
			stats->free_inodes = 0x146B;
			stats->unk5        = 0x0394;

			/* Flush cache */
			os_sync_after_write(iobuf, iolen);

			return 0;
		}

		break;
	}

	/** Get file stats **/
	case IOCTL_ISFS_GETFILESTATS: {
		FS_printf("FS_GetFileStats(): %d\n", message->fd);

		/* Disable flag */
		*performed = 0;

		break;
	}

	/** Get attributes **/
	case IOCTL_ISFS_GETATTR: {
		char *path = (char *)inbuf;
		
		FS_printf("FS_GetAttributes(): %s\n", path);

		/* Check path */
		ret = FS_CheckRealPath(path);
		if (ret) {
			*performed = 0;
			break;
		}

		/* FAT mode */
		if (config.mode) {
			fsattr *attr = (fsattr *)iobuf;
			char    fatpath[FAT_MAXPATH];

			/* Generate path */
			FS_GeneratePath(path, fatpath);

			/* Check path */
			ret = FAT_GetStats(fatpath, NULL);
			if (ret < 0)
				return ret;

			/* Fake attributes */
			attr->owner_id   = FS_GetUID();
			attr->group_id   = FS_GetGID();
			attr->ownerperm  = ISFS_OPEN_RW;
			attr->groupperm  = ISFS_OPEN_RW;
			attr->otherperm  = ISFS_OPEN_RW;
			attr->attributes = 0;
		  
			/* Copy filepath */
			memcpy(attr->filepath, path, ISFS_MAXPATH);

			/* Flush cache */
			os_sync_after_write(iobuf, iolen);

			return 0;
		}

		break;
	}

	/** Set attributes **/
	case IOCTL_ISFS_SETATTR: {
		fsattr *attr = (fsattr *)inbuf;

		FS_printf("FS_SetAttributes(): %s\n", attr->filepath);

		/* Check path */
		ret = FS_CheckRealPath(attr->filepath);
		if (ret) {
			*performed = 0;
			break;
		}

		/* FAT mode */
		if (config.mode) {
			char fatpath[FAT_MAXPATH];

			/* Generate path */
			FS_GeneratePath(attr->filepath, fatpath);

			/* Check path exists, permission ignored */
			return FAT_GetStats(fatpath, NULL);
		}

		break;
	}

	/** Format **/
	case IOCTL_ISFS_FORMAT: {
		FS_printf("FS_Format():\n");

		/* FAT mode */
		if (config.mode) {
			/* Do nothing */
			return 0;
		}

		break;
	}

	/** Set FS mode **/
	case IOCTL_ISFS_SETMODE: {
		u32 mode = inbuf[0];

		FS_printf("FS_SetMode(): (mode: %d, path: /)\n", mode);

		/* Set flag */
		*performed = 1;
		
		return __FS_SetMode(mode, "");
	}

	default:
		FS_printf("FS_Ioctl(): default case reached cmd = %x\n", cmd);
		break;
	}

	/* Call handler */
	return -6;
}

/*
 * NOTE: 
 * The 2nd parameter is used to determine if call the original handler or not. 
 */
s32 FS_Ioctlv(ipcmessage *message, u32 *performed)
{
	ioctlv *vector = message->ioctlv.vector;
	u32     inlen  = message->ioctlv.num_in;
	u32     iolen  = message->ioctlv.num_io;
	u32     cmd    = message->ioctlv.command;

	s32 ret;

	/* Set flag */
	*performed = config.mode;

	/* Parse command */
	switch (cmd) {
	/** Read directory **/
	case IOCTL_ISFS_READDIR: {
		char *dirpath = (char *)vector[0].data;

		FS_printf("FS_Readdir(): (path: %s, iolen: %d)\n", dirpath, iolen);

		/* Check path */
		ret = FS_CheckRealPath(dirpath);
		if (ret) {
			*performed = 0;
			break;
		}

		/* FAT mode */
		if (config.mode) {
			char *outbuf = NULL;
			u32  *outlen = NULL;
			u32   buflen = 0;
			
			char fatpath[FAT_MAXPATH];
			u32  entries;

			/* Set pointers/values */
			if (iolen > 1) {
				entries = *(u32 *)vector[1].data;
				outbuf  = (char *)vector[2].data;
				outlen  =  (u32 *)vector[3].data;
				buflen  =         vector[2].len;
			} else
				outlen  =  (u32 *)vector[1].data;

			/* Generate path */
			FS_GeneratePath(dirpath, fatpath);

			/* Read directory */
			ret = FAT_ReadDir(fatpath, outbuf, &entries);
			if (ret >= 0) {
				*outlen = entries;
				os_sync_after_write(outlen, sizeof(u32));
			}

			/* Flush cache */
			if (outbuf)
				os_sync_after_write(outbuf, buflen);

			return ret;
		}

		break;
	}

	/** Get device usage **/
	case IOCTL_ISFS_GETUSAGE: {
		char *dirpath = (char *)vector[0].data;

		FS_printf("FS_GetUsage(): %s\n", dirpath);

		/* Check path */
		ret = FS_CheckRealPath(dirpath);
		if (ret) {
			*performed = 0;
			break;
		}

		/* FAT mode */
		if (config.mode) {
			char fatpath[FAT_MAXPATH];
			char *fakepath = NULL;

			u32 *blocks = (u32 *)vector[1].data;
			u32 *inodes = (u32 *)vector[2].data;

			ret     = 0;
			*blocks = 0;
			*inodes = 1;        // empty folders return a file count of 1

			/* Set fake path to speed up access for huge emulated nand */
//			if (!FS_MatchPath(dirpath, "/title/0001000#/########", 0) && !FS_MatchPath(dirpath, "/ticket/0001000#", 0))
//				fakepath = "/ticket";
			if (!strcmp(dirpath, "/") || !strcmp(dirpath, "/title") || FS_MatchPath(dirpath, "/title/0001000#", 1))
				fakepath = "/ticket";
			
			/* Generate path */
			FS_GeneratePath(dirpath, fatpath);

			if (fakepath) {
				/* Check path */
				ret = FAT_GetStats(fatpath, NULL);
				if (ret >= 0) {

					FS_printf("FS_GetUsage(): Fake path = %s\n", fakepath);

					/* Generate fake path */
					FS_GeneratePath(fakepath, fatpath);
				}  
			}  

			if (ret >= 0) {
				/* Get usage */
				ret = FAT_GetUsage(fatpath, blocks, inodes);
			}  

			/* Flush cache */
			os_sync_after_write(blocks, sizeof(u32));
			os_sync_after_write(inodes, sizeof(u32));

			return ret;
		}

		break;
	}

	/** Set FS mode **/
	case IOCTL_ISFS_SETMODE: {
		u32  mode  = *(u32 *)vector[0].data;
		char *path = "";

		/* Get path */
		if (inlen > 1)
			path = (char *)vector[1].data;

		FS_printf("FS_SetMode(): (mode: %d, path: %s)\n", mode, path);

		/* Set flag */
		*performed = 1;
		
		return __FS_SetMode(mode, path);
	}

	/** Get FS mode **/
	case IOCTL_ISFS_GETMODE: {
		u32  *mode     = (u32 *) vector[0].data;
		u32   mode_len = (u32)   vector[0].len;
		char *path     = (char *)vector[1].data;
		u32   path_len = (u32)   vector[1].len;

		FS_printf("FS_GetMode():\n");

		/* Copy config */
		*mode = config.mode;
		memcpy(path, config.path, path_len);

		/* Flush cache */
		os_sync_after_write(mode, mode_len);
		os_sync_after_write(path, path_len);

		/* Set flag */
		*performed = 1;
		
		return 0;
	}

	default:
		FS_printf("FS_Ioctlv(): default case reached cmd = %x\n", cmd);
		break;
	}

	/* Call handler */
	return -6;
}


#ifdef DEBUG
s32 FS_Exit(s32 ret)
{
	FS_printf("FS returned: %d\n", ret);

	return ret;
}
#endif
