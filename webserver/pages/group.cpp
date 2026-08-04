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
	page += F("<label class='guest-option'>"
		"<input type='radio' name='rws_group_mode' value='");
	page += String(mode);
	page += F("'");
	if (mode == selected) {
		page += F(" checked");
	}
	page += F("/><span>");
	page += label;
	page += F("</span></label>");
}

static void appendSaveFeedback(String& page_content, RwsGroupApplyResult save_result) {
	if (save_result == RwsGroupApply_None) {
		return;
	}
	if (save_result == RwsGroupApply_Ok) {
		page_content += F("<div class='ui-notice ui-notice--ok'><strong>");
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
	page_content += F("<div class='ui-notice ui-notice--err'><strong>");
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

static void appendGroupOverview(String& page_content, Robonomics* robonomics, const String& self_ss58) {
	page_content += F("<div class='data-sheet'>");
	page_content += F("<div class='data-block'><h3 class='data-block__title'>");
	page_content += FPSTR(INTL_GROUP_STATUS_LABEL);
	page_content += F("</h3><div class='data-block__rows'><div class='data-line data-line--stack'>"
		"<span class='data-line__val'>");
	page_content += groupStatusText(robonomics, self_ss58);
	page_content += F("</span></div></div></div>");
	page_content += F("</div>");
}

void webserver_group_page(String& page_content, const String& self_ss58, Robonomics* robonomics,
                          RwsGroupApplyResult save_result, const char* form_action, bool hub_embed) {
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

	if (!hub_embed) {
		append_app_page_body_start(page_content, FPSTR(INTL_GROUP_INTRO));
	}

	appendSaveFeedback(page_content, save_result);

	appendGroupOverview(page_content, robonomics, self_ss58);

	page_content += F("<form class='page-form' method='POST' action='");
	page_content += form_action;
	page_content += F("'>");

	page_content += F("<section class='config-section'><h2 class='config-section__title'>");
	page_content += FPSTR(INTL_GROUP_MODE_TITLE);
	page_content += F("</h2><div class='config-section__body'><div class='radio-list'>");
	appendModeRadio(page_content, RWS_GROUP_STANDALONE, FPSTR(INTL_GROUP_MODE_STANDALONE), mode);
	appendModeRadio(page_content, RWS_GROUP_MASTER, FPSTR(INTL_GROUP_MODE_MASTER), mode);
	appendModeRadio(page_content, RWS_GROUP_FOLLOWER, FPSTR(INTL_GROUP_MODE_FOLLOWER), mode);
	appendModeRadio(page_content, RWS_GROUP_MANUAL, FPSTR(INTL_GROUP_MODE_MANUAL), mode);
	page_content += F("</div></div></section>");

	page_content += F("<section class='config-section'><h2 class='config-section__title'>");
	page_content += FPSTR(INTL_GROUP_SELF_ADDRESS);
	page_content += F("</h2><div class='config-section__body'><code class='code-mono'>");
	page_content += self_display;
	page_content += F("</code></div></section>");

	if (group_id_seed.length() > 0) {
		page_content += F("<input type='hidden' id='rws_group_id_seed' name='rws_group_id_seed' value='");
		page_content += group_id_seed;
		page_content += F("'/>");
	}

	page_content += F("<section id='panel_master' class='group-panel config-section'><h2 class='config-section__title'>");
	page_content += FPSTR(INTL_GROUP_MASTER_PANEL);
	page_content += F("</h2><div class='config-section__body'>"
		"<p class='form-hint'><strong>");
	page_content += FPSTR(INTL_GROUP_ID_LABEL);
	page_content += F(":</strong> <code id='group_id_display' class='code-mono code-mono--inline'>");
	page_content += group_id_display;
	page_content += F("</code></p>"
		"<p class='form-hint'><strong>");
	page_content += FPSTR(INTL_GROUP_MASTER_ADDRESS);
	page_content += F(":</strong></p><code class='code-mono'>");
	page_content += self_display;
	page_content += F("</code>"
		"<p class='form-hint'><strong>");
	page_content += FPSTR(INTL_GROUP_CURRENT_DEVICES);
	page_content += F(":</strong></p>"
		"<code id='master_devices_preview' class='code-mono code-mono--pre'>");
	page_content += current_devices;
	page_content += F("</code>"
		"<div class='form-group'><label for='rws_devices_extra'>");
	page_content += FPSTR(INTL_GROUP_KNOWN_DEVICES);
	page_content += F("</label>"
		"<textarea id='rws_devices_extra' name='rws_devices_extra' rows='5' maxlength='");
	page_content += String(LEN_RWS_DEVICES_EXTRA - 1);
	page_content += F("'>");
	page_content += String(cfg::rws_devices_extra);
	page_content += F("</textarea></div>"
		"<p class='form-hint'>");
	page_content += FPSTR(INTL_GROUP_KNOWN_DEVICES_HINT);
	page_content += F("</p></div></section>");

	page_content += F("<section id='panel_follower' class='group-panel config-section'><h2 class='config-section__title'>");
	page_content += FPSTR(INTL_GROUP_FOLLOWER_PANEL);
	page_content += F("</h2><div class='config-section__body'>"
		"<div class='form-group'><label for='rws_master_owner'>");
	page_content += FPSTR(INTL_GROUP_MASTER_ADDRESS);
	page_content += F("</label>"
		"<input type='text' class='input-mono' id='rws_master_owner' name='rws_master_owner' maxlength='");
	page_content += String(LEN_RWS_OWNER - 1);
	page_content += F("' value='");
	if (mode == RWS_GROUP_FOLLOWER) {
		page_content += String(cfg::rws_owner);
	}
	page_content += F("'/></div><p class='form-hint'>");
	page_content += FPSTR(INTL_GROUP_FOLLOWER_HINT);
	page_content += F("</p></div></section>");

	page_content += F("<section id='panel_manual' class='group-panel config-section'><h2 class='config-section__title'>");
	page_content += FPSTR(INTL_GROUP_MANUAL_PANEL);
	page_content += F("</h2><div class='config-section__body'>"
		"<div class='form-group'><label for='rws_manual_owner'>");
	page_content += FPSTR(INTL_RWS_OWNER);
	page_content += F("</label>"
		"<input type='text' class='input-mono' id='rws_manual_owner' name='rws_manual_owner' maxlength='");
	page_content += String(LEN_RWS_OWNER - 1);
	page_content += F("' value='");
	if (mode == RWS_GROUP_MANUAL || rwsOwnerIsExternal(self_ss58)) {
		page_content += String(cfg::rws_owner);
	}
	page_content += F("'/></div><p class='form-hint'>");
	page_content += FPSTR(INTL_GROUP_MANUAL_HINT);
	page_content += F("</p></div></section>");

	page_content += F("<div class='page-form-footer'>"
		"<input type='hidden' name='save_group' value='1'/>");
	page_content += form_submit(FPSTR(INTL_SAVE));
	page_content += F("</div></form>");

	page_content += F("<script>"
		"function groupPanels(){"
		"var m=document.querySelector('input[name=rws_group_mode]:checked');"
		"var v=m?parseInt(m.value,10):0;"
		"document.getElementById('panel_master').classList.toggle('group-panel--visible',v===1);"
		"document.getElementById('panel_follower').classList.toggle('group-panel--visible',v===2);"
		"document.getElementById('panel_manual').classList.toggle('group-panel--visible',v===3);"
		"}"
		"document.querySelectorAll('input[name=rws_group_mode]').forEach(function(el){"
		"el.addEventListener('change',groupPanels);});"
		"groupPanels();"
		"</script>");

	if (!hub_embed) {
		append_app_page_body_end(page_content);
	}
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
