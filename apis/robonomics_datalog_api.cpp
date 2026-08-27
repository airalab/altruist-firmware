#include "robonomics_datalog_api.h"
#include "../config_manager/config_helpers.h"
#include "helpers/message_formatter.h"
#include "helpers/proto_envelope.h"
#include "../utils.h"

#include <string>

namespace
{

    String datalogToken(const String &text)
    {
        String token;
        token.reserve(text.length());
        bool last_separator = false;
        for (size_t i = 0; i < text.length(); ++i)
        {
            char c = text.charAt(i);
            if (c >= 'A' && c <= 'Z')
            {
                c = c - 'A' + 'a';
            }
            if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
            {
                token += c;
                last_separator = false;
            }
            else if (!last_separator && token.length() > 0)
            {
                token += '_';
                last_separator = true;
            }
        }
        if (token.endsWith("_"))
        {
            token.remove(token.length() - 1);
        }
        return token.length() > 0 ? token : String(F("unknown"));
    }

    const char *datalogEncoding(DatalogFormatStatus status)
    {
        return status == DATALOG_FORMAT_CPS ? "cps" : "plain";
    }

    const char *datalogFormatFailureReason(DatalogFormatStatus status)
    {
        switch (status)
        {
        case DATALOG_FORMAT_PAYLOAD_EMPTY:
            return "payload_empty";
        case DATALOG_FORMAT_ENCRYPTION_FAILED:
            return "encryption_failed";
        case DATALOG_FORMAT_PAYLOAD_TOO_LARGE:
            return "payload_too_large";
        default:
            return "formatting_failed";
        }
    }

    void logDatalogAttempt(size_t payload_len, const char *encoding, bool using_self_owner)
    {
        Serial.print(F("[DATALOG] attempt payload_len="));
        Serial.print(payload_len);
        Serial.print(F(" encoding="));
        Serial.print(encoding);
        Serial.print(F(" owner_self_fallback="));
        Serial.println(using_self_owner ? 1 : 0);
    }

    void logDatalogSuccess(size_t response_len)
    {
        Serial.print(F("\r\n[DATALOG] success response_len="));
        Serial.println(response_len);
    }

    void logDatalogLocalFailure(const char *reason)
    {
        Serial.print(F("[DATALOG] failed reason="));
        Serial.println(reason);
    }

    void logDatalogFailure(const String &reason, int code, const String &message, size_t response_len)
    {
        Serial.print(F("\r\n[DATALOG] failed reason="));
        Serial.print(reason);
        Serial.print(F(" code="));
        Serial.print(code);
        Serial.print(F(" message="));
        Serial.print(message);
        Serial.print(F(" response_len="));
        Serial.println(response_len);
    }

    bool parseRpcError(const String &response, int &code, String &message)
    {
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, response);
        if (error)
        {
            return false;
        }

        JsonVariant error_object = doc["error"];
        JsonVariant code_value;
        JsonVariant message_value;
        if (error_object.isNull())
        {
            code_value = doc["code"];
            message_value = doc["message"];
        }
        else
        {
            code_value = error_object["code"];
            message_value = error_object["message"];
        }
        if (code_value.isNull() && message_value.isNull())
        {
            return false;
        }

        if (code_value.is<int>())
        {
            code = code_value.as<int>();
        }
        else
        {
            code = 0;
        }
        if (message_value.is<const char *>())
        {
            message = datalogToken(String(message_value.as<const char *>()));
        }
        else
        {
            message = F("rpc_error");
        }
        return true;
    }

} // namespace

void RobonomicsDatalogAPI::setup()
{
    api_name = "Robonomics Datalog";
    timeout = getConfigUintValue("datalog_sending_intervall_ms");
    rws_owner = getConfigStringValue("rws_owner");
    private_key = getConfigStringValue("private_key");
    robonomics_public_node = getConfigStringValue("robonomics_public_node");
    if (strcmp(private_key.c_str(), "Not Set") == 0)
    {
        robonomics->generateAndSetPrivateKey();
        saveRobonomicsPrivateKey(robonomics->getPrivateKey());
    }
    else
    {
        robonomics->setPrivateKey(private_key.c_str());
    }
    robonomics->setup(robonomics_public_node);
    // CPS vectors: call valueCryptoSelfTest() manually when touching value_crypto.
    debug_outln_info(F("Robonomics datalog API is ready with sending interval (sec): "), String(timeout / 1000));
}

