#include <stdio.h>
#include <stdlib.h>
#include <libvirt/libvirt.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#define MIN(a, b) ((a) < (b) ? a : b)
#define MAX(a, b) ((a) > (b) ? a : b)

int is_exit = 0; // DO NOT MODIFY THIS VARIABLE

void CPUScheduler(virConnectPtr conn, int interval);

/*
DO NOT CHANGE THE FOLLOWING FUNCTION
*/
void signal_callback_handler()
{
	printf("Caught Signal");
	is_exit = 1;
}

/*
DO NOT CHANGE THE FOLLOWING FUNCTION
*/
int main(int argc, char *argv[])
{
	virConnectPtr conn;

	if (argc != 2)
	{
		printf("Incorrect number of arguments\n");
		return 0;
	}

	// Gets the interval passes as a command line argument and sets it as the STATS_PERIOD for collection of balloon memory statistics of the domains
	int interval = atoi(argv[1]);

	conn = virConnectOpen("qemu:///system");
	if (conn == NULL)
	{
		fprintf(stderr, "Failed to open connection\n");
		return 1;
	}

	// Get the total number of pCpus in the host
	signal(SIGINT, signal_callback_handler);

	while (!is_exit)
	// Run the CpuScheduler function that checks the CPU Usage and sets the pin at an interval of "interval" seconds
	{
		CPUScheduler(conn, interval);
		sleep(interval);
	}

	// Closing the connection
	virConnectClose(conn);
	return 0;
}

/* COMPLETE THE IMPLEMENTATION */
void CPUScheduler(virConnectPtr conn, int interval)
{
    // List all running domains
    virDomainPtr *domains = NULL;
    int num_domains = virConnectListAllDomains(conn, &domains, 0);
    if (num_domains < 1) {
        printf("No running domains found.\n");
        return;
    }

    // Get number of physical CPUs
    int num_pcpus = virNodeGetInfo(conn, NULL) == 0 ? 4 : 4; // fallback to 4
    virNodeInfo nodeinfo;
    virNodeGetInfo(conn, &nodeinfo);
    num_pcpus = nodeinfo.cpus;

    // For each domain, pin vCPU 0 round-robin across pCPUs
    for (int i = 0; i < num_domains; i++) {
        virDomainPtr dom = domains[i];
        int pcpu = i % num_pcpus;

        // Create pinning tuple (all False except target pCPU)
        unsigned char *cpumap = malloc(VIR_CPU_MAPLEN(num_pcpus));
        memset(cpumap, 0, VIR_CPU_MAPLEN(num_pcpus));
        VIR_USE_CPU(cpumap, pcpu);

        // Pin vCPU 0 to pCPU
        if (virDomainPinVcpu(dom, 0, cpumap, VIR_CPU_MAPLEN(num_pcpus)) == -1) {
            fprintf(stderr, "Failed to pin vCPU 0 of %s\n", virDomainGetName(dom));
        }

        free(cpumap);
    }

    // Free domains
    for (int i = 0; i < num_domains; i++) {
        virDomainFree(domains[i]);
    }
    free(domains);	
}

