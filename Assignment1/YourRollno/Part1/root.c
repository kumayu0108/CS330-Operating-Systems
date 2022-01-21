#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

int main(int argv, char *argc[])
{

	if(argv < 2){printf("UNABLE TO EXECUTE\n"); return 0;}
	int flag = 0;
	int ind = 0;
	while(argc[argv - 1][ind] != '\0'){if(argc[argv-1][ind] > '9' || argc[argv-1][ind] < '0'){flag = 1; break;} ind++;}
	if(flag){printf("UNABLE TO EXECUTE\n"); return 0;}

	unsigned long long ret = atol(argc[argv - 1]);

	ret = round(sqrt(ret));

	if(argv == 2 || (argc[0][0] == 'e' && argv == 3)){
		printf("%d\n", (int)ret);
		return 0;
	}

	if((argc[0][0] != 'e' && strcmp(argc[1], "root") == 0) || (argc[0][0] == 'e' && strcmp(argc[2], "root") == 0)){
		int sz;
		if(argc[0][0] == 'e'){sz = argv - 1;}
		else sz = argv;
		sz++;
		char *par[sz];
		
		if(argc[0][0] == 'e'){
			par[0] = "e";
			for(int i = 2; i < argv - 1; i++){
				par[i-1] = argc[i];
			}
		}
		else {
			par[0] = "e";
			for(int i = 1; i < argv - 1; i++){
				par[i] = argc[i];
			}
		}
		
		char s[30]; 
		sprintf(s, "%llu", ret);
		
		par[sz - 2] = s;
		par[sz - 1] = NULL;
		
		if(execv("./root", par) < 0){
			printf("UNABLE TO EXECUTE\n");
		}
	}
	else if((argc[0][0] != 'e' && strcmp(argc[1], "square") == 0) || (argc[0][0] == 'e' && strcmp(argc[2], "square") == 0)){
		int sz;
		if(argc[0][0] == 'e'){sz = argv - 1;}
		else sz = argv;
		sz++;
		char *par[sz];
		
		if(argc[0][0] == 'e'){
			par[0] = "e";
			for(int i = 2; i < argv - 1; i++){
				par[i-1] = argc[i];
			}
		}
		else {
			par[0] = "e";
			for(int i = 1; i < argv - 1; i++){
				par[i] = argc[i];
			}
		}
		
		char s[30]; 
		sprintf(s, "%llu", ret);
		
		par[sz - 2] = s;
		par[sz - 1] = NULL;
		
		if(execv("./square", par) < 0){
			printf("UNABLE TO EXECUTE\n");
		}
	}
	else if((argc[0][0] != 'e' && strcmp(argc[1], "double") == 0) || (argc[0][0] == 'e' && strcmp(argc[2], "double") == 0)){
		int sz;
		if(argc[0][0] == 'e'){sz = argv - 1;}
		else sz = argv;
		sz++;
		char *par[sz];
		
		if(argc[0][0] == 'e'){
			par[0] = "e";
			for(int i = 2; i < argv - 1; i++){
				par[i-1] = argc[i];
			}
		}
		else {
			par[0] = "e";
			for(int i = 1; i < argv - 1; i++){
				par[i] = argc[i];
			}
		}
		
		char s[30]; 
		sprintf(s, "%llu", ret);
		
		par[sz - 2] = s;
		par[sz - 1] = NULL;
		
		if(execv("./double", par) < 0){
			printf("UNABLE TO EXECUTE\n");
		}
	}
	else {
		printf("UNABLE TO EXECUTE\n");
	}
	
	return 0;
}
