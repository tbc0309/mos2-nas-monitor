#pragma once
const char managementHTML[] PROGMEM = R"MOSWEB(<!doctype html>
<html lang="zh-CN">
<meta charset="utf-8" /><meta
name="viewport"
content="width=device-width,initial-scale=1"
/><title>MOS2-NAS</title>
<style>
.displayCards > .clockPalette { display:flex; flex-direction:column; }
.clockPalette > .fields { flex:1; grid-template-rows:auto 1fr; }
.clockPalette .colorPresets { grid-auto-rows:minmax(42px,1fr); row-gap:16px; margin-top:8px; }
.subGroup .colorPresets { row-gap:14px; margin-top:8px; }
.displayCards { grid-column:1/-1; display:grid; grid-template-columns:1fr 1fr; gap:16px; align-items:stretch; }
.displayCards > .settings-group { margin:0; grid-column:auto; }
.displayCards .fields { grid-template-columns:repeat(2,minmax(0,1fr)); }
.displayCards .colorPresets, .subGroup .colorPresets { grid-template-columns:repeat(3,minmax(0,1fr)); }
.deviceDetails > h3 { font-size:16px; margin-bottom:8px; }
.deviceDetails > .subGroup { grid-column:auto; margin:12px 0 0; padding:14px; background:#f7f9fb; border:1px solid #dfe7ee; border-radius:8px; align-content:start; }
.deviceDetails > .subGroup[data-snmp="3"] { grid-column:1/-1; }
.subGroup h4 { grid-column:1/-1; font-size:13px; margin:0 0 6px; }
@media(max-width:750px) { .displayCards { grid-template-columns:1fr; } .deviceDetails > .subGroup { grid-column:1/-1; } }
.snmpGroup.compactGroup { grid-template-columns:repeat(2,minmax(0,1fr)); }
.snmpGroup .wideField { grid-column:1/-1; }
@media(max-width:600px) { .snmpGroup.compactGroup { grid-template-columns:1fr; } }
.colorPresets { display:grid; grid-template-columns:repeat(5,minmax(0,1fr)); gap:8px; grid-column:1/-1; padding:0; background:transparent; border:0; }
.colorPresets strong { display:none; }
.colorPresets .paletteReset { grid-column:4; grid-row:1; padding:5px; font-size:12px; border:0; background:transparent; color:#42667b; }
@media(max-width:600px) { .colorPresets { grid-template-columns:repeat(2,minmax(0,1fr)); } .colorPresets strong { grid-column:1; } .colorPresets .paletteReset { grid-column:2; } }
.colorPresets button { display:flex; align-items:center; justify-content:center; gap:8px; width:100%; padding:10px 8px; margin:0; }
.colorSwatch { width:32px; height:20px; flex-shrink:0; border:1px solid #bcc8d2; border-radius:4px; }
* {
box-sizing: border-box;
}
body {
margin: 0;
background: #f4f6f8;
color: #25313b;
font:
14px system-ui,
-apple-system,
"Segoe UI",
sans-serif;
}
main {
max-width: 1000px;
margin: auto;
padding: 28px 20px;
}
header {
display: flex;
justify-content: space-between;
align-items: center;
margin-bottom: 24px;
}
h1 {
font-size: 24px;
margin: 0;
}
h2 {
font-size: 16px;
margin: 0 0 16px;
}
small,
.note {
color: #70808e;
font-size: 12px;
}
a {
color: #147e87;
}
section.dirty {
border-color: #d55b54;
box-shadow: 0 0 0 1px #d55b54;
}
details {
grid-column: 1/-1;
border-top: 1px solid #e7ecef;
padding: 12px 0;
}
summary {
cursor: pointer;
font-weight: 600;
}
.fields {
display: grid;
grid-template-columns: 1fr 1fr;
gap: 0 12px;
}
.fields h3,
.fields .actions,
.fields .days,
.fields p,
.fields small,
.fields > button {
grid-column: 1/-1;
}
.fields h3 {
border-top: 1px solid #e7ecef;
padding-top: 14px;
}
.fields label {
margin: 8px 0;
}
.fields h3:first-child {
border: 0;
}
fieldset {
border: 0;
padding: 0;
margin: 0;
}
h3 {
font-size: 14px;
margin: 22px 0 8px;
}
.row {
display: grid;
grid-template-columns: 1fr 1fr;
gap: 12px;
}
.row label {
margin: 8px 0;
}
section {
background: white;
border: 1px solid #dfe5ea;
border-radius: 10px;
padding: 20px;
}
form {
display: grid;
grid-template-columns: 1fr 1fr;
gap: 16px;
}
label {
display: block;
margin: 14px 0;
color: #566573;
font-size: 13px;
}
input,
select,
button {
font: inherit;
border-radius: 6px;
border: 1px solid #d3dce3;
padding: 9px 10px;
max-width: 100%;
width: 100%;
margin-top: 6px;
background: white;
color: #25313b;
}
button {
cursor: pointer;
background: #157e87;
color: white;
border-color: #157e87;
font-weight: 500;
}
button.secondary,
.actions button {
background: #f6f8fa;
color: #344653;
border-color: #dce3e8;
}
button:disabled {
opacity: 0.5;
cursor: default;
}
.wide {
grid-column: 1/-1;
}
.actions {
display: flex;
gap: 8px;
flex-wrap: wrap;
}
.actions button {
flex: 1;
min-width: 80px;
width: auto;
}
.stats {
display: grid;
grid-template-columns: repeat(4, 1fr);
gap: 12px;
margin-bottom: 16px;
}
.stats div {
background: white;
border: 1px solid #dfe5ea;
border-radius: 10px;
padding: 15px;
}
.stats b {
display: block;
font-size: 22px;
margin-top: 7px;
}
.status {
color: #147e87;
font-size: 12px;
}
.tabs {
display: flex;
gap: 8px;
margin: 16px 0;
}
.tabs button {
width: auto;
background: transparent;
color: #596977;
border-color: transparent;
margin: 0;
}
.tabs button.active {
color: #157e87;
background: #e6f1f2;
}
.hidden {
display: none !important;
}
.cropLayout {
display: grid;
grid-template-columns: 240px 1fr;
gap: 24px;
}
.cropLayout canvas {
width: 240px;
height: 240px;
background: #e6eaee;
touch-action: none;
cursor: move;
border-radius: 6px;
}
.albumGrid {
display: grid;
grid-template-columns: repeat(auto-fill, minmax(110px, 1fr));
gap: 12px;
margin-top: 20px;
}
.albumGrid img {
width: 100%;
aspect-ratio: 1;
object-fit: cover;
border-radius: 6px;
}
.albumGrid button {
background: #f6f8fa;
color: #435565;
border-color: #dce3e8;
}
.days {
display: flex;
gap: 5px;
flex-wrap: wrap;
}
.days label {
display: flex;
align-items: center;
gap: 4px;
margin: 5px 0;
}
.days input {
width: auto;
margin: 0;
}
.danger {
background: white;
color: #b84b42;
border-color: #dfb4af;
width: auto;
}
.topLanguage {
width: 100px;
margin: 0;
}
.feedback {
min-height: 18px;
color: #147e87;
}
.maintenance {
display: flex;
justify-content: space-between;
align-items: center;
}
.cropLayout input[type="range"] {
padding: 0;
}
input[type="color"] {
height: 40px;
padding: 4px;
}
#credentials:empty {
display: none;
}
@media (max-width: 650px) {
form {
grid-template-columns: 1fr;
}
.stats {
grid-template-columns: repeat(2, 1fr);
}
.cropLayout {
grid-template-columns: 1fr;
}
.tabs {
flex-wrap: wrap;
}
section {
padding: 16px;
}
}
form {
grid-template-columns: 1fr;
align-items: start;
}
.fields {
grid-template-columns: repeat(4, minmax(0, 1fr));
gap: 4px 16px;
}
.settings-group {
grid-column: span 2;
background: #f8fafb;
border: 1px solid #e7ecef;
border-radius: 8px;
padding: 16px;
margin-top: 12px;
}
.settings-group h3:first-child {
margin: 0 0 10px;
padding: 0;
}
.settings-group .row {
gap: 12px;
}
section {
padding: 22px;
}
.fields h3 {
margin-top: 16px;
}
@media (max-width: 750px) {
.fields {
grid-template-columns: 1fr 1fr;
}
.settings-group {
grid-column: 1/-1;
}
}
@media (max-width: 420px) {
.fields,
.settings-group .row {
grid-template-columns: 1fr;
}
}
.settings-group .fields {
grid-template-columns: repeat(2, minmax(0, 1fr));
gap: 4px 12px;
}
.settings-group label.wide {
grid-column: 1/-1;
}
.settings-group .days {
display: grid;
grid-template-columns: repeat(7, minmax(0, 1fr));
gap: 4px;
width: 100%;
}
.settings-group .days label {
margin: 0;
justify-content: center;
white-space: nowrap;
}
.settings-group input,
.settings-group select {
min-width: 0;
}
@media (max-width: 420px) {
.settings-group .fields {
grid-template-columns: 1fr;
}
}
.desktop-groups {
grid-column: 1/-1;
display: grid;
grid-template-columns: 1fr 1fr;
gap: 16px;
align-items: start;
margin-top: 12px;
}
.settings-stack {
display: grid;
gap: 16px;
}
.desktop-groups .settings-group {
margin: 0;
min-width: 0;
grid-column: auto;
}
.albumHeader {
display: flex;
justify-content: space-between;
align-items: center;
gap: 12px;
}
.albumHeader h2,
.albumHeader p {
margin: 0;
}
#tab-photos > .note {
margin: 10px 0 20px;
}
.cropLayout {
grid-template-columns: 264px minmax(0, 1fr);
gap: 24px;
align-items: start;
}
.cropPreview {
padding: 11px;
background: #f8fafb;
border: 1px solid #e7ecef;
border-radius: 10px;
}
.cropLayout canvas {
display: block;
}
.cropControls #choosePhoto {
margin: 0;
}
.cropControls label {
margin: 14px 0;
}
.cropControls .actions {
margin-top: 10px;
}
#photoMessage:empty {
display: none;
}
.albumGrid {
grid-template-columns: repeat(auto-fill, minmax(120px, 140px));
gap: 16px;
border-top: 1px solid #e7ecef;
padding-top: 20px;
margin-top: 24px;
}
.albumGrid > div {
padding: 8px;
border: 1px solid #e7ecef;
border-radius: 9px;
background: #f8fafb;
}
.albumGrid img {
display: block;
}
.albumGrid small {
display: block;
margin-top: 8px;
}
.albumGrid button {
padding: 7px 10px;
}
@media (max-width: 750px) {
.desktop-groups {
grid-template-columns: 1fr;
}
}
@media (max-width: 650px) {
.cropLayout {
grid-template-columns: 1fr;
}
.cropPreview {
width: 264px;
max-width: 100%;
margin: auto;
}
.cropLayout canvas {
max-width: 100%;
height: auto;
}
.albumGrid {
grid-template-columns: repeat(auto-fill, minmax(110px, 1fr));
}
}
.desktop-groups {
align-items: stretch;
}
.desktop-groups > .settings-group {
height: 100%;
}
.connection-columns {
grid-column: 1/-1;
display: grid;
grid-template-columns: 1fr;
gap: 16px;
align-items: stretch;
}
.connection-columns .fields {
grid-template-columns: repeat(4, minmax(0, 1fr));
}
@media (max-width: 750px) {
.connection-columns .fields {
grid-template-columns: repeat(2, minmax(0, 1fr));
}
}
@media (max-width: 420px) {
.connection-columns .fields {
grid-template-columns: 1fr;
}
}
@media (max-width: 750px) {
.connection-columns {
grid-template-columns: 1fr;
}
.desktop-groups > .settings-group {
height: auto;
}
}
.tabs {
align-items: center;
}
.settingsActions {
margin-left: auto;
display: flex;
align-items: center;
gap: 12px;
min-width: 0;
}
.settingsActions .feedback {
margin: 0;
text-align: right;
}
.tabs .settingsActions button {
background: #157e87;
color: white;
border-color: #157e87;
white-space: nowrap;
}
@media (max-width: 650px) {
.settingsActions {
width: 100%;
justify-content: flex-end;
}
.settingsActions .feedback {
flex: 1;
}
}
.upgradeKeep {
display: flex;
align-items: center;
gap: 8px;
}
.upgradeKeep input {
width: auto;
margin: 0;
}
.projectLinks {
display: flex;
justify-content: center;
align-items: center;
flex-wrap: wrap;
gap: 12px;
margin-top: 24px;
font-size: 12px;
color: #70808e;
}
.projectLinks a {
color: #647582;
text-decoration: none;
}
.projectLinks a:hover {
color: #147e87;
text-decoration: underline;
}
.projectLinks a[data-i18n="Sign out"] {
color: #c43d36;
}
.projectLinks a[data-i18n="Sign out"]:hover {
color: #a52c26;
}
.access-columns {
display: grid;
grid-template-columns: 1fr 1fr;
gap: 16px;
}
.access-columns .fields {
grid-template-columns: repeat(2, minmax(0, 1fr));
}
.access-columns .wide {
grid-column: 1/-1;
}
@media (max-width: 750px) {
.access-columns {
grid-template-columns: 1fr;
}
}
@media (max-width: 420px) {
.access-columns .fields {
grid-template-columns: 1fr;
}
}
.githubProject {
display: inline-flex;
align-items: center;
gap: 5px;
}
.githubProject svg {
flex: none;
}
.fields > label {
min-width: 0;
}
.fields > label {
line-height: 18px;
}
.fields > label > span:first-child {
display: inline;
}
.fields input,
.fields select {
min-width: 0;
}
#privacyPasswordField > label {
margin: 8px 0;
}
@media (max-width: 420px) {
.settings-group .days label {
flex-direction: column;
gap: 4px;
}
}
.field-title {
display: block !important;
white-space: nowrap;
}
.fields > label input,
.fields > label select {
display: block;
}
.fields > label > span:first-child {
vertical-align: top;
}
@media (min-width: 751px) {
.connection-columns > section > .fields {
grid-template-columns: repeat(3, minmax(0, 1fr));
}
}
@media (min-width: 421px) {
.fields > label {
display: flex;
flex-direction: column;
}
.fields > label > span:first-child {
min-height: 18px;
}
}
.fields > label:has(#cityCodeLookup) {
display: block;
}
.machineList {
display: grid;
grid-template-columns: 1fr 1fr;
gap: 8px;
margin-bottom: 12px;
}
.machineItem {
display: flex;
align-items: center;
gap: 8px;
padding: 10px;
border: 1px solid #dce3e8;
border-radius: 8px;
}
.machineItem.active {
border-color: #157e87;
background: #e6f1f2;
}
.machineItem button {
width: auto;
margin: 0;
}
.machineItem .choose {
flex: 1;
text-align: left;
background: transparent;
color: #25313b;
border: 0;
}
.machineItem small {
display: block;
}
.machineItem .move {
padding: 5px 8px;
background: white;
color: #435565;
border-color: #dce3e8;
}
@media (max-width: 650px) {
.machineList {
grid-template-columns: 1fr;
}
}
.snmpGroup {
grid-column: 1/-1;
display: grid;
grid-template-columns: repeat(4, minmax(0, 1fr));
gap: 4px 16px;
border-top: 1px solid #e7ecef;
padding-top: 12px;
margin-top: 12px;
}
.snmpGroup h3 {
grid-column: 1/-1;
margin: 0;
}
.snmpGroup label {
min-width: 0;
display: flex;
flex-direction: column;
}
.snmpGroup input,
.snmpGroup select {
min-width: 0;
}
.snmpGroup #privacyPasswordField {
grid-column: auto;
}
@media (max-width: 750px) {
.snmpGroup {
grid-template-columns: repeat(2, minmax(0, 1fr));
}
}
@media (max-width: 420px) {
.snmpGroup {
grid-template-columns: 1fr;
}
}
</style>
<main>
<header>
<div>
<h1>MOS2-NAS</h1>
<small>SNMP · v1.2.0</small>
</div>
<div>
<span id="state" class="status"></span>
<select id="language" class="topLanguage">
<option value="zh">中文</option>
<option value="en">English</option>
</select>
</div>
</header>
<div class="stats">
<div>
<small><span data-i18n="CPU">CPU</span></small
><b id="cpu">--</b>
</div>
<div>
<small><span data-i18n="Memory">Memory</span></small
><b id="memory">--</b>
</div>
<div>
<small><span data-i18n="NAS temperature">NAS temperature</span></small
><b id="temperature">--</b>
</div>
<div>
<small><span data-i18n="Receive / send">Receive / send</span></small
><b id="traffic">--</b>
</div>
</div>
<nav class="tabs">
<button type="button" data-tab="screen" class="active">
<span data-i18n="Screen">Screen</span></button
><button type="button" data-tab="settings" class="">
<span data-i18n="Settings">Settings</span></button
><button type="button" data-tab="photos" class="">
<span data-i18n="Photos">Photos</span></button
><button type="button" data-tab="maintenance" class="">
<span data-i18n="Maintenance">Maintenance</span>
</button>
<div id="settingsActions" class="settingsActions hidden">
<p id="message" class="feedback" role="status" aria-live="polite"></p>
<button type="submit" form="settings">
<span id="saveActionLabel" data-i18n="Save & restart">Save & restart</span>
</button>
</div>
</nav>
<section id="tab-screen">
<h2><span data-i18n="Desktop">Desktop</span></h2>
<div class="actions">
<button type="button" onclick="setPage(6)">
<span data-i18n="Clock">Clock</span></button
><button type="button" id="weatherPage" onclick="setPage(5)">
<span data-i18n="Weather">Weather</span></button
><button type="button" onclick="setPage(7)">
<span data-i18n="Photos">Photos</span>
</button>
</div>
<h2 style="margin-top: 24px">SNMP</h2>
<div class="actions">
<button type="button" onclick="setPage(0)">
<span data-i18n="NAS overview">NAS overview</span></button
><button type="button" onclick="setPage(1)">
<span data-i18n="Network">Network</span></button
><button type="button" onclick="setPage(2)">
<span data-i18n="CPU / memory">CPU / memory</span></button
><button type="button" onclick="setPage(8)">
<span data-i18n="Disk health">Disk health</span></button
><button type="button" onclick="setPage(9)">
<span data-i18n="Volumes">Volumes</span></button
><button type="button" onclick="setPage(4)">
<span data-i18n="Status">Status</span>
</button>
</div>
<div class="actions">
<button type="button" id="switchMonitor"><span id="details">SNMP</span> ↻</button>
<button
type="button"
class="secondary"
id="rediscoverStorage"
data-i18n="Rediscover storage"
>
Rediscover storage
</button>
</div>
<p id="storageFeedback" role="status" class="note"></p>
<small
><span
data-i18n="Tap: next · Double: photos / disk groups · Triple: mode · Five: management · Six: Wi-Fi"
>Tap: next · Double: photos / disk groups · Triple: mode · Five:
management · Six: Wi-Fi</span
></small
>
</section>
<form id="settings" class="hidden" method="post" action="/save">
<input type="hidden" name="token" value="{{token}}" /><input
type="hidden"
name="machineIndex"
/><input type="hidden" name="alarmDays" value="31" />
<section>
<h2>
<span data-i18n="Desktop &amp; display">Desktop &amp; display</span>
</h2>
<div class="fields">
<label
><span data-i18n="Display language">Display language</span
><select name="language">
<option value="zh">中文</option>
<option value="en">English</option>
</select></label
><label
><span data-i18n="Start page">Start page</span
><select name="home">
<option value="6" data-i18n="Clock">大字时钟</option>
<option value="5" data-i18n="Weather">天气</option>
<option value="7" data-i18n="Photos">相册</option>
<option value="0" data-i18n="NAS overview">NAS 总览</option>
<option value="1" data-i18n="Network">网络</option>
<option value="2" data-i18n="CPU / memory">CPU / 内存</option>
<option value="8" data-i18n="Disk health">硬盘健康</option>
<option value="9" data-i18n="Volumes">存储卷</option>
<option value="4" data-i18n="Status">设备状态</option>
</select></label
><label
><span data-i18n="Hour color">Hour color</span
><input name="clockColor" type="color" /><button
type="button"
class="secondary"
onclick="resetColors('clock')"
data-i18n="Default clock colors"
>
恢复时钟默认颜色
</button></label
><label
><span data-i18n="Minute color">Minute color</span
><input name="minuteColor" type="color" /></label
><label
><span data-i18n="Show seconds">Show seconds</span
><select name="showSeconds">
<option value="1" data-i18n="On">开启</option>
<option value="0" data-i18n="Off">关闭</option>
</select></label
><label
><span data-i18n="Brightness / %">Brightness / %</span
><input name="brightness" type="number" min="5" max="100" /></label
><label
><span data-i18n="Orientation">Orientation</span
><select name="rotation">
<option value="0" data-i18n="0°">0°</option>
<option value="1" data-i18n="90°">90°</option>
<option value="2" data-i18n="180°">180°</option>
<option value="3" data-i18n="270°">270°</option>
</select></label
><label
><span data-i18n="Touch sound">Touch sound</span
><select name="sound">
<option value="0" data-i18n="Off">关闭</option>
<option value="1" data-i18n="On">开启</option></select
><button type="button" class="secondary" onclick="testBeep()">
<span data-i18n="Test sound">Test sound</span>
</button></label
>
<div class="desktop-groups">
<div class="settings-stack">
<div class="settings-group">
<h3>
<span data-i18n="Weather &amp; time">Weather &amp; time</span>
</h3>
<div class="fields">
<label
><span data-i18n="Weather">Weather</span
><select name="weatherEnabled">
<option value="0" data-i18n="Off">关闭</option>
<option value="1" data-i18n="On">开启</option>
</select></label
><label><span data-i18n="Province">Province</span><select id="weatherProvince" disabled><option>—</option></select></label>
<label><span data-i18n="City">City</span><select id="weatherCity" disabled><option>—</option></select></label>
<label
><span class="field-title"
><span data-i18n="City code">City code</span> ·
<a
id="cityCodeLookup"
href="https://www.weather.com.cn/"
target="_blank"
rel="noopener noreferrer"
data-i18n="Look up"
>查询</a
></span
><input
name="cityCode"
type="text"
pattern="10[12][0-9]{6}"
maxlength="9"
required /></label
><label
><span data-i18n="Weather refresh / minutes"
>Refresh / minutes</span
><input
name="weatherInterval"
type="number"
min="10"
max="120"
required /></label
><label
><span data-i18n="Time server">Time server</span
><input
name="ntpServer"
type="text"
maxlength="63"
required
/></label>
<p id="weatherDetails" class="note"></p>
</div>
</div>
<div class="settings-group">
<div class="fields">
<h3>
<span data-i18n="Photo playback">Photo playback</span>
</h3>
<label
><span data-i18n="Playback">Playback</span
><select name="albumFixed">
<option value="0" data-i18n="Slideshow">自动轮播</option>
<option
value="1"
data-i18n="Fixed (double tap: next photo)"
>
固定显示（双击换照片）
</option>
</select></label
><label
><span data-i18n="Slide duration / seconds"
>Duration / seconds</span
><input
name="albumInterval"
type="number"
min="3"
max="300"
required /></label
><small
><span data-i18n="Up to 7 photos, 48KB each"
>Up to 7 photos, 48KB each</span
></small
>
</div>
</div>
</div>
<div class="settings-group">
<h3><span data-i18n="Alarm">Alarm</span></h3>
<div class="fields">
<label
><span data-i18n="Alarm">Alarm</span
><select name="alarmEnabled">
<option value="0" data-i18n="Off">关闭</option>
<option value="1" data-i18n="On">开启</option>
</select></label
><label
><span data-i18n="Time">Time</span
><input name="alarmTime" type="time" required /></label
><label class="wide"
><span data-i18n="Repeat">Repeat</span></label
>
<div class="days">
<label
><input type="checkbox" class="day" value="1" /><span
data-i18n="Mon"
>Mon</span
></label
><label
><input type="checkbox" class="day" value="2" /><span
data-i18n="Tue"
>Tue</span
></label
><label
><input type="checkbox" class="day" value="4" /><span
data-i18n="Wed"
>Wed</span
></label
><label
><input type="checkbox" class="day" value="8" /><span
data-i18n="Thu"
>Thu</span
></label
><label
><input type="checkbox" class="day" value="16" /><span
data-i18n="Fri"
>Fri</span
></label
><label
><input type="checkbox" class="day" value="32" /><span
data-i18n="Sat"
>Sat</span
></label
><label
><input type="checkbox" class="day" value="64" /><span
data-i18n="Sun"
>Sun</span
></label
>
</div>
<label
><span data-i18n="8-bit melody">8-bit melody</span
><select name="alarmTune">
<option value="0" data-i18n="Morning">清晨</option>
<option value="1" data-i18n="Arcade">街机</option>
<option value="2" data-i18n="Starlight">星光</option>
<option value="3" data-i18n="Journey">启程</option>
</select></label
><label
><span data-i18n="Snooze / minutes">Snooze / minutes</span
><input name="snooze" type="number" min="1" max="30" required
/></label>
<div class="actions">
<button type="button" onclick="alarmAction('test')">
<span data-i18n="Test alarm">Test alarm</span></button
><button type="button" onclick="alarmAction('tap')">
<span data-i18n="Simulate tap">Simulate tap</span>
</button>
</div>
<small
><span data-i18n="Tap to snooze; three taps dismiss today"
>Tap to snooze; three taps dismiss today</span
></small
>
<p id="alarmDetails" class="note"></p>
</div>
</div>
</div>
</div>
</section>
<div class="connection-columns">
<section>
<h2 data-i18n="SNMP settings">SNMP 配置</h2>
<div class="actions">
<select id="machineSelect" aria-label="Device"></select
><button type="button" id="addMachine" data-i18n="Add device">
Add device</button
><button
type="button"
id="deleteMachine"
class="danger"
data-i18n="Delete device"
>
Delete device
</button>
</div>
<p class="note" data-i18n="Finish device list edits, then save and restart.">Finish device list edits, then save and restart.</p>
<div class="fields">
<label
><span data-i18n="Device name">Device name</span
><input name="machineName" maxlength="48" /><small data-i18n="Blank: automatic name; filled: custom name">Blank: automatic name; filled: custom name</small></label
><label
><span data-i18n="NAS background">NAS background</span
><input name="nasBackground" type="color" /><button
type="button"
class="secondary"
onclick="resetColors('nas')"
data-i18n="Default NAS color"
>
恢复默认底色
</button></label
><label
><span data-i18n="Card color">Card color</span
><input
type="color"
name="cardBackground"
value="#103040" /></label
><label
><span data-i18n="Address">Address</span
><input name="nas" type="text" required /></label
><label
><span data-i18n="SNMP version">SNMP version</span
><select name="snmpVersion">
<option value="2c">SNMP v2c</option>
<option value="3">SNMP v3</option>
</select></label
><label data-snmp="2c"
><span data-i18n="Community (blank to keep)"
>Community (blank to keep)</span
><input
name="community"
type="password"
autocomplete="off" /></label
><label data-snmp="3"
><span data-i18n="SNMP username">Username</span
><input
name="snmpUser"
type="text"
maxlength="32"
autocomplete="off" /></label
><label data-snmp="3"
><span data-i18n="Authentication">Auth protocol</span
><select name="snmpAuth">
<option value="MD5">MD5</option>
<option value="SHA">SHA</option>
</select></label
><label data-snmp="3"
><span data-i18n="Authentication password (blank to keep)"
>Auth password</span
><input
name="snmpPassword"
type="password"
maxlength="64"
autocomplete="new-password" /></label
><label data-snmp="3"
><span data-i18n="Privacy encryption">Privacy protocol</span
><select name="snmpPrivacy">
<option value="none" data-i18n="None">不加密</option>
<option value="DES">DES</option>
<option value="AES">AES</option>
</select></label
><label data-snmp="3" id="privacyPasswordField"
><span data-i18n="Privacy password (blank to keep)"
>Privacy password</span
><input
name="snmpPrivacyPassword"
type="password"
autocomplete="new-password"
maxlength="64" /></label
><label
><span data-i18n="Port">Port</span
><input name="port" type="number" min="1" max="65535" /></label
><label
><span data-i18n="Network interface">Network interface</span
><select name="iface">
<option value="" data-i18n="Automatic">自动</option>
</select></label
><input type="hidden" name="storage" /><label
><span data-i18n="Refresh interval / seconds"
>Refresh / seconds</span
><input name="interval" type="number" min="3" max="60"
/></label>
</div>
</section>
<div class="access-columns">
<section>
<h2><span data-i18n="Wi-Fi connection">Wi-Fi connection</span></h2>
<p id="wifiCurrent" class="note" aria-live="polite"></p>
<div class="fields">
<label class="wide"
><span data-i18n="Nearby Wi-Fi">Nearby Wi-Fi</span
><select id="manageNetworks">
<option value=""></option></select></label
><button type="button" class="secondary" id="manageRescan">
<span data-i18n="Scan Wi-Fi">Scan Wi-Fi</span></button
><small id="manageScanStatus" role="status"></small
><label
><span data-i18n="Wi-Fi name">Wi-Fi name</span
><input
name="ssid"
type="text"
maxlength="32"
required /></label
><label
><span data-i18n="Wi-Fi password (blank to keep)"
>Wi-Fi password</span
><input name="pass" type="password" autocomplete="new-password"
/></label>
</div>
</section>
<section>
<h2>
<span data-i18n="Management password">Management password</span>
</h2>
<div class="fields">
<label class="wide"
><span data-i18n="Admin password (blank to keep)"
>Admin password</span
><input
name="admin"
type="password"
autocomplete="new-password"
/></label>
<p id="credentials">{{credentials}}</p>
</div>
</section>
</div>
</div>
</form>
<section id="tab-photos" class="hidden">
<div class="albumHeader">
<h2><span data-i18n="Photos">Photos</span></h2>
<p id="photoCount" class="status"></p>
</div>
<p class="note">
<span data-i18n="Drag to crop, zoom or rotate."
>Drag to crop, zoom or rotate.</span
>
</p>
<input id="photoFile" type="file" accept="image/*" hidden />
<div class="cropLayout">
<div class="cropPreview">
<canvas
id="crop"
width="240"
height="240"
aria-label="照片裁剪预览"
></canvas>
</div>
<div class="cropControls">
<button
type="button"
id="choosePhoto"
class="secondary"
onclick="document.getElementById('photoFile').click()"
>
<span data-i18n="Choose photo">Choose photo</span></button
><label
><span data-i18n="Zoom">Zoom</span
><input
id="zoom"
type="range"
min="1"
max="4"
step="0.01"
value="1"
/></label>
<div class="actions">
<button type="button" onclick="rotatePhoto()">
<span data-i18n="Rotate 90°">Rotate 90°</span></button
><button type="button" onclick="resetCrop()">
<span data-i18n="Reset crop">Reset crop</span>
</button>
</div>
<label
><span data-i18n="Upload destination">Upload destination</span
><select id="photoSlot"></select></label
><button
id="uploadPhoto"
type="button"
onclick="uploadPhoto()"
disabled
>
<span data-i18n="Crop &amp; upload">Crop &amp; upload</span>
</button>
<p id="photoMessage" role="status"></p>
</div>
</div>
<div id="album" class="albumGrid"></div>
</section>
<section id="tab-maintenance" class="hidden">
<h2><span data-i18n="Maintenance">Maintenance</span></h2>
<h3 data-i18n="Firmware upgrade">固件升级</h3>
<div class="row">
<label
><span data-i18n="Firmware file (.bin)">Firmware file (.bin)</span
><input id="firmwareFile" type="file" accept=".bin"
/></label>
<div>
<label class="upgradeKeep"
><input id="firmwareKeep" type="checkbox" checked /><span
data-i18n="Keep settings and photos"
>Keep settings and photos</span
></label
><button
id="firmwareUpgrade"
type="button"
onclick="upgradeFirmware()"
>
<span data-i18n="Upload & upgrade">Upload &amp; upgrade</span>
</button>
</div>
</div>
<p
class="note"
data-i18n="Upload the program firmware only. System resources are retained."
>
仅上传程序固件，系统资源始终保留。
</p>
<progress
id="firmwareProgress"
class="hidden"
max="100"
value="0"
style="width: 100%"
></progress>
<p id="firmwareMessage" role="status" aria-live="polite"></p>
<hr style="border: 0; border-top: 1px solid #e7ecef; margin: 24px 0" />
<div class="maintenance">
<small
><span data-i18n="Factory reset deletes all settings and photos."
>Factory reset deletes all settings and photos.</span
></small
><button type="button" class="danger" onclick="resetSettings()">
<span data-i18n="Reset device">Reset device</span>
</button>
</div>
<p class="note">
<span
data-i18n="Eight taps request reset; three more confirm. Expires after 10 seconds."
>Eight taps request reset; three more confirm. Expires after 10
seconds.</span
>
</p>
</section>
<footer class="projectLinks">
<a href="https://imnks.com/" target="_blank" rel="noopener noreferrer"
>我不是矿神</a
><span aria-hidden="true">·</span
><a
href="https://jsrun.net/hw/mos2"
target="_blank"
rel="noopener noreferrer"
data-i18n="MOS2 official"
>MOS2官方</a
><span aria-hidden="true">·</span
><a
href="https://github.com/tbc0309/mos2-nas-monitor"
target="_blank"
rel="noopener noreferrer"
class="githubProject"
><svg
width="16"
height="16"
viewBox="0 0 24 24"
fill="currentColor"
aria-hidden="true"
>
<path
d="M12 .75a11.25 11.25 0 0 0-3.56 21.92c.56.1.77-.24.77-.54v-2.1c-3.13.68-3.79-1.33-3.79-1.33-.51-1.3-1.25-1.65-1.25-1.65-1.02-.7.08-.69.08-.69 1.13.08 1.72 1.16 1.72 1.16 1 1.72 2.63 1.22 3.27.93.1-.73.39-1.22.71-1.5-2.5-.28-5.13-1.25-5.13-5.56 0-1.23.44-2.23 1.16-3.02-.12-.28-.5-1.43.11-2.98 0 0 .94-.3 3.1 1.15a10.8 10.8 0 0 1 5.62 0c2.16-1.46 3.1-1.15 3.1-1.15.61 1.55.23 2.7.11 2.98.72.79 1.16 1.8 1.16 3.02 0 4.32-2.63 5.27-5.14 5.55.4.35.76 1.03.76 2.08v3.1c0 .3.2.65.77.54A11.25 11.25 0 0 0 12 .75Z"
/></svg
><span data-i18n="Project">MOS2-NAS project</span></a
><span aria-hidden="true">·</span
><a href="/logout" onclick="signOut(event)" data-i18n="Sign out"
>退出登录</a
>
</footer>
</main>
<script>
const config={{config}};
</script>
<script>
// ESP8266 serves one HTTP client at a time. Avoid speculative parallel downloads.
const scripts = ["/i18n.js?v=d5cb9cf32d5d8e7d", "/firmware-bundle.js?v=01f53c4ad106a1d2", "/management.js?v=cb49036793af0bb4"];
function loadNextScript() {
const path = scripts.shift();
if (!path) return;
const script = document.createElement("script");
script.src = path;
script.onload = loadNextScript;
script.onerror = () => { document.getElementById("state").textContent = "Load failed / 加载失败，请刷新"; };
document.body.append(script);
}
loadNextScript();
</script>
</html>)MOSWEB";
struct WebSegment {uint32_t offset,length;uint8_t key;};
const WebSegment webSegments[] PROGMEM={
{0,15991,1},
{16000,10453,3},
{26468,4223,2},
{30701,593,0},
};
const char accessLocaleJS[] PROGMEM = R"MOSJS(// Generated from locales/access/*.json.
const AccessI18n={"en":{"network":"Wi-Fi network","ssid":"Network name","password":"Password","scan":"Refresh","select":"Select network","scanning":"Searching for networks…","empty":"No networks found. Enter the name manually.","scanFailed":"Scan failed. Refresh or enter the name.","hint":"2.4GHz only. Enter hidden networks manually.","saving":"Saving…","saved":"Network saved","networkSaved":"Connecting to: ","step1":"Join the selected Wi-Fi on your phone.","step2":"Once connected, tap the device five times.","step3":"Scan the screen QR code to open management.","retry":"If connection fails, rejoin the device hotspot after about 30 seconds.","invalid":"Check network name and password","saveFailed":"Save failed. Try again.","wrong":"Incorrect password","wait":"Try again shortly","expired":"Page expired. Refresh to continue.","failed":"Save did not complete. Please retry.","open":"Open","setupTitle":"Connect to Wi-Fi","loginTitle":"Sign in","setupSubmit":"Save & connect","loginSubmit":"Sign in","changeTitle":"Change default password","changeHint":"Set a new management password, then sign in again.","newPassword":"New password","confirmPassword":"Confirm password","changeSubmit":"Save password","passwordInvalid":"Use at least 8 characters, different from the default. Both entries must match.","passwordChanged":"Password changed. Sign in again."},"zh":{"network":"Wi-Fi 网络","ssid":"网络名称","password":"密码","scan":"刷新","select":"选择网络","scanning":"正在搜索附近网络…","empty":"未找到网络，可手动输入名称","scanFailed":"搜索失败，请刷新或手动输入","hint":"仅支持 2.4GHz，隐藏网络可手动填写。","saving":"正在保存…","saved":"网络已保存","networkSaved":"即将连接：","step1":"手机连接刚设置的 Wi-Fi。","step2":"联网完成后，连击MOS2五次。","step3":"扫描屏幕二维码，打开管理页。","retry":"若连接失败，约30秒后重新加入设备热点重试。","invalid":"请检查网络名称和密码","saveFailed":"保存失败，请重试","wrong":"密码错误","wait":"请稍后重试","expired":"页面已过期，请刷新","failed":"保存未完成，请重试","open":"开放网络","setupTitle":"连接 Wi-Fi","loginTitle":"管理登录","setupSubmit":"保存并连接","loginSubmit":"登录","changeTitle":"修改默认密码","changeHint":"请先设置新管理密码，再重新登录。","newPassword":"新密码","confirmPassword":"确认密码","changeSubmit":"保存密码","passwordInvalid":"密码至少 8 字符，不能使用默认密码，且两次输入需一致。","passwordChanged":"密码已修改，请重新登录。"}};
)MOSJS";
const char accessLocaleJSGzip[] PROGMEM = {31,139,8,0,0,0,0,0,2,255,109,84,91,83,20,71,20,126,247,87,116,237,67,30,82,100,81,200,131,69,42,15,9,21,149,84,69,137,107,42,207,237,76,239,206,232,108,247,214,204,44,4,45,170,22,21,118,17,16,208,69,133,90,47,40,32,137,178,139,9,9,55,97,255,139,217,158,153,125,202,95,200,233,233,185,186,62,205,116,159,219,119,190,115,250,235,239,71,23,9,37,38,182,137,138,242,38,43,34,131,41,216,32,86,63,86,20,98,89,253,95,102,111,88,140,102,207,40,140,90,54,250,206,191,28,57,119,158,126,123,59,67,104,102,232,118,134,18,123,156,153,55,51,67,153,95,245,175,46,232,40,60,247,101,44,75,87,225,250,178,188,64,20,23,9,220,150,176,101,193,89,88,70,195,95,240,85,48,100,203,92,37,121,147,88,154,184,32,6,81,108,184,202,249,63,201,180,224,74,117,90,240,109,216,84,52,248,71,121,102,134,46,214,199,202,22,184,145,98,201,158,16,229,89,100,0,175,50,85,179,232,7,106,19,19,217,26,241,49,161,34,166,101,108,24,19,217,32,249,5,172,27,68,224,203,193,1,229,253,83,22,5,208,16,20,34,169,120,17,6,24,4,214,129,236,215,23,47,221,66,140,66,178,160,138,166,171,42,161,49,132,84,49,60,22,244,225,255,72,220,112,73,146,180,201,115,95,200,115,46,48,15,51,74,129,23,209,187,205,134,144,8,180,73,233,28,88,126,100,58,245,193,73,10,97,174,114,48,140,162,9,86,54,81,73,99,212,7,45,2,6,32,224,10,85,8,82,100,62,162,246,33,27,151,252,120,149,140,233,96,201,235,99,4,217,122,145,88,97,208,96,200,141,95,69,49,9,52,248,243,85,72,161,130,35,67,172,4,103,232,19,23,72,17,168,18,81,38,177,77,49,140,145,124,88,72,103,146,90,171,15,153,228,70,8,57,40,169,49,219,42,49,27,225,188,160,16,95,103,101,27,13,158,133,134,32,88,245,97,232,116,12,27,254,122,13,107,68,185,25,242,43,7,138,169,138,74,137,221,2,202,226,153,194,33,154,233,53,115,2,225,2,214,169,72,57,110,50,127,24,35,84,97,166,41,118,46,145,99,28,235,98,192,81,0,178,52,102,218,198,132,88,180,223,74,186,73,228,62,23,8,10,142,241,194,0,35,0,27,38,85,246,89,207,167,144,168,186,138,40,116,170,176,98,201,32,54,201,162,81,131,96,139,32,159,49,225,47,216,20,67,18,31,241,44,236,114,233,154,110,27,36,222,1,81,193,31,49,216,13,86,208,105,104,207,233,5,138,244,40,44,87,190,94,244,187,240,43,127,17,78,34,140,138,205,81,152,162,97,90,32,81,57,255,4,51,202,227,178,145,98,71,250,93,146,143,32,71,96,112,48,144,241,196,14,68,206,125,98,204,192,158,44,17,147,15,238,163,177,48,92,134,224,100,122,70,243,186,89,76,56,12,203,155,94,12,233,30,19,230,240,119,36,90,156,95,128,101,108,35,65,183,141,206,35,136,55,49,60,0,19,54,82,213,243,121,98,10,220,190,36,202,205,244,187,206,162,239,153,173,9,5,48,117,2,175,185,12,177,69,108,43,90,54,81,67,18,149,84,56,36,209,193,86,228,210,157,79,246,101,110,105,159,147,81,247,100,217,61,110,196,42,42,207,124,105,193,125,179,155,86,81,222,154,113,95,78,197,26,202,107,251,206,227,221,164,132,118,43,179,206,220,239,113,198,88,64,157,157,215,188,177,237,44,53,220,189,87,221,213,123,94,123,89,122,165,37,212,105,252,225,204,158,242,218,174,52,254,247,97,158,47,182,156,217,57,126,127,219,59,125,196,167,55,35,88,41,249,148,105,249,198,123,111,111,19,98,188,214,190,132,230,212,30,39,131,99,249,236,28,79,59,245,150,51,63,133,164,144,66,80,119,109,201,123,178,24,52,31,22,229,235,111,249,204,234,191,149,59,73,13,149,173,116,218,207,248,206,211,180,146,6,209,251,127,74,99,175,150,242,133,191,248,238,140,215,126,238,60,0,160,107,9,45,133,122,78,227,72,90,120,109,205,107,158,186,39,77,119,237,158,124,111,1,130,64,70,189,169,58,84,226,205,121,167,182,196,151,30,136,142,219,207,121,245,248,167,43,185,129,206,81,221,121,183,30,251,15,250,185,223,58,139,139,252,253,34,63,92,233,28,205,187,199,123,48,70,136,114,102,31,241,15,21,183,185,238,46,205,116,215,255,150,65,161,128,122,115,155,1,154,144,86,247,104,107,240,172,251,230,33,148,236,86,23,128,94,126,255,37,176,10,80,249,70,213,189,187,227,222,57,132,123,175,181,34,19,197,194,9,243,112,94,87,156,23,155,201,213,226,15,231,227,125,74,234,166,228,46,57,76,153,53,33,156,50,176,91,95,245,90,173,88,50,193,211,221,94,144,232,164,127,44,152,208,94,247,217,43,152,140,215,174,58,141,23,201,37,73,42,165,172,13,91,40,201,253,164,124,32,144,192,153,83,63,141,151,60,41,148,146,178,207,139,164,36,218,93,61,230,39,43,61,74,25,116,125,248,143,204,208,35,149,81,88,90,41,59,237,166,83,63,236,30,63,245,154,27,17,157,41,145,20,125,78,215,228,62,65,183,18,132,116,21,207,107,102,65,206,82,230,151,147,75,75,164,24,116,148,185,71,31,221,245,102,111,233,79,187,10,173,189,210,40,77,94,21,158,197,50,72,35,223,121,226,190,219,2,92,157,131,5,239,238,73,231,164,237,214,183,147,221,249,166,122,231,96,3,150,92,62,234,110,163,210,57,168,120,213,61,137,189,87,25,101,160,255,40,5,87,209,76,83,77,79,78,126,115,230,127,239,240,201,174,162,10,0,0};
const char localeJS[] PROGMEM = R"MOSJS(const I18n={locales:{"en":{"CPU":"CPU","Memory":"Memory","NAS temperature":"Device temperature","Receive / send":"Receive / send","Screen":"Screen","Settings":"Settings","Photos":"Photos","Maintenance":"Maintenance","Desktop":"Desktop","Clock":"Clock","Weather":"Weather","NAS overview":"Overview","Network":"Network","CPU / memory":"CPU / memory","Disk health":"Disk health","Volumes":"Volumes","Status":"Status","Tap: next · Double: photos / disk groups · Triple: mode · Five: management · Six: Wi-Fi":"Tap: next · Double: photos / disk groups · Triple: mode · Five: management · Six: Wi-Fi","Desktop & display":"Desktop & display","Display language":"Display language","Start page":"Start page","Hour color":"Hour color","Minute color":"Minute color","Brightness / %":"Brightness / %","Orientation":"Orientation","0°":"0°","90°":"90°","180°":"180°","270°":"270°","Touch sound":"Touch sound","Off":"Off","On":"On","Test sound":"Test sound","Weather & time":"Weather & time","Weather refresh / minutes":"Refresh / minutes","Time server":"Time server","Alarm":"Alarm","Time":"Time","Repeat":"Repeat","Mon":"Mon","Tue":"Tue","Wed":"Wed","Thu":"Thu","Fri":"Fri","Sat":"Sat","Sun":"Sun","8-bit melody":"8-bit melody","Morning":"Morning","Arcade":"Arcade","Starlight":"Starlight","Journey":"Journey","Snooze / minutes":"Snooze / minutes","Test alarm":"Test alarm","Simulate tap":"Simulate tap","Tap to snooze; three taps dismiss today":"Rings up to 5 min; one snooze; triple tap stops","Photo playback":"Photo playback","Playback":"Playback","Slideshow":"Slideshow","Fixed (double tap: next photo)":"Fixed (double tap: next photo)","Slide duration / seconds":"Duration / seconds","Up to 7 photos, 48KB each":"Up to 7 photos, 48KB each","Address":"Address","Community (blank to keep)":"Community (blank to keep)","Port":"Port","Interface index (blank: auto)":"Interface index (blank: auto)","Refresh interval / seconds":"Refresh / seconds","Connection & access":"Connection & access","Wi-Fi name":"Wi-Fi name","Wi-Fi password (blank to keep)":"Wi-Fi password (blank: keep)","Admin password (blank to keep)":"Admin password (blank: keep)","Save & restart":"Save & restart","Drag to crop, zoom or rotate.":"Drag to crop, zoom or rotate.","Choose photo":"Choose photo","Zoom":"Zoom","Rotate 90°":"Rotate 90°","Reset crop":"Reset crop","Upload destination":"Upload destination","Crop & upload":"Crop & upload","Reset device":"Reset device","Eight taps request reset; three more confirm. Expires after 10 seconds.":"Eight taps request reset; three more confirm. Expires after 10 seconds.","Photo crop preview":"Photo crop preview","Select at least one weekday":"Select at least one weekday","Online":"Online","Offline":"Offline"," disks / ":" disks / "," volumes":" volumes","Waiting for weather":"Waiting for weather","Idle":"Idle","Ringing":"Ringing","Snoozed":"Snoozed","Dismissed today":"Dismissed today","Disconnected":"Disconnected","Sound sent":"Sound sent","Failed":"Failed","No active alarm":"No active alarm","Reset complete. Reconnect to the device Wi-Fi.":"Reset complete. Reconnect to the device Wi-Fi.","Cannot open photo":"Cannot open photo","Add new photo (automatic slot)":"Add new photo (automatic slot)","Replace photo ":"Replace photo "," (occupied)":" (occupied)","Photo ":"Photo ","Delete":"Delete","Cannot load photos":"Cannot load photos","Replace this photo?":"Replace this photo?","Photo exceeds 48KB":"Photo exceeds 48KB","Photo saved":"Photo saved","Overview":"Overview","All volumes":"All volumes","Double tap · More":"Double tap · More","Triple tap · Mode":"Triple tap · Mode","Five taps · Management":"Five taps · Management","Six taps · Wi-Fi":"Six taps · Wi-Fi","Eight taps · Reset":"Eight taps · Reset","Management →":"Management →","Tap three times to confirm":"Tap three times to confirm","Tap: next page":"Tap: next page","Pages & controls":"Pages & controls","NAS monitor":"SNMP monitor","Reset confirmation":"Reset confirmation","Wi-Fi setup":"Wi-Fi setup","Management":"Management","Alarm: ":"Alarm: ","Show seconds":"Show seconds","Clear all settings and photos? This cannot be undone.":"Clear all settings and photos? This cannot be undone.","Firmware updates keep settings and photos. Factory reset deletes all settings and photos.":"Firmware updates keep settings and photos. Factory reset deletes all settings and photos.","All settings and photos will be deleted":"All settings and photos will be deleted","Default NAS color":"Reset color","Default clock colors":"Reset colors","NAS background":"Device background","Saving…":"Saving…","Network interface":"Network interface","Automatic":"Automatic","Connected":"Connected","Not connected":"Not connected","Firmware upgrade":"Firmware upgrade","Firmware file (.bin)":"Firmware file (.bin)","Keep settings and photos":"Keep settings and photos","Upload & upgrade":"Upload & upgrade","Upload the program firmware only. System resources are retained.":"Use the same firmware for installation and updates. Settings and photos are kept by default.","Factory reset deletes all settings and photos.":"Factory reset deletes all settings and photos.","Choose a valid program .bin (up to 1 MiB).":"Choose a valid MOS2-NAS firmware file.","Upgrade firmware and keep settings and photos?":"Upgrade firmware and keep settings and photos?","Upgrade firmware and delete all settings and photos?":"Upgrade firmware and delete all settings and photos?","Uploading… Do not disconnect power.":"Uploading… Do not disconnect power.","Checking firmware… Do not disconnect power.":"Checking firmware… Do not disconnect power.","Simulation passed. No firmware was flashed.":"Simulation passed. No firmware was flashed.","Upgrade complete. 1. Wait for restart. 2. Reopen management at the same address.":"Upgrade complete. 1. Wait for restart. 2. Reopen management at the same address.","Upgrade complete. 1. Wait for restart. 2. Connect to the device hotspot. 3. Set up Wi-Fi again.":"Upgrade complete. 1. Wait for restart. 2. Connect to the device hotspot. 3. Set up Wi-Fi again.","Connection interrupted. Check the device before retrying.":"Connection interrupted. Check the device before retrying.","Session expired. Reload and sign in again.":"Session expired. Reload and sign in again.","Firmware is too large for the program partition.":"Firmware is too large for the program partition.","Upgrade failed: invalid, incompatible or incomplete firmware. Data was not cleared.":"Upgrade failed: invalid, incompatible or incomplete firmware. Data was not cleared.","Saved.":"Saved.","City code":"City code","Look up":"Look up","Search for a city on China Weather. The nine digits in its weather page URL are the city code.":"Search for a city on China Weather. The nine digits in its weather page URL are the city code.","MOS2 official":"MOS2 official","Project":"MOS2-NAS project","SNMP version":"SNMP version","SNMP username":"Username","Authentication":"Auth protocol","Authentication password (blank to keep)":"Auth password (blank: keep)","Authentication only, without privacy encryption.":"Authentication only, without privacy encryption.","SNMP v3 requires a username and an authentication password (8–64 bytes).":"SNMP v3 requires a username and an authentication password (8–64 bytes).","Privacy encryption":"Privacy protocol","None":"None","Privacy password (blank to keep)":"Privacy password (blank: keep)","Default password changed":"Default password changed","Sign out":"Sign out","Wi-Fi connection":"Wi-Fi connection","Management password":"Management password","Nearby Wi-Fi":"Nearby Wi-Fi","Scan Wi-Fi":"Scan Wi-Fi","Scanning…":"Scanning…","Choose a network":"Choose a network","No networks found":"No networks found","Wi-Fi scan failed":"Wi-Fi scan failed","Current Wi-Fi":"Current Wi-Fi","Leave passwords blank to keep.":"Leave passwords blank to keep.","Add device":"Add device","Delete device":"Delete device","Device name":"Device name","Discard unsaved changes?":"Discard unsaved changes?","Delete this device?":"Delete this device?","Double tap: switch device":"Overview / status: double tap to switch device","SNMP settings":"SNMP settings","Device & display":"Device & display","Connection":"Connection","Security":"Security","Card color":"Card color","Move up":"Move up","Move down":"Move down","Device list":"Device list","Selected":"Selected","Keep one device":"Keep one device","Invalid device":"Invalid device","Invalid order":"Invalid order","Save failed":"Save failed","Reload page":"Reload page","Rediscover storage":"Rediscover storage","Storage discovery queued":"Storage discovery queued","Discovery already requested":"Discovery already requested","Province":"Province","City":"City","Checking city code…":"Checking city code…","This city provides forecasts only; live weather is unavailable.":"This city provides forecasts only; live weather is unavailable.","City code not found or unsupported.":"City code not found or unsupported.","Weather service unavailable. Try again.":"Weather service unavailable. Try again.","Verify the city code before saving.":"Verify the city code before saving.","Save":"Save","Save & sign in":"Save & sign in","Saved. Applied immediately.":"Saved. Applied immediately.","Saved. Sign in with the new password.":"Saved. Sign in with the new password.","Saved. Restarting device.":"Saved. Restarting device.","Maximum 6 devices":"Maximum 6 devices","Blank: automatic name; filled: custom name":"Leave blank for automatic name; enter a name to override.","White & orange":"White & orange","Cyan & white":"Cyan & white","Green & gold":"Green & gold","Ocean":"Bay blue","Forest":"Emerald","Slate":"Slate","Clock palettes":"Clock palettes","Device palettes":"Device & card palettes","White & pink":"White & pink","Amber":"Amber","Violet":"Iris","Default palette":"Default","White & blue":"White & blue","Device information":"Device information","Display colors":"Display colors","Display options":"Display options","Gold & white":"Gold & white","Violet & cyan":"Violet & cyan","Lime & white":"Lime & white","White & coral":"White & coral","Rose":"Berry","Finish device list edits, then save and restart.":"Finish adding, removing and reordering devices, then save and restart."},"zh":{"CPU":"CPU","Memory":"内存","NAS temperature":"设备温度","Receive / send":"接收 / 发送","Screen":"屏幕","Settings":"设置","Photos":"相册","Maintenance":"维护","Desktop":"桌面","Clock":"大字时钟","Weather":"天气","NAS overview":"监控总览","Network":"网络","CPU / memory":"CPU / 内存","Disk health":"硬盘健康","Volumes":"存储卷","Status":"状态","Tap: next · Double: photos / disk groups · Triple: mode · Five: management · Six: Wi-Fi":"单击翻页 · 双击照片 / 更多硬盘与卷 · 三击切换功能 · 五击管理 · 六击配网","Desktop & display":"桌面与显示","Display language":"MOS2语言","Start page":"启动页面","Hour color":"时的颜色","Minute color":"分的颜色","Brightness / %":"亮度 / %","Orientation":"方向","0°":"0°","90°":"90°","180°":"180°","270°":"270°","Touch sound":"触摸提示音","Off":"关闭","On":"开启","Test sound":"试听","Weather & time":"天气与时间","Weather refresh / minutes":"天气刷新 / 分钟","Time server":"时间服务器","Alarm":"闹钟","Time":"时间","Repeat":"重复","Mon":"一","Tue":"二","Wed":"三","Thu":"四","Fri":"五","Sat":"六","Sun":"日","8-bit melody":"8-bit 铃声","Morning":"清晨","Arcade":"街机","Starlight":"星光","Journey":"启程","Snooze / minutes":"贪睡 / 分钟","Test alarm":"测试闹钟","Simulate tap":"模拟短按","Tap to snooze; three taps dismiss today":"每轮最多响5分钟，单击贪睡一次，三击停止","Photo playback":"相册播放","Playback":"播放方式","Slideshow":"自动轮播","Fixed (double tap: next photo)":"固定显示（双击换照片）","Slide duration / seconds":"停留时间 / 秒","Up to 7 photos, 48KB each":"最多 7 张，每张不超过 48KB","Address":"地址","Community (blank to keep)":"社群（留空保留）","Port":"端口","Interface index (blank: auto)":"网口索引（空白自动）","Refresh interval / seconds":"刷新间隔 / 秒","Connection & access":"连接与管理","Wi-Fi name":"Wi-Fi 名称","Wi-Fi password (blank to keep)":"Wi-Fi 密码（留空保留）","Admin password (blank to keep)":"管理密码（留空保留）","Save & restart":"保存并重启","Drag to crop, zoom or rotate.":"拖动裁剪，缩放或旋转后上传。","Choose photo":"选择照片","Zoom":"缩放","Rotate 90°":"旋转 90°","Reset crop":"重置构图","Upload destination":"上传到","Crop & upload":"裁剪并上传","Reset device":"重置设备","Eight taps request reset; three more confirm. Expires after 10 seconds.":"连续八击进入重置确认，再连续三击执行；十秒自动取消。","Photo crop preview":"照片裁剪预览","Select at least one weekday":"请选择至少一天","Online":"在线","Offline":"离线"," disks / ":" 块硬盘 / "," volumes":" 个卷","Waiting for weather":"等待天气","Idle":"待机","Ringing":"响铃","Snoozed":"贪睡","Dismissed today":"今天已停止","Disconnected":"连接中断","Sound sent":"已发送提示音","Failed":"失败","No active alarm":"没有活动闹钟","Reset complete. Reconnect to the device Wi-Fi.":"已重置，请重新连接设备热点。","Cannot open photo":"无法打开这张照片","Add new photo (automatic slot)":"新增照片（自动选择空位）","Replace photo ":"替换第 "," (occupied)":" 张（已占用）","Photo ":"第 ","Delete":"删除","Cannot load photos":"相册读取失败","Replace this photo?":"替换选中的这张照片？","Photo exceeds 48KB":"照片超过 48KB","Photo saved":"已保存照片","Overview":"总览","All volumes":"所有存储卷","Double tap · More":"双击 · 换照片 / 更多","Triple tap · Mode":"三击 · 切换功能","Five taps · Management":"五击 · 管理","Six taps · Wi-Fi":"六击 · 配网","Eight taps · Reset":"八击 · 重置","Management →":"网页管理 →","Tap three times to confirm":"连续三击确认重置","Tap: next page":"单击下一页","Pages & controls":"页面与操作","NAS monitor":"SNMP 监控","Reset confirmation":"重置确认","Wi-Fi setup":"无线配网","Management":"设备管理","Alarm: ":"闹钟：","Show seconds":"秒数显示","Clear all settings and photos? This cannot be undone.":"清除所有配置和照片？此操作无法撤销。","Firmware updates keep settings and photos. Factory reset deletes all settings and photos.":"更新固件保留配置和照片；恢复出厂设置清除全部配置和照片。","All settings and photos will be deleted":"全部配置和照片将删除","Default NAS color":"恢复默认底色","Default clock colors":"恢复时钟默认颜色","NAS background":"设备底色","Saving…":"正在保存…","Network interface":"网络接口","Automatic":"自动","Connected":"已连接","Not connected":"未连接","Firmware upgrade":"固件升级","Firmware file (.bin)":"固件文件（.bin）","Keep settings and photos":"保留配置和照片","Upload & upgrade":"上传并升级","Upload the program firmware only. System resources are retained.":"安装和更新使用同一个固件文件，默认保留配置与照片。","Factory reset deletes all settings and photos.":"恢复出厂设置清除全部配置和照片。","Choose a valid program .bin (up to 1 MiB).":"请选择有效的 MOS2-NAS 固件文件。","Upgrade firmware and keep settings and photos?":"升级固件并保留配置和照片？","Upgrade firmware and delete all settings and photos?":"升级固件并删除全部配置和照片？","Uploading… Do not disconnect power.":"正在上传，请勿断电。","Checking firmware… Do not disconnect power.":"正在校验固件，请勿断电。","Simulation passed. No firmware was flashed.":"模拟验证通过，未写入实体机固件。","Upgrade complete. 1. Wait for restart. 2. Reopen management at the same address.":"升级完成。1. 等待重启。2. 使用原地址重新打开管理页。","Upgrade complete. 1. Wait for restart. 2. Connect to the device hotspot. 3. Set up Wi-Fi again.":"升级完成。1. 等待重启。2. 连接设备热点。3. 重新配网。","Connection interrupted. Check the device before retrying.":"连接中断，请先确认设备状态再重试。","Session expired. Reload and sign in again.":"登录已过期，请刷新页面重新登录。","Firmware is too large for the program partition.":"固件超过程序分区可用空间。","Upgrade failed: invalid, incompatible or incomplete firmware. Data was not cleared.":"升级失败：固件无效、不兼容或上传不完整，未清除数据。","Saved.":"已保存。","City code":"城市代码","Look up":"查询","Search for a city on China Weather. The nine digits in its weather page URL are the city code.":"在中国天气网搜索城市，天气页面网址中的 9 位数字就是城市代码。","MOS2 official":"MOS2官方","Project":"MOS2-NAS 项目","SNMP version":"SNMP 版本","SNMP username":"SNMP 用户名","Authentication":"认证协议","Authentication password (blank to keep)":"认证密码（留空保留）","Authentication only, without privacy encryption.":"仅认证，不启用隐私加密。","SNMP v3 requires a username and an authentication password (8–64 bytes).":"SNMP v3 需要用户名和认证密码（8–64 字节）。","Privacy encryption":"隐私加密","None":"不加密","Privacy password (blank to keep)":"隐私密码（留空保留）","Default password changed":"默认密码已修改","Sign out":"退出登录","Wi-Fi connection":"Wi-Fi 连接","Management password":"管理密码","Nearby Wi-Fi":"附近 Wi-Fi","Scan Wi-Fi":"搜索 Wi-Fi","Scanning…":"正在搜索…","Choose a network":"选择 Wi-Fi","No networks found":"未找到 Wi-Fi","Wi-Fi scan failed":"搜索失败，请重试","Current Wi-Fi":"当前 Wi-Fi","Leave passwords blank to keep.":"密码留空则保留。","Add device":"添加设备","Delete device":"删除设备","Device name":"设备名称","Discard unsaved changes?":"放弃未保存的修改？","Delete this device?":"删除这台设备的配置？","Double tap: switch device":"总览 / 设备状态：双击切换设备","SNMP settings":"SNMP 配置","Device & display":"设备与显示","Connection":"连接","Security":"认证与加密","Card color":"卡片底色","Move up":"上移","Move down":"下移","Device list":"设备列表","Selected":"当前设备","Keep one device":"至少保留一台设备","Invalid device":"设备无效","Invalid order":"排序无效","Save failed":"保存失败","Reload page":"请刷新页面","Rediscover storage":"重新发现存储","Storage discovery queued":"已安排重新发现，现有数据继续显示","Discovery already requested":"已安排发现，请稍后再试","Province":"省份","City":"城市","Checking city code…":"正在校验城市代码…","This city provides forecasts only; live weather is unavailable.":"该城市仅提供预报，暂不支持实时天气。","City code not found or unsupported.":"城市代码不存在或不支持。","Weather service unavailable. Try again.":"天气服务暂时不可用，请稍后重试。","Verify the city code before saving.":"请先校验城市代码再保存。","Save":"保存","Save & sign in":"保存并重新登录","Saved. Applied immediately.":"已保存，已立即生效。","Saved. Sign in with the new password.":"已保存，请使用新密码重新登录。","Saved. Restarting device.":"已保存，设备正在重启。","Maximum 6 devices":"最多添加六台设备","Blank: automatic name; filled: custom name":"留空自动获取；填写后显示自定义名称。","White & orange":"白橙","Cyan & white":"青白","Green & gold":"绿金","Ocean":"海湾蓝","Forest":"翡翠绿","Slate":"岩灰","Clock palettes":"时分配色","Device palettes":"设备与卡片配色","White & pink":"白粉","Amber":"琥珀金","Violet":"鸢尾紫","Default palette":"默认","White & blue":"白蓝","Device information":"设备信息","Display colors":"显示配色","Display options":"基本显示","Gold & white":"金白","Violet & cyan":"紫青","Lime & white":"绿白","White & coral":"白珊瑚","Rose":"莓果粉","Finish device list edits, then save and restart.":"设备列表可连续添加、删除和排序，完成后统一保存并重启。"}} ,text(key,language){return this.locales[language]?.[key]??this.locales.en[key]??key;},apply(parent,language){parent.querySelectorAll('[data-i18n]').forEach(el=>el.textContent=this.text(el.dataset.i18n,language));}};
)MOSJS";
const char localeJSGzip[] PROGMEM = {31,139,8,0,0,0,0,0,2,255,189,92,91,115,20,71,150,126,223,95,81,209,17,235,129,136,118,207,224,185,97,17,94,2,176,61,235,157,241,37,44,60,142,88,135,31,138,238,146,84,75,119,85,79,85,181,64,118,16,33,97,116,3,181,36,64,66,232,130,133,64,24,129,209,197,24,131,238,138,152,159,226,85,86,117,63,241,23,246,156,60,89,89,89,221,213,45,53,3,251,162,206,60,121,63,121,174,89,95,40,107,91,174,167,125,116,236,184,245,222,183,121,59,171,231,13,183,237,219,148,97,165,224,239,153,207,190,72,181,241,191,233,212,199,70,193,118,122,160,42,10,233,212,39,167,218,53,207,40,20,13,71,247,74,142,1,77,239,27,221,102,214,136,17,211,169,207,141,172,97,118,27,218,111,53,215,176,114,208,171,134,144,78,181,103,29,3,23,12,11,64,49,60,207,180,58,93,164,133,197,116,234,179,46,219,179,145,38,10,176,41,221,180,60,195,210,173,44,46,175,214,210,176,25,247,188,103,23,249,182,168,148,78,157,129,19,158,199,35,241,223,116,234,75,67,247,186,12,7,40,97,137,142,101,119,27,78,183,105,92,128,134,79,195,34,180,24,222,5,219,193,241,97,41,141,204,129,131,20,66,230,196,170,176,7,211,61,175,117,25,122,222,235,194,125,40,181,116,234,239,118,190,84,48,240,56,97,9,206,237,1,215,248,169,169,144,78,157,213,139,109,154,101,92,244,180,127,190,208,222,183,75,231,242,70,155,86,228,231,135,133,114,56,99,167,99,151,138,46,182,159,117,204,34,182,23,236,156,129,245,15,129,205,80,211,45,189,211,40,24,22,159,163,221,188,216,166,125,105,190,253,161,9,203,188,201,217,229,13,104,111,225,76,197,188,222,19,221,133,66,227,92,194,146,150,215,173,206,18,76,70,172,138,147,56,111,28,79,43,82,187,82,73,167,254,211,46,57,90,214,206,219,120,145,74,5,228,195,180,74,158,33,219,98,213,116,234,180,99,118,118,121,150,225,226,97,255,29,218,107,8,233,212,167,142,9,7,211,61,211,70,241,84,107,233,212,239,254,185,6,52,252,155,78,189,75,149,119,169,118,236,56,85,233,55,157,122,231,207,84,167,95,184,83,187,148,237,210,92,187,196,213,65,173,193,138,29,29,184,18,252,133,50,95,20,215,58,107,128,142,202,1,81,69,74,48,240,211,51,11,70,36,200,33,33,234,225,24,29,142,225,118,161,120,114,54,184,92,21,107,105,176,22,12,3,205,116,186,185,94,168,181,116,234,84,94,119,10,64,165,95,234,43,58,113,77,47,194,90,124,90,94,0,254,115,190,125,204,249,117,182,196,187,150,104,79,57,190,85,60,193,217,174,18,210,225,111,58,245,161,131,82,137,127,225,190,249,84,237,124,158,246,18,55,15,37,156,231,248,219,231,76,15,84,44,111,231,80,160,98,85,92,209,177,192,92,240,85,169,4,187,118,178,122,14,23,23,5,146,165,60,222,181,16,37,42,167,83,255,5,194,99,25,56,109,88,130,190,150,109,127,99,196,216,86,71,18,55,164,11,246,40,21,24,111,22,74,121,29,196,206,211,209,24,197,170,92,191,53,207,214,92,62,227,9,205,235,2,11,136,77,46,234,71,193,4,73,244,236,28,215,156,207,209,10,106,37,222,253,143,184,242,9,205,182,140,104,36,215,78,28,170,185,160,96,210,94,106,168,71,231,116,110,246,106,8,208,67,105,139,168,237,121,51,7,82,97,163,249,139,202,112,59,230,69,35,167,29,201,113,59,129,43,9,219,193,13,198,81,188,184,230,29,196,204,90,174,228,112,37,226,30,32,107,91,57,228,233,251,245,196,116,234,11,126,218,63,11,147,148,214,254,112,252,175,167,53,67,207,162,53,109,220,6,55,158,203,129,92,227,180,97,9,44,181,93,40,148,44,211,235,209,142,156,3,179,114,30,71,159,55,140,34,110,188,113,27,240,200,118,80,76,248,79,58,245,17,120,24,167,67,7,47,103,90,57,227,162,232,223,166,233,37,98,65,243,246,180,212,56,244,84,78,183,158,143,241,32,82,199,136,5,103,108,203,50,178,156,51,111,105,122,54,75,199,74,162,130,90,161,225,213,44,157,12,65,84,9,91,138,186,235,130,223,202,37,112,32,177,67,155,100,194,169,28,8,92,179,241,137,29,162,241,237,58,56,252,183,192,6,185,104,184,185,94,199,8,224,4,28,189,19,39,204,58,118,49,173,125,99,219,5,205,6,163,101,131,189,53,50,40,31,77,219,129,79,93,182,237,26,36,13,200,32,181,154,78,253,55,244,7,42,255,129,75,224,163,52,97,181,213,26,94,144,107,120,124,21,126,33,178,130,194,152,183,245,156,6,234,0,1,73,232,19,18,136,176,23,135,123,184,18,111,195,205,196,234,225,26,57,30,45,201,85,68,53,157,250,0,141,17,217,0,199,248,71,9,109,137,131,61,66,235,0,177,5,250,48,171,195,116,10,25,237,131,139,69,19,154,53,189,3,4,74,59,246,187,80,116,144,103,175,107,166,208,148,32,35,180,162,99,136,208,40,129,136,161,91,30,4,83,211,61,45,111,232,176,32,218,168,11,134,113,158,140,88,179,86,116,120,121,211,50,184,211,227,5,238,14,67,146,40,165,83,60,46,65,15,13,212,168,12,244,110,25,81,201,34,72,190,110,98,252,168,117,128,180,92,136,194,189,4,42,40,119,46,143,75,241,159,52,55,184,228,74,194,82,232,12,114,210,7,228,40,124,65,67,13,150,47,52,213,181,20,222,39,75,42,203,7,199,170,48,43,58,115,140,134,185,102,68,21,48,185,186,153,231,35,68,1,130,80,27,244,221,195,248,57,244,54,181,20,41,195,118,1,60,2,104,135,246,185,33,86,67,5,130,179,10,89,163,64,45,19,201,249,97,7,128,132,235,150,101,195,245,21,13,43,82,185,58,26,55,196,224,2,46,80,93,59,130,86,176,0,90,146,213,220,188,237,145,221,104,218,129,7,21,121,52,167,212,129,130,11,149,0,247,126,196,206,102,75,69,211,200,225,132,106,45,148,91,41,171,60,40,197,35,242,72,148,23,228,89,184,30,23,195,20,35,129,24,237,197,235,50,93,162,158,84,54,164,82,195,149,141,139,89,195,200,185,220,57,201,93,196,136,97,79,23,12,98,78,118,161,90,58,202,62,98,137,200,169,124,94,17,118,181,6,231,147,206,23,227,113,8,130,248,89,235,137,16,122,68,17,3,39,242,16,41,129,136,142,191,91,4,37,72,147,1,63,247,248,201,45,24,246,92,148,13,97,186,81,79,139,89,59,160,114,65,140,91,46,73,197,116,79,230,26,191,14,220,224,25,95,140,32,194,41,138,161,32,38,117,185,195,32,251,70,217,78,163,70,53,209,18,41,70,13,1,238,9,126,92,48,227,48,198,115,236,60,79,68,107,73,148,62,22,108,136,37,120,198,209,254,201,199,159,201,106,164,152,124,209,208,133,36,16,67,135,13,13,165,162,244,206,84,83,217,16,99,65,24,159,183,105,97,132,222,134,2,223,14,193,155,18,99,196,170,152,17,27,186,3,134,35,143,179,243,68,91,211,173,80,228,79,106,103,81,166,179,164,11,231,12,13,44,19,88,236,12,207,160,95,101,28,138,146,83,184,160,131,219,41,21,115,224,117,93,30,33,36,205,145,209,62,4,171,6,105,52,185,44,176,64,168,174,110,163,37,51,92,24,223,212,220,164,114,9,77,218,5,19,26,206,25,98,138,156,80,199,195,244,68,83,212,161,151,242,158,134,34,19,166,168,161,48,80,134,26,246,200,226,123,5,81,221,120,167,80,228,48,116,199,84,157,103,136,226,29,70,161,241,8,12,118,244,107,239,15,20,124,137,178,124,212,160,128,20,3,215,232,121,67,161,1,3,66,203,140,71,148,101,25,159,242,163,159,81,124,218,39,54,151,105,217,22,175,199,4,161,211,161,244,172,142,164,244,234,0,247,167,29,201,156,51,173,163,106,79,149,156,78,253,181,193,125,195,136,134,77,50,174,123,75,217,73,29,73,246,66,103,88,116,108,160,22,180,142,112,23,182,149,239,201,104,237,61,174,103,20,80,164,32,119,204,162,56,65,147,99,120,58,132,45,57,148,207,47,92,131,143,119,33,38,143,6,99,8,98,90,16,6,231,243,148,0,225,230,132,4,195,164,9,146,132,163,206,27,69,208,172,30,144,38,46,33,92,181,90,214,151,86,149,64,4,213,186,6,137,139,153,147,124,64,246,107,71,40,51,61,166,125,108,158,62,154,137,34,240,176,243,199,159,182,191,243,54,74,106,135,122,119,25,206,89,206,227,168,1,23,109,164,186,39,249,237,180,52,160,193,10,116,222,134,38,172,209,58,7,13,11,37,133,20,76,123,223,214,208,6,230,100,192,167,21,237,11,134,147,145,66,118,80,63,228,186,145,61,207,131,85,177,143,3,230,109,173,191,124,160,64,209,195,252,13,100,85,131,144,82,30,250,130,238,106,29,121,221,237,34,33,110,165,119,196,249,40,180,60,150,209,48,248,230,98,47,178,191,140,246,14,134,156,60,112,84,158,19,33,73,144,218,162,83,26,159,81,110,229,181,77,217,202,46,207,36,134,197,112,245,110,209,134,30,191,231,10,139,143,52,228,177,245,78,80,254,214,54,253,106,43,196,222,9,184,217,118,74,69,15,47,135,75,131,58,213,57,163,195,38,195,228,244,128,148,100,226,175,9,45,142,197,124,207,117,113,160,193,179,199,28,114,157,27,74,212,9,215,236,196,25,35,54,180,208,89,177,253,38,134,106,182,6,17,77,39,153,75,213,8,23,129,113,38,110,61,22,1,28,118,136,98,27,120,130,213,6,59,224,246,42,13,5,188,44,144,116,140,156,185,141,14,47,79,74,123,70,123,95,247,116,46,243,168,92,89,140,137,132,169,127,3,179,210,43,10,41,33,21,224,214,241,205,42,75,161,123,84,78,167,254,102,219,231,53,30,58,134,37,188,41,221,201,118,113,110,232,90,22,59,195,77,156,233,50,45,93,19,111,196,25,136,219,12,205,2,127,5,134,162,211,244,92,188,15,252,17,41,50,143,132,181,47,62,255,27,119,65,200,210,108,184,38,221,238,27,93,0,2,95,240,33,154,221,209,97,102,77,61,143,177,111,172,14,17,186,99,255,15,200,177,104,225,222,166,40,72,105,10,197,33,135,114,41,234,142,85,69,107,201,53,28,241,122,246,69,88,228,145,79,23,24,15,51,27,6,236,72,192,137,61,27,66,176,186,14,77,95,201,248,200,134,143,108,241,121,48,170,72,67,216,232,117,217,37,48,215,142,217,173,103,123,52,195,202,58,61,197,80,224,91,30,18,242,225,247,252,69,136,222,123,228,185,185,30,234,22,62,86,38,31,232,248,175,189,55,255,244,7,136,59,32,80,224,78,254,53,78,134,215,87,187,95,76,177,4,81,225,247,39,54,127,17,226,63,209,168,38,108,111,208,37,226,124,24,103,203,14,217,46,221,234,164,167,154,70,77,232,56,193,98,1,163,185,87,20,197,48,115,203,74,147,42,211,55,133,20,75,101,195,153,227,249,172,164,98,136,174,59,16,234,133,89,116,172,138,95,79,129,201,50,195,142,42,212,98,69,65,191,82,83,66,57,75,126,212,172,35,241,199,38,81,1,175,46,146,139,122,154,204,86,113,241,142,240,197,170,158,6,171,150,28,7,15,23,110,55,94,7,179,101,224,179,112,120,116,87,139,221,36,202,219,1,29,232,193,73,62,172,42,149,240,225,39,106,140,215,211,97,210,36,244,95,173,209,243,157,14,119,95,178,248,195,140,144,1,30,36,54,108,146,43,242,119,33,90,230,100,180,110,140,170,62,219,180,105,46,40,48,216,81,185,211,240,229,7,191,11,240,239,194,109,90,244,137,133,127,61,138,13,16,42,238,42,223,208,99,117,121,212,248,119,217,26,146,26,81,196,66,4,238,73,178,37,7,12,51,183,249,162,136,239,104,168,29,34,143,85,42,248,81,174,219,32,111,20,150,4,45,103,95,176,66,42,47,203,189,229,77,215,139,182,197,107,225,219,50,61,191,134,69,145,245,225,67,178,100,88,45,5,63,219,80,22,34,187,212,16,162,30,32,86,252,133,56,94,23,95,48,164,112,171,53,124,214,161,119,66,122,56,82,107,216,198,99,110,184,66,252,46,231,132,93,234,136,248,81,146,151,180,176,169,71,251,71,201,40,209,114,141,154,196,203,50,39,233,121,199,208,115,61,225,83,127,244,208,220,160,149,251,203,110,147,176,19,178,72,49,133,8,39,212,236,67,122,98,50,39,201,116,252,156,139,47,63,72,43,226,148,57,3,173,132,99,100,117,23,220,60,250,167,19,112,155,221,134,244,248,208,187,100,233,221,192,74,253,92,158,135,17,255,234,12,74,84,196,3,40,110,164,48,214,2,21,45,21,139,182,227,81,16,117,152,94,209,231,115,252,6,142,162,168,46,165,157,69,198,134,193,237,97,123,166,83,127,55,28,179,163,39,30,223,132,177,181,203,223,102,112,190,195,244,34,185,20,2,25,125,103,19,177,116,244,157,45,36,132,49,164,118,170,88,204,155,96,174,204,66,1,100,81,247,140,124,79,20,88,38,183,202,177,237,34,82,199,56,131,111,143,63,224,11,155,172,204,114,64,63,57,223,231,148,254,160,48,145,58,42,115,36,180,161,243,188,8,89,104,65,251,147,160,185,220,117,214,210,210,169,211,209,183,87,250,168,128,230,252,4,62,60,240,160,60,91,2,229,43,132,22,159,92,11,57,20,30,194,214,140,50,48,51,66,239,136,177,13,24,93,84,43,199,164,192,244,203,46,211,67,54,131,150,90,92,193,107,8,32,146,61,58,126,170,189,128,116,148,61,181,154,78,253,5,113,79,80,239,180,243,168,181,177,106,58,245,105,214,208,241,46,79,235,61,176,63,14,160,248,208,198,156,17,31,202,11,134,163,243,94,237,136,42,224,95,236,117,250,172,193,223,12,139,58,120,27,66,45,212,16,164,169,85,186,72,47,192,93,154,210,53,60,79,209,180,206,43,199,227,85,112,186,133,115,220,102,210,47,200,183,105,231,249,51,254,71,142,233,198,194,43,62,97,20,85,41,51,243,131,181,197,171,114,139,166,5,55,34,31,204,19,136,17,136,72,62,146,214,16,162,30,54,15,45,213,46,33,5,46,2,56,174,92,83,172,26,158,11,217,211,195,47,36,94,135,240,5,193,50,209,232,88,53,58,105,22,132,34,175,28,149,234,248,85,218,197,81,167,65,172,122,120,250,107,153,110,232,212,185,7,212,64,23,61,55,141,170,100,241,47,68,60,196,14,31,15,120,250,203,135,232,57,124,85,74,67,75,1,141,122,167,232,198,61,89,164,72,141,39,186,148,78,125,211,213,16,255,199,6,250,217,242,237,68,252,95,101,101,151,45,14,250,235,143,216,230,15,73,200,63,127,244,129,63,241,28,234,108,236,122,181,183,79,133,254,177,159,198,216,198,100,28,250,7,211,5,59,43,42,240,47,152,93,103,3,35,117,192,191,96,235,153,127,117,49,134,249,243,23,70,170,119,238,41,144,63,182,248,144,45,79,249,83,207,171,55,230,99,200,63,182,248,200,95,155,168,7,254,5,179,215,253,209,135,126,239,86,229,225,80,12,252,23,236,92,15,182,230,26,97,255,36,123,226,216,191,96,225,73,48,123,155,245,61,96,155,47,98,224,63,232,204,46,47,177,242,11,21,254,23,92,125,238,115,254,188,89,248,31,43,79,178,193,173,96,111,171,186,240,11,54,178,177,17,172,247,63,12,134,7,97,122,127,246,25,91,156,161,157,239,175,143,194,30,177,211,254,250,48,116,98,67,131,126,249,30,187,58,95,249,110,135,83,55,39,112,232,202,66,48,62,192,167,234,95,134,122,181,191,12,220,106,0,5,164,43,130,137,253,219,187,193,226,102,50,16,16,19,249,202,234,114,101,169,183,22,2,200,198,87,217,213,37,216,57,221,115,12,4,8,215,28,204,92,169,222,155,171,12,63,173,135,1,178,161,1,181,181,14,6,184,191,185,2,2,156,8,1,244,111,109,176,241,235,175,17,253,87,121,248,131,127,125,221,31,27,7,14,84,231,127,150,8,64,214,255,115,117,106,57,4,1,178,237,94,56,110,45,16,176,178,58,201,198,159,36,129,0,73,166,145,181,32,239,83,207,14,64,1,82,111,54,244,194,191,181,134,18,60,52,64,58,18,71,2,210,84,254,92,153,93,93,96,211,75,10,26,176,58,181,17,13,144,61,85,64,96,117,176,204,22,203,18,16,184,191,222,43,1,129,251,155,35,18,16,8,162,37,1,129,108,118,86,2,2,65,184,36,32,16,4,75,2,2,253,169,7,141,0,129,213,155,223,177,251,107,49,64,160,191,222,239,211,190,67,60,96,101,97,202,159,219,172,193,3,250,183,231,89,255,112,12,12,8,188,15,150,174,37,99,1,43,207,30,7,119,22,98,108,83,161,128,254,47,215,224,154,36,135,106,192,128,254,210,130,127,109,62,152,95,246,71,134,91,2,3,250,171,99,149,157,21,127,174,23,20,148,221,44,255,145,22,127,185,61,66,42,77,155,2,54,251,79,22,128,40,84,182,111,206,95,190,151,132,11,36,179,234,223,88,246,39,118,227,168,64,162,161,212,111,143,213,32,3,43,131,143,65,255,112,23,55,150,15,3,14,100,179,155,108,101,134,116,253,229,246,16,217,26,176,33,100,110,94,110,15,55,199,7,194,238,131,201,105,18,45,104,8,30,222,56,0,31,72,204,129,70,182,125,23,120,0,28,131,194,254,122,185,242,188,191,178,55,24,130,47,34,164,32,155,91,99,119,122,15,0,10,6,139,187,193,238,34,108,31,246,18,60,218,220,223,187,3,5,218,187,192,9,6,63,174,178,177,251,135,64,10,130,97,132,142,193,179,123,108,123,18,39,124,180,25,76,239,16,83,105,194,166,80,65,210,86,224,69,117,102,66,178,35,25,43,88,217,251,30,28,47,24,3,178,206,201,56,65,54,94,14,30,174,29,30,41,200,86,7,130,187,125,137,140,56,16,43,72,251,104,50,67,29,90,16,26,209,79,110,60,71,67,194,13,225,65,112,65,255,218,45,148,206,251,125,108,248,49,220,126,176,253,8,229,120,232,150,63,117,173,178,243,132,141,143,238,175,95,221,223,190,251,191,189,151,235,177,131,213,222,97,255,218,35,146,203,8,59,72,83,212,161,7,105,194,68,244,32,108,22,226,23,255,251,43,108,118,183,17,126,144,182,193,134,214,18,96,131,180,123,56,53,117,170,7,14,210,2,20,117,189,86,224,32,136,76,176,181,204,250,127,68,91,178,55,203,250,31,208,82,193,2,172,182,136,102,102,160,76,125,200,180,248,195,15,43,11,35,47,183,103,89,185,15,68,145,164,152,141,221,242,159,15,17,135,19,241,131,196,97,58,101,245,222,21,138,179,154,227,7,43,171,47,232,118,42,131,63,179,181,235,96,225,192,121,169,200,65,54,183,20,108,238,197,144,131,193,15,91,68,138,1,7,217,157,41,138,108,234,193,131,251,235,143,41,30,75,198,14,6,203,195,108,183,95,134,141,2,52,8,36,242,37,17,108,16,12,51,184,161,24,106,144,12,115,34,104,112,127,235,42,204,201,94,60,149,118,186,6,52,24,234,241,178,127,107,185,22,52,136,195,120,76,173,70,18,18,60,200,22,127,170,60,123,144,8,30,244,159,46,248,115,195,254,179,45,140,165,66,47,213,50,126,16,150,39,1,1,209,192,43,26,44,131,113,162,253,146,116,6,223,45,7,151,55,132,182,37,96,7,253,169,187,254,207,147,254,240,77,136,116,42,123,211,96,169,165,250,29,8,33,132,165,216,189,239,67,55,50,68,194,39,116,24,236,202,78,57,52,167,53,64,66,127,118,15,221,207,147,39,9,32,66,238,51,134,144,171,229,187,193,132,48,200,18,76,40,198,72,40,33,27,186,91,157,94,108,4,37,36,239,90,89,221,2,133,144,55,145,12,37,164,45,193,222,225,150,33,68,85,57,241,114,123,190,17,170,80,232,81,204,175,197,65,133,112,16,50,161,146,171,10,176,80,166,56,113,88,161,63,220,11,146,161,166,39,137,208,66,242,227,72,144,190,92,166,14,141,64,134,100,52,120,154,160,164,17,77,129,134,148,95,32,89,122,177,36,132,33,165,29,72,145,169,71,50,192,144,108,27,239,200,5,55,9,93,8,227,33,189,16,57,205,33,240,133,170,69,36,75,41,231,174,67,23,82,156,182,191,126,13,76,24,44,146,12,47,164,228,6,195,248,155,229,253,157,185,100,116,33,229,169,141,192,133,170,221,174,131,21,130,214,129,101,148,156,138,49,92,232,109,200,236,8,89,72,86,226,229,246,76,61,182,16,76,191,63,185,38,211,185,87,197,22,66,160,14,234,68,242,199,55,183,194,110,140,72,45,240,151,23,137,29,194,102,220,88,172,78,244,146,101,121,147,224,66,144,104,52,52,179,155,251,91,207,41,86,169,219,218,172,223,119,15,210,28,54,184,201,70,47,211,227,5,29,133,245,47,85,191,91,170,233,79,91,62,60,174,48,113,18,182,54,32,141,79,18,156,144,118,84,221,186,13,215,207,54,39,41,225,109,128,42,164,190,244,66,66,35,100,138,92,135,48,36,233,144,51,170,224,66,127,249,62,56,97,50,55,141,33,134,244,136,2,254,129,130,101,21,93,72,6,188,6,90,8,22,140,252,73,2,180,208,159,123,44,219,18,96,133,116,103,172,60,24,108,62,108,12,41,164,94,254,173,65,248,11,166,31,201,100,245,155,128,10,19,229,32,25,81,40,226,188,141,104,31,175,1,80,200,86,134,43,247,251,97,101,18,206,253,157,61,240,85,108,28,242,189,94,140,97,98,39,26,161,43,85,183,140,41,129,34,138,173,171,68,235,226,222,18,130,80,6,123,24,161,76,14,129,67,140,80,132,234,225,104,234,150,97,132,116,19,66,58,54,26,41,245,252,171,227,7,107,22,32,69,77,100,78,184,204,225,128,131,66,195,184,68,81,192,197,174,237,65,80,24,76,252,18,114,185,53,196,32,77,232,223,93,168,62,30,161,237,38,78,219,26,80,144,222,54,96,198,202,106,95,181,119,6,98,19,204,194,231,30,179,129,105,200,39,216,202,247,251,59,55,33,98,166,245,226,87,248,26,209,130,226,14,86,70,252,161,113,88,4,230,163,240,157,18,73,160,192,100,66,111,70,231,233,9,128,162,87,138,69,201,255,129,31,110,117,131,175,10,20,60,204,126,19,3,107,152,145,54,78,238,92,8,194,191,0,10,84,211,13,33,14,253,67,20,72,136,165,249,35,53,100,130,176,110,101,117,82,200,72,43,112,192,96,122,139,237,76,114,211,62,232,207,205,139,85,232,101,131,135,62,116,34,234,86,227,226,15,15,10,36,9,163,240,56,88,186,198,54,199,216,208,0,27,217,100,99,171,112,237,144,34,84,167,158,213,152,144,215,138,5,20,87,202,35,127,8,155,132,229,130,224,101,18,146,227,190,253,245,50,235,223,102,43,27,254,208,45,82,105,164,192,253,79,62,35,125,17,209,16,196,85,229,21,193,227,16,6,40,195,122,113,219,10,24,144,205,143,178,245,203,251,91,247,131,187,125,49,60,160,63,255,160,178,122,239,255,3,14,200,109,212,50,155,221,161,84,25,132,210,31,159,195,247,46,190,53,124,68,224,116,186,105,124,13,187,211,75,105,143,246,46,168,100,25,78,204,150,167,216,218,79,254,237,85,245,52,116,214,36,40,32,91,185,237,223,218,72,198,1,86,23,54,130,217,149,6,48,192,96,120,200,159,123,146,128,2,164,214,137,37,127,232,5,27,47,39,97,1,81,27,86,251,88,121,172,178,178,210,18,20,80,12,108,242,140,214,58,26,112,127,171,159,166,229,111,190,104,48,96,239,213,153,241,224,97,31,187,122,23,214,18,242,243,38,16,128,213,185,222,202,15,125,146,87,224,218,106,78,40,70,194,157,86,174,194,237,15,139,135,161,36,204,159,186,229,8,239,135,39,10,73,135,0,253,137,73,26,51,184,9,232,79,4,204,124,44,87,178,21,127,98,35,142,248,171,246,246,66,228,67,166,169,25,234,79,70,166,201,136,63,245,45,180,30,234,87,157,190,82,217,187,158,12,245,35,101,106,0,246,19,30,157,119,105,8,248,163,224,74,206,144,4,246,67,251,51,188,203,134,214,100,175,36,168,159,208,107,97,225,196,11,16,184,132,4,196,31,219,185,201,134,203,135,7,252,17,103,232,242,216,208,52,221,159,72,157,84,168,159,255,98,11,132,67,62,133,214,130,253,40,246,82,154,85,172,159,72,102,194,23,240,38,96,63,127,98,151,109,127,7,76,17,175,41,51,87,72,52,40,126,75,198,251,137,165,247,166,217,216,154,112,156,51,87,40,254,19,195,154,64,255,232,109,70,251,173,166,186,92,116,34,252,213,133,94,79,228,169,18,97,127,180,82,50,232,143,70,170,95,96,99,152,63,41,186,10,222,143,148,26,191,6,135,170,24,131,253,177,242,2,102,165,97,94,24,1,255,192,181,5,15,183,106,112,127,251,235,215,136,24,7,253,137,251,24,154,170,44,44,197,81,127,36,61,242,192,245,200,63,241,44,204,165,4,31,135,67,150,39,65,0,5,90,129,59,226,4,0,160,63,122,3,66,5,217,28,199,255,137,15,18,209,107,158,10,0,172,9,98,26,129,0,41,184,97,99,215,131,209,53,122,102,107,14,2,196,215,200,149,97,216,150,58,16,191,109,140,174,241,4,9,3,132,96,235,97,176,181,172,126,80,111,2,14,148,19,202,169,96,231,193,18,248,141,81,124,223,231,250,171,64,4,131,185,190,253,173,157,8,32,72,78,185,9,68,48,150,86,40,30,252,117,1,5,43,171,15,194,105,251,253,177,241,253,221,217,234,189,43,254,85,52,65,254,204,101,112,23,254,196,170,63,210,7,169,134,63,245,156,130,141,154,64,169,41,62,80,221,50,250,30,184,34,56,13,70,105,98,98,154,236,240,0,65,218,2,125,65,135,13,194,166,112,90,30,133,170,188,87,195,233,195,33,5,41,54,175,103,52,220,162,26,31,10,168,32,145,146,192,130,234,103,54,25,119,31,136,25,148,81,40,134,116,47,158,6,63,94,99,229,159,131,137,121,30,223,94,110,1,54,168,78,4,71,162,180,12,37,157,251,128,250,92,160,25,124,48,54,23,41,58,151,70,153,71,53,128,17,210,87,99,242,38,248,152,172,24,144,150,208,132,228,177,196,55,242,209,23,108,236,22,126,29,91,248,17,179,223,113,97,111,177,117,101,102,127,99,152,92,143,16,167,90,40,97,48,189,227,63,154,174,135,16,86,239,220,128,166,122,240,96,176,181,87,29,188,174,192,6,253,95,94,248,27,187,149,155,119,84,220,96,176,183,16,236,221,133,190,10,114,144,61,125,20,244,173,37,65,7,81,127,134,6,192,149,132,15,136,181,192,65,233,73,200,1,200,158,53,184,65,216,112,240,116,88,65,12,6,227,15,130,177,94,218,175,68,13,86,215,239,177,181,221,224,217,143,137,208,65,138,205,234,145,131,248,245,156,159,49,17,51,40,54,184,183,224,247,173,38,161,5,233,70,162,19,214,97,5,217,252,38,228,8,210,178,214,0,5,225,4,116,25,181,16,65,56,5,92,84,61,54,16,56,79,3,106,81,129,200,162,177,171,193,245,153,8,17,88,25,189,233,127,63,71,140,123,5,84,160,234,78,193,218,208,87,10,146,112,200,63,197,171,212,141,17,242,119,168,195,252,241,1,164,52,216,154,199,151,196,248,183,119,148,210,75,151,180,180,103,92,244,142,156,55,122,210,33,88,235,232,183,142,225,149,28,139,199,63,25,241,175,4,191,10,91,191,62,153,249,10,122,127,125,242,164,218,156,49,44,65,133,191,39,46,165,117,176,48,61,71,32,123,135,144,81,153,152,8,25,240,92,78,15,69,3,182,115,42,159,63,242,155,175,114,144,115,191,109,30,59,110,125,253,155,163,25,184,239,15,244,108,215,17,35,255,222,127,24,249,12,238,16,226,25,15,134,190,199,23,229,91,134,6,28,4,97,82,6,199,69,171,28,61,113,233,210,137,127,251,63,228,4,26,19,15,81,0,0};
const char managementJS[] PROGMEM = R"MOSJS(// Generated by tools/embed_web.py; edit web/management/*.js.
let firmwareBusy = false;
let settingsBusy = false;
const form = document.getElementById("settings");
for (const [k, v] of Object.entries(config)) {
  if (form.elements[k]) form.elements[k].value = v;
}
if (config.iface) {
  form.elements.iface.add(new Option(config.iface, config.iface));
  form.elements.iface.value = config.iface;
}
let lang = config.adminLanguage || "zh";
const msg = (key) => I18n.text(key, lang);
// Group device fields without changing their submitted names.
const snmpSection = form.elements.nas.closest("section");
snmpSection.classList.add("snmpSettings");
const snmpFields = snmpSection.querySelector(".fields");
let deviceDetails;
for (const [title, names] of [
  [
    "Device information",
    ["machineName", "nas"],
  ],
  ["Display colors", ["nasBackground", "cardBackground"]],
  ["Connection", ["snmpVersion", "port", "iface", "interval", "community"]],
  [
    "Security",
    [
      "snmpUser",
      "snmpAuth",
      "snmpPassword",
      "snmpPrivacy",
      "snmpPrivacyPassword",
    ],
  ],
]) {
  const group = document.createElement("fieldset");
  group.className = "snmpGroup";
  if (title !== "Security") group.classList.add("compactGroup");
  if (title === "Security") group.dataset.snmp = "3";
  const heading = document.createElement(title === "Device information" ? "h3" : "h4");
  heading.dataset.i18n = title;
  heading.textContent = title;
  group.append(heading);
  for (const name of names) {
    const field = form.elements[name];
    if (field) {
      const wrapper =
        field.closest("#privacyPasswordField") || field.closest("label");
      if (name === "community") wrapper.classList.add("wideField");
      group.append(wrapper);
    }
  }
  if (title === "Device information") {
    deviceDetails = group; group.classList.add("deviceDetails"); snmpFields.append(group);
  } else {
    group.classList.add("subGroup"); deviceDetails.append(group);
  }
}
// Keep general display options and clock colors in separate peer cards.
const desktopFields = form.elements.clockColor.closest(".fields");
const displayCards = document.createElement("div");
displayCards.className = "displayCards";
for (const [title, names] of [
  ["Display options", ["language", "home", "showSeconds", "brightness", "rotation", "sound"]],
  ["Clock palettes", ["clockColor", "minuteColor"]],
]) {
  const card = document.createElement("div"); card.className = "settings-group";
  if (title === "Clock palettes") card.classList.add("clockPalette");
  const heading = document.createElement("h3"); heading.dataset.i18n = title; heading.textContent = msg(title);
  const fields = document.createElement("div"); fields.className = "fields";
  for (const name of names) if (form.elements[name]) fields.append(form.elements[name].closest("label"));
  card.append(heading, fields); displayCards.append(card);
}
desktopFields.insertBefore(displayCards, desktopFields.querySelector(".desktop-groups"));
// Presets only edit the form; saving remains an explicit user action.
function colorPresets(anchor, headingKey, fields, choices) {
  const row = document.createElement("div");
  row.className = "colorPresets";
  const heading = document.createElement("strong");
  heading.dataset.i18n = headingKey;
  heading.textContent = msg(headingKey);
  const reset = form.elements[fields[0]].closest("label").querySelector("button");
  if (reset) reset.remove();
  row.append(heading);
  for (const [key, colors] of choices) {
    const button = document.createElement("button");
    button.type = "button";
    button.className = "secondary";
    const swatch = document.createElement("span");
    swatch.className = "colorSwatch";
    swatch.style.background = `linear-gradient(90deg, ${colors[0]} 50%, ${colors[1]} 50%)`;
    const title = document.createElement("span");
    title.dataset.i18n = key;
    title.textContent = msg(key);
    button.append(swatch, title);
    button.addEventListener("click", () => {
      fields.forEach((name, index) => {
        const input = form.elements[name];
        input.value = colors[index];

      });
      dirty(); // One local form update; selecting a palette never sends a save request.
    });
    row.append(button);
  }
  const groups = anchor.querySelector(":scope > .desktop-groups");
  anchor.insertBefore(row, groups || null);
}
colorPresets(form.elements.minuteColor.closest(".fields"), "Clock palettes", ["clockColor", "minuteColor"], [
  ["Default palette", ["#ffffff", "#ffa500"]],
  ["White & blue", ["#ffffff", "#58baff"]],
  ["Cyan & white", ["#42d9ed", "#ffffff"]],
  ["Green & gold", ["#92ed56", "#ffcb55"]],
  ["White & pink", ["#ffffff", "#ff80b5"]],
  ["Gold & white", ["#ffcb55", "#ffffff"]],
  ["Violet & cyan", ["#c5a3ff", "#42d9ed"]],
  ["Lime & white", ["#bbef66", "#ffffff"]],
  ["White & coral", ["#ffffff", "#ff927c"]],
]);
colorPresets(form.elements.cardBackground.closest("fieldset"), "Device palettes", ["nasBackground", "cardBackground"], [
  ["Default palette", ["#21455c", "#103040"]],
  ["Ocean", ["#68acc4", "#32738a"]],
  ["Forest", ["#74b08a", "#3c7b5e"]],
  ["Amber", ["#d6b24c", "#856737"]],
  ["Violet", ["#a58bc9", "#69477d"]],
  ["Rose", ["#d781a2", "#854761"]],
]);
let editedMachine = config.selectedMachine ?? config.activeMachine ?? 0;
const machineSelect = document.getElementById("machineSelect");
(config.machines || [{ name: config.nas }]).forEach((m, i) =>
  machineSelect.add(new Option(m.name || m.address || config.nas, i)),
);
machineSelect.value = editedMachine;
machineSelect.classList.add("hidden");
machineSelect.setAttribute("aria-label", msg("Device list"));
const machineList = document.createElement("div");
machineList.className = "machineList";
machineSelect
  .closest("section")
  .insertBefore(machineList, machineSelect.closest(".actions"));
