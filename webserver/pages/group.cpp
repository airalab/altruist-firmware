#include "pages.h"
#include "../../apis/rws_group.h"
#include "../../apis/rws_devices_registry.h"
#include "../../config_manager/config_defaults.h"
#include "../../defines.h"
#include "../../intl.h"
#include "../html-content.h"
#include "../utils.h"
#include <Robonomics.h>

static bool extrasTextEmpty() {
	String extras = String(cfg::rws_devices_extra);
	extras.trim();
	return extras.length() == 0;
}

static void appendExtrasTokens(const String& self_ss58, const String& extras, String& list, bool& first_line) {
	int start = 0;
	while (start < extras.length()) {
		int end = start;
		while (end < extras.length()) {
			const char ch = extras[end];
			if (ch == '\n' || ch == '\r' || ch == ',' || ch == ';') {
				break;
			}
			++end;
		}
		String token = extras.substring(start, end);
		token.trim();
		start = end + 1;
		if (token.length() == 0 || token == self_ss58) {
			continue;
		}
		if (!first_line) {
			list += '\n';
		}
		list += token;
		first_line = false;
	}
}

/** Full on-chain device list: master + followers from config. */
static String buildMasterDevicesListText(const String& self_ss58) {
	String list;
	bool first_line = true;
	if (self_ss58.length() > 0 && self_ss58 != F("Not Set") && self_ss58 != F("-")) {
		list += self_ss58;
		first_line = false;
	}
	appendExtrasTokens(self_ss58, String(cfg::rws_devices_extra), list, first_line);
	if (list.length() == 0) {
		return String(F("-"));
	}
	return list;
}

static void appendModeRadio(String& page, unsigned mode, const __FlashStringHelper* label, unsigned selected) {
	page += F("<label style='display:block;margin:8px 0;'>"
		"<input type='radio' name='rws_group_mode' value='");
	page += String(mode);
	page += F("'");
	if (mode == selected) {
		page += F(" checked");
	}
	page += F("/> ");
	page += label;
	page += F("</label>");
}

static void appendSaveFeedback(String& page_content, RwsGroupApplyResult save_result) {
	if (save_result == RwsGroupApply_None) {
		return;
	}
	if (save_result == RwsGroupApply_Ok) {
		page_content += F("<div style='margin:12px 0;padding:12px;background:#eef6ee;border:1px solid #c8e6c9;"
			"border-radius:6px;color:#2e7d32;'><strong>");
		page_content += FPSTR(INTL_GROUP_SAVE_OK);
		page_content += F("</strong></div>");
		return;
	}
	const __FlashStringHelper* message = FPSTR(INTL_GROUP_SAVE_FAILED);
	switch (save_result) {
	case RwsGroupApply_InvalidFollowerMaster:
		message = FPSTR(INTL_GROUP_ERROR_INVALID_MASTER);
		break;
	case RwsGroupApply_InvalidManualOwner:
		message = FPSTR(INTL_GROUP_ERROR_INVALID_MANUAL_OWNER);
		break;
	case RwsGroupApply_ConfigWriteFailed:
		message = FPSTR(INTL_GROUP_SAVE_CONFIG_FAILED);
		break;
	default:
		break;
	}
	page_content += F("<div style='margin:12px 0;padding:12px;background:#ffebee;border:1px solid #ef9a9a;"
		"border-radius:6px;color:#c62828;'><strong>");
	page_content += message;
	page_content += F("</strong></div>");
}

