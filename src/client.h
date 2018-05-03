#ifndef __NETCONFD_CLIENT_H__
#define __NETCONFD_CLIENT_H__

struct connection_client
{
	int sock_fd;
	char capabilities[5][64];
	int session_id;
	int stream;
	int num_capability;
};

enum notify_stream
{
	NONE,
	STREAM_NETCONF,
	STREAM_SNMP,
	_STREAM_MAX,
};

int analyze_message_hello(char *xml_in, struct connection_client *c);
struct connection_client *client_connect(void);
void display_capabilities(struct connection_client *c);

void get(struct connection_client *c, char *filter);
void get_config(struct connection_client *c, char *source, char *filter);
void edit_config(struct connection_client *c, char *target, char *config);
void copy_config(struct connection_client *c, char *target, char *source);
void delete_config(struct connection_client *c, char *target);
void lock(struct connection_client *c, char *target);
void unlock(struct connection_client *c, char *target);
void close_session(struct connection_client *c);
void kill_session(struct connection_client *c);
void stream(struct connection_client *c, char *stream);
void create_subscription(struct connection_client *c, char *stream);

#endif /* __FREENETCONFD_CONNECTION_H__ */