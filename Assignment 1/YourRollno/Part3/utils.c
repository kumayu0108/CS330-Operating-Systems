#include "wc.h"
#include <unistd.h>


extern struct team teams[NUM_TEAMS];
extern int test;
extern int finalTeam1;
extern int finalTeam2;

int processType = HOST;
const char *team_names[] = {
  "India", "Australia", "New Zealand", "Sri Lanka",   // Group A
  "Pakistan", "South Africa", "England", "Bangladesh" // Group B
};


void teamPlay(void)
{
  char addrIn[400] = "test/", testNum[20], *tmp = "/inp/";
  int pos = 0, ind = 0;
  while(addrIn[pos] != '\0'){pos++;}
  sprintf(testNum, "%d", test);
  while(testNum[ind] != '\0'){addrIn[pos] = testNum[ind]; pos++; ind++;}
  ind = 0;
  while(tmp[ind] != '\0'){addrIn[pos] = tmp[ind]; pos++; ind++;}
  ind = 0;
  while(teams[processType].name[ind] != '\0'){addrIn[pos] = teams[processType].name[ind]; pos++; ind++;}
  addrIn[pos] = '\0';

  int fd = open(addrIn, O_RDONLY);
  
  char buf[400];
  while(1){
    read(fd, buf, 1);
    write(teams[processType].matchpipe[1], buf, 1);
    read(teams[processType].commpipe[0], buf, 1);
    
    if(buf[0] == '2'){
      exit(0);
    }
  }
  exit(0);
}

void endTeam(int teamID)
{
  write(teams[teamID].commpipe[1], "2", 1);
}

