#include "common.h"

#include "sysinfo.h"

#if !defined(_WINDOWS)
#include <scsi/sg.h>
#endif

#define DEF_ALLOC_LEN 		252
#define VPD_UNIT_SERIAL_NUM 	0x80
#define INQUIRY_CMDLEN		6
#define INQUIRY_CMD 		0x12
#define SENSE_BUFF_LEN		32
#define DEF_TIMEOUT 		60000


static char* get_hdd_serial (const char* dev)
{
	int fd = open (dev, O_RDONLY | O_NONBLOCK);
	unsigned char inqCmdBlk[INQUIRY_CMDLEN] = {INQUIRY_CMD, 1, VPD_UNIT_SERIAL_NUM, (DEF_ALLOC_LEN >> 8) && 0xFF, DEF_ALLOC_LEN & 0xFF, 0};
	unsigned char buf[DEF_ALLOC_LEN];
	unsigned char sense_b[SENSE_BUFF_LEN];
	char* res = NULL;
	struct sg_io_hdr hdr;
	int len;
	char* p;

	if (fd < 0)
		return NULL;

	/* prepare scsi header */
	memset (&hdr, 0, sizeof (hdr));
	memset (&buf, 0, sizeof (buf));
	hdr.interface_id = 'S';
	hdr.dxfer_direction = SG_DXFER_FROM_DEV;
	hdr.dxfer_len = DEF_ALLOC_LEN;
	hdr.dxferp = buf;
	hdr.cmdp = inqCmdBlk;
	hdr.cmd_len = sizeof (inqCmdBlk);
	hdr.mx_sb_len = sizeof (sense_b);
	hdr.sbp = sense_b;
	hdr.timeout = DEF_TIMEOUT;

	memset (&sense_b, 0, sizeof (sense_b));

	if (ioctl (fd, SG_IO, &hdr) >= 0) {
		len = buf[3];
		p = buf+4;

		while (*p == ' ' && len > 0) {
			p++;
			len--;
		}

		if (len > 0) {
			res = calloc (sizeof (unsigned char), len + 1);
			memcpy (res, p, len);
		}
	}

	close (fd);
	return res;
}


int	PROFILE (const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
	/* obtain serial numbers of all disks visible by system */
	const char* dev = "/dev/sdd";
	char *serial, *serial_res = NULL;
	DIR* dir;
	struct dirent* dirent;
	int len;
	static char path_buf[1024];
	const char* path = "/dev/disk/by-id";
	int count = 0;
	char* res = NULL;

	/* enumerate all visible block devices */
	dir = opendir (path);

	if (!dir)
		return SYSINFO_RET_FAIL;

	while (dirent = readdir (dir)) {
		if (!strncmp (dirent->d_name, "scsi-", 5) && !strstr (dirent->d_name, "-part")) {
			snprintf (path_buf, sizeof (path_buf), "%s/%s", path, dirent->d_name);
			serial = get_hdd_serial (path_buf);
			if (serial) {
				count++;
				if (serial_res) {
					serial_res = (char*)realloc (serial_res, strlen (serial_res) + 1 + strlen (serial) + 1);
					strcat (serial_res, " ");
					strcat (serial_res, serial);
					free (serial);
				}
				else
					serial_res = serial;
			}

		}
	}

	closedir (dir);

	SET_UI64_RESULT(result, count);

	if (serial_res) {
		if (res) {
			res = (char*)realloc (res, strlen (res) + 1 + strlen (serial_res) + 16);
			strcat (res, "DisksSN: ");
			strcat (res, serial_res);
			strcat (res, "\n");
		}
		else {
			res = (char*)malloc (strlen (serial_res) + 16);
			snprintf (res, strlen (serial_res) + 16, "DisksSN: %s\n", serial_res);
		}
		free (serial_res);
	}

	if (res)
		SET_ERR_RESULT(result, res);

	return SYSINFO_RET_OK;
}
