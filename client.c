#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <arpa/inet.h>

#define PORT 4250
#define DATA_LEN 9

int DATA_SEND_RECV(int);
int Read(int);
int Write(int);
char check();
void Print();
void Put(char);
void win();
void lose();
void draw();
void end_init();

bool myTurn = false;
bool imFirst = false;
int Round = 0;
int game = 0;
int score[2] = { 0, 0 };
char board[9] = { 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e' };

int main(int argc, char* argv[])
{
	// 1. 소켓을 생성한다.
	int socket_fd;
	socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socket_fd < 0)
	{
		perror("Socket 생성 실패...\n");
		exit(EXIT_FAILURE);
	}
	else
		printf("Socket 생성 성공...\n");

	// 2. 연결을 요쳥한다
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);
	inet_aton("127.0.0.1", (struct in_addr*)&server_address.sin_addr.s_addr);
	//server_address.sin_addr.s_addr = inet_addr(argv[1]);

	if (connect(socket_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0)
	{
		perror("서버ㅡ클라이언트 연결 실패 \n");
		exit(EXIT_FAILURE);
	}
	else
		printf("서버ㅡ클라이언트 연결 성공 \n");

	// 3. 데이터 송수신, 송수신에 대한 처리를 한다.
	DATA_SEND_RECV(socket_fd);

	// 4. 소켓을 닫는다.
	close(socket_fd);
}


int DATA_SEND_RECV(int socket_fd)
{
	int game_result;
	char check_result;
	char mychar;
	char opchar;
	bool endflag = false;
	

	while (1) {
		if (endflag) {
			break;
		}
		if (Round == 0) {
			// 내가 선공이라면, O를 사용한다.
			if (imFirst) {
				mychar = 'o';
				opchar = 'x';
			}
			else {
				mychar = 'x';
				opchar = 'o';
			}

			// 만약 0라운드이고, 내가 선공이라면 먼저 돌을 놓는다.
			// 만약 0라운드이고, 내가 후공이라면 내 차례로 바꾸고 반복문 처음으로 간다
			if (imFirst) {
				Put(mychar);
				Write(socket_fd);
				myTurn = false;
				Round++;
				Print();
				continue;
			}
			else {
				myTurn = true;
				Round++;
				continue;
			}
		}
		// 상대가 전송할때까지 대기한다.
		while (1) {
			sleep(1);
			if (Read(socket_fd) > 0) {
				myTurn = true;
				break;
			}
		}

		// 만약 내 차례가 됐다면, 
		if (myTurn) {
			//1. 종료조건을 검사한다.
			check_result = check();
			// 만약 항복을 했으면 점수 올리고 종료
			if (check_result == 's') { // 상대가 항복한 경우 
				score[0] += 1;
				win();
				endflag = true;
				end_init();
				continue;
			}
			else if (check_result == mychar) { // 이긴 경우
				score[0] += 1;
				win();
				end_init();
				continue;
			}
			else if (check_result == opchar) { // 진 경우
				score[1] += 1;
				lose();
				end_init();
				continue;
			}
			else if (check_result == 'd') { // 비긴 경우
				draw();
				end_init();
				continue;
			}

			// 2. 화면에 게임판 띄우기 (TODO)
			Print();

			// 3. 놓을 곳을 클릭으로 입력 받기
			Put(mychar);

			// 4. 종료조건 검사
			check_result = check();
			if (check_result == 's') { // 내가 항복한 경우 
				score[1] += 1;
				Write(socket_fd);
				lose();
				endflag = true;
				end_init();
				continue;
			}
			else if (check_result == mychar) { // 이긴 경우
				score[0] += 1;
				Write(socket_fd);
				win();
				end_init();
				continue;
			}
			else if (check_result == opchar) { // 진 경우
				score[1] += 1;
				Write(socket_fd);
				lose();
				end_init();
				continue;
			}
			else if (check_result == 'd') { // 비긴 경우
				Write(socket_fd);
				draw();
				end_init();
				continue;
			}

			// 5. 화면에 게임판 띄우기 (TODO)
			Print();

			// 6. 게임판 전송
			Write(socket_fd);
			myTurn = false;
			Round++;
		}
	}
	printf("[%dGame] score -> my(%d) : op(%d)\n", game, score[0], score[1]);
	return 0;
}
int Read(int socket_fd) {
	char data[DATA_LEN];
	memset(board, 0x00, DATA_LEN);
	int readbyte = read(socket_fd, data, DATA_LEN);
	if (readbyte < 0) {
		printf("READ socket error!\n");
		return readbyte;
	}
	else {
		printf("READ socket success!\n");
		for (int i = 0; i < DATA_LEN; i++) {
			board[i] = data[i];
		}
		return readbyte;
	}
}
int Write(int socket_fd) {
	int writebyte = write(socket_fd, board, DATA_LEN);
	if (writebyte < 0) {
		printf("WRITE socket error!\n");
		return writebyte;
	}
	else {
		printf("WRITE socket success!\n");
		return writebyte;
	}
}

void end_init() {
	imFirst = !imFirst;
	Round = 0;
	game++;
	memset(board, 'e', DATA_LEN);
}

void win() {
	printf("win\n");
}
void lose() {
	printf("lose\n");
}
void draw() {
	printf("draw\n");
}
void Print() {
	printf("%c %c %c\n", board[0], board[1], board[2]);
	printf("%c %c %c\n", board[3], board[4], board[5]);
	printf("%c %c %c\n", board[6], board[7], board[8]);
}

void Put(char myChar) {
	int put = 0;
	while (1) {
		// 놓을 곳 입력
		printf("put : ");
		scanf("%d", &put);
		getchar();
		if (put == 9) {
			memset(board, 0x00, DATA_LEN);
			strcpy(board, "endgame");
			break;
		}
		else if (board[put] == 'e') {
			board[put] = myChar;
			break;
		}
		else
		{
			printf("can't put %d\n", put);
			continue;
		}
	}
}

char check() {
	int cnt = 0;
	if (strncmp(board, "endgame", 7) == 0) {
		return 's';
	}
	else {
		for (int i = 0; i < DATA_LEN; i++) {
			if (board[i] == 'e')
				cnt++;
		}
		if ((board[0] == board[1]) && (board[1] == board[2]) && (board[0] != 'e')) {
			return board[0];
		}
		else if ((board[3] == board[4]) && (board[4] == board[5]) && (board[3] != 'e')) {
			return board[3];
		}
		else if ((board[6] == board[7]) && (board[7] == board[8]) && (board[6] != 'e')) {
			return board[6];
		}
		else if ((board[0] == board[3]) && (board[3] == board[6]) && (board[0] != 'e')) {
			return board[0];
		}
		else if ((board[1] == board[4]) && (board[4] == board[7]) && (board[1] != 'e')) {
			return board[1];
		}
		else if ((board[2] == board[5]) && (board[5] == board[8]) && (board[2] != 'e')) {
			return board[2];
		}
		else if ((board[0] == board[4]) && (board[4] == board[8]) && (board[0] != 'e')) {
			return board[0];
		}
		else if ((board[2] == board[4]) && (board[4] == board[6]) && (board[2] != 'e')) {
			return board[2];
		}
		else if (cnt == 0) {
			return 'd';
		}
		else {
			return 'e';
		}
	}
}
