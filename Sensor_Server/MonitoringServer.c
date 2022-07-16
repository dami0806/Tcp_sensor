#include <stdio.h>

/*
====================================================================================================
Name        : 
Author      : 
Version     :
Description :

����	��¥		����
------- ----------- --------------------------------------------------------------------------------


*/

#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define BUF_SIZE 1024
#define MAX_CLIENTLIST_SIZE 100 // �ִ� ���� ������ Ŭ���̾�Ʈ ��
#define FIRST_CLIENT_NUMBER 1

#define FALSE 0
#define TRUE 1

void ErrorHandling(char* message);

typedef struct ClientSensor {
	boolean pauseFlag; // ���� �Ͻ� �ߴ� ����
	char	sensorNum; // ���� ���� ��ȣ
	double	lowestLimit; // �� ������ ���� ���� ����
	double	highestLimit; // �� ������ �ְ� ���� ����
} Sensor;

typedef struct ClientInfo {
	char	clientNum; // Ŭ���̾�Ʈ ��ȣ
	int	cntSensor; // ��ġ�� ���� ��
	Sensor* sensor; // �� ���� ����ü �迭
} Client;

int main(void) {
	WSADATA			wsaData;
	SOCKET			hServSock, hClntSock;
	SOCKADDR_IN		servAdr, clntAdr, clientAddr;

	int				adrSize, strLen, fdNum, addrLen;
	int				retVal;
	char            initBuf[BUF_SIZE]; // ���� ���� �ʱ� ���� ������ �����ϴ� ����
	char			buf[BUF_SIZE];
	char			temp[100];

	int				cntClient = FIRST_CLIENT_NUMBER; // �ֱ� ��ϵ� Ŭ���̾�Ʈ ���� ��ȣ
	Client			clientList[MAX_CLIENTLIST_SIZE]; // �� Ŭ���̾�Ʈ ������ ��� ����ü �迭

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) // ���� Ȯ��
		ErrorHandling("WSAStartup error");

	hServSock = socket(PF_INET, SOCK_STREAM, 0); // ���� �ּ� ����ü �ʱ�ȭ
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servAdr.sin_port = htons(9000);

	if (bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR) // ���� Ȯ��
		ErrorHandling("Bind error");

	if (listen(hServSock, 5) == SOCKET_ERROR) // ���� Ȯ��
		ErrorHandling("Listen error");

	fd_set cpyReads, cpyReadsSensorNum, cpyWrites, reads;
	TIMEVAL timeout;

	FD_ZERO(&reads);
	FD_SET(hServSock, &reads);

	while (1) {
		cpyReads = reads;

		timeout.tv_sec = 5;
		timeout.tv_usec = 0;

		fdNum = select(0, &cpyReads, 0, 0, &timeout); // Read Event Target

		if (fdNum == SOCKET_ERROR) { // ���� Ȯ��
			printf("Select Read error\n");
		}
		else if (fdNum == 0) { // �ð� �Ҵ� ����
			continue;
		}

		for (int i = 0; i < reads.fd_count; i++) {
			if (FD_ISSET(reads.fd_array[i], &cpyReads)) {
				if (reads.fd_array[i] == hServSock) { // ���� ���Ͽ� ���� ��û �̺�Ʈ �߻�
					adrSize = sizeof(clntAdr);
					hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &adrSize);

					clientList[cntClient - 1].clientNum = cntClient; // Ŭ���̾�Ʈ ���� ��ȣ ����

					printf("Connected Client: Port: %d, IP: %s\n\n",
						clntAdr.sin_port, inet_ntoa(clntAdr.sin_addr));

					FD_SET(hClntSock, &reads);

					while (1) { // ���� ������ ���Ͽ� �̺�Ʈ�� �߻����� �ʾҴٸ� �ݺ�
						/* Receive */ // Ŭ���̾�Ʈ�κ��� ���� �� ����
						cpyReadsSensorNum = reads;

						timeout.tv_sec = 5;
						timeout.tv_usec = 0;

						fdNum = select(0, &cpyReadsSensorNum, 0, 0, &timeout); // Read Event Target

						if (fdNum == SOCKET_ERROR) { // ���� Ȯ��
							printf("Select Read error\n");
						}
						else if (fdNum == 0) { // �ð� �Ҵ� ����
							continue;
						}

						if (FD_ISSET(reads.fd_array[reads.fd_count - 1], &cpyReadsSensorNum)) {  // ���� ��ũ���� ������ ���� ������ ��� �����Ͽ� ������ ���Ͽ� �̺�Ʈ �߻� ���� Ȯ��
							retVal = recv(reads.fd_array[reads.fd_count - 1], (char*)&clientList[cntClient - 1].cntSensor, sizeof(int), 0); // ���� ��

							if (retVal == SOCKET_ERROR) { // ���� Ȯ��
								ErrorHandling("Recieve error");
							}
							
							
							clientList[cntClient - 1].sensor = malloc(sizeof(Sensor) * clientList[cntClient - 1].cntSensor); // ���� ����ü �迭 ���� ���� ���� ���� �Ҵ�

							retVal = recv(reads.fd_array[reads.fd_count - 1], initBuf, sizeof(double) * (2 * clientList[cntClient - 1].cntSensor), 0); // ���� ����

							if (retVal == SOCKET_ERROR) { // ���� Ȯ��
								ErrorHandling("Recieve error");
							}

							for (int j = 0; j < clientList[cntClient - 1].cntSensor; j++) {
								clientList[cntClient - 1].sensor[j].pauseFlag = FALSE; // �Ͻ� �ߴ����� �ʵ��� ����
								clientList[cntClient - 1].sensor[j].sensorNum = j + 1; // ���� ��ȣ ����
								clientList[cntClient - 1].sensor[j].lowestLimit = *(double*)(initBuf + sizeof(double) * (j * 2)); // �ʱ�ȭ �迭�κ��� �� ������ ���� ���� ���� ����
								printf("����%02d�� ���� ���� ����: %f\n", j + 1, clientList[cntClient - 1].sensor[j].lowestLimit);
								clientList[cntClient - 1].sensor[j].highestLimit = *(double*)(initBuf + sizeof(double) * (j * 2 + 1)); // �ʱ�ȭ �迭�κ��� �� ������ �ְ� ���� ���� ����
								printf("����%02d�� �ְ� ���� ����: %f\n", j + 1, clientList[cntClient - 1].sensor[j].highestLimit);
							}

							puts("");

							break;
						}
					}


					/* Send */ // Ŭ���̾�Ʈ ���� ��ȣ �۽�
					cpyWrites = reads;

					timeout.tv_sec = 5;
					timeout.tv_usec = 0;

					fdNum = select(0, 0, &cpyWrites, 0, &timeout); // Write Event Target

					if (fdNum == SOCKET_ERROR) {
						printf("Select Write error\n");
					}
					else if (fdNum == 0) {
						continue;
					}


					if (FD_ISSET(reads.fd_array[reads.fd_count - 1], &cpyWrites)) { // ���� ��ũ���� ������ ���� ������ ��� �����Ͽ� ������ ���Ͽ� �̺�Ʈ �߻� ���� Ȯ��
						printf("Send to Client: Client Number\n\n");

						retVal = send(reads.fd_array[reads.fd_count - 1], &clientList[cntClient - 1].clientNum, sizeof(char), 0);

						if (retVal == SOCKET_ERROR) {
							printf("Send error\n");

							break;
						}
					}

					cntClient++;
				}
				else {
					strLen = recv(reads.fd_array[i], buf, BUF_SIZE, 0); /* Receive */

					if (strLen <= 0) {
						closesocket(reads.fd_array[i]);

						printf("Closed Client: %d, StrLen: %d\n", reads.fd_array[i], strLen);

						FD_CLR(reads.fd_array[i], &reads);
					}
					else {
						addrLen = sizeof(clientAddr); // ?
						getpeername(reads.fd_array[i], (SOCKADDR*)&clientAddr, &addrLen); // ?

						printf("[Ŭ���̾�Ʈ %02d]\n\n", clientList[i - 1].clientNum);

						for (int j = 0; j < clientList[i - 1].cntSensor; j++) {
							printf("���� %02d: %f\n", j + 1, *(double*)(buf + j * sizeof(double)));
						}

						puts("");
					}
				}
			}
		}
	}

	closesocket(hServSock);
	WSACleanup();

	return 0;
}

void ErrorHandling(char* message) {
	fputs(message, stderr);
	fputc('\n', stderr);

	exit(1);
}