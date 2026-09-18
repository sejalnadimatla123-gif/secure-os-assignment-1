#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#define _STRING(X) #X
#define STRING(X) _STRING(X)

#define OPEN_FAILURE -1
#define EXEC_FAILURE -2

#define APP_NAME_MAX 256

const bool IsValidApplication(const char * app_name, const char ** app_list, const int app_count)
{

	for (int i = 0; i < app_count; i++)
	{
		if (strcmp(app_name, app_list[i]) == 0)
		{
			return true;
		}
	}

	return false;
}

const bool ExecuteApplication(const char * app_path)
{
	int threadId = fork();

	if (threadId != 0 && threadId != -1)
	{
		return true;
	}

	else
	{
		int executionSuccess;
		executionSuccess = execl(app_path, "", (char *) NULL);

		if (executionSuccess != 0)
		{
			printf("\nCould not launch application\n");
			return false;
		}
	}

	return true;
}

int main()
{
	// Making this a set will be better for scalability
	const char * applicationWhitelist[] =
	{
		"gnome-calculator",
		"gnome-calendar",
		"gnome-screenshot",
		"gnome-terminal",
		"gnome-system-monitor",
		"libreoffice-writer",
		"libreoffice-calc",
		"libreoffice-draw",
		"libreoffice-impress"
	};

	const int applicationCount = sizeof(applicationWhitelist) / sizeof(char*);

	char * userInput = (char *) malloc(sizeof(char) * (APP_NAME_MAX + 1));

	printf("What Application Would You Like to Start?\n> ");
	fflush(stdin);

	int readCharacter = scanf(" %" STRING(APP_NAME_MAX) "s", userInput);

	const bool requestedAppValidity = IsValidApplication((const char *) userInput, applicationWhitelist, applicationCount);

	if (requestedAppValidity == false)
	{
		printf("\nThe provided name does not match a valid application name\n");
		return OPEN_FAILURE;
	}

	char applicationPath[1024] = "/bin/\0";
	strcat(applicationPath, userInput);

	ExecuteApplication((const char *) applicationPath);
	
	return 0;
}