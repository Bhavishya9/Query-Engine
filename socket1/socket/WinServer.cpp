#include "stdafx.h"
#include <winsock2.h>
#include <windows.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
struct node
{
	char msg[128];
	int msg_id;
	node *next;
}*flist,*alist,*printid;

struct bufserv{
	
		int userId;
		int forumId;
		int msgId;
		int commentId;
		int choice;
		char *forumname;
		char msg[128];
}buf1;

struct details
{
	int id;
	char name[30];
	char fname[30];
	char mname[30];
	int rollno;
};

struct marks
{
	int id;
	int m_arr[6];
};

bool flag=true;
int mid = 0;
int count1 =0;
char *Data[100];
int count=1;
int values[100];
DWORD WINAPI SocketHandler(void*);
void replyto_client(char *buf, int *csock);
int findNumberLines(FILE *fp);
void get_data(int *csock);
struct details get_details(FILE *fp);
char *record(FILE *fp);
void parsequery(int *csock, struct details *userdetails, struct marks *usermarks,int lines);
void displayAll(char *ch, struct details *userdetails, struct marks *usermarks, int limit, int *csock);
void displayAllDetails(struct details *userdetails,int len,int *csock);
void displayAllMarks(struct marks *usermarks,int len,int *csock);
void displayDetailsSelectedColumns(char *ch, struct details *userdetails, int limit, int *csock);
void displayMarksSelectedColumns(char *ch, struct marks *usermarks, int limit, int *csock);

char* StrStr(const char *str, const char *target);

void socket_server() {

	//The port you want the server to listen on
	int host_port= 1101;

	unsigned short wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD( 2, 2 );
 	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 || ( LOBYTE( wsaData.wVersion ) != 2 ||
		    HIBYTE( wsaData.wVersion ) != 2 )) {
	    fprintf(stderr, "No sock dll %d\n",WSAGetLastError());
		goto FINISH;
	}

	//Initialize sockets and set options
	int hsock;
	int * p_int ;
	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if(hsock == -1){
		printf("Error initializing socket %d\n",WSAGetLastError());
		goto FINISH;
	}
	
	p_int = (int*)malloc(sizeof(int));
	*p_int = 1;
	if( (setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1 )||
		(setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1 ) ){
		printf("Error setting options %d\n", WSAGetLastError());
		free(p_int);
		goto FINISH;
	}
	free(p_int);

	//Bind and listen
	struct sockaddr_in my_addr;
	my_addr.sin_family = AF_INET ;
	my_addr.sin_port = htons(host_port);
	
	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = INADDR_ANY ;
	
	/* if you get error in bind 
	make sure nothing else is listening on that port */
	if( bind( hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1 ){
		fprintf(stderr,"Error binding to socket %d\n",WSAGetLastError());
		goto FINISH;
	}
	if(listen( hsock, 10) == -1 ){
		fprintf(stderr, "Error listening %d\n",WSAGetLastError());
		goto FINISH;
	}
	
	//Now lets do the actual server stuff

	int* csock;
	sockaddr_in sadr;
	int	addr_size = sizeof(SOCKADDR);
	
	while(true){
		printf("waiting for a connection\n");
		csock = (int*)malloc(sizeof(int));
		
		if((*csock = accept( hsock, (SOCKADDR*)&sadr, &addr_size))!= INVALID_SOCKET ){
			//printf("Received connection from %s",inet_ntoa(sadr.sin_addr));
			CreateThread(0,0,&SocketHandler, (void*)csock , 0,0);
		}
		else{
			fprintf(stderr, "Error accepting %d\n",WSAGetLastError());
		}
	}

FINISH:
;
}


void process_input(char *recvbuf, int recv_buf_cnt, int* csock) 
{

	char replybuf[1024]={'\0'};
	printf("%s",recvbuf);
	replyto_client(replybuf, csock);
	replybuf[0] = '\0';
}

void replyto_client(char *buf, int *csock) {
	int bytecount;
	
	if((bytecount = send(*csock, buf, strlen(buf), 0))==SOCKET_ERROR){
		fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
		free (csock);
	}
	printf("replied to client: %s\n",buf);
}

DWORD WINAPI SocketHandler(void* lp){
    int *csock = (int*)lp;
	get_data(csock);
	/*memset(recvbuf, 0, recvbuf_len);
	if((recv_byte_cnt = recv(*csock, recvbuf, recvbuf_len, 0))==SOCKET_ERROR){
		fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
		free (csock);
		return 0;
	}

	//printf("Received bytes %d\nReceived string \"%s\"\n", recv_byte_cnt, recvbuf);
	process_input(recvbuf, recv_byte_cnt, csock);
	*/
    return 0;
}

int findNumberLines(FILE *fp)
{
	char ch;
	ch = fgetc(fp);
	int lines = 0;
	while (ch != EOF)
	{
		if (ch == '\n')
			lines++;
		ch = fgetc(fp);
	}
	return lines;
}

