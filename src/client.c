#include <stdio.h>
#include <stdlib.h>
#include <roxml.h>
#include <string.h>
#include <sys/types.h>    
#include <sys/socket.h>
#include <libubox/usock.h>

#include "../include/netconfd.h"
#include "client.h"
#include "message.h"

void display_capabilities(struct connection_client *c){
	int i = 0;
	printf("\n---------capabilities---------\n");
	for (i = 0; i < c->num_capability; i++){
		printf("%s\n", c->capabilities[i]);
	}
}


int analyze_message_hello(char *xml_in, struct connection_client *c){

	int num_nodes = 0;
	int rc = -1;
	node_t **nodes;
	node_t *root = roxml_load_buf(xml_in);

	if (!root) goto exit;

	node_t *hello = roxml_get_nodes(root, ROXML_ELM_NODE, "hello", 0);

	if (!hello) goto exit;

	node_t *session_id = roxml_get_nodes(hello, ROXML_ELM_NODE, "session-id", 0);

	if (!session_id) goto exit;
	c->session_id = atoi(roxml_get_content(session_id, NULL, 0, NULL));

	nodes = roxml_xpath(root, "//capabilities/capability", &num_nodes);
	c->num_capability = 0;
	for (int i = 0; i < num_nodes; i++){
		if (!nodes[i]) continue;

		char *value = roxml_get_content(nodes[i], NULL, 0, NULL);
		strncpy(c->capabilities[i], value, strlen(value)+1);
		c->num_capability++;
	}

	rc = 0;

exit:
	roxml_release(RELEASE_ALL);
	roxml_close(root);

	return rc;
}

/*
	Connect to server and return the socket fd
 */
struct connection_client *client_connect(void){

	struct connection_client *c;

	char recv_buf[1024] = {0};
	char *hello = NULL;

	int rc;

	int type = USOCK_TCP  | USOCK_NOCLOEXEC | USOCK_IPV4ONLY;
	const char *host = "10.10.10.143";
	const char *service = "1831";
	int c_fd = usock(type, host, service);    /* create a linker socket*/
	if (c_fd < 0) {
		perror("usock");
		return 0;
	}

	c=(struct connection_client *)malloc(sizeof(struct connection_client));
	c->sock_fd = c_fd;

	/* Create Hello message and send */
	node_t *root = roxml_load_buf(XML_NETCONF_HELLO_C);
	if (!root)
	{
		ERROR("unable to load 'netconf hello' message template\n");
		return 0;
	}

	node_t *n_hello = roxml_get_chld(root, NULL, 0);

	if (!n_hello)
	{
		ERROR("unable to parse 'netconf hello' message template\n");
		return 0;
	}

	roxml_commit_changes(root, NULL, &hello, 0);	

	strcat(hello, XML_NETCONF_BASE_1_0_END);
	send(c_fd, hello, strlen(hello)+1, 0);

	/* recv Hello message from server */
	recv(c_fd, recv_buf, 1024, 0);
	printf("recv:%s\n", recv_buf);
	rc = analyze_message_hello(recv_buf, c);
	c->stream = NONE;
	if (rc != 0){
		ERROR("analyze message hello error!\n");
	}

	return c;
}

