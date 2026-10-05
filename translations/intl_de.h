/*
 *	airRohr firmware
 *	Copyright (C) 2016-2018  Code for Stuttgart a.o.
 *
 *  German translations
 *
 *	Texts should be as short as possible
 */

#define INTL_LANG "DE"
#define INTL_PM_SENSOR "Feinstaubsensor"
const char INTL_CONFIGURATION[] PROGMEM = "Konfiguration";
#define INTL_COMMON_SETTINGS "Verbindung"
#define INTL_APIS_SETTINGS "Datenexport"
#define INTL_NAV_MONITOR "Daten & Status"
#define INTL_NAV_SETTINGS "Einstellungen"
#define INTL_NAV_MAINTENANCE "Wartung"
#define INTL_NAV_HOME "Start"
#define INTL_NAV_READINGS "Messwerte"
#define INTL_NAV_STATUS "Status"
#define INTL_NAV_MAIN "Hauptnavigation"
#define INTL_BREADCRUMB_ARIA "Brotkrumennavigation"
#define INTL_DASH_TITLE "Übersicht"
#define INTL_DASH_ALL_READINGS "Alle Messwerte"
#define INTL_DASH_DEVICE_HEALTH "Gerätezustand"
#define INTL_DASH_WIFI_OK "WLAN verbunden"
#define INTL_DASH_WIFI_OFF "WLAN offline"
#define INTL_DASH_DATALOG_OK "Datalog OK"
#define INTL_DASH_DATALOG_ERR "Datalog-Problem"
#define INTL_DASH_MAP_OK "Karte OK"
#define INTL_DASH_MAP_ERR "Karten-Problem"
#define INTL_DASH_NAV "Schnellzugriff"
#define INTL_DASH_SECTION_MONITOR_INTRO "Sehen Sie, was Ihr Sensor misst und ob er funktioniert."
#define INTL_DASH_SECTION_SETTINGS_INTRO "WLAN, Updates und wie Ihre Daten geteilt werden."
#define INTL_DASH_SECTION_MAP_INTRO "Finden Sie dieses Gerät auf der öffentlichen Luftqualitätskarte."
#define INTL_DASH_READINGS_DESC "Temperatur, Luftfeuchtigkeit und Luftqualität live"
#define INTL_DASH_STATUS_DESC "Verbindung, Firmware und Gerätedetails"
#define INTL_DASH_CONFIG_DESC "WLAN, Standort und Veröffentlichungsoptionen"
#define INTL_DASH_GROUP_DESC "Messwerte mit Sensoren in der Nähe kombinieren"
#define INTL_DASH_OTA_DESC "Firmware-Updates prüfen und installieren"
#define INTL_DASH_SCREEN_DESC "Auswählen, was das Display anzeigt"
#define INTL_DASH_MAP_DESC "sensors.social in einem neuen Tab öffnen"
#define INTL_DASH_SECTION_MAINTENANCE_INTRO "Erweiterte Aktionen — nur wenn Sie wissen, was Sie tun."
#define INTL_DASH_DEBUG_DESC "Ausführlichere Logs zur Fehlersuche"
#define INTL_DASH_RESTART_DESC "Sensor neu starten (etwa eine Minute)"
#define INTL_DASH_DELETE_CONFIG_DESC "Gespeicherte Einstellungen vom Gerät löschen"
#define INTL_DASH_HEALTH_TITLE "Auf einen Blick"
#define INTL_HUB_LOCAL_TITLE "Lokal"
#define INTL_HUB_LOCAL_DESC "Messwerte und Geräteeinstellungen"
#define INTL_HUB_SOCIAL_DESC "Öffentliche Karte und Robonomics-Netzwerk"
#define INTL_HUB_CUSTOM_DESC "Home Assistant, API, InfluxDB, CSV"
#define INTL_NAV_ADVANCED "System"
#define INTL_HUB_ADVANCED_DESC "Debug-Log, Neustart und Werkseinstellungen"
#define INTL_HUB_DIV_NETWORK "Netzwerk"
#define INTL_HUB_DIV_ACCESS "Zugang"
#define INTL_HUB_DIV_DEVICE "Gerät"
#define INTL_HUB_DIV_PUBLISH "Veröffentlichung"
#define INTL_HUB_DIV_LOCATION "Standort"
#define INTL_HUB_DIV_DIAGNOSTICS "Diagnose"
#define INTL_HUB_DIV_DANGER "Gefahrenbereich"
#define INTL_DASH_GROUP_LOCAL_INTRO "Messwerte, Einstellungen und Wartung in Ihrem Heimnetz."
#define INTL_DASH_GROUP_SOCIAL_TITLE "Karte"
#define INTL_DASH_GROUP_SOCIAL_INTRO "Öffentliche Karte, Robonomics-Netzwerk und was Sie veröffentlichen."
#define INTL_DASH_GROUP_CUSTOM_TITLE "Eigene"
#define INTL_DASH_GROUP_CUSTOM_INTRO "Home Assistant, eigene API, InfluxDB und CSV-Export."
#define INTL_DASH_CAT_DATA "Daten"
#define INTL_DASH_CAT_SETTINGS "Einstellungen"
#define INTL_DASH_CAT_MAINTENANCE "Wartung"
#define INTL_DASH_CAT_MAP "Karte"
#define INTL_DASH_CAT_NETWORK "Robonomics"
#define INTL_DASH_CAT_INTEGRATIONS "Integrationen"
#define INTL_DASH_CONFIG_MAP_DESC "Wählen Sie, welche Messwerte auf der öffentlichen Karte erscheinen."
#define INTL_DASH_CONFIG_ROBONOMICS_DESC "Einstellungen für Eigentümer, Node und On-Chain-Daten."
#define INTL_DASH_CONFIG_INTEGRATIONS_DESC "Den Tab Datenexport in der Konfiguration öffnen."
#define INTL_DASH_CUSTOM_API_DESC "HTTP-Push an Ihren Server (z. B. Home Assistant REST)."
#define INTL_DASH_INFLUX_DESC "Messwerte an InfluxDB senden."
#define INTL_DASH_CSV_DESC "CSV-Datei auf dem Gerät oder an Ihrem Endpunkt schreiben."
#define INTL_PAGE_READINGS_INTRO "Neueste Messwerte Ihrer Sensoren."
#define INTL_PAGE_STATUS_INTRO "Zuerst ein kurzer Zustandscheck — Details in den Abschnitten unten."
#define INTL_PAGE_OTA_INTRO "Nach Updates suchen oder die Firmware-Sprache wechseln."
#define INTL_PAGE_DEBUG_INTRO "Live-Logausgabe und wie detailliert protokolliert wird."
#define INTL_PAGE_RESTART_INTRO "Der Sensor startet neu und verbindet sich wieder mit dem WLAN."
#define INTL_PAGE_DELETE_CONFIG_INTRO "Wählen Sie, was gelöscht werden soll. Das lässt sich nicht rückgängig machen."
#define INTL_DELETE_CONFIG_ALL "Alle Einstellungen"
#define INTL_DELETE_CONFIG_WIFI "Nur WLAN"
#define INTL_DELETE_CONFIG_ALL_DESC "Alles, was auf dem Gerät gespeichert ist"
#define INTL_DELETE_CONFIG_WIFI_DESC "Nur das gespeicherte WLAN vergessen"
#define INTL_WIFI_CREDENTIALS_DELETED "WLAN-Zugangsdaten wurden gelöscht. Sie können diese Seite schließen."
#define INTL_CONFIG_TAB_INTEGRATIONS "Datenexport"
#define INTL_CONFIG_PANEL1_INTRO "WLAN, Robonomics, Standort und welche Daten auf der öffentlichen Karte erscheinen."
#define INTL_CONFIG_PANEL2_INTRO "Sicherheit, Firmware-Updates und Optionen für erfahrene Nutzer."
#define INTL_CONFIG_PANEL3_INTRO "Optionale Exporte an Ihre eigenen Dienste. Öffnen Sie einen Abschnitt nur bei Bedarf."
#define INTL_BADGE_BETA "Beta"
#define INTL_BADGE_EXPERIMENTAL "Experimentell"
#define INTL_WIFI_NETWORKS "Lade WLAN-Netzwerke ..."
#define INTL_LANGUAGE "Sprache"
#define INTL_NO_WLAN_PWD "Aktivieren, wenn das WLAN kein Passwort hat"
const char INTL_NO_NETWORKS[] PROGMEM =  "Kein WLAN-Netzwerk gefunden";
const char INTL_NETWORKS_FOUND[] PROGMEM = "Gefundene Netzwerke: ";
const char INTL_AB_HIER_NUR_ANDERN[] PROGMEM = "Erweiterte Einstellungen (nur wenn Sie wissen, was Sie tun)";
const char INTL_SAVE[] PROGMEM = "Speichern";
const char INTL_SENSORS[] PROGMEM = "Sensoren";
const char INTL_MORE_SENSORS[] PROGMEM = "Weitere Sensoren";
const char INTL_SDS011[] PROGMEM = "SDS011 ({pm})";
const char INTL_GC[] PROGMEM = "Geigerzähler";
const char INTL_DBMETER[] PROGMEM = "Lärmpegelsensor (Sendeintervall muss > 30 s sein)";
const char INTL_I2SNOISE[] PROGMEM = "I2S-Lärmpegelsensor (Sendeintervall muss > 30 s sein)";
const char INTL_PMS[] PROGMEM = "Plantower PMS(1,3,5,6,7)003 ({pm})";
const char INTL_HPM[] PROGMEM = "Honeywell PM ({pm})";
const char INTL_NPM[] PROGMEM = "Tera Sensor Next PM ({pm})";
const char INTL_SPS30[] PROGMEM = "Sensirion SPS30 ({pm})";
const char INTL_PPD42NS[] PROGMEM = "PPD42NS ({pm})";
const char INTL_DHT22[] PROGMEM = "DHT22 ({t}, {h})";
const char INTL_HTU21D[] PROGMEM = "HTU21D ({t}, {h})";
const char INTL_BMP180[] PROGMEM = "BMP180 ({t}, {p})";
const char INTL_BMX280[] PROGMEM = "BME280 ({t}, {h}, {p}), BMP280 ({t}, {p})";
const char INTL_SHT3X[] PROGMEM = "SHT3X ({t}, {h})";
const char INTL_DS18B20[] PROGMEM = "DS18B20 ({t})";
const char INTL_CCS811_27[] PROGMEM = "CCS811 (I2C: 0x5A)";
const char INTL_CCS811_3F[] PROGMEM = "CCS811 (I2C: 0x5B)";
const char INTL_DNMS[] PROGMEM = "DNMS ({l_a})";
const char INTL_DNMS_CORRECTION[] PROGMEM = "Korrektur in dB(A)";
const char INTL_TEMP_CORRECTION[] PROGMEM = "Temperaturkorrektur in °C";
#define INTL_PRESSURE_CORRECTION INTL_PRESSURE_CORRECTION
const char INTL_PRESSURE_CORRECTION[] PROGMEM = "Luftdruckkorrektur in hPa";
const char INTL_CUSTOM_ALTRUIST[] PROGMEM = "Eigene Altruist-Urban-Adresse";
const char INTL_USE_CUSTOM_URBAN[] PROGMEM = "Eigene Altruist-Urban-Adresse verwenden";
#define INTL_MAIN_URBAN "Haupt-Urban (erste Seite mit Insight)"
#define INTL_PANEL_TITLE_EXTRA_URBANS "Weitere Urbans"
#define INTL_URBAN_PAGES_GROUP "Weitere Urbans auf dem Display"
#define INTL_URBAN_PAGES_HINT "Fügen Sie weitere Urbans neben dem Haupt-Urban hinzu. Jeder bekommt eine eigene Display-Seite nur mit seinen Daten. Wählen Sie ein gefundenes Gerät oder geben Sie eine IP ein und vergeben Sie einen Namen (Küche, Balkon…)."
#define INTL_EXTRA_URBAN "Urban"
#define INTL_URBAN_NAME "Name auf dem Display"
#define INTL_URBAN_NONE "— keiner —"
#define INTL_URBAN_CUSTOM_IP "IP-Adresse"
#define INTL_ADD_EXTRA_URBAN "Urban hinzufügen"
#define INTL_REMOVE_EXTRA_URBAN "Entfernen"
const char INTL_INSIGHT_STANDALONE[] PROGMEM = "Insight eigenständig";