char *record(FILE *fp)
{
	char string[30];
	char ch = fgetc(fp);
	int i = 0;
	while (ch != ',')
	{
		string[i] = ch;
		i++;
		ch = fgetc(fp);
	}
	string[i] = '\0';
	return string;
}

struct details get_details(FILE *fp)
{
	struct details d;
	memset(&d, 0, sizeof(d));
	char ch;
	fscanf(fp, "%d%c", &d.id, &ch);
	strcpy(d.name,record(fp));
	strcpy(d.fname, record(fp));
	strcpy(d.mname, record(fp));
	fscanf(fp, "%d%c", &d.rollno, &ch);
	return d;
}

void get_data(int *csock)
{
	FILE *fd = fopen("C:\\Users\\Bhavisya\\Documents\\Binary File\\full_details.txt", "r");
	FILE *fm = fopen("C:\\Users\\Bhavisya\\Documents\\Binary File\\marks.txt", "r");
	int lines = findNumberLines(fd);
	lines++;
	struct details *userdetails = (struct details *)malloc(lines*(sizeof(struct details)));
	struct marks *usermarks = (struct marks *)malloc(lines*(sizeof(struct marks)));
	fseek(fd, 0, 0);
	int temp1 = 0;
	char ch;
	int temp = ftell(fd);
	while (!feof(fd))
	{
		struct details d;
		d = get_details(fd);
		userdetails[temp1] = d;
		temp1++;
	}
	temp1 = 0;
	while (temp1 < lines)
	{
		fscanf(fm, "%d%c", &usermarks[temp1].id,&ch);
		fscanf(fm, "%d%c", &usermarks[temp1].m_arr[0], &ch);
		fscanf(fm, "%d%c", &usermarks[temp1].m_arr[1], &ch);
		fscanf(fm, "%d%c", &usermarks[temp1].m_arr[2], &ch);
		fscanf(fm, "%d%c", &usermarks[temp1].m_arr[3], &ch);
		fscanf(fm, "%d%c", &usermarks[temp1].m_arr[4], &ch);
		fscanf(fm, "%d%c", &usermarks[temp1].m_arr[5], &ch);
		temp1++;
	}
	for (int i = 0; i < lines; i++)
	{
		printf("%d\t%s\t%s\t%s\t%d\t", userdetails[i].id, userdetails[i].name, userdetails[i].fname, userdetails[i].mname, userdetails[i].rollno);
	}
	for (int i = 0; i < lines; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			printf("%d\t", usermarks[i].m_arr[j]);
		}
		printf("\n");
	}
	parsequery(csock,userdetails,usermarks,lines);
}

void parsequery(int *csock, struct details *userdetails, struct marks *usermarks,int lines)
{
	char recvbuf[1024];
	int recvbuf_len = 1024;
	int recv_byte_cnt;
	while (1)
	{
		memset(recvbuf, 0, recvbuf_len);
		if ((recv_byte_cnt = recv(*csock, recvbuf, recvbuf_len, 0)) == SOCKET_ERROR){
			fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
			free(csock);
			return;
		}
		char *details = StrStr(recvbuf, "details");
		char *join = StrStr(recvbuf, "innerjoin");
		char *ch;
		ch = strtok(recvbuf, " ");
		/*while (ch != NULL) {
			printf("%s\n", ch);
			ch = strtok(NULL, " ,");
			}*/
			if (!strcmp(ch, "select"))
			{
				ch = strtok(NULL, " ,");
				if (!strcmp(ch, "*"))
				{
					displayAll(ch, userdetails, usermarks, lines, csock);
				}
				else
				{
					if (details)
					{
						displayDetailsSelectedColumns(ch, userdetails, 3, csock);
					}
					else
					{
						displayMarksSelectedColumns(ch, usermarks, 3, csock);
					}
				}
			}
		}
}

void displayAll(char *ch, struct details *userdetails, struct marks *usermarks, int limit, int *csock)
{
	ch = strtok(NULL, " ,");
	if (strcmp(ch, "from"))
	{
		printf("Not valid query");
	}
	ch = strtok(NULL, " ,");
	if (!strcmp(ch, "details;"))
	{
		displayAllDetails(userdetails, limit, csock);
	}
	else if (!strcmp(ch, "marks;"))
	{
		displayAllMarks(usermarks, limit, csock);
	}
}

void displayAllDetails(struct details *userdetails,int limit,int *csock)
{
	char buf[1024];
	memset(buf, '\0', 1024);
	strcpy(buf, "ID\tNAME\tFNAME\tMNAME\tROLLNO\n");
	for (int i = 0; i < limit; i++)
	{
		char temp[120];
		char rno[4];
		memset(rno, '\0', 4);
		memset(temp, '\0', 120);
		int index = 0;
		sprintf(temp, "\n%d",userdetails[i].id);
		strcat(temp, "\t");
		strcat(temp, userdetails[i].name);
		strcat(temp, "\t");
		strcat(temp, userdetails[i].fname);
		strcat(temp, "\t");
		strcat(temp, userdetails[i].mname);
		strcat(temp, "\t");
		sprintf(rno, "%d\n", userdetails[i].rollno);
		strcat(temp, rno);
		strcat(buf, temp);
	}
	replyto_client(buf, csock);
}

