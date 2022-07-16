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
	boolean pauseFlag; // ���� �Ͻ� �ߴ� ����
	char pauseSeconds; // ���� �ߴ� �ð�
	int sensorNum; // ���� ���� �ϵ���� ��ȣ
} Sensor;

typedef struct ClientInfo {
	char clientNum; // Ŭ���̾�Ʈ ���� ��ȣ
	int cntSensor; // ��ġ�� ���� ��
	Sensor* sensor; // �� ���� ����ü �迭
} Client;

int main(void) {
	WSADATA	wsaData;
	SOCKET	clientSocket;
	SOCKADDR_IN	serverAddr;

	int	retVal;
	int strLen;
	char initBuf[BUF_SIZE]; // ���� ���� �ʱ� ���� ������ �����ϴ� ����

	double sensorVal; // ���� ��
	double* sensorValArr; // �ֱ������� ���ŵǴ� �� ���� �� �ӽ� ����

	Client client;
	
	srand(time(NULL)); // ���� �õ� �ʱ�ȭ

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) // ���� Ȯ��
		ErrorHandling("WSAStartup error");
	
	clientSocket = socket(AF_INET, SOCK_STREAM, 0); // ���� ����

	if (clientSocket == INVALID_SOCKET) { // ���� Ȯ��
		ErrorHandling("Socket error");
	}
	
	ZeroMemory(&serverAddr, sizeof(serverAddr)); // ���� �ּ� ����ü �ʱ�ȭ
	serverAddr.sin_family		= AF_INET;
	serverAddr.sin_port			= htons(9000);
	serverAddr.sin_addr.s_addr	= inet_addr("127.0.0.1");
	
	retVal = connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)); // ���� ���Ͽ� ���� ��û

	if (retVal == SOCKET_ERROR) { // ���� Ȯ��
		ErrorHandling("Connect error");
	}

	printf("���� ��: "); 
	scanf("%d", &client.cntSensor); // Ŭ���̾�Ʈ ���� �� ����
	puts("");

	*(int*)initBuf = client.cntSensor; // �ʱ�ȭ �迭�� �켱 ���� �� ����

	for (int i = 0; i < client.cntSensor; i++) {
		printf("����%02d�� ���� ���� ����: ", i + 1); 
		scanf("%lf", (double*)&initBuf[(sizeof(int) + sizeof(double) * (i * 2))]); // �ʱ�ȭ �迭�� ���� ���� ���� ����
		printf("����%02d�� �ְ� ���� ����: ", i + 1);
		scanf("%lf", (double*)&initBuf[(sizeof(int) + sizeof(double) * (i * 2 + 1))]); // �ʱ�ȭ �迭�� �ְ� ���� ���� ����
	}

	puts("");

	client.sensor = malloc(sizeof(Sensor) * client.cntSensor); // ���� ����ü �迭 ���� ���� ���� ���� �Ҵ�
	
	send(clientSocket, initBuf, sizeof(int) + sizeof(double) * (2 * client.cntSensor), 0); /* Send */ // ���� ���� ���� ���� ������ �۽�
	sensorValArr = malloc(sizeof(double) * client.cntSensor); // ���� �� �迭 ���� ���� ���� ���� �Ҵ�

	strLen = recv(clientSocket, &client.clientNum, sizeof(client.clientNum), 0); /* Receive */ // Ŭ���̾�Ʈ ��ȣ �����κ��� ����

	if (strLen <= 0) // ���� Ȯ��
		printf("Receive error\n");

	printf("[Ŭ���̾�Ʈ %02d]\n\n", client.clientNum);
	
	while (1) { // �ֱ������� ���� ���� �� �߻� �� ������ ����
		for (int i = 0; i < client.cntSensor; i++) {
			sensorValArr[i] = (double)rand() / RAND_MAX;
			printf("����%02d: %f\n", i + 1, sensorValArr[i]);
		}
		puts("");

		retVal = send(clientSocket, (char*)sensorValArr, sizeof(double) * client.cntSensor, 0); /* Send */ // ���� ���� �� �迭 ������ �۽�
		
		if (retVal == SOCKET_ERROR) { // ���� Ȯ��
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