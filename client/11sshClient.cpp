#include "../common/common.h"

void commandSSH(ssh_session& ssh, const char* cmd) {
    int ret;
    int bys;
    char out[128];

    ssh_channel channel = ssh_channel_new(ssh);
    if (!channel)
        return;

    ret = ssh_channel_open_session(channel);
    if (ret != SSH_OK)
        return;
    
    ret = ssh_channel_request_exec(channel, cmd);
    if (ret != SSH_OK)
        return;
    
    while (!ssh_channel_is_eof(channel)) {
        bys = ssh_channel_read(channel, out, sizeof(out), 0);
        printf("%.*s", bys, out);
        memset(out, 0, sizeof(out));

        if (bys == 0) {
            break;
        }
        else if (bys == -1) {
            if (ssh_channel_is_eof(channel))
                break;
            else {
                ssh_channel_close(channel);
                ssh_channel_free(channel);
                break;
            }
        }
    }

    printf("\n");
    ret = ssh_channel_send_eof(channel);
    if (ret != SSH_OK) {
        fprintf(stderr, "send eof error\n");
    }
    ssh_channel_close(channel);
    ssh_channel_free(channel);
}

void testSSHClient() {
	const char* hostname = "192.168.232.129";
    int port = 22;
	const char* user = "root";
    const char* cmd = "ls -alh";
    //const char* cmd = "du -h /";

	printf("libssh version: %s\n", ssh_version(0));
    ssh_session ssh = ssh_new();
    connectSSH(ssh, hostname, port, user);
    commandSSH(ssh, cmd);
    
    ssh_disconnect(ssh);
    ssh_free(ssh);
}