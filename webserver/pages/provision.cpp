#include "pages.h"
#include "../../intl.h"
#include "../html-content.h"
#include "../utils.h"

void webserver_provision_page(String& page_content) {
	append_app_page_body_start(page_content, FPSTR(INTL_PROVISION_INTRO));

	page_content += F("<section class='hub-backup__panel hub-provision'>"
		"<h3 class='hub-backup__title'>");
	page_content += FPSTR(INTL_PROVISION_CURRENT);
	page_content += F("</h3><p class='code-mono hub-provision__addr' id='provision-current'>—</p>"
		"</section>"
		"<form id='provision-form' class='page-form hub-provision' autocomplete='off' onsubmit='return false;'"
		" data-err-mnemonic='");
	page_content += FPSTR(INTL_PROVISION_ERR_MNEMONIC);
	page_content += F("' data-err-index='");
	page_content += FPSTR(INTL_PROVISION_ERR_INDEX);
	page_content += F("' data-err-crypto='");
	page_content += FPSTR(INTL_PROVISION_ERR_CRYPTO);
	page_content += F("' data-err-install='");
	page_content += FPSTR(INTL_PROVISION_ERR_INSTALL);
	page_content += F("' data-confirm-install='");
	page_content += FPSTR(INTL_PROVISION_CONFIRM);
	page_content += F("' data-ok-install='");
	page_content += FPSTR(INTL_PROVISION_OK);
	page_content += F("' data-mismatch='");
	page_content += FPSTR(INTL_PROVISION_MISMATCH);
	page_content += F("' data-need-browser='");
	page_content += FPSTR(INTL_PROVISION_NEED_BROWSER);
	page_content += F("'>"
		"<div id='provision-status' class='ui-notice' hidden></div>"
		"<section class='config-section'><h2 class='config-section__title'>");
	page_content += FPSTR(INTL_PROVISION_SEED);
	page_content += F("</h2><div class='config-section__body'>"
		"<textarea id='provision-seed' rows='3' spellcheck='false' autocapitalize='off' autocomplete='off'></textarea>"
		"<p class='form-hint'>");
	page_content += FPSTR(INTL_PROVISION_SEED_HINT);
	page_content += F("</p>"
		"<label class='hub-backup__file-label' for='provision-password'>");
	page_content += FPSTR(INTL_PROVISION_PASSWORD);
	page_content += F("</label>"
		"<input id='provision-password' type='password' autocomplete='off'/>"
		"<p class='form-hint'>");
	page_content += FPSTR(INTL_PROVISION_PASSWORD_HINT);
	page_content += F("</p>"
		"<label class='hub-backup__file-label' for='provision-index'>");
	page_content += FPSTR(INTL_PROVISION_INDEX);
	page_content += F("</label>"
		"<input id='provision-index' type='number' min='0' step='1' value='0'/>"
		"<p class='form-hint'>");
	page_content += FPSTR(INTL_PROVISION_INDEX_HINT);
	page_content += F("</p>"
		"<button type='button' class='encrypt-key-btn' id='provision-derive'>");
	page_content += FPSTR(INTL_PROVISION_DERIVE);
	page_content += F("</button></div></section>"
		"<section id='provision-result' class='config-section' hidden><h2 class='config-section__title'>");
	page_content += FPSTR(INTL_PROVISION_ADDRESS);
	page_content += F("</h2><div class='config-section__body'>"
		"<p class='form-hint'><strong>");
	page_content += FPSTR(INTL_PROVISION_PATH);
	page_content += F(":</strong> <code id='provision-path' class='code-mono code-mono--inline'></code></p>"
		"<p class='code-mono hub-provision__addr' id='provision-address'></p>"
		"<details class='hub-provision__hex-wrap'><summary>");
	page_content += FPSTR(INTL_PROVISION_HEX);
	page_content += F("</summary>"
		"<p class='code-mono hub-provision__hex' id='provision-hex'></p></details>"
		"<label class='guest-option' for='provision-ack'>"
		"<input type='checkbox' id='provision-ack'>"
		"<span>");
	page_content += FPSTR(INTL_PROVISION_ACK);
	page_content += F("</span></label>"
		"<div class='confirm-action__buttons' style='margin-top:12px'>"
		"<button type='button' class='confirm-btn confirm-btn--danger' id='provision-install' disabled>");
	page_content += FPSTR(INTL_PROVISION_INSTALL);
	page_content += F("</button></div></div></section></form>"
		"<script src='" WEB_STATIC_PROVISION_JS_HREF "' defer></script>");

	append_app_page_body_end(page_content);
}