(config.machines || []).forEach((m, i) => {
  const row = document.createElement("div");
  row.className = "machineItem" + (i == editedMachine ? " active" : "");
  const choose = document.createElement("button");
  choose.type = "button";
  choose.className = "choose";
  choose.textContent = i + 1 + ". " + (m.name || m.address || "SNMP");
  const address = document.createElement("small");
  address.textContent = m.address || "";
  choose.append(address);
  choose.onclick = () => {
    if (i != editedMachine) machineAction("select", i);
  };
  row.append(choose);
  for (const [label, offset, key] of [
    ["↑", -1, "Move up"],
    ["↓", 1, "Move down"],
  ]) {
    const move = document.createElement("button");
    move.type = "button";
    move.className = "move";
    move.textContent = label;
    move.dataset.i18nTitle = key;
    move.title = msg(key);
    move.setAttribute("aria-label", msg(key));
    move.disabled = i + offset < 0 || i + offset >= config.machines.length;
    move.onclick = () => machineAction("move", i + ":" + (i + offset));
    row.append(move);
  }
  machineList.append(row);
});
form.elements.machineIndex.value = editedMachine;
document.getElementById("addMachine").disabled =
  (config.machines || []).length >= 6;
document.getElementById("deleteMachine").disabled =
  (config.machines || []).length <= 1;
