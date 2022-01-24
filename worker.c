#define _GNU_SOURCE 
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "zmq.h"
#include <stdlib.h>
#include <sys/wait.h>

typedef struct MD{
	int port2, port3;
	char message[128];
	char text_string[128];
	char pattern_string[128];
	int key;
} MessageMD;

typedef struct node{
	int key, port2, port3, n2, n3;
} Node;

void find_pattern(char *string, char *pattern)
{
	int count = 0;
	int i, j, k;
	printf("%d Found:", getpid());
    for(i = 0; i < strlen(string); i++){
    	for(j = i, k = 0; k < strlen(pattern) && string[j] == pattern[k]; j++, k++);
    	if(k == strlen(pattern)){
    		count++;
    		printf(" %d", i);
    	}
    }
    if(!count) printf("0\n");
    else printf("\n");
}

void function(void *worker2, void *worker3, char *msg, int key, int global_key, int port2, int port3){
	MessageMD md1;
	md1.key = key;
	strcpy(md1.message, msg);
	md1.port2 = port2; md1.port3 = port3;
	zmq_msg_t req;
	zmq_msg_init_size(&req, sizeof(MessageMD));
	memcpy(zmq_msg_data(&req), &md1, sizeof(MessageMD));		
				
	if(key < global_key) zmq_msg_send(&req, worker2, 0);
	else zmq_msg_send(&req, worker3, 0);
		
	zmq_msg_close(&req);

	zmq_msg_t reply;
	zmq_msg_init(&reply);
	if(key < global_key) zmq_msg_recv(&reply, worker2, 0);
	else zmq_msg_recv(&reply, worker3, 0);	

	zmq_msg_close(&reply);
}

void function2(void *worker, char *msg, int key, int *n){ 
	MessageMD md1;
	md1.key = key;
	strcpy(md1.message, msg);
	zmq_msg_t req;
	zmq_msg_init_size(&req, sizeof(MessageMD));
	memcpy(zmq_msg_data(&req), &md1, sizeof(MessageMD));		
				
	zmq_msg_send(&req, worker, 0); 
	zmq_msg_close(&req);

	zmq_msg_t reply;
	zmq_msg_init(&reply);
	zmq_msg_recv(&reply, worker, 0);
	if(!strcmp((char*)zmq_msg_data(&reply),"remove\0")){
		(*n) = 0; 
	}	
	zmq_msg_close(&reply);
	zmq_msg_t req2;
	zmq_msg_init_size(&req2, strlen("..\0"));
	memcpy(zmq_msg_data(&req2), "..\0", strlen("..\0"));					
	zmq_msg_send(&req2, worker, 0); 
	zmq_msg_close(&req2);
	zmq_msg_t req3;
	zmq_msg_init(&req3);
	zmq_msg_recv(&req3, worker, 0);
	zmq_msg_close(&req3);
	if(!(*n)) wait(0);
}

void sendd(void *worker, char *msg, int n){
	zmq_msg_t req;
	zmq_msg_init_size(&req,n);
	memcpy(zmq_msg_data(&req), msg, n);
	zmq_msg_send(&req, worker, 0);
	zmq_msg_close(&req);

	zmq_msg_t req2;
	zmq_msg_init(&req2);
	zmq_msg_recv(&req2, worker, 0);
	zmq_msg_close(&req2);

	zmq_msg_t reply;
	zmq_msg_init_size(&reply, strlen("world"));
	memcpy(zmq_msg_data(&reply), "world", 5);
	zmq_msg_send(&reply, worker, 0);
	zmq_msg_close(&reply);
}


