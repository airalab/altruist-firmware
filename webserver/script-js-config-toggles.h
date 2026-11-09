#ifndef _SCRIPT_JS_CONFIG_TOGGLES_H
#define _SCRIPT_JS_CONFIG_TOGGLES_H

/** Appended to WEB_PAGE_STATIC_JS_CONFIG (Insight): conditional form fields, OTA, Urban scan. */
#define WEB_PAGE_STATIC_JS_CONFIG_SUFFIX R"rawliteral(
;(function() {
  function byId(id) { return document.getElementById(id); }
  function toggleDisplay(el, show) {
    if (el) el.style.display = show ? 'block' : 'none';
  }
  function bindSelectCustom(selId, customVal, wrapId, inputId) {
    var sel = byId(selId);
    if (!sel) return;
    function sync() {
      var custom = sel.value === customVal;
      toggleDisplay(byId(wrapId), custom);
      var input = byId(inputId);
      if (input) input.disabled = !custom;
    }
    sync();
    sel.onchange = sync;
  }
  bindSelectCustom('robonomics_public_node_select', 'custom',
    'robonomics_public_node_custom_wrap', 'robonomics_public_node_custom');

  var connMode = byId('robonomics_connectivity_mode');
  if (connMode) {
    function syncConn() {
      var m = connMode.value || 'auto';
      toggleDisplay(byId('robonomics_connectivity_preset_wrap'), m === 'preset');
      toggleDisplay(byId('robonomics_connectivity_host_wrap'), m === 'custom');
      var ta = byId('robonomics_connectivity_hosts');
      var wTa = ta ? ta.parentElement : null;
      toggleDisplay(wTa, m === 'pool');
      toggleDisplay(byId('robonomics_connectivity_hosts_hint'), m === 'pool');
      if (ta) ta.disabled = m !== 'pool';
    }
    syncConn();
    connMode.onchange = syncConn;
  }

  var urbanCb = byId('use_custom_urban');
  if (urbanCb) {
    function syncUrban() {
      var custom = urbanCb.checked;
      var urbanField = byId('custom_altruist_urban');
      var chosenField = byId('chosen_altruist_urban');
      if (urbanField) urbanField.disabled = !custom;
      if (chosenField) chosenField.disabled = custom;
      if (typeof window.stripMainFromExtraPicks === 'function') window.stripMainFromExtraPicks();
    }
    syncUrban();
    urbanCb.onchange = syncUrban;
  }

  var autoUpdate = byId('auto_update');
  if (autoUpdate) {
    function syncOta() {
      var on = autoUpdate.checked;
      var beta = byId('use_beta');
      if (beta) beta.disabled = !on;
    }
    syncOta();
    autoUpdate.onchange = syncOta;
  }

  var offHour = byId('leds_off_hour');
  var onHour = byId('leds_on_hour');
  if (offHour) { offHour.min = '0'; offHour.max = '23'; offHour.step = '1'; }
  if (onHour) { onHour.min = '0'; onHour.max = '23'; onHour.step = '1'; }

  var morningAuto = byId('analytics_morning_autoswitch');
  if (morningAuto) {
    function syncMorningDisplay() {
      toggleDisplay(byId('analytics_morning_end_wrap'), morningAuto.checked);
      var endInput = byId('analytics_morning_end_time');
      if (endInput) endInput.disabled = !morningAuto.checked;
    }
    syncMorningDisplay();
    morningAuto.onchange = syncMorningDisplay;
  }

  var configTabs = document.querySelectorAll('.config-nav .tab');
  function nudgeGpsMap() {
    if (!byId('map')) return;
    window.dispatchEvent(new Event('resize'));
  }
  if (byId('map')) {
    setTimeout(nudgeGpsMap, 200);
    setTimeout(nudgeGpsMap, 900);
    window.addEventListener('load', function() { setTimeout(nudgeGpsMap, 100); });
    if (window.ResizeObserver) {
      var mapBox = document.querySelector('.map-container');
      if (mapBox) {
        new ResizeObserver(function() { nudgeGpsMap(); }).observe(mapBox);
      }
    }
  }
  if (configTabs.length) {
    var hashTab = { integrations: 3, export: 3, advanced: 2, more: 2, map: 1, robonomics: 1 };
    function openConfigTabFromHash() {
      var h = (location.hash || '').replace(/^#/, '').toLowerCase();
      var id = hashTab[h];
      if (id && configTabs[id - 1]) configTabs[id - 1].click();
      if (id === 1) setTimeout(nudgeGpsMap, 150);
    }
    openConfigTabFromHash();
    window.addEventListener('hashchange', openConfigTabFromHash);
    configTabs.forEach(function(tab) {
      tab.addEventListener('click', function() {
        if (tab.dataset.id === '1') setTimeout(nudgeGpsMap, 150);
      });
    });
  }

  function extraMainUrbanIp() {
    var customCb = byId('use_custom_urban');
    var custom = byId('custom_altruist_urban');
    var chosen = byId('chosen_altruist_urban');
    if (customCb && customCb.checked && custom) {
      var v = String(custom.value || '').trim();
      if (v) return v;
    }
    if (chosen) {
      var c = String(chosen.value || '').trim();
      if (c && c !== '__custom__') return c;
    }
    return '';
  }
  function fillUrbanSelect(selectEl, devices, keepValue, includeEmpty, skipIp) {
    if (!selectEl) return;
    var cur = keepValue != null ? keepValue : selectEl.value;
    if (skipIp && cur === skipIp) cur = '';
    selectEl.innerHTML = '';
    if (includeEmpty) {
      var none = document.createElement('option');
      none.value = '';
      none.textContent = ")rawliteral" INTL_URBAN_NONE R"rawliteral(";
      selectEl.appendChild(none);
    }
    devices.forEach(function(d) {
      if (skipIp && d.ip === skipIp) return;
      var o = document.createElement('option');
      o.value = d.ip;
      o.textContent = d.hostname ? (d.hostname + ' (' + d.ip + ')') : d.ip;
      if (d.ip === cur) o.selected = true;
      selectEl.appendChild(o);
    });
    if (includeEmpty) {
      var custom = document.createElement('option');
      custom.value = '__custom__';
      custom.textContent = ")rawliteral" INTL_URBAN_CUSTOM_IP R"rawliteral(";
      selectEl.appendChild(custom);
    }
    if (cur && cur !== skipIp && selectEl.value !== cur) {
      var extra = document.createElement('option');
      extra.value = cur;
      extra.textContent = cur;
      extra.selected = true;
      selectEl.insertBefore(extra, selectEl.lastChild);
    }
  }
  function setScanStatus(msg) {
    ['scan_status', 'scan_status_extra'].forEach(function(id) {
      var el = byId(id);
      if (el) el.textContent = msg;
    });
  }
  function runUrbanScan(btn) {
    var sel = byId('chosen_altruist_urban');
    if (btn) btn.disabled = true;
    setScanStatus(")rawliteral" INTL_SCAN_SCANNING R"rawliteral(");
    fetch('/scan_urbans').then(function(r) { return r.json(); }).then(function(devices) {
      if (!devices.length) {
        setScanStatus(")rawliteral" INTL_SCAN_NO_URBANS R"rawliteral(");
      } else {
        setScanStatus(")rawliteral" INTL_SCAN_FOUND_PREFIX R"rawliteral(" + devices.length + ")rawliteral" INTL_SCAN_FOUND_SUFFIX R"rawliteral(");
      }
      if (sel) fillUrbanSelect(sel, devices, sel.value, false);
      var mainIp = extraMainUrbanIp();
      document.querySelectorAll('.js-extra-urban-pick').forEach(function(pick) {
        var row = pick.closest('.extra-urban-row');
        var ipInput = row ? row.querySelector('.js-extra-ip') : null;
        fillUrbanSelect(pick, devices, ipInput ? ipInput.value : pick.value, true, mainIp);
        if (typeof window.syncExtraUrbanPick === 'function') window.syncExtraUrbanPick(pick);
      });
      if (btn) btn.disabled = false;
    }).catch(function(e) {
      setScanStatus(")rawliteral" INTL_SCAN_FAILED R"rawliteral(" + e);
      if (btn) btn.disabled = false;
    });
  }
  var scanBtn = byId('btn_scan_urbans');
  if (scanBtn) scanBtn.addEventListener('click', function() { runUrbanScan(scanBtn); });
  var scanBtnExtra = byId('btn_scan_urbans_extra');
  if (scanBtnExtra) scanBtnExtra.addEventListener('click', function() { runUrbanScan(scanBtnExtra); });

  var extraList = byId('extra-urban-list');
  var extraTpl = byId('extra-urban-row-tpl');
  var extraHidden = byId('extra_urbans');
  var extraAdd = byId('btn-add-extra-urban');
  var EXTRA_URBAN_MAX = extraList ? (parseInt(extraList.getAttribute('data-max'), 10) || 16) : 16;
  function extraRowCount() {
    return extraList ? extraList.querySelectorAll('.extra-urban-row').length : 0;
  }
  function syncExtraAddBtn() {
    if (extraAdd) extraAdd.style.display = extraRowCount() >= EXTRA_URBAN_MAX ? 'none' : '';
  }
  function serializeExtraUrbans() {
    if (!extraHidden || !extraList) return;
    var parts = [];
    extraList.querySelectorAll('.extra-urban-row').forEach(function(row) {
      var pick = row.querySelector('.js-extra-urban-pick');
      var ipInput = row.querySelector('.js-extra-ip');
      var nameInput = row.querySelector('.js-extra-name');
      var ip = '';
      if (pick && pick.value === '__custom__') ip = ipInput ? String(ipInput.value || '').trim() : '';
      else if (pick) ip = String(pick.value || '').trim();
      if (!ip) return;
      if (ip === extraMainUrbanIp()) return;
      var name = nameInput ? String(nameInput.value || '').trim() : '';
      parts.push(ip.replace(/[|;]/g, '') + '|' + name.replace(/[|;]/g, ' '));
    });
    extraHidden.value = parts.join(';');
  }
  function syncExtraUrbanPick(sel) {
    if (!sel) return;
    var row = sel.closest ? sel.closest('.extra-urban-row') : null;
    var wrap = row ? row.querySelector('.js-extra-ip-wrap') : null;
    var input = row ? row.querySelector('.js-extra-ip') : null;
    if (sel.value === '__custom__') {
      toggleDisplay(wrap, true);
    } else {
      if (input) input.value = sel.value;
      toggleDisplay(wrap, false);
    }
    serializeExtraUrbans();
  }
  window.syncExtraUrbanPick = syncExtraUrbanPick;
  function stripMainFromExtraPicks() {
    var main = extraMainUrbanIp();
    document.querySelectorAll('.js-extra-urban-pick').forEach(function(pick) {
      if (!main) return;
      for (var i = pick.options.length - 1; i >= 0; i--) {
        if (pick.options[i].value === main) pick.remove(i);
      }
      if (pick.value === main) pick.value = '';
    });
    serializeExtraUrbans();
  }
  window.stripMainFromExtraPicks = stripMainFromExtraPicks;
  function bindExtraRow(row) {
    if (!row) return;
    var pick = row.querySelector('.js-extra-urban-pick');
    var nameInput = row.querySelector('.js-extra-name');
    var ipInput = row.querySelector('.js-extra-ip');
    var rm = row.querySelector('.js-extra-urban-remove');
    if (pick) {
      if (ipInput && ipInput.value) {
        var match = false;
        for (var i = 0; i < pick.options.length; i++) {
          if (pick.options[i].value === ipInput.value) { match = true; break; }
        }
        pick.value = match ? ipInput.value : '__custom__';
      }
      pick.addEventListener('change', function() { syncExtraUrbanPick(pick); });
      syncExtraUrbanPick(pick);
    }
    if (nameInput) nameInput.addEventListener('input', serializeExtraUrbans);
    if (ipInput) ipInput.addEventListener('input', serializeExtraUrbans);
    if (rm) rm.addEventListener('click', function() {
      row.remove();
      serializeExtraUrbans();
      syncExtraAddBtn();
    });
  }
  if (extraList && extraTpl) {
    extraList.querySelectorAll('.extra-urban-row').forEach(bindExtraRow);
    if (extraAdd) extraAdd.addEventListener('click', function() {
      if (extraRowCount() >= EXTRA_URBAN_MAX) return;
      extraList.appendChild(extraTpl.content.cloneNode(true));
      bindExtraRow(extraList.lastElementChild);
      serializeExtraUrbans();
      syncExtraAddBtn();
    });
    if (extraHidden && extraHidden.form) extraHidden.form.addEventListener('submit', serializeExtraUrbans);
    var chosenUrban = byId('chosen_altruist_urban');
    if (chosenUrban) chosenUrban.addEventListener('change', stripMainFromExtraPicks);
    var customUrban = byId('custom_altruist_urban');
    if (customUrban) customUrban.addEventListener('input', stripMainFromExtraPicks);
    syncExtraAddBtn();
    serializeExtraUrbans();
    stripMainFromExtraPicks();
  }

  document.querySelectorAll('form.js-delete-config').forEach(function(form) {
    var ask = form.querySelector('[data-delete-step="ask"]');
    var confirmStep = form.querySelector('[data-delete-step="confirm"]');
    var askBtn = form.querySelector('.js-delete-ask');
    var cancelBtn = form.querySelector('.js-delete-cancel');
    if (!ask || !confirmStep || !askBtn) return;
    askBtn.addEventListener('click', function() {
      ask.hidden = true;
      confirmStep.hidden = false;
    });
    if (cancelBtn) {
      cancelBtn.addEventListener('click', function() {
        confirmStep.hidden = true;
        ask.hidden = false;
      });
    }
  });

})();
)rawliteral"

#endif // _SCRIPT_JS_CONFIG_TOGGLES_H