void get(struct connection_client *c, char *filter){

	char *get = NULL;
	char recv_buf[1024] = {0};
	char id[3] = {0};


	/* Create get message and send */
	node_t *root = roxml_add_node(NULL, 0, ROXML_PI_NODE, "xml", "version=\"1.0\" encoding=\"UTF-8\"");
	if (!root)
	{
		ERROR("unable to add 'netconf get' message template\n");
		return;
	}
	node_t *rpc = roxml_add_node(root, 0, ROXML_ELM_NODE, "rpc", NULL);

	sprintf(id, "%d", c->session_id);
	roxml_add_node(rpc, 0, ROXML_ATTR_NODE, "session-id", id);
	roxml_add_node(rpc, 0, ROXML_ATTR_NODE, "xmlns", "urn:ietf:params:xml:ns:netconf:base:1.0");
	node_t *get_n = roxml_add_node(rpc, 0, ROXML_ELM_NODE, "get", NULL);
	if (filter != NULL){
		/* filter */
		node_t *filter_n = roxml_add_node(get_n, 0, ROXML_ELM_NODE, "filter", NULL);
		roxml_add_node(filter_n, 0, ROXML_ATTR_NODE, "type", "subtree");
		node_t *top = roxml_add_node(filter_n, 0, ROXML_ELM_NODE, "top", NULL);
		roxml_add_node(top, 0, ROXML_ATTR_NODE, "xmlns", "urn:ietf:params:xml:ns:yang:ietf-interfaces");


	}
	roxml_commit_changes(root, NULL, &get, 0);	
	roxml_close(root);
	
	strcat(get, XML_NETCONF_BASE_1_0_END);
//	printf("get:%s\n", get);
	send(c->sock_fd, get, strlen(get)+1, 0);

	/* recv rpc-reply from server */
	recv(c->sock_fd, recv_buf, 1024, 0);
	printf("recv:%s\n", recv_buf);
}

void get_config(struct connection_client *c, char *source, char *filter){
	char *get_config = NULL;
	char recv_buf[1024] = {0};
	char id[3] = {0};


	/* Create get_config message and send */
	node_t *root = roxml_add_node(NULL, 0, ROXML_PI_NODE, "xml", "version=\"1.0\" encoding=\"UTF-8\"");
	if (!root)
	{
		ERROR("unable to add 'netconf get_config' message template\n");
		return;
	}
	node_t *rpc = roxml_add_node(root, 0, ROXML_ELM_NODE, "rpc", NULL);

	sprintf(id, "%d", c->session_id);
	roxml_add_node(rpc, 0, ROXML_ATTR_NODE, "session-id", id);
	roxml_add_node(rpc, 0, ROXML_ATTR_NODE, "xmlns", "urn:ietf:params:xml:ns:netconf:base:1.0");

	/* get-config */
	node_t *rpc_get_config = roxml_add_node(rpc, 0, ROXML_ELM_NODE, "get-config", NULL);
	node_t *source_n = roxml_add_node(rpc_get_config, 0, ROXML_ELM_NODE, "source", NULL);
	roxml_add_node(source_n, 0, ROXML_ELM_NODE, source, NULL);

	/* filter */
	if (filter != NULL){
		/* filter */
	}
	roxml_commit_changes(root, NULL, &get_config, 0);	
	roxml_close(root);
	
	strcat(get_config, XML_NETCONF_BASE_1_0_END);
//	printf("get_config:%s\n", get_config);
	send(c->sock_fd, get_config, strlen(get_config)+1, 0);

	/* recv rpc-reply from server */
	recv(c->sock_fd, recv_buf, 1024, 0);
	printf("recv:%s\n", recv_buf);
}

void edit_config(struct connection_client *c, char *target, char *config){
	char *edit_config = NULL;
	char recv_buf[1024] = {0};
	char id[3] = {0};


	/* Create edit_config message and send */
	node_t *root = roxml_add_node(NULL, 0, ROXML_PI_NODE, "xml", "version=\"1.0\" encoding=\"UTF-8\"");
	if (!root)
	{
		ERROR("unable to add 'netconf edit_config' message template\n");
		return;
	}
	node_t *rpc = roxml_add_node(root, 0, ROXML_ELM_NODE, "rpc", NULL);

	sprintf(id, "%d", c->session_id);
	roxml_add_node(rpc, 0, ROXML_ATTR_NODE, "session-id", id);
	roxml_add_node(rpc, 0, ROXML_ATTR_NODE, "xmlns", "urn:ietf:params:xml:ns:netconf:base:1.0");

	/* edit-config */
	node_t *rpc_edit_config = roxml_add_node(rpc, 0, ROXML_ELM_NODE, "edit-config", NULL);
	// target
	node_t *target_n = roxml_add_node(rpc_edit_config, 0, ROXML_ELM_NODE, "target", NULL);
	roxml_add_node(target_n, 0, ROXML_ELM_NODE, target, NULL);
	// config
	node_t *config_n = roxml_add_node(rpc_edit_config, 0, ROXML_ELM_NODE, "config", NULL);


	roxml_commit_changes(root, NULL, &edit_config, 0);	
	roxml_close(root);
	
	strcat(edit_config, XML_NETCONF_BASE_1_0_END);
//	printf("edit_config:%s\n", edit_config);
	send(c->sock_fd, edit_config, strlen(edit_config)+1, 0);

	/* recv rpc-reply from server */
	recv(c->sock_fd, recv_buf, 1024, 0);
	printf("recv:%s\n", recv_buf);
}

