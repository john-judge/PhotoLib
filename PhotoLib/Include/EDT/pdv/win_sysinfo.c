#include <windows.h>
#include <stdio.h>

/* win_sysinfo: get windows specific system info.
 * That includes screen res, user name, os type (98/nt/etc),
 * and memory size.
 */

int main (int argc, char **argv) 
{
	/* Screen Resolution */
	HDC scr = GetDC(0);
	printf("Screen: %d x %d Pixels, %d Bits/Pixel\n", 
		GetDeviceCaps(scr, HORZRES),
		GetDeviceCaps(scr, VERTRES),
		GetDeviceCaps(scr, BITSPIXEL)
	);

	ReleaseDC(0,scr);

	/* User Name */
	{
#define SIZE 100
	DWORD size = SIZE;
	char user_name[SIZE]; 
	GetUserName(user_name, &size);
	printf("Current user: %s\n", user_name);
	}

	/* OS Info */
	{
	OSVERSIONINFO os;
	os.dwOSVersionInfoSize = sizeof(os);
	GetVersionEx(&os);
	printf("Generic Platform: ");
	switch(os.dwPlatformId) {
		case VER_PLATFORM_WIN32_NT: puts("WinNT"); break;
		case VER_PLATFORM_WIN32_WINDOWS: puts("Win95"); break;
		case VER_PLATFORM_WIN32s: puts("Win3.1"); break;
		default: printf("unknown. (dwPlatformId=%d)\n", os.dwPlatformId); break;
	}
	printf("Specific Windows version: %d.%d (Build %d) Platform: %d. ",
		os.dwMajorVersion, os.dwMinorVersion, 
		LOWORD(os.dwBuildNumber)
	);
	printf("(Windows ");
	switch(os.dwMajorVersion) {
		case 5:
		switch(os.dwMinorVersion) {
			case 0: printf("2000"); break;
			case 1: printf("XP"); break;
			case 2: printf("Server 2003"); break;
		}
		break;
		case 4:
		switch(os.dwMinorVersion) {
			case 90: printf("ME"); break;
			case 10: printf("98"); break;
			case 0:  
			switch(os.dwPlatformId) {
				case VER_PLATFORM_WIN32_WINDOWS: 
				printf("95"); break;
				case VER_PLATFORM_WIN32_NT:
				printf("NT 4.0"); break;
			}
			break;
		}
		break;
		case 3:
		printf("NT 3.51"); break;
	}
	puts(")");

	/* Service Pack */
	if (*os.szCSDVersion) printf("%s\n", os.szCSDVersion);

	printf("Debugging kernel: %s\n",GetSystemMetrics(SM_DEBUG)?"yes":"no");
	printf("Secure system: %s\n",GetSystemMetrics(SM_SECURE)?"yes":"no");

	
	} /* end os info */
	

	/* System Info */
	{
	char buffer[100];
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	puts("System:");
	printf("%d processors level %d revision %04x - ",
		info.dwNumberOfProcessors,
		info.wProcessorLevel,
		info.wProcessorRevision);
	switch (info.wProcessorArchitecture) {
		case PROCESSOR_ARCHITECTURE_INTEL: puts("Intel"); break;
		case PROCESSOR_ARCHITECTURE_MIPS: puts("MIPS"); break;
		case PROCESSOR_ARCHITECTURE_ALPHA: puts("DEC Alpha"); break;
		case PROCESSOR_ARCHITECTURE_PPC: puts("IBM Power PC"); break;
		default: puts("unknown Processor"); break;
	}
	ExpandEnvironmentStrings("   (%PROCESSOR_IDENTIFIER%)",buffer,sizeof(buffer));
	puts(buffer);
	printf("Slow machine: %s\n",GetSystemMetrics(SM_SLOWMACHINE)?"yes":"no");
	printf("addresses %#x - %#x, pages %d KB, alloc chunks %d KB\n",
		info.lpMinimumApplicationAddress,
		info.lpMaximumApplicationAddress,
		info.dwPageSize/1024,
		info.dwAllocationGranularity/1024);
	} /* end system info */

	/* Memory Info */
	{
	/* width used in printfs below. */
	#define WIDTH 7 

	//use to change the divisor from Kb to Mb.
	#define DIV 1024
	//#define DIV 1

	char *divisor = "K";
	//char *divisor = "";


	MEMORYSTATUS mem;
	GlobalMemoryStatus(&mem);
	printf("%ld percent of memory is in use.\n",
		mem.dwMemoryLoad);
	printf("There are %*ld total %sbytes of physical memory.\n",
		WIDTH, mem.dwTotalPhys/DIV, divisor);
	printf("There are %*ld free %sbytes of physical memory.\n",
		WIDTH, mem.dwAvailPhys/DIV, divisor);
	} /* end memory info */
	
	
		
	
	return 0;
}