// Urban selection (guest setup & config page)
#define INTL_SCANNING_URBANS "Suche nach Altruist-Urban-Geräten..."
#define INTL_SELECT_URBAN_TITLE "Altruist-Urban-Gerät auswählen"
#define INTL_SELECT_URBAN_DESC "Wählen Sie, von welchem Urban dieses Insight die Außenmesswerte liest."
#define INTL_NO_URBANS_FOUND "Keine Altruist-Urban-Geräte in diesem Netzwerk gefunden. Prüfen Sie, ob Ihr Urban eingeschaltet und mit demselben WLAN verbunden ist. Sie können unten eine eigene IP-Adresse eingeben und speichern. Urban lässt sich später in den Einstellungen hinzufügen oder ändern."
#define INTL_USE_CUSTOM_IP "Eigene IP-Adresse verwenden:"
#define INTL_SETUP_INSIGHT_MODE_HINT "Um mit einem Altruist Urban zu koppeln, aktivieren Sie das Kästchen unten und drücken Sie Weiter. Ohne Häkchen wird die Einrichtung im eigenständigen Modus fortgesetzt."
#define INTL_SETUP_PAIR_WITH_URBAN "Verbindung zu einem Altruist Urban jetzt einrichten"
#define INTL_SETUP_CONTINUE "Weiter"
#define INTL_SKIP_URBAN_SELECTION "Überspringen &mdash; später in den Einstellungen konfigurieren"
#define INTL_SETUP_COMPLETE "Einrichtung abgeschlossen"
#define INTL_SETTINGS_SAVED "Einstellungen gespeichert"
#define INTL_DEVICE_RESTARTING "Gerät wird neu gestartet..."
#define INTL_GUEST_CONNECTED "Verbunden"
#define INTL_GUEST_CONNECTING "Verbinde mit WLAN…"
#define INTL_GUEST_CONNECTING_HINT "Das dauert meist nur ein paar Sekunden. Lassen Sie diese Seite geöffnet."
#define INTL_GUEST_CONNECT_FAILED "Verbindung fehlgeschlagen"
#define INTL_GUEST_CONNECT_FAILED_HINT "Prüfen Sie Netzwerkname und Passwort und versuchen Sie es erneut."
#define INTL_GUEST_CONNECT_FAILED_INSIGHT "Prüfen Sie Netzwerkname und Passwort und versuchen Sie es auf der Einrichtungsseite erneut."
#define INTL_GUEST_WIFI_STEP_TITLE "WLAN verbunden"
#define INTL_GUEST_SETUP_STEP_1_LABEL "Schritt 1 von 2"
#define INTL_GUEST_SETUP_STEP_2_LABEL "Schritt 2 von 2"
#define INTL_GUEST_SETUP_STEP_1_TITLE "Mit WLAN verbinden"
#define INTL_GUEST_INSIGHT_FINISH_HINT "Drücken Sie Weiter, um die Einrichtung abzuschließen und das Gerät neu zu starten."
#define INTL_GUEST_INSIGHT_AUTO_FINISH_HINT "Wenn Sie nicht Weiter drücken, wird die Einrichtung automatisch (eigenständig) abgeschlossen in"
#define INTL_GUEST_INSIGHT_AUTO_FINISH_SUFFIX "Sekunden (eigenständiger Modus)."
#define INTL_GUEST_IP_ADDRESS "IP-Adresse:"
#define INTL_GUEST_OPEN_IP_HINT "Kopieren Sie die IP-Adresse und öffnen Sie sie im Browser."
#define INTL_GUEST_DEVICE_INFO_HINT "iPhone: Sichern → Teilen → In Dateien sichern. Android: landet in Downloads. Oder als Text kopieren."
#define INTL_GUEST_DEVICE_INFO_DOWNLOAD "Geräteinfo speichern"
#define INTL_GUEST_DEVICE_INFO_COPY "Als Text kopieren"
#define INTL_GUEST_DEVICE_INFO_SAVED "Gespeichert"
#define INTL_GUEST_DEVICE_INFO_SHARED "Im Teilen-Menü „In Dateien sichern“ wählen"
#define INTL_GUEST_DEVICE_INFO_COPIED "Kopiert — in Notizen einfügen"
#define INTL_GUEST_DEVICE_INFO_SAVE_FAIL "Speichern fehlgeschlagen — IP oben kopieren"
#define INTL_GUEST_SENSOR_ADDRESS "Robonomics-Adresse:"
#define INTL_GUEST_RESTART_PAUSE_HINT "Das Gerät startet gleich neu — kopieren oder speichern Sie vorher Ihre IP."
#define INTL_GUEST_FINISH_SETUP "Einrichtung abschließen"
#define INTL_GUEST_FINISHING_SETUP "Einrichtung wird abgeschlossen — Neustart…"
#define INTL_GUEST_KEEP_OPEN_HINT "Schließen Sie diese Seite erst, wenn Sie Weiter gedrückt haben."
#define INTL_DISP_MAP_PROMO_TITLE "Bessere Auswertung auf dem Smartphone"
#define INTL_DISP_MAP_PROMO_LINE1 "Einfach unsere Webkarte öffnen: AQI, Verlauf,"
#define INTL_DISP_MAP_PROMO_LINE2 "farbige Diagramme, einfaches Teilen und"
#define INTL_DISP_MAP_PROMO_LINE3 "weitere Funktionen in Kürze"
#define INTL_DISP_MAP_DOMAIN "SENSORS.SOCIAL"
#define INTL_SCAN_BTN "Suchen"
#define INTL_SCAN_SCANNING "Suche..."
#define INTL_SCAN_NO_URBANS "Kein Urban per mDNS gefunden. Für C3-Urban eigene Urban-IP (Browser-IP) nutzen."
#define INTL_SCAN_FOUND_PREFIX "Gefunden: "
#define INTL_SCAN_FOUND_SUFFIX " Urban-Gerät(e)."
#define INTL_SCAN_FAILED "Suche fehlgeschlagen: "

