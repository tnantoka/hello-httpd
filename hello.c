#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>

#define DOCUMENT_ROOT "./htdocs"
#define BUF_SIZE 1024

#define DEBUG

int main(int argc, char *argv[]) {

	struct sockaddr_in server;
	int sockfd;
	int port = (argc == 2) ? atoi(argv[1]) : 80;

	// socket作成
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
	
	// Port, IPアドレスを設定(IPv4)
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;	
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);
	
	// socketをすぐ再利用できるように
	char opt = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

	// ディスクリプタとPortを結び付ける
	if (bind(sockfd, (struct sockaddr *) &server, sizeof(server)) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}
	
	// listen準備
	if (listen(sockfd, SOMAXCONN) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	// メインループ
	while (1) {
		struct sockaddr_in client;
		int newfd;

		char buf[BUF_SIZE] = "";
		char method[BUF_SIZE] = "";
		char url[BUF_SIZE] = "";
		char protocol[BUF_SIZE] = "";
		int len;

		int filefd;
		char file[BUF_SIZE] = "";

		// clientからの接続を受け付ける
		memset(&client, 0, sizeof(client));
		len = sizeof(client);
		if ((newfd = accept(sockfd, (struct sockaddr *) &client, &len)) < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}

		// request line読み込み
		if (recv(newfd, buf, sizeof(buf), 0) < 0) {
			perror("recv");
			exit(EXIT_FAILURE);
		}
		sscanf(buf, "%s %s %s", method, url, protocol);

		// request headerの終わりまで読み飛ばし
		// bodyは無視		
		do {
			if (strstr(buf, "\r\n\r\n")) {
				break;
			}
			if (strlen(buf) >= sizeof(buf)) {
				memset(&buf, 0, sizeof(buf));
			}
		} while (recv(newfd, buf+strlen(buf), sizeof(buf) - strlen(buf), 0) > 0);

		// ファイルパス生成（脆弱性有り）	
		sprintf(file, DOCUMENT_ROOT);
		strcat(file, url);		

		// index.html補完
		if( file[strlen(file)-1] == '/' ) {
			strcat(file, "index.html" );
		}

		// debug
		#ifdef DEBUG
			sleep(1);
		#endif

		// （最低限の）header送信
		char *header = "HTTP/1.0 200 OK\n"
								"Content-type: text/html\n"
								"\n";
		send(newfd, header, strlen(header), 0);

		//	body送信
		if ((filefd = open(file, O_RDONLY)) < 0) {
			perror("open");
			fprintf(stderr, "file: %s\n", file);
		}
		else {
			while ((len = read(filefd, buf, sizeof(buf))) > 0) {
				if (send(newfd, buf, len, 0) < 0) {
					perror("send2");
				}
			}
		}

		close(newfd);
	}
}