int main (int argc, char const *argv[]) 
{
	void* context = zmq_ctx_new();
	void* worker = zmq_socket(context, ZMQ_REP); 
	void* worker2 = zmq_socket(context, ZMQ_REQ);
	void* worker3 = zmq_socket(context, ZMQ_REQ);

	char *adress, *adress_, *adress__;
	int key, port2, port3, n2 = 0, n3 = 0;
	port2 = atoi(argv[2]);
	port3 = atoi(argv[3]);

    asprintf(&adress, "tcp://*:%s", argv[1]);    
	zmq_bind(worker, adress);

	zmq_msg_t request;
	zmq_msg_init(&request);
	zmq_msg_recv(&request, worker, 0);
	MessageMD *m = (MessageMD*) zmq_msg_data(&request);
	printf("\nNode Pid:%d K:%d created\n", getpid(), m->key); fflush(stdout);
	zmq_msg_close(&request);

	key = m->key;
	asprintf(&adress_, "tcp://localhost:%s", argv[2]);
    asprintf(&adress__, "tcp://localhost:%s", argv[3]);
    zmq_connect(worker2, adress_);
	zmq_connect(worker3, adress__);

	zmq_msg_t reply;
	zmq_msg_init_size(&reply, strlen("world"));
	memcpy(zmq_msg_data(&reply), "world", 5);
	zmq_msg_send(&reply, worker, 0);
	zmq_msg_close(&reply);
	
	for(;;){
		zmq_msg_t request;
		zmq_msg_init(&request);
		zmq_msg_recv(&request, worker, 0);
		MessageMD *md = (MessageMD*) zmq_msg_data(&request);
		char *msg = " ";
		zmq_msg_close(&request);

		if(!strcmp(md->message,"create\0")){
		 	if(n2 == 0 && md->key < key){
		  		n2 = 1;
		  		if(fork() == 0){
		  			sprintf(adress, "%d", port2);
		  			sprintf(adress_, "%d", md->port2);
		  			sprintf(adress__, "%d", md->port3);
		  			execl("./worker", "./worker", adress, adress_, adress__, NULL);
		  		}
		  		else function(worker2, worker3, "create\0", md->key, key, md->port2, md->port3); 
			}
			else if(n3 == 0 && md->key > key){
				n3 = 1;
				if(fork() == 0){
					sprintf(adress, "%d", port3);
		  			sprintf(adress_, "%d", md->port2);
		  			sprintf(adress__, "%d", md->port3);
		  			execl("./worker", "./worker", adress, adress_, adress__, NULL);
				}
				else function(worker2, worker3, "create\0", md->key, key, md->port2, md->port3); 
			}
			else function(worker2, worker3, "create\0", md->key, key, md->port2, md->port3);
		}
		else if(!strcmp(md->message,"remove\0")){
			if(md->key == key){
				sendd(worker, "remove\0", strlen("remove\0")+1);
				break;
			}
			if(md->key < key){
				sendd(worker,"hello\0", strlen("hello\0")+1);
				function2(worker2, "remove\0", md->key, &n2);
			}
			else if(md->key > key){
				sendd(worker,"hello\0", strlen("hello\0")+1);
				function2(worker3, "remove\0", md->key, &n3);
			}
		}
		else if(!strcmp(md->message,"ping\0")){
			if(md->key == key){
				zmq_msg_t reply;
				zmq_msg_init_size(&reply, strlen("ok\0"));
				memcpy(zmq_msg_data(&reply), "ok\0", strlen("ok\0"));
				zmq_msg_send(&reply, worker, 0);
				zmq_msg_close(&reply);
				continue;
			}
			else {
				MessageMD md1;
				md1.key = md->key;
				strcpy(md1.message, "ping\0");
				zmq_msg_t req;
				zmq_msg_init_size(&req, sizeof(MessageMD));
				memcpy(zmq_msg_data(&req), &md1, sizeof(MessageMD));
				if(md->key < key) zmq_msg_send(&req, worker2, 0); 
				else zmq_msg_send(&req, worker3, 0);
				zmq_msg_close(&req);

				zmq_msg_t reply;
				zmq_msg_init(&reply);
				if(md->key < key) zmq_msg_recv(&reply, worker2, 0);
				else zmq_msg_recv(&reply, worker3, 0);
				msg = (char*)zmq_msg_data(&reply);
				zmq_msg_close(&reply);
			}
		}
		else if(!strcmp(md->message,"exec\0")){
			if(md->key == key){
				find_pattern(md->text_string, md->pattern_string);
			}
			else{
				MessageMD md1;
				md1.key = md->key;
				strcpy(md1.message, "exec\0");
				strcpy(md1.text_string, md->text_string);
				strcpy(md1.pattern_string, md->pattern_string);

				zmq_msg_t req;
				zmq_msg_init_size(&req, sizeof(MessageMD));
				memcpy(zmq_msg_data(&req), &md1, sizeof(MessageMD));
				if(md->key < key) zmq_msg_send(&req, worker2, 0); 
				else zmq_msg_send(&req, worker3, 0); 
				zmq_msg_close(&req);

				zmq_msg_t reply;
				zmq_msg_init(&reply);
				if(md->key < key) zmq_msg_recv(&reply, worker2, 0);
				else zmq_msg_recv(&reply, worker3, 0);	
				msg = (char*)zmq_msg_data(&reply);
				zmq_msg_close(&reply);
			}
		}
		zmq_msg_t reply;
		zmq_msg_init_size(&reply, strlen(msg));
		memcpy(zmq_msg_data(&reply), msg, strlen(msg));
		zmq_msg_send(&reply, worker, 0);
		zmq_msg_close(&reply);
	}

	printf("K:%d Pid:%d finished work\n", key, getpid());
	zmq_close(worker2);
	zmq_close(worker3);
	zmq_close(worker);
	zmq_ctx_destroy(context);
	return 0;
}