void displayAllMarks(struct marks *usermarks, int limit, int *csock)
{
	char buf[1024];
	memset(buf, '\0', 1024);
	strcpy(buf, "ID\tMATHS\tJAVA\tWT\tC\tC++\tPYTHON\n");
	for (int i = 0; i < limit; i++)
	{
		char temp[120];
		memset(temp, '\0', 120);
		sprintf(temp, "\n%d\t", usermarks[i].id);
		for (int j = 0; j < 6; j++)
		{
			char temp1[4];
			memset(temp1, '\0', 4);
			sprintf(temp1, "%d\t", usermarks[i].m_arr[j]);
			strcat(temp, temp1);
		}
		strcat(buf, temp);
	}
	replyto_client(buf, csock);
}

char* StrStr(const char *str, const char *target) {
	if (!*target) return (char*)str;
	char *p1 = (char*)str, *p2 = (char*)target;
	char *p1Adv = (char*)str;
	while (*++p2)
		p1Adv++;
	while (*p1Adv) {
		char *p1Begin = p1;
		p2 = (char*)target;
		while (*p1 && *p2 && *p1 == *p2) {
			p1++;
			p2++;
		}
		if (!*p2)
			return p1Begin;
		p1 = p1Begin + 1;
		p1Adv++;
	}
	return NULL;
}

void displayDetailsSelectedColumns(char *ch, struct details *userdetails, int limit, int *csock)
{
	char cols[5][20] = { "id", "name", "fname", "mname", "rollno" };
	int temp[5] = { 0 };
	int index = 0;
	char buf[1024];
	memset(buf, '\0', 1024);
	while (strcmp(ch, "from"))
	{
		for (int i = 0; i < 5; i++)
		{
			if (!strcmp(cols[i], ch))
			{
				temp[i] = 1;
				break;
			}
		}
		ch = strtok(NULL, " ,");
	}
	for (int i = 0; i < 5; i++)
	{
		if (temp[i])
		{
			strcat(buf, cols[i]);
			strcat(buf, "\t");
		}
	}
	strcat(buf, "\n");
	for (int index = 0; index < limit; index++)
	{
		for (int fields = 0; fields < 5; fields++)
		{
			if (temp[fields] != 0)
			{

				char num[5];
				memset(num, '\0', 5);
				switch (fields)
				{
				case 0:
					sprintf(num, "%d\t", userdetails[index].id);
					strcat(buf, num);
					break;
				case 1:
					strcat(buf, userdetails[index].name);
					strcat(buf, "\t");
					break;
				case 2:
					strcat(buf, userdetails[index].fname);
					strcat(buf, "\t");
					break;
				case 3:
					strcat(buf, userdetails[index].mname);
					strcat(buf, "\t");
					break;
				case 4:
					sprintf(num, "%d\t", userdetails[index].rollno);
					strcat(buf, num);
					break;
				}
			}
		}
		strcat(buf, "\n");
	}
	replyto_client(buf,csock);
}

void displayMarksSelectedColumns(char *ch, struct marks *usermarks, int limit, int *csock)
{
	char cols[7][20] = { "id", "maths", "java", "wt", "c","c++","python" };
	int temp[7] = { 0 };
	int index = 0;
	char buf[1024];
	memset(buf, '\0', 1024);
	while (strcmp(ch, "from"))
	{
		for (int i = 0; i < 7; i++)
		{
			if (!strcmp(cols[i], ch))
			{
				temp[i] = 1;
				break;
			}
		}
		ch = strtok(NULL, " ,");
	}
	for (int i = 0; i < 7; i++)
	{
		if (temp[i])
		{
			strcat(buf, cols[i]);
			strcat(buf, "\t");
		}
	}
	strcat(buf, "\n");
	for (int index = 0; index < limit; index++)
	{
		for (int fields = 0; fields < 7; fields++)
		{
			if (temp[fields] != 0)
			{

				char num[5];
				memset(num, '\0', 5);
				switch (fields)
				{
				case 0:
					sprintf(num, "%d\t", usermarks[index].id);
					strcat(buf, num);
					break;
				case 1:
					sprintf(num, "%d\t", usermarks[index].m_arr[0]);
					strcat(buf, num);
					break;
				case 2:
					sprintf(num, "%d\t", usermarks[index].m_arr[1]);
					strcat(buf, num);
					break;
				case 3:
					sprintf(num, "%d\t", usermarks[index].m_arr[2]);
					strcat(buf, num);
					break;
				case 4:
					sprintf(num, "%d\t", usermarks[index].m_arr[3]);
					strcat(buf, num);
					break;
				case 5:
					sprintf(num, "%d\t", usermarks[index].m_arr[4]);
					strcat(buf, num);
					break;
				case 6:
					sprintf(num, "%d\t", usermarks[index].m_arr[5]);
					strcat(buf, num);
					break;
				}
			}
		}
		strcat(buf, "\n");
	}
	replyto_client(buf, csock);

}