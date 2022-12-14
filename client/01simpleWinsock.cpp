#include "../common/common.h"

void testInitSocket() {
	INIT_SOCK

	printf("Init socket ok\n");

	DESTORY_SOCK
}

void testInterfaceAddr() {
	INIT_SOCK

	int ret;
	DWORD dwSize = 0;
	char ap[128] = { '\0' };
	PIP_ADAPTER_ADDRESSES adapters;

	GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, 0, 0, &dwSize);
	adapters = (PIP_ADAPTER_ADDRESSES)malloc(dwSize);
	GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, 0, adapters, &dwSize);

	// retrieve info
	while (adapters) {
		PIP_ADAPTER_UNICAST_ADDRESS address = adapters->FirstUnicastAddress;
		wprintf(L"\nAdapter name: %s\n", adapters->FriendlyName);
			
		while (address) {
			printf("\t%s", address->Address.lpSockaddr->sa_family == AF_INET ? "IPv4" : "IPv6");
			getnameinfo(address->Address.lpSockaddr, address->Address.iSockaddrLength, ap, sizeof(ap), 0, 0, NI_NUMERICHOST);
			printf("\t%s\n", ap);
			address = address->Next;
			memset(ap, 0, sizeof(ap));
		}
			
		adapters = adapters->Next;
	}
	free(adapters);

	DESTORY_SOCK
}