void RobonomicsDatalogAPI::_send(JsonDocument &data)
{
    rws_owner = String(cfg::rws_owner);
    rws_owner.trim();
    bool using_self_owner = false;
    if (rws_owner.length() == 0 || rws_owner.equalsIgnoreCase(F("not set")))
    {
        rws_owner = String(robonomics->getSs58Address());
        using_self_owner = true;
    }

    if (protoDatalogEnabled())
    {
        static uint8_t proto_envelope[PROTO_ENVELOPE_BUF_BYTES];
        size_t proto_len = 0;
        const ProtoBuildStatus proto_status =
            protoBuildSignedEnvelope(data, robonomics, proto_envelope, sizeof(proto_envelope), &proto_len);
        if (proto_status != PROTO_BUILD_OK)
        {
            logDatalogLocalFailure(protoBuildStatusReason(proto_status));
            is_ok = false;
            return;
        }
        if (proto_len > DATALOG_CHAIN_SAFE_BYTES)
        {
            Serial.print(F("[DATALOG] failed reason=payload_too_large encoding=proto payload_len="));
            Serial.println(proto_len);
            is_ok = false;
            return;
        }
        logDatalogAttempt(proto_len, "proto", using_self_owner);
        const std::string record(reinterpret_cast<const char *>(proto_envelope), proto_len);
        const char *res = robonomics->sendRWSDatalogRecord(record, rws_owner.c_str());
        const String res_s = String(res ? res : "");
        const bool lib_level_error = (res_s == "error");
        const bool json_error_object = (res_s.startsWith("{") && (res_s.indexOf("\"code\"") >= 0 || res_s.indexOf("\"message\"") >= 0));
        is_ok = (!lib_level_error && !json_error_object);
        if (is_ok)
        {
            logDatalogSuccess(res_s.length());
            debug_outln_verbose(F("[Datalog] OK, proto result: "), res_s);
        }
        else
        {
            int rpc_code = 0;
            String rpc_message = F("unknown");
            const bool parsed_rpc_error = parseRpcError(res_s, rpc_code, rpc_message);
            logDatalogFailure(
                lib_level_error ? String(F("client_error")) : String(F("rpc_error")),
                parsed_rpc_error ? rpc_code : 0,
                parsed_rpc_error ? rpc_message : datalogToken(res_s),
                res_s.length());
            debug_outln_verbose(F("[Datalog] Error response: "), res_s);
        }
        return;
    }

    String datalog_data;
    const DatalogFormatStatus format_status = formatRobonomicsDatalogString(data, datalog_data);
    if (
        format_status == DATALOG_FORMAT_PAYLOAD_EMPTY ||
        format_status == DATALOG_FORMAT_ENCRYPTION_FAILED ||
        format_status == DATALOG_FORMAT_PAYLOAD_TOO_LARGE)
    {
        logDatalogLocalFailure(datalogFormatFailureReason(format_status));
        is_ok = false;
        return;
    }
    debug_outln_verbose(F("[Datalog] Sending: "), datalog_data);
    debug_outln_verbose(F("[Datalog] RWS owner: "), rws_owner);
    debug_outln_verbose(F("[Datalog] Node: "), robonomics_public_node);
    logDatalogAttempt(datalog_data.length(), datalogEncoding(format_status), using_self_owner);
    const char *res = robonomics->sendRWSDatalogRecord(datalog_data.c_str(), rws_owner.c_str());
    const String res_s = String(res ? res : "");
    const bool lib_level_error = (res_s == "error");
    const bool json_error_object = (res_s.startsWith("{") && (res_s.indexOf("\"code\"") >= 0 || res_s.indexOf("\"message\"") >= 0));
    is_ok = (!lib_level_error && !json_error_object);
    if (is_ok)
    {
        logDatalogSuccess(res_s.length());
        debug_outln_verbose(F("[Datalog] OK, result: "), res_s);
    }
    else
    {
        int rpc_code = 0;
        String rpc_message = F("unknown");
        const bool parsed_rpc_error = parseRpcError(res_s, rpc_code, rpc_message);
        logDatalogFailure(
            lib_level_error ? String(F("client_error")) : String(F("rpc_error")),
            parsed_rpc_error ? rpc_code : 0,
            parsed_rpc_error ? rpc_message : datalogToken(res_s),
            res_s.length());
        debug_outln_verbose(F("[Datalog] Error response: "), res_s);
    }
}