const char INTL_NEO6M[] PROGMEM = "GPS (NEO 6M)";
const char INTL_RWS_OWNER[] PROGMEM = "RWS-Eigentümeradresse";
#define INTL_NODE_ID INTL_NODE_ID
const char INTL_NODE_ID[] PROGMEM = "CPS-Node-ID (0 = nicht gesetzt)";
const char INTL_GROUP_MENU[] PROGMEM = "Gerätegruppe (RWS)";
const char INTL_GROUP_INTRO[] PROGMEM = "Wählen Sie, wie dieses Gerät an Robonomics Web Services teilnimmt (Eigentümer und On-Chain-Geräteliste).";
const char INTL_GROUP_MODE_TITLE[] PROGMEM = "Betriebsmodus";
const char INTL_GROUP_MODE_STANDALONE[] PROGMEM = "Eigenständig — dieses Gerät ist sein eigener Master (setDevices nur mit sich selbst)";
const char INTL_GROUP_MODE_MASTER[] PROGMEM = "Gruppe erstellen — dieses Gerät ist der Gruppen-Master";
const char INTL_GROUP_MODE_FOLLOWER[] PROGMEM = "Gruppe beitreten — einem Master-Gerät folgen (Master-Adresse unten eingeben)";
const char INTL_GROUP_MODE_MANUAL[] PROGMEM = "Manueller Eigentümer — alt: nur Eigentümer setzen, kein automatisches setDevices";
const char INTL_GROUP_SELF_ADDRESS[] PROGMEM = "Robonomics-Adresse dieses Geräts (beim Beitritt zum Master kopieren)";
const char INTL_GROUP_MASTER_PANEL[] PROGMEM = "Gruppen-Master";
const char INTL_GROUP_FOLLOWER_PANEL[] PROGMEM = "Gruppe beitreten";
const char INTL_GROUP_MANUAL_PANEL[] PROGMEM = "Manueller Eigentümer";
const char INTL_GROUP_ID_LABEL[] PROGMEM = "Gruppen-ID";
const char INTL_GROUP_MASTER_ADDRESS[] PROGMEM = "Robonomics-Adresse des Masters";
const char INTL_GROUP_MASTER_INCLUDED[] PROGMEM = "Master-Gerät (wird automatisch zu setDevices hinzugefügt)";
const char INTL_GROUP_KNOWN_DEVICES[] PROGMEM = "Weitere Geräte — Follower (SS58, eins pro Zeile)";
const char INTL_GROUP_KNOWN_DEVICES_HINT[] PROGMEM = "Ihre Geräteadresse oben ist immer als Master enthalten. Fügen Sie hier SS58-Adressen der Follower hinzu und speichern Sie.";
const char INTL_GROUP_FOLLOWER_HINT[] PROGMEM = "Kopieren Sie Ihre Adresse oben in die Geräteliste des Masters, geben Sie hier die Master-Adresse ein und speichern Sie.";
const char INTL_GROUP_MANUAL_HINT[] PROGMEM = "Datalog nutzt diesen Eigentümer. setDevices wird nicht automatisch aufgerufen.";
const char INTL_GROUP_STATUS_GROUP_CREATING[] PROGMEM = "Gruppe erstellt — On-Chain-Synchronisierung läuft";
const char INTL_GROUP_STATUS_LIST_UPDATED[] PROGMEM = "Geräteliste aktualisiert — On-Chain-Synchronisierung läuft";
const char INTL_GROUP_STATUS_LIST_SYNCED[] PROGMEM = "Geräteliste on-chain synchronisiert";
const char INTL_GROUP_STATUS_LABEL[] PROGMEM = "Status";
const char INTL_GROUP_STATUS_CREATED[] PROGMEM = "Gruppe erstellt, Geräte synchronisiert";
const char INTL_GROUP_CURRENT_DEVICES[] PROGMEM = "Aktuelle Geräteliste";
const char INTL_GROUP_STATUS_PENDING[] PROGMEM = "Synchronisierung ausstehend";
const char INTL_GROUP_STATUS_DEVICES_SYNCED[] PROGMEM = "Geräte on-chain synchronisiert";
const char INTL_GROUP_STATUS_JOINED[] PROGMEM = "Gruppe beigetreten";
const char INTL_GROUP_STATUS_MANUAL[] PROGMEM = "Manueller Eigentümer eingerichtet";
const char INTL_GROUP_SAVE_OK[] PROGMEM = "Gruppeneinstellungen gespeichert.";
const char INTL_GROUP_BACKUP_TITLE[] PROGMEM = "Gerätebackup";
const char INTL_GROUP_BACKUP_HINT[] PROGMEM =
	"Dieses Gerät ist sein eigener Eigentümer. Nach dem Speichern laden wir einmalig ein Backup herunter — importieren Sie es beim Login auf sensors.map, um verschlüsselte Messwerte zu entschlüsseln, oder stellen Sie es auf einem anderen Gerät wieder her. Halten Sie die Datei privat (lokales HTTP ist nicht verschlüsselt).";
const char INTL_GROUP_BACKUP_SAVED_NOTICE[] PROGMEM =
	"Download des Gerätebackups gestartet. Bewahren Sie es sicher auf und importieren Sie es dann auf sensors.map oder stellen Sie es unter Wartung wieder her.";
