#include<ulib.h>

void do_end_handler()
{
	printf("In End Handler\n");
}

int fn_2()
{
	printf("In fn2\n");
	return 0;
}

int fn_1()
{
	printf("In fn1\n");
	return 0;
}


int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{

  printf("DUMMY Testcase\n In Main\n");
	fn_1();
	fn_2();
  printf("Exiting");

	return 0;
}
