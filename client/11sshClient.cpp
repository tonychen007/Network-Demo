#include "../common/common.h"

// test on ssh linux
void connectSSH(ssh_session& ssh, const char* hostname, int port, const char* user) {

    int verbosity = SSH_LOG_PROTOCOL;
#ifdef DEBUG
    ssh_options_set(ssh, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
#endif
    ssh_options_set(ssh, SSH_OPTIONS_HOST, hostname);
    ssh_options_set(ssh, SSH_OPTIONS_PORT, &port);
    ssh_options_set(ssh, SSH_OPTIONS_USER, user);

    int ret = ssh_connect(ssh);
    if (ret != SSH_OK) {
        fprintf(stderr, "ssh_connect() failed.\n%s\n", ssh_get_error(ssh));
        exit(-1);
    }

    printf("Connected to %s on port %d.\n", hostname, port);
    printf("Banner:\n%s\n", ssh_get_serverbanner(ssh));

    ssh_key key;
    if (ssh_get_server_publickey(ssh, &key) != SSH_OK) {
        fprintf(stderr, "ssh_get_server_publickey() failed.\n%s\n", ssh_get_error(ssh));
        exit(-1);
    }

    unsigned char* hash = 0;
    size_t hash_len = 0;
    printf("Host public key hash:\n"); ssh_print_hash(SSH_PUBLICKEY_HASH_SHA1, hash, hash_len);
    ssh_clean_pubkey_hash(&hash);
    ssh_key_free(key);

    printf("Checking ssh_session_is_known_server()\n");
    enum ssh_known_hosts_e known = ssh_session_is_known_server(ssh);
    switch (known) {
    case SSH_KNOWN_HOSTS_OK: printf("Host Known.\n"); break;
    case SSH_KNOWN_HOSTS_CHANGED: printf("Host Changed.\n"); break;
    case SSH_KNOWN_HOSTS_OTHER: printf("Host Other.\n"); break;
    case SSH_KNOWN_HOSTS_UNKNOWN: printf("Host Unknown.\n"); break;
    case SSH_KNOWN_HOSTS_NOT_FOUND: printf("No host file.\n"); break;
    case SSH_KNOWN_HOSTS_ERROR: printf("Host error. %s\n", ssh_get_error(ssh)); return;
    default: printf("Error. Known: %d\n", known); return;
    }

    if (known == SSH_KNOWN_HOSTS_CHANGED ||
        known == SSH_KNOWN_HOSTS_OTHER ||
        known == SSH_KNOWN_HOSTS_UNKNOWN ||
        known == SSH_KNOWN_HOSTS_NOT_FOUND) {
        printf("Do you want to accept and remember this host? Y/N\n");
        char answer[10];
        fgets(answer, sizeof(answer), stdin);
        if (answer[0] != 'Y' && answer[0] != 'y') {
            return;
        }

        ssh_session_update_known_hosts(ssh);
    }

    /*
    printf("Password: ");
    char password[128];
    fgets(password, sizeof(password), stdin);
    password[strlen(password) - 1] = 0;
    */

    if (ssh_userauth_password(ssh, 0, "1") != SSH_AUTH_SUCCESS)
    {
        fprintf(stderr, "ssh_userauth_password() failed.\n%s\n", ssh_get_error(ssh));
        return;
    }
    else {
        printf("Authentication successful!\n");
    }
}

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

void scp(ssh_session& ssh, const char* file) {

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