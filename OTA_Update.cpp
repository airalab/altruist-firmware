#include "OTA_Update.h"
#include <StreamString.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#include "defines.h"
#include "config_manager/config_helpers.h"
#include <MD5Builder.h>
#include <Update.h>

/*****************************************************************
 * OTAUpdate                                                     *
 *****************************************************************/

static bool fwDownloadStream(WiFiClient& client, const String& url, Stream* ostream, device_status_t &deviceStatus) {

	HTTPClient http;
	int bytes_written = -1;

	// work with 128kbit/s downlinks
	http.setTimeout(60 * 1000);
	String agent(SOFTWARE_VERSION_STR);
    String esp_chipid = get_chipid();
	agent += ' ';
	agent += esp_chipid;
	agent += ' ';
	agent += String(cfg::current_lang);
	agent += ' ';
	agent += String(CURRENT_LANG);
	agent += ' ';

	http.setUserAgent(agent);
	http.setReuse(false);

	debug_outln_info(F("HTTP GET: "), String(FPSTR(FW_DOWNLOAD_HOST)) + ':' + String(FW_DOWNLOAD_PORT) + url);

	if (http.begin(client, FPSTR(FW_DOWNLOAD_HOST), FW_DOWNLOAD_PORT, url)) {
		int r = http.GET();
		debug_outln_info(F("GET r: "), String(r));
		deviceStatus.last_update_returncode = r;
		if (r == HTTP_CODE_OK) {
			bytes_written = http.writeToStream(ostream);
		}
		http.end();
	}

	if (bytes_written > 0)
		return true;

	return false;
}

bool downloadAndUpdate(const char* url, const String& expectedMD5) {
    WiFiClient client;
    HTTPClient http;
    http.begin(client, FPSTR(FW_DOWNLOAD_HOST), FW_DOWNLOAD_PORT, url);
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        debug_outln_info(F("Failed to download file, http code: "), httpCode);
        http.end();
        return false;
    }

    int contentLength = http.getSize();
    if (contentLength <= 0) {
        debug_outln_error(F("Content-Length not defined or invalid"));
        http.end();
        return false;
    }

    bool canBegin = Update.begin(contentLength);
    if (!canBegin) {
        debug_outln_error(F("Not enough space to begin OTA"));
        http.end();
        return false;
    }

    debug_outln_info(F("Begin OTA. This may take some time..."));

    // WiFiClient *stream = http.getStreamPtr();
    // size_t written = Update.writeStream(*stream);

	WiFiClient stream = http.getStream();
	const size_t bufferSize = 1024;
	uint8_t buffer[bufferSize];
	size_t written = 0;
	int lastPercent = -1;

	while (stream.connected() && written < contentLength) {
		size_t available = stream.available();
		if (available) {
			size_t toRead = (available > bufferSize) ? bufferSize : available;
			int bytesRead = stream.readBytes(buffer, toRead);

			if (bytesRead > 0) {
				if (Update.write(buffer, bytesRead) != bytesRead) {
					debug_outln_error(F("Update.write() failed"));
					return false;
				}

				written += bytesRead;

				// Print progress
				int percent = (written * 100) / contentLength;
				if (percent != lastPercent && percent % 5 == 0) {  // only print every 5%
					debug_outln_info(F("OTA Progress: "), percent);
					lastPercent = percent;
				}
			}
		}
		delay(1);  // yield
	}

    if (written == contentLength) {
        debug_outln_info(F("Written successfully: "), written);
    } else {
		debug_outln_info(F("Content length: "), contentLength);
        debug_outln_info(F("Written only: "), written);
    }

    if (Update.end()) {
        if (Update.isFinished()) {
            debug_outln_info(F("Update successfully completed."));
            String md5String = Update.md5String();
            if (md5String.equalsIgnoreCase(expectedMD5)) {
                debug_outln_info(F("MD5 verified successfully."));
                http.end();
                return true;
            } else {
                debug_outln_error(F("MD5 verification failed."));
            }
        } else {
            debug_outln_error(F("Update not finished? Something went wrong!"));
        }
    } else {
        debug_outln_error(F("Error Occurred during update"));
    }

    http.end();
    return false;
}


void twoStageOTAUpdate(device_status_t &deviceStatus) {
	if (!cfg::auto_update) return;

	debug_outln_info(F("twoStageOTAUpdate"));
	String lang_variant(cfg::current_lang);
	if (lang_variant.length() != 2) {
		lang_variant = CURRENT_LANG;
	}
	lang_variant.toLowerCase();
	String fetch_name(F("/latest32c3_"));
	if (cfg::use_beta) {
		fetch_name += F("beta");
	} else {
		fetch_name += lang_variant;
	}

	fetch_name += F(".bin");

	WiFiClient client;
	String fetch_md5_name(fetch_name);
	fetch_md5_name += F(".md5");
	debug_outln_info(F("download md5 begin"));

	StreamString newFwmd5;
	if (!fwDownloadStream(client, fetch_md5_name, &newFwmd5, deviceStatus)){
		debug_outln_info(F("download md5 fail"));
		return;}
	debug_outln_info(F("download md5 end"));

	newFwmd5.trim();
	if (newFwmd5 == ESP.getSketchMD5()) {
		debug_outln_info(F("No newer version available."));
		return;
	}

	debug_outln_info(F("Update md5: "), newFwmd5);
	debug_outln_info(F("Sketch md5: "), ESP.getSketchMD5());

	if (downloadAndUpdate(fetch_name.c_str(), newFwmd5)) {
        sensor_restart();
    }
}