let machineBusy = false;
async function machineAction(action, index) {
  if (machineBusy) return;
  if (
    (action === "select" ? snmpSection.classList.contains("dirty") : [...form.querySelectorAll("section.dirty")].length) &&
    !confirm(msg("Discard unsaved changes?"))
  ) {
    machineSelect.value = editedMachine;
    form.elements.machineIndex.value = editedMachine;
    return;
  }
  if (action === "delete" && !confirm(msg("Delete this device?"))) return;
  machineBusy = true;
  try {
    const response = await post("/api/machine", { action, index: index ?? "" });
    if (response.ok) {
      const result = await response.json();
      if (action === "select") {
        editedMachine = result.selectedMachine;
        Object.assign(config, result);
        for (const name of ["machineName", "nas", "nasBackground", "cardBackground", "snmpVersion", "port", "iface", "interval", "snmpUser", "snmpAuth", "snmpPrivacy"]) {
          const field = form.elements[name];
          if (name === "iface") field.replaceChildren(new Option(msg("Automatic"), ""), ...(result.iface ? [new Option(result.iface, result.iface)] : []));
          field.value = result[name] ?? "";
          originals.set(field, field.value);
        }
        for (const name of ["community", "snmpPassword", "snmpPrivacyPassword"]) {
          form.elements[name].value = "";
          originals.set(form.elements[name], "");
        }
        form.elements.machineIndex.value = editedMachine;
        machineSelect.value = editedMachine;
        [...machineList.children].forEach((row, i) => row.classList.toggle("active", i === editedMachine));
        snmpOptions(); dirty();
        history.replaceState(null, "", "/manage?device=" + editedMachine + "#settings");
        return;
      }
      location.href = "/manage?device=" + (result.selectedMachine ?? result.activeMachine) + "#settings";
    } else
      document.getElementById("message").textContent = msg(
        (await response.text()).trim() || "Failed",
      );
  } catch (_) {
    document.getElementById("message").textContent = msg("Failed");
  } finally {
    machineBusy = false;
  }
}
machineSelect.onchange = () => machineAction("select", machineSelect.value);
document.getElementById("addMachine").onclick = () => machineAction("add");
document.getElementById("deleteMachine").onclick = () =>
  machineAction("delete", editedMachine);
