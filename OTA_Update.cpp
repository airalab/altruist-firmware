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

static const char* const FW_HOSTS[] = { FW_DOWNLOAD_HOST, FW_DOWNLOAD_HOST_ALTERNATIVE };
static constexpr int FW_HOST_COUNT = 2;
static constexpr unsigned long FW_HOST_TIMEOUT_MS = 30000;  // 30s per host attempt

static String buildFirmwarePath() {
	String language(cfg::current_lang);
	language.toUpperCase();
	if (language != F("EN") && language != F("RU")) {
		language = CURRENT_LANG;
	}
	language.toLowerCase();

#if defined(ALTRUIST_INSIGHT)
	String path(F("/latest32c6ins_"));
#elif defined(ALTRUIST_URBAN) && defined(CONFIG_IDF_TARGET_ESP32C3)
	String path(F("/latest32c3_"));
#elif defined(ALTRUIST_URBAN) && defined(CONFIG_IDF_TARGET_ESP32C6)
	String path(F("/latest32c6urb_"));
#else
#error "Unsupported Altruist model or ESP target for OTA"
#endif

	path += language;
	path += F(ALTRUIST_OTA_CHANNEL_SUFFIX);
	path += F(".bin");
	return path;
}

static String buildUserAgent() {
	String agent(SOFTWARE_VERSION_STR);
	agent += ' ';
	agent += get_chipid();
	agent += ' ';
	agent += String(cfg::current_lang);
	agent += ' ';
	agent += String(CURRENT_LANG);
	return agent;
}

static bool fwDownloadStream(WiFiClient& client, const String& url, Stream* ostream, device_status_t &deviceStatus) {

	String agent = buildUserAgent();

	for (int h = 0; h < FW_HOST_COUNT; h++) {
		HTTPClient http;
		http.setTimeout(FW_HOST_TIMEOUT_MS);
		http.setUserAgent(agent);
		http.setReuse(false);

		debug_outln_info(F("HTTP GET: "), String(FW_HOSTS[h]) + ':' + String(FW_DOWNLOAD_PORT) + url);

		if (http.begin(client, FW_HOSTS[h], FW_DOWNLOAD_PORT, url)) {
			int r = http.GET();
			debug_outln_info(F("GET r: "), String(r));
			deviceStatus.last_update_returncode = r;
			if (r == HTTP_CODE_OK) {
				int bytes_written = http.writeToStream(ostream);
				http.end();
				if (bytes_written > 0) return true;
			} else {
				http.end();
			}
		}
		debug_outln_info(F("Host failed, trying next..."));
	}

	return false;
}