const char INTL_GROUP_BACKUP_DOWNLOAD_AGAIN[] PROGMEM = "Backup erneut herunterladen…";
const char INTL_GROUP_BACKUP_REDOWNLOAD_CONFIRM[] PROGMEM =
	"Gerätebackup erneut herunterladen? Wer diese Datei hat, kann Ihre verschlüsselten Messwerte entschlüsseln und Ihre Einstellungen wiederherstellen.";
const char INTL_DEVICE_BACKUP_TITLE[] PROGMEM = "Backup & Wiederherstellung";
const char INTL_DEVICE_BACKUP_HINT[] PROGMEM =
	"Vollständiges Backup herunterladen (Einstellungen + Eigentümerschlüssel). Sie können es hier wiederherstellen oder dieselbe Datei beim Login auf sensors.map importieren.";
const char INTL_DEVICE_BACKUP_DOWNLOAD[] PROGMEM = "Backup herunterladen";
const char INTL_DEVICE_BACKUP_RESTORE[] PROGMEM = "Aus Backup wiederherstellen";
const char INTL_DEVICE_BACKUP_FILE_LABEL[] PROGMEM = "Backup-Datei";
const char INTL_DEVICE_BACKUP_FILE_CHOOSE[] PROGMEM = "Datei auswählen…";
const char INTL_DEVICE_BACKUP_FILE_EMPTY[] PROGMEM = "Keine Datei ausgewählt";
const char INTL_DEVICE_BACKUP_FILE_REQUIRED[] PROGMEM = "Bitte zuerst eine Backup-JSON-Datei auswählen.";
const char INTL_DEVICE_BACKUP_RESTORE_HINT[] PROGMEM =
	"Die Wiederherstellung ersetzt alle Einstellungen auf diesem Gerät und startet es neu.";
const char INTL_DEVICE_BACKUP_RESTORE_CONFIRM[] PROGMEM =
	"Einstellungen aus diesem Backup wiederherstellen? Die aktuelle Konfiguration wird ersetzt.";
const char INTL_DEVICE_BACKUP_RESTORE_OK[] PROGMEM = "Backup wiederhergestellt. Neustart…";
const char INTL_DEVICE_BACKUP_RESTORE_FAILED[] PROGMEM = "Backup konnte nicht wiederhergestellt werden. Prüfen Sie das Dateiformat.";
const char INTL_GUEST_RESTORE_HINT[] PROGMEM =
	"Haben Sie ein Backup von vor dem Zurücksetzen? Stellen Sie es hier wieder her, um WLAN, Eigentümerschlüssel und Einstellungen zurückzuholen — danach startet das Gerät neu.";
const char INTL_GROUP_OWNER_ACCESS_TITLE[] PROGMEM = "Kartenzugang (eigener Eigentümer)";
const char INTL_GROUP_OWNER_ACCESS_HINT[] PROGMEM =
	"Dieses Gerät ist sein eigener Eigentümer. Nach dem Speichern laden wir einmalig eine geheime JSON-Datei herunter — importieren Sie sie beim Login auf sensors.map, um verschlüsselte Messwerte zu entschlüsseln. Halten Sie die Datei privat (lokales HTTP ist nicht verschlüsselt).";
const char INTL_GROUP_OWNER_ACCESS_SAVED_NOTICE[] PROGMEM =
	"Download der Eigentümer-JSON gestartet. Bewahren Sie sie sicher auf und importieren Sie sie dann auf sensors.map.";
const char INTL_GROUP_OWNER_ACCESS_DOWNLOAD_AGAIN[] PROGMEM = "Erneut herunterladen…";
const char INTL_GROUP_OWNER_ACCESS_REDOWNLOAD_CONFIRM[] PROGMEM =
	"Geheime Eigentümer-JSON erneut herunterladen? Wer diese Datei hat, kann Ihre verschlüsselten Messwerte entschlüsseln.";
const char INTL_GROUP_SAVE_FAILED[] PROGMEM = "Gruppeneinstellungen konnten nicht gespeichert werden.";
const char INTL_GROUP_SAVE_CONFIG_FAILED[] PROGMEM = "Konfiguration konnte nicht in den Speicher geschrieben werden.";
const char INTL_GROUP_ERROR_INVALID_MASTER[] PROGMEM = "Geben Sie eine gültige Robonomics-Adresse des Masters ein.";
const char INTL_GROUP_ERROR_INVALID_MANUAL_OWNER[] PROGMEM = "Geben Sie eine gültige Robonomics-Adresse des Eigentümers ein.";
const char INTL_SCREEN_MENU[] PROGMEM = "Displaymodus";
const char INTL_SCREEN_INTRO[] PROGMEM = "Wählen Sie, wie das E-Paper-Display aktualisiert wird. Verschiedene Display-Chargen verhalten sich bei Teilaktualisierungen unterschiedlich.";
const char INTL_SCREEN_MODE_SAFE[] PROGMEM = "Sicher";
const char INTL_SCREEN_MODE_SAFE_HINT[] PROGMEM = "Nur vollständige Bildschirmaktualisierungen. Für alle Geräte empfohlen. Verhindert Bildfehler.";
const char INTL_SCREEN_MODE_EXPERIMENTAL[] PROGMEM = "Experimentelle Teilaktualisierung";
const char INTL_SCREEN_MODE_EXPERIMENTAL_HINT[] PROGMEM = "Schnellere Teilaktualisierungen des Bildschirms. Weniger Flackern, kann aber auf manchen Panels Geisterbilder oder Bildfehler verursachen.";
const char INTL_SCREEN_SAVE_OK[] PROGMEM = "Displaymodus gespeichert.";
const char INTL_SCREEN_SAVE_FAILED[] PROGMEM = "Displaymodus konnte nicht gespeichert werden.";
const char INTL_SCREEN_SAVE_INVALID_MODE[] PROGMEM = "Ungültiger Displaymodus ausgewählt.";
const char INTL_SCREEN_SAVE_CONFIG_FAILED[] PROGMEM = "Konfiguration konnte nicht in den Speicher geschrieben werden.";
const char INTL_ROBONOMICS_PUBLIC_NODE[] PROGMEM = "Öffentlicher Robonomics-Node";
const char INTL_ROBONOMICS_CONNECTIVITY_HOST[] PROGMEM = "Robonomics-Karten-Host (Verbindung)";
const char INTL_ROBONOMICS_CONNECTIVITY_HOSTS[] PROGMEM = "Pool der Robonomics-Karten-Hosts (einer pro Zeile)";
const char INTL_ROBONOMICS_CONNECTIVITY_MODE_AUTO[] PROGMEM = "Standard-Pool (automatisch)";
const char INTL_ROBONOMICS_CONNECTIVITY_MODE_PRESET[] PROGMEM = "Fest (Vorgabe)";
const char INTL_ROBONOMICS_CONNECTIVITY_MODE_CUSTOM[] PROGMEM = "Eigener Host";
const char INTL_ROBONOMICS_CONNECTIVITY_MODE_POOL[] PROGMEM = "Eigener Pool";
const char INTL_ROBONOMICS_CONNECTIVITY_PRESET_LABEL[] PROGMEM = "Fester Host (Vorgabe)";
const char INTL_ROBONOMICS_CONNECTIVITY_CUSTOM_LABEL[] PROGMEM = "Eigener Host";
const char INTL_MAP_SEND_CSV[] PROGMEM = "JSON/CSV an die Karte senden (aktuelle Verbindung)";
const char INTL_MAP_SEND_PROTO[] PROGMEM = "Protobuf senden (neues Protokoll)";
const char INTL_MAP_PROTO_HOST[] PROGMEM = "Protobuf-URL (leer = Firmware-Standard)";
const char INTL_MAP_DUAL_HINT[] PROGMEM = "Beides kann gleichzeitig aktiv sein. Protobuf geht zuerst; JSON ist Reserve, falls Protobuf fehlschlägt. Datalog wird nicht doppelt gesendet.";
const char INTL_LORA_UART_ENABLED[] PROGMEM = "An Meshtastic senden (LoRa UART)";
const char INTL_LORA_UART_INTERVAL[] PROGMEM = "LoRa-Sendeintervall (Sek.)";
const char INTL_LORA_DEST_NODE[] PROGMEM = "Meshtastic-Ziel (!xxxxxxxx)";
const char INTL_LORA_UART_HINT[] PROGMEM = "Urban C6: TX GPIO22 / RX GPIO20, 115200. Serial Module: PROTO. Ziel gesetzt: Unicast SignedEnvelope Port 256 an diesen Node (kein Chat). Ziel leer: es wird nichts gesendet.";
const char INTL_ROBONOMICS_PUBLIC_NODE_CUSTOM[] PROGMEM = "Eigener öffentlicher Robonomics-Node";
const char INTL_GUEST_CONNECTED_SENSORS[] PROGMEM = "Verbundene Sensoren";
const char INTL_COORD_LAT[] PROGMEM = "Breitengrad";
const char INTL_COORD_LON[] PROGMEM = "Längengrad";
const char INTL_COORDS[] PROGMEM = "GPS: Breitengrad, Längengrad";
const char INTL_BASICAUTH[] PROGMEM = "Authentifizierung";
#define INTL_REPORT_ISSUE "Problem melden"