function translate() {
  document.documentElement.lang = lang === "en" ? "en" : "zh-CN";
  document.getElementById("language").value = lang;
  I18n.apply(document, lang);
  document.querySelectorAll("[data-i18n-title]").forEach((el) => {
    el.title = msg(el.dataset.i18nTitle);
    el.setAttribute("aria-label", el.title);
  });
  machineSelect.setAttribute("aria-label", msg("Device list"));
  updateWifiLabels();
  updateSaveAction();
  document.getElementById("cityCodeLookup").title = msg(
    "Search for a city on China Weather. The nine digits in its weather page URL are the city code.",
  );
  document
    .getElementById("crop")
    .setAttribute("aria-label", msg("Photo crop preview"));
  if (typeof albumEntries !== "undefined") updateAlbumLabels();
  tick();
}
document.getElementById("language").onchange = async (e) => {
  if (settingsBusy) { e.target.value = lang; return; }
  settingsBusy = true;
  const previous = lang;
  lang = e.target.value;
  translate();
  try {
    const r = await post("/api/language", { language: lang });
    if (!r.ok) throw new Error("save");
    config.adminLanguage = lang;
  } catch (error) {
    lang = previous;
    translate();
  } finally { settingsBusy = false; }
};
document.querySelectorAll(".day").forEach((c) => {
  c.checked = !!((config.alarmDays ?? 31) & +c.value);
  c.onchange = () => {
    form.elements.alarmDays.value = [
      ...document.querySelectorAll(".day:checked"),
    ].reduce((n, c) => n | +c.value, 0);
  };
});
const originals = new Map(
  [...form.querySelectorAll("input,select")].map((el) => [
    el,
    el.type === "checkbox" ? el.checked : el.value,
  ]),
);
function updateSaveAction() {
  const key = "Save & restart";
  const label = document.getElementById("saveActionLabel");
  label.dataset.i18n = key;
  label.textContent = msg(key);
}
function dirty() {
  updateSaveAction();
  for (const card of form.querySelectorAll("section"))
    card.classList.toggle(
      "dirty",
      [
        ...card.querySelectorAll(
          "input[name]:not([type=hidden]),select[name],input.day",
        ),
      ].some(
        (el) =>
          (el.type === "checkbox" ? el.checked : el.value) !==
          originals.get(el),
      ),
    );
}
form.addEventListener("input", dirty);
form.addEventListener("change", dirty);
form.onsubmit = async (e) => {
  e.preventDefault();
  if (settingsBusy) return;
  const message = document.getElementById("message"),
    button = document.querySelector("#settingsActions button");
  if (!+form.elements.alarmDays.value) {
    message.textContent = msg("Select at least one weekday");
    return;
  }
  button.disabled = true;
  settingsBusy = true;
  let validatingCity = true;
  message.textContent = msg("Checking city code…");
  try {
    const cityCode = form.elements.cityCode.value.trim();
    form.elements.cityCode.value = cityCode;
    const deadline = Date.now() + 12000;
    let status = "pending";
    while (Date.now() < deadline) {
      const checkAbort = new AbortController();
      const checkTimer = setTimeout(() => checkAbort.abort(), 4000);
      const checked = await fetch("/api/weather/check", {
        signal: checkAbort.signal,
        method: "POST", body: new URLSearchParams({code:cityCode,token:form.elements.token.value}),
      }).finally(() => clearTimeout(checkTimer));
      if (checked.status === 401 || checked.status === 403) throw new Error(msg("Reload page"));
      const result = await checked.json(); status = result.status;
      if (status !== "pending" && status !== "busy") break;
      await new Promise(resolve => setTimeout(resolve, 500));
    }
    if (status !== "valid") {
      message.textContent = status === "forecast" ? msg("This city provides forecasts only; live weather is unavailable.") :
        status === "invalid" ? msg("City code not found or unsupported.") : msg("Weather service unavailable. Try again.");
      message.style.color = "#b84b42"; button.disabled = false; return;
    }
    validatingCity = false;
    message.textContent = msg("Saving…");
    const response = await fetch(form.action, {
      method: "POST",
      signal: AbortSignal.timeout(12000),
      body: new URLSearchParams(new FormData(form)),
    });
    const content = await response.text();
    message.textContent = (response.headers.get("Content-Type") || "").includes(
      "text/html",
    )
      ? response.ok
        ? msg("Saved.")
        : msg("Failed")
      : msg(content);
    message.style.color = response.ok ? "#147e87" : "#b84b42";
    if (response.ok) {
      for (const name of ["pass", "admin", "community", "snmpPassword", "snmpPrivacyPassword"])
        form.elements[name].value = "";
      // A successful settings save always restarts the device; do not submit
      // another restart request or allow a second save during the boot window.
      setTimeout(() => { location.href = "/manage"; }, 8000);
    } else button.disabled = false;
  } catch (error) {
    message.textContent = validatingCity ? msg("Weather service unavailable. Try again.") : msg("Failed");
    message.style.color = "#b84b42";
    button.disabled = false;
  } finally { settingsBusy = false; }
};
document.querySelectorAll("[data-tab]").forEach(
  (b) =>
    (b.onclick = () => {
      document
        .querySelectorAll("[data-tab]")
        .forEach((x) => x.classList.toggle("active", x === b));
      ["screen", "photos", "maintenance"].forEach((n) =>
        document
          .getElementById("tab-" + n)
          .classList.toggle("hidden", b.dataset.tab !== n),
      );
      form.classList.toggle("hidden", b.dataset.tab !== "settings");
      document
        .getElementById("settingsActions")
        .classList.toggle("hidden", b.dataset.tab !== "settings");
      if (b.dataset.tab === "settings") loadWeatherCities();
      if (b.dataset.tab === "photos" && !albumLoaded) refreshAlbum();
    }),
);
const format = (v, u) => (v < 0 || v == null ? "--" : v.toFixed(1) + u);
const rate = (v) =>
  v < 0
    ? "--"
    : v >= 1048576
      ? (v / 1048576).toFixed(1) + " MB/s"
      : (v / 1024).toFixed(1) + " KB/s";
function weatherOptions() {
  const enabled = form.elements.weatherEnabled.value === "1";
  form.elements.home.querySelector('option[value="5"]').disabled = !enabled;
  document.getElementById("weatherPage").disabled = !enabled;
  if (!enabled && form.elements.home.value === "5")
    form.elements.home.value = "6";
}
form.elements.weatherEnabled.onchange = weatherOptions;
weatherOptions();
let statusBusy = false;
async function tick() {
  if (firmwareBusy || settingsBusy || statusBusy || document.hidden) return;
  statusBusy = true;
  try {
    const response = await fetch("/api/status?management=1", {
      cache: "no-store",
      signal: AbortSignal.timeout(6000),
    });
    if (response.status === 401) {
      location.href = "/manage";
      return;
    }
    const d = await response.json();
    updateInterfaces(d);
    updateCurrentWifi(d);
    document.getElementById("state").textContent = d.online
      ? msg("Online")
      : msg("Offline");
    for (const k of ["cpu", "memory"])
      document.getElementById(k).textContent = format(d[k], "%");
    document.getElementById("temperature").textContent = format(
      d.temperature,
      "°C",
    );
    document.getElementById("traffic").textContent =
      rate(d.rx) + " / " + rate(d.tx);
    document.getElementById("traffic").style.fontSize = "15px";
    document.getElementById("details").textContent =
      (d.model || "SNMP") +
      " · " +
      (d.interface || "--") +
      " · " +
      (d.disks || []).length +
      msg(" disks / ") +
      (d.volumes || []).length +
      msg(" volumes");
    const w = d.weather;
    document.getElementById("weatherDetails").textContent =
      w && w.valid
        ? (lang === "en"
            ? (w.cityEnglish || "Tianjin") + " · " + (w.conditionEnglish || "")
            : (w.city || "") + " · " + (w.condition || "")) +
          " · " +
          w.temperature +
          "°C · " +
          w.humidity +
          "%"
        : msg("Waiting for weather");
    const a = d.alarm;
    if (a)
      document.getElementById("alarmDetails").textContent =
        [msg("Idle"), msg("Ringing"), msg("Snoozed"), msg("Dismissed today")][
          a.state
        ] + (a.state === 2 ? " · " + a.remaining + "s" : "");
  } catch (e) {
    document.getElementById("state").textContent = msg("Disconnected");
  } finally {
    statusBusy = false;
  }
}
async function post(path, data = {}) {
  return fetch(path, {
    signal: AbortSignal.timeout(12000),
    method: "POST",
    body: new URLSearchParams({ ...data, token: form.elements.token.value }),
  });
}
async function setPage(page) {
  await post("/api/page", { page });
  tick();
}
async function testBeep() {
  const r = await post("/api/beep");
  document.getElementById("message").textContent = r.ok
    ? msg("Sound sent")
    : msg("Failed");
}
async function alarmAction(a) {
  const r = await post("/api/alarm/" + a, {
    tune: form.elements.alarmTune.value,
  });
  if (!r.ok)
    document.getElementById("alarmDetails").textContent =
      msg("No active alarm");
  else tick();
}
async function resetSettings() {
  const r = await post("/api/reset/request");
  if (!r.ok) return;
  const d = await r.json();
  if (!confirm(msg("Clear all settings and photos? This cannot be undone.")))
    return;
  const done = await post("/api/reset/confirm", { nonce: d.nonce });
  if (done.ok) {
    alert(msg("Reset complete. Reconnect to the device Wi-Fi."));
    location.reload();
  }
}

const crop = document.getElementById("crop"),
  cropCtx = crop.getContext("2d");
let photo = null,
  angle = 0,
  zoom = 1,
  offsetX = 0,
  offsetY = 0,
  drag = null;
let albumEntries = [];
function drawCrop() {
  cropCtx.fillStyle = "#0b1421";
  cropCtx.fillRect(0, 0, 240, 240);
  if (!photo) return;
  let w = angle % 180 ? photo.height : photo.width,
    h = angle % 180 ? photo.width : photo.height,
    scale = Math.max(240 / w, 240 / h) * zoom;
  offsetX = Math.max(
    -(w * scale - 240) / 2,
    Math.min((w * scale - 240) / 2, offsetX),
  );
  offsetY = Math.max(
    -(h * scale - 240) / 2,
    Math.min((h * scale - 240) / 2, offsetY),
  );
  cropCtx.save();
  cropCtx.translate(120 + offsetX, 120 + offsetY);
  cropCtx.rotate((angle * Math.PI) / 180);
  cropCtx.scale(scale, scale);
  cropCtx.drawImage(photo, -photo.width / 2, -photo.height / 2);
  cropCtx.restore();
}
function resetCrop() {
  zoom = 1;
  offsetX = offsetY = 0;
  document.getElementById("zoom").value = 1;
  drawCrop();
}
function rotatePhoto() {
  angle = (angle + 90) % 360;
  resetCrop();
}
document.getElementById("zoom").oninput = (e) => {
  zoom = +e.target.value;
  drawCrop();
};
document.getElementById("photoFile").onchange = (e) => {
  const file = e.target.files[0];
  if (!file) return;
  const url = URL.createObjectURL(file),
    img = new Image();
  img.onload = () => {
    photo = img;
    angle = 0;
    resetCrop();
    URL.revokeObjectURL(url);
    document.getElementById("uploadPhoto").disabled =
      !document.getElementById("photoSlot").value;
  };
  img.onerror = () => {
    URL.revokeObjectURL(url);
    document.getElementById("photoMessage").textContent =
      msg("Cannot open photo");
  };
  img.src = url;
};
crop.onpointerdown = (e) => {
  if (!photo) return;
  drag = [e.clientX, e.clientY, offsetX, offsetY];
  crop.setPointerCapture(e.pointerId);
};
crop.onpointermove = (e) => {
  if (!drag) return;
  offsetX = drag[2] + ((e.clientX - drag[0]) * 240) / crop.clientWidth;
  offsetY = drag[3] + ((e.clientY - drag[1]) * 240) / crop.clientHeight;
  drawCrop();
};
crop.onpointerup = crop.onpointercancel = () => (drag = null);
let albumRefreshBusy = false, albumLoaded = false;
function updateAlbumLabels() {
  document.getElementById("photoCount").textContent = msg("Photos") + " · " + albumEntries.length + " / 7";
  document.querySelectorAll("#album small[data-slot]").forEach(el => {
    el.textContent = msg("Photo ") + (Number(el.dataset.slot) + 1);
  });
  for (const option of document.getElementById("photoSlot").options) {
    option.textContent = option.value === "add" ? msg("Add new photo (automatic slot)") :
      msg("Replace photo ") + (Number(option.value) + 1) + msg(" (occupied)");
  }
}
async function refreshAlbum() {
  if (albumRefreshBusy) return;
  albumRefreshBusy = true;
  try {
    const entries = await (await fetch("/api/album")).json();
    albumEntries = entries;
    const slots = document.getElementById("photoSlot"),
      previous = slots.value;
    slots.replaceChildren();
    document.getElementById("photoCount").textContent =
      msg("Photos") + " · " + entries.length + " / 7";
    if (entries.length < 7) {
      let o = document.createElement("option");
      o.value = "add";
      o.textContent = msg("Add new photo (automatic slot)");
      slots.append(o);
    }
    for (const entry of entries) {
      let o = document.createElement("option");
      o.value = entry.slot;
      o.textContent =
        msg("Replace photo ") + (entry.slot + 1) + msg(" (occupied)");
      slots.append(o);
    }
    if (previous === "add" && entries.length < 7) slots.value = "add";
    document.getElementById("uploadPhoto").disabled = !photo || !slots.value;
    const container = document.getElementById("album");
    container.replaceChildren();
    for (const entry of entries) {
      let card = document.createElement("div"),
        img = document.createElement("img"),
        caption = document.createElement("small"),
        button = document.createElement("button");
      img.decoding = "async";
      caption.dataset.slot = entry.slot;
      caption.textContent = msg("Photo ") + (entry.slot + 1);
      button.textContent = msg("Delete");
      button.dataset.i18n = "Delete";
      button.onclick = async () => {
        const r = await fetch("/api/album/delete", {
          method: "POST",
          body: new URLSearchParams({
            slot: entry.slot,
            token: form.elements.token.value,
          }),
        });
        if (r.ok) refreshAlbum();
      };
      card.append(img, caption, button);
      container.append(card);
      // Only load the visible album, one image at a time. ESP8266 cannot afford
      // a browser's six parallel TCP image downloads alongside settings saves.
      await new Promise(resolve => {
        img.onload = img.onerror = resolve;
        img.src = entry.url + "&v=" + Date.now();
      });
    }
    albumLoaded = true;
  } catch (e) {
    document.getElementById("photoMessage").textContent =
      msg("Cannot load photos");
  } finally {
    albumRefreshBusy = false;
  }
}
async function uploadPhoto() {
  if (!photo) return;
  const destination = document.getElementById("photoSlot").value;
  if (!destination) return;
  if (destination !== "add" && !confirm(msg("Replace this photo?"))) return;
  let button = document.getElementById("uploadPhoto");
  button.disabled = true;
  try {
    let blob;
    for (let q = 0.9; q >= 0.3; q -= 0.1) {
      blob = await new Promise((resolve) =>
        crop.toBlob(resolve, "image/jpeg", q),
      );
      if (blob && blob.size <= 49152) break;
    }
    if (!blob || blob.size > 49152) throw Error(msg("Photo exceeds 48KB"));
    let data = new FormData();
    data.append("file", blob, "photo.jpg");
    let r = await fetch(
      "/api/album/upload?" +
        (destination === "add" ? "mode=add" : "slot=" + destination),
      {
        method: "POST",
        headers: { "X-CSRF-Token": form.elements.token.value },
        body: data,
      },
    );
    if (!r.ok) throw Error(await r.text());
    document.getElementById("photoMessage").textContent = msg("Photo saved");
    refreshAlbum();
  } catch (e) {
    document.getElementById("photoMessage").textContent = e.message;
  } finally {
    button.disabled = !photo || !document.getElementById("photoSlot").value;
  }
}
document.getElementById("photoSlot").onchange = () =>
  (document.getElementById("uploadPhoto").disabled = !photo);