int match(int team1, int team2)
{
  
  char buf1[300], buf2[300];
  write(teams[team1].commpipe[1], "1", 1);
  write(teams[team2].commpipe[1], "1", 1);
  read(teams[team1].matchpipe[0], buf1, 1);
  read(teams[team2].matchpipe[0], buf2, 1);

  int firstBat, secondBat;
  if((buf1[0] - '0' + buf2[0] - '0') % 2 != 0){
    firstBat = team1;
    secondBat = team2;
  }
  else {
    firstBat = team2;
    secondBat = team1;
  }
  char addrOut[400] = "test/", testNum[20], *tmp = "/out/";
  int pos = 0, ind = 0;
  while(addrOut[pos] != '\0'){pos++;}
  sprintf(testNum, "%d", test);
  while(testNum[ind] != '\0'){addrOut[pos] = testNum[ind]; pos++; ind++;}
  ind = 0;
  while(tmp[ind] != '\0'){addrOut[pos] = tmp[ind]; pos++; ind++;}
  ind = 0;
  while(teams[firstBat].name[ind] != '\0'){addrOut[pos] = teams[firstBat].name[ind]; pos++; ind++;}
  addrOut[pos] = 'v'; pos++;
  ind = 0;
  while(teams[secondBat].name[ind] != '\0'){addrOut[pos] = teams[secondBat].name[ind]; pos++; ind++;}
  addrOut[pos] = '\0';
  if(team1 / 4 != team2 / 4){
    char *fin = "-Final";
    ind = 0;
    while(fin[ind] != '\0'){addrOut[pos] = fin[ind]; pos++; ind++;}
    addrOut[pos] = '\0';
  }
  printf("%s\n", addrOut);
  
  int nwFile = open(addrOut, O_RDWR | O_CREAT, 0644);
  printf("%d\n", nwFile);
  char *inn1 = "Innings 1: ", *inn2 = "Innings 2: ", *bats = " bats\n";
  buf1[0] = 0;
  strcat(buf1, inn1);
  strcat(buf1, teams[firstBat].name);
  strcat(buf1, bats);
  pos = 0; while(buf1[pos] != '\0')pos++;
  write(nwFile, buf1, pos);
  int balls, totRuns1 = 0, lastBats = 0, wickets = 0;
  for(balls = 0 ; balls < 120; balls++){
    write(teams[firstBat].commpipe[1], "1", 1);
    write(teams[secondBat].commpipe[1], "1", 1);
    read(teams[firstBat].matchpipe[0], buf1, 1);
    read(teams[secondBat].matchpipe[0], buf2, 1);
    if(buf1[0] != buf2[0]){
      totRuns1 += atoi(buf1);
      lastBats += atoi(buf1);
    }
    else {
      wickets++;
      char batsmanNum[20], batsmanRuns[20];
      sprintf(batsmanNum, "%d", wickets); sprintf(batsmanRuns, "%d", lastBats);
      buf1[0] = 0;
      strcat(buf1, batsmanNum);
      strcat(buf1, ":");
      strcat(buf1, batsmanRuns);
      strcat(buf1, "\n");
      pos = 0; while(buf1[pos] != '\0')pos++;
      write(nwFile, buf1, pos);
      lastBats = 0;
      if(wickets == 10)break;
    }
  }
  if(wickets < 10){
    char batsmanNum[20], batsmanRuns[20];
    sprintf(batsmanNum, "%d", wickets + 1); sprintf(batsmanRuns, "%d", lastBats);
    buf1[0] = 0;
    strcat(buf1, batsmanNum);
    strcat(buf1, ":");
    strcat(buf1, batsmanRuns);
    strcat(buf1, "*\n");
    pos = 0; while(buf1[pos] != '\0')pos++;
    write(nwFile, buf1, pos);
  }
  buf1[0] = 0; buf2[0] = 0;
  sprintf(buf2, "%d", totRuns1);
  strcat(buf1, teams[firstBat].name);
  strcat(buf1, " Total: ");
  strcat(buf1, buf2);
  strcat(buf1, "\n\n");
  pos = 0; while(buf1[pos] != '\0')pos++;
  write(nwFile, buf1, pos);

  buf1[0] = 0;
  strcat(buf1, inn2);
  strcat(buf1, teams[secondBat].name);
  strcat(buf1, bats);
  pos = 0; while(buf1[pos] != '\0')pos++;
  write(nwFile, buf1, pos);

  int totRuns2 = 0; wickets = 0; lastBats = 0;
  for(balls = 0 ; balls < 120; balls++){
    write(teams[firstBat].commpipe[1], "1", 1);
    write(teams[secondBat].commpipe[1], "1", 1);
    read(teams[firstBat].matchpipe[0], buf2, 1);
    read(teams[secondBat].matchpipe[0], buf1, 1);
    if(buf1[0] != buf2[0]){
      totRuns2 += atoi(buf1);
      lastBats += atoi(buf1);
      if(totRuns2 > totRuns1)break;
    }
    else {
      wickets++;
      char batsmanNum[20], batsmanRuns[20];
      sprintf(batsmanNum, "%d", wickets); sprintf(batsmanRuns, "%d", lastBats);
      buf1[0] = 0;
      strcat(buf1, batsmanNum);
      strcat(buf1, ":");
      strcat(buf1, batsmanRuns);
      strcat(buf1, "\n");
      pos = 0; while(buf1[pos] != '\0')pos++;
      write(nwFile, buf1, pos);
      lastBats = 0;
      if(wickets == 10)break;
    }
  }
  if(wickets < 10){
    char batsmanNum[20], batsmanRuns[20];
    sprintf(batsmanNum, "%d", wickets + 1); sprintf(batsmanRuns, "%d", lastBats);
    buf1[0] = 0;
    strcat(buf1, batsmanNum);
    strcat(buf1, ":");
    strcat(buf1, batsmanRuns);
    strcat(buf1, "*\n");
    pos = 0; while(buf1[pos] != '\0')pos++;
    write(nwFile, buf1, pos);
  }
  buf1[0] = 0; buf2[0] = 0;
  sprintf(buf2, "%d", totRuns2);
  strcat(buf1, teams[secondBat].name);
  strcat(buf1, " Total: ");
  strcat(buf1, buf2);
  strcat(buf1, "\n\n");
  pos = 0; while(buf1[pos] != '\0')pos++;
  write(nwFile, buf1, pos);

  if(totRuns2 > totRuns1){
    buf1[0] = 0; buf2[0] = 0;
    sprintf(buf2, "%d", 10 - wickets);
    strcat(buf1, teams[secondBat].name);
    strcat(buf1, " beats ");
    strcat(buf1, teams[firstBat].name);
    strcat(buf1, " by ");
    strcat(buf1, buf2);
    strcat(buf1, " wickets\n");
    pos = 0; while(buf1[pos] != '\0')pos++;
    write(nwFile, buf1, pos);
    return secondBat;
  }
  else if(totRuns2 < totRuns1){
    buf1[0] = 0; buf2[0] = 0;
    sprintf(buf2, "%d", totRuns1 - totRuns2);
    strcat(buf1, teams[firstBat].name);
    strcat(buf1, " beats ");
    strcat(buf1, teams[secondBat].name);
    strcat(buf1, " by ");
    strcat(buf1, buf2);
    strcat(buf1, " runs\n");
    pos = 0; while(buf1[pos] != '\0')pos++;
    write(nwFile, buf1, pos);
    return firstBat;
  }
  else {
    buf1[0] = 0; buf2[0] = 0;
    strcat(buf1, "TIE: ");
    strcat(buf1, teams[team1].name);
    strcat(buf1, " beats ");
    strcat(buf1, teams[team2].name);
    strcat(buf1, "\n");
    pos = 0; while(buf1[pos] != '\0')pos++;
    write(nwFile, buf1, pos);
    return team1;
  }
	return 0;
}

