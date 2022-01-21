#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

struct team {
  char name[15];
  int matchpipe[2];
  int commpipe[2];
};

struct team teams[8];
int test = 1;

const char *team_names[] = {
  "India", "Australia", "New Zealand", "Sri Lanka",   // Group A
  "Pakistan", "South Africa", "England", "Bangladesh" // Group B
};

int main(int argv, char *argc[])
{   
    for(int j = 0; j < 8; j++){
        strcpy(teams[j].name, team_names[j]);
        // pipe(teams[j].matchpipe);
        // pipe(teams[j].commpipe);
    }
    int team1 = 0, team2 = 1;
    char addrOut[300] = "test/", testNum[20], *tmp = "/out/";
    int pos = 0, ind = 0;
    while(addrOut[pos] != '\0'){pos++;}
    sprintf(testNum, "%d", test);
    while(testNum[ind] != '\0'){addrOut[pos] = testNum[ind]; pos++; ind++;}
    ind = 0;
    while(tmp[ind] != '\0'){addrOut[pos] = tmp[ind]; pos++; ind++;}

    ind = 0;
    while(teams[team1].name[ind] != '\0'){addrOut[pos] = teams[team1].name[ind]; pos++; ind++;}
    addrOut[pos] = 'v'; pos++;
    ind = 0;
    while(teams[team2].name[ind] != '\0'){addrOut[pos] = teams[team2].name[ind]; pos++; ind++;}
    if(team1 / 4 != team2 / 4){
        char *fin = "-Final";
        ind = 0;
        while(fin[ind] != '\0'){addrOut[pos] = fin[ind]; pos++; ind++;}
        addrOut[pos] = '\0';
    }
    // printf("%d, %d", team1%4, team2%4);
    int nwFile = open(addrOut, O_RDWR | O_CREAT, 0644);
    // puts(addrOut);
    char buf1[200];
    char *inn1 = "Innings 1: ", *inn2 = "Innings 2:", *bats = " bats\n";
    buf1[0] = 0;
    strcat(buf1, inn1);
    strcat(buf1, teams[team1].name);
    strcat(buf1, bats);
    pos = 0; while(buf1[pos] != '\0')pos++;
    write(nwFile, buf1, pos);
    // write(nwFile, "mm", 2);
    return 0;
}