#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h> 
#include <arpa/inet.h> 
#include <sys/wait.h> 
#include <signal.h>
#include "llist.h"

#define PORT "7777"    
#define BACKLOG 10   
#define MAXDATASIZE 100 
#define nlen 21
#define msglen 64
int play_gr[3][3];

int check_gr()
{
	int res = 0, i, j;
	
	if ((play_gr[0][0] == play_gr[1][0]) && (play_gr[0][0] == play_gr[2][0]) && (play_gr[0][0] != 0)) {
		res = 1;
	}
	if ((play_gr[0][1] == play_gr[1][1]) && (play_gr[0][1] == play_gr[2][1]) && (play_gr[0][1] != 0)) {
		res = 1;
	}
	if ((play_gr[0][2] == play_gr[1][2]) && (play_gr[0][2] == play_gr[2][2]) && (play_gr[0][2] != 0)) {
		res = 1;
	}
	
	if ((play_gr[0][0] == play_gr[0][1]) && (play_gr[0][0] == play_gr[0][2]) && (play_gr[0][0] != 0)) {
		res = 1;
	}
	if ((play_gr[1][0] == play_gr[1][1]) && (play_gr[1][0] == play_gr[1][2]) && (play_gr[1][0] != 0)) {
		res = 1;
	}
	if ((play_gr[2][0] == play_gr[2][1]) && (play_gr[2][0] == play_gr[2][2]) && (play_gr[2][0] != 0)) {
		res = 1;
	}
	
	if ((play_gr[0][0] == play_gr[1][1]) && (play_gr[0][0] == play_gr[2][2]) && (play_gr[0][0] != 0)) {
		res = 1;
	}
	if ((play_gr[2][0] == play_gr[1][1]) && (play_gr[2][0] == play_gr[0][2]) && (play_gr[2][0] != 0)) {
		res = 1;
	}
	if (res != 1) {
		int fl = 0;
		for (i = 0; i < 3; i++)
		{
			for (j = 0; j < 3; j++)
			{
				if (play_gr[i][j] == 0)
				{
					fl = 1;
				}
				
			}
		}
		if (fl == 0)
		{
			res = 2;
		}
	}
	
	return res;
}

void sigchld_handler(int s) 
{ 
	while(waitpid(-1, NULL, WNOHANG) > 0); 
}

void *get_in_addr(struct sockaddr *sa) 
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr); 
}