void copy_config(struct connection_client *c, char *target, char *source){
	char *copy_config = NULL;
	char recv_buf[1024] = {0};
	char id[3] = {0};


	/* Create copy_config message and send */
	node_t *root = roxml_add_node(NULL, 0, ROXML_PI_NODE, "xml", "version=\"1.0\" encoding=\"UTF-8\"");
	if (!root)
	{
		ERROR("unable to add 'netconf copy_config' message template\n");
		return;
	}
	node_t *rpc = roxml_add_node(root, 0, ROXML_ELM_NODE, "rpc", NULL);

	sprintf(id, "%d", c->session_id);
	roxml_add_node(rpc, 0, ROXML_ATTR_NODE, "session-id", id);
	roxml_add_node(rpc, 0, ROXML_ATTR_NODE, "xmlns", "urn:ietf:params:xml:ns:netconf:base:1.0");

	/* edit-config */
	node_t *rpc_copy_config = roxml_add_node(rpc, 0, ROXML_ELM_NODE, "copy-config", NULL);
	// target
	node_t *target_n = roxml_add_node(rpc_copy_config, 0, ROXML_ELM_NODE, "target", NULL);
	roxml_add_node(target_n, 0, ROXML_ELM_NODE, target, NULL);
	// source
	node_t *source_n = roxml_add_node(rpc_copy_config, 0, ROXML_ELM_NODE, "source", NULL);
	roxml_add_node(source_n, 0, ROXML_ELM_NODE, source, NULL);


	roxml_commit_changes(root, NULL, &copy_config, 0);	
	roxml_close(root);
	
	strcat(copy_config, XML_NETCONF_BASE_1_0_END);
//	printf("copy_config:%s\n", copy_config);
	send(c->sock_fd, copy_config, strlen(copy_config)+1, 0);

	/* recv rpc-reply from server */
	recv(c->sock_fd, recv_buf, 1024, 0);
	printf("recv:%s\n", recv_buf);
}

void delete_config(struct connection_client *c, char *target){
	char *delete_config = NULL;
	char recv_buf[1024] = {0};
	char id[3] = {0};


	/* Create delete_config message and send */
	node_t *root = roxml_add_node(NULL, 0, ROXML_PI_NODE, "xml", "version=\"1.0\" encoding=\"UTF-8\"");
	if (!root)
	{
		ERROR("unable to add 'netconf delete_config' message template\n");
		return;
	}
	node_t *rpc = roxml_add_node(root, 0, ROXML_ELM_NODE, "rpc", NULL);

	sprintf(id, "%d", c->session_id);
	roxml_add_node(rpc, 0, ROXML_ATTR_NODE, "session-id", id);
	roxml_add_node(rpc, 0, ROXML_ATTR_NODE, "xmlns", "urn:ietf:params:xml:ns:netconf:base:1.0");

	/* edit-config */
	node_t *rpc_delete_config = roxml_add_node(rpc, 0, ROXML_ELM_NODE, "delete-config", NULL);
	// target
	node_t *target_n = roxml_add_node(rpc_delete_config, 0, ROXML_ELM_NODE, "target", NULL);
	roxml_add_node(target_n, 0, ROXML_ELM_NODE, target, NULL);

	roxml_commit_changes(root, NULL, &delete_config, 0);	
	roxml_close(root);
	
	strcat(delete_config, XML_NETCONF_BASE_1_0_END);
//	printf("delete_config:%s\n", delete_config);
	send(c->sock_fd, delete_config, strlen(delete_config)+1, 0);

	/* recv rpc-reply from server */
	recv(c->sock_fd, recv_buf, 1024, 0);
	printf("recv:%s\n", recv_buf);
}

