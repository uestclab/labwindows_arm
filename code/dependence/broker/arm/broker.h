#ifndef	__BROKER__
#define	__BROKER__


typedef int (*recv_cb)(char* buf, int buf_len, char *from, void* arg); 

typedef int (*exception_cb)(void* arg); 

typedef int (*log_print)(const char *format,...);

struct waite_ack_s;

struct msg_priv;


struct dev_priv;

struct g_broker_s;

void dev_set_log(log_print log_func);
int init_broker(char *src, char *host, int port, exception_cb excep_cb, void *arg);
int close_broker(void);
int dev_send(char *buf, int buf_len, char *event);
int dev_transfer(char *buf, int buf_len, char **out_buf, int *out_buf_len, char *dst, int to_sec);
int register_callback(char *src, recv_cb cb, char *type);
int unregister_callback(char *src, char *type);
int dev_send_respond(char *buf, int buf_len, char *from);


static inline char* is_pub(char *topic)
{
	return strstr(topic, "/pub/");
}

#endif