int main(void) 
{ 
	int sockfd, new_fd, numbytes, yes=1, rv, op1, op2, fdmax, command;
	struct addrinfo hints, *servinfo, *p; 
    struct sockaddr_storage their_addr;  
	socklen_t sin_size; 
	struct sigaction sa;  
	char s[INET6_ADDRSTRLEN], message[msglen]; 
    struct list *ptrlist;
    struct lroot *root;
    fd_set read_fds, master;
	
	memset(&hints, 0, sizeof hints); 
	hints.ai_family = AF_UNSPEC; 
	hints.ai_socktype = SOCK_STREAM; 
    hints.ai_flags = AI_PASSIVE;    
	
	FD_ZERO(&read_fds);
	FD_ZERO(&master);
	
	root = init();
	
	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) { 
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv)); 
		return 1; 
	}

	for(p = servinfo; p != NULL; p = p->ai_next) { 
		if ((sockfd = socket(p->ai_family, p->ai_socktype, 
			p->ai_protocol)) == -1) { 
			perror("server: socket"); 
			continue; 
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd); 
			perror("server: bind"); 
			continue; 
		}
		break; 
	}
	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n"); 
		return 2;
	}
	
	freeaddrinfo(servinfo);   

	if (listen(sockfd, BACKLOG) == -1) { 
		perror("listen");
		exit(1);
	}
    
    sa.sa_handler = sigchld_handler;    
	sigemptyset(&sa.sa_mask); 
	sa.sa_flags = SA_RESTART; 
	
	if (sigaction(SIGCHLD, &sa, NULL) == -1) { 
		perror("sigaction");
		exit(1);
	}

    printf("server: waiting for connections...\n");

    while(1) {  
		sin_size = sizeof their_addr; 
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size); 
		if (new_fd == -1) { 
			perror("accept"); 
			continue;
		}

		inet_ntop(their_addr.ss_family, 
		get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);

        printf("server: got connection from %s\n", s);

		/*********** Начало обмена м-ду сервером и новым клиентом ********/
		memset(message, 0, msglen);
		message[0] = 100;
		if (send(new_fd, message, msglen, 0) == -1) {
			perror("send");
		}
		
		if ((numbytes = recv(new_fd, message, msglen, 0)) == -1) { 
			perror("recv"); 
			exit(1); 
		}
		command = (int) message[0];
		
		if (command == 1) {
			message[numbytes] = '\0';
			
			addelem(root, new_fd, message + 4);

		} else {
			if(root->count != 0) {
				ptrlist = root->first_node;
				message[0] = 2;
				message[1] = root->count;
				if (send(new_fd, message, msglen, 0) == -1) {
					perror("send");
				}
				int i;
				for (i = 0; i < root->count; i++)
				{
					if (send(new_fd, ptrlist->name, strlen(ptrlist->name), 0) == -1) {
						perror("send");
					}
					ptrlist = ptrlist->ptr;
					if ((numbytes = recv(new_fd, message, msglen, 0)) == -1) { 
						perror("recv"); 
						exit(1); 
					}
				}
				memset(message, 0, msglen);
				if ((numbytes = recv(new_fd, message, msglen, 0)) == -1) { 
					perror("recv"); 
					exit(1); 
				}
				message[numbytes] = '\0';
				ptrlist = listfind(root, message + 4);
			
				memset(message, 0, msglen);
				message[0] = 100;
				if (send(ptrlist->fd, message, msglen, 0) == -1) {
					perror("send");
				}
				
				
				op1 = ptrlist->fd;
				op2 = new_fd;
				
				deletelem(ptrlist, root);
				
				if (!fork()) {
					int i, j, ij;
					int chk;
					
					close(sockfd);
					
					for (i = 0; i < 3; i++) {
						for (j = 0; j < 3; j++) {
							play_gr[i][j] = 0;
						}
					}
					
					if(op1 > op2) {
						fdmax = op1;
					} else {
						fdmax = op2;
					}
					
					FD_SET(op1, &master);
					FD_SET(op2, &master);
					
					while(1) {
						read_fds = master;
						if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
							perror("select");
							break;
						}
						if (FD_ISSET(op1, &read_fds)) {
							memset(message, 0, msglen);
							if((numbytes = recv(op1, message, msglen, 0)) <= 0) {
								perror("recv");
							} else {
								command = message[0];
								if (command == 10) {//сообщение с i и j
										ij = (int) message[1];
										i = ij / 10;
										j = ij % 10;
										
										
										play_gr[i][j] = 1;
										//отправить i и j op2
										if (send(op2, message, msglen, 0) == -1) {
											perror("send");
										}
										memset(message, 0, msglen);
										//проверка на выигрыш
										chk = check_gr();
										if (chk == 1) {
											//игрок 1 выиграл
											message[0] = 30;
											if (send(op1, message, msglen, 0) == -1) {
												perror("send");
											}
											if (send(op2, message, msglen, 0) == -1) {
												perror("send");
											}
											break;
										} else {
											if (chk == 2)
											{
												//ничья
												message[0] = 50;
											
												if (send(op1, message, msglen, 0) == -1) {
												perror("send");
												}
												if (send(op2, message, msglen, 0) == -1) {
													perror("send");
												}
												break;
											} else {
												//продолжить играть
												message[0] = 60;
												if (send(op1, message, msglen, 0) == -1) {
													perror("send");
												}
												if (send(op2, message, msglen, 0) == -1) {
													perror("send");
												}
											}
										}
										
								} else {//сообщение чата
									if (send(op2, message, msglen, 0) == -1) {
										perror("send");
									}
									memset(message, 0, msglen);
								}
							
							}
						}
						if (FD_ISSET(op2, &read_fds)) {
							memset(message, 0, msglen);
							if((numbytes = recv(op2, message, msglen, 0)) <= 0) {
								perror("recv");
							} else {
								command = message[0];
								if (command == 10) {//сообщение с i и j
										ij = (int) message[1];
										i = ij / 10;
										j = ij % 10;
										
										
										play_gr[i][j] = 2;
										//отправить i и j op2
										if (send(op1, message, msglen, 0) == -1) {
											perror("send");
										}
										memset(message, 0, msglen);
										//проверка на выигрыш
										chk = check_gr();
										if (chk == 1) {
											//игрок 2 выиграл
											message[0] = 40;
											if (send(op1, message, msglen, 0) == -1) {
												perror("send");
											}
											if (send(op2, message, msglen, 0) == -1) {
												perror("send");
											}
											break;
										} else {
											if (chk == 2)
											{
												//ничья
												message[0] = 50;
											
												if (send(op1, message, msglen, 0) == -1) {
												perror("send");
												}
												if (send(op2, message, msglen, 0) == -1) {
													perror("send");
												}
												break;
											} else {
												//продолжить играть
												message[0] = 60;
												if (send(op1, message, msglen, 0) == -1) {
													perror("send");
												}
												if (send(op2, message, msglen, 0) == -1) {
													perror("send");
												}
											}
										}
										
								} else {//сообщение чата
									if (send(op1, message, msglen, 0) == -1) {
										perror("send");
									}
									memset(message, 0, msglen);
								}
							
							}
						}
					}
						
					close(op1);
					close(op2); 
					exit(0); 
				}
				close(op1);
				close(op2);	
			} else {
				memset(message, 0, msglen);
				message[1] = 255;
				if (send(new_fd, message, msglen, 0) == -1) {
					perror("send");
				}
			}

		}
	}
	return 0; 
}