bool downloadAndUpdate(const char* url, const String& expectedMD5, device_status_t &deviceStatus)
{
    static constexpr unsigned long OTA_TOTAL_TIMEOUT_MS = 600000;  // 10 min total across all attempts
    static constexpr int MAX_ATTEMPTS = FW_HOST_COUNT * 2;         // each host gets 2 tries

    const unsigned long totalStartTime = millis();
    String agent = buildUserAgent();

    for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++) {
        int hostIdx = attempt % FW_HOST_COUNT;
        const char* host = FW_HOSTS[hostIdx];

        if (millis() - totalStartTime > OTA_TOTAL_TIMEOUT_MS) {
            debug_outln_error(F("OTA total timeout exceeded"));
            return false;
        }

        if (WiFi.status() != WL_CONNECTED) {
            debug_outln_error(F("WiFi not connected, aborting OTA"));
            return false;
        }

        debug_outln_info(F("OTA attempt "), String(attempt + 1) + F(" host: ") + String(host));

        WiFiClient client;
        HTTPClient http;
        http.setTimeout(FW_HOST_TIMEOUT_MS);
        http.setUserAgent(agent);
        http.setReuse(false);

        if (!http.begin(client, host, FW_DOWNLOAD_PORT, url)) {
            debug_outln_error(F("HTTP begin failed"));
            continue;
        }

        int httpCode = http.GET();
        if (httpCode != HTTP_CODE_OK) {
            debug_outln_info(F("HTTP GET failed, code: "), String(httpCode));
            http.end();
            continue;
        }

        int contentLength = http.getSize();
        if (contentLength <= 0) {
            debug_outln_error(F("Invalid content length"));
            http.end();
            continue;
        }

        debug_outln_info(F("Content Length: "), String(contentLength));

        if (!Update.begin(contentLength)) {
            debug_outln_error(F("Not enough space for OTA"));
            http.end();
            return false;
        }

        WiFiClient& stream = http.getStream();
        const size_t bufferSize = 1024;
        uint8_t buffer[bufferSize];

        size_t written = 0;
        int lastPercent = -1;
        unsigned long lastDataTime = millis();
        bool download_ok = true;

        deviceStatus.ota_progress_percent = 0;

        while (written < (size_t)contentLength) {

            if (millis() - totalStartTime > OTA_TOTAL_TIMEOUT_MS) {
                debug_outln_error(F("OTA total timeout exceeded during download"));
                Update.abort();
                http.end();
                return false;
            }

            if (WiFi.status() != WL_CONNECTED) {
                debug_outln_error(F("WiFi disconnected during OTA"));
                Update.abort();
                http.end();
                return false;
            }

            size_t available = stream.available();

            if (available) {
                size_t toRead = (available > bufferSize) ? bufferSize : available;
                int bytesRead = stream.readBytes(buffer, toRead);

                if (bytesRead > 0) {
                    lastDataTime = millis();

                    if (Update.write(buffer, bytesRead) != (size_t)bytesRead) {
                        debug_outln_error(F("Update.write failed"));
                        Update.abort();
                        http.end();
                        return false;
                    }

                    written += bytesRead;

                    int percent = (written * 100) / contentLength;
                    if (percent != lastPercent) {
                        deviceStatus.ota_progress_percent = percent;
                        if (percent % 5 == 0) {
                            debug_outln_info(F("OTA Progress: "), String(percent) + F("%"));
                        }
                        lastPercent = percent;
                    }
                }
            } else {
                if (millis() - lastDataTime > FW_HOST_TIMEOUT_MS) {
                    debug_outln_error(F("OTA stalled, switching host..."));
                    download_ok = false;
                    break;
                }
            }

            delay(1);
        }

        if (!download_ok) {
            Update.abort();
            http.end();
            deviceStatus.ota_progress_percent = 0;
            continue;
        }

        debug_outln_info(F("Download complete. Written: "), String(written));

        if (!Update.end()) {
            debug_outln_error(F("Update.end failed"));
            Update.abort();
            http.end();
            continue;
        }

        if (!Update.isFinished()) {
            debug_outln_error(F("Update not finished properly"));
            http.end();
            continue;
        }

        String md5String = Update.md5String();
        if (!md5String.equalsIgnoreCase(expectedMD5)) {
            debug_outln_error(F("MD5 mismatch!"));
            debug_outln_info(F("Expected: "), expectedMD5);
            debug_outln_info(F("Actual: "), md5String);
            http.end();
            continue;
        }

        debug_outln_info(F("OTA successful and verified"));
        http.end();
        return true;
    }

    debug_outln_error(F("OTA failed after all attempts"));
    return false;
}

void twoStageOTAUpdate(device_status_t &deviceStatus, bool manual) {
	if (!manual) {
		if (!ALTRUIST_AUTOMATIC_OTA_ALLOWED || !cfg::auto_update) return;
	}

	debug_outln_info(F("twoStageOTAUpdate"));
	String fetch_name = buildFirmwarePath();
	debug_outln_info(
		F("OTA source: "),
		String(ALTRUIST_OTA_SOURCE_CHANNEL) + F(", firmware channel: ") +
			String(ALTRUIST_BUILD_CHANNEL) + F(", artifact: ") + fetch_name
	);

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

	// Show "Updating firmware" screen before download starts
	deviceStatus.ota_in_progress = true;
	deviceStatus.ota_progress_percent = 0;
	// Give display a chance to refresh (main loop runs displayManager.process)
	vTaskDelay(pdMS_TO_TICKS(800));

	if (downloadAndUpdate(fetch_name.c_str(), newFwmd5, deviceStatus)) {
		deviceStatus.ota_in_progress = false;
		deviceStatus.ota_success = true;
		// Give display time to show success screen before sensor_restart() shuts down peripherals
		vTaskDelay(pdMS_TO_TICKS(3000));
		set_restart_reason(RESTART_REASON_OTA);
		sensor_restart();
    }

	// Download failed — show error on display before returning to main screen
	deviceStatus.ota_in_progress = false;
	deviceStatus.ota_failed = true;
	deviceStatus.ota_progress_percent = -1;
	// Keep the failure screen visible for 15 seconds so the user can read it
	vTaskDelay(pdMS_TO_TICKS(15000));
	deviceStatus.ota_failed = false;
}
