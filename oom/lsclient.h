#ifndef _LS_CLIENT_
#define _LS_CLIENT_

#include <lscp/client.h>
#include <lscp/device.h>

	static int g_test_step  = 0;
	static int g_test_count = 0;
	static int g_test_fails = 0;

class LSClient 
{
	public:
		LSClient(const char* host = "localhost", int port = 8888);
		~LSClient();
		bool initClient();
		void testClient();
	private:
		lscp_client_t* _client;
		const char* _hostname;
		int _port;

//The following code was taken from the liblscp example_client.c	
	typedef int *                        isplit;
	typedef char **                      szsplit;
	typedef lscp_status_t                status;
	typedef lscp_driver_info_t *         driver_info;
	typedef lscp_device_info_t *         device_info;
	typedef lscp_device_port_info_t *    device_port_info;
	typedef lscp_param_info_t  *         param_info;
	typedef lscp_server_info_t *         server_info;
	typedef lscp_engine_info_t *         engine_info;
	typedef lscp_channel_info_t *        channel_info;
	typedef lscp_buffer_fill_t *         buffer_fill;
	typedef lscp_fxsend_info_t *         fxsend_info;
	typedef lscp_midi_instrument_t *     midi_instruments;
	typedef lscp_midi_instrument_info_t *midi_instrument_info;

	void  client_test_start   ( clock_t *pclk ) { *pclk = clock(); }
	float client_test_elapsed ( clock_t *pclk ) { return (float) ((long) clock() - *pclk) / (float) CLOCKS_PER_SEC; }
	int client_test_params ( lscp_param_t *pParams );
	int client_test_device_port_info ( lscp_device_port_info_t *pDevicePortInfo );
	int client_test_server_info ( lscp_server_info_t *pServerInfo );
	int client_test_szsplit ( char **ppszSplit );
	int client_test_isplit ( int *piSplit );
	int client_test_status ( lscp_status_t s );
	int client_test_size_t ( int i );


#define CLIENT_TEST(p, t, x) { clock_t c; void *v; g_test_count++; \
	printf("\n" #x ":\n"); client_test_start(&c); v = (void *) (x); \
	printf("  elapsed=%gs  errno=%d  result='%s...' ret=", \
		client_test_elapsed(&c), \
		lscp_client_get_errno(p), \
		lscp_client_get_result(p)); \
	if (client_test_##t((t)(v))) { g_test_fails++; getchar(); } \
	else if (g_test_step) getchar(); }
//END example_client.c

};

#endif