drawCrop();


let wifiNetworks = [],
  wifiScanning = false,
  wifiScanned = false,
  wifiScanMessage = "",
  currentWifi = null;
function updateWifiLabels() {
  const select = document.getElementById("manageNetworks"),
    selected = select.value;
  select.replaceChildren(
    new Option(msg("Choose a network"), ""),
    ...wifiNetworks.map(
      (n) => new Option(n.ssid + " · " + n.rssi + " dBm", n.ssid),
    ),
  );
  select.value = selected;
  document.getElementById("manageScanStatus").textContent = wifiScanMessage
    ? msg(wifiScanMessage)
    : "";
  if (currentWifi)
    document.getElementById("wifiCurrent").textContent =
      currentWifi.wifiConnected
        ? msg("Current Wi-Fi") + " · " + currentWifi.wifiSsid
        : msg("Not connected");
}
function updateCurrentWifi(d) {
  currentWifi = d;
  updateWifiLabels();
}
async function scanManagementWifi(refresh = false) {
  if (wifiScanning || firmwareBusy) return;
  wifiScanning = true;
  wifiScanned = true;
  const button = document.getElementById("manageRescan");
  button.disabled = true;
  wifiScanMessage = "Scanning…";
  updateWifiLabels();
  try {
    const r = await fetch("/api/wifi/scan" + (refresh ? "?refresh=1" : ""), {
      cache: "no-store",
    });
    if (r.status === 401) {
      location.replace("/manage");
      return;
    }
    if (!r.ok) throw Error();
    const d = await r.json();
    if (d.scanning) {
      setTimeout(() => scanManagementWifi(), 1000);
      return;
    }
    wifiNetworks = d.networks || [];
    wifiScanMessage = wifiNetworks.length ? "" : "No networks found";
  } catch (_) {
    wifiScanMessage = "Wi-Fi scan failed";
  } finally {
    wifiScanning = false;
    button.disabled = false;
    updateWifiLabels();
  }
}
document.getElementById("manageNetworks").onchange = (e) => {
  if (e.target.value) {
    form.elements.ssid.value = e.target.value;
    dirty();
  }
};
// Discover only when the network selector is used; opening unrelated settings
// must not start an SDK scan during a configuration save or page transfer.
document.getElementById("manageNetworks").onfocus = () => {
  if (!wifiScanned) scanManagementWifi();
};
document.getElementById("manageRescan").onclick = () =>
  scanManagementWifi(true);

translate();
setInterval(tick, 3000);

function resetColors(group) {
  const defaults =
    group === "clock"
      ? { clockColor: "#ffffff", minuteColor: "#ffa500" }
      : { nasBackground: "#21455c" };
  for (const [name, value] of Object.entries(defaults))
    form.elements[name].value = value;
  dirty();
}

function updateInterfaces(d) {
  if (editedMachine !== config.activeMachine) d = {}; // Do not offer another device's ports.
  const select = form.elements.iface,
    selected = select.value;
  const options = [
    {
      index: "",
      name: msg("Automatic") + (d.interface ? " · " + d.interface : ""),
    },
  ];
  for (const item of d.interfaces || [])
    options.push({
      index: String(item.index),
      name: item.name + " · " + msg(item.up ? "Connected" : "Not connected"),
    });
  if (selected && !options.some((x) => x.index === selected))
    options.push({ index: selected, name: selected });
  const key = JSON.stringify(options);
  if (select.dataset.options !== key) {
    select.replaceChildren(...options.map((x) => new Option(x.name, x.index)));
    select.value = selected;
    select.dataset.options = key;
  }
}


async function upgradeFirmware() {
  if (firmwareBusy) return;
  const input = document.getElementById("firmwareFile"),
    keep = document.getElementById("firmwareKeep"),
    button = document.getElementById("firmwareUpgrade"),
    progress = document.getElementById("firmwareProgress"),
    message = document.getElementById("firmwareMessage"),
    file = input.files[0];
  const feedback = (key, color = "#147e87") => {
    message.dataset.i18n = key;
    message.textContent = msg(key);
    message.style.color = color;
  };
  if (
    !file ||
    !file.name.toLowerCase().endsWith(".bin") ||
    file.size < 4096 ||
    file.size > 0x3fa000
  ) {
    feedback("Choose a valid program .bin (up to 1 MiB).", "#b84b42");
    return;
  }
  if (
    !confirm(
      keep.checked
        ? msg("Upgrade firmware and keep settings and photos?")
        : msg("Upgrade firmware and delete all settings and photos?"),
    )
  )
    return;
  firmwareBusy = true;
  input.disabled = keep.disabled = button.disabled = true;
  progress.value = 0;
  progress.classList.remove("hidden");
  feedback("Uploading… Do not disconnect power.");
  try {
    const bytes = new Uint8Array(await file.arrayBuffer()),
      body = new FormData();
    const program = FirmwareBundle.program(bytes);
    body.append("firmware", new Blob([program]), file.name);
    if (bytes[0] !== 0xe9) throw new Error("invalid");
    const result = await new Promise((resolve, reject) => {
      const request = new XMLHttpRequest();
      request.open(
        "POST",
        "/api/firmware?" +
          new URLSearchParams({
            size: program.length,
            crc: firmwareCRC(program),
            keep: keep.checked ? "1" : "0",
          }),
      );
      request.setRequestHeader("X-CSRF-Token", form.elements.token.value);
      request.timeout = 180000;
      request.upload.onprogress = (e) => {
        if (e.lengthComputable)
          progress.value = Math.round((e.loaded / e.total) * 100);
      };
      request.upload.onload = () => {
        feedback("Checking firmware… Do not disconnect power.");
      };
      request.onload = () => {
        let d;
        try {
          d = JSON.parse(request.responseText);
        } catch (e) {
          return reject(new Error("write"));
        }
        if (request.status === 200 && d.updated) resolve(d);
        else reject(new Error(d.error || "write"));
      };
      request.onerror = request.ontimeout = () => reject(new Error("network"));
      request.send(body);
    });
    progress.value = 100;
    feedback(
      result.simulated
        ? "Simulation passed. No firmware was flashed."
        : result.preserved
          ? "Upgrade complete. 1. Wait for restart. 2. Reopen management at the same address."
          : "Upgrade complete. 1. Wait for restart. 2. Connect to the device hotspot. 3. Set up Wi-Fi again.",
    );
    if (!result.simulated) return;
  } catch (error) {
    feedback(
      error.message === "network"
        ? "Connection interrupted. Check the device before retrying."
        : ["login_required", "csrf"].includes(error.message)
          ? "Session expired. Reload and sign in again."
          : error.message === "size"
            ? "Firmware is too large for the program partition."
            : "Upgrade failed: invalid, incompatible or incomplete firmware. Data was not cleared.",
      "#b84b42",
    );
  }
  firmwareBusy = false;
  input.disabled = keep.disabled = button.disabled = false;
}

function snmpOptions() {
  const v = form.elements.snmpVersion.value;
  document
    .querySelectorAll("[data-snmp]")
    .forEach((e) => e.classList.toggle("hidden", e.dataset.snmp !== v));
  form.elements.snmpUser.required = v === "3";
  form.elements.snmpPassword.minLength = v === "3" ? 8 : 0;
  const encrypted = v === "3" && form.elements.snmpPrivacy.value !== "none";
  document
    .getElementById("privacyPasswordField")
    .classList.toggle("hidden", !encrypted);
  form.elements.snmpPrivacyPassword.minLength = encrypted ? 8 : 0;
}
form.elements.snmpPrivacy.addEventListener("change", snmpOptions);
form.elements.snmpVersion.addEventListener("change", snmpOptions);
snmpOptions();

async function signOut(event) {
  event.preventDefault();
  try {
    const response = await post("/logout");
    if (response.ok || response.status === 401) location.replace("/manage");
    else throw Error();
  } catch (_) {
    document.getElementById("message").textContent = msg("Failed");
  }
}

if (location.hash === "#settings")
  document.querySelector('[data-tab="settings"]').click();

document.getElementById("rediscoverStorage").onclick = async (e) => {
  const button = e.currentTarget,
    feedback = document.getElementById("storageFeedback");
  button.disabled = true;
  try {
    const r = await post("/api/storage/rediscover", {});
    feedback.dataset.i18n = r.ok
      ? "Storage discovery queued"
      : "Discovery already requested";
    feedback.textContent = msg(feedback.dataset.i18n);
  } catch (error) {
    feedback.dataset.i18n = "Failed";
    feedback.textContent = msg("Failed");
  }
  setTimeout(() => (button.disabled = false), 60000);
};

// Monitoring selection is explicit and independent of the configuration editor.
document.getElementById("switchMonitor").onclick = async (event) => {
  const button = event.currentTarget;
  button.disabled = true;
  try {
    const response = await post("/api/machine", {action: "activate"});
    if (!response.ok) throw new Error("Switch failed");
    const result = await response.json();
    config.activeMachine = result.activeMachine;
    await tick();
  } catch (_) { document.getElementById("storageFeedback").textContent = msg("Failed"); }
  finally { button.disabled = false; }
};

