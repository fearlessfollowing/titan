
request cmd:
module_tool "{\"name\":\"test_module_communication\"}"
module_tool "{\"name\":\"test_module_spi\"}"
module_tool "{\"name\":\"test_module_connection\"}"
module_tool "{\"name\":\"power_on\"}"
module_tool "{\"name\":\"power_off\"}"
module_tool "{\"name\":\"blc_calibration\"}"
module_tool "{\"name\":\"bpc_calibration\"}"
module_tool "{\"name\":\"get_uuid_sensorid\"}"
module_tool "{\"name\":\"query_storage\"}"

module_tool "{\"name\":\"set_calibration_data\",\"parameters\":{\"index\":5,\"bpc\":\"/sdcard/bpc.bin\",\"vig\":\"/sdcard/vig.bin\",\"vig_max_value\":[6400,10000,10000,6800]}}"

rsp:
{"respone_code":int, "description":string, "results":obj}

respone_code = 0 : success
respone_code != 0 : error


{"name":"power_on"}
{"name":"power_off"}"
{"name":"blc_calibration"}"
{"name":"bpc_calibration"}"
{"name":"get_uuid_sensorid"}"
{"name":"set_calibration_data","parameters":{"index":int,"bpc":string,"vig":string,"vig_max_value":intarray}}"
{"name":"test_module_communication"}
{"name":"test_module_spi"}
{"name":"query_storage"}