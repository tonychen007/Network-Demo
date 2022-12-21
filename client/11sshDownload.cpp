#include "../common/common.h"

void scp(ssh_session& ssh, const char* srcfile, const char* dstpath){
	int bys = 0;
	int total = 0;
	char dstfile[MAX_PATH] = { 0 };
	FILE* f;

	ssh_scp scp = ssh_scp_new(ssh, SSH_SCP_READ, srcfile);
	if (!scp)
		return;

	if (ssh_scp_init(scp) != SSH_OK) {
		fprintf(stderr, "ssh_scp_init() failed.\n%s\n", ssh_get_error(ssh));
		return;
	}

	if (ssh_scp_pull_request(scp) != SSH_SCP_REQUEST_NEWFILE) {
		fprintf(stderr, "ssh_scp_pull_request() failed.\n%s\n", ssh_get_error(ssh));
		return;
	}

	int fsize = ssh_scp_request_get_size(scp);
	char* fname = _strdup(ssh_scp_request_get_filename(scp));
	int fpermission = ssh_scp_request_get_permissions(scp);

	printf("Downloading file %s (%d bytes, permissions 0%o\n", fname, fsize, fpermission);
	free(fname);

	strcat_s(dstfile, dstpath);
	strcat_s(dstfile, "\\");
	const char* basefile = strrchr(srcfile, '/');
	basefile++;
	strcat_s(dstfile, basefile);

	// read large data by chunk
	int chunk = 8192;
	char* buffer = (char*)malloc(chunk);
	fopen_s(&f, dstfile, "wb");

	printf("Downloading");
	while (1) {
		ssh_scp_accept_request(scp);
		bys = ssh_scp_read(scp, buffer, chunk);
		if (bys == SSH_ERROR) {
			fprintf(stderr, "ssh_scp_read() failed.\n%s\n", ssh_get_error(ssh));
			return;
		}
		total += bys;		
		fwrite(buffer, sizeof(char), bys, f);
		
		static int i = fsize / 10;
		if (total >= i) {
			printf(".");
			i += fsize / 10;
		}

		if (total >= fsize)
			break;
	}
	fclose(f);
	printf("[ok]\n");

	if (ssh_scp_pull_request(scp) != SSH_SCP_REQUEST_EOF) {
		fprintf(stderr, "ssh_scp_pull_request() unexpected.\n%s\n", ssh_get_error(ssh));
		return;
	}

	ssh_scp_close(scp);
	ssh_scp_free(scp);
}

void testSSHDownload() {
	const char* hostname = "192.168.232.129";
	int port = 22;
	const char* user = "root";
	const char* filename = "/root/111.tar"; // 1.1G
	const char* outfile = "z:/";

	ssh_session ssh = ssh_new();
	connectSSH(ssh, hostname, port, user);
	scp(ssh, filename, outfile);

	ssh_disconnect(ssh);
	ssh_free(ssh);
}