// The city catalog loads only when settings are opened; selected codes remain editable.
var weatherCitiesLoading = false;
function loadWeatherCities() {
  if (weatherCitiesLoading) return;
  weatherCitiesLoading = true;
  const script = document.createElement("script");
  script.src = "/weather-cities.js?v=china-city-20261011";
  script.onload = () => {
    const province = document.getElementById("weatherProvince"), city = document.getElementById("weatherCity");
    const rows = window.weatherCities;
    province.replaceChildren(new Option("—", ""));
    for (const name of new Set(rows.map(r => r[0]))) province.add(new Option(name, name));
    function cities(code) {
      city.replaceChildren(new Option("—", ""));
      for (const row of rows.filter(r => r[0] === province.value)) city.add(new Option(row[1], row[2]));
      city.value = code || "";
    }
    const current = rows.find(r => r[2] === form.elements.cityCode.value);
    if (current) province.value = current[0];
    cities(current && current[2]);
    province.disabled = city.disabled = false;
    province.onchange = () => cities();
    city.onchange = () => { if (city.value) { form.elements.cityCode.value = city.value; form.elements.cityCode.dispatchEvent(new Event("input", {bubbles:true})); } };
    form.elements.cityCode.addEventListener("input", () => {
      const row = rows.find(r => r[2] === form.elements.cityCode.value);
      province.value = row ? row[0] : ""; cities(row && row[2]);
    });
  };
  script.onerror = () => { weatherCitiesLoading = false; document.getElementById("state").textContent = msg("Failed"); };
  document.body.append(script);
}
)MOSJS";
const char managementJSGzip[] PROGMEM = {31,139,8,0,0,0,0,0,2,255,181,125,219,114,220,70,178,224,59,191,162,4,175,101,244,184,9,94,68,221,72,211,10,137,178,102,116,44,89,10,81,62,30,7,131,49,7,221,168,102,195,68,3,61,0,154,77,142,134,17,231,105,223,55,118,191,97,159,247,109,223,207,167,204,151,108,94,234,138,91,147,158,217,137,24,139,13,100,101,101,85,101,101,229,173,18,59,59,226,143,50,151,101,92,203,68,76,110,68,93,20,89,181,35,23,19,153,252,101,45,39,209,242,230,72,200,36,173,5,252,216,89,196,121,124,33,23,50,175,119,254,16,253,86,69,91,153,172,197,44,45,23,235,184,148,175,86,213,141,56,22,179,56,171,228,17,189,169,100,93,167,249,69,229,191,153,22,121,5,173,138,114,1,207,146,98,186,66,124,209,133,172,127,200,8,245,171,155,183,73,24,232,182,193,232,104,11,96,69,200,205,206,46,199,226,234,92,20,51,241,97,242,155,156,214,17,52,40,83,89,225,235,89,122,49,26,137,47,91,66,164,51,17,98,7,145,100,148,213,217,229,249,72,52,159,68,87,113,182,146,64,196,213,209,214,237,22,182,97,36,81,58,139,167,146,49,121,109,248,69,20,39,73,152,203,181,248,176,172,211,34,247,26,141,133,135,2,104,239,70,161,123,118,161,145,8,156,181,44,206,47,236,171,56,89,164,249,59,120,180,130,153,23,127,255,187,8,254,54,15,244,36,46,42,132,12,47,229,205,72,28,127,47,222,238,61,203,163,90,94,215,248,100,76,136,128,130,29,88,225,178,88,45,69,34,175,210,169,132,245,146,89,82,137,117,90,207,139,85,45,166,115,0,131,153,22,245,92,166,165,168,86,147,69,90,35,51,228,241,66,194,18,115,79,85,190,88,158,194,124,195,128,113,37,189,33,229,113,21,77,179,162,146,85,141,235,70,64,184,108,78,27,120,31,87,213,187,180,170,105,246,2,126,101,87,216,118,242,134,169,59,118,123,140,254,186,146,229,205,41,116,56,173,139,50,12,34,30,2,54,196,9,227,113,189,150,117,156,102,149,207,45,117,90,103,176,40,52,20,226,154,51,88,16,252,191,16,193,107,158,142,52,199,209,196,68,244,152,222,156,5,139,120,58,79,115,249,19,52,11,198,34,128,1,6,231,248,142,254,115,22,188,78,171,101,22,223,192,18,101,69,89,1,196,25,130,188,138,167,151,23,48,209,121,130,109,166,113,153,56,79,206,85,211,147,34,207,213,12,97,51,28,228,191,203,178,226,223,193,178,40,107,252,151,248,129,254,200,107,89,2,179,16,198,98,177,88,229,105,125,163,145,241,48,96,142,86,37,62,85,196,211,127,225,57,162,254,185,146,165,122,174,158,188,92,213,115,255,201,71,88,152,117,81,38,141,167,101,122,21,79,111,58,31,54,90,232,153,57,231,29,195,51,127,65,28,231,236,239,105,41,65,196,168,45,30,6,188,130,178,14,104,139,16,52,243,8,206,57,180,163,254,136,109,131,35,181,161,105,45,197,131,227,99,103,204,35,183,169,101,47,152,170,101,60,173,185,253,200,71,112,220,137,32,137,235,24,200,137,176,91,236,254,17,117,203,99,153,203,56,73,105,79,246,140,198,65,220,193,84,226,133,8,230,143,2,113,8,255,28,48,53,10,163,233,53,133,141,11,232,9,143,251,30,247,50,112,76,13,157,184,175,153,228,120,185,148,121,18,42,88,45,106,52,235,35,203,35,195,19,235,243,202,232,241,208,220,55,119,241,25,2,158,31,17,24,137,79,4,210,237,116,203,117,137,157,150,226,88,61,21,140,203,238,254,175,150,62,139,208,110,134,73,6,185,213,128,204,226,137,204,120,54,116,151,68,50,205,162,101,245,145,238,179,185,196,235,52,145,10,187,198,225,77,139,106,166,94,222,110,241,255,27,108,208,177,90,122,200,158,80,129,201,34,228,71,221,236,230,193,2,61,142,32,211,228,80,59,34,230,86,72,56,6,85,47,157,232,64,4,107,206,245,201,232,64,6,103,6,200,247,31,165,92,138,11,58,198,51,145,40,233,84,208,241,84,137,56,79,4,76,251,244,82,201,43,24,46,156,204,203,24,143,124,177,148,176,156,40,171,140,164,79,100,117,89,23,86,14,251,178,158,16,157,32,30,187,148,142,56,86,40,152,130,19,68,59,32,3,146,244,10,219,184,208,190,12,112,223,4,119,17,236,70,50,171,177,147,140,205,212,225,137,50,116,94,176,68,175,230,197,26,100,64,145,39,8,19,76,202,244,98,94,231,178,162,95,101,81,171,227,0,0,125,233,77,211,184,140,225,212,169,37,99,183,19,130,224,112,86,175,106,201,63,207,155,50,17,167,121,227,116,16,84,67,20,170,131,114,251,162,67,30,18,31,55,232,26,57,72,28,161,136,64,31,25,134,55,205,29,197,27,74,47,32,108,80,104,245,136,44,80,80,152,78,167,191,153,230,172,13,19,193,112,254,84,40,78,27,150,117,109,237,143,100,219,72,99,84,187,168,3,164,37,158,152,108,156,76,95,218,142,21,46,220,161,46,251,42,40,108,49,66,125,206,219,76,81,154,195,121,92,191,146,208,179,12,221,118,99,127,215,181,244,29,245,150,25,160,34,178,96,215,127,44,37,172,67,37,138,60,187,97,29,29,84,56,218,175,32,129,226,43,92,211,82,46,226,148,100,128,144,215,203,44,157,2,208,10,136,16,49,43,86,91,179,85,206,74,29,201,6,133,49,140,243,233,188,40,199,122,81,127,68,109,146,7,12,10,238,188,0,137,84,185,140,93,22,235,205,219,92,32,152,191,154,110,159,247,57,111,131,170,46,139,252,98,240,44,181,164,247,31,168,200,157,22,206,97,81,34,169,117,66,242,12,156,237,158,183,249,164,185,98,147,85,93,179,18,204,123,149,16,142,24,111,4,139,82,92,201,208,204,201,240,65,126,70,186,60,139,110,146,116,222,2,104,138,185,195,129,41,115,41,18,10,60,170,111,150,180,16,234,165,247,174,33,130,80,86,198,229,141,130,81,250,250,58,174,167,243,161,117,90,198,166,75,6,238,96,128,83,122,17,120,80,85,125,147,201,104,98,244,103,0,254,143,12,52,242,184,132,61,0,211,132,216,159,239,38,18,54,226,127,251,194,115,3,235,114,43,30,239,126,237,60,217,227,39,163,255,112,137,86,98,243,78,52,19,108,147,179,46,153,165,244,219,54,75,93,42,94,50,83,169,22,152,199,54,22,86,32,90,136,36,249,225,10,48,160,180,198,99,28,165,117,58,189,132,35,37,36,203,78,171,97,74,130,1,123,252,0,38,74,72,42,211,24,14,244,68,94,123,112,122,176,105,190,92,213,67,170,30,233,94,8,228,152,164,52,119,132,19,128,20,212,173,209,178,146,180,172,111,128,121,5,72,160,15,185,20,112,170,128,210,65,230,252,106,9,83,5,103,65,69,251,0,183,112,172,15,37,145,203,43,16,59,21,204,3,72,35,20,79,18,118,3,236,26,56,157,182,220,14,156,13,193,115,163,244,28,207,176,192,211,131,101,84,115,227,29,86,211,2,120,250,123,209,146,153,136,70,181,241,228,48,116,56,214,88,65,77,205,87,89,70,162,219,19,136,190,18,228,156,242,29,90,208,88,220,83,79,24,107,229,69,206,226,85,86,235,118,212,236,171,25,253,15,219,192,159,241,227,221,93,163,140,252,50,79,97,94,31,138,9,172,91,11,246,241,179,73,12,127,26,197,229,6,78,128,135,98,141,77,24,246,96,63,121,46,19,133,151,154,105,216,63,150,82,34,240,69,145,37,12,251,124,95,38,143,159,40,216,233,228,241,227,22,13,203,52,191,236,160,247,217,238,196,194,254,17,240,249,52,40,100,29,52,252,123,90,160,101,255,80,76,129,112,6,158,62,142,31,41,196,138,118,13,252,46,93,72,31,241,100,34,103,79,158,116,33,214,4,79,139,146,172,234,38,197,207,247,159,78,149,230,118,52,196,2,190,121,111,185,192,218,181,99,99,94,120,124,176,209,81,48,200,13,251,123,7,143,31,79,137,214,189,221,71,187,7,150,27,62,76,165,158,169,39,207,226,233,244,128,128,30,237,63,125,244,44,54,64,111,128,225,171,154,161,158,30,76,118,225,21,65,77,159,78,30,75,3,245,114,49,65,183,1,2,37,79,38,251,7,220,223,179,199,79,158,62,122,218,88,33,134,138,129,219,166,207,9,234,201,243,131,167,79,237,210,124,130,105,81,152,158,62,219,139,247,21,166,131,167,79,246,204,44,227,66,163,246,34,147,247,236,118,177,206,47,22,36,246,197,139,23,198,45,6,242,229,74,58,207,119,141,79,140,159,177,72,24,242,48,122,128,40,31,180,39,79,189,32,113,112,246,133,180,202,67,221,47,44,159,184,61,31,89,9,188,0,241,139,162,23,134,235,33,108,122,9,23,17,105,169,128,114,129,175,96,29,8,191,69,139,120,70,227,45,160,195,199,163,229,178,55,69,77,160,134,170,63,79,147,68,210,41,230,131,1,99,190,172,235,50,5,201,42,195,32,46,211,120,155,21,152,49,29,92,154,97,51,64,68,74,166,55,163,136,126,179,154,231,0,251,103,189,243,34,104,208,5,115,215,225,64,196,167,158,164,118,48,140,69,115,252,90,10,179,98,203,58,114,231,130,118,45,222,191,64,151,85,125,188,173,229,34,16,223,138,48,5,203,172,193,214,47,68,32,152,111,201,25,228,218,96,112,44,193,0,238,168,190,49,112,151,242,166,222,248,58,22,61,115,95,251,26,75,10,212,238,193,255,131,72,16,225,61,140,26,156,254,244,254,163,75,178,126,55,160,73,45,226,76,185,120,20,112,83,89,242,58,112,73,84,26,128,122,237,142,186,200,73,51,66,183,183,163,241,160,142,157,138,7,141,25,31,105,46,121,73,76,129,188,69,123,29,87,157,180,138,134,6,206,93,180,20,112,218,34,99,80,189,103,176,127,198,168,255,25,143,3,202,184,127,252,247,255,1,24,183,247,64,180,189,7,189,30,180,32,118,22,243,187,255,9,239,204,171,164,88,231,202,147,236,171,239,104,16,220,89,121,71,224,110,213,157,222,248,108,9,79,220,151,254,2,208,200,156,183,174,166,251,89,233,201,70,221,229,246,234,169,175,231,210,171,13,194,5,161,93,112,176,127,227,73,38,19,197,130,60,187,226,59,177,139,204,224,60,249,222,156,6,122,27,71,153,204,47,234,185,131,171,201,20,141,101,167,73,24,19,210,224,80,237,78,141,127,212,214,59,17,220,104,157,174,60,83,239,1,20,53,68,14,76,185,106,161,18,1,168,57,247,137,237,222,211,8,88,93,1,129,53,105,231,6,40,232,147,98,60,11,56,63,79,6,240,38,64,94,45,127,31,234,239,142,197,30,159,207,10,192,15,228,197,213,77,62,21,198,131,224,79,58,203,97,99,155,232,168,156,131,8,205,225,122,85,230,218,70,166,101,80,237,216,173,165,182,43,8,206,238,80,18,80,95,163,123,3,229,114,73,206,226,67,113,22,69,17,45,139,103,24,188,204,50,115,178,68,10,248,92,13,115,36,30,62,164,190,31,208,108,148,139,144,207,194,180,34,135,221,42,71,99,37,225,56,153,172,94,192,201,2,208,122,251,222,233,172,38,203,237,222,172,66,92,105,102,72,123,175,221,249,225,181,13,128,254,38,237,244,66,212,243,180,82,30,100,36,219,157,112,127,65,235,114,69,29,214,229,141,39,150,64,248,46,225,15,36,46,94,199,41,40,163,5,30,178,59,241,50,221,81,24,96,95,125,17,222,98,31,242,63,168,148,5,129,177,235,148,23,132,208,69,197,101,51,178,0,175,80,217,213,253,24,200,223,42,224,37,47,74,208,193,32,35,199,238,109,106,146,140,184,169,73,90,227,87,133,145,129,165,210,11,29,203,29,171,86,35,11,214,225,109,236,12,19,242,63,131,26,254,88,220,43,236,103,227,120,110,4,207,143,210,157,187,51,112,231,48,79,87,228,133,73,80,174,210,168,148,203,12,126,159,204,211,12,14,226,220,211,103,145,207,128,150,2,227,38,83,50,119,240,63,176,251,66,53,229,132,10,246,238,153,211,202,125,165,103,89,197,202,207,113,243,158,143,70,46,109,76,133,222,31,12,206,35,96,238,114,97,139,50,189,72,115,144,76,120,16,113,248,106,236,34,112,16,223,14,47,171,141,62,141,155,33,210,238,56,104,99,250,187,28,203,122,12,131,52,183,219,141,181,154,216,65,248,239,16,39,247,146,88,164,193,192,122,122,26,189,226,132,115,171,67,147,255,132,181,104,163,20,19,108,93,92,92,100,168,9,176,202,139,231,239,113,83,39,118,151,27,103,150,217,164,10,201,175,206,126,38,243,30,132,25,72,242,27,205,148,167,53,104,72,33,186,107,112,146,224,255,42,59,229,5,11,188,99,60,231,125,97,0,167,255,87,110,90,137,70,108,165,162,59,199,232,215,162,227,98,94,202,25,174,92,7,254,176,91,186,32,115,170,55,158,157,58,242,73,80,161,73,138,7,106,231,90,175,177,10,106,48,198,176,70,29,254,70,51,144,176,33,60,41,21,100,4,77,202,116,17,82,16,54,120,19,167,153,180,177,126,21,145,156,146,31,55,252,139,9,126,254,30,50,52,110,133,115,134,140,157,221,248,231,164,175,64,112,252,210,103,72,80,229,232,160,237,211,229,140,10,223,193,199,163,187,234,87,27,244,69,128,13,70,247,208,169,26,232,236,241,170,17,170,131,122,220,96,254,35,27,126,169,203,56,175,50,228,104,94,3,211,183,254,67,17,16,169,236,32,254,7,37,182,228,12,3,252,231,16,147,131,182,79,126,34,206,234,165,222,196,67,71,102,247,227,35,108,67,9,68,160,229,102,55,161,110,110,210,136,28,140,109,213,234,12,45,135,109,52,27,182,201,66,56,15,28,43,91,102,142,165,38,51,207,134,128,159,45,163,67,109,77,120,53,96,82,104,60,204,109,163,163,150,255,229,190,190,14,161,220,215,191,164,179,244,29,2,86,161,243,244,20,116,64,181,154,163,193,217,157,194,185,113,82,36,242,93,81,92,98,8,223,27,173,206,216,137,75,216,110,120,246,196,2,225,5,48,0,156,177,121,44,126,1,179,111,46,203,72,124,158,75,145,163,40,73,224,132,168,41,102,143,255,172,249,189,88,98,50,216,207,159,222,137,184,148,20,245,35,52,83,232,55,162,189,237,209,72,189,182,9,45,139,37,57,89,224,221,166,185,250,56,47,234,66,96,11,177,44,97,222,228,90,77,25,197,161,193,18,133,115,51,206,38,171,197,15,156,141,199,89,58,160,236,72,144,2,40,18,212,44,190,68,24,119,114,65,117,184,12,57,82,122,7,118,117,100,3,219,30,161,52,156,133,148,184,249,134,176,141,4,72,192,184,4,124,62,159,107,121,79,146,190,145,162,168,21,97,86,9,104,172,197,170,114,118,136,218,126,62,102,86,157,205,6,238,212,164,187,84,104,39,51,225,139,208,63,14,185,15,87,111,126,80,146,198,92,207,209,57,133,218,212,15,101,137,193,14,52,76,244,65,214,153,46,104,233,54,50,94,98,83,45,231,213,104,244,56,85,88,203,31,137,35,201,187,19,58,81,136,59,194,178,45,28,96,135,223,184,242,96,106,93,110,160,80,200,233,37,57,1,30,60,8,181,57,26,103,113,185,120,29,223,84,120,144,62,218,3,3,77,124,59,117,180,184,105,251,148,248,210,97,100,25,52,102,253,117,114,28,104,53,27,232,61,84,132,129,78,203,73,110,160,118,36,171,169,12,67,48,115,120,0,185,248,187,33,107,44,118,181,71,233,214,248,77,141,118,7,61,227,170,189,143,151,40,1,6,44,84,138,196,141,181,85,115,14,170,215,210,136,207,51,37,20,199,70,140,146,7,136,210,165,144,214,73,113,141,7,1,188,208,115,122,136,63,152,60,242,56,145,115,217,156,56,109,185,230,120,65,47,37,174,111,128,111,97,242,49,104,16,151,181,19,162,39,233,48,152,190,107,240,190,179,137,94,212,170,39,156,202,239,250,130,169,183,150,110,165,23,18,177,221,178,217,81,234,201,130,7,225,52,236,17,96,147,94,52,179,102,148,2,171,51,32,217,109,160,53,167,51,163,117,193,114,82,195,22,122,71,209,231,149,101,157,254,48,47,234,240,12,87,239,152,253,244,176,50,188,228,74,231,231,120,44,110,154,177,65,49,210,127,158,71,85,177,144,142,206,199,236,225,244,21,222,143,55,70,40,172,59,109,18,88,83,196,110,116,69,254,131,87,3,231,179,29,180,38,202,65,154,209,84,105,255,88,71,108,155,246,110,19,14,22,140,114,143,187,164,187,140,80,70,1,18,21,21,11,205,233,227,203,124,171,202,43,191,42,235,171,131,97,32,173,210,142,157,128,188,219,160,17,97,54,250,59,179,92,37,154,185,30,15,190,29,20,67,198,115,196,253,118,41,210,42,118,21,195,62,147,49,10,18,208,3,214,82,94,146,28,237,114,12,169,44,2,199,165,170,15,178,158,243,13,125,122,64,77,10,219,7,222,158,160,242,96,95,14,80,118,130,236,131,129,125,163,111,252,227,63,255,119,208,121,226,105,69,168,157,66,168,94,240,116,40,227,164,203,73,230,3,98,84,80,61,112,211,57,18,25,39,25,187,122,94,131,44,136,242,98,29,162,157,181,183,191,187,187,203,128,116,21,1,172,69,58,200,3,116,225,194,8,148,245,181,6,131,86,138,208,105,250,157,65,217,244,81,209,222,121,57,41,202,90,9,116,250,27,103,168,44,178,12,24,219,216,148,14,252,231,116,129,9,179,184,16,248,103,177,170,67,62,177,44,182,40,198,255,134,163,177,56,0,154,187,144,208,146,178,250,48,147,112,140,43,253,65,105,131,59,4,131,74,132,181,165,211,11,216,194,135,110,39,252,200,74,148,133,172,231,69,2,38,195,199,15,167,159,161,241,164,72,110,14,105,88,160,87,178,138,250,49,46,227,69,21,126,193,117,62,212,147,63,174,139,75,153,31,250,75,69,207,120,157,110,141,192,184,133,51,159,85,7,61,100,96,231,82,207,130,157,157,145,231,223,83,3,142,244,138,129,16,59,216,221,163,88,105,215,155,71,109,189,136,56,245,147,204,138,56,33,69,57,24,53,230,180,225,108,212,120,149,175,209,242,138,182,238,233,183,75,163,130,32,45,87,243,19,122,97,221,231,19,216,113,160,248,78,74,25,95,234,182,220,31,82,250,177,44,22,105,37,209,129,80,100,24,251,249,222,229,16,245,116,44,30,3,63,56,41,204,237,222,105,19,59,14,208,238,189,235,204,88,128,161,212,41,72,21,60,16,104,166,62,163,151,152,246,243,178,44,174,210,4,212,119,13,195,185,133,71,96,36,93,73,99,123,0,244,42,143,175,192,216,71,97,19,161,215,221,242,157,211,81,154,51,113,186,159,19,45,50,4,28,128,208,5,166,117,193,73,189,2,177,191,68,15,40,172,0,121,240,9,88,25,66,48,41,37,153,105,110,143,226,51,136,154,248,34,78,243,200,186,113,244,192,57,115,140,50,55,112,183,127,53,121,118,48,57,216,15,142,58,100,164,82,95,93,247,15,79,114,75,50,26,127,197,176,216,166,156,75,35,16,123,157,232,188,133,249,108,84,142,115,187,124,222,166,220,242,119,51,109,228,83,250,1,102,37,179,10,201,57,179,229,250,55,49,62,123,83,224,65,84,199,212,247,72,53,186,245,136,157,154,33,117,250,146,134,230,192,186,247,49,147,81,150,172,64,4,10,96,251,51,168,36,124,19,32,0,99,32,205,167,217,10,88,205,104,87,136,107,103,94,47,50,53,234,145,122,241,66,56,81,3,195,103,47,204,140,19,215,152,231,135,190,39,106,203,125,170,134,214,24,131,207,47,78,95,232,84,249,106,239,224,169,124,246,148,60,43,134,147,134,195,25,157,30,229,101,204,153,229,100,157,249,119,121,238,236,98,222,186,159,123,121,103,71,188,20,213,106,58,133,113,206,86,153,81,4,56,245,46,206,214,104,94,41,197,190,34,23,2,251,53,143,64,241,161,13,202,202,152,197,22,195,67,220,145,170,141,206,222,195,45,12,34,30,4,112,44,56,75,148,123,72,86,165,186,84,6,76,9,232,214,105,158,20,235,72,115,116,243,52,252,210,235,111,133,173,123,59,22,207,236,201,168,46,80,244,237,231,94,99,183,155,107,27,123,253,197,61,197,79,147,227,134,152,171,193,66,131,3,248,103,109,110,118,200,213,241,196,245,196,97,196,119,98,172,133,112,210,147,76,209,240,28,145,141,51,220,131,133,51,70,62,231,163,94,15,197,3,174,233,156,152,216,211,249,44,168,166,152,124,72,49,49,116,57,209,166,193,12,118,88,172,56,159,202,192,137,59,228,158,217,211,162,184,195,227,5,180,110,163,215,62,31,185,80,109,2,85,246,20,168,68,198,86,133,166,116,222,230,35,207,115,110,182,227,189,144,4,237,32,68,123,194,251,238,195,190,212,249,77,91,191,111,4,29,157,163,44,243,1,143,125,64,129,170,148,218,18,176,75,240,186,237,166,198,106,249,40,52,77,158,193,119,128,66,38,104,161,205,64,130,204,201,19,168,177,220,178,107,194,94,12,142,233,60,185,26,139,21,113,81,120,165,243,66,174,48,171,10,99,62,40,157,183,183,81,48,95,193,168,223,164,215,50,9,247,80,239,95,25,68,116,191,9,209,40,70,33,36,91,124,122,96,219,45,62,26,174,48,131,98,111,247,224,217,227,167,79,204,153,3,61,238,232,135,35,191,131,64,188,127,181,83,5,230,104,81,160,251,7,45,184,31,17,206,241,184,40,5,202,196,184,28,127,11,176,183,150,1,158,94,173,154,252,192,175,181,152,199,9,222,11,218,119,139,241,118,83,195,92,253,134,111,66,157,81,203,227,224,113,112,254,141,155,3,34,30,168,158,7,125,218,138,138,143,236,133,237,105,77,166,175,30,7,44,123,7,105,14,249,143,21,3,247,67,137,224,73,96,92,13,125,19,226,248,1,253,217,61,218,106,206,182,186,146,78,10,234,96,30,11,251,164,237,61,114,247,122,59,176,160,39,143,241,183,197,8,191,204,28,242,22,116,61,18,94,215,119,206,184,112,237,61,198,240,194,222,194,63,222,115,172,190,41,136,69,9,138,74,94,108,99,160,84,222,73,125,124,98,181,199,206,76,13,223,10,179,106,78,255,105,189,213,14,170,222,186,246,250,112,142,7,123,244,222,98,254,3,102,5,84,97,226,189,56,89,149,37,140,27,99,52,230,77,191,15,18,35,196,173,80,37,50,13,90,247,91,158,38,249,129,158,53,52,198,224,195,108,198,143,141,123,66,235,118,151,42,85,96,185,162,35,74,46,138,242,198,42,105,125,36,93,54,137,97,97,23,38,103,151,24,234,255,58,216,52,164,90,46,150,88,171,97,85,182,7,166,112,105,18,34,7,214,220,226,254,175,255,115,18,24,55,222,112,79,101,60,155,97,98,135,223,139,94,93,12,15,36,81,121,205,146,110,135,82,71,213,195,250,250,238,184,89,65,154,1,246,211,244,111,180,233,247,30,47,175,131,13,237,19,125,211,182,147,54,32,97,1,86,102,230,100,173,138,111,245,4,136,255,250,191,72,171,5,77,53,171,17,56,28,11,67,192,32,250,46,155,73,114,250,53,49,140,96,8,152,143,145,219,238,170,200,96,36,195,45,21,140,111,59,82,46,178,22,122,27,102,69,65,189,30,156,156,53,138,230,117,68,90,175,99,77,133,94,60,217,81,143,232,229,154,92,113,63,228,23,89,90,205,105,158,62,167,113,254,91,138,183,165,191,53,211,68,112,160,255,167,40,23,92,224,96,228,33,60,212,8,213,203,62,20,234,181,157,200,142,53,161,33,185,156,238,3,3,187,119,193,207,87,139,52,65,2,60,224,175,131,166,25,249,11,136,41,52,98,112,223,171,217,245,151,39,166,229,33,7,175,149,157,241,38,41,16,176,71,120,112,157,64,25,38,18,222,38,25,186,166,153,158,79,41,85,234,48,191,79,243,162,248,27,197,168,132,78,89,92,164,85,5,231,111,93,144,175,248,252,204,25,96,76,194,92,154,39,231,56,223,234,33,45,253,62,101,168,171,149,136,35,190,59,138,227,135,5,170,156,148,117,107,94,109,204,23,233,22,193,38,191,146,139,97,244,229,140,116,157,213,156,50,210,56,177,41,164,186,132,245,25,11,84,68,1,250,203,45,147,198,167,144,58,68,25,66,225,190,163,75,165,203,33,51,224,41,165,200,34,144,48,22,236,37,21,189,110,82,193,126,210,91,10,167,52,198,3,42,6,234,90,33,58,47,121,28,173,208,241,82,135,141,41,19,224,182,17,76,111,106,52,96,167,191,146,114,233,169,156,157,17,233,9,64,5,195,9,14,125,41,64,165,246,207,104,223,12,57,248,42,120,169,246,127,203,82,110,209,73,251,66,231,15,111,164,149,160,119,136,87,245,170,214,171,92,30,118,69,129,63,195,11,27,10,189,181,177,26,138,172,111,253,179,187,149,6,246,83,161,110,119,240,56,120,26,201,85,209,187,48,116,181,76,215,193,217,188,58,4,190,163,28,47,65,99,16,173,216,151,163,105,57,42,22,53,240,146,133,79,208,43,143,62,28,235,35,194,146,17,108,193,189,16,236,30,142,115,244,9,77,208,19,146,20,57,122,125,85,204,180,213,107,145,203,126,218,85,199,196,185,57,40,239,176,90,73,68,127,56,139,66,29,88,159,90,156,201,178,214,78,125,188,17,128,213,93,48,159,42,18,159,164,18,33,176,221,28,31,150,248,37,221,126,147,70,198,239,111,148,213,146,130,2,161,41,157,161,44,69,74,108,25,8,17,114,170,12,114,14,254,117,82,95,99,44,10,254,66,64,98,133,107,24,229,126,162,235,17,209,204,9,182,83,199,116,235,244,130,50,128,118,241,199,223,138,2,235,94,237,225,223,124,3,225,207,250,21,255,252,85,255,76,202,248,66,97,97,188,94,134,205,49,104,18,142,109,9,192,235,19,32,73,51,17,211,25,205,210,44,59,69,29,139,220,79,187,147,189,131,125,54,28,93,0,152,197,58,220,29,67,175,98,255,128,255,99,89,133,6,227,50,23,82,178,166,251,183,56,170,175,197,222,179,93,216,242,4,22,205,37,214,209,128,157,206,63,215,105,2,18,151,150,96,222,211,130,64,76,3,110,207,45,170,105,76,100,191,7,169,29,45,226,235,16,200,2,197,106,77,244,193,31,243,145,248,3,205,230,145,55,147,6,156,144,108,135,107,128,98,84,219,52,48,104,185,207,29,48,100,154,135,221,48,26,231,200,228,83,217,245,105,118,50,191,67,39,243,129,78,126,181,157,232,133,65,39,106,232,61,177,121,57,112,64,153,251,43,127,30,11,247,231,175,94,19,170,100,34,195,144,103,254,15,76,205,199,183,216,55,44,130,223,31,146,22,210,127,199,76,167,247,26,249,235,237,130,14,37,92,169,177,216,118,215,143,134,178,237,241,0,60,242,73,145,100,148,134,126,50,7,73,5,135,111,245,238,240,215,212,217,24,131,71,19,182,118,178,26,9,139,221,24,126,199,52,51,148,215,166,186,214,187,84,77,214,183,226,57,172,209,215,226,209,19,234,211,33,116,48,91,77,145,80,228,250,230,189,147,196,160,6,247,109,59,121,204,35,114,32,241,148,102,248,77,154,53,146,225,156,62,116,234,63,13,197,116,132,191,177,64,130,217,213,248,160,125,98,172,74,204,233,1,165,70,221,63,227,187,17,240,59,36,120,230,232,116,113,161,194,223,204,15,44,42,22,23,104,86,99,196,213,247,37,107,89,8,0,44,139,141,48,212,121,12,206,188,226,3,236,189,148,87,160,40,217,222,129,174,77,38,229,106,137,157,211,122,54,47,57,209,213,158,225,41,61,205,138,90,51,142,185,24,200,99,162,16,66,99,80,191,147,70,234,234,125,183,250,228,42,18,39,124,216,22,75,153,243,252,5,35,143,168,170,156,2,65,208,33,49,11,29,68,69,190,44,200,146,197,27,134,62,71,116,75,113,117,184,156,225,117,65,172,166,1,98,68,255,249,235,216,138,22,181,243,206,245,78,198,76,209,143,220,211,73,188,68,131,43,148,145,234,250,109,50,234,32,72,221,108,108,18,132,253,187,244,216,205,142,111,206,246,201,56,9,13,117,32,51,233,249,238,57,74,125,37,62,169,35,6,248,5,197,144,47,163,9,254,145,143,231,87,141,103,175,7,207,159,72,120,117,108,73,127,80,84,218,206,127,52,197,248,68,102,24,37,116,78,239,145,115,124,127,98,247,183,107,212,140,133,227,32,183,150,78,35,81,207,75,157,245,83,196,59,249,236,4,212,239,186,219,242,250,200,190,121,207,240,118,117,11,227,159,32,207,206,211,96,67,238,247,87,212,86,208,109,95,14,10,85,176,157,220,184,19,206,139,155,255,221,67,18,57,77,68,248,211,10,235,15,184,153,225,136,143,114,122,156,100,111,199,23,199,46,110,116,200,221,105,151,171,218,96,90,189,228,159,13,162,212,67,199,97,141,119,2,180,105,243,50,73,72,0,178,112,11,99,125,11,74,16,161,78,66,132,82,90,233,190,138,130,246,198,232,118,195,3,132,255,176,59,40,44,166,211,213,50,149,201,40,24,245,152,189,126,40,197,236,173,38,159,185,219,172,131,7,251,60,209,210,40,154,172,203,135,109,127,52,97,3,45,219,243,224,54,180,84,133,198,43,27,4,179,84,13,233,219,206,106,105,207,165,147,138,77,205,173,172,22,234,65,243,174,218,157,164,113,231,46,113,23,175,107,175,200,254,109,194,43,208,0,248,78,60,117,28,231,32,9,138,129,171,222,204,18,54,56,87,216,120,8,178,160,125,220,177,139,54,241,165,110,204,19,166,110,51,23,94,126,145,179,173,112,20,55,184,171,212,112,254,21,99,32,156,180,159,123,6,98,211,211,250,182,142,69,49,188,97,54,12,20,215,201,50,149,217,225,15,31,138,174,213,115,120,206,95,137,123,43,36,130,207,98,116,110,62,104,113,178,205,193,137,211,156,210,6,7,188,18,180,247,76,59,110,209,183,11,238,188,172,119,169,81,104,115,8,89,23,236,3,134,183,46,240,52,94,170,242,193,27,10,70,216,38,247,172,106,198,234,81,34,167,133,170,31,23,144,196,52,155,70,81,224,157,43,157,76,169,1,55,156,84,13,86,212,173,117,113,181,14,159,39,223,3,107,66,54,50,226,53,88,3,202,38,107,168,244,232,206,138,95,101,103,228,144,184,101,199,92,67,115,111,171,118,167,156,109,240,115,122,14,125,156,128,67,103,22,199,222,219,77,142,80,23,250,214,89,251,91,231,134,38,133,36,149,147,171,157,62,160,84,99,147,193,175,246,59,240,194,88,175,228,88,56,197,196,252,29,227,151,108,52,233,78,31,176,168,34,25,52,232,87,186,74,171,20,246,47,31,110,99,74,200,78,209,250,193,28,237,88,160,227,56,18,63,156,126,124,182,255,228,137,246,149,197,51,24,114,226,228,79,137,73,89,172,43,89,126,83,137,42,189,22,88,128,53,203,64,53,250,124,242,81,33,67,229,29,187,172,160,159,34,191,168,210,68,250,121,91,85,116,151,68,210,47,238,254,180,102,153,111,207,40,248,35,15,150,45,11,94,73,52,6,225,104,123,120,69,55,93,109,130,244,81,163,34,220,173,61,245,141,10,171,117,138,123,68,10,238,105,28,113,114,175,58,156,187,130,7,125,218,118,143,46,229,136,107,71,149,106,219,77,166,70,46,44,74,220,20,103,119,178,45,217,252,177,8,154,101,48,92,220,15,220,115,201,247,218,234,163,145,106,60,80,71,205,18,15,40,206,219,2,116,240,156,58,26,188,75,96,53,68,66,157,21,19,231,112,193,71,127,69,187,62,122,126,4,127,124,143,127,61,194,191,182,241,47,39,115,0,219,25,25,229,114,176,102,97,47,175,139,140,172,186,120,5,141,108,86,116,64,219,101,231,183,165,188,0,97,246,215,118,78,22,37,37,97,63,15,31,82,127,81,133,209,229,239,142,197,193,243,189,199,251,94,82,182,213,6,30,80,11,56,151,109,139,239,117,3,78,49,119,210,203,249,24,144,215,83,41,97,187,30,60,251,241,149,245,54,99,73,122,14,67,121,73,183,90,33,133,191,181,208,9,208,175,130,105,90,208,163,206,121,139,126,91,94,4,14,170,166,64,215,193,105,71,174,243,26,190,112,195,156,30,27,185,6,76,128,177,241,99,250,1,50,31,197,53,237,111,151,33,245,116,126,217,218,116,70,168,68,223,67,241,69,4,127,222,62,57,253,244,102,251,51,74,246,96,48,240,229,156,239,116,194,80,192,76,75,21,47,59,161,117,249,145,87,64,71,53,212,253,246,127,194,237,226,158,233,84,230,197,222,175,105,158,51,255,34,89,38,100,164,34,104,93,82,171,189,249,28,117,241,158,62,172,33,55,165,103,15,55,238,82,98,150,232,239,85,107,241,214,188,227,56,217,34,199,199,58,157,165,63,201,122,93,148,151,28,183,192,53,198,135,167,120,84,178,166,198,158,16,247,185,245,133,184,143,223,155,11,93,1,241,225,212,38,4,153,88,73,195,115,226,94,232,118,124,164,213,29,10,4,98,70,147,166,92,235,165,186,214,3,221,233,177,181,7,248,178,21,253,110,234,224,212,172,89,58,229,132,43,190,197,240,130,240,235,10,42,91,234,74,161,59,105,116,15,84,103,147,228,124,249,212,162,203,163,10,52,5,215,64,205,163,18,30,209,147,228,21,6,219,24,68,95,224,51,177,134,202,175,1,162,71,54,28,254,165,73,193,165,56,165,240,124,139,191,27,43,229,132,132,27,111,116,84,56,48,21,205,157,197,220,16,147,69,84,42,23,172,71,89,112,112,209,92,158,232,92,131,230,29,2,133,134,99,134,190,161,223,196,113,90,57,73,51,135,58,234,75,6,155,205,99,184,109,242,159,159,179,198,28,232,113,109,210,87,124,160,157,24,128,179,103,178,255,8,163,18,84,122,175,88,221,197,219,96,244,253,5,155,199,232,170,9,141,141,168,15,123,127,31,250,247,226,55,107,21,204,37,159,36,18,188,73,173,232,216,218,154,30,188,75,211,95,153,161,255,142,189,119,73,14,90,237,16,29,92,176,133,167,11,14,194,23,234,239,227,61,149,219,178,49,149,210,203,144,220,156,26,169,228,64,104,114,35,71,253,201,145,61,39,157,151,109,212,29,208,87,90,35,6,240,104,206,44,33,173,75,22,29,236,3,131,222,115,47,29,182,41,107,72,239,36,202,245,15,202,102,59,50,64,254,26,122,242,75,185,82,168,32,24,76,235,79,133,48,72,232,30,88,208,93,134,166,131,51,104,151,210,64,128,223,41,145,164,235,24,237,58,92,54,94,186,232,99,179,193,131,180,121,70,244,132,229,200,43,232,133,253,70,157,37,11,80,76,91,103,89,43,78,40,220,170,72,116,5,4,12,75,74,164,194,226,218,244,29,128,245,92,230,100,178,170,25,86,82,189,224,123,123,21,72,119,10,44,225,196,172,242,82,102,244,221,47,109,100,34,186,197,10,175,46,21,148,42,93,214,248,229,128,211,215,63,242,140,171,59,61,177,42,54,177,42,89,189,164,251,62,133,42,74,66,81,234,25,216,213,247,154,179,25,192,86,94,148,141,54,132,35,132,70,157,220,59,28,46,245,165,80,71,153,158,14,148,40,149,80,115,241,170,96,192,252,188,85,37,217,66,76,230,25,139,71,188,105,154,145,108,170,157,174,190,207,226,232,26,9,223,90,175,212,233,164,62,142,68,215,243,177,44,120,96,114,145,191,8,91,39,252,80,56,181,169,157,130,225,135,182,20,184,41,86,133,26,184,87,119,14,129,84,177,104,246,142,184,5,77,185,106,60,113,85,215,167,212,52,181,163,142,20,253,198,125,51,27,193,214,124,121,187,213,60,255,188,100,110,187,27,188,210,92,104,232,118,85,118,30,145,208,251,114,75,213,230,95,243,197,180,98,54,195,15,88,168,235,104,156,241,243,13,216,192,69,89,147,127,164,161,222,117,124,126,109,163,34,231,198,149,42,83,55,68,203,85,85,229,48,48,134,16,87,136,110,86,196,195,227,198,205,47,118,146,43,221,199,135,86,237,35,227,231,188,177,90,48,77,11,138,109,217,54,58,145,216,137,95,85,209,114,85,205,195,6,137,167,53,110,216,16,81,68,92,136,211,39,153,94,208,53,69,71,235,193,113,208,11,224,81,160,217,40,78,74,116,123,234,142,123,46,114,53,6,53,167,232,175,208,148,81,193,10,115,33,140,139,67,34,243,107,224,81,215,64,244,16,52,208,88,145,108,122,184,117,106,17,115,185,146,127,59,253,240,19,156,202,56,228,116,118,19,234,56,159,79,155,113,182,234,213,69,214,163,15,232,125,113,184,162,165,195,131,70,174,233,163,162,44,215,77,45,252,58,226,93,165,6,56,210,102,233,128,142,109,94,54,73,50,69,81,40,71,173,237,170,194,79,96,200,55,74,159,235,185,170,210,118,90,233,140,148,94,113,169,155,115,130,9,47,237,37,126,89,234,14,109,126,164,196,209,190,18,26,125,173,126,230,161,232,134,203,178,184,104,214,148,238,107,250,81,193,6,38,81,119,99,165,15,221,244,189,95,241,67,37,203,112,213,21,55,83,70,229,210,72,153,224,135,72,212,135,22,213,167,88,40,159,78,221,14,118,252,240,250,226,103,223,39,67,250,111,143,59,197,148,59,47,143,210,191,54,13,68,215,203,165,76,30,16,7,246,7,241,97,84,23,239,138,53,38,105,84,192,32,17,126,117,227,151,180,6,149,56,154,80,214,190,106,64,240,236,27,3,29,246,249,147,246,243,239,197,238,245,163,89,12,167,157,83,244,86,79,137,99,197,210,133,2,94,192,120,33,176,23,17,130,252,168,11,177,39,222,167,175,70,17,85,254,87,247,95,71,125,197,109,253,50,188,74,84,33,11,234,186,53,77,227,77,49,144,49,110,40,115,149,120,182,43,151,181,125,81,188,179,61,135,73,122,51,98,53,223,80,25,224,198,64,26,223,66,213,38,142,170,232,99,149,78,26,147,243,187,223,58,210,123,194,72,144,93,239,169,189,251,169,190,42,228,212,254,119,23,234,103,242,219,176,73,165,15,210,196,100,224,195,225,9,204,18,117,23,114,153,220,212,82,151,176,250,25,206,160,103,47,203,50,190,209,233,0,200,41,49,62,120,181,194,115,57,28,121,181,8,122,124,160,186,188,26,115,203,177,120,99,166,45,79,50,172,245,67,47,66,234,89,127,40,7,176,57,110,83,134,71,215,6,160,39,239,240,153,106,117,62,26,11,179,15,28,11,137,144,193,206,38,129,191,123,45,159,119,20,84,211,37,43,154,245,27,220,106,33,93,46,107,44,101,139,42,148,23,145,211,173,249,154,60,79,196,159,223,191,251,83,93,47,63,241,195,208,49,187,248,83,56,168,154,219,218,78,77,119,43,27,180,122,240,47,252,107,45,119,136,211,193,134,62,212,179,174,108,50,63,86,55,45,167,135,134,135,79,62,157,132,10,120,228,131,33,247,30,122,251,18,21,5,54,163,119,131,238,128,94,107,168,176,181,212,52,252,137,188,200,161,239,65,30,247,123,144,91,168,212,197,13,76,249,196,58,1,187,205,247,236,180,196,92,45,123,192,56,198,153,141,25,72,53,43,39,197,2,54,44,110,69,247,234,82,107,43,82,58,45,169,219,152,98,150,113,244,107,7,237,182,162,142,51,204,48,219,115,76,235,219,94,178,58,242,38,155,98,86,149,93,210,107,179,121,23,119,246,216,219,21,197,44,108,44,208,138,0,117,147,73,107,87,203,184,164,88,35,163,211,55,56,63,195,105,230,86,41,110,121,202,93,223,130,218,42,161,179,237,214,37,126,185,103,212,89,232,152,111,163,42,150,177,30,151,253,221,93,212,49,147,136,109,12,186,88,78,91,49,76,28,52,116,251,162,213,95,18,113,16,20,239,151,53,187,238,152,50,27,49,213,79,44,191,241,60,182,71,100,156,186,29,108,143,95,182,2,97,102,238,190,31,109,117,50,215,158,230,99,195,6,6,19,87,37,74,23,43,178,222,157,51,49,56,229,135,116,41,42,198,155,96,145,248,169,176,167,219,58,174,196,12,78,140,57,86,77,113,206,66,133,114,137,70,108,121,229,160,36,164,250,144,180,247,46,246,34,241,11,73,255,194,212,3,137,196,62,222,199,160,164,85,123,65,25,227,227,232,140,168,208,198,208,31,1,113,239,23,30,222,7,253,73,231,101,15,56,149,129,13,1,226,81,36,78,129,143,65,237,96,47,145,170,209,209,17,81,106,204,160,171,43,119,215,14,105,174,1,189,140,140,206,137,150,188,94,114,119,57,236,215,158,5,25,111,229,106,137,101,142,4,237,103,119,12,19,250,194,13,146,81,222,224,119,10,221,197,57,11,178,226,34,205,255,130,28,148,150,252,205,174,105,85,206,130,115,91,69,199,163,103,228,47,223,41,60,69,10,228,245,18,155,227,42,145,20,64,149,6,175,193,97,109,89,53,85,222,194,116,140,17,79,144,230,237,208,64,31,222,232,96,170,139,66,100,232,180,162,181,195,1,234,67,30,36,71,77,215,58,163,160,113,27,212,42,97,228,208,195,207,7,208,41,140,159,19,160,15,57,215,148,122,129,30,172,92,243,136,97,233,8,51,19,98,98,108,20,134,84,97,12,153,219,220,120,214,58,167,195,5,183,109,69,205,56,1,127,135,166,166,218,186,142,15,175,148,185,227,7,186,106,249,35,156,47,1,56,215,1,188,194,189,125,165,87,176,169,174,189,226,20,91,38,137,36,135,138,130,72,255,107,215,168,15,93,117,125,170,94,127,121,32,210,156,135,14,31,102,132,71,65,55,184,174,87,132,87,94,222,177,211,215,105,3,220,242,12,22,124,247,200,169,123,49,45,111,150,181,143,186,93,61,194,169,136,164,228,35,101,70,228,32,156,131,205,149,142,187,191,73,189,181,169,118,202,3,67,92,207,220,52,106,52,121,99,182,3,51,99,110,150,178,112,7,53,80,35,211,225,165,214,23,103,92,238,185,51,10,191,206,126,203,189,128,34,225,195,170,14,169,220,38,51,47,253,217,89,129,243,174,31,12,1,9,6,7,102,208,253,45,16,60,136,123,11,78,108,140,166,240,13,203,102,204,228,255,75,77,123,220,227,72,187,45,126,17,99,228,13,89,209,249,176,64,111,182,124,248,141,169,153,116,108,171,235,96,65,22,242,75,211,98,244,82,9,219,79,249,249,79,1,149,169,135,237,103,4,54,239,1,25,79,12,200,3,14,57,126,166,136,194,216,59,212,6,235,248,114,103,111,20,232,221,83,132,6,174,177,42,156,59,118,72,152,146,168,21,33,77,85,211,135,82,218,250,111,120,168,49,14,161,49,220,8,152,234,21,172,148,113,136,7,175,205,171,56,43,193,196,184,209,58,152,212,201,187,166,171,246,194,119,82,49,218,172,32,180,114,57,223,216,40,213,96,135,13,78,235,136,222,133,61,71,15,216,187,88,197,101,151,131,33,24,194,121,95,228,41,204,15,26,12,234,83,172,168,128,84,246,19,208,120,244,163,155,18,173,105,36,161,152,113,17,121,47,172,131,30,250,98,40,142,83,173,83,152,8,213,89,39,59,178,8,233,97,73,18,42,30,91,222,147,183,238,246,113,34,46,177,8,252,64,129,5,44,71,112,219,208,5,109,17,191,150,67,224,148,134,168,67,141,67,110,129,206,106,54,157,31,171,60,238,252,54,136,186,60,65,184,244,125,241,134,24,187,199,46,29,100,48,165,0,233,2,115,189,133,49,111,53,63,125,54,31,24,0,230,6,81,46,56,73,213,198,27,173,155,12,212,64,52,1,48,204,104,60,245,88,243,179,82,159,34,39,174,162,50,122,91,87,177,169,170,193,69,197,222,21,250,187,223,205,43,80,29,245,199,108,158,67,7,10,47,191,161,187,11,63,163,161,154,150,233,114,232,235,150,12,192,28,192,127,171,100,217,64,87,225,221,158,82,15,176,254,47,174,142,113,73,99,124,114,179,189,191,187,255,100,111,119,143,175,93,171,166,157,118,184,113,138,93,165,120,31,254,120,115,61,46,5,138,153,11,83,46,84,186,169,9,214,56,108,176,113,177,174,40,92,79,229,25,189,201,50,134,41,245,50,244,49,168,224,31,255,249,191,2,202,95,106,223,55,208,85,48,17,28,204,51,252,94,16,199,80,74,50,157,241,74,223,104,100,123,105,124,54,149,131,42,228,202,211,168,205,23,235,153,19,144,187,156,74,205,48,192,123,81,234,209,138,187,31,72,37,18,103,105,6,22,155,165,146,206,121,67,38,59,162,70,220,95,131,102,104,126,182,119,62,70,52,103,251,206,23,173,8,214,126,102,59,145,246,179,151,126,141,44,37,20,81,82,48,37,248,209,65,166,99,159,233,24,42,148,237,72,55,133,104,212,160,27,187,231,55,42,210,33,204,100,170,158,65,3,215,16,251,231,163,6,35,56,146,130,134,212,157,76,97,192,91,223,106,152,122,37,4,9,69,251,123,14,76,191,153,49,20,127,119,168,14,174,108,168,62,80,160,116,137,242,148,20,101,246,215,92,241,85,21,85,184,254,203,100,53,129,161,84,135,40,30,110,71,40,43,181,75,168,7,101,127,17,252,176,203,27,76,95,151,253,39,86,85,180,151,18,113,190,32,102,219,61,231,116,58,61,195,248,6,86,82,241,161,235,111,186,245,132,81,227,142,243,176,84,254,93,197,126,156,179,199,203,46,116,189,250,76,14,165,16,252,63,248,114,58,42,149,142,0,0};
const char weatherCitiesJS[] PROGMEM = R"MOSJS(window.weatherCities=[["北京","北京","101010100"],["上海","上海","101020100"],["天津","天津","101030100"],["重庆","重庆","101040100"],["黑龙江","哈尔滨","101050101"],["黑龙江","齐齐哈尔","101050201"],["黑龙江","牡丹江","101050301"],["黑龙江","佳木斯","101050401"],["黑龙江","绥化","101050501"],["黑龙江","黑河","101050601"],["黑龙江","大兴安岭","101050701"],["黑龙江","伊春","101050801"],["黑龙江","大庆","101050901"],["黑龙江","七台河","101051002"],["黑龙江","鸡西","101051101"],["黑龙江","鹤岗","101051201"],["黑龙江","双鸭山","101051301"],["吉林","长春","101060101"],["吉林","吉林","101060201"],["吉林","延边","101060306"],["吉林","四平","101060401"],["吉林","通化","101060501"],["吉林","白城","101060601"],["吉林","辽源","101060701"],["吉林","松原","101060801"],["吉林","白山","101060901"],["辽宁","沈阳","101070101"],["辽宁","大连","101070201"],["辽宁","鞍山","101070301"],["辽宁","抚顺","101070401"],["辽宁","本溪","101070501"],["辽宁","丹东","101070601"],["辽宁","锦州","101070701"],["辽宁","营口","101070801"],["辽宁","阜新","101070901"],["辽宁","辽阳","101071001"],["辽宁","铁岭","101071101"],["辽宁","朝阳","101071201"],["辽宁","盘锦","101071301"],["辽宁","葫芦岛","101071401"],["内蒙古","呼和浩特","101080101"],["内蒙古","包头","101080201"],["内蒙古","乌海","101080301"],["内蒙古","乌兰察布","101080405"],["内蒙古","通辽","101080501"],["内蒙古","赤峰","101080601"],["内蒙古","鄂尔多斯","101080701"],["内蒙古","巴彦淖尔","101080811"],["内蒙古","锡林郭勒","101080902"],["内蒙古","呼伦贝尔","101081013"],["内蒙古","兴安盟","101081108"],["内蒙古","阿拉善盟","101081213"],["河北","石家庄","101090101"],["河北","保定","101090201"],["河北","张家口","101090301"],["河北","承德","101090402"],["河北","唐山","101090501"],["河北","廊坊","101090601"],["河北","沧州","101090701"],["河北","衡水","101090801"],["河北","邢台","101090901"],["河北","邯郸","101091001"],["河北","秦皇岛","101091101"],["河北","雄安新区","101091201"],["山西","太原","101100101"],["山西","大同","101100201"],["山西","阳泉","101100301"],["山西","晋中","101100401"],["山西","长治","101100501"],["山西","晋城","101100601"],["山西","临汾","101100701"],["山西","运城","101100801"],["山西","朔州","101100901"],["山西","忻州","101101001"],["山西","吕梁","101101100"],["陕西","西安","101110101"],["陕西","咸阳","101110200"],["陕西","延安","101110300"],["陕西","榆林","101110401"],["陕西","渭南","101110501"],["陕西","商洛","101110601"],["陕西","安康","101110701"],["陕西","汉中","101110801"],["陕西","宝鸡","101110901"],["陕西","铜川","101111001"],["陕西","杨凌","101111101"],["山东","济南","101120101"],["山东","青岛","101120201"],["山东","淄博","101120301"],["山东","德州","101120401"],["山东","烟台","101120501"],["山东","潍坊","101120601"],["山东","济宁","101120701"],["山东","泰安","101120801"],["山东","临沂","101120901"],["山东","菏泽","101121001"],["山东","滨州","101121101"],["山东","东营","101121201"],["山东","威海","101121301"],["山东","枣庄","101121401"],["山东","日照","101121501"],["山东","莱芜","101121601"],["山东","聊城","101121701"],["新疆","乌鲁木齐","101130101"],["新疆","克拉玛依","101130201"],["新疆","石河子","101130301"],["新疆","昌吉","101130401"],["新疆","吐鲁番","101130501"],["新疆","巴音郭楞","101130609"],["新疆","阿拉尔","101130701"],["新疆","阿克苏","101130801"],["新疆","喀什","101130901"],["新疆","伊犁","101131012"],["新疆","塔城","101131101"],["新疆","哈密","101131201"],["新疆","和田","101131301"],["新疆","阿勒泰","101131401"],["新疆","克州","101131505"],["新疆","博尔塔拉","101131604"],["新疆","图木舒克","101131701"],["新疆","五家渠","101131801"],["新疆","铁门关","101131901"],["新疆","昆玉","101131920"],["新疆","北屯","101132101"],["新疆","双河","101132201"],["新疆","可克达拉","101132301"],["西藏","拉萨","101140101"],["西藏","日喀则","101140201"],["西藏","山南","101140301"],["西藏","林芝","101140401"],["西藏","昌都","101140501"],["西藏","那曲","101140601"],["西藏","阿里","101140701"],["青海","西宁","101150101"],["青海","海东","101150207"],["青海","黄南","101150305"],["青海","海南","101150402"],["青海","果洛","101150507"],["青海","玉树","101150601"],["青海","海西","101150702"],["青海","海北","101150804"],["甘肃","兰州","101160101"],["甘肃","定西","101160201"],["甘肃","平凉","101160301"],["甘肃","庆阳","101160401"],["甘肃","武威","101160501"],["甘肃","金昌","101160601"],["甘肃","张掖","101160701"],["甘肃","酒泉","101160801"],["甘肃","天水","101160901"],["甘肃","陇南","101161010"],["甘肃","临夏","101161101"],["甘肃","甘南","101161209"],["甘肃","白银","101161301"],["甘肃","嘉峪关","101161401"],["宁夏","银川","101170101"],["宁夏","石嘴山","101170201"],["宁夏","吴忠","101170301"],["宁夏","固原","101170401"],["宁夏","中卫","101170501"],["河南","郑州","101180101"],["河南","安阳","101180201"],["河南","新乡","101180301"],["河南","许昌","101180401"],["河南","平顶山","101180501"],["河南","信阳","101180601"],["河南","南阳","101180701"],["河南","开封","101180801"],["河南","洛阳","101180901"],["河南","商丘","101181001"],["河南","焦作","101181101"],["河南","鹤壁","101181201"],["河南","濮阳","101181301"],["河南","周口","101181401"],["河南","漯河","101181501"],["河南","驻马店","101181601"],["河南","三门峡","101181701"],["河南","济源","101181801"],["江苏","南京","101190101"],["江苏","无锡","101190201"],["江苏","镇江","101190301"],["江苏","苏州","101190401"],["江苏","南通","101190501"],["江苏","扬州","101190601"],["江苏","盐城","101190701"],["江苏","徐州","101190801"],["江苏","淮安","101190901"],["江苏","连云港","101191001"],["江苏","常州","101191101"],["江苏","泰州","101191201"],["江苏","宿迁","101191301"],["湖北","武汉","101200101"],["湖北","襄阳","101200201"],["湖北","鄂州","101200301"],["湖北","孝感","101200401"],["湖北","黄冈","101200501"],["湖北","黄石","101200601"],["湖北","咸宁","101200701"],["湖北","荆州","101200801"],["湖北","宜昌","101200901"],["湖北","恩施","101201001"],["湖北","十堰","101201101"],["湖北","神农架","101201201"],["湖北","随州","101201301"],["湖北","荆门","101201401"],["湖北","天门","101201501"],["湖北","仙桃","101201601"],["湖北","潜江","101201701"],["浙江","杭州","101210101"],["浙江","湖州","101210201"],["浙江","嘉兴","101210301"],["浙江","宁波","101210401"],["浙江","绍兴","101210507"],["浙江","台州","101210601"],["浙江","温州","101210701"],["浙江","丽水","101210801"],["浙江","金华","101210901"],["浙江","衢州","101211001"],["浙江","舟山","101211101"],["安徽","合肥","101220101"],["安徽","蚌埠","101220201"],["安徽","芜湖","101220301"],["安徽","淮南","101220401"],["安徽","马鞍山","101220501"],["安徽","安庆","101220601"],["安徽","宿州","101220701"],["安徽","阜阳","101220801"],["安徽","亳州","101220901"],["安徽","黄山","101221001"],["安徽","滁州","101221101"],["安徽","淮北","101221201"],["安徽","铜陵","101221301"],["安徽","宣城","101221401"],["安徽","六安","101221501"],["安徽","池州","101221701"],["福建","福州","101230101"],["福建","厦门","101230201"],["福建","宁德","101230301"],["福建","莆田","101230401"],["福建","泉州","101230501"],["福建","漳州","101230601"],["福建","龙岩","101230701"],["福建","三明","101230801"],["福建","南平","101230901"],["福建","钓鱼岛","101231001"],["江西","南昌","101240101"],["江西","九江","101240201"],["江西","上饶","101240301"],["江西","抚州","101240401"],["江西","宜春","101240501"],["江西","吉安","101240601"],["江西","赣州","101240701"],["江西","景德镇","101240801"],["江西","萍乡","101240901"],["江西","新余","101241001"],["江西","鹰潭","101241101"],["湖南","长沙","101250101"],["湖南","湘潭","101250201"],["湖南","株洲","101250301"],["湖南","衡阳","101250401"],["湖南","郴州","101250501"],["湖南","常德","101250601"],["湖南","益阳","101250700"],["湖南","娄底","101250801"],["湖南","邵阳","101250901"],["湖南","岳阳","101251001"],["湖南","张家界","101251101"],["湖南","怀化","101251201"],["湖南","永州","101251401"],["湖南","湘西","101251509"],["贵州","贵阳","101260101"],["贵州","遵义","101260201"],["贵州","安顺","101260301"],["贵州","黔南","101260413"],["贵州","黔东南","101260506"],["贵州","铜仁","101260601"],["贵州","毕节","101260701"],["贵州","六盘水","101260803"],["贵州","黔西南","101260906"],["四川","成都","101270101"],["四川","攀枝花","101270201"],["四川","自贡","101270301"],["四川","绵阳","101270401"],["四川","南充","101270501"],["四川","达州","101270601"],["四川","遂宁","101270701"],["四川","广安","101270801"],["四川","巴中","101270901"],["四川","泸州","101271001"],["四川","宜宾","101271101"],["四川","内江","101271201"],["四川","资阳","101271301"],["四川","乐山","101271401"],["四川","眉山","101271501"],["四川","凉山","101271601"],["四川","雅安","101271701"],["四川","甘孜","101271801"],["四川","阿坝","101271901"],["四川","德阳","101272001"],["四川","广元","101272101"],["广东","广州","101280101"],["广东","韶关","101280201"],["广东","惠州","101280301"],["广东","梅州","101280401"],["广东","汕头","101280501"],["广东","深圳","101280601"],["广东","珠海","101280701"],["广东","佛山","101280800"],["广东","肇庆","101280901"],["广东","湛江","101281001"],["广东","江门","101281101"],["广东","河源","101281201"],["广东","清远","101281301"],["广东","云浮","101281401"],["广东","潮州","101281501"],["广东","东莞","101281601"],["广东","中山","101281701"],["广东","阳江","101281801"],["广东","揭阳","101281901"],["广东","茂名","101282001"],["广东","汕尾","101282101"],["云南","昆明","101290101"],["云南","大理","101290201"],["云南","红河","101290301"],["云南","曲靖","101290401"],["云南","保山","101290501"],["云南","文山","101290601"],["云南","玉溪","101290701"],["云南","楚雄","101290801"],["云南","普洱","101290901"],["云南","昭通","101291001"],["云南","临沧","101291101"],["云南","怒江","101291201"],["云南","迪庆","101291305"],["云南","丽江","101291401"],["云南","德宏","101291501"],["云南","西双版纳","101291602"],["广西","南宁","101300101"],["广西","崇左","101300201"],["广西","柳州","101300301"],["广西","来宾","101300401"],["广西","桂林","101300501"],["广西","梧州","101300601"],["广西","贺州","101300701"],["广西","贵港","101300801"],["广西","玉林","101300901"],["广西","百色","101301001"],["广西","钦州","101301101"],["广西","河池","101301201"],["广西","北海","101301301"],["广西","防城港","101301401"],["海南","海口","101310101"],["海南","三亚","101310201"],["海南","东方","101310202"],["海南","临高","101310203"],["海南","澄迈","101310204"],["海南","儋州","101310205"],["海南","昌江","101310206"],["海南","白沙","101310207"],["海南","琼中","101310208"],["海南","定安","101310209"],["海南","屯昌","101310210"],["海南","琼海","101310211"],["海南","文昌","101310212"],["海南","保亭","101310214"],["海南","万宁","101310215"],["海南","陵水","101310216"],["海南","乐东","101310221"],["海南","五指山","101310222"],["海南","三沙","101310301"],["香港","香港","101320101"],["澳门","澳门","101330101"],["台湾","台北","101340101"],["台湾","高雄","101340201"],["台湾","台中","101340401"]];
)MOSJS";
const char weatherCitiesJSGzip[] PROGMEM = {31,139,8,0,0,0,0,0,2,255,117,154,217,110,34,73,22,134,239,231,49,234,186,53,34,35,89,53,154,171,121,140,86,95,140,52,45,77,223,204,72,51,45,245,173,177,13,54,20,24,236,242,2,6,108,83,101,188,84,21,152,205,101,22,3,47,67,36,153,111,49,17,144,249,159,32,15,163,170,66,169,250,206,137,229,143,237,196,201,252,227,183,127,253,227,223,127,252,249,143,95,255,254,251,63,127,253,207,223,126,251,253,183,95,255,251,215,159,127,254,32,11,87,203,201,247,15,63,209,131,21,217,252,137,124,248,229,167,159,63,44,71,121,231,245,77,253,55,30,52,19,224,242,254,217,25,182,180,127,240,160,185,13,238,29,21,229,36,171,254,27,15,154,71,137,79,79,189,121,213,233,221,234,34,62,29,203,238,185,51,125,242,173,98,186,29,204,202,155,149,213,223,141,45,12,197,14,195,85,174,185,28,141,55,207,27,43,123,135,213,114,54,112,234,79,206,229,11,172,162,187,202,154,182,100,225,18,38,177,93,237,154,158,58,253,1,76,226,59,76,228,253,163,204,12,101,39,39,251,109,24,38,118,53,234,61,239,84,90,48,73,254,159,178,160,103,44,146,218,85,202,232,64,150,186,70,163,148,232,130,183,123,212,116,91,11,152,236,148,124,124,47,251,87,48,217,37,182,44,21,188,81,91,246,122,176,10,196,150,229,156,115,163,157,189,139,5,117,42,142,177,5,199,195,134,11,198,167,63,220,249,24,220,142,196,67,188,86,147,227,1,120,148,213,191,119,77,99,24,199,24,130,175,170,51,121,123,2,30,15,115,119,62,115,38,101,240,68,152,59,141,185,60,185,5,79,238,42,31,250,196,49,98,170,88,217,73,107,255,254,177,87,9,218,159,128,62,224,106,196,221,197,13,184,8,115,239,166,72,229,39,48,217,169,252,252,181,215,156,128,71,25,175,127,119,38,95,193,99,97,174,22,211,114,84,7,143,179,250,207,31,228,27,181,47,17,230,110,185,37,75,95,192,147,204,191,82,119,46,187,224,76,31,245,96,232,163,230,114,216,255,83,154,214,85,194,178,120,255,26,166,63,211,111,85,171,168,46,128,51,253,220,211,111,110,254,65,246,107,48,193,20,203,102,220,179,170,44,221,235,81,58,125,151,103,5,231,245,121,149,11,230,106,146,230,186,105,88,200,200,251,33,76,196,14,147,229,184,64,123,110,18,35,26,50,145,153,174,124,185,149,163,3,24,70,35,49,102,168,102,191,234,9,76,98,59,202,114,95,239,229,160,11,147,248,14,19,239,112,95,109,187,242,254,154,54,204,36,173,4,179,119,111,67,57,123,112,222,46,105,151,78,70,146,214,142,18,207,155,106,121,120,7,109,249,241,12,134,41,127,159,10,9,187,124,127,112,135,13,163,68,245,99,115,195,245,22,187,170,221,194,74,253,227,245,86,22,206,199,156,188,60,52,13,133,95,156,218,51,213,121,168,231,196,237,64,118,126,200,201,161,111,146,194,80,194,100,185,104,200,206,53,184,8,115,249,126,167,139,192,204,79,97,28,97,226,228,22,114,254,6,30,245,123,79,69,156,151,105,101,167,48,118,196,167,121,217,200,131,199,89,249,253,71,90,153,41,140,23,184,219,108,58,221,33,120,50,204,189,253,207,234,32,1,79,113,254,226,29,140,2,142,149,73,42,62,62,172,174,143,104,229,164,44,166,162,87,59,84,131,166,214,191,44,76,96,133,53,209,235,109,142,40,121,255,21,91,172,174,134,243,71,89,46,128,51,127,181,254,157,65,14,220,14,115,167,250,113,57,106,131,71,153,191,58,194,250,83,240,216,14,127,28,33,138,199,195,124,57,26,58,189,57,120,34,204,221,69,217,244,79,178,242,235,231,24,72,197,83,172,255,139,169,193,49,16,196,203,23,206,231,52,56,130,176,234,133,95,127,107,161,70,193,231,22,133,95,1,151,103,35,108,161,150,14,3,67,254,234,136,54,253,237,48,119,30,178,56,226,45,139,34,45,112,21,66,20,137,199,88,253,23,89,103,88,3,143,51,174,66,171,201,27,120,130,149,223,203,209,248,90,20,87,145,127,67,133,67,224,169,48,247,62,213,229,91,35,224,208,151,202,111,60,201,163,2,184,49,63,55,39,167,243,122,66,253,19,17,198,189,198,25,86,137,226,130,249,191,29,202,226,53,184,29,230,106,23,161,241,23,230,252,221,240,213,193,45,22,178,226,49,86,254,172,136,141,68,241,248,174,246,119,210,224,9,198,7,93,26,127,97,206,223,13,215,243,191,191,15,158,10,115,183,84,114,6,179,128,155,243,215,47,127,250,100,244,143,235,171,126,85,148,1,206,244,147,79,105,28,168,122,179,103,229,223,124,193,94,175,56,211,207,185,106,173,50,143,224,76,63,247,164,231,230,235,224,76,63,55,157,167,245,45,44,108,196,151,221,213,101,118,115,154,123,253,180,186,143,168,27,142,111,101,211,137,19,88,201,204,71,117,116,173,78,106,203,121,3,86,34,108,165,142,46,189,187,182,169,32,59,108,226,84,10,42,58,5,143,178,138,202,101,213,156,213,197,87,152,196,152,201,219,208,187,29,168,227,219,105,221,192,74,5,183,219,86,254,97,27,28,220,202,132,117,93,153,168,126,185,31,75,48,73,178,186,46,247,150,211,61,240,20,83,239,61,191,202,7,179,211,86,63,34,228,223,60,39,245,109,139,235,170,238,148,47,89,112,166,168,138,235,86,231,93,112,123,87,23,62,158,57,3,50,137,238,24,58,154,192,182,154,64,177,16,47,94,235,248,170,121,174,228,130,149,186,203,132,172,106,115,53,71,220,227,51,85,28,172,248,100,154,156,171,192,195,25,221,193,132,41,170,66,102,239,234,73,102,6,48,73,241,57,146,93,157,80,91,82,34,18,106,75,225,74,246,94,2,46,184,168,165,2,110,160,138,115,81,75,47,122,220,231,115,163,199,2,177,119,107,225,94,233,25,161,160,91,246,243,2,90,85,198,175,90,106,118,200,227,42,76,68,216,68,45,66,218,123,163,17,94,197,205,149,155,111,128,71,25,175,20,188,131,25,120,44,204,189,253,47,78,173,15,30,103,188,178,240,112,54,68,233,108,106,156,109,182,164,245,217,27,204,94,35,245,17,112,245,139,235,151,165,51,30,137,109,238,77,15,169,127,58,215,17,99,254,38,15,130,76,226,55,117,58,91,117,138,35,84,190,154,5,206,221,41,120,124,71,251,144,71,176,116,90,67,240,250,11,84,127,210,159,212,171,243,138,187,127,176,94,26,93,90,26,148,30,32,222,185,166,242,41,61,64,124,60,144,71,57,112,155,241,73,150,98,23,74,15,128,59,237,7,117,60,128,199,194,220,59,58,85,83,0,60,206,202,127,191,115,78,46,193,19,204,63,115,70,177,39,165,7,200,255,254,25,65,184,69,233,1,242,175,30,209,248,197,117,112,182,205,213,217,42,239,75,224,76,63,245,96,250,11,127,143,38,94,157,121,159,126,128,115,253,42,57,57,248,74,123,69,156,142,199,78,122,83,179,242,167,240,136,50,24,224,250,54,85,25,226,42,99,81,18,3,38,178,60,148,139,59,112,155,241,218,132,110,0,148,196,0,95,234,8,242,27,184,121,85,90,119,222,59,56,165,89,150,220,186,205,173,185,10,95,104,150,36,183,110,115,107,174,246,173,229,184,9,110,135,185,219,25,209,44,73,210,145,138,242,199,3,175,249,131,36,72,242,38,46,23,77,179,9,113,86,68,241,202,228,9,198,223,247,100,55,13,158,100,93,24,214,76,255,20,243,191,200,46,71,149,128,111,221,230,214,124,117,248,176,156,213,193,153,132,58,91,248,5,245,91,92,194,69,199,168,223,98,18,202,211,39,220,150,21,103,18,58,239,47,116,162,36,45,62,196,207,83,239,249,187,156,92,193,132,73,184,28,229,244,185,55,192,64,90,76,69,21,234,34,219,167,56,84,236,221,110,98,20,101,133,124,185,101,164,5,2,238,92,221,121,231,77,112,17,230,222,197,17,82,211,150,145,19,8,184,250,165,137,154,162,137,100,212,239,237,93,131,199,88,253,185,239,166,127,60,204,87,53,227,170,105,228,4,80,254,188,108,250,179,254,59,111,29,10,245,141,156,0,218,191,184,89,78,78,157,81,16,26,24,105,1,84,49,26,25,85,88,92,194,65,215,228,76,66,217,89,184,139,52,56,36,28,95,250,105,143,246,131,186,240,109,184,160,108,1,184,219,58,196,68,20,148,45,0,215,201,174,160,126,65,217,2,112,217,110,56,135,183,224,81,230,175,78,228,236,49,120,108,7,87,59,34,120,156,149,127,54,66,68,32,40,91,64,237,47,102,205,246,37,153,127,167,142,189,72,80,182,128,244,73,63,59,151,179,128,211,248,192,191,152,150,119,93,112,166,223,170,117,35,179,117,231,230,7,76,184,132,215,37,163,137,124,136,84,23,212,66,4,103,18,170,67,209,228,76,194,229,180,234,52,15,192,153,132,206,172,142,85,166,56,36,124,245,95,91,56,141,54,181,143,18,30,196,85,65,6,23,97,174,15,197,204,16,220,102,188,147,118,6,159,193,163,97,190,154,22,77,255,32,232,34,255,82,215,172,63,206,218,55,122,54,57,235,223,114,52,67,80,33,40,225,1,174,130,26,89,60,1,79,133,185,219,252,108,148,79,83,4,252,248,22,103,153,48,18,30,157,156,92,39,155,101,249,216,221,247,47,228,194,72,120,4,220,189,46,200,219,59,112,193,120,190,174,134,0,220,14,115,189,11,5,65,141,48,18,30,1,87,7,129,249,90,68,24,57,15,52,81,231,140,178,224,113,206,23,36,129,145,243,64,21,149,58,237,34,70,206,35,224,203,201,192,244,79,49,127,181,75,80,251,40,231,129,46,78,211,134,63,151,88,75,80,128,4,22,147,208,251,84,247,170,175,224,54,239,223,23,28,4,194,200,121,128,103,218,216,232,133,145,243,64,253,189,59,179,125,136,123,31,74,114,170,83,185,250,1,156,178,25,224,242,228,129,150,56,229,49,136,171,208,46,72,142,11,74,98,128,187,39,89,220,202,5,37,49,192,85,208,109,214,31,99,252,125,96,242,120,152,123,243,170,236,63,131,179,254,169,88,194,169,156,128,39,89,251,139,87,120,45,41,40,107,65,229,159,125,242,122,239,72,251,9,123,235,160,220,164,37,139,87,180,145,71,183,98,141,77,90,121,220,160,93,46,186,21,107,248,105,231,188,215,250,1,110,135,185,147,191,38,9,162,91,177,134,159,22,173,227,205,173,136,110,197,26,126,90,57,71,83,36,186,21,107,108,210,202,175,95,204,242,19,172,254,234,139,26,98,21,17,193,36,201,138,40,23,17,119,43,158,98,69,168,184,124,86,13,56,151,208,27,119,157,89,27,220,60,203,138,193,203,233,126,224,31,139,48,238,140,43,228,31,219,138,21,54,252,238,171,51,236,131,219,97,238,54,41,168,23,177,173,88,193,191,151,12,73,162,216,86,172,176,9,138,85,172,132,85,16,219,138,21,54,65,121,45,111,150,159,240,51,239,228,255,116,40,39,23,224,73,86,255,254,171,233,159,98,245,247,7,6,223,138,21,138,244,150,107,117,81,128,9,151,112,111,15,239,223,69,204,226,18,118,71,134,4,86,116,199,16,32,1,160,120,204,191,192,186,195,215,141,155,122,160,38,82,2,1,220,75,191,46,199,57,112,17,230,250,234,23,188,31,23,148,64,32,255,233,57,157,53,241,72,212,127,87,104,242,229,168,110,154,196,252,79,20,200,228,83,125,57,77,131,199,195,85,56,47,23,110,126,31,60,193,154,152,105,175,106,21,58,209,117,26,129,183,66,103,146,140,86,164,130,15,37,106,181,205,237,220,57,46,35,147,37,140,107,58,248,249,158,115,211,112,243,61,152,136,176,137,123,244,213,29,54,193,237,48,95,77,141,177,48,174,233,1,215,155,98,38,3,30,99,229,207,231,52,23,232,91,3,112,47,189,79,161,49,125,107,64,229,143,233,69,150,160,111,13,136,191,13,241,34,72,208,183,6,36,193,192,152,139,244,173,1,249,119,234,178,51,7,103,18,202,108,134,118,100,250,214,128,250,247,106,92,61,232,91,3,240,229,152,222,248,10,227,67,3,232,91,207,153,156,233,39,143,182,56,215,175,150,49,244,177,152,126,58,89,212,174,131,51,253,116,170,187,209,0,103,250,233,237,156,250,39,34,187,198,39,115,0,14,253,198,11,255,69,205,216,8,186,140,239,39,2,238,221,254,64,38,74,24,31,79,4,220,57,184,51,253,109,198,63,103,76,30,101,188,119,129,143,51,132,241,181,4,248,91,79,214,169,254,120,152,175,74,119,120,209,36,140,47,36,2,190,156,213,104,124,116,142,38,178,205,221,253,35,10,74,41,71,67,245,143,107,52,191,40,71,99,180,255,150,130,42,202,209,16,239,15,144,221,16,148,163,33,62,202,184,139,58,56,211,79,95,237,95,59,224,92,191,89,199,208,215,98,250,233,23,117,39,196,227,156,183,13,125,44,166,159,126,145,111,244,63,201,234,47,181,105,254,37,45,166,159,91,216,151,229,98,192,69,100,215,248,119,231,224,129,126,170,219,254,89,84,201,82,208,71,217,31,112,121,255,184,42,103,193,69,152,175,38,159,145,195,18,148,253,161,242,107,125,175,113,9,30,13,115,253,209,9,244,161,236,15,249,95,30,153,60,206,234,63,201,225,91,47,65,217,31,242,111,93,123,181,67,240,36,227,213,142,51,164,242,83,92,159,54,178,83,130,82,63,212,126,253,34,248,17,156,235,187,119,70,227,75,169,31,112,119,241,149,214,135,78,253,196,194,229,207,76,127,166,159,126,81,222,41,129,51,253,244,17,90,42,172,114,199,171,9,70,73,191,116,8,102,9,226,114,156,66,118,196,220,165,124,62,60,146,111,15,224,34,204,157,91,186,122,216,17,115,151,10,62,39,104,225,148,177,35,230,46,229,243,230,62,62,167,176,35,230,46,229,243,207,143,102,249,241,48,119,135,19,147,39,56,127,69,2,207,54,62,71,9,184,158,69,70,253,41,198,171,115,55,215,15,184,185,75,249,65,249,217,131,81,191,197,244,211,187,84,239,14,156,233,167,238,189,216,101,237,136,197,244,243,42,125,117,181,53,186,64,65,101,240,50,76,63,4,201,102,219,76,0,189,33,83,188,156,92,131,11,206,235,206,229,216,224,34,204,135,222,183,138,193,237,80,253,243,67,119,113,108,240,232,54,151,135,244,194,120,205,99,33,255,74,1,19,125,205,227,219,124,85,157,225,94,179,230,137,16,47,191,35,16,90,243,100,168,254,206,53,2,133,53,79,133,120,239,5,87,83,205,253,183,82,102,249,52,68,154,135,245,191,60,218,246,15,235,183,104,44,39,70,251,172,104,88,223,35,90,130,154,135,244,241,170,175,8,151,215,60,164,143,10,180,240,86,85,115,17,30,223,201,185,83,160,189,116,109,194,134,56,103,74,140,143,204,31,170,155,153,135,7,205,41,7,230,204,7,155,35,26,15,154,83,130,68,127,186,61,158,251,57,192,32,193,99,71,57,87,243,11,123,181,77,183,127,211,159,134,216,191,221,255,242,151,63,253,15,78,204,160,147,18,48,0,0};
const char firmwareBundleJS[] PROGMEM = R"MOSJS(function firmwareCRC(bytes) {
  let crc = 0xffffffff;
  for (const b of bytes) {
    crc ^= b;
    for (let i = 0; i < 8; i++) crc = (crc >>> 1) ^ (crc & 1 ? 0xedb88320 : 0);
  }
  return ((crc ^ 0xffffffff) >>> 0).toString(16).padStart(8, "0");
}
// The install image is also the update container. Only its verified application
// is sent to OTA; the installed compatible resources and user data stay in place.
const FirmwareBundle = {
  program(bytes) {
    if (bytes[0] !== 0xe9) throw new Error("invalid");
    if (bytes.length <= 0x100000) return bytes; // Existing program-only uploads.
    if (bytes.length !== 0x3fa000) throw new Error("invalid");
    const offset = 0x100000;
    if (String.fromCharCode(...bytes.slice(offset, offset + 8)) !== "MOS2ALL1")
      throw new Error("invalid");
    const view = new DataView(bytes.buffer, bytes.byteOffset + offset, 24);
    if (view.getUint32(20, true) !== 1) throw new Error("invalid");
    const length = view.getUint32(8, true);
    if (length < 4096 || length > offset) throw new Error("size");
    const crc = (part) => parseInt(firmwareCRC(part), 16);
    const program = bytes.slice(0, length);
    if (
      crc(program) !== view.getUint32(12, true) ||
      crc(bytes.slice(0x200000)) !== view.getUint32(16, true)
    )
      throw new Error("invalid");
    return program;
  },
};
)MOSJS";
const char firmwareBundleJSGzip[] PROGMEM = {31,139,8,0,0,0,0,0,2,255,141,84,93,107,219,48,20,125,207,175,56,203,195,176,104,230,218,110,41,217,92,103,116,89,7,131,142,192,218,237,101,172,160,216,114,42,112,36,35,203,253,216,210,255,190,107,89,166,78,41,172,134,68,190,246,61,231,158,123,174,228,178,85,185,149,90,161,148,102,123,199,141,88,126,95,6,235,7,43,26,134,191,19,160,18,22,185,201,145,33,186,47,253,149,210,243,82,27,4,185,86,141,197,26,186,196,8,2,7,184,206,176,78,93,228,82,59,30,217,177,164,180,156,98,78,203,193,1,243,212,65,183,44,22,11,196,12,215,125,244,22,49,62,82,77,81,172,231,243,163,36,194,7,68,172,227,123,164,159,17,182,53,10,129,203,188,30,41,99,142,37,98,161,213,151,214,72,181,9,226,19,22,214,188,184,180,220,216,96,62,195,52,154,18,205,227,228,240,16,87,55,2,146,26,224,85,5,185,229,27,138,26,240,170,209,176,244,166,173,11,110,5,168,69,203,165,18,38,196,74,85,15,144,182,193,173,48,178,148,162,0,175,235,74,230,188,243,175,35,36,120,35,148,133,213,88,93,157,165,142,198,23,160,228,92,111,107,74,93,87,130,244,55,186,53,185,160,114,170,64,219,8,3,42,198,65,169,84,65,161,174,120,46,194,73,111,239,23,63,152,79,173,42,8,155,57,143,107,163,55,134,111,131,61,219,101,137,254,193,175,232,55,222,100,221,200,196,123,70,50,140,190,131,18,119,56,55,70,155,96,42,213,45,175,100,49,101,233,62,44,172,132,218,216,27,156,118,200,56,234,46,54,120,237,18,82,80,151,231,247,178,177,100,237,160,225,157,238,124,105,235,74,243,162,9,95,102,236,197,28,149,220,81,254,79,80,223,183,46,203,134,54,205,147,150,39,181,253,108,195,210,232,237,242,134,155,165,46,68,16,134,97,95,178,161,145,136,160,71,207,6,150,3,204,25,115,50,166,223,86,151,201,217,197,69,60,101,142,15,175,148,115,43,41,33,115,105,159,105,86,63,41,244,61,174,219,178,20,102,6,31,209,255,106,40,58,168,72,142,71,94,119,76,225,70,216,31,82,217,163,36,72,162,25,172,105,69,47,47,126,173,61,222,217,12,207,232,230,158,237,169,222,48,85,28,71,239,79,176,219,13,208,133,151,247,66,197,70,254,17,251,229,252,65,173,233,24,49,100,11,208,77,35,190,42,27,140,191,27,238,237,12,116,230,198,80,191,79,8,62,30,16,117,221,235,24,41,245,3,161,90,129,7,245,166,60,107,49,78,6,199,118,187,17,100,143,253,62,233,247,239,203,4,39,158,192,161,95,187,15,252,73,240,202,220,183,104,54,121,76,39,255,0,122,154,93,101,67,5,0,0};
const char setupJS[] PROGMEM = R"MOSJS(let networkItems = [],
  scanState = "select";
