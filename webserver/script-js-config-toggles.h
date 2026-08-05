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

  var scanBtn = byId('btn_scan_urbans');
  if (scanBtn) {
    scanBtn.addEventListener('click', function() {
      var sel = byId('chosen_altruist_urban');
      var st = byId('scan_status');
      if (!sel || !st) return;
      scanBtn.disabled = true;
      // Quotes stay in the rawliteral parts: INTL macros expand to unquoted text when concatenated.
      st.textContent = ")rawliteral" INTL_SCAN_SCANNING R"rawliteral(";
      fetch('/scan_urbans').then(function(r) { return r.json(); }).then(function(devices) {
        var cur = sel.value;
        sel.innerHTML = '';
        if (!devices.length) {
          st.textContent = ")rawliteral" INTL_SCAN_NO_URBANS R"rawliteral(";
        } else {
          st.textContent = ")rawliteral" INTL_SCAN_FOUND_PREFIX R"rawliteral(" + devices.length + ")rawliteral" INTL_SCAN_FOUND_SUFFIX R"rawliteral(";
          devices.forEach(function(d) {
            var o = document.createElement('option');
            o.value = d.ip;
            o.textContent = d.hostname + ' (' + d.ip + ')';
            if (d.ip === cur) o.selected = true;
            sel.appendChild(o);
          });
        }
        scanBtn.disabled = false;
      }).catch(function(e) {
        st.textContent = ")rawliteral" INTL_SCAN_FAILED R"rawliteral(" + e;
        scanBtn.disabled = false;
      });
    });
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
