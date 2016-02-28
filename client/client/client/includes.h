#define OPENSSL_EXTRA 1

#pragma once
#define RECEIVE_PORT "1235"
//#define CHAIN_CERT "../../certs/ca-chain.cert.pem"
//#define CHAIN_CERT "../../certs/www.example.com.cert.pem"
#define CHAIN_CERT "../../certs/ca-cert.pem"

const int UDP_BROADCAST_PORT = 1234;
const int PACKET_SIZE = 1024;
const int HANDSHAKE_PORT = 1236;

//static INLINE int PasswordCallBack(char* passwd, int sz, int rw, void* userdata)
//{
//	(void)rw;
//	(void)userdata;
//	strncpy(passwd, "yassl123", sz);
//	return 8;
//}
//
//static INLINE int ChangeToWolfRoot(void)
//{
//#if !defined(NO_FILESYSTEM) 
//	int depth;
//	XFILE file;
//	char path[MAX_PATH];
//	XMEMSET(path, 0, MAX_PATH);
//
//	for (depth = 0; depth <= MAX_WOLF_ROOT_DEPTH; depth++) {
//		file = XFOPEN(ntruKey, "rb");
//		//file = XFOPEN("hellomynameis.txt", "w");
//		if (file != XBADFILE) {
//			XFCLOSE(file);
//			return depth;
//		}
//#ifdef USE_WINDOWS_API
//		XSTRNCAT(path, "..\\", MAX_PATH - XSTRLEN(path));
//		SetCurrentDirectoryA(path);
//#else
//		XSTRNCAT(path, "../", MAX_PATH - XSTRLEN(path));
//		if (chdir(path) < 0) {
//			printf("chdir to %s failed\n", path);
//			break;
//		}
//#endif
//	}
//
//	std::cout << "wolf root not found" << std::endl;;
//	return -1;
//#else
//	return 0;
//#endif
//}
//
//static INLINE void tcp_connect(SOCKET_T* sockfd, const char* ip, word16 port,
//	int udp, WOLFSSL* ssl)
//{
//	SOCKADDR_IN_T addr;
//	build_addr(&addr, ip, port, udp);
//	if (udp) {
//		wolfSSL_dtls_set_peer(ssl, &addr, sizeof(addr));
//	}
//	tcp_socket(sockfd, udp);
//
//	if (!udp) {
//		if (connect(*sockfd, (const struct sockaddr*)&addr, sizeof(addr)) != 0)
//			err_sys("tcp connect failed");
//	}
//}
//
//static INLINE void build_addr(SOCKADDR_IN_T* addr, const char* peer,
//	word16 port, int udp)
//{
//	int useLookup = 0;
//	(void)useLookup;
//	(void)udp;
//
//	memset(addr, 0, sizeof(SOCKADDR_IN_T));
//
//#ifndef TEST_IPV6
//	/* peer could be in human readable form */
//	if ((peer != INADDR_ANY) && isalpha((int)peer[0])) {
//#if defined(WOLFSSL_MDK_ARM) || defined(WOLFSSL_KEIL_TCP_NET)
//		int err;
//		struct hostent* entry = gethostbyname(peer, &err);
//#elif defined(WOLFSSL_TIRTOS)
//		struct hostent* entry = DNSGetHostByName(peer);
//#elif defined(WOLFSSL_VXWORKS)
//		struct hostent* entry = (struct hostent*)hostGetByName((char*)peer);
//#else
//		struct hostent* entry = gethostbyname(peer);
//#endif
//
//		if (entry) {
//			memcpy(&addr->sin_addr.s_addr, entry->h_addr_list[0],
//				entry->h_length);
//			useLookup = 1;
//		}
//		else
//			err_sys("no entry for host");
//	}
//#endif
//
//
//#ifndef TEST_IPV6
//#if defined(WOLFSSL_MDK_ARM) || defined(WOLFSSL_KEIL_TCP_NET)
//	addr->sin_family = PF_INET;
//#else
//	addr->sin_family = AF_INET_V;
//#endif
//	addr->sin_port = htons(port);
//	if (peer == INADDR_ANY)
//		addr->sin_addr.s_addr = INADDR_ANY;
//	else {
//		if (!useLookup)
//			addr->sin_addr.s_addr = inet_addr(peer);
//	}
//#else
//	addr->sin6_family = AF_INET_V;
//	addr->sin6_port = htons(port);
//	if (peer == INADDR_ANY)
//		addr->sin6_addr = in6addr_any;
//	else {
//#ifdef HAVE_GETADDRINFO
//		struct addrinfo  hints;
//		struct addrinfo* answer = NULL;
//		int    ret;
//		char   strPort[80];
//
//		memset(&hints, 0, sizeof(hints));
//
//		hints.ai_family = AF_INET_V;
//		hints.ai_socktype = udp ? SOCK_DGRAM : SOCK_STREAM;
//		hints.ai_protocol = udp ? IPPROTO_UDP : IPPROTO_TCP;
//
//		SNPRINTF(strPort, sizeof(strPort), "%d", port);
//		strPort[79] = '\0';
//
//		ret = getaddrinfo(peer, strPort, &hints, &answer);
//		if (ret < 0 || answer == NULL)
//			err_sys("getaddrinfo failed");
//
//		memcpy(addr, answer->ai_addr, answer->ai_addrlen);
//		freeaddrinfo(answer);
//#else
//		printf("no ipv6 getaddrinfo, loopback only tests/examples\n");
//		addr->sin6_addr = in6addr_loopback;
//#endif
//	}
//#endif
//}
//
//static INLINE void tcp_socket(SOCKET_T* sockfd, int udp)
//{
//	if (udp)
//		*sockfd = socket(AF_INET_V, SOCK_DGRAM, 0);
//	else
//		*sockfd = socket(AF_INET_V, SOCK_STREAM, 0);
//
//	if (WOLFSSL_SOCKET_IS_INVALID(*sockfd)) {
//		err_sys("socket failed\n");
//	}
//
//#ifndef USE_WINDOWS_API
//#ifdef SO_NOSIGPIPE
//	{
//		int       on = 1;
//		socklen_t len = sizeof(on);
//		int       res = setsockopt(*sockfd, SOL_SOCKET, SO_NOSIGPIPE, &on, len);
//		if (res < 0)
//			err_sys("setsockopt SO_NOSIGPIPE failed\n");
//	}
//#elif defined(WOLFSSL_MDK_ARM) || defined (WOLFSSL_TIRTOS) ||\
//                                          defined(WOLFSSL_KEIL_TCP_NET)
//	/* nothing to define */
//#else  /* no S_NOSIGPIPE */
//	signal(SIGPIPE, SIG_IGN);
//#endif /* S_NOSIGPIPE */
//
//#if defined(TCP_NODELAY)
//	if (!udp)
//	{
//		int       on = 1;
//		socklen_t len = sizeof(on);
//		int       res = setsockopt(*sockfd, IPPROTO_TCP, TCP_NODELAY, &on, len);
//		if (res < 0)
//			err_sys("setsockopt TCP_NODELAY failed\n");
//	}
//#endif
//#endif  /* USE_WINDOWS_API */
//}