static const __FlashStringHelper* groupStatusText(Robonomics* robonomics, const String& self_ss58) {
	if (cfg::rws_group_mode == RWS_GROUP_FOLLOWER) {
		return rwsGroupDevicesSynced(robonomics, self_ss58) ? FPSTR(INTL_GROUP_STATUS_JOINED)
		                                                    : FPSTR(INTL_GROUP_STATUS_PENDING);
	}
	if (cfg::rws_group_mode == RWS_GROUP_MANUAL) {
		return FPSTR(INTL_GROUP_STATUS_MANUAL);
	}
	if (cfg::rws_group_mode == RWS_GROUP_MASTER) {
		if (rwsGroupDevicesSynced(robonomics, self_ss58)) {
			return extrasTextEmpty() ? FPSTR(INTL_GROUP_STATUS_CREATED)
			                         : FPSTR(INTL_GROUP_STATUS_LIST_SYNCED);
		}
		if (extrasTextEmpty() && cfg::rws_devices_registered_hash[0] == '\0') {
			return FPSTR(INTL_GROUP_STATUS_GROUP_CREATING);
		}
		return FPSTR(INTL_GROUP_STATUS_LIST_UPDATED);
	}
	return rwsGroupDevicesSynced(robonomics, self_ss58) ? FPSTR(INTL_GROUP_STATUS_DEVICES_SYNCED)
	                                                    : FPSTR(INTL_GROUP_STATUS_PENDING);
}

