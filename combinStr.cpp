// split.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <string.h>
#include <stdio.h>
#include <iostream>


int _tmain(int argc, _TCHAR* argv[])
{
	char input[256] = {0};

	char *if1 = "veth0";
	char *if2 = "veth1";
	char *if3 = "veth2";

	strcat(input, if1);
	strcat(input, "&&");
	strcat(input, if2);
	strcat(input, "&&");
	strcat(input, if3);
	strcat(input, "&&");

	std::cout << input << std::endl;
	return 0;
}