#define INTL_PANEL_TITLE_WIFI "WLAN-Zugangsdaten"
#define INTL_PANEL_TITLE_ROBONOMICS "Robonomics"
#define INTL_PANEL_TITLE_GPS "GPS & Sensoren"
#define INTL_PANEL_TITLE_AUTH "Authentifizierung"
#define INTL_PANEL_TITLE_DEBUG "Debug-Level"
#define INTL_PANEL_TITLE_LEDS "LEDs"
#define INTL_PANEL_TITLE_SLEEP_ANALYTICS "Schlafanalyse"
#define INTL_ANALYTICS_GROUP_NIGHT "Nachtfenster"
#define INTL_ANALYTICS_GROUP_MORNING "Morgenanzeige"
#define INTL_ANALYTICS_GROUP_DATA "Daten"
#define INTL_ANALYTICS_MORNING_AUTOSWITCH "Schlafanalyse jeden Morgen auf dem Display öffnen (06:00–Ende)"
#define INTL_ANALYTICS_NIGHT_START_TIME "Beginn der Nacht für die Schlafanalyse (lokal, HH:MM)"
#define INTL_ANALYTICS_NIGHT_END_TIME "Ende der Nacht für die Schlafanalyse (lokal, HH:MM)"
#define INTL_ANALYTICS_NIGHT_END_HINT "Das Ende zählt bei Stundenwerten nicht mit (z. B. 07:00 nutzt Stunden bis 06:xx)."
#define INTL_ANALYTICS_MORNING_END_TIME "Morgenanzeige bis (lokal, HH:MM)"
#define INTL_ANALYTICS_MORNING_END_HINT "Danach kehrt das Display zum Hauptbildschirm zurück. Die Endzeit zählt nicht mit (10:00 heißt bis 09:59)."
#define INTL_ANALYTICS_SLEEP_ADD_URBAN "Urban-Daten zur Schlafanalyse hinzufügen (PM2.5 & Lärm)"
#define INTL_ANALYTICS_SLEEP_ADD_URBAN_STANDALONE_HINT "Deaktivieren Sie „Insight eigenständig“ im Abschnitt Firmware, um mit Urban zu koppeln und PM2.5/Lärm von außen in der Schlafanalyse zu nutzen."
#define INTL_PANEL_TITLE_FIRMWARE "System"
#define INTL_PANEL_TITLE_WIFI_CONFIG "WLAN im Konfigurationsmodus"
#define INTL_PANEL_TITLE_CSV "CSV"
#define INTL_PANEL_TITLE_CUSTOMAPI "Eigene API"
#define INTL_PANEL_TITLE_INFLUX "InfluxDB"
#define INTL_PANEL_TITLE_DATA_SHARING "Auf der Karte veröffentlichen"
#define INTL_DATA_SHARING_DISCLAIMER "Standardmäßig werden alle Sensordaten auf der öffentlichen Sensorkarte geteilt. Unten können Sie wählen, welche Datenarten geteilt werden. Nicht geteilte Daten werden weiterhin auf dem Display angezeigt und sind lokal verfügbar."
#define INTL_DATA_SHARING_ADDITIONAL "Weitere Sensoren (optional)"
#define INTL_PANEL_TITLE_DATA_ENCRYPT "Kartenwerte verschlüsseln"
#define INTL_DATA_ENCRYPT_DISCLAIMER "Optional. Ausgewählte Messwerte werden verschlüsselt für den Geräteeigentümer gesendet (CPS / AES-256-GCM). Melden Sie sich auf sensors.map als Eigentümer an, um sie zu sehen."
#define INTL_DATA_ENCRYPT_BACKUP_HINT "Keine Robonomics-Seed-Phrase für den Login auf sensors.map? Importieren Sie stattdessen ein Gerätebackup (nur bei eigenem Eigentümer):"
#define INTL_DATA_ENCRYPT_BACKUP_LINK "Backup & Wiederherstellung öffnen"
#define INTL_DATA_ENCRYPT_KEY_LABEL "Verschlüsselungsschlüssel des Geräts"
#define INTL_DATA_ENCRYPT_KEY_SHOW "Schlüssel anzeigen"
#define INTL_DATA_ENCRYPT_KEY_HIDE "Schlüssel ausblenden"
#define INTL_DATA_ENCRYPT_KEY_COPY "Schlüssel kopieren"
#define INTL_DATA_ENCRYPT_KEY_COPIED "Schlüssel kopiert"
#define INTL_DATA_ENCRYPT_KEY_HINT "Mit der Handykamera scannen (gleiches WLAN), um die Schlüssel-JSON herunterzuladen. Oder auf „Schlüssel anzeigen“ tippen und den Text kopieren."
#define INTL_DATA_ENCRYPT_QR_FAIL "QR-Code des Schlüssels konnte nicht erstellt werden."
const char INTL_SHARE_TEMPERATURE[] PROGMEM = "Temperatur";
const char INTL_SHARE_HUMIDITY[] PROGMEM = "Luftfeuchtigkeit";
const char INTL_SHARE_PRESSURE[] PROGMEM = "Luftdruck";
const char INTL_SHARE_CO2[] PROGMEM = "CO2";
const char INTL_SHARE_PM[] PROGMEM = "Feinstaub (PM2.5/PM10)";
const char INTL_SHARE_NOISE[] PROGMEM = "Lärmpegel";
const char INTL_SHARE_CO[] PROGMEM = "Kohlenmonoxid (CO)";
const char INTL_SHARE_RADIATION[] PROGMEM = "Strahlung";
const char INTL_SHARE_O3[] PROGMEM = "Ozon (O3)";
const char INTL_SHARE_NO2[] PROGMEM = "Stickstoffdioxid (NO2)";
const char INTL_SHARE_FAST_AQI[] PROGMEM = "FAST AQI";
const char INTL_SHARE_EPA_AQI[] PROGMEM = "EPA AQI";
const char INTL_ENCRYPT_TEMPERATURE[] PROGMEM = "Klima verschlüsseln (Temperatur & Luftfeuchtigkeit)";
const char INTL_ENCRYPT_HUMIDITY[] PROGMEM = "Luftfeuchtigkeit verschlüsseln";
const char INTL_ENCRYPT_PRESSURE[] PROGMEM = "Luftdruck verschlüsseln";
const char INTL_ENCRYPT_CO2[] PROGMEM = "CO2 verschlüsseln";
const char INTL_ENCRYPT_PM[] PROGMEM = "PM verschlüsseln";
const char INTL_ENCRYPT_NOISE[] PROGMEM = "Lärm verschlüsseln";
const char INTL_ENCRYPT_CO[] PROGMEM = "CO verschlüsseln";
const char INTL_ENCRYPT_RADIATION[] PROGMEM = "Strahlung verschlüsseln";
const char INTL_ENCRYPT_O3[] PROGMEM = "O3 verschlüsseln";
const char INTL_ENCRYPT_NO2[] PROGMEM = "NO2 verschlüsseln";
const char INTL_ENCRYPT_FAST_AQI[] PROGMEM = "FAST AQI verschlüsseln";
const char INTL_ENCRYPT_EPA_AQI[] PROGMEM = "EPA AQI verschlüsseln";

