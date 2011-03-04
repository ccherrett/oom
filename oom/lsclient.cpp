#include "config.h"
#ifdef LSCP_SUPPORT
#include "lsclient.h"

LSClient::LSClient(const char* host, int p)
{
	_hostname = host;
	_port = p;
}

LSClient::~LSClient()
{
}

lscp_status_t client_callback ( lscp_client_t *pClient, lscp_event_t event, const char *pchData, int cchData, void *pvData )
{
	lscp_status_t ret = LSCP_FAILED;

	char *pszData = (char *) malloc(cchData + 1);
	if (pszData) {
		memcpy(pszData, pchData, cchData);
		pszData[cchData] = (char) 0;
		printf("client_callback: event=%s (0x%04x) [%s]\n", lscp_event_to_text(event), (int) event, pszData);
		free(pszData);
		ret = LSCP_OK;
	}

	return ret;
}

bool LSClient::initClient()
{
	_client = ::lscp_client_create(_hostname, _port, client_callback, this);
	if(_client == NULL)
	{
		return false;
	}
	return true;
}

int LSClient::client_test_size_t ( int i )
{
	printf("%d\n", i);
	return (i >= 0 ? 0 : 1);
}


int LSClient::client_test_status ( lscp_status_t s )
{
	const char *pszStatus;

	switch (s) {
	case LSCP_OK:         pszStatus = "OK";       break;
	case LSCP_FAILED:     pszStatus = "FAILED";   break;
	case LSCP_ERROR:      pszStatus = "ERROR";    break;
	case LSCP_WARNING:    pszStatus = "WARNING";  break;
	case LSCP_TIMEOUT:    pszStatus = "TIMEOUT";  break;
	case LSCP_QUIT:       pszStatus = "QUIT";     break;
	default:              pszStatus = "NONE";     break;
	}
	printf("%s\n", pszStatus);
	return (s == LSCP_OK ? 0 : 1);
}


int LSClient::client_test_isplit ( int *piSplit )
{
	int i;

	printf("{");
	for (i = 0; piSplit && piSplit[i] >= 0; i++) {
		if (i > 0)
			printf(",");
		printf(" %d", piSplit[i]);
	}
	printf(" }\n");
	return 0;
}


int LSClient::client_test_szsplit ( char **ppszSplit )
{
	int i;

	printf("{");
	for (i = 0; ppszSplit && ppszSplit[i]; i++) {
		if (i > 0)
			printf(",");
		printf(" %s", ppszSplit[i]);
	}
	printf(" }\n");
	return 0;
}


int LSClient::client_test_params ( lscp_param_t *pParams )
{
	int i;

	printf("{");
	for (i = 0; pParams && pParams[i].key; i++) {
		if (i > 0)
			printf(",");
		printf(" %s='%s'", pParams[i].key, pParams[i].value);
	}
	printf(" }\n");
	return 0;
}

int LSClient::client_test_device_port_info ( lscp_device_port_info_t *pDevicePortInfo )
{
	if (pDevicePortInfo == NULL) {
		printf("(nil)\n");
		return 1;
	}
	printf("{\n");
	printf("    device_port_info.name   = %s\n", pDevicePortInfo->name);
	printf("    device_port_info.params = "); client_test_params(pDevicePortInfo->params);
	printf("  }\n");
	return 0;
}

int LSClient::client_test_server_info ( lscp_server_info_t *pServerInfo )
{
	if (pServerInfo == NULL) {
		printf("(nil)\n");
		return 1;
	}
	printf("{\n");
	printf("    server_info.description      = %s\n", pServerInfo->description);
	printf("    server_info.version          = %s\n", pServerInfo->version);
	printf("    server_info.protocol_version = %s\n", pServerInfo->protocol_version);
	printf("  }\n");
	return 0;
}



void LSClient::testClient()
{
	const char **ppszAudioDrivers, **ppszMidiDrivers, **ppszEngines;
	const char *pszAudioDriver, *pszMidiDriver, *pszEngine = 0;
	int iAudioDriver, iMidiDriver, iEngine;
	int iAudio, iAudioDevice, iMidi, iMidiDevice;
	int iNewAudioDevice, iNewMidiDevice;
	int *piAudioDevices, *piMidiDevices;
	lscp_midi_instrument_t midi_instr;
	int i, j, k;

	CLIENT_TEST(_client, server_info, lscp_get_server_info(_client));
	CLIENT_TEST(_client, size_t, lscp_get_available_audio_drivers(_client));
	if (ppszAudioDrivers == NULL) {
		fprintf(stderr, "client_test: No audio drivers available.\n");
		return;
	}

	CLIENT_TEST(_client, size_t, lscp_get_available_midi_drivers(_client));
	CLIENT_TEST(_client, szsplit, ppszMidiDrivers = lscp_list_available_midi_drivers(_client));
	if (ppszMidiDrivers == NULL) {
		fprintf(stderr, "client_test: No MIDI drivers available.\n");
		return;
	}
}
//#else
void foobar()
{
}
#endif