void lock(struct connection_client *c, char *target){
	char *lock = NULL;
	char recv_buf[1024] = {0};
	char id[3] = {0};


	/* Create lock message and send */
	node_t *root = roxml_add_node(NULL, 0, ROXML_PI_NODE, "xml", "version=\"1.0\" encoding=\"UTF-8\"");
	if (!root)
	{
		ERROR("unable to add 'netconf lock' message template\n");
		return;
	}
	node_t *rpc = roxml_add_node(root, 0, ROXML_ELM_NODE, "rpc", NULL);

	sprintf(id, "%d", c->session_id);
	roxml_add_node(rpc, 0, ROXML_ATTR_NODE, "session-id", id);
	roxml_add_node(rpc, 0, ROXML_ATTR_NODE, "xmlns", "urn:ietf:params:xml:ns:netconf:base:1.0");

	/* edit-config */
	node_t *rpc_lock = roxml_add_node(rpc, 0, ROXML_ELM_NODE, "lock", NULL);
	// target
	node_t *target_n = roxml_add_node(rpc_lock, 0, ROXML_ELM_NODE, "target", NULL);
	roxml_add_node(target_n, 0, ROXML_ELM_NODE, target, NULL);

	roxml_commit_changes(root, NULL, &lock, 0);	
	roxml_close(root);
	
	strcat(lock, XML_NETCONF_BASE_1_0_END);
//	printf("lock:%s\n", lock);
	send(c->sock_fd, lock, strlen(lock)+1, 0);

	/* recv rpc-reply from server */
	recv(c->sock_fd, recv_buf, 1024, 0);
	printf("recv:%s\n", recv_buf);
}

void unlock(struct connection_client *c, char *target){
	char *unlock = NULL;
	char recv_buf[1024] = {0};
	char id[3] = {0};


	/* Create unlock message and send */
	node_t *root = roxml_add_node(NULL, 0, ROXML_PI_NODE, "xml", "version=\"1.0\" encoding=\"UTF-8\"");
	if (!root)
	{
		ERROR("unable to add 'netconf unlock' message template\n");
		return;
	}
	node_t *rpc = roxml_add_node(root, 0, ROXML_ELM_NODE, "rpc", NULL);

	sprintf(id, "%d", c->session_id);
	roxml_add_node(rpc, 0, ROXML_ATTR_NODE, "session-id", id);
	roxml_add_node(rpc, 0, ROXML_ATTR_NODE, "xmlns", "urn:ietf:params:xml:ns:netconf:base:1.0");

	/* edit-config */
	node_t *rpc_unlock = roxml_add_node(rpc, 0, ROXML_ELM_NODE, "unlock", NULL);
	// target
	node_t *target_n = roxml_add_node(rpc_unlock, 0, ROXML_ELM_NODE, "target", NULL);
	roxml_add_node(target_n, 0, ROXML_ELM_NODE, target, NULL);

	roxml_commit_changes(root, NULL, &unlock, 0);	
	roxml_close(root);
	
	strcat(unlock, XML_NETCONF_BASE_1_0_END);
//	printf("unlock:%s\n", unlock);
	send(c->sock_fd, unlock, strlen(unlock)+1, 0);

	/* recv rpc-reply from server */
	recv(c->sock_fd, recv_buf, 1024, 0);
	printf("recv:%s\n", recv_buf);
}