const char INTL_FS_WIFI_DESCRIPTION[] PROGMEM = "WLAN-Sensor im Konfigurationsmodus";
const char INTL_FS_WIFI_NAME[] PROGMEM = "Netzwerkname";
const char INTL_MORE_SETTINGS[] PROGMEM = "Gerät";
const char INTL_AUTO_UPDATE[] PROGMEM = "Firmware automatisch aktualisieren";
const char INTL_USE_BETA[] PROGMEM = "Beta-Firmware laden";
const char INTL_DISPLAY[] PROGMEM = "OLED SSD1306";
const char INTL_SH1106[] PROGMEM = "OLED SH1106";
const char INTL_FLIP_DISPLAY[] PROGMEM = "OLED-Display drehen";
const char INTL_LCD1602_27[] PROGMEM = "LCD 1602 (I2C: 0x27)";
const char INTL_LCD1602_3F[] PROGMEM = "LCD 1602 (I2C: 0x3F)";
const char INTL_LCD2004_27[] PROGMEM = "LCD 2004 (I2C: 0x27)";
const char INTL_LCD2004_3F[] PROGMEM = "LCD 2004 (I2C: 0x3F)";
const char INTL_DISPLAY_WIFI_INFO[] PROGMEM = "WLAN-Info anzeigen";
const char INTL_DISPLAY_DEVICE_INFO[] PROGMEM = "Geräteinfo anzeigen";
const char INTL_DEBUG_LEVEL[] PROGMEM = "Debug-Level";
const char INTL_MEASUREMENT_INTERVAL[] PROGMEM = "Sendeintervall (Sek.)";
const char INTL_LEDS_BRIGHTNESS[] PROGMEM = "LED-Helligkeit (%)";
const char INTL_LEDS_ON[] PROGMEM = "LEDs einschalten";
const char INTL_LEDS_OFF_HOUR[] PROGMEM = "Ausschalten um (lokale Stunde)";
const char INTL_LEDS_ON_HOUR[] PROGMEM = "Wieder einschalten um (lokale Stunde)";
const char INTL_LEDS_SCHEDULE_HINT[] PROGMEM = "Volle Stunden, lokale Gerätezeit (0 = Mitternacht, 23 = 23 Uhr). Die LEDs dimmen etwa 2 Stunden vor dem Ausschalten. Für beide dieselbe Stunde setzen, damit die LEDs nachts an bleiben.";
const char INTL_SDS_MEAS_INTERVAL[] PROGMEM = "SDS-Messintervall (Sek.)";
const char INTL_DATALOG_SENDING_INTERVAL[] PROGMEM = "Datalog-Sendeintervall (Sek.)";
const char INTL_DURATION_ROUTER_MODE[] PROGMEM = "Dauer Router-Modus";
const char INTL_MORE_APIS[] PROGMEM = "Weitere APIs";
const char INTL_SEND_TO_OWN_API[] PROGMEM = "Daten an eigene API senden";
const char INTL_SERVER[] PROGMEM = "Server";
const char INTL_PATH[] PROGMEM = "Pfad";
const char INTL_PORT[] PROGMEM = "Port";
const char INTL_USER[] PROGMEM = "Benutzer";
const char INTL_PASSWORD[] PROGMEM = "Passwort";
const char INTL_LOCAL_HOSTNAME[] PROGMEM = "Lokaler Hostname (ändern, wenn mehr als ein Altruist im selben Netzwerk ist)";
const char INTL_MEASUREMENT[] PROGMEM = "Messung";
const char INTL_SEND_TO[] PROGMEM = "Senden an {v}";
const char INTL_READ_FROM[] PROGMEM = "Lesen von {v}";
const char INTL_SENSOR_IS_REBOOTING[] PROGMEM = "Sensor startet neu.";
const char INTL_RESTART_DEVICE[] PROGMEM = "Gerät neu starten";
const char INTL_DELETE_CONFIG[] PROGMEM = "gespeicherte Konfiguration löschen";
const char INTL_RESTART_SENSOR[] PROGMEM = "Sensor neu starten";
#define INTL_HOME "Start"
#define INTL_BACK_TO_HOME "Zurück zur Startseite"
const char INTL_CURRENT_DATA[] PROGMEM = "Aktuelle Daten";
const char INTL_DATA_BUSY[] PROGMEM = "Sensordaten werden aktualisiert — gleich neu laden.";
// Graphs screen
#define INTL_DISP_GRAPHS_HEADER_PREFIX "Aktuell"
#define INTL_DISP_GRAPHS_HINT_LINE1 "lang drücken ->"
#define INTL_DISP_GRAPHS_HINT_LINE2 "vor/zurück"
#define INTL_DISP_GRAPHS_HINT_LINE3 "Bildschirm"
const char INTL_DEVICE_STATUS[] PROGMEM = "Gerätestatus";
#define INTL_ACTIVE_SENSORS_MAP "Karte aktiver Sensoren (externer Link)"
#define INTL_CONFIGURATION_DELETE "Konfiguration löschen"
#define INTL_CONFIGURATION_REALLY_DELETE "Möchten Sie die Konfiguration wirklich löschen?"
#define INTL_CONFIGURATION_DELETE_WARNING "Das lässt sich nicht rückgängig machen. Das Gerät startet nach dem Löschen der ausgewählten Einstellungen neu."
#define INTL_CONFIGURATION_DELETE_CONFIRM "Ja, endgültig löschen"
#define INTL_DELETE "Löschen"
#define INTL_CANCEL "Abbrechen"
#define INTL_REALLY_RESTART_SENSOR "Möchten Sie den Sensor wirklich neu starten?"
#define INTL_RESTART "Neu starten"
const char INTL_SAVE_AND_RESTART[] PROGMEM = "Konfiguration speichern und neu starten";
#define INTL_FIRMWARE "Firmware:"
#define INTL_IP_ADDRESS "IP-Adresse"
const char INTL_SD_CONNECTED[] PROGMEM = "SD-Karte verbunden";
const char INTL_FREE_RAM[] PROGMEM = "Freier Speicher (RAM)";
const char INTL_LAST_OTA[] PROGMEM = "Letzte OTA-Prüfung";
#define INTL_OTA_UPDATE "Firmware-Update"
const char INTL_OTA_CHECK_UPDATE[] PROGMEM = "Nach Update suchen";
const char INTL_OTA_CURRENT_VERSION[] PROGMEM = "Aktuelle Version";
const char INTL_OTA_LATEST_VERSION[] PROGMEM = "Neueste verfügbare";
const char INTL_OTA_UP_TO_DATE[] PROGMEM = "Firmware ist aktuell";
const char INTL_OTA_UPDATE_AVAILABLE[] PROGMEM = "Eine neuere Firmware ist verfügbar";
const char INTL_OTA_INSTALL[] PROGMEM = "Aktualisieren";
const char INTL_OTA_INSTALL_REQUESTED[] PROGMEM = "Update gestartet. Lassen Sie das WLAN verbunden, bis das Gerät neu startet.";
const char INTL_OTA_UPDATING[] PROGMEM = "Firmware wird aktualisiert";
const char INTL_OTA_CHECK_FAILED[] PROGMEM = "Update-Prüfung fehlgeschlagen. Später erneut versuchen.";
const char INTL_OTA_CHECK_REQUESTED[] PROGMEM = "Update-Prüfung angefordert. Das Gerät lädt und installiert die neue Firmware, falls verfügbar.";
const char INTL_OTA_NO_WIFI[] PROGMEM = "WLAN ist nicht verbunden. Update-Prüfung nicht möglich.";
const char INTL_OTA_SWITCH_LANG[] PROGMEM = "Sprache wechseln";
const char INTL_OTA_CURRENT_LANG[] PROGMEM = "Aktuelle Sprache";
const char INTL_OTA_SWITCH_LANG_NOTE[] PROGMEM = "Das Gerät lädt und installiert die Firmware in der gewählten Sprache";
const char INTL_OTA_LANG_SAME[] PROGMEM = "Diese Sprache ist bereits aktiv.";
const char INTL_OTA_LANG_REQUESTED[] PROGMEM = "Sprachwechsel angefordert. Bitte warten. Das Gerät lädt und installiert die Firmware in der gewählten Sprache.";
const char INTL_UPTIME[] PROGMEM = "Laufzeit";
const char INTL_RESET_REASON[] PROGMEM = "Grund für Neustart";
const char INTL_OTA_RETURN[] PROGMEM = "OTA-Rückgabe";
const char INTL_COUNT_SUCCESS_SENDS[] PROGMEM = "erfolgreiche Sendungen";
const char INTL_LAST_SEND_TIME[] PROGMEM = "letzte Sendung";
#define INTL_CHIP_TYPE "Chiptyp"
#define INTL_ROBONOMICS_ADDR "Robonomics-Adresse"
const char INTL_DEBUG_SETTING_TO[] PROGMEM = "Debug-Level setzen auf";
#define INTL_NONE "aus"
#define INTL_ERROR "nur Fehler"
#define INTL_WARNING "Warnungen"
#define INTL_MIN_INFO "min. Info"
#define INTL_MED_INFO "mittl. Info"
#define INTL_MAX_INFO "max. Info"
#define INTL_CONFIG_DELETED "Konfiguration wurde gelöscht"
#define INTL_CONFIG_CAN_NOT_BE_DELETED "Konfiguration kann nicht gelöscht werden"
#define INTL_CONFIG_NOT_FOUND "Konfiguration nicht gefunden"
const char INTL_TIME_TO_FIRST_MEASUREMENT[] PROGMEM = "Noch {v} Sekunden bis zur ersten Messung.";
const char INTL_TIME_SINCE_LAST_MEASUREMENT[] PROGMEM = " Sekunden seit der letzten Messung.";
const char INTL_PARTICLES_PER_LITER[] PROGMEM = "Partikel/Liter";
const char INTL_PARTICULATE_MATTER[] PROGMEM = "Feinstaub";
const char INTL_TEMPERATURE[] PROGMEM = "Temperatur";
const char INTL_NOISE[] PROGMEM = "Lärm";
const char INTL_NOISE_MAX[] PROGMEM = "max. Lärm";
const char INTL_NOISE_MEAN[] PROGMEM = "mittl. Lärm";
const char INTL_HUMIDITY[] PROGMEM = "Luftfeuchtigkeit";
const char INTL_PRESSURE[] PROGMEM = "Luftdruck";
const char INTL_RADIATION[] PROGMEM = "Strahlung";
const char INTL_CO2[] PROGMEM = "CO2";
const char INTL_LEQ_A[] PROGMEM = "LAeq";
const char INTL_LA_MIN[] PROGMEM = "LA min";
const char INTL_LA_MAX[] PROGMEM = "LA max";
const char INTL_LATITUDE[] PROGMEM = "Breitengrad";
const char INTL_LONGITUDE[] PROGMEM = "Längengrad";
const char INTL_ALTITUDE[] PROGMEM = "Höhe";
const char INTL_TIME_LOCAL[] PROGMEM = "Zeit";
const char INTL_SIGNAL_STRENGTH[] PROGMEM = "Signalstärke";
const char INTL_SIGNAL_QUALITY[] PROGMEM = "Signalqualität";
#define INTL_NUMBER_OF_MEASUREMENTS "Anzahl der Messungen"
#define INTL_TIME_SENDING_MS "Dauer des Uploads"
#define INTL_SENSOR "Sensor"
#define INTL_PARAMETER "Parameter"
#define INTL_VALUE "Wert"

