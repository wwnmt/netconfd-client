#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  

#include "client.h"

int main()
{
	struct connection_client *c;
	char recv_buf[1024] = {0};
	c = client_connect();
	if (c == NULL ){
		printf("connect failed!\n");
	}

	get(c, NULL);
	get_config(c, "running", NULL);
	edit_config(c, "running", NULL);
	copy_config(c, "running", "candicate");
	delete_config(c, "running");
	lock(c, "running");
	unlock(c, "running");

	display_capabilities(c);
	create_subscription(c, "netconf");
	c->stream = 1;
	int i;
	for (i = 0; i < 3; i++){
		recv(c->sock_fd, recv_buf, 1024, 0);
		printf("len : %d\n#recv :%s\n", strlen(recv_buf), recv_buf);
		bzero(recv_buf, 1024);
	}
	close_session(c);
	// kill_session(c);
	return 0;
}  