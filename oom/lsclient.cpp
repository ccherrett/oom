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

lscp_status_t client_callback ( lscp_client_t* /*_client*/, lscp_event_t event, const char *pchData, int cchData, void* /*pvData*/ )
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

int LSClient::client_test_midi_instruments ( lscp_midi_instrument_t *pInstrs )
{
	int i;

	printf("{");
	for (i = 0; pInstrs && pInstrs[i].prog >= 0; i++) {
		if (i > 0)
			printf(",");
		printf("{%d,%d,%d}", pInstrs[i].map, pInstrs[i].bank, pInstrs[i].prog);
	}
	printf(" }\n");
	return 0;
}


int LSClient::client_test_param_info ( lscp_param_info_t *pParamInfo )
{
	const char *pszType;

	if (pParamInfo == NULL) {
		printf("(nil)\n");
		return 1;
	}
	switch (pParamInfo->type) {
	case LSCP_TYPE_BOOL:      pszType = "BOOL";   break;
	case LSCP_TYPE_INT:       pszType = "INT";    break;
	case LSCP_TYPE_FLOAT:     pszType = "FLOAT";  break;
	case LSCP_TYPE_STRING:    pszType = "STRING"; break;
	default:                  pszType = "NONE";   break;
	}
	printf("{\n");
	printf("    param_info.type          = %d (%s)\n", (int) pParamInfo->type, pszType);
	printf("    param_info.description   = %s\n", pParamInfo->description);
	printf("    param_info.mandatory     = %d\n", pParamInfo->mandatory);
	printf("    param_info.fix           = %d\n", pParamInfo->fix);
	printf("    param_info.multiplicity  = %d\n", pParamInfo->multiplicity);
	printf("    param_info.depends       = "); client_test_szsplit(pParamInfo->depends);
	printf("    param_info.defaultv      = %s\n", pParamInfo->defaultv);
	printf("    param_info.range_min     = %s\n", pParamInfo->range_min);
	printf("    param_info.range_max     = %s\n", pParamInfo->range_max);
	printf("    param_info.possibilities = "); client_test_szsplit(pParamInfo->possibilities);
	printf("  }\n");
	return 0;
}


int LSClient::client_test_driver_info ( lscp_driver_info_t *pDriverInfo )
{
	if (pDriverInfo == NULL) {
		printf("(nil)\n");
		return 1;
	}
	printf("{\n");
	printf("    driver_info.description = %s\n", pDriverInfo->description);
	printf("    driver_info.version     = %s\n", pDriverInfo->version);
	printf("    driver_info.parameters  = "); client_test_szsplit(pDriverInfo->parameters);
	printf("  }\n");
	return 0;
}


int LSClient::client_test_device_info ( lscp_device_info_t *pDeviceInfo )
{
	if (pDeviceInfo == NULL) {
		printf("(nil)\n");
		return 1;
	}
	printf("{\n");
	printf("    device_info.driver = %s\n", pDeviceInfo->driver);
	printf("    device_info.params = "); client_test_params(pDeviceInfo->params);
	printf("  }\n");
	return 0;
}

int LSClient::client_test_engine_info ( lscp_engine_info_t *pEngineInfo )
{
	if (pEngineInfo == NULL) {
		printf("(nil)\n");
		return 1;
	}
	printf("{\n");
	printf("    engine_info.description = %s\n", pEngineInfo->description);
	printf("    engine_info.version     = %s\n", pEngineInfo->version);
	printf("  }\n");
	return 0;
}

int LSClient::client_test_channel_info ( lscp_channel_info_t *pChannelInfo )
{
	if (pChannelInfo == NULL) {
		printf("(nil)\n");
		return 1;
	}
	printf("{\n");
	printf("    channel_info.engine_name       = %s\n", pChannelInfo->engine_name);
	printf("    channel_info.audio_device      = %d\n", pChannelInfo->audio_device);
	printf("    channel_info.audio_channels    = %d\n", pChannelInfo->audio_channels);
	printf("    channel_info.audio_routing     = "); client_test_isplit(pChannelInfo->audio_routing);
	printf("    channel_info.instrument_file   = %s\n", pChannelInfo->instrument_file);
	printf("    channel_info.instrument_nr     = %d\n", pChannelInfo->instrument_nr);
	printf("    channel_info.instrument_name   = %s\n", pChannelInfo->instrument_name);
	printf("    channel_info.instrument_status = %d\n", pChannelInfo->instrument_status);
	printf("    channel_info.midi_device       = %d\n", pChannelInfo->midi_device);
	printf("    channel_info.midi_port         = %d\n", pChannelInfo->midi_port);
	printf("    channel_info.midi_channel      = %d\n", pChannelInfo->midi_channel);
	printf("    channel_info.midi_map          = %d\n", pChannelInfo->midi_map);
	printf("    channel_info.volume            = %g\n", pChannelInfo->volume);
	printf("    channel_info.mute              = %d\n", pChannelInfo->mute);
	printf("    channel_info.solo              = %d\n", pChannelInfo->solo);
	printf("  }\n");
	return 0;
}