void webserver_group_page(String& page_content, const String& self_ss58, Robonomics* robonomics,
                          RwsGroupApplyResult save_result) {
	const unsigned mode = cfg::rws_group_mode;
	const String self_display =
	    (self_ss58.length() > 0 && self_ss58 != F("Not Set")) ? self_ss58 : String(F("-"));
	String current_devices = rwsBuildExpectedDeviceFingerprint(robonomics);
	if (current_devices.length() == 0) {
		current_devices = rwsBuildExpectedDeviceFingerprint(self_ss58);
	}
	if (current_devices.length() == 0) {
		current_devices = buildMasterDevicesListText(self_ss58);
	}

	String group_id_display = String(cfg::rws_group_id);
	String group_id_seed;
	if (group_id_display.length() == 0) {
		group_id_seed = rwsGenerateGroupId();
		group_id_display = group_id_seed;
	}

	page_content += F("<p style='font-size:13px;color:#666;'>");
	page_content += FPSTR(INTL_GROUP_INTRO);
	page_content += F("</p>");

	appendSaveFeedback(page_content, save_result);

	page_content += F("<p><strong>");
	page_content += FPSTR(INTL_GROUP_STATUS_LABEL);
	page_content += F(":</strong> ");
	page_content += groupStatusText(robonomics, self_ss58);
	page_content += F("</p>");

	if (mode == RWS_GROUP_FOLLOWER) {
		page_content += F("<div style='margin:12px 0;padding:12px;background:#eef3ff;border:1px solid #c8d9ff;"
			"border-radius:6px;'>"
			"<p style='margin:0 0 8px 0;'><strong>");
		page_content += FPSTR(INTL_GROUP_MASTER_ADDRESS);
		page_content += F(":</strong><br/><code style='font-size:11px;word-break:break-all;'>");
		page_content += String(cfg::rws_owner);
		page_content += F("</code></p></div>");
	}

	if (mode == RWS_GROUP_MASTER) {
		page_content += F("<div style='margin:12px 0;padding:12px;background:#eef6ee;border:1px solid #c8e6c9;"
			"border-radius:6px;'>"
			"<p style='margin:0 0 8px 0;'><strong>");
		page_content += FPSTR(INTL_GROUP_ID_LABEL);
		page_content += F(":</strong> <code style='font-size:11px;'>");
		page_content += group_id_display;
		page_content += F("</code></p>"
			"<p style='margin:0 0 8px 0;'><strong>");
		page_content += FPSTR(INTL_GROUP_MASTER_ADDRESS);
		page_content += F(":</strong><br/><code style='font-size:11px;word-break:break-all;'>");
		page_content += self_display;
		page_content += F("</code></p>"
			"<p style='margin:0;'><strong>");
		page_content += FPSTR(INTL_GROUP_CURRENT_DEVICES);
		page_content += F(":</strong></p>"
			"<pre style='margin:6px 0 0 0;padding:8px;background:#fff;border:1px solid #ddd;"
			"border-radius:4px;font-size:11px;white-space:pre-wrap;word-break:break-all;'>");
		page_content += current_devices;
		page_content += F("</pre></div>");
	}

	page_content += F("<form method='POST' action='/group'>");

	page_content += F("<h4>");
	page_content += FPSTR(INTL_GROUP_MODE_TITLE);
	page_content += F("</h4>");

	appendModeRadio(page_content, RWS_GROUP_STANDALONE, FPSTR(INTL_GROUP_MODE_STANDALONE), mode);
	appendModeRadio(page_content, RWS_GROUP_MASTER, FPSTR(INTL_GROUP_MODE_MASTER), mode);
	appendModeRadio(page_content, RWS_GROUP_FOLLOWER, FPSTR(INTL_GROUP_MODE_FOLLOWER), mode);
	appendModeRadio(page_content, RWS_GROUP_MANUAL, FPSTR(INTL_GROUP_MODE_MANUAL), mode);

	page_content += F("<div style='margin:14px 0;padding:10px;background:#f6f6f6;border-radius:6px;'>"
		"<strong>");
	page_content += FPSTR(INTL_GROUP_SELF_ADDRESS);
	page_content += F("</strong><br/><code style='font-size:11px;word-break:break-all;'>");
	page_content += self_display;
	page_content += F("</code></div>");

	if (group_id_seed.length() > 0) {
		page_content += F("<input type='hidden' id='rws_group_id_seed' name='rws_group_id_seed' value='");
		page_content += group_id_seed;
		page_content += F("'/>");
	}

	page_content += F("<div id='panel_master' style='display:none;margin-top:12px;padding:12px;"
		"background:#f9f9f9;border:1px solid #e0e0e0;border-radius:6px;'>"
		"<h4 style='margin-top:0;'>");
	page_content += FPSTR(INTL_GROUP_MASTER_PANEL);
	page_content += F("</h4><p style='margin:8px 0;'><strong>");
	page_content += FPSTR(INTL_GROUP_ID_LABEL);
	page_content += F(":</strong> <code id='group_id_display' style='font-size:11px;'>");
	page_content += group_id_display;
	page_content += F("</code></p>"
		"<p style='margin:8px 0;'><strong>");
	page_content += FPSTR(INTL_GROUP_MASTER_ADDRESS);
	page_content += F(":</strong><br/><code style='font-size:11px;word-break:break-all;'>");
	page_content += self_display;
	page_content += F("</code></p>"
		"<p style='margin:8px 0 4px 0;'><strong>");
	page_content += FPSTR(INTL_GROUP_CURRENT_DEVICES);
	page_content += F(":</strong></p>"
		"<pre id='master_devices_preview' style='margin:0 0 12px 0;padding:8px;background:#fff;"
		"border:1px solid #ddd;border-radius:4px;font-size:11px;white-space:pre-wrap;word-break:break-all;'>");
	page_content += current_devices;
	page_content += F("</pre>"
		"<label for='rws_devices_extra' style='display:block;margin-top:4px;'>");
	page_content += FPSTR(INTL_GROUP_KNOWN_DEVICES);
	page_content += F("</label>"
		"<textarea id='rws_devices_extra' name='rws_devices_extra' rows='5' maxlength='");
	page_content += String(LEN_RWS_DEVICES_EXTRA - 1);
	page_content += F("' style='width:100%;font-family:monospace;font-size:11px;'>");
	page_content += String(cfg::rws_devices_extra);
	page_content += F("</textarea>"
		"<p style='font-size:11px;color:#666;'>");
	page_content += FPSTR(INTL_GROUP_KNOWN_DEVICES_HINT);
	page_content += F("</p></div>");

	page_content += F("<div id='panel_follower' style='display:none;margin-top:12px;'>"
		"<h4>");
	page_content += FPSTR(INTL_GROUP_FOLLOWER_PANEL);
	page_content += F("</h4><label for='rws_master_owner'>");
	page_content += FPSTR(INTL_GROUP_MASTER_ADDRESS);
	page_content += F("</label>"
		"<input type='text' id='rws_master_owner' name='rws_master_owner' maxlength='");
	page_content += String(LEN_RWS_OWNER - 1);
	page_content += F("' style='width:100%;font-family:monospace;font-size:11px;' value='");
	if (mode == RWS_GROUP_FOLLOWER) {
		page_content += String(cfg::rws_owner);
	}
	page_content += F("'/><p style='font-size:11px;color:#666;'>");
	page_content += FPSTR(INTL_GROUP_FOLLOWER_HINT);
	page_content += F("</p></div>");

	page_content += F("<div id='panel_manual' style='display:none;margin-top:12px;'>"
		"<h4>");
	page_content += FPSTR(INTL_GROUP_MANUAL_PANEL);
	page_content += F("</h4><label for='rws_manual_owner'>");
	page_content += FPSTR(INTL_RWS_OWNER);
	page_content += F("</label>"
		"<input type='text' id='rws_manual_owner' name='rws_manual_owner' maxlength='");
	page_content += String(LEN_RWS_OWNER - 1);
	page_content += F("' style='width:100%;font-family:monospace;font-size:11px;' value='");
	if (mode == RWS_GROUP_MANUAL || rwsOwnerIsExternal(self_ss58)) {
		page_content += String(cfg::rws_owner);
	}
	page_content += F("'/><p style='font-size:11px;color:#666;'>");
	page_content += FPSTR(INTL_GROUP_MANUAL_HINT);
	page_content += F("</p></div>");

	page_content += F("<input type='hidden' name='save_group' value='1'/>");
	page_content += form_submit(FPSTR(INTL_SAVE));
	page_content += F("</form>");

	page_content += F("<script>"
		"function groupPanels(){"
		"var m=document.querySelector('input[name=rws_group_mode]:checked');"
		"var v=m?parseInt(m.value,10):0;"
		"document.getElementById('panel_master').style.display=(v===1)?'block':'none';"
		"document.getElementById('panel_follower').style.display=(v===2)?'block':'none';"
		"document.getElementById('panel_manual').style.display=(v===3)?'block':'none';"
		"}"
		"document.querySelectorAll('input[name=rws_group_mode]').forEach(function(el){"
		"el.addEventListener('change',groupPanels);});"
		"groupPanels();"
		"</script>");
}

