#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "zmq.h"
#include <stdlib.h>
#include "bal_tree.h"
#include <sys/wait.h>
#include <time.h>

typedef struct MD{
	int port2, port3;
	char message[128];
	char text_string[128];
	char pattern_string[128];
	int key;
} MessageMD;


void function1(void *respond, int key, int port2, int port3, char *command){
	MessageMD md;
	md.key = key; md.port2 = port2; md.port3 = port3;
	strcpy(md.message,command);
	zmq_msg_t req;
	zmq_msg_init_size(&req, sizeof(MessageMD));
	memcpy(zmq_msg_data(&req), &md, sizeof(MessageMD));
		
	zmq_msg_send(&req, respond, 0);
	zmq_msg_close(&req);
		
	zmq_msg_t reply;
	zmq_msg_init(&reply);
	zmq_msg_recv(&reply, respond, 0);
	zmq_msg_close(&reply);
}

void function2(void *respond, int key, char *command){
	int n = 0;
	MessageMD md;
	md.key = key; 
	strcpy(md.message, command);
	zmq_msg_t req;
	zmq_msg_init_size(&req, sizeof(MessageMD));
	memcpy(zmq_msg_data(&req), &md, sizeof(MessageMD));						
	zmq_msg_send(&req, respond, 0); 
	zmq_msg_close(&req);

	zmq_msg_t reply;
	zmq_msg_init(&reply);
	zmq_msg_recv(&reply, respond, 0);
	if(!strcmp((char*)zmq_msg_data(&reply),"remove\0")) n = 1;
	zmq_msg_close(&reply);

	zmq_msg_t req2;
	zmq_msg_init_size(&req2, strlen("..\0"));
	memcpy(zmq_msg_data(&req2), "..\0", strlen("..\0"));					
	zmq_msg_send(&req2, respond, 0); 
	zmq_msg_close(&req2);

	zmq_msg_t req3;
	zmq_msg_init(&req3);
	zmq_msg_recv(&req3, respond, 0);
	zmq_msg_close(&req3);

	if(n) {
		wait(0);
	}
}

void function3(void *respond, int key, char *command, char *text, char *pattern){
	MessageMD md;
	md.key = key;
	strcpy(md.message, command);
	strcpy(md.text_string, text);
	strcpy(md.pattern_string, pattern);

	zmq_msg_t req;
	zmq_msg_init_size(&req, sizeof(MessageMD));
	memcpy(zmq_msg_data(&req), &md, sizeof(MessageMD));
    zmq_msg_send(&req, respond, 0); 
	zmq_msg_close(&req);

	zmq_msg_t reply;
	zmq_msg_init(&reply);
	zmq_msg_recv(&reply, respond, 0);	
	zmq_msg_close(&reply);
}

void function4(void *respond, int key, double time, char *command){
	MessageMD md;
	md.key = key;
	strcpy(md.message, "ping\0");
	zmq_msg_t req;
	zmq_msg_init_size(&req, sizeof(MessageMD));
	memcpy(zmq_msg_data(&req), &md, sizeof(MessageMD));
	clock_t start = clock();
	zmq_msg_send(&req, respond, 0); 
	zmq_msg_close(&req);

	zmq_msg_t reply;
	zmq_msg_init(&reply);
	zmq_msg_recv(&reply, respond, 0);	
	start = clock() - start;
	zmq_msg_close(&reply);

	if((double)start/(CLOCKS_PER_SEC) < time * 4)
		printf("Time: %f %f\n", time, (double)start/(CLOCKS_PER_SEC));
	else 
		printf("No %f %f\n", time, (double)start/(CLOCKS_PER_SEC));

}

int main (int argc, char const *argv[]) 
{
	void* context = zmq_ctx_new();
	void* respond = zmq_socket(context, ZMQ_REQ);
	
	int key, r_key, port2 = 5051, port3 = 7071;
	double time = 0;
	char command[10], text_string[128], pattern_string[128];
	printf("Input root key: "); scanf("%d", &key);

	zmq_connect(respond, "tcp://localhost:4040");

	if(!fork()) execl("./worker", "./worker", "4040", "5050", "7070", NULL);

	MessageMD md1;
	md1.key = key; md1.port2 = port2; md1.port3 = port3;
	strcpy(md1.message, "start\0");
	zmq_msg_t req;
	zmq_msg_init_size(&req, sizeof(MessageMD));
	memcpy(zmq_msg_data(&req), &md1, sizeof(MessageMD));	
	zmq_msg_send(&req, respond, 0);
	zmq_msg_close(&req);

	zmq_msg_t reply;
	zmq_msg_init(&reply);
	zmq_msg_recv(&reply, respond, 0);		
	zmq_msg_close(&reply);

	Node *tree = create(key);
	r_key = key;

	for(;;) 
	{
		printf("input command "); scanf("%s", command); 
		if(!strcmp(command,"exit")){
			int k2;
			while((k2 = find_key(tree->left)) != 0){
				function2(respond,k2,"remove\0");
				tree = delete_node(tree, k2);
			}
			while((k2 = find_key(tree->right)) != 0){
				function2(respond,k2,"remove\0");
				tree = delete_node(tree, k2);				
			}
			function2(respond,r_key,"remove\0");
			break;
		} 
		else if(!strcmp("remove",command)){
			printf("input key: "); scanf("%d", &key);
			Node *k = search(tree,key);
			if(!k || r_key == key) printf("can't remove\n"); 
			else {
				int k2;
				while((k2 = find_key(k->left))!= 0){
					function2(respond,k2,"remove\0");
					tree = delete_node(tree, k2);
					k = delete_node(k, k2);
					
				}
				while((k2 = find_key(k->right)) != 0){
					function2(respond,k2,"remove\0");
					tree = delete_node(tree, k2);
					k = delete_node(k, k2);
				}
				function2(respond,key,"remove\0");
				tree = delete_node(tree, key);
			}
		}
		else if(!strcmp("ping",command)){
			printf("input key: "); scanf("%d", &key);
			if(!search(tree,key)){
				printf("Not exist\n"); 
			}
			else function4(respond, key, time, "ping\0");

		}
		else if(!strcmp("create",command)){
			printf("input key: "); scanf("%d", &key);
			if(!search(tree, key)){
				port2++; port3++;
				function1(respond,key,port2,port3,"create\0");
				tree = insert(tree, key);
			}
			else printf("Node exists\n");
		}
		else if(!strcmp("heartbit",command)){
			printf("input time sec: "); scanf("%lf", &time);
		} 
		else if(!strcmp("exec",command)){
			printf("input key: "); scanf("%d", &key);
			if(search(tree,key)){
				printf("input text string: "); scanf("%s", text_string);
				printf("input pattern string: "); scanf("%s", pattern_string);
				function3(respond, key, "exec\0", text_string, pattern_string);
			}
			else printf("Not exist\n");	
		}
		else if(!strcmp("print",command)) print(tree);
		sleep(1);
	}
	zmq_close(respond);
	zmq_ctx_destroy(context);
	
	return 0;
}