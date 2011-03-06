#ifndef _LS_CLIENT_
#define _LS_CLIENT_

#include <lscp/client.h>
#include <lscp/device.h>
#include <QList>

class QString;
class LSClient 
{
	public:
		LSClient(const char* host = "localhost", int port = 8888);
		~LSClient();
		bool initClient();
		void testClient();
		QList<QList<int>*>* getKeyBindings(QString filename, int instr);
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

	int client_test_status ( lscp_status_t s );
	int client_test_channel_info ( lscp_channel_info_t *pChannelInfo );

//END example_client.c

};

#endif
