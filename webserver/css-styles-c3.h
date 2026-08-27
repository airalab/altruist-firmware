#ifndef _CSS_STYLES_C3_H
#define _CSS_STYLES_C3_H

const char WEB_PAGE_STATIC_CSS[] PROGMEM = R"rawliteral(

body,ul{margin:0}.canvas,body{display:flex}hr,table{margin-bottom:20px}.canvas,.canvas-info,td,th{text-align:left}.b,a{font-weight:500;text-decoration:none}.canvas-info h3,.guest-subtitle,.guest__connect-text,.guest__connect-title,.panel-subtitle,th{text-transform:uppercase}.copy-btn,.radio-input+label,.s_red,.submit-btn,.tab{cursor:pointer}.content,.loader::before,body{box-sizing:border-box}body{font-family:Roboto,system-ui;background-color:#f4f4f4;color:#333;padding:0;flex-direction:column;min-height:100vh}a{color:#007bff}ul{padding:0 20px}li{font-size:16px;font-weight:500}li:not(:last-child){margin-bottom:10px}.canvas{background:#007bff;color:#fff;padding:28px 40px;align-items:center;gap:24px 48px;flex-wrap:wrap;border-bottom:1px solid #0062cc;box-shadow:0 4px 20px rgba(0,123,255,.25);box-sizing:border-box;width:100%}.canvas>a.b{display:flex;flex-shrink:0;background:none!important;padding:0;margin:0;border-radius:0}.canvas>a.b:hover{background:none!important;opacity:.9}.content,table{background:#fff;border-radius:10px;box-shadow:0 4px 8px rgba(0,0,0,.1);width:100%}.canvas img,.canvas-logo{width:148px;height:auto;max-width:148px;object-fit:contain;border-radius:12px;display:block}.canvas-info{flex:1;min-width:240px;max-width:720px;line-height:1.55;width:auto}.canvas-info h3{margin:0 0 10px;font-size:26px;font-weight:700;letter-spacing:.01em;text-transform:none}.canvas-info small{display:block;font-size:15px;color:#ccc;word-wrap:break-word;max-width:100%}.canvas-info small span{font-weight:600;color:#888;font-size:11px;letter-spacing:.07em;text-transform:uppercase}.canvas-info small br{margin-bottom:5px}.canvas-info .canvas-address{display:inline-block;font-size:17px;font-weight:500;color:#fff;line-height:1.45;word-break:break-all;letter-spacing:.01em}.canvas-info .canvas-address[title]{cursor:pointer;border-bottom:1px dashed rgba(255,255,255,.45)}.canvas-info .canvas-address[title]:hover{opacity:.9}.content{max-width:500px;margin:40px auto;padding:20px;overflow-x:auto}.content-debug{max-width:800px}.content-config{max-width:1400px}.configuration .content-config{max-width:500px}.content-subtitle{padding-left:12px;text-align:left;color:#000}.b,td,th{padding:12px}table{border-collapse:collapse;margin-top:20px;overflow:hidden;table-layout:fixed}.b,th{background:#007bff;color:#fff}td,th{border-bottom:1px solid #ddd;word-wrap:break-word}th{width:100%;hyphens:auto}.content-table th{border-right:1px solid #ececec}.delete-table{min-width:unset;box-shadow:unset}.content-debug .content-table{min-width:490px;box-shadow:unset}@media screen and (max-width:720px){.content-debug .content-table{font-size:12px}}.r{text-align:right}.b,.footer{text-align:center}.b{display:block;border-radius:8px;margin-bottom:10px;transition:background .2s ease,box-shadow .2s ease;box-shadow:0 2px 8px rgba(0,0,0,.12)}.footer-polkadot,.radio-input+label{display:inline-flex;align-items:center}.b:hover{background:#0062cc;box-shadow:0 4px 14px rgba(0,123,255,.28)}.danger{background:#d32f2f}.radio-input[value=all]:checked+label,.submit-btn{background-color:#007bff;color:#fff}.danger:hover{background:#b71c1c}.delete-table .b{margin-bottom:0}.footer{padding:16px;background:#007bff;color:#fff;margin-top:auto;font-size:14px}.footer a{transition:opacity .2s ease-in-out}.footer a:hover{opacity:.7}.footer-polkadot{margin-top:5px;font-size:18px;display:inline-flex;align-items:center;justify-content:center;font-weight:300;font-style:italic}input[type=email],input[type=number],input[type=password],input[type=text],select,textarea{width:100%;max-width:100%;padding:8px;margin:8px 0;border:2px solid #ccc;border-radius:5px;font-size:14px;background-color:#fff;box-sizing:border-box;transition:border-color .3s ease-in-out}select{padding:8px 32px 8px 8px;-webkit-appearance:none;appearance:none;background-image:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='8' viewBox='0 0 12 8'%3E%3Cpath fill='%23666' d='M1.1 1.2l4.9 4.9 4.9-4.9 1.1 1.1-6 6-6-6z'/%3E%3C/svg%3E");background-repeat:no-repeat;background-position:right 12px center;background-size:12px 8px}.s_red,.submit-btn,label{font-size:16px}input[type=email]:focus,input[type=number]:focus,input[type=password]:focus,input[type=text]:focus,select:focus,textarea:focus{border-color:#007bff;outline:0}input[type=checkbox],input[type=radio]{margin-right:8px;vertical-align:middle}label{margin-bottom:4px;display:inline-block}.form-group{text-align:left;margin-bottom:6px;overflow:hidden}.form-group input[type=radio]:checked+label{font-weight:700;color:#000}.radio-input{display:none}.radio-input+label{font-size:16px;padding:5px 10px;border-radius:20px;transition:.3s}.radio-input+label::before{content:"";display:inline-block;width:20px;height:20px;border-radius:50%;border:2px solid gray;background-color:#fff;margin-right:10px;transition:.3s}.radio-input[value=wifi]:checked+label{background-color:#fc0;color:#000}.radio-input[value=all]:checked+label::before{background-color:#007bff;border-color:#fff}.radio-input[value=wifi]:checked+label::before{background-color:#ffcf11;border-color:#fff}.submit-btn{display:block;margin:0 auto;padding:14px 22px;border-radius:8px;border:none;font-weight:600;box-shadow:0 2px 10px rgba(0,0,0,.14);transition:background-color .2s ease,box-shadow .2s ease}.submit-btn--config{margin-top:20px;font-weight:600}.submit-btn:hover{background-color:#0062cc;box-shadow:0 4px 16px rgba(0,123,255,.3)}.home-btn{display:inline-block;background-color:#007bff}.s_red{background:#d32f2f;color:#fff;padding:12px 20px;border:none;border-radius:5px;transition:background .3s ease-in-out;width:100%}.s_red:hover{background:#c0392b}.tabs{display:flex;align-items:center;gap:20px;justify-content:center;margin-bottom:40px}.tab{padding:10px;background:#f4f4f4;border-radius:5px;font-weight:500;font-size:18px;transition:background .3s ease-in-out,color .3s ease-in-out}.tab:hover{background:#ddd}.panel{display:none;margin-bottom:20px}.panel.active{display:flex;gap:30px;justify-content:space-around}.panel-subtitle{text-align:center;padding:5px 12px;background:#007bff;color:#fff}.panel.active#panel3{display:grid;grid-template-columns:repeat(2,1fr)}.text-small{display:block;margin-bottom:6px;text-align:left;font-size:14px}.guest-subtitle,.map-text,.panels{text-align:center}#slog{text-align:initial}.map-text{display:block;margin-top:20px;margin-bottom:30px;font-size:10px;color:#6e6e6e;z-index:10}.panel-container{width:100%}#panel1 .panel-container:not(:last-of-type),#panel2 .panel-container:not(:last-of-type){padding-right:20px;border-right:1px solid #bababa}.panels{min-height:200px;overflow:auto;padding:20px;border:2px solid #007bff;margin-bottom:1em}.guest-page{max-width:640px;margin:0 auto;width:100%}
.content-guest{max-width:640px;width:calc(100% - 24px);margin:16px auto 24px;padding:0;background:transparent!important;box-shadow:none!important;border-radius:0}
.guest-page--backup{margin-top:16px}
.guest-card--connect{text-align:center}
.guest__connect-status{display:flex;flex-direction:column;align-items:center;justify-content:center;gap:14px;padding:40px 16px;text-align:center;min-height:200px}
.guest__connect-hint{margin:0;max-width:22rem;line-height:1.45}.guest-card{background:#fff;border:1px solid #e0e0e0;border-radius:16px;box-shadow:0 2px 12px rgba(0,0,0,.06);padding:20px 22px 24px;text-align:left}.guest-form .config-section{margin-bottom:16px}.guest-form .config-section:last-of-type{margin-bottom:0}.guest-wifilist{margin-bottom:12px;font-size:14px;color:#555}.guest-sensor-list{margin:0;padding:0 0 0 18px;color:#444}.guest-sensor-list li{margin-bottom:6px;font-size:14px;line-height:1.4}.guest-hint{background:#fff8e6;border:1px solid #f0c040;border-radius:10px;padding:12px 14px;margin-bottom:14px}.guest-form-footer{margin-top:20px;padding-top:16px;border-top:1px solid #eee;text-align:center}.guest-form-footer .submit-btn{margin:0 auto}.guest__setup-header{margin:0 0 18px;text-align:left}.guest__step-label{display:block;margin:0 0 8px;font-size:12px;font-weight:600;letter-spacing:.06em;text-transform:uppercase;color:#888}.guest__step-title{margin:0;font-size:22px;font-weight:700;color:#111;line-height:1.3;text-transform:none;background:none;padding:0}.guest__connect-status{display:flex;flex-direction:column;align-items:center;justify-content:center;gap:14px;padding:40px 16px;text-align:center;min-height:200px}.guest__connect-subtitle{margin:0;font-size:18px;font-weight:700;color:#111;background:none;padding:0;text-transform:none}.guest__connect-subtitle.error{color:#b71c1c}.guest__connect-title{margin:0 0 12px;font-size:20px;font-weight:700;color:#111;background:none;padding:0}.guest__connected{margin:0 0 16px}.guest__reboot{margin:0 0 12px;font-size:14px;color:#555;font-style:normal;letter-spacing:0;line-height:1.45}.guest__reboot--ip{font-size:15px;color:#333}.guest-ip{font-weight:700;cursor:pointer}.guest-option{display:flex;align-items:flex-start;gap:12px;padding:14px 16px;border:1px solid #e0e0e0;border-radius:10px;background:#fafafa;cursor:pointer;font-size:15px;line-height:1.35;margin-bottom:18px}.guest-option input{margin-top:3px;flex-shrink:0}.guest__setup-finish-btn{width:100%;margin-top:4px}.configuration .content-config .content-subtitle{margin-bottom:16px}.copy-btn{min-height:30px;min-width:30px;border:1px solid #ccc;border-radius:4px}.loader{width:28px;height:28px; margin:0 auto;border-radius:50%;animation:1s linear infinite rotate}.loader::before{content:"";position:absolute;inset:0px;border-radius:50%;border:5px solid #007bff;animation:2s linear infinite prixClipFix}.guest__connect-status--initial.no-loader .loader,.hide{display:none}.error{color:#fff;background-color:#ab0202}@keyframes rotate{100%{transform:rotate(360deg)}}@keyframes prixClipFix{0%{clip-path:polygon(50% 50%,0 0,0 0,0 0,0 0,0 0)}25%{clip-path:polygon(50% 50%,0 0,100% 0,100% 0,100% 0,100% 0)}50%{clip-path:polygon(50% 50%,0 0,100% 0,100% 100%,100% 100%,100% 100%)}75%{clip-path:polygon(50% 50%,0 0,100% 0,100% 100%,0 100%,0 100%)}100%{clip-path:polygon(50% 50%,0 0,100% 0,100% 100%,0 100%,0 0)}}@media screen and (max-width:1000px){.panel.active{flex-direction:column}.panel.active#panel3{display:grid;grid-template-columns:1fr}.panel-subtitle{font-size:18px;line-height:1.2}}@media screen and (max-width:550px){}@media screen and (max-width:395px){th{font-size:12px;hyphens:auto;word-break:unset;word-wrap:unset;overflow-wrap:break-word}.content-table th{padding:10px 5px;text-align:center}}@media screen and (max-width:370px){.tab{font-size:12px;gap:10px}}
.home-nav{max-width:640px;margin:0 auto}.nav-section{margin-bottom:28px}.nav-section-title{font-size:13px;font-weight:600;text-transform:uppercase;letter-spacing:.06em;color:#666;margin:0 0 12px;padding-bottom:6px;border-bottom:2px solid #e8e8e8}.nav-section--danger .nav-section-title{color:#b71c1c;border-color:#ffcdd2}.nav-grid{display:grid;grid-template-columns:1fr;gap:8px}.b.b-secondary{background:#444}.b.b-secondary:hover{background:#222}.configuration .content.content-config{max-width:1440px;width:calc(100% - 32px);margin:16px auto 24px;padding:0;background:transparent;box-shadow:none;border-radius:0;text-align:left}.configuration .content-subtitle,.content-data .content-subtitle{margin:0 0 14px}.config-form{width:100%}.config-layout{display:flex;background:#fff;border-radius:16px;border:1px solid #e2e8f4;box-shadow:0 8px 32px rgba(26,43,109,.08);overflow:visible;min-height:480px}.config-sidebar{flex:0 0 210px;background:linear-gradient(180deg,#fafafa 0%,#f2f2f2 100%);border-right:1px solid #e5e5e5;padding:20px 12px;position:sticky;top:0;align-self:flex-start;max-height:100vh;overflow-y:auto;overscroll-behavior:contain;box-sizing:border-box}.config-nav.tabs{display:flex;flex-direction:column;gap:6px;margin:0;padding:0;border:none;align-items:stretch;justify-content:flex-start}.config-form .tab{padding:12px 16px;border-radius:10px;border:1px solid transparent;background:transparent;font-size:14px;font-weight:600;color:#4a5568;text-align:left;box-shadow:none;transition:background .15s,color .15s,border-color .15s,box-shadow .15s}.config-form .tab:hover{background:rgba(0,0,0,.05);color:#000;border-color:transparent}.config-form .tab.active{background:#fff;color:#000;border-color:#ddd;border-left:3px solid #007bff;padding-left:13px;box-shadow:0 2px 8px rgba(0,0,0,.08)}.config-main{flex:1;min-width:0;display:flex;flex-direction:column;padding:20px 24px;background:#fff}.config-panels{flex:1;min-width:0;width:100%;max-width:none;margin:0}.config-form .panel{display:none}.config-form .panel.active{display:grid;grid-template-columns:repeat(auto-fill,minmax(280px,1fr));gap:16px;align-content:start}.config-section{background:#fafbfd;border:1px solid #e8ecf4;border-radius:12px;margin:0;overflow:hidden;height:fit-content}.config-section__title{margin:0;padding:12px 16px;font-size:12px;font-weight:700;color:#000;background:transparent;border-bottom:1px solid #e8ecf4;text-transform:uppercase;letter-spacing:.06em;line-height:1.35}.config-section__body{padding:14px 16px 16px;display:flex;flex-direction:column;gap:12px}.config-section__body>.form-group{margin-bottom:0}.config-section__body>br{display:none}.config-cluster{display:flex;flex-direction:column;gap:12px}.config-cluster__title{margin:0;font-size:11px;font-weight:700;letter-spacing:.06em;text-transform:uppercase;color:#888}.config-cluster+.config-cluster{margin-top:2px;padding-top:14px;border-top:1px solid #e8e8e8}.config-cluster>.form-group{margin-bottom:0}.config-cluster>br{display:none}.config-section__body--compact{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:0 14px}.guest-page .config-section__body--compact{grid-template-columns:1fr}.config-section__body--compact>.form-group:only-child,.config-section__body--compact>p.form-hint{grid-column:1/-1}.config-row{display:grid;grid-template-columns:1fr;gap:16px}.config-row>.config-section{min-width:0}@media screen and (min-width:980px){.config-row{grid-template-columns:1fr 1fr;align-items:start}}.config-tag{display:inline-block;font-size:10px;font-weight:600;text-transform:uppercase;padding:2px 8px;border-radius:999px;background:#fff3e0;color:#e65100;letter-spacing:.04em;vertical-align:middle;margin-left:6px}.config-tag--muted{background:#eceff1;color:#546e7a}.checkbox-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(170px,1fr));gap:8px 18px;margin:8px 0 2px}.checkbox-grid .form-group{margin-bottom:2px}.checkbox-grid .form-group br{display:none}.checkbox-grid .form-group label{display:flex;align-items:flex-start;gap:10px;font-size:14px;line-height:1.35}.checkbox-grid .form-group input[type=checkbox]{margin:2px 0 0;flex-shrink:0}.form-hint{font-size:12px;color:#666;margin:4px 0 10px;line-height:1.45}.link-inline{font-size:13px}.config-form-footer{position:sticky;bottom:0;background:linear-gradient(transparent,#fff 32%);padding:16px 0 4px;margin-top:16px;border-top:1px solid #eef1f7;text-align:right;z-index:5}.config-form-footer .submit-btn{margin:0;display:inline-block;min-width:200px;padding:12px 28px;border-radius:10px}.config-form .map-container{min-width:0;max-width:100%;height:350px;overflow:hidden;position:relative;contain:layout paint}.map-container{position:relative;width:100%;min-width:0;max-width:100%;height:350px;min-height:220px;margin:12px 0;overflow:hidden;border-radius:8px;border:1px solid #e8e8e8;background:#f5f7fa;box-sizing:border-box;contain:layout paint}.configuration #map,.config-form #map,.hub-config-form #map,.map-container #map{position:absolute;inset:0;width:100%!important;height:100%!important;min-height:0;overflow:hidden;z-index:0}.config-form #map .leaflet-container,.configuration #map .leaflet-container,.map-container .leaflet-container,.hub-config-form #map .leaflet-container{position:absolute;inset:0;width:100%!important;height:100%!important}.config-section--gps{overflow:hidden}.config-section--gps .config-section__body{overflow:hidden}.config-form .map-text{font-size:12px;margin-top:12px;margin-bottom:0}.config-section--full{grid-column:1/-1}.content-data{max-width:640px;width:calc(100% - 28px);margin:12px auto 32px;padding:0;background:transparent!important;box-shadow:none!important;border-radius:0;text-align:left}.data-sheet{display:flex;flex-direction:column;gap:16px;padding:0 2px}.data-block{background:#fff;border:1px solid #e0e0e0;border-radius:14px;overflow:hidden;box-shadow:0 2px 8px rgba(0,0,0,.04)}.data-block__title{margin:0;padding:12px 18px 11px 14px;font-size:13px;font-weight:700;letter-spacing:.06em;text-transform:uppercase;color:#333;background:#f7f7f7;border-bottom:1px solid #ececec;border-left:4px solid #007bff}.data-block__rows{padding:4px 0 6px}.data-line{display:grid;grid-template-columns:minmax(0,1fr) auto;gap:8px 24px;align-items:center;padding:11px 18px;border-bottom:1px solid #f3f3f3}.data-line:last-child{border-bottom:none}.data-line:nth-child(even){background:#fafafa}.data-line__name{font-size:14px;font-weight:500;color:#555;line-height:1.4}.data-line__reading{text-align:right;line-height:1.2}.data-line__val{font-size:15px;font-weight:600;color:#111;font-variant-numeric:tabular-nums;word-break:break-word}.data-sheet--readings .data-line__val{font-size:19px;font-weight:600;letter-spacing:-.01em}.data-line__unit{font-size:12px;font-weight:500;color:#999;margin-left:5px}.data-sheet--readings .data-line__unit{font-size:13px}.data-api{display:grid;grid-template-columns:minmax(0,1.1fr) minmax(0,1.9fr);gap:10px 20px;align-items:center;padding:13px 18px;border-bottom:1px solid #f0f0f0}.data-block__rows .data-api:last-child{border-bottom:none}.data-api:nth-child(even){background:#fafafa}.data-api__name{font-size:14px;font-weight:600;color:#111;line-height:1.35}.data-api__details{display:flex;flex-wrap:wrap;gap:6px 14px;justify-content:flex-end;align-items:center}.data-api__badge{display:inline-block;padding:4px 11px;border-radius:20px;font-size:11px;font-weight:700;letter-spacing:.04em;text-transform:uppercase}.data-api__badge--ok{background:#e6f4ea;color:#137333}.data-api__badge--err{background:#fce8e6;color:#c5221f}.data-api__detail{font-size:13px;color:#333;white-space:nowrap}.data-api__detail-lbl{color:#999;font-size:10px;font-weight:600;text-transform:uppercase;letter-spacing:.05em;margin-right:4px}.data-busy-msg{margin:24px 0;padding:20px;text-align:center;color:#666;font-size:15px;background:#fff;border-radius:12px;border:1px solid #e8e8e8}@media screen and (max-width:520px){.data-line{grid-template-columns:1fr;gap:4px;padding:12px 16px}.data-line__reading{text-align:left}.data-sheet--readings .data-line__val{font-size:17px}.data-api{grid-template-columns:1fr;gap:8px}.data-api__details{justify-content:flex-start}}@media screen and (min-width:1280px){.config-form .panel.active{grid-template-columns:repeat(auto-fill,minmax(300px,1fr))}.checkbox-grid{grid-template-columns:repeat(auto-fill,minmax(155px,1fr))}}@media screen and (max-width:860px){.config-layout{flex-direction:column}.config-sidebar{flex:none;border-right:none;border-bottom:1px solid #e2e8f4;padding:12px;position:static;max-height:none;overflow:visible}.config-nav.tabs{flex-direction:row;flex-wrap:wrap;gap:8px}.config-form .tab{text-align:center;padding:8px 14px;border-left:1px solid transparent}.config-form .tab.active{border-left:1px solid #ddd;padding-left:14px}.config-main{padding:16px}.config-form .panel.active{grid-template-columns:1fr}.config-section__body--compact{grid-template-columns:1fr}.config-form-footer{text-align:center}.config-form-footer .submit-btn{width:100%}}@media screen and (max-width:600px){.configuration .content.content-config,.content-data{width:calc(100% - 16px);margin:12px auto 20px}.config-form .tab{font-size:13px}}.submit-btn:disabled,.submit-btn.is-loading{opacity:.5;cursor:wait;pointer-events:none;box-shadow:none;transform:none}.page-form{display:flex;flex-direction:column;gap:16px;margin-top:16px}.ui-notice{margin:0 0 14px;padding:12px 16px;border-radius:12px;font-size:14px;line-height:1.45;border:1px solid}.ui-notice--ok{background:#e6f4ea;border-color:#c8e6c9;color:#137333}.ui-notice--warn{background:#fff8e6;border-color:#f0c040;color:#8a6d00}.ui-notice--err{background:#fce8e6;border-color:#ef9a9a;color:#c5221f}.ui-notice--info{background:#eef3ff;border-color:#c8d9ff;color:#1a237e}.code-mono{display:block;font-family:ui-monospace,SFMono-Regular,Consolas,monospace;font-size:12px;line-height:1.45;word-break:break-all;color:#111;background:#fff;border:1px solid #e0e0e0;border-radius:8px;padding:8px 10px;margin:6px 0}.code-mono--inline{display:inline;padding:2px 6px;margin:0}.code-mono--pre{white-space:pre-wrap;margin:8px 0 0}.radio-list{display:flex;flex-direction:column;gap:8px;margin:4px 0 2px}.radio-list .guest-option{margin-bottom:0}.group-panel{display:none}.group-panel--visible{display:block}.page-form-footer{margin-top:4px;padding-top:16px;border-top:1px solid #eef1f7;text-align:right}.page-form-footer .submit-btn{margin:0;display:inline-block;min-width:200px;padding:12px 28px;border-radius:10px}.data-line--stack{grid-template-columns:1fr;gap:6px}.data-line--stack .data-line__reading{text-align:left}.config-section .form-group textarea,.config-section .form-group .input-mono{font-family:ui-monospace,SFMono-Regular,Consolas,monospace;font-size:12px}@media screen and (max-width:600px){.page-form-footer{text-align:center}.page-form-footer .submit-btn{width:100%}}
body.app-shell{display:flex;flex-direction:column;min-height:100vh;background:#f4f4f4;padding:0}
.app-topbar{padding:0;background:#007bff;color:#fff;border-bottom:1px solid #0062cc;box-sizing:border-box;width:100%}
.app-topbar__inner{display:grid;grid-template-columns:auto 1fr;grid-template-areas:"brand primary" "chips chips";align-items:center;column-gap:12px;row-gap:10px;width:100%;max-width:1400px;margin:0 auto;padding:14px 20px 16px;box-sizing:border-box}
.app-topbar__brand{grid-area:brand;display:flex;flex-shrink:0;background:none!important;padding:0;margin:0}
.app-topbar__logo{width:44px;height:44px;max-width:44px;border-radius:11px;object-fit:contain;display:block}
.app-topbar__primary{grid-area:primary;display:flex;align-items:center;min-width:0}
.app-topbar__heading{min-width:0;flex:1;display:flex;flex-direction:column;gap:2px}
.app-topbar__title{margin:0;font-size:18px;font-weight:700;line-height:1.2;letter-spacing:-.01em;text-transform:none;min-width:0;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.app-topbar__tags{display:flex;flex-wrap:wrap;gap:6px;margin-top:4px}
.app-topbar__tag{display:inline-flex;align-items:center;max-width:100%;padding:3px 8px;border-radius:999px;font-size:10px;font-weight:600;line-height:1.3;color:rgba(255,255,255,.85);background:#1a3a66;border:1px solid rgba(255,255,255,.2);text-decoration:none;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.app-topbar__tag--paired{color:#d6d6d6;border-color:#3a3a3a}
.app-topbar__tag--ota{color:#ffb040;border-color:rgba(255,179,64,.45);background:rgba(255,179,64,.1)}
.app-topbar__tag--ota:hover{background:rgba(255,179,64,.16);border-color:rgba(255,179,64,.6)}
.app-topbar__chips{grid-area:chips;display:grid;grid-template-columns:minmax(0,1fr) minmax(0,1.7fr) minmax(0,1fr);gap:10px}
.app-topbar__chip{display:flex;flex-direction:column;align-items:flex-start;gap:8px;min-width:0;padding:14px 16px;border-radius:14px;border:none;background:#fff;box-shadow:0 2px 12px rgba(10,40,90,.1);text-align:left;color:#152238;font:inherit;box-sizing:border-box}
.app-topbar__chip--send{gap:0}
.app-topbar__chip--ok,.app-topbar__chip--warn,.app-topbar__chip--off{border:none;background:#fff}
.app-topbar__chip--copy{cursor:pointer;transition:box-shadow .15s}
.app-topbar__chip--copy:hover{box-shadow:0 4px 16px rgba(10,40,90,.14)}
.app-topbar__chip-head{display:flex;align-items:center;justify-content:space-between;gap:8px;width:100%;min-width:0}
.app-topbar__chip-lbl{font-size:10px;font-weight:600;letter-spacing:.08em;text-transform:uppercase;color:#9aa3af;line-height:1.2}
.app-topbar__chip-status{display:inline-flex;align-items:center;gap:6px;font-size:13px;font-weight:700;color:#152238;line-height:1.2;min-width:0}
.app-topbar__chip-val{font-size:15px;font-weight:700;color:#152238;line-height:1.3;word-break:break-word;display:inline-flex;align-items:center;gap:8px;flex-shrink:0}
.app-topbar__chip-val--mono{font-family:ui-monospace,SFMono-Regular,Consolas,monospace;font-weight:700;font-size:15px;word-break:break-all;color:#152238}
.app-topbar__chip-val--ip::before{content:"";width:18px;height:18px;flex-shrink:0;background:#2f80ed;-webkit-mask:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' fill='none' viewBox='0 0 24 24' stroke='black' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'%3E%3Ccircle cx='12' cy='12' r='10'/%3E%3Cpath d='M2 12h20M12 2a15.3 15.3 0 0 1 4 10 15.3 15.3 0 0 1-4 10 15.3 15.3 0 0 1-4-10 15.3 15.3 0 0 1 4-10z'/%3E%3C/svg%3E") center/contain no-repeat;mask:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' fill='none' viewBox='0 0 24 24' stroke='black' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'%3E%3Ccircle cx='12' cy='12' r='10'/%3E%3Cpath d='M2 12h20M12 2a15.3 15.3 0 0 1 4 10 15.3 15.3 0 0 1-4 10 15.3 15.3 0 0 1-4-10 15.3 15.3 0 0 1 4-10z'/%3E%3C/svg%3E") center/contain no-repeat}
.app-topbar__chip-wifi{font-size:12px;font-weight:500;color:#8b95a5;line-height:1.35;text-align:right;word-break:break-word;overflow-wrap:anywhere;min-width:0;flex:1;display:inline-flex;align-items:center;justify-content:flex-end;gap:6px}
.app-topbar__chip-wifi::before{content:"";width:14px;height:14px;flex-shrink:0;background:#2f80ed;-webkit-mask:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'%3E%3Cpath fill='black' d='M12 18.5a1.5 1.5 0 1 0 0 3 1.5 1.5 0 0 0 0-3zm0-5.2c-1.8 0-3.4.7-4.6 1.9l1.4 1.4A4.5 4.5 0 0 1 12 15.3c1.2 0 2.3.5 3.2 1.3l1.4-1.4A6.5 6.5 0 0 0 12 13.3zm0-4.4c-3 0-5.7 1.2-7.7 3.2l1.4 1.4A8.9 8.9 0 0 1 12 11c2.4 0 4.6.9 6.3 2.5l1.4-1.4A11 11 0 0 0 12 8.9zm0-4.4C6.7 4.5 2.2 6.7.1 10.1l1.4 1.4C3.4 8.4 7.4 6.5 12 6.5s8.6 1.9 10.5 5l1.4-1.4C21.8 6.7 17.3 4.5 12 4.5z'/%3E%3C/svg%3E") center/contain no-repeat;mask:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'%3E%3Cpath fill='black' d='M12 18.5a1.5 1.5 0 1 0 0 3 1.5 1.5 0 0 0 0-3zm0-5.2c-1.8 0-3.4.7-4.6 1.9l1.4 1.4A4.5 4.5 0 0 1 12 15.3c1.2 0 2.3.5 3.2 1.3l1.4-1.4A6.5 6.5 0 0 0 12 13.3zm0-4.4c-3 0-5.7 1.2-7.7 3.2l1.4 1.4A8.9 8.9 0 0 1 12 11c2.4 0 4.6.9 6.3 2.5l1.4-1.4A11 11 0 0 0 12 8.9zm0-4.4C6.7 4.5 2.2 6.7.1 10.1l1.4 1.4C3.4 8.4 7.4 6.5 12 6.5s8.6 1.9 10.5 5l1.4-1.4C21.8 6.7 17.3 4.5 12 4.5z'/%3E%3C/svg%3E") center/contain no-repeat}
.app-topbar__chip-sub{font-size:12px;font-weight:500;color:#6b7280;line-height:1.35;word-break:break-word}
.app-topbar__chip-sub--muted{color:#9aa3af;font-size:12px}
.app-topbar__chip-sub--fw{color:#152238;font-size:13px;font-family:ui-monospace,SFMono-Regular,Consolas,monospace;font-weight:700;word-break:break-word;overflow-wrap:anywhere}
.app-topbar__chip-sub--ota{color:#007bff;text-decoration:none}
.app-topbar__chip-sub--ota:hover{text-decoration:underline}
.app-topbar__chip-meta{display:flex;flex-direction:column;gap:4px;width:100%;padding-top:10px;margin-top:2px;border-top:1px solid #eef1f4}
.app-topbar__chip-meta-lbl{font-size:11px;font-weight:500;color:#9aa3af;line-height:1.2}
.app-topbar__send-rows{display:flex;flex-direction:column;gap:0;width:100%;min-width:0}
.app-topbar__send-row{display:flex;flex-direction:column;align-items:stretch;gap:2px;width:100%;min-width:0;padding:10px 0;border-top:1px solid #eef1f4}
.app-topbar__send-row:first-child{border-top:none;padding-top:8px}
.app-topbar__send-row-top{display:flex;align-items:center;justify-content:space-between;gap:10px;width:100%;min-width:0}
.app-topbar__send-row-lbl{font-size:14px;font-weight:700;color:#152238;line-height:1.3;flex-shrink:0}
.app-topbar__send-row-val{display:inline-flex;align-items:center;gap:6px;font-size:13px;font-weight:600;color:#8b95a5;line-height:1.3;min-width:0}
.app-topbar__send-row-age{font-size:11px;font-weight:500;color:#9aa3af;line-height:1.3;padding-left:0}
.app-topbar__send-row--muted .app-topbar__send-row-val{color:#9aa3af;font-weight:500}
.app-topbar__send-row--warn .app-topbar__send-row-val{color:#f59e0b}
.app-topbar__chip-addr{display:flex;flex-direction:column;align-items:flex-start;gap:4px;width:100%;margin:0;padding:10px 0 0;border:0;border-top:1px solid #eef1f4;background:none;color:inherit;font:inherit;text-align:left;cursor:pointer;box-sizing:border-box}
.app-topbar__chip-addr-lbl{color:#9aa3af;font-size:11px;font-weight:500;letter-spacing:0;line-height:1.4}
.app-topbar__chip-addr-val{color:#152238;font-family:ui-monospace,SFMono-Regular,Consolas,monospace;font-size:13px;font-weight:700;line-height:1.4;text-align:left;word-break:break-all;overflow-wrap:anywhere;min-width:0;width:100%;display:flex;align-items:flex-start;gap:8px}
.app-topbar__chip-addr-val::after{content:"";width:14px;height:14px;flex-shrink:0;margin-top:2px;background:#2f80ed;-webkit-mask:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'%3E%3Cpath fill='black' d='M16 1H4c-1.1 0-2 .9-2 2v12h2V3h12V1zm3 4H8c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h11c1.1 0 2-.9 2-2V7c0-1.1-.9-2-2-2zm0 16H8V7h11v14z'/%3E%3C/svg%3E") center/contain no-repeat;mask:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'%3E%3Cpath fill='black' d='M16 1H4c-1.1 0-2 .9-2 2v12h2V3h12V1zm3 4H8c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h11c1.1 0 2-.9 2-2V7c0-1.1-.9-2-2-2zm0 16H8V7h11v14z'/%3E%3C/svg%3E") center/contain no-repeat}
.app-topbar__chip-addr:hover .app-topbar__chip-addr-val{color:#0d47a1}
.app-topbar__chip-copy{cursor:pointer}
.app-topbar__chip-hint{font-size:12px;font-weight:500;color:#2f80ed;line-height:1.2;display:inline-flex;align-items:center;gap:6px}
.app-topbar__chip-hint::after{content:"";width:13px;height:13px;background:#2f80ed;-webkit-mask:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'%3E%3Cpath fill='black' d='M16 1H4c-1.1 0-2 .9-2 2v12h2V3h12V1zm3 4H8c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h11c1.1 0 2-.9 2-2V7c0-1.1-.9-2-2-2zm0 16H8V7h11v14z'/%3E%3C/svg%3E") center/contain no-repeat;mask:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'%3E%3Cpath fill='black' d='M16 1H4c-1.1 0-2 .9-2 2v12h2V3h12V1zm3 4H8c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h11c1.1 0 2-.9 2-2V7c0-1.1-.9-2-2-2zm0 16H8V7h11v14z'/%3E%3C/svg%3E") center/contain no-repeat}
.app-topbar__chip--copy:hover .app-topbar__chip-hint{color:#0d47a1}
.app-topbar__dot{width:8px;height:8px;border-radius:50%;flex-shrink:0;background:#c5cbd3}
.app-topbar__chip--ok .app-topbar__chip-val .app-topbar__dot{background:#22c55e;box-shadow:0 0 0 3px rgba(34,197,94,.18)}
.app-topbar__chip--warn .app-topbar__chip-val .app-topbar__dot{background:#f59e0b;box-shadow:0 0 0 3px rgba(245,158,11,.18)}
.app-topbar__chip--off .app-topbar__chip-val .app-topbar__dot{background:#ef4444;box-shadow:0 0 0 3px rgba(239,68,68,.16)}
.app-topbar__send-row--ok .app-topbar__dot{width:16px;height:16px;border-radius:50%;background:#22c55e;box-shadow:none;position:relative}
.app-topbar__send-row--ok .app-topbar__dot::after{content:"";position:absolute;left:5px;top:3px;width:4px;height:8px;border:solid #fff;border-width:0 2px 2px 0;transform:rotate(45deg)}
.app-topbar__send-row--warn .app-topbar__dot{width:0;height:0;border-radius:0;background:transparent;box-shadow:none;border-left:7px solid transparent;border-right:7px solid transparent;border-bottom:12px solid #f59e0b}
.app-topbar__send-row--muted .app-topbar__dot{background:#c5cbd3;box-shadow:none}
@media screen and (max-width:900px){.app-topbar__chips{grid-template-columns:1fr}}
@media screen and (max-width:520px){.app-topbar__inner{padding:10px 12px 12px;column-gap:8px;row-gap:8px}.app-topbar__logo{width:36px;height:36px;max-width:36px;border-radius:9px}.app-topbar__title{font-size:15px}.app-topbar__chips{gap:6px}.app-topbar__chip{padding:8px 10px;gap:3px;border-radius:10px}.app-topbar__chip-val{font-size:13px}.app-topbar__chip-val--mono{font-size:12px}.app-topbar__chip-status{font-size:12px}.app-topbar__send-row-val{font-size:12px}.app-topbar__chip-addr{font-size:12px}.app-topbar__chip-hint{font-size:10px}}
.app-breadcrumb{display:none;align-items:center;flex-wrap:wrap;gap:6px 8px;margin:0 0 12px;font-size:14px;line-height:1.4}
.app-breadcrumb__link{color:#007bff;font-weight:600;text-decoration:none}
.app-breadcrumb__link:hover{text-decoration:underline;color:#0062cc}
.app-breadcrumb__sep{color:#ccc;font-size:16px;line-height:1}
.app-breadcrumb__current{color:#555;font-weight:500}
@media screen and (min-width:721px){.app-breadcrumb{display:flex}body[data-page=local] .app-breadcrumb{display:none}.app-shell .app-page-title{display:none}}
.status-pill{display:inline-block;padding:4px 10px;border-radius:999px;font-size:10px;font-weight:700;text-transform:uppercase;letter-spacing:.04em;white-space:nowrap;flex-shrink:0}
.status-pill--ok{background:#e6f4ea;color:#137333}
.status-pill--warn{background:#fce8e6;color:#c5221f}
.content-app{max-width:none;width:100%;margin:24px 0 0;padding:0 12px;background:transparent!important;box-shadow:none!important;border-radius:0;text-align:left;flex:1;box-sizing:border-box}
html{scroll-behavior:smooth}
.app-layout{display:flex;flex:1;width:100%;max-width:1400px;margin:0 auto;box-sizing:border-box;align-items:stretch}
.app-sidebar{display:none;flex:0 0 220px;padding:32px 10px 20px 12px;border-right:1px solid #e8e8e8;background:#fafafa;box-sizing:border-box}
.app-sidebar__nav{display:flex;flex-direction:column;gap:18px}
.app-sidebar__block{display:flex;flex-direction:column;gap:2px}
.app-sidebar__hub{gap:12px}
.app-sidebar__heading{display:block;margin:10px 0 4px 10px;font-size:10px;font-weight:600;letter-spacing:.08em;text-transform:uppercase;color:#999}
.app-sidebar__block>.app-sidebar__heading:first-child{margin-top:0}
.app-sidebar__item,.app-sidebar__subitem{display:block;padding:10px 12px;border-radius:10px;font-size:14px;font-weight:600;color:#444;text-decoration:none;line-height:1.3;transition:background .15s,color .15s}
.app-sidebar__subitem{font-size:13px;font-weight:500;color:#555;padding:8px 12px 8px 16px}
.app-sidebar__item:hover,.app-sidebar__subitem:hover,.app-sidebar__subitem.is-active{background:rgba(0,0,0,.04);color:#000}
.app-sidebar__item--local,.app-sidebar__item--social,.app-sidebar__item--custom,.app-sidebar__item--advanced{border-left:3px solid #007bff;padding-left:9px}
.app-sidebar__subitem--danger{color:#b71c1c}
.hub-section[id],.config-section[id],.hub-map-link[id]{scroll-margin-top:16px}
.app-sidebar__sub{display:none;margin:2px 0 8px 8px;padding-left:4px;border-left:2px solid #e6e6e6}
body[data-page=local] .app-sidebar__sub--local,body[data-page=social] .app-sidebar__sub--social,body[data-page=custom] .app-sidebar__sub--custom,body[data-page=advanced] .app-sidebar__sub--advanced{display:flex;flex-direction:column;gap:2px}
body[data-page=local] .app-sidebar__item[data-tab=local],body[data-page=social] .app-sidebar__item[data-tab=social],body[data-page=custom] .app-sidebar__item[data-tab=custom],body[data-page=advanced] .app-sidebar__item[data-tab=advanced]{background:#fff;color:#000;box-shadow:0 1px 4px rgba(0,0,0,.06);border-left-color:#c7fe6c}
.hub-welcome__areas{display:flex;flex-direction:column;gap:12px;margin-top:8px}
.hub-welcome__area{padding:16px 18px;background:#fff;border:1px solid #ddd;border-radius:12px}
.hub-welcome__area--local{border-left:4px solid #007bff}
.hub-welcome__area--social{border-left:4px solid #2e7d32}
.hub-welcome__area--custom{border-left:4px solid #5c6bc0}
.hub-welcome__title{margin:0 0 6px;font-size:17px;font-weight:700;color:#111}
.hub-welcome__desc{margin:0;font-size:13px;color:#666;line-height:1.45}
@media screen and (min-width:721px){.app-sidebar{display:block;position:sticky;top:0;align-self:flex-start;max-height:100vh;overflow-y:auto;overscroll-behavior:contain}.app-layout .content-app{max-width:none;width:auto;margin:0;padding:32px 20px 24px 20px;flex:1;min-width:0}}
@media screen and (max-width:720px){.app-layout{padding:0;max-width:1400px;margin:0 auto}.app-layout .content-app{width:100%;margin:24px 0 0;padding:0 12px}}
body.app-shell .app-main{padding-bottom:calc(24px + env(safe-area-inset-bottom,0px))}
.app-page-title{margin:0 0 16px;font-size:22px;font-weight:700;color:#111;line-height:1.25}
body[data-page=local] .app-page-title{display:none}
.dashboard{display:flex;flex-direction:column;gap:16px}
.dashboard{display:flex;flex-direction:column;gap:18px}
.hub-grid{display:flex;flex-direction:column;gap:12px;margin-top:4px}
.hub-card{display:flex;flex-direction:column;gap:6px;padding:20px 18px;background:#fff;border:1px solid #ddd;border-radius:14px;color:#111;text-decoration:none;position:relative;transition:background .15s,border-color .15s,box-shadow .15s;box-shadow:0 1px 3px rgba(0,0,0,.04)}
.hub-card:hover{background:#fafafa;border-color:#ccc;color:#000;box-shadow:0 4px 14px rgba(0,0,0,.06)}
.hub-card__title{font-size:20px;font-weight:700;line-height:1.25;color:#111;letter-spacing:-.01em}
.hub-card__desc{font-size:13px;font-weight:400;color:#666;line-height:1.4;padding-right:28px}
.hub-card__chev{position:absolute;right:16px;top:50%;transform:translateY(-50%);font-size:22px;color:#ccc;line-height:1}
.hub-card--local{border-left:4px solid #007bff}
.hub-card--social{border-left:4px solid #2e7d32}
.hub-card--custom{border-left:4px solid #5c6bc0}
.hub-links{display:flex;flex-direction:column;gap:0;border:1px solid #ddd;border-radius:12px;overflow:hidden;background:#fff}
.hub-link{display:flex;align-items:center;justify-content:space-between;gap:12px;padding:16px 18px;border-bottom:1px solid #efefef;background:#fff;color:#111;text-decoration:none;font-size:16px;font-weight:600;line-height:1.3;transition:background .15s}
.hub-link:last-child{border-bottom:none}
.hub-link:hover{background:#f9f9f9;color:#000}
.hub-link__label{flex:1;min-width:0}
.hub-link__chev{flex-shrink:0;font-size:18px;color:#ccc;font-weight:400}
.hub-link--external .hub-link__chev{font-size:15px;color:#999}
.hub-link--danger .hub-link__label{color:#b71c1c}
.hub-link--danger:hover{background:#fdf6f6}
.hub-page{display:flex;flex-direction:column;gap:44px}
.hub-group{display:flex;flex-direction:column;gap:16px}
.hub-group+.hub-group{padding-top:36px;border-top:2px solid #ddd}
.hub-group__head{padding:0 2px 10px}
.hub-group__title{margin:0;font-size:13px;font-weight:700;letter-spacing:.12em;text-transform:uppercase;color:#111}
.hub-group__intro{margin:6px 0 0;font-size:13px;color:#666;line-height:1.4}
.hub-group__sections{display:flex;flex-direction:column;gap:28px}.hub-group--settings>.hub-group__sections{gap:28px}
.hub-group__sections>.hub-section{margin:0}
.hub-section[id='restart'] .hub-section__head,.hub-section[id='reset'] .hub-section__head{background:#fdf6f6;border-bottom-color:#f5e0e0}
.hub-section[id='restart'] .hub-section__title,.hub-section[id='reset'] .hub-section__title{color:#b71c1c}
.hub-subsection{margin-top:24px;padding-top:22px;border-top:1px solid #e5e5e5}
.hub-subsection:first-child{margin-top:0;padding-top:0;border-top:0}
.hub-subsection__title{margin:0 0 12px;font-size:13px;font-weight:600;color:#444}
.hub-section{background:#fff;border:1px solid #ddd;border-radius:12px;overflow:hidden;box-shadow:0 1px 3px rgba(0,0,0,.04)}.hub-config-stack>.config-section,.hub-config-stack>.config-row{margin:0}.hub-config-stack>.config-section,.hub-page>.hub-section{box-shadow:0 1px 3px rgba(0,0,0,.04)}
.map-container{position:relative;width:100%;max-width:100%;height:320px;overflow:hidden;box-sizing:border-box;border-radius:8px}
.map-container #map,.map-container .leaflet-container{position:absolute!important;top:0;right:0;bottom:0;left:0;width:100%!important;height:100%!important;max-width:100%;min-height:0;overflow:hidden!important}
.config-section--gps,.config-section--gps .config-section__body,.hub-config-stack .config-section--gps{overflow:hidden;max-width:100%;min-width:0}
.config-section--gps .map-container{flex:0 0 auto;width:100%}
.hub-section:has(.map-container){overflow:hidden}
.hub-section:has(.map-container) .hub-section__body{overflow:hidden}
.hub-section:has(.map-container) .map-container{height:320px;max-width:100%}
.hub-section__head{padding:12px 14px;border-bottom:1px solid #efefef;background:#fafafa}
.hub-section__title{margin:0;font-size:15px;font-weight:700;letter-spacing:.04em;text-transform:uppercase;color:#333}
.hub-section__body{padding:12px 14px 14px}
.hub-section__body .data-sheet{padding:0;gap:12px}
.hub-config-form{margin:0}
.hub-config-stack>.config-row{grid-template-columns:1fr}
.hub-config-stack>.config-section--full,.hub-config-stack>.config-row>.config-section--full{width:100%;max-width:100%}
.confirm-action__step[hidden]{display:none!important}
.confirm-action__step .ui-notice{margin:0 0 16px}
.confirm-action__step .ui-notice .dash-row__desc{margin:6px 0 0;color:inherit;opacity:.9}
.hub-config-stack{display:flex;flex-direction:column;gap:24px}
.hub-ota,.hub-ota__actions{display:flex;flex-direction:column;gap:20px}
.hub-ota .config-section{margin:0}
.hub-ota .data-block{margin:0}
.hub-ota__check-row{display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap;padding:12px 16px}
.hub-ota__check-row .config-section__title{border-bottom:none;padding:0;flex:1 1 auto;min-width:0}
.hub-ota__check-row .submit-btn{flex:0 0 auto;width:auto;margin:0}
.hub-ota__check-result{display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap;padding:14px 16px 16px;border-top:1px solid #e8e8e8}
.hub-ota__check-result[hidden]{display:none!important}
.hub-ota__check-result-main{flex:1 1 12rem;min-width:0;display:flex;flex-direction:column;gap:10px}
.hub-ota__check-msg{margin:0;flex:1 1 auto;min-width:0;font-size:14px;line-height:1.4;color:#222}
.hub-ota__check-msg--warn{color:#8a5a00}
.hub-ota__check-result .submit-btn,.hub-ota__install-btn{flex:0 0 auto;width:auto;margin:0}
.hub-ota__install-btn{font-size:14px;padding:10px 18px;line-height:1.3;min-height:0;background:#c7fe6c;color:#000;border:none}
.hub-ota__install-btn:hover{background:#b8ef5d;color:#000}
.hub-ota__progress{display:flex;align-items:center;gap:10px}
.hub-ota__progress[hidden]{display:none!important}
.hub-ota__progress-track{flex:1 1 auto;height:8px;border-radius:999px;background:#ececec;overflow:hidden;min-width:80px}
.hub-ota__progress-bar{height:100%;width:0;border-radius:999px;background:#c7fe6c;transition:width .25s ease}
.hub-ota__progress-pct{flex:0 0 auto;font-size:13px;font-weight:600;color:#111;min-width:2.75em;text-align:right}
.hub-social-top{display:flex;flex-direction:column;gap:16px}.hub-map-link{margin:0}
.hub-map-link .b{display:inline-flex;width:auto;max-width:100%}
.hub-config-form>.hub-config-stack{margin:0}
.hub-config-footer{position:static;background:transparent;border-top:1px solid #e5e5e5;margin-top:18px;padding-top:16px;text-align:center;order:999}
.hub-config-footer .submit-btn{width:100%;max-width:320px}

.dashboard-health{padding:14px 16px;background:#fff;border:1px solid #ddd;border-radius:12px;border-left:3px solid #007bff}
.dashboard-health__title{margin:0 0 10px;font-size:12px;font-weight:600;letter-spacing:.06em;text-transform:uppercase;color:#888}
.dashboard-health__pills{display:flex;flex-wrap:wrap;gap:8px 12px;align-items:center}
.dashboard-health__item{font-size:13px;color:#555;line-height:1.35}
.dash-group{border:1px solid #ddd;border-radius:12px;overflow:hidden;background:#fff}
.dash-group__head{padding:14px 16px 12px;background:#f7f7f7;border-bottom:1px solid #e5e5e5;border-left:3px solid #007bff}
.dash-group--local .dash-group__head{border-left-color:#007bff}
.dash-group--social .dash-group__head{border-left-color:#2e7d32;background:#f6faf6}
.dash-group--custom .dash-group__head{border-left-color:#5c6bc0;background:#f7f7fc}
.dash-group__title{margin:0 0 4px;font-size:15px;font-weight:700;letter-spacing:.01em;text-transform:none;color:#111;line-height:1.3}
.dash-group__intro{margin:0;font-size:13px;color:#666;line-height:1.45}
.dash-group__list{display:flex;flex-direction:column;background:#fff}
.dash-group__category{margin:0;padding:10px 16px 6px;font-size:11px;font-weight:600;letter-spacing:.06em;text-transform:uppercase;color:#888;background:#fafafa;border-top:1px solid #eee}
.dash-group__list>.dash-group__category:first-child{border-top:none}
.dash-row{display:flex;align-items:center;gap:12px;padding:14px 16px;border-bottom:1px solid #efefef;background:#fff;color:#111;text-decoration:none;min-height:54px;transition:background .15s;margin:0;box-shadow:none;border-radius:0;font-weight:400}
.dash-row:last-child{border-bottom:none}
.dash-row:hover{background:#f9f9f9;color:#000}
.dash-row__body{flex:1;min-width:0}
.dash-row__label{display:block;font-size:15px;font-weight:600;color:#111;line-height:1.3}
.dash-row__desc{display:block;margin-top:3px;font-size:12px;font-weight:400;color:#777;line-height:1.35}
.dash-row__chev{flex-shrink:0;font-size:18px;line-height:1;color:#ccc;font-weight:400}
.dash-row--external{border-top:1px solid #e8e8e8;background:#fcfcfc}
.dash-row--external .dash-row__chev{font-size:15px;color:#999}
.dash-row--danger .dash-row__label{color:#b71c1c}
.dash-row--danger:hover{background:#fdf6f6}
.app-page-body{display:flex;flex-direction:column;gap:14px}
.app-page-lead{margin:0;font-size:14px;color:#666;line-height:1.45}
.app-shell .app-page-title{margin:0 0 4px;font-size:20px;font-weight:700;color:#111}
.app-shell .data-sheet{gap:12px}
.app-shell .data-sheet .data-block{border:1px solid #ddd;border-radius:12px;box-shadow:none;overflow:hidden}
.app-shell .data-block__title{background:#f7f7f7;border-bottom:1px solid #e5e5e5;border-left:3px solid #007bff;padding:10px 12px;margin:0}
.data-block__intro{margin:0;padding:8px 12px 2px;font-size:13px;color:#666;line-height:1.45}
.reading-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px;padding:10px 12px 12px}
.reading-card{display:flex;flex-direction:column;justify-content:center;gap:6px;padding:14px 16px;background:#fafafa;border:1px solid #eee;border-radius:10px;min-height:84px;min-width:0;overflow:hidden}
.reading-card__label{font-size:15px;font-weight:500;color:#555;line-height:1.3}
.reading-card__reading{display:flex;flex-wrap:nowrap;align-items:baseline;gap:6px;min-width:0}
.reading-card__value{font-size:34px;font-weight:700;color:#111;font-variant-numeric:tabular-nums;line-height:1.05;letter-spacing:-.02em;}
.reading-card__unit{font-size:15px;font-weight:500;color:#888;white-space:nowrap;flex-shrink:0}
.reading-card--text{min-height:0;grid-column:1/-1}
.reading-card--text .reading-card__reading{flex-wrap:wrap}
.reading-card--text .reading-card__value{font-size:17px;font-weight:600;letter-spacing:0;line-height:1.35;font-family:ui-monospace,SFMono-Regular,Consolas,monospace;font-variant-numeric:normal;word-break:break-word;overflow-wrap:anywhere}
@media screen and (max-width:860px){.reading-card--text .reading-card__value{font-size:15px}}
.data-block--technical .data-block__title{color:#777;border-left-color:#bbb;background:#fafafa}
.data-block__head{display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap;background:#fafafa;border-bottom:1px solid #e5e5e5}
.data-block__head .data-block__title{border-bottom:none;flex:1;min-width:0}
.data-block__head #tech-details-copy{margin:8px 12px 8px 0;flex-shrink:0}
.data-block--technical .data-line{padding:11px 16px}
.data-block--technical .data-line__name{font-size:14px;color:#888}
.data-block--technical .data-line__val{font-size:15px;font-weight:500}
@media screen and (min-width:900px){.reading-grid{grid-template-columns:repeat(3,minmax(0,1fr))}}
@media screen and (max-width:500px){.reading-grid{grid-template-columns:1fr}.reading-card__value{font-size:30px}.reading-card--text .reading-card__value{font-size:14px}}
.app-shell .config-section__title{padding:12px 14px;font-size:14px}
.app-shell .config-section__body{padding:12px 14px 14px;gap:10px}
.app-shell input[type=email],.app-shell input[type=number],.app-shell input[type=password],.app-shell input[type=text],.app-shell select,.app-shell textarea{font-size:16px;padding:10px 12px;margin:6px 0}
.app-shell label{font-size:16px}
.input-narrow{width:12ch!important;max-width:10rem!important;min-width:6rem;display:block;box-sizing:content-box}
.app-shell .form-group:has(.input-narrow){overflow:visible}.form-fields-pack{display:flex;flex-wrap:wrap;gap:12px 20px;align-items:flex-start}.form-fields-pack>.form-group{flex:0 1 auto;width:auto;max-width:16rem;margin-bottom:0;overflow:visible}
.app-shell .page-form{gap:14px;margin-top:2px}
.app-shell .page-form .config-section{border:1px solid #ddd;border-radius:12px;overflow:hidden;box-shadow:none}
.app-panel{border:1px solid #ddd;border-radius:12px;overflow:hidden;background:#fff}.debug-log{display:block;margin:0;padding:12px 16px;max-height:280px;overflow:auto;background:#111;color:#e8e8e8;font-family:ui-monospace,SFMono-Regular,Consolas,monospace;font-size:11px;line-height:1.55;white-space:pre-wrap;overflow-wrap:anywhere;word-break:break-word;border:none;border-radius:0;-webkit-overflow-scrolling:touch}
.app-panel__title{margin:0;padding:12px 16px;font-size:13px;font-weight:700;background:#f7f7f7;border-bottom:1px solid #e5e5e5;border-left:3px solid #007bff}
.app-panel--confirm{border-color:#e8d4d4}
.app-panel--confirm .app-panel__title{border-left-color:#b71c1c}

.debug-level-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:8px;padding:12px 16px 16px}
.debug-level-btn{display:flex;align-items:center;justify-content:center;padding:12px 10px;background:#fafafa;border:1px solid #ddd;border-radius:10px;font-size:14px;font-weight:600;color:#111;text-align:center;min-height:44px;margin:0;box-shadow:none}
.debug-level-btn:hover{background:#fff;border-color:#999;color:#000}
.app-shell.configuration .app-page-lead{max-width:900px}
.confirm-action{padding:4px 2px 2px}
.app-panel .confirm-action{padding:14px 16px 16px}
.confirm-action__question{margin:0 0 16px;font-size:16px;font-weight:600;color:#111;line-height:1.45}
.confirm-action__options{margin-bottom:16px}
.confirm-action__buttons{display:flex;flex-direction:column;gap:8px}
.confirm-btn{display:flex;align-items:center;justify-content:center;padding:14px 18px;border-radius:10px;font-size:15px;font-weight:600;text-align:center;min-height:48px;margin:0;box-shadow:none;border:1px solid transparent;cursor:pointer;text-decoration:none}
.confirm-btn--danger{background:#b71c1c;color:#fff;border-color:#b71c1c}
.confirm-btn--danger:hover{background:#9f1818;color:#fff}
.confirm-btn--cancel{background:#fafafa;color:#111;border-color:#ddd}
.confirm-btn--cancel:hover{background:#fff;border-color:#bbb;color:#000}
.app-shell .data-busy-msg{margin:0}
.app-shell .ui-notice{margin-top:0}
.app-shell .guest-option{margin-bottom:0}
.app-shell .confirm-action .guest-option .dash-row__desc{display:block;margin-top:4px;font-weight:400}
@media screen and (min-width:520px){.confirm-action__buttons{flex-direction:row-reverse}.confirm-action__buttons .confirm-btn{flex:1}}
.app-bottom-nav{display:none;position:fixed;bottom:0;left:0;right:0;z-index:100;background:#fff;border-top:1px solid #e0e0e0;padding-bottom:env(safe-area-inset-bottom,0);box-shadow:0 -2px 12px rgba(0,0,0,.06)}
.app-tab{flex:1;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:3px;min-height:56px;min-width:44px;padding:6px 4px 5px;font-size:11px;font-weight:600;color:#777;text-align:center;background:none;margin:0;border-radius:0;box-shadow:none}
.app-tab__label{display:block;line-height:1.15;max-width:100%;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.app-tab:hover{color:#000;background:rgba(0,0,0,.03)}
body[data-page=local] .app-tab[data-tab=local],body[data-page=social] .app-tab[data-tab=social],body[data-page=custom] .app-tab[data-tab=custom],body[data-page=advanced] .app-tab[data-tab=advanced]{color:#000;font-weight:700;border-top:2px solid #007bff;margin-top:-1px}
.footer--app{margin-top:0}
.footer--app .footer-polkadot{font-size:16px;height:36px;margin-top:8px}
.app-shell.configuration .content-app.content-config{max-width:none;width:100%;margin:0;padding:32px 20px 24px 20px}
@media screen and (max-width:720px){.app-shell.configuration .content-app.content-config{width:100%;margin:24px 0 0;padding:0 12px}}
@media screen and (min-width:721px){.app-shell.configuration .content-app.content-config{max-width:none}}
@media screen and (max-width:720px){.app-bottom-nav{display:flex}body.app-shell .app-main{padding-bottom:calc(104px + env(safe-area-inset-bottom,0px))}body.app-shell .app-layout{padding-bottom:32px}.footer--app{margin-top:32px;margin-bottom:calc(72px + env(safe-area-inset-bottom,0px))}body:not(.app-shell) .content{padding-bottom:32px}}
.encrypt-key-panel{margin-top:4px;width:100%;max-width:100%;overflow:hidden}
.encrypt-key-row{display:flex;gap:16px;flex-wrap:wrap;align-items:flex-start}
.encrypt-key-qr{flex:0 0 auto}
.encrypt-key-qr-img{display:block;width:240px;max-width:100%;height:auto;background:#fff;border:1px solid #e2e8f4;border-radius:10px}
.encrypt-key-copy{flex:1 1 12rem;min-width:0;max-width:100%}
.encrypt-key-field{display:block;box-sizing:border-box;width:100%;max-width:100%;margin:0;font-family:ui-monospace,SFMono-Regular,Consolas,monospace;font-size:12px;line-height:1.45;padding:10px 12px;background:#f5f7fa;border:1px solid #e2e8f4;border-radius:10px;color:#111;resize:none;overflow-wrap:anywhere;word-break:break-all;white-space:pre-wrap;overflow-x:hidden;-webkit-text-size-adjust:100%}
.encrypt-key-field.is-revealed{background:#fff}
.encrypt-key-actions{display:flex;flex-wrap:wrap;gap:8px;margin-top:10px;align-items:center}
.encrypt-key-btn{margin:0;font-size:14px;font-weight:600;border-radius:10px;padding:10px 18px;line-height:1.2}
.encrypt-key-btn--ghost{display:inline-block;background:#fff;color:#111;border:1px solid #d0d7e2;box-shadow:none;cursor:pointer;transition:background .15s,border-color .15s}
.encrypt-key-btn--ghost:hover{background:#f5f7fa;border-color:#b8c0cc}
.encrypt-key-btn--copy{display:inline-block;min-width:0;padding:10px 20px}
.encrypt-key-copy-status{min-height:1.2em;margin:8px 0 0;font-size:12px;font-weight:600;color:#137333}
.hub-backup{display:flex;flex-direction:column;gap:16px;padding:4px 2px 2px}
.hub-backup__panel{padding:16px;border:1px solid #e2e8f4;border-radius:12px;background:#f7f9fc}
.hub-backup__panel--restore{background:#fff}
.hub-backup__title{margin:0 0 8px;font-size:15px;font-weight:600;color:#111;line-height:1.35}
.hub-backup__panel .form-hint{margin:0 0 14px}
.hub-backup__panel .encrypt-key-btn{margin:0}
.hub-backup__form{margin:0}
.hub-backup__file-field{display:flex;flex-direction:column;align-items:stretch;gap:8px;margin:0 0 12px}
.hub-backup__file-label{display:block;margin:0;font-size:14px;font-weight:600;color:#333;line-height:1.35}
.hub-backup__file-picker{display:flex;flex-wrap:wrap;align-items:center;gap:10px;margin:0 0 10px}
.hub-backup__file-input{position:absolute;width:1px;height:1px;padding:0;margin:-1px;overflow:hidden;clip:rect(0,0,0,0);white-space:nowrap;border:0}
.hub-backup__file-input--visible{position:static;width:100%;max-width:100%;height:auto;margin:0 0 12px;padding:10px 12px;overflow:visible;clip:auto;white-space:normal;border:1px solid #d0d7e2;border-radius:10px;background:#fff;font-size:14px;box-sizing:border-box}
.hub-backup__file-btn{display:inline-flex;align-items:center;justify-content:center;margin:0;padding:10px 18px;border-radius:10px;font-size:14px;font-weight:600;line-height:1.2;background:#fff;color:#111;border:1px solid #d0d7e2;box-shadow:none;cursor:pointer;transition:background .15s,border-color .15s}
.hub-backup__file-btn:hover{background:#f5f7fa;border-color:#b8c0cc}
.hub-backup__file-name{flex:1;min-width:10rem;font-size:13px;color:#666;line-height:1.35;word-break:break-all}
.hub-backup__form .confirm-action__buttons{margin-top:0}
.encrypt-backup-hint{margin-top:16px;padding:14px 16px;border:1px solid #e2e8f4;border-radius:12px;background:#f7f9fc}
.encrypt-backup-hint .form-hint{margin:0 0 12px}
.encrypt-backup-hint .encrypt-key-btn{display:inline-block;text-decoration:none}
@media screen and (min-width:720px){.hub-backup{flex-direction:row;align-items:stretch}.hub-backup__panel{flex:1;min-width:0}}
.guest-access{margin:14px 0 18px;padding:14px;background:#f7faf3;border:1px solid #d7e8b8;border-radius:12px;text-align:left}
.guest-access__hint{margin:0 0 12px}
.guest-access__row{display:flex;align-items:flex-start;gap:8px;margin:0 0 10px;flex-wrap:wrap}
.guest-access__label{flex:0 0 100%;font-size:12px;font-weight:600;color:#666;text-transform:uppercase;letter-spacing:.04em}
.guest-access__value{flex:1;min-width:0;font-size:20px;font-weight:700;color:#111;word-break:break-all;line-height:1.3}
.guest-access__value--addr{font-size:14px;font-weight:600}
.guest-access .copy-btn{flex:0 0 auto;margin-top:2px}
.guest-device-info__btn{display:block;width:100%;max-width:100%;margin:8px 0 0;box-sizing:border-box;text-align:center}
.guest-device-info__copy{display:block;width:100%;max-width:100%;margin:8px 0 0;box-sizing:border-box;text-align:center}.guest-finish-form{margin-top:16px;padding-top:16px;border-top:1px solid #eee}.guest-finish-form .guest__setup-finish-btn{margin-top:0}
.guest-access__dl-status{min-height:1.2em;margin:8px 0 0;font-size:12px;font-weight:600;color:#137333}
)rawliteral";

#endif // _CSS_STYLES_C3_H