function updateNetworkLabels() {
  const networks = document.getElementById("networks"),
    selected = networks.value;
  networks.replaceChildren(new Option(accessText("select"), ""));
  for (const item of networkItems)
    networks.add(
      new Option(
        item.ssid +
          " · " +
          item.rssi +
          " dBm" +
          (item.open ? " · " + accessText("open") : ""),
        item.ssid,
      ),
    );
  networks.value = selected;
  document.getElementById("scanStatus").textContent =
    scanState === "select" ? "" : accessText(scanState);
}
if (setupMode) {
  const networks = document.getElementById("networks"),
    button = document.getElementById("rescan"),
    ssid = document.getElementById("ssid");
  let scanning = false,
    setupSaved = false;
  async function scan(refresh = false) {
    if (scanning || setupSaved) return;
    scanning = true;
    button.disabled = true;
    scanState = "scanning";
    updateNetworkLabels();
    try {
      const response = await fetch(
        "/api/wifi/scan" + (refresh ? "?refresh=1" : ""),
      );
      if (!response.ok) throw Error();
      const data = await response.json();
      if (data.scanning) {
        setTimeout(() => scan(), 1000);
        return;
      }
      networkItems = data.networks;
      scanState = networkItems.length ? "select" : "empty";
    } catch (_) {
      scanState = "scanFailed";
    } finally {
      scanning = false;
      button.disabled = false;
      updateNetworkLabels();
    }
  }
  networks.onchange = () => {
    ssid.value = networks.value;
  };
  button.onclick = () => scan(true);
  scan();
  accessForm.onsubmit = async (event) => {
    event.preventDefault();
    const submit = document.getElementById("submitButton"),
      feedback = document.getElementById("feedback");
    submit.disabled = true;
    submit.textContent = accessText("saving");
    feedback.dataset.error = "";
    feedback.textContent = "";
    try {
      const response = await fetch("/wifi/save", {
        method: "POST",
        body: new URLSearchParams(new FormData(accessForm)),
      });
      const data = await response.json();
      if (!response.ok) throw Error(data.error || "failed");
      setupSaved = true;
      document.activeElement?.blur();
      document.getElementById("savedNetwork").dataset.ssid = ssid.value;
      accessForm.classList.add("hidden");
      document.getElementById("hint").classList.add("hidden");
      document.getElementById("saved").classList.remove("hidden");
      translateAccess();
      window.scrollTo(0, 0);
    } catch (error) {
      feedback.dataset.error = accessMessages.zh[error.message]
        ? error.message
        : "failed";
      feedback.textContent = accessText(feedback.dataset.error);
      submit.disabled = false;
      submit.textContent = accessText("submit");
    }
  };
}
)MOSJS";
const char setupJSGzip[] PROGMEM = {31,139,8,0,0,0,0,0,2,255,165,86,203,110,219,58,16,221,231,43,120,185,146,112,13,37,221,38,112,131,166,15,160,64,250,64,147,174,138,162,160,165,145,197,134,34,5,146,178,235,182,254,174,187,191,95,214,33,41,82,82,92,167,69,187,136,35,115,94,135,51,231,140,44,192,18,9,118,171,244,221,75,11,173,33,75,242,225,227,226,132,16,83,50,121,99,153,5,60,161,6,4,148,150,94,156,212,189,44,45,87,146,244,93,133,182,215,33,242,154,173,64,152,44,39,223,48,176,84,210,164,156,46,95,165,202,190,5,105,139,53,216,231,2,220,227,213,238,101,149,209,232,67,115,87,16,75,250,42,80,97,76,52,21,27,38,122,184,64,115,58,209,208,9,86,194,211,134,139,74,131,204,36,108,201,155,206,129,202,88,89,130,49,183,240,197,102,17,114,190,32,148,230,185,203,80,43,77,178,128,142,227,85,137,170,103,55,207,61,134,84,134,85,85,230,79,220,89,170,48,156,16,159,161,48,134,87,228,223,116,70,8,37,255,255,135,31,211,35,239,168,209,243,158,99,117,213,206,29,51,239,169,58,144,228,50,37,34,211,43,57,27,205,201,185,187,209,226,16,73,60,26,108,249,172,107,190,143,216,216,216,99,103,60,58,152,56,251,30,71,83,88,172,253,84,73,139,86,178,12,115,26,169,177,28,201,225,80,83,4,55,65,156,28,17,203,254,132,215,36,51,96,251,238,149,170,224,111,185,178,234,173,69,26,62,16,161,193,149,79,220,114,163,122,192,219,217,169,111,153,64,69,184,72,201,229,26,35,106,38,12,68,126,34,248,27,182,241,12,245,231,206,159,153,157,44,73,18,134,11,205,52,212,88,190,137,110,225,178,56,42,215,129,152,250,251,247,73,194,156,104,124,214,242,34,53,120,40,111,117,160,127,188,113,81,113,195,86,194,67,24,109,115,177,14,209,52,216,126,170,212,96,178,122,55,32,139,163,64,212,29,62,184,60,108,203,184,37,53,216,178,25,105,79,79,89,199,79,183,188,230,167,190,187,200,208,116,89,156,255,229,240,188,124,68,231,52,29,10,134,22,252,19,171,20,234,46,39,182,209,106,75,158,107,173,116,150,220,2,26,4,206,18,146,20,244,217,160,16,103,9,157,95,17,175,157,167,43,249,137,221,242,22,84,111,51,220,78,203,199,97,58,184,19,30,157,157,157,165,20,100,214,124,66,246,73,247,179,189,232,171,68,30,70,223,105,231,167,254,133,0,185,182,190,41,81,31,216,16,104,59,187,27,230,178,39,37,195,222,146,236,211,136,248,96,142,47,24,199,89,167,136,154,75,38,196,110,230,63,229,105,68,117,200,149,153,249,1,74,184,187,239,167,139,67,201,178,97,114,237,16,133,22,126,75,122,74,75,229,112,91,239,221,199,128,2,51,8,94,222,165,4,126,6,142,187,190,100,152,136,23,146,95,28,47,148,198,45,40,77,191,106,113,232,203,65,94,25,108,80,170,147,250,254,123,209,105,255,255,25,212,172,23,54,222,33,144,39,101,56,174,121,239,113,229,81,142,84,173,1,170,21,243,120,143,70,70,31,58,84,12,153,142,104,51,216,102,107,116,182,214,13,219,56,181,14,169,98,234,194,209,13,249,91,128,83,134,227,3,189,231,48,207,24,205,191,173,105,58,200,24,215,15,93,76,52,211,130,109,84,133,100,125,251,230,230,150,142,239,153,149,170,118,231,254,93,248,254,221,245,13,48,93,54,111,153,102,173,241,111,96,55,182,103,136,56,27,167,152,167,150,238,255,80,215,199,23,133,215,98,232,12,238,81,90,7,153,164,224,217,166,30,71,49,121,229,49,220,214,27,24,134,122,89,172,68,63,217,62,199,25,227,82,14,186,193,87,99,28,209,240,114,25,53,17,19,77,40,93,10,102,204,53,55,214,255,178,160,13,175,42,247,54,255,101,201,134,75,252,21,243,199,225,30,241,44,94,67,171,54,112,152,194,106,38,141,192,189,240,196,131,30,187,177,229,178,82,91,220,175,90,9,113,171,178,179,5,137,187,51,237,48,63,138,113,143,29,37,113,104,200,43,252,99,107,48,197,215,230,131,183,20,109,56,249,152,216,118,73,102,134,116,126,158,134,125,113,191,214,81,133,253,28,205,72,150,3,241,206,150,229,175,245,235,29,232,116,129,186,159,59,63,0,150,184,63,216,90,11,0,0};
const char accessJS[] PROGMEM = R"MOSJS(const accessMode = document.querySelector("main").dataset.mode;
const setupMode = accessMode === "setup";
const accessForm = document.getElementById("accessForm");
let accessLanguage;
try {
  accessLanguage = localStorage.getItem("mos2-access-language") || "zh";
} catch (_) {
  accessLanguage = "zh";
}
if (accessLanguage !== "en") accessLanguage = "zh";
const accessMessages = AccessI18n;
for (const language of ["zh", "en"]) {
  accessMessages[language].title =
    accessMessages[language][setupMode ? "setupTitle" : "loginTitle"];
  accessMessages[language].submit =
    accessMessages[language][setupMode ? "setupSubmit" : "loginSubmit"];
}
const accessText = (key) => accessMessages[accessLanguage][key] || key;
function translateAccess() {
  document.documentElement.lang = accessLanguage;
  document.getElementById("accessLanguage").value = accessLanguage;
  document.getElementById("accessLang").value = accessLanguage;
  document.getElementById("title").textContent = accessText("title");
  document
    .querySelectorAll("[data-text]")
    .forEach((el) => (el.textContent = accessText(el.dataset.text)));
  const feedback = document.getElementById("feedback");
  feedback.textContent = feedback.dataset.error
    ? accessText(feedback.dataset.error)
    : "";
  document.getElementById("hint").textContent = setupMode
    ? accessText("hint")
    : "";
  if (typeof updateNetworkLabels === "function") updateNetworkLabels();
  const savedNetwork = document.getElementById("savedNetwork");
  if (savedNetwork.dataset.ssid)
    savedNetwork.textContent =
      accessText("networkSaved") + savedNetwork.dataset.ssid;
}
document.getElementById("wifiFields").classList.toggle("hidden", !setupMode);
document.getElementById("ssid").disabled = !setupMode;
document.getElementById("ssid").required = setupMode;
document.getElementById("accessLanguage").onchange = (event) => {
  accessLanguage = event.target.value;
  try {
    localStorage.setItem("mos2-access-language", accessLanguage);
  } catch (_) {}
  translateAccess();
};
translateAccess();

