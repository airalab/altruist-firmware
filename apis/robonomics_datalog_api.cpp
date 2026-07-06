#include "robonomics_datalog_api.h"
#include "../config_manager/config_helpers.h"
#include "helpers/message_formatter.h"
#include "../utils.h"

void RobonomicsDatalogAPI::setup() {
    api_name = "Robonomics Datalog";
    timeout = getConfigUintValue("datalog_sending_intervall_ms");
    rws_owner = getConfigStringValue("rws_owner");
    private_key = getConfigStringValue("private_key");
    robonomics_public_node = getConfigStringValue("robonomics_public_node");
    if (strcmp(private_key.c_str(), "Not Set") == 0) {
		robonomics->generateAndSetPrivateKey();
		saveRobonomicsPrivateKey(robonomics->getPrivateKey());
	} else {
		robonomics->setPrivateKey(private_key.c_str());
	}
    robonomics->setup(robonomics_public_node);
    debug_outln_info(F("Robonomics datalog API is ready with sending interval (sec): "), String(timeout/1000));
}

void RobonomicsDatalogAPI::_send(JsonDocument &data) {
    rws_owner = String(cfg::rws_owner);
    rws_owner.trim();
    if (rws_owner.length() == 0 || rws_owner.equalsIgnoreCase(F("not set"))) {
        rws_owner = String(robonomics->getSs58Address());
    }
    String datalog_data;
    formatRobonomicsString(data, datalog_data);
    if (datalog_data.length() == 0) {
        debug_outln_error(F("[Datalog] WARNING: data string is empty (all sharing disabled or no sensor data?)"));
    }
    debug_outln_verbose(F("[Datalog] Sending: "), datalog_data);
    debug_outln_verbose(F("[Datalog] RWS owner: "), rws_owner);
    debug_outln_verbose(F("[Datalog] Node: "), robonomics_public_node);
    const char* res = robonomics->sendRWSDatalogRecord(datalog_data.c_str(), rws_owner.c_str());
    const String res_s = String(res ? res : "");
    const bool lib_level_error = (res_s == "error");
    const bool json_error_object = (res_s.startsWith("{") && (res_s.indexOf("\"code\"") >= 0 || res_s.indexOf("\"message\"") >= 0));
    is_ok = (!lib_level_error && !json_error_object);
    if (is_ok) {
        debug_outln_verbose(F("[Datalog] OK, result: "), res_s);
    } else {
        debug_outln_error(F("[Datalog] FAILED"));
        debug_outln_verbose(F("[Datalog] Error response: "), res_s);
    }
}