#define INTL_DATA_SECTION_SDS "SDS"
#define INTL_DATA_SECTION_SCD "SCD4x"
#define INTL_DATA_SECTION_BME "BME"
#define INTL_DATA_SECTION_URBAN "Urban-Daten"
#define INTL_DATA_SECTION_OVERVIEW "Übersicht"
#define INTL_DATA_SECTION_DEVICE "Gerät"
#define INTL_DATA_SECTION_RUNTIME "Laufzeit"
#define INTL_DATA_SECTION_NETWORK "Netzwerk"
#define INTL_DATA_SECTION_EXPORT "Datenexport"
#define INTL_DATA_SECTION_TECHNICAL "Technische Details"
#define INTL_COPY_ALL "Alles kopieren"
#define INTL_COPIED "Kopiert"
#define INTL_TOPBAR_ONLINE "Online"
#define INTL_TOPBAR_OFFLINE "Offline"
#define INTL_TOPBAR_SEND_ISSUE "Sendeproblem"
#define INTL_TOPBAR_SEND_OK "OK"
#define INTL_TOPBAR_SEND_ERR "Nicht gesendet"
#define INTL_TOPBAR_LAST_SEND "Letzte Sendung"
#define INTL_TOPBAR_NO_SENDS "Noch keine Sendungen"
#define INTL_TOPBAR_MAP "Karte"
#define INTL_TOPBAR_DATALOG "Datalog"
#define INTL_TOPBAR_JUST_NOW "gerade eben"
#define INTL_TOPBAR_WIFI "WLAN"
#define INTL_TOPBAR_HOST "Hostname"
#define INTL_TOPBAR_DEVICE "Gerät"
#define INTL_TOPBAR_SEND "Senden"
#define INTL_TOPBAR_STANDALONE "Eigenständig"
#define INTL_TOPBAR_PAIRED "Urban gekoppelt"
#define INTL_TOPBAR_UPDATE "Update"
#define INTL_TOPBAR_FW_CURRENT "Aktuell"
#define INTL_TOPBAR_ROBONOMICS "Robonomics-Adresse"
#define INTL_TOPBAR_Q_ACCESS "Im Browser öffnen"
#define INTL_TOPBAR_TAP_COPY "Zum Kopieren tippen"
#define INTL_TOPBAR_MORE "Geräteinfo"
#define INTL_VALUE_YES "Ja"
#define INTL_VALUE_NO "Nein"
#define INTL_READINGS_SECTION_NETWORK_INTRO "WLAN-Signalstärke am Sensor."
#define INTL_STATUS_SECTION_OVERVIEW_INTRO "Läuft das Gerät gerade normal?"
#define INTL_STATUS_SECTION_DEVICE_INTRO "Firmware-Version, Arbeitsspeicher und Speicherplatz."
#define INTL_STATUS_SECTION_TECH_INTRO "Build-Details — nützlich für Anfragen an den Support."
#define INTL_STATUS_SECTION_EXPORT_INTRO "Ob Ihre Daten bei jedem Dienst ankommen."
#define INTL_API_SENDS_SHORT "Sendungen"
#define INTL_API_LAST_SHORT "Zuletzt"

#define INTL_REGION "Region"
#define INTL_REGION_GLOBAL "Global"
#define INTL_REGION_RU "Russland"
#define INTL_REGION_HINT "Wird für die Verbindung zu sensors.social genutzt. Automatisch aus Koordinaten / OTA, sofern Sie es hier nicht ändern."
#define INTL_REGION_EU "Europa"
#define INTL_REGION_AS "Asien"
#define INTL_REGION_AF "Afrika"
#define INTL_REGION_AU "Australien"
#define INTL_REGION_NA "Nordamerika"
#define INTL_REGION_SA "Südamerika"