const passwordDialog = document.getElementById("passwordDialog");
passwordDialog.addEventListener("cancel", (event) => event.preventDefault());
if (accessMode === "change") {
  accessForm.classList.add("hidden");
  passwordDialog.showModal();
}
if (new URLSearchParams(location.search).get("password") === "changed") {
  const feedback = document.getElementById("feedback");
  feedback.dataset.error = "passwordChanged";
  feedback.className = "success";
}
translateAccess();
document
  .getElementById("passwordForm")
  .addEventListener("submit", async (event) => {
    event.preventDefault();
    const form = event.target,
      button = document.getElementById("passwordSubmit"),
      feedback = document.getElementById("passwordFeedback");
    if (
      form.newPassword.value === "12345678" ||
      form.newPassword.value !== form.confirmPassword.value
    ) {
      feedback.textContent = accessText("passwordInvalid");
      return;
    }
    button.disabled = true;
    feedback.textContent = "";
    try {
      const response = await fetch("/password/change", {
        method: "POST",
        body: new URLSearchParams(new FormData(form)),
      });
      const result = await response.json();
      if (!response.ok || !result.changed)
        throw new Error(result.error || "saveFailed");
      location.replace("/manage?password=changed");
    } catch (error) {
      feedback.textContent = accessText(error.message);
      button.disabled = false;
    }
  });
)MOSJS";
const char accessJSGzip[] PROGMEM = {31,139,8,0,0,0,0,0,2,255,165,87,91,111,219,54,20,126,247,175,96,244,36,97,142,130,118,183,162,134,23,116,77,2,4,72,187,96,206,158,12,99,160,37,218,214,66,145,46,73,197,243,214,252,247,157,195,155,37,219,114,211,238,33,144,201,115,229,119,174,41,164,208,134,208,162,96,90,127,144,37,35,99,82,202,162,169,153,48,249,167,134,169,237,132,113,86,24,169,210,164,166,149,72,178,188,164,134,106,102,242,26,184,71,131,194,202,195,185,89,123,241,182,174,241,152,36,150,150,4,78,71,189,145,170,110,91,90,50,115,205,25,254,252,117,123,91,166,201,142,43,201,70,3,206,130,220,29,21,203,134,46,193,174,81,91,242,239,128,236,221,131,78,46,11,202,39,224,48,28,81,239,173,97,53,248,46,245,235,115,199,123,206,61,115,146,145,207,159,73,242,207,10,156,123,38,5,53,197,138,164,127,102,199,213,122,182,65,181,32,233,30,241,12,95,201,0,154,62,169,246,203,63,192,31,208,52,16,223,217,139,219,87,111,196,104,176,144,138,164,142,47,184,71,228,130,76,81,193,208,106,159,181,29,11,90,166,129,121,150,155,202,112,48,9,44,253,76,211,93,156,46,125,96,30,80,44,33,111,73,194,229,178,18,238,56,27,157,178,164,155,121,93,153,175,55,53,177,114,59,91,254,60,67,88,219,16,61,176,191,65,59,73,31,217,54,35,227,95,246,77,116,65,158,77,129,109,134,129,132,47,224,216,136,194,84,82,16,163,168,208,156,26,230,80,78,29,122,49,225,194,15,159,117,57,250,29,115,119,151,101,228,75,41,122,23,115,41,127,162,188,97,223,170,227,219,228,109,204,65,212,0,98,239,165,48,64,136,10,16,197,200,209,214,98,195,214,173,237,119,156,167,201,20,11,251,28,85,205,146,204,49,65,86,94,211,98,149,166,140,219,72,192,183,223,22,208,66,107,64,158,44,179,86,93,96,23,140,149,115,90,60,158,170,249,192,227,188,13,167,61,123,241,58,152,98,74,73,101,189,189,108,59,115,156,207,61,11,242,47,57,9,235,170,18,230,0,213,152,207,135,198,188,64,71,57,246,9,179,93,51,40,226,102,13,78,176,143,204,108,164,122,188,163,115,198,181,235,140,33,89,161,115,28,225,73,91,248,105,250,196,74,79,61,133,97,155,207,225,136,126,180,111,35,32,90,87,165,115,185,67,238,60,218,146,73,231,169,194,241,77,80,6,252,254,142,244,42,199,178,238,117,116,83,45,170,155,138,241,82,3,208,5,167,144,240,149,134,196,145,203,37,103,8,104,89,66,207,27,146,179,8,59,188,166,255,217,96,14,39,83,165,233,156,179,18,16,218,201,125,89,76,177,79,77,165,172,216,75,164,14,10,95,138,98,5,39,172,221,148,61,1,159,173,149,163,83,196,146,115,67,21,40,117,5,143,49,10,179,140,116,135,151,62,57,188,134,123,218,109,180,59,99,236,217,170,222,107,131,16,21,28,158,7,183,190,3,175,33,18,16,201,242,170,162,208,163,79,165,90,151,19,147,173,123,147,211,178,188,198,231,98,96,153,96,176,64,20,84,20,140,131,231,45,152,28,34,107,101,191,87,108,65,27,110,82,108,29,187,65,187,91,37,28,206,73,123,18,226,142,208,74,32,48,26,179,199,34,178,231,148,94,201,13,232,163,220,34,97,141,8,182,33,127,252,126,55,97,84,21,171,123,170,104,173,83,140,3,86,38,196,0,111,51,124,253,238,201,224,64,203,157,210,251,243,191,59,93,167,85,225,242,16,236,189,247,118,58,220,246,205,31,105,109,215,12,221,88,48,236,134,114,36,184,173,214,223,27,71,183,108,33,199,97,224,220,196,199,148,211,91,81,236,103,57,233,9,226,200,210,60,44,110,225,107,231,255,208,119,151,121,99,12,12,236,23,164,154,95,24,178,32,249,18,172,227,243,58,152,187,190,24,212,96,10,65,22,220,123,214,48,137,49,196,175,94,127,255,195,143,63,253,252,38,129,13,227,52,59,110,129,150,4,15,94,84,170,238,146,173,108,230,225,234,157,110,237,70,27,28,191,21,160,0,251,212,200,203,42,232,81,74,184,211,243,96,135,96,187,253,25,229,58,75,175,37,55,164,218,173,39,68,74,49,189,134,31,118,19,217,208,10,51,26,58,74,154,92,4,135,46,124,21,14,163,32,33,53,51,43,89,194,236,187,255,109,242,144,12,227,253,92,150,219,183,228,88,133,225,29,166,220,21,36,125,138,184,101,49,172,207,241,169,209,35,200,167,232,79,112,48,255,75,75,145,70,94,12,232,89,164,201,71,92,9,207,156,104,238,11,53,139,126,153,149,146,27,235,215,53,22,91,234,249,92,229,225,63,5,56,213,110,104,197,89,11,248,216,20,20,91,115,90,192,152,186,168,169,128,214,123,25,160,25,199,142,224,195,19,218,177,219,62,190,34,252,86,32,175,221,206,27,61,56,12,244,130,114,205,118,185,128,208,253,7,187,72,40,7,210,13,0,0};
const char accessHTML[] PROGMEM = R"MOSWEB(<!doctype html>
<html lang="zh">
  <meta charset="utf-8" /><meta
    name="viewport"
    content="width=device-width,initial-scale=1"
  /><title>MOS2-NAS</title
  ><style>
    * {
      box-sizing: border-box;
    }
    body {
      margin: 0;
      background: #f4f6f8;
      color: #25313b;
      font: 14px system-ui;
    }
    main {
      width: calc(100% - 32px);
      max-width: 400px;
      margin: 8vh auto;
      padding: 24px;
      background: #fff;
      border: 1px solid #dfe5ea;
      border-radius: 12px;
    }
    header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
    }
    h1 {
      font-size: 20px;
      margin: 0;
    }
    h2 {
      font-size: 16px;
      margin: 24px 0 18px;
    }
    label {
      display: block;
      margin: 16px 0;
      color: #566573;
    }
    input,
    select,
    button {
      font: inherit;
      border: 1px solid #d3dce3;
      border-radius: 6px;
      padding: 11px;
      width: 100%;
      margin-top: 7px;
      background: #fff;
      color: #25313b;
    }
    header select {
      width: auto;
      margin: 0;
      padding: 6px;
    }
    button {
      cursor: pointer;
      background: #157e87;
      color: white;
      border-color: #157e87;
    }
    .networkRow {
      display: flex;
      gap: 8px;
      align-items: center;
    }
    .networkRow select {
      flex: 1;
      min-width: 0;
    }
    .networkRow button {
      width: auto;
      background: #f6f8fa;
      color: #344653;
      border-color: #dce3e8;
    }
    p {
      line-height: 1.6;
      color: #70808e;
      font-size: 13px;
    }
    .steps {
      padding-left: 22px;
      line-height: 1.8;
      color: #566573;
    }
    .steps li {
      margin: 8px 0;
    }
    .error {
      color: #b84b42;
    }
    .success {
      padding: 16px;
      background: #edf7f3;
      border-radius: 8px;
      color: #287b58;
    }
    .hidden {
      display: none !important;
    }
    a {
      color: #157e87;
    }
    dialog {
      width: calc(100% - 32px);
      max-width: 380px;
      padding: 24px;
      border: 1px solid #dfe5ea;
      border-radius: 12px;
      color: #25313b;
    }
    dialog::backdrop {
      background: #162a3b55;
    }
    dialog h2 {
      margin-top: 0;
    }
  </style>
  <main data-mode="{{mode}}">
    <header>
      <h1>MOS2-NAS</h1>
      <select id="accessLanguage" aria-label="Language">
        <option value="zh">中文</option>
        <option value="en">English</option>
      </select>
    </header>
    <h2 id="title">{{entryTitle}}</h2>
    <form id="accessForm" method="post" action="{{action}}">
      <input type="hidden" name="token" value="{{token}}" />
      <div id="wifiFields" class="{{wifiClass}}">
        <label
          ><span data-text="network">Wi-Fi 网络</span>
          <div class="networkRow">
            <select id="networks"></select
            ><button type="button" id="rescan" data-text="scan">刷新</button>
          </div></label
        >
        <p id="scanStatus"></p>
        <label
          ><span data-text="ssid">网络名称</span
          ><input
            {{ssidDisabled}}
            id="ssid"
            name="ssid"
            value="{{ssid}}"
            maxlength="32"
            autocomplete="off"
        /></label>
      </div>
      <label
        ><span data-text="password">密码</span
        ><input
          name="password"
          type="password"
          required
          maxlength="64"
          autocomplete="current-password"
      /></label>
      <p id="feedback" class="error" role="status" data-error="{{error}}"></p>
      <button id="submitButton" data-text="submit">{{entrySubmit}}</button
      ><input type="hidden" name="language" id="accessLang" value="zh" />
    </form>
    <div id="saved" class="hidden" role="status">
      <h2 class="success" data-text="saved">网络已保存</h2>
      <p id="savedNetwork"></p>
      <ol class="steps">
        <li data-text="step1"></li>
        <li data-text="step2"></li>
        <li data-text="step3"></li>
      </ol>
      <p data-text="retry"></p>
    </div>
    <p id="hint"></p>
  </main>
  <dialog id="passwordDialog" aria-labelledby="changeTitle">
    <h2 id="changeTitle" data-text="changeTitle"></h2>
    <p data-text="changeHint"></p>
    <form id="passwordForm">
      <input type="hidden" name="token" value="{{token}}" /><label
        ><span data-text="newPassword"></span
        ><input
          name="newPassword"
          type="password"
          minlength="8"
          maxlength="64"
          required
          autocomplete="new-password" /></label
      ><label
        ><span data-text="confirmPassword"></span
        ><input
          name="confirmPassword"
          type="password"
          minlength="8"
          maxlength="64"
          required
          autocomplete="new-password"
      /></label>
      <p id="passwordFeedback" class="error" role="status"></p>
      <button id="passwordSubmit" data-text="changeSubmit"></button>
    </form>
  </dialog>
  <script src="/access-i18n.js?v=1.2.0"></script>
  <script src="/access.js"></script>
  <script src="/setup.js"></script>
</html>
)MOSWEB";
