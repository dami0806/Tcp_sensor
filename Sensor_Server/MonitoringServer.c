#include <stdio.h>

/*
====================================================================================================
Name        : 
Author      : 
Version     :
Description :

구분	날짜		내용
------- ----------- --------------------------------------------------------------------------------


*/

#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define BUF_SIZE 1024
#define MAX_CLIENTLIST_SIZE 100 // 최대 생성 가능한 클라이언트 수
#define FIRST_CLIENT_NUMBER 1

#define FALSE 0
#define TRUE 1

void ErrorHandling(char* message);

typedef struct ClientSensor {
	boolean pauseFlag; // 센서 일시 중단 여부
	char	sensorNum; // 센서 가상 번호
	double	lowestLimit; // 각 센서의 최저 안전 범위
	double	highestLimit; // 각 센서의 최고 안전 범위
} Sensor;

typedef struct ClientInfo {
	char	clientNum; // 클라이언트 번호
	int	cntSensor; // 설치된 센서 수
	Sensor* sensor; // 각 센서 구조체 배열
} Client;

int main(void) {
	WSADATA			wsaData;
	SOCKET			hServSock, hClntSock;
	SOCKADDR_IN		servAdr, clntAdr, clientAddr;

	int				adrSize, strLen, fdNum, addrLen;
	int				retVal;
	char            initBuf[BUF_SIZE]; // 센서 수와 초기 안전 범위를 저장하는 버퍼
	char			buf[BUF_SIZE];
	char			temp[100];

	int				cntClient = FIRST_CLIENT_NUMBER; // 최근 등록된 클라이언트 가상 번호
	Client			clientList[MAX_CLIENTLIST_SIZE]; // 각 클라이언트 정보가 담긴 구조체 배열

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) // 오류 확인
		ErrorHandling("WSAStartup error");

	hServSock = socket(PF_INET, SOCK_STREAM, 0); // 소켓 주소 구조체 초기화
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servAdr.sin_port = htons(9000);

	if (bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR) // 오류 확인
		ErrorHandling("Bind error");

	if (listen(hServSock, 5) == SOCKET_ERROR) // 오류 확인
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

		if (fdNum == SOCKET_ERROR) { // 오류 확인
			printf("Select Read error\n");
		}
		else if (fdNum == 0) { // 시간 할당 종료
			continue;
		}

		for (int i = 0; i < reads.fd_count; i++) {
			if (FD_ISSET(reads.fd_array[i], &cpyReads)) {
				if (reads.fd_array[i] == hServSock) { // 서버 소켓에 연결 요청 이벤트 발생
					adrSize = sizeof(clntAdr);
					hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &adrSize);

					clientList[cntClient - 1].clientNum = cntClient; // 클라이언트 가상 번호 저장

					printf("Connected Client: Port: %d, IP: %s\n\n",
						clntAdr.sin_port, inet_ntoa(clntAdr.sin_addr));

					FD_SET(hClntSock, &reads);

					while (1) { // 직전 생성된 소켓에 이벤트가 발생하지 않았다면 반복
						/* Receive */ // 클라이언트로부터 센서 수 수신
						cpyReadsSensorNum = reads;

						timeout.tv_sec = 5;
						timeout.tv_usec = 0;

						fdNum = select(0, &cpyReadsSensorNum, 0, 0, &timeout); // Read Event Target

						if (fdNum == SOCKET_ERROR) { // 오류 확인
							printf("Select Read error\n");
						}
						else if (fdNum == 0) { // 시간 할당 종료
							continue;
						}

						if (FD_ISSET(reads.fd_array[reads.fd_count - 1], &cpyReadsSensorNum)) {  // 파일 디스크립터 집합의 가장 마지막 요소 선택하여 생성된 소켓에 이벤트 발생 여부 확인
							retVal = recv(reads.fd_array[reads.fd_count - 1], (char*)&clientList[cntClient - 1].cntSensor, sizeof(int), 0); // 센서 수

							if (retVal == SOCKET_ERROR) { // 오류 확인
								ErrorHandling("Recieve error");
							}
							
							
							clientList[cntClient - 1].sensor = malloc(sizeof(Sensor) * clientList[cntClient - 1].cntSensor); // 센서 구조체 배열 센서 수에 따라 동적 할당

							retVal = recv(reads.fd_array[reads.fd_count - 1], initBuf, sizeof(double) * (2 * clientList[cntClient - 1].cntSensor), 0); // 안전 범위

							if (retVal == SOCKET_ERROR) { // 오류 확인
								ErrorHandling("Recieve error");
							}

							for (int j = 0; j < clientList[cntClient - 1].cntSensor; j++) {
								clientList[cntClient - 1].sensor[j].pauseFlag = FALSE; // 일시 중단하지 않도록 설정
								clientList[cntClient - 1].sensor[j].sensorNum = j + 1; // 센서 번호 설정
								clientList[cntClient - 1].sensor[j].lowestLimit = *(double*)(initBuf + sizeof(double) * (j * 2)); // 초기화 배열로부터 각 센서에 최저 안전 범위 저장
								printf("센서%02d의 최저 안전 범위: %f\n", j + 1, clientList[cntClient - 1].sensor[j].lowestLimit);
								clientList[cntClient - 1].sensor[j].highestLimit = *(double*)(initBuf + sizeof(double) * (j * 2 + 1)); // 초기화 배열로부터 각 센서에 최고 안전 범위 저장
								printf("센서%02d의 최고 안전 범위: %f\n", j + 1, clientList[cntClient - 1].sensor[j].highestLimit);
							}

							puts("");

							break;
						}
					}


					/* Send */ // 클라이언트 가상 번호 송신
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


					if (FD_ISSET(reads.fd_array[reads.fd_count - 1], &cpyWrites)) { // 파일 디스크립터 집합의 가장 마지막 요소 선택하여 생성된 소켓에 이벤트 발생 여부 확인
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

						printf("[클라이언트 %02d]\n\n", clientList[i - 1].clientNum);

						for (int j = 0; j < clientList[i - 1].cntSensor; j++) {
							printf("센서 %02d: %f\n", j + 1, *(double*)(buf + j * sizeof(double)));
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