/* Insight display (e-paper) strings */
#define INTL_DISP_PRODUCT_INSIGHT "Altruist Insight"
#define INTL_DISP_WIFI_CLEARED "WLAN-Daten gelöscht"
#define INTL_DISP_RESTARTING "Gerät startet neu..."
#define INTL_DISP_WIFI_SETUP "WLAN-Einrichtung"
#define INTL_DISP_CONNECT_TO "Verbinden mit"
#define INTL_DISP_PASSWORD_PREFIX "Passwort: "
#define INTL_DISP_TITLE_INSIGHT "ALTRUIST INSIGHT"
#define INTL_DISP_CONNECTING_WIFI "Verbinde mit WLAN"
#define INTL_DISP_PLEASE_WAIT "Bitte warten..."
#define INTL_DISP_SD_NOT_FOUND "SD-Karte nicht gefunden"
#define INTL_DISP_INSERT_SD "Bitte SD-Karte einlegen"
#define INTL_DISP_FAT32_FORMATTED "(FAT32 formatiert)"
#define INTL_DISP_NO_DATA_FILES "Keine Datendateien"
#define INTL_DISP_DEVICE_WILL_CREATE "Das Gerät erstellt"
#define INTL_DISP_FILES_AUTOMATICALLY "Dateien automatisch"
#define INTL_DISP_AFTER_COLLECTING "nach dem Sammeln von Daten"
#define INTL_DISP_SD_NOT_AVAILABLE "SD-Karte nicht verfügbar"
#define INTL_DISP_GRAPHS_REQUIRE_SD "Diagramme brauchen SD-Karte"
#define INTL_DISP_ENABLE_SD "Bitte SD-Karte aktivieren"
#define INTL_DISP_INSIGHT_HEADER "Insight"
#define INTL_DISP_INSIGHT_ONLY "Nur Insight"
#define INTL_DISP_URBAN_HEADER "Urban"
#define INTL_DISP_URBAN_ONLY "Nur Urban"
#define INTL_DISP_GOING_TO_SLEEP "Ruhemodus..."
#define INTL_DISP_OTA_UPDATING "Firmware-Update"
#define INTL_DISP_OTA_DO_NOT_DISCONNECT "Strom nicht trennen"
#define INTL_DISP_OTA_FAILED "Update fehlgeschlagen"
#define INTL_DISP_OTA_WILL_RETRY "Neuer Versuch später"
#define INTL_DISP_OTA_SUCCESS "Firmware aktualisiert"
#define INTL_DISP_OTA_RESTARTING "Neustart..."
#define INTL_DISP_WAITING_URBAN_ID "Warte auf Urban-ID..."
#define INTL_DISP_URBAN_IP "Urban-IP:"
#define INTL_DISP_INSIGHT_IP "Insight-IP:"
#define INTL_DISP_SD_CARD "SD-Karte:"
#define INTL_DISP_WIFI_STATUS "WLAN-Status:"
#define INTL_DISP_WIFI_NAME "WLAN-Name:"
#define INTL_DISP_UNIQUE_ADDR "Eindeut. Adr.:"
#define INTL_DISP_DEVICE_INFO "Geräteinfo"
#define INTL_DISP_SCAN_FOR_MORE "Scannen für mehr"
#define INTL_DISP_NO_DATA "--"
#define INTL_DISP_TEMPERATURE "Temperatur"
#define INTL_DISP_HUMIDITY "Feuchte"
#define INTL_DISP_PRESSURE "Luftdruck"
#define INTL_DISP_AIR "Luft"
#define INTL_DISP_AIR_QUALITY "Luftqualität"
#define INTL_DISP_NOISE "Lärm"
#define INTL_DISP_MAIN_URBAN "URBAN"
#define INTL_DISP_MAIN_INSIGHT "INSIGHT"
#define INTL_DISP_SENSORS_MAP "SENSORKARTE"
#define INTL_DISP_EXPLORE_ADVANTAGES "Alle Vorteile entdecken"
#define INTL_DISP_EXPLORE_ENVIRONMENT "Entdecken Sie Ihre Umgebung"
#define INTL_DISP_EXPLORE_YOUR "Entdecken Sie Ihre"
#define INTL_DISP_ENVIRONMENT_CAPS "UMGEBUNG"
#define INTL_DISP_MAP_ENV_BETTER "Kennen Sie Ihre Umgebung besser."
#define INTL_DISP_MAP_REVIEW_INSIGHTS "Verläufe über die Zeit ansehen."
#define INTL_DISP_MAP_COMPARE_CONDITIONS "Mit anderen in der Nähe vergleichen."
#define INTL_DISP_SCAN_TO_OPEN "Scannen, um online zu öffnen"
#define INTL_DISP_POWERED_BY "Powered by Robonomics"
#define INTL_DISP_POWERED "Powered"
#define INTL_DISP_BY_ROBONOMICS "by Robonomics"
#define INTL_DISP_NOT_CONNECTED "Nicht verbunden"
#define INTL_DISP_CONNECTED "Verbunden"
#define INTL_DISP_DISCONNECTED "Getrennt"
#define INTL_DISP_NOT_SET "Nicht gesetzt"
#define INTL_DISP_NOISE_MAX "Lärm max."
#define INTL_DISP_NOISE_AVG "Lärm mittl."
#define INTL_DISP_NO_DATA_AVAILABLE "Keine Daten verfügbar"
#define INTL_DISP_NOT_ENOUGH_DATA_YET "Noch nicht genug Daten"
#define INTL_DISP_COLLECTING_DATA "Sammle Daten..."
#define INTL_DISP_LOADING "Lade..."
#define INTL_DISP_ANALYTICS_C_LEGEND "C=Konservativ"
#define INTL_DISP_ANALYTICS_B_LEGEND "B=Biohacking"
#define INTL_DISP_ANALYTICS_GRADE "Note"
#define INTL_DISP_ANALYTICS_COL_METRIC "Messwert"
#define INTL_DISP_ANALYTICS_COL_MAX "Max"
#define INTL_DISP_ANALYTICS_COL_MIN "Min"
#define INTL_DISP_ANALYTICS_COL_CONSERV "Konserv"
#define INTL_DISP_ANALYTICS_COL_BIOHACK "Biohack"
#define INTL_DISP_ANALYTICS_ROW_CO2 "CO2 ppm"
#define INTL_DISP_ANALYTICS_ROW_TEMP "Temperatur C"
#define INTL_DISP_ANALYTICS_ROW_HUM "Feuchte %"
#define INTL_DISP_ANALYTICS_ROW_PM25 "PM2.5 ug/m3"
#define INTL_DISP_ANALYTICS_ROW_NOISE "Lärm dB"
#define INTL_DISP_ANALYTICS_AT "um"
#define INTL_DISP_ANALYTICS_HOUR_SUFFIX "h"
#define INTL_DISP_INFO_LABEL "Info:"
#define INTL_DISP_LEVEL_HIGH "hoch"
#define INTL_DISP_LEVEL_LOW "niedrig"
#define INTL_DISP_IS_TOO "ist zu"
#define INTL_DISP_CHECK_MAP_FULL_DATA "Auf unserer Sensorkarte finden Sie alle Daten und Auswertungen."
/** Legacy short shop line (unused in two-row footer; kept for intl parity). */
#define INTL_STANDALONE_SHOP_PROMPT "Mehr Messwerte für Ihr Zuhause"
/** Insight standalone: second footer row beside shop QR (Font8, wraps). */
#define INTL_STANDALONE_INSIGHT_FOOTER_PROMPT \
    "Ergänzen Sie Ihr Insight um Lärm, Feinstaub und Messwerte der Außenluft."
#define INTL_DISP_DEW_POINT_U_PREFIX "Taupunkt (U): "
#define INTL_DISP_DEW_POINT_IS "Taupunkt ist "
#define INTL_DISP_TEMP_SHORT "Temp"
#define INTL_DISP_PRESS_SHORT "Druck"
#define INTL_DISP_NOISE_AVGMAX_SUFFIX "(mittl. | max)"