int LSClient::client_test_fxsend_info ( lscp_fxsend_info_t *pFxSendInfo )
{
	if (pFxSendInfo == NULL) {
		printf("(nil)\n");
		return 1;
	}
	printf("{\n");
	printf("    fxsend_info.engine_name     = %s\n", pFxSendInfo->name);
	printf("    fxsend_info.midi_controller = %d\n", pFxSendInfo->midi_controller);
	printf("    fxsend_info.audio_routing   = "); client_test_isplit(pFxSendInfo->audio_routing);
	printf("    fxsend_info.level           = %g\n", pFxSendInfo->level);
	printf("  }\n");
	return 0;
}


int LSClient::client_test_buffer_fill ( lscp_buffer_fill_t *pBufferFill )
{
	if (pBufferFill == NULL) {
		printf("(nil)\n");
		return 1;
	}
	printf("{ <%p> }\n", pBufferFill);
	return 0;
}


int LSClient::client_test_load_mode ( lscp_load_mode_t load_mode )
{
	const char *pszLoadMode;

	switch (load_mode) {
	case LSCP_LOAD_ON_DEMAND:      pszLoadMode = "ON_DEMAND";      break;
	case LSCP_LOAD_ON_DEMAND_HOLD: pszLoadMode = "ON_DEMAND_HOLD"; break;
	case LSCP_LOAD_PERSISTENT:     pszLoadMode = "PERSISTENT";     break;
	default:                       pszLoadMode = "DEFAULT";        break;
	}
	printf("%s\n", pszLoadMode);
	return 0;
}


int LSClient::client_test_midi_instrument_info ( lscp_midi_instrument_info_t *pInstrInfo )
{
	if (pInstrInfo == NULL) {
		printf("(nil)\n");
		return 1;
	}
	printf("{\n");
	printf("    midi_instrument_info.name            = %s\n", pInstrInfo->name);
	printf("    midi_instrument_info.engine_name     = %s\n", pInstrInfo->engine_name);
	printf("    midi_instrument_info.instrument_file = %s\n", pInstrInfo->instrument_file);
	printf("    midi_instrument_info.instrument_nr   = %d\n", pInstrInfo->instrument_nr);
	printf("    midi_instrument_info.instrument_name = %s\n", pInstrInfo->instrument_name);
	printf("    midi_instrument_info.load_mode       = "); client_test_load_mode(pInstrInfo->load_mode);
	printf("    midi_instrument_info.volume          = %g\n", pInstrInfo->volume);
	printf("  }\n");
	return 0;
}



void LSClient::testClient()
{
	const char **ppszAudioDrivers = NULL, **ppszMidiDrivers, **ppszEngines;
	//const char *pszAudioDriver, *pszMidiDriver, *pszEngine = 0;
	//int iAudioDriver, iMidiDriver, iEngine;
	//int iAudio, iAudioDevice, iMidi, iMidiDevice;
	//int iNewAudioDevice, iNewMidiDevice;
	//int *piAudioDevices, *piMidiDevices;
	//lscp_midi_instrument_t midi_instr;
	//int i, j, k;

	CLIENT_TEST(_client, server_info, lscp_get_server_info(_client));
	CLIENT_TEST(_client, size_t, lscp_get_available_audio_drivers(_client));
	if (ppszAudioDrivers == NULL) {
		fprintf(stderr, "LSClient: No audio drivers available.\n");
	}

	CLIENT_TEST(_client, size_t, lscp_get_available_midi_drivers(_client));
	CLIENT_TEST(_client, szsplit, ppszMidiDrivers = lscp_list_available_midi_drivers(_client));
	if (ppszMidiDrivers == NULL) {
		fprintf(stderr, "LSClient: No MIDI drivers available.\n");
	}

	CLIENT_TEST(_client, size_t, lscp_get_available_engines(_client));
	CLIENT_TEST(_client, szsplit, ppszEngines = lscp_list_available_engines(_client));
	if (ppszEngines == NULL) {
		fprintf(stderr, "LSClient: No engines available.\n");
	}
	printf("\n\n");
	printf("  LSClient Total: %d tests, %d failed.\n\n", g_test_count, g_test_fails);
}
#else
void foobar()
{
}
#endif
