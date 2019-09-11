#ifndef __SFTP_WRITE_NONBLOCK_H__
#define __SFTP_WRITE_NONBLOCK_H__

int sw_sftp_write_nonblock(int argc, char *ip, char *servername, char *serverpswd, char *localpath, char *remotepath, char *type, int *uploadsize);

#endif
