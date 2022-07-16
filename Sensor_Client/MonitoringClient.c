#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define	BUF_SIZE 1024

#define FALSE 0
#define TRUE 1

void ErrorHandling(char* message);

typedef struct ClientSensor {
	boolean pauseFlag; // 센서 일시 중단 여부
	char pauseSeconds; // 남은 중단 시간
	int sensorNum; // 실제 센서 하드웨어 번호
} Sensor;

typedef struct ClientInfo {
	char clientNum; // 클라이언트 가상 번호
	int cntSensor; // 설치된 센서 수
	Sensor* sensor; // 각 센서 구조체 배열
} Client;

int main(void) {
	WSADATA	wsaData;
	SOCKET	clientSocket;
	SOCKADDR_IN	serverAddr;

	int	retVal;
	int strLen;
	char initBuf[BUF_SIZE]; // 센서 수와 초기 안전 범위를 저장하는 버퍼

	double sensorVal; // 센서 값
	double* sensorValArr; // 주기적으로 갱신되는 각 센서 값 임시 저장

	Client client;
	
	srand(time(NULL)); // 랜덤 시드 초기화

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) // 오류 확인
		ErrorHandling("WSAStartup error");
	
	clientSocket = socket(AF_INET, SOCK_STREAM, 0); // 소켓 생성

	if (clientSocket == INVALID_SOCKET) { // 오류 확인
		ErrorHandling("Socket error");
	}
	
	ZeroMemory(&serverAddr, sizeof(serverAddr)); // 소켓 주소 구조체 초기화
	serverAddr.sin_family		= AF_INET;
	serverAddr.sin_port			= htons(9000);
	serverAddr.sin_addr.s_addr	= inet_addr("127.0.0.1");
	
	retVal = connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)); // 서버 소켓에 연결 요청

	if (retVal == SOCKET_ERROR) { // 오류 확인
		ErrorHandling("Connect error");
	}

	printf("센서 수: "); 
	scanf("%d", &client.cntSensor); // 클라이언트 센서 수 설정
	puts("");

	*(int*)initBuf = client.cntSensor; // 초기화 배열에 우선 센서 수 저장

	for (int i = 0; i < client.cntSensor; i++) {
		printf("센서%02d의 최저 안전 범위: ", i + 1); 
		scanf("%lf", (double*)&initBuf[(sizeof(int) + sizeof(double) * (i * 2))]); // 초기화 배열에 최저 안전 범위 저장
		printf("센서%02d의 최고 안전 범위: ", i + 1);
		scanf("%lf", (double*)&initBuf[(sizeof(int) + sizeof(double) * (i * 2 + 1))]); // 초기화 배열에 최고 안전 범위 저장
	}

	puts("");

	client.sensor = malloc(sizeof(Sensor) * client.cntSensor); // 센서 구조체 배열 센서 수에 따라 동적 할당
	
	send(clientSocket, initBuf, sizeof(int) + sizeof(double) * (2 * client.cntSensor), 0); /* Send */ // 센서 수와 안전 범위 서버에 송신
	sensorValArr = malloc(sizeof(double) * client.cntSensor); // 센서 값 배열 센서 수에 따라 동적 할당

	strLen = recv(clientSocket, &client.clientNum, sizeof(client.clientNum), 0); /* Receive */ // 클라이언트 번호 서버로부터 수신

	if (strLen <= 0) // 오류 확인
		printf("Receive error\n");

	printf("[클라이언트 %02d]\n\n", client.clientNum);
	
	while (1) { // 주기적으로 랜덤 센서 값 발생 후 서버로 전달
		for (int i = 0; i < client.cntSensor; i++) {
			sensorValArr[i] = (double)rand() / RAND_MAX;
			printf("센서%02d: %f\n", i + 1, sensorValArr[i]);
		}
		puts("");

		retVal = send(clientSocket, (char*)sensorValArr, sizeof(double) * client.cntSensor, 0); /* Send */ // 랜덤 센서 값 배열 서버에 송신
		
		if (retVal == SOCKET_ERROR) { // 오류 확인
			printf("Send error\n");

			break;
		}

		Sleep(2000);
	}

	closesocket(clientSocket);	
	WSACleanup();

	free(client.sensor);
	free(sensorValArr);

	return 0;
}

void ErrorHandling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);

	exit(1);
}