void spawnTeams(void)
{
  for(int j = 0; j < 8; j++){
    strcpy(teams[j].name, team_names[j]);
    // puts(teams[j].name);
    pipe(teams[j].matchpipe);
    pipe(teams[j].commpipe);
  }
  for(int j = 0; j < 8; j++){
    if(processType == HOST){
      int pid = fork();
      if(pid == 0){
        processType = j;
        close(teams[j].matchpipe[0]);
        close(teams[j].commpipe[1]);
      }
      else {
        close(teams[j].matchpipe[1]);
        close(teams[j].commpipe[0]);
      }
    }
  }
  if(processType == HOST)return;
  teamPlay();
}

void conductGroupMatches(void)
{
  int group[2][2];
  pipe(group[0]); pipe(group[1]);
  for(int j = 0; j < 2; j++){
    if(processType == HOST){
      int pid = fork();
      if(pid == 0){
        close(group[j][0]);
        processType = 10 + j;

        int wins[4];
        wins[0] = 0; wins[1] = 0; wins[2] = 0; wins[3] = 0;
        for(int team1 = 4*j; team1 < 4*j + 4; team1++){
          for(int team2 = team1 + 1; team2 < 4*j + 4; team2++){
            wins[match(team1, team2) - 4*j]++;
          }
        }
        int mxWins = 0;
        int winner; //Compute winner
        
        for(int i = 0; i < 4; i++){
          if(mxWins < wins[i]){
            mxWins = wins[i];
            winner = i + 4*j;
          }
        }

        for(int teamm = 4*j; teamm < 4*j + 4; teamm++){
          if(teamm == winner)continue;
          endTeam(teamm);
        }
        char tmp[40];
        tmp[0] = winner + '0';
        write(group[j][1], tmp, 1);
        exit(0);
      }
    }
  }
  if(processType == HOST){
    close(group[0][1]);
    close(group[1][1]);
    char tmp[40];

    wait(NULL);
    read(group[0][0], tmp, 1);
    finalTeam1 = atoi(tmp);
    read(group[1][0], tmp, 1);
    finalTeam2 = atoi(tmp);
  }
}
