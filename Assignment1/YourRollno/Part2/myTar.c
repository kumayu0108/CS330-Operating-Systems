#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

int main(int argv, char *argc[])
{
	// DIR *d = opendir(argc[1]);
	// struct dirent *pdir;
	// struct stat sss;
	// while((pdir = readdir(d)) > 0){
	// 	// printf("%s, %lu, %u", pdir->d_name, pdir->d_reclen, pdir->d_type);
	// 	char buf[100];
	// 	strcat(buf, argc[1]);
	// 	strcat(buf, "/");
	// 	strcat(buf, pdir->d_name);
	// 	if(stat(buf, &sss) >= 0){
	// 		struct tm dt;
	// 		printf("%s", pdir->d_name);
	// 		printf("\nFile access: ");
	// 		if (sss.st_mode & R_OK)
	// 			printf("read ");
	// 		if (sss.st_mode & W_OK)
	// 			printf("write ");
	// 		if (sss.st_mode & X_OK)
	// 			printf("execute");
	// 		printf("\nFile size: %ld", sss.st_size);
	// 		dt = *(gmtime(&sss.st_ctime));
	// 		printf("\nCreated on: %d-%d-%d %d:%d:%d", dt.tm_mday, dt.tm_mon, dt.tm_year + 1900, 
	// 												dt.tm_hour, dt.tm_min, dt.tm_sec);
	// 		dt = *(gmtime(&sss.st_mtime));
	// 		printf("\nModified on: %d-%d-%d %d:%d:%d", dt.tm_mday, dt.tm_mon, dt.tm_year + 1900, 
	// 												dt.tm_hour, dt.tm_min, dt.tm_sec);
	// 	}
	// 	else {printf("stat failed for %s\n", buf);}
	// 	buf[0] = 0;	printf("\n");
	// }

	// int fd = open("bla.tar", O_RDWR | O_CREAT, 0764);
	// int fd_to_copy = open("Makefile", O_RDONLY);
	// char buf[30];
	// int num;
	// struct stat sss;
	// stat("Makefile", &sss);
	// while((num = read(fd_to_copy, buf, 30)) > 0){
	// 	write(fd, buf, num);
	// }
	// lseek(fd, -1 * (sss.st_size), SEEK_CUR);
	// printf("%ld\n", lseek(fd, 0, SEEK_CUR));
	// buf[0] = 0;
	// printf("%s\n--\n", buf);
	// char buf2[] = "fileName|3000      ";
	// while((num = read(fd, buf2, 30)) > 0){
	// 	write(1, buf2, num);
	// }
	// write(fd, buf, 19);
	// write(fd, "ayush", 5);
	// printf("\n");
	
	// int fd = open("bla.tar", O_RDWR | O_CREAT, 0764);
	// DIR *d = opendir(argc[1]);
	// struct dirent *pdir;
	// struct stat sss;
	// while((pdir = readdir(d)) != NULL){
	// 	if(strcmp(pdir->d_name, ".") == 0 || strcmp(pdir->d_name, "..") == 0)continue;
	// 	char buf[40], addr[100];
	// 	buf[0] = 0; addr[0] = 0;
	// 	strcat(addr, argc[1]);
	// 	if(argc[1][(sizeof(argc[1])/sizeof(argc[1][0])) - 1] != '/'){
	// 		strcat(addr, "/");
	// 	}
	// 	strcat(addr, pdir->d_name);
	// 	stat(addr, &sss);
	// 	int file_to_copy = open(addr, O_RDONLY);
	// 	strcat(buf, pdir->d_name);
	// 	strcat(buf, "|");
	// 	char s[30]; sprintf(s, "%lu", sss.st_size);
	// 	strcat(buf, s);
	// 	strcat(buf, "|");
	// 	int pos = 0; while(buf[pos] != '|')pos++; pos++; while(buf[pos] != '|')pos++; pos++;
	// 	while(pos < 40){buf[pos] = ' '; pos++;}
	// 	// buf[40] = '\0';
	// 	write(fd, buf, 40);
	// 	// lseek(fd, -40, SEEK_SET);
	// 	// read(fd, buf, 40);
	// 	// puts(buf);
	// 	// break;
	// 	int num;
	// 	while((num = read(file_to_copy, buf, 40)) > 0){
	// 		write(fd, buf, num);
	// 	}
	// 	printf("file done\n");
	// }

	if(strcmp(argc[1], "-c") == 0){
		if(argv < 4){printf("Failed to complete creation operation\n"); return 0;}
		char direc[500];
		int pos = 0, pos2 = 0; while(argc[2][pos] != '\0'){direc[pos] = argc[2][pos]; pos++;}
		if(direc[pos-1] != '/'){direc[pos] = '/'; pos++;}
		while(argc[3][pos2] != '\0'){direc[pos] = argc[3][pos2]; pos++; pos2++;} direc[pos] = '\0';
		// puts(direc);

		int fd = open(direc, O_RDWR | O_CREAT, 0664);
		if(fd < 0){printf("Failed to complete creation operation\n"); return 0;}
		DIR *d = opendir(argc[2]);
		if(d == NULL){printf("Failed to complete creation operation\n"); return 0;}

		struct dirent *pdir;
		struct stat sss;
		while((pdir = readdir(d)) != NULL){
			if(strcmp(pdir->d_name, ".") == 0 || strcmp(pdir->d_name, "..") == 0 || strcmp(pdir->d_name, argc[3]) == 0)continue;
			char buf[40], addr[500];
			buf[0] = 0; addr[0] = 0;
			strcat(addr, argc[2]);
			if(argc[2][(sizeof(argc[2])/sizeof(argc[2][0])) - 1] != '/'){
				strcat(addr, "/");
			}
			strcat(addr, pdir->d_name);
			stat(addr, &sss);
			int file_to_copy = open(addr, O_RDONLY);
			if(file_to_copy < 0){printf("Failed to complete creation operation\n"); return 0;}

			strcat(buf, pdir->d_name);
			strcat(buf, "|");
			char s[30]; sprintf(s, "%lu", sss.st_size);
			strcat(buf, s);
			strcat(buf, "|");
			int pos = 0; while(buf[pos] != '|')pos++; pos++; while(buf[pos] != '|')pos++; pos++;
			while(pos < 40){buf[pos] = ' '; pos++;}
			
			write(fd, buf, 40);

			int num;
			while((num = read(file_to_copy, buf, 40)) > 0){
				write(fd, buf, num);
			}
			
		}
	}
	else if(strcmp(argc[1], "-d") == 0){
		if(argv < 3){printf("Failed to complete extraction operation\n"); return 0;}
		int fd = open(argc[2], O_RDWR, 0664);
		if(fd < 0){printf("Failed to complete extraction operation\n"); return 0;}
		char *DirAdd = argc[2];
		int pos = 0, pos2 = 1;
		while(DirAdd[pos] != '\0'){
			if(DirAdd[pos] == '.' && DirAdd[pos+1] == 't' && DirAdd[pos+2] == 'a' && DirAdd[pos+3] == 'r' && DirAdd[pos+4] == '\0'){
				DirAdd[pos] = 'D'; DirAdd[pos+1] = 'u'; DirAdd[pos+2] = 'm'; DirAdd[pos+3] = 'p';
			}
			pos++;
		}
		mkdir(DirAdd, 0777);
		
		char buf[41];
		buf[0] = 0;
		while(read(fd, buf, 40) == 40){
			buf[40] = '\0';
			
			char *token = strtok(buf, "|");
			char bla[500]; bla[0] = 0;
			pos = 0; while(DirAdd[pos] != '\0'){bla[pos] = DirAdd[pos]; pos++;} bla[pos] = '/'; bla[pos+1] = '\0';
			strcat(bla, token);
			
			int nwFile = open(bla, O_RDWR | O_CREAT, 0664);
			if(nwFile < 0){printf("Failed to complete extraction operation\n"); return 0;}
			
			token = strtok(NULL, "|");
			long long sz = atoll(token);
			while(sz > 0){
				long long byte_to_read = (sz < 40 ? sz : 40);
				read(fd, buf, byte_to_read);
				write(nwFile, buf, byte_to_read);
				sz -= byte_to_read;
			}

		}
	}
	else if(strcmp(argc[1], "-e") == 0){
		if(argv < 4){printf("Failed to complete extraction operation\n"); return 0;}
		int fd = open(argc[2], O_RDWR, 0664);
		if(fd < 0){printf("Failed to complete extraction operation\n"); return 0;}
		char DirInd[500], buf[40]; DirInd[0] = 0;
		int pos = 0, ind = 0;
		while(argc[2][ind] != '\0'){
			if(argc[2][ind] == '/')pos = ind;
			ind++;
		}
		if(pos != 0)pos++;
		char *folder = "IndividualDump";
		ind = 0;
		while(ind < pos){DirInd[ind] = argc[2][ind]; ind++;}
		
		ind = 0;
		while(folder[ind] != '\0'){
			DirInd[pos] = folder[ind];
			pos++; ind++;
		}
		DirInd[pos] = '\0';
		mkdir(DirInd, 0777);

		while(read(fd, buf, 40) == 40){
			buf[40] = '\0';
			char *token = strtok(buf, "|");
			if(strcmp(token, argc[3]) != 0){
				token = strtok(NULL, "|");
				long long sz = atoll(token);
				lseek(fd, sz, SEEK_CUR);
				continue;
			}
			token = strtok(NULL, "|");
			long long sz = atoll(token);

			char bla[500]; bla[0] = 0;
			strcat(bla, DirInd);
			bla[pos] = '/';
			strcat(bla, argc[3]);
			
			int nwFile = open(bla, O_RDWR | O_CREAT, 0644);
			if(nwFile < 0){printf("Failed to complete extraction operation\n"); return 0;}
			while(sz > 0){
				long long byte_to_read = (sz < 40 ? sz : 40);
				read(fd, buf, byte_to_read);
				write(nwFile, buf, byte_to_read);
				sz -= byte_to_read;
			}
			return 0;
		}
		printf("No such file is present in tar file.\n");
	}
	else if(strcmp(argc[1], "-l") == 0){
		if(argv < 3){printf("Failed to complete list operation\n"); return 0;}
		int fd = open(argc[2], O_RDWR | O_CREAT, 0664);
		if(fd < 0){printf("Failed to complete list operation\n"); return 0;}
		char DirInd[500], buf[40]; DirInd[0] = 0;
		int pos = 0, ind = 0;
		while(argc[2][ind] != '\0'){
			if(argc[2][ind] == '/')pos = ind;
			ind++;
		}
		if(pos != 0)pos++;
		char *folder = "tarStructure";
		ind = 0;
		while(ind < pos){DirInd[ind] = argc[2][ind]; ind++;}
		ind = 0;
		while(folder[ind] != '\0'){
			DirInd[pos] = folder[ind];
			pos++; ind++;
		}
		DirInd[pos] = '\0';
		
		int nwFile = open(DirInd, O_RDWR | O_CREAT, 0644);
		if(nwFile < 0){printf("Failed to complete list operation\n"); return 0;}
		unsigned long long totSz = lseek(fd, 0, SEEK_END);
		lseek(fd, 0, SEEK_SET);
		int numFiles = 0;

		while(read(fd, buf, 40) == 40){
			char *token = strtok(buf, "|");
			token = strtok(NULL, "|");
			long long sz = atoll(token);
			lseek(fd, sz, SEEK_CUR);
			numFiles++;
		}
		sprintf(buf, "%llu", totSz);
		pos = 0;	while(buf[pos] != '\0'){pos++;}
		buf[pos] = '\n'; buf[pos + 1] = '\0';
		write(nwFile, buf, pos + 1);
		sprintf(buf, "%d", numFiles);
		pos = 0;	while(buf[pos] != '\0'){pos++;}
		buf[pos] = '\n'; buf[pos + 1] = '\0';
		write(nwFile, buf, pos + 1);
		lseek(fd, 0, SEEK_SET);
		while(read(fd, buf, 40) == 40){
			char *token = strtok(buf, "|");
			char tmp[400];
			char *siz = strtok(NULL, "|");
			long long sz = atoll(siz);

			int flg = 0;
			ind = 0; pos = 0;
			while(token[ind] != '\0'){
				tmp[pos] = token[ind];
				pos++; ind++;
			}
			tmp[pos] = ' ';
			ind = 0; pos++;
			while(siz[ind] != '\0'){
				tmp[pos] = siz[ind];
				pos++; ind++;
			}
			tmp[pos] = '\n';
			pos++;
			
			write(nwFile, tmp, pos);
			lseek(fd, sz, SEEK_CUR);
		}
	}
	return 0;
}