RwsGroupApplyResult webserver_group_post(WebServer& server, const String& self_ss58) {
	if (!server.hasArg(F("save_group"))) {
		return RwsGroupApply_None;
	}
	unsigned mode = cfg::rws_group_mode;
	if (server.hasArg(F("rws_group_mode"))) {
		mode = static_cast<unsigned>(server.arg(F("rws_group_mode")).toInt());
	}
	const String master_owner = server.hasArg(F("rws_master_owner")) ? server.arg(F("rws_master_owner")) : String();
	const String manual_owner = server.hasArg(F("rws_manual_owner")) ? server.arg(F("rws_manual_owner")) : String();
	const String known_devices = server.hasArg(F("rws_devices_extra")) ? server.arg(F("rws_devices_extra")) : String();
	const String group_id_seed = server.hasArg(F("rws_group_id_seed")) ? server.arg(F("rws_group_id_seed")) : String();

	const RwsGroupApplyResult result =
	    rwsApplyGroupSettings(mode, self_ss58, master_owner, known_devices, manual_owner, group_id_seed);
	if (result == RwsGroupApply_Ok) {
		debug_outln_info(F("[RWS] group settings saved from /group"));
	} else if (result != RwsGroupApply_None) {
		debug_outln_error(F("[RWS] group settings save failed"));
	}
	return result;
}