void close_session(struct connection_client *c){
	char *close_sess = NULL;
	char id[3] = {0};


	/* Create close_sess message and send */
	node_t *root = roxml_add_node(NULL, 0, ROXML_PI_NODE, "xml", "version=\"1.0\" encoding=\"UTF-8\"");
	if (!root)
	{
		ERROR("unable to add 'netconf close_sess' message template\n");
		return;
	}
	node_t *rpc = roxml_add_node(root, 0, ROXML_ELM_NODE, "rpc", NULL);

	sprintf(id, "%d", c->session_id);
	roxml_add_node(rpc, 0, ROXML_ATTR_NODE, "session-id", id);
	roxml_add_node(rpc, 0, ROXML_ATTR_NODE, "xmlns", "urn:ietf:params:xml:ns:netconf:base:1.0");

	/* close_session */
	roxml_add_node(rpc, 0, ROXML_ELM_NODE, "close-session", NULL);

	roxml_commit_changes(root, NULL, &close_sess, 0);	
	roxml_close(root);
	
	strcat(close_sess, XML_NETCONF_BASE_1_0_END);
	send(c->sock_fd, close_sess, strlen(close_sess)+1, 0);
	return;
}


void kill_session(struct connection_client *c){
	char *kill_sess = NULL;
	char id[3] = {0};


	/* Create kill_sess message and send */
	node_t *root = roxml_add_node(NULL, 0, ROXML_PI_NODE, "xml", "version=\"1.0\" encoding=\"UTF-8\"");
	if (!root)
	{
		ERROR("unable to add 'netconf kill_sess' message template\n");
		return;
	}
	node_t *rpc = roxml_add_node(root, 0, ROXML_ELM_NODE, "rpc", NULL);

	sprintf(id, "%d", c->session_id);
	roxml_add_node(rpc, 0, ROXML_ATTR_NODE, "session-id", id);
	roxml_add_node(rpc, 0, ROXML_ATTR_NODE, "xmlns", "urn:ietf:params:xml:ns:netconf:base:1.0");

	/* kill_session */
	roxml_add_node(rpc, 0, ROXML_ELM_NODE, "kill-session", NULL);

	roxml_commit_changes(root, NULL, &kill_sess, 0);	
	roxml_close(root);
	
	strcat(kill_sess, XML_NETCONF_BASE_1_0_END);
	send(c->sock_fd, kill_sess, strlen(kill_sess)+1, 0);
	return;
}

void stream(struct connection_client *c, char *stream){
	/* use filter get streams by method_get */
	get(c, stream);
}

void create_subscription(struct connection_client *c, char *stream){
	char *create_sub = NULL;
	char id[3] = {0};
	char recv_buf[1024] = {0};

	/* Create create_subscription message and send */
	node_t *root = roxml_add_node(NULL, 0, ROXML_PI_NODE, "xml", "version=\"1.0\" encoding=\"UTF-8\"");
	if (!root)
	{
		ERROR("unable to add 'netconf create_subscription' message template\n");
		return;
	}
	node_t *rpc = roxml_add_node(root, 0, ROXML_ELM_NODE, "rpc", NULL);

	sprintf(id, "%d", c->session_id);
	roxml_add_node(rpc, 0, ROXML_ATTR_NODE, "session-id", id);
	roxml_add_node(rpc, 0, ROXML_ATTR_NODE, "xmlns", "urn:ietf:params:xml:ns:netconf:base:1.0");

	/* create_subscription */
	node_t *create = roxml_add_node(rpc, 0, ROXML_ELM_NODE, "create-subscription", NULL);
	roxml_add_node(create, 0, ROXML_ATTR_NODE, "xmlns", "urn:ietf:params:xml:ns:netconf:notification:1.0");

	/* stream */
	node_t *stream_n = roxml_add_node(create, 0, ROXML_ELM_NODE, "stream", NULL);
	roxml_add_node(stream_n, 0, ROXML_ELM_NODE, stream, NULL);


	roxml_commit_changes(root, NULL, &create_sub, 0);	
	roxml_close(root);
	
	strcat(create_sub, XML_NETCONF_BASE_1_0_END);
	send(c->sock_fd, create_sub, strlen(create_sub)+1, 0);

	/* recv rpc-reply from server */
	recv(c->sock_fd, recv_buf, 1024, 0);
	printf("recv:%s\n", recv_buf);
}