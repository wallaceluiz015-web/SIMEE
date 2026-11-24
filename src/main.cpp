#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "EmonLib.h"
#include <WiFiManager.h>
#include <WebServer.h>
#include <ThingSpeak.h>
#include <math.h>

// DISPLAY OLED 128x64
#define LARGURA_OLED 128
#define ALTURA_OLED 64
#define RESET_OLED -1
Adafruit_SSD1306 display(LARGURA_OLED, ALTURA_OLED, &Wire, RESET_OLED);

// PINOS E SENSORES
EnergyMonitor emonFaseA;
EnergyMonitor emonFaseB;

const int pinoTensao = 32;
const int pinoCorrenteA = 34;
const int pinoCorrenteB = 35;

// THINGSPEAK
unsigned long myChannelNumber = 3572132;
const char * myWriteAPIKey = "CHAVE-API";
WiFiClient client;

// SERVIDOR WEB LOCAL
WebServer server(80);

// VARIAVEIS GLOBAIS COMPARTILHADAS ENTRE OS NUCLEOS
volatile float tensao = 0.0;
volatile float correnteA = 0.0, potenciaA = 0.0;
volatile float correnteB = 0.0, potenciaB = 0.0;

// DASHBOARD HTML LOCAL
const char paginaHTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="theme-color" content="#121212">
<title>SIMEE · Monitoramento de Energia</title>
<style>
  :root { --bg: #121212; --card: #171717; --line: rgba(255,255,255,.08); --ink: #ededed; --muted: rgba(255,255,255,.5); --emerald: #34d399; --blue: #60a5fa; --amber: #fbbf24; --red: #f87171; }
  * { box-sizing: border-box; margin: 0; }
  body { font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; background: var(--bg); color: var(--ink); min-height: 100vh; padding: max(20px, env(safe-area-inset-top)) 16px max(32px, env(safe-area-inset-bottom)); -webkit-font-smoothing: antialiased; }
  .wrap { max-width: 1040px; margin: 0 auto; }
  .num { font-variant-numeric: tabular-nums; font-feature-settings: "tnum" 1; }
  header { display: flex; align-items: center; justify-content: space-between; gap: 12px; margin-bottom: 28px; }
  .brand { display: flex; align-items: center; gap: 12px; }
  .logo { width: 36px; height: 36px; border-radius: 10px; display: grid; place-items: center; color: var(--emerald); background: rgba(52,211,153,.08); border: 1px solid rgba(52,211,153,.3); }
  .icon { width: 16px; height: 16px; stroke: currentColor; stroke-width: 2; fill: none; stroke-linecap: round; stroke-linejoin: round; flex-shrink: 0; }
  .logo .icon { width: 20px; height: 20px; }
  h1 { font-size: 1rem; font-weight: 600; letter-spacing: -.01em; }
  .sub { font-size: .8125rem; color: var(--muted); margin-top: 1px; }
  .status { display: inline-flex; align-items: center; gap: 9px; padding: 7px 13px; border-radius: 999px; background: var(--card); border: 1px solid var(--line); font-size: .8125rem; font-weight: 500; }
  .dot { position: relative; width: 8px; height: 8px; border-radius: 50%; background: var(--red); }
  .status.online .dot { background: var(--emerald); animation: blink 2s ease-in-out infinite; }
  .status.online .dot::after { content: ""; position: absolute; inset: 0; border-radius: 50%; background: var(--emerald); animation: ring 2s ease-out infinite; }
  @keyframes blink { 0%,100% { opacity: 1; } 50% { opacity: .45; } }
  @keyframes ring { 0% { transform: scale(1); opacity: .6; } 100% { transform: scale(3.2); opacity: 0; } }
  .alert { display: none; align-items: center; gap: 10px; margin-bottom: 20px; padding: 13px 18px; border-radius: 12px; background: rgba(248,113,113,.08); border: 1px solid rgba(248,113,113,.25); color: #fca5a5; font-size: .875rem; }
  body.offline .alert { display: flex; }
  .grid { display: grid; grid-template-columns: repeat(6, 1fr); gap: 16px; }
  .grid > * { grid-column: span 6; }
  .card { position: relative; padding: 28px; border-radius: 16px; background: var(--card); border: 1px solid var(--line); box-shadow: inset 0 1px 0 rgba(255,255,255,.04); }
  .lbl { display: flex; align-items: center; gap: 8px; font-size: .6875rem; font-weight: 600; text-transform: uppercase; letter-spacing: 1px; color: var(--muted); }
  .val { display: flex; align-items: baseline; font-weight: 500; letter-spacing: -.04em; line-height: 1; }
  .val .unit { font-size: .3em; font-weight: 500; letter-spacing: 0; color: var(--muted); margin-left: .45em; }
  .total { border-top-color: rgba(52,211,153,.35); }
  .total .val { font-size: clamp(3rem, 14vw, 5.25rem); margin: 36px 0 32px; }
  .share { height: 6px; border-radius: 99px; display: flex; gap: 3px; overflow: hidden; }
  .share i { display: block; height: 100%; width: 50%; border-radius: 99px; transition: width .9s cubic-bezier(.22,1,.36,1); }
  .share .sa { background: var(--emerald); } .share .sb { background: var(--blue); }
  .legend { display: flex; gap: 22px; margin-top: 14px; font-size: .8125rem; color: var(--muted); }
  .legend b { color: var(--ink); font-weight: 500; margin-left: 6px; }
  .legend span::before { content: ""; display: inline-block; width: 7px; height: 7px; border-radius: 50%; margin-right: 8px; background: var(--emerald); }
  .legend span.lb::before { background: var(--blue); }
  .volt { display: flex; flex-direction: column; justify-content: space-between; min-height: 170px; border-top-color: rgba(96,165,250,.35); }
  .volt .val { font-size: 3rem; margin-top: 28px; }
  .phase-top { display: flex; align-items: center; justify-content: space-between; }
  .chip { font-size: .6875rem; font-weight: 500; padding: 3px 9px; border-radius: 99px; color: var(--emerald); background: rgba(0,0,0,.4); border: 1px solid rgba(52,211,153,.22); transition: color .3s, border-color .3s; }
  .phase.warn .chip { color: var(--amber); border-color: rgba(251,191,36,.25); }
  .phase.danger .chip { color: var(--red); border-color: rgba(248,113,113,.28); }
  .phase.nodata .chip { color: var(--muted); border-color: var(--line); }
  .phase-body { display: flex; align-items: center; gap: 28px; margin-top: 28px; }
  .gauge { position: relative; width: 92px; height: 92px; flex-shrink: 0; }
  .gauge svg { width: 100%; height: 100%; transform: rotate(-90deg); }
  .gauge circle { fill: none; stroke-width: 7; }
  .gauge .track { stroke: rgba(255,255,255,.08); }
  .gauge .bar { stroke: var(--emerald); stroke-linecap: round; stroke-dasharray: 251.33; stroke-dashoffset: 251.33; transition: stroke-dashoffset .9s cubic-bezier(.22,1,.36,1), stroke .3s; }
  .phase.warn .bar { stroke: var(--amber); } .phase.danger .bar { stroke: var(--red); }
  .gauge .mid { position: absolute; inset: 0; display: flex; align-items: baseline; justify-content: center; padding-top: 34px; font-size: 1.125rem; font-weight: 500; letter-spacing: -.02em; }
  .gauge .mid small { font-size: .65rem; color: var(--muted); margin-left: 1px; }
  .stats { display: grid; gap: 22px; min-width: 0; }
  .stats .val { font-size: 1.5rem; margin-top: 10px; letter-spacing: -.03em; }
  .stats .val .unit { font-size: .55em; margin-left: .35em; }
  .stats .val .cap { font-size: .5em; color: rgba(255,255,255,.3); margin-left: .5em; letter-spacing: 0; }
  .chart-head { display: flex; align-items: flex-start; justify-content: space-between; gap: 20px; flex-wrap: wrap; }
  .kpis { display: flex; gap: 32px; }
  .kpis .lbl { gap: 0; }
  .kpis b { display: block; margin-top: 8px; font-size: 1.125rem; font-weight: 500; letter-spacing: -.02em; color: var(--ink); text-transform: none; }
  .chart-box { position: relative; height: 150px; margin: 24px 0 0; width: 100%; overflow: hidden; }
  .chart-svg { width: 100%; height: 100%; display: block; overflow: visible; }
  .axis { display: flex; justify-content: space-between; margin-top: 12px; font-size: .75rem; color: rgba(255,255,255,.3); }
  footer { margin-top: 28px; text-align: center; font-size: .75rem; color: var(--muted); }
  body.offline .total .val, body.offline .volt .val { color: rgba(255,255,255,.4); }
  @media (min-width: 640px) { body { padding: 40px 28px; } .grid { gap: 20px; } .card { padding: 32px; } .phase { grid-column: span 3; } }
  @media (min-width: 900px) { .grid { gap: 24px; } .total { grid-column: span 4; } .volt { grid-column: span 2; } }
</style>
</head>
<body>
<div class="wrap">
  <header>
    <div class="brand">
      <div class="logo">
        <svg class="icon" viewBox="0 0 24 24"><polygon points="13 2 3 14 12 14 11 22 21 10 12 10 13 2"/></svg>
      </div>
      <div><h1>SIMEE</h1><p class="sub">Monitoramento de eficiência energética</p></div>
    </div>
    <div class="status" id="status" role="status" aria-live="polite"><span class="dot"></span><span id="statusText">Conectando...</span></div>
  </header>
  <div class="alert" id="alert">
    <svg class="icon" viewBox="0 0 24 24"><line x1="1" y1="1" x2="23" y2="23"/><path d="M16.72 11.06A10.94 10.94 0 0 1 19 12.55"/><path d="M5 12.55a10.94 10.94 0 0 1 5.17-2.39"/><path d="M10.71 5.05A16 16 0 0 1 22.58 9"/><path d="M1.42 9a15.91 15.91 0 0 1 4.7-2.88"/><path d="M8.53 16.11a6 6 0 0 1 6.95 0"/><line x1="12" y1="20" x2="12.01" y2="20"/></svg>
    Sem comunicação com o equipamento. Reconectando automaticamente...
  </div>
  <main class="grid">
    <section class="card total" aria-label="Potência total">
      <p class="lbl">
        <svg class="icon" viewBox="0 0 24 24"><rect x="2" y="7" width="16" height="10" rx="2"/><line x1="22" y1="11" x2="22" y2="13"/><polyline points="11 10 9 12 12 12 10 14"/></svg>
        Potência total consumida
      </p>
      <p class="val num"><span id="total">-</span><span class="unit">W</span></p>
      <div class="share" aria-hidden="true"><i class="sa" id="shareA"></i><i class="sb" id="shareB"></i></div>
      <div class="legend"><span>Fase A<b class="num" id="pctA">-</b></span><span class="lb">Fase B<b class="num" id="pctB">-</b></span></div>
    </section>
    <section class="card volt" aria-label="Tensão da rede">
      <p class="lbl">
        <svg class="icon" viewBox="0 0 24 24"><polygon points="13 2 3 14 12 14 11 22 21 10 12 10 13 2"/></svg>
        Tensão da rede
      </p>
      <p class="val num"><span id="tensao">-</span><span class="unit">V</span></p>
    </section>
    <section class="card phase" id="phaseA" aria-label="Fase A">
      <div class="phase-top">
        <p class="lbl">
          <svg class="icon" viewBox="0 0 24 24"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"/></svg>
          Fase A
        </p>
        <span class="chip" id="chipA">Normal</span>
      </div>
      <div class="phase-body">
        <div class="gauge" title="Carga">
          <svg viewBox="0 0 100 100"><circle class="track" cx="50" cy="50" r="40"/><circle class="bar" id="barA" cx="50" cy="50" r="40"/></svg>
          <div class="mid num"><span id="aCarga">-</span><small>%</small></div>
        </div>
        <div class="stats">
          <div><p class="lbl">Corrente</p><p class="val num"><span id="aCorrente">-</span><span class="unit">A</span><span class="cap">/ 20 A</span></p></div>
          <div><p class="lbl">Potência</p><p class="val num"><span id="aPotencia">-</span><span class="unit">W</span></p></div>
        </div>
      </div>
    </section>
    <section class="card phase" id="phaseB" aria-label="Fase B">
      <div class="phase-top">
        <p class="lbl">
          <svg class="icon" viewBox="0 0 24 24"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"/></svg>
          Fase B
        </p>
        <span class="chip" id="chipB">Normal</span>
      </div>
      <div class="phase-body">
        <div class="gauge" title="Carga">
          <svg viewBox="0 0 100 100"><circle class="track" cx="50" cy="50" r="40"/><circle class="bar" id="barB" cx="50" cy="50" r="40"/></svg>
          <div class="mid num"><span id="bCarga">-</span><small>%</small></div>
        </div>
        <div class="stats">
          <div><p class="lbl">Corrente</p><p class="val num"><span id="bCorrente">-</span><span class="unit">A</span><span class="cap">/ 20 A</span></p></div>
          <div><p class="lbl">Potência</p><p class="val num"><span id="bPotencia">-</span><span class="unit">W</span></p></div>
        </div>
      </div>
    </section>
    
    <!-- GRÁFICO EM SVG NATIVO OFFLINE -->
    <section class="card" aria-label="Consumo em tempo real">
      <div class="chart-head">
        <p class="lbl">
          <svg class="icon" viewBox="0 0 24 24"><polyline points="23 6 13.5 15.5 8.5 10.5 1 18"/><polyline points="17 6 23 6 23 12"/></svg>
          Consumo em Tempo Real (Sessão)
        </p>
        <div class="kpis">
          <div><p class="lbl">Pico</p><b class="num" id="kPico">0 W</b></div>
          <div><p class="lbl">Média</p><b class="num" id="kMedia">0 W</b></div>
        </div>
      </div>
      <div class="chart-box">
        <svg class="chart-svg" viewBox="0 0 600 150" preserveAspectRatio="none">
          <defs>
            <linearGradient id="gradEmerald" x1="0" y1="0" x2="0" y2="1">
              <stop offset="0%" stop-color="#34d399" stop-opacity="0.32"/>
              <stop offset="100%" stop-color="#34d399" stop-opacity="0.0"/>
            </linearGradient>
          </defs>
          <path id="svgArea" fill="url(#gradEmerald)" d="M0,150 L600,150 Z"/>
          <path id="svgLine" fill="none" stroke="#34d399" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round" d="M0,148 L600,148"/>
        </svg>
      </div>
      <div class="axis"><span id="lblInicio">Início da sessão</span><span id="lblAgora">Agora</span></div>
    </section>
  </main>
  <footer>Última atualização: <span class="num" id="updated">-</span></footer>
</div>

<script>
  const API_URL = '/api/dados';
  const INTERVALO_MS = 3000;
  const TIMEOUT_MS = 5000;
  const DISJUNTOR_A = 20; 
  const ALERTA_A = 15; 
  const CRITICO_A = 18;

  const $ = (id) => document.getElementById(id);
  const fmt = (n, c = 0) => n.toLocaleString('pt-BR', { minimumFractionDigits: c, maximumFractionDigits: c });
  const ok = (n) => typeof n === 'number' && isFinite(n);

  let histDados = [];
  let falhasSeguidas = 0;

  function animar(el, para, casas) {
    cancelAnimationFrame(el._raf);
    if (!ok(para)) { el.textContent = '-'; el._v = null; return; }
    const de = ok(el._v) ? el._v : 0;
    if (de === para) { el.textContent = fmt(para, casas); el._v = para; return; }
    const t0 = performance.now(), dur = 500;
    const passo = (t) => {
      const p = Math.min(1, (t - t0) / dur);
      const e = 1 - Math.pow(1 - p, 3);
      el._v = de + (para - de) * e;
      el.textContent = fmt(el._v, casas);
      if (p < 1) el._raf = requestAnimationFrame(passo);
    };
    el._raf = requestAnimationFrame(passo);
  }

  const CIRC = 2 * Math.PI * 40;
  function gauge(fase, corrente) {
    const card = $('phase' + fase), bar = $('bar' + fase), chip = $('chip' + fase);
    card.classList.remove('warn', 'danger', 'nodata');
    if (!ok(corrente)) { card.classList.add('nodata'); bar.style.strokeDashoffset = CIRC; chip.textContent = 'Sem dados'; return; }
    bar.style.strokeDashoffset = CIRC * (1 - Math.min(corrente / DISJUNTOR_A, 1));
    if (corrente > CRITICO_A) { card.classList.add('danger'); chip.textContent = 'Crítico'; }
    else if (corrente > ALERTA_A) { card.classList.add('warn'); chip.textContent = 'Atenção'; }
    else chip.textContent = 'Normal';
  }

  function desenharGraficoSVG() {
    if (histDados.length === 0) return;
    const W = 600, H = 150, PAD = 12;
    const maxVal = Math.max(...histDados, 100);
    const minVal = 0;
    const faixa = (maxVal - minVal) || 1;

    const pontos = histDados.map((v, i) => {
      const x = histDados.length === 1 ? W : (i / (histDados.length - 1)) * W;
      const y = H - PAD - ((v - minVal) / faixa) * (H - PAD * 2);
      return [x, y];
    });

    if (pontos.length === 1) {
      pontos.unshift([0, pontos[0][1]]);
    }

    const cmdLinha = pontos.map((p, i) => (i === 0 ? 'M' : 'L') + p[0].toFixed(1) + ',' + p[1].toFixed(1)).join(' ');
    const cmdArea = cmdLinha + ` L${W},${H} L0,${H} Z`;

    $('svgLine').setAttribute('d', cmdLinha);
    $('svgArea').setAttribute('d', cmdArea);
  }

  async function buscarDados() {
    const ctrl = new AbortController();
    const t = setTimeout(() => ctrl.abort(), TIMEOUT_MS);
    try {
      const res = await fetch(API_URL, { cache: 'no-store', signal: ctrl.signal });
      if (!res.ok) throw new Error('HTTP ' + res.status);
      return await res.json();
    } finally { clearTimeout(t); }
  }

  function setStatus(on) {
    $('status').classList.toggle('online', on);
    $('statusText').textContent = on ? 'Online' : 'Offline';
    document.body.classList.toggle('offline', !on);
  }

  function render(d) {
    if (!d || d.status !== 'online') throw new Error('Status invalido');
    falhasSeguidas = 0;
    setStatus(true);

    const A = d.faseA || {}, B = d.faseB || {};
    const carga = (f) => ok(f.corrente_a) ? f.corrente_a / DISJUNTOR_A * 100 : NaN;
    animar($('tensao'), d.tensao_v, 1);
    animar($('total'), d.potencia_total_w, 1);
    animar($('aCorrente'), A.corrente_a, 2);
    animar($('aPotencia'), A.potencia_w, 1);
    animar($('aCarga'), carga(A), 0);
    animar($('bCorrente'), B.corrente_a, 2);
    animar($('bPotencia'), B.potencia_w, 1);
    animar($('bCarga'), carga(B), 0);
    gauge('A', A.corrente_a); gauge('B', B.corrente_a);

    const soma = (A.potencia_w || 0) + (B.potencia_w || 0);
    const pa = soma > 0 ? A.potencia_w / soma * 100 : 50;
    $('shareA').style.width = pa + '%'; $('shareB').style.width = (100 - pa) + '%';
    $('pctA').textContent = fmt(pa, 0) + '%'; $('pctB').textContent = fmt(100 - pa, 0) + '%';
    
    const agora = new Date().toLocaleTimeString('pt-BR');
    $('updated').textContent = agora;
    $('lblAgora').textContent = agora;

    histDados.push(d.potencia_total_w || 0);
    if (histDados.length > 30) histDados.shift();
    desenharGraficoSVG();

    $('kPico').textContent = fmt(Math.max(...histDados), 1) + ' W';
    $('kMedia').textContent = fmt(histDados.reduce((a, b) => a + b, 0) / histDados.length, 1) + ' W';
  }

  function renderOffline() {
    setStatus(false);
    ['tensao','total','aCorrente','aPotencia','aCarga','bCorrente','bPotencia','bCarga'].forEach((id) => animar($(id), NaN));
    gauge('A', NaN); gauge('B', NaN);
    $('shareA').style.width = '50%'; $('shareB').style.width = '50%';
    $('pctA').textContent = $('pctB').textContent = '-';
  }

  let ocupado = false;
  async function atualizar() {
    if (ocupado) return;
    ocupado = true;
    try {
      render(await buscarDados());
    } catch (e) {
      falhasSeguidas++;
      // Só declara Offline se falhar 3 tentativas seguidas (evita piscar quando 2 celulares acessam juntos)
      if (falhasSeguidas >= 3) renderOffline();
    } finally {
      ocupado = false;
    }
  }

  atualizar();
  setInterval(atualizar, INTERVALO_MS);
</script>
</body>
</html>
)=====";

void enviarHTML() {

  server.sendHeader("Connection", "close");
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.send_P(200, "text/html", paginaHTML);
}

void enviarDadosJSON() {
  char json[256];
  float t = tensao;
  float cA = correnteA, pA = potenciaA;
  float cB = correnteB, pB = potenciaB;

  snprintf(json, sizeof(json),
           "{\"status\":\"online\",\"tensao_v\":%.1f,\"faseA\":{\"corrente_a\":%.2f,\"potencia_w\":%.1f},\"faseB\":{\"corrente_a\":%.2f,\"potencia_w\":%.1f},\"potencia_total_w\":%.1f}",
           t, cA, pA, cB, pB, (pA + pB));
  

  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

// TAREFA ISOLADA NO NÚCLEO 0: SENSORES + DISPLAY OLED + THINGSPEAK
void tarefaSensoresENuvem(void * parametro) {
  unsigned long ultimoEnvioNuvem = millis();

  for (;;) {
    // LEITUDA DOS SENSORES
    emonFaseA.calcVI(20, 600);
    float leituraTensao = emonFaseA.Vrms;
    float leituraCorrenteA = emonFaseA.Irms;
    float leituraPotenciaA = fabs(emonFaseA.realPower);

    vTaskDelay(pdMS_TO_TICKS(20));

    emonFaseB.calcVI(20, 600);
    float leituraCorrenteB = emonFaseB.Irms;
    float leituraPotenciaB = fabs(emonFaseB.realPower);

    // FILTROS DE RUIDO
    if (leituraTensao < 65.0) { 
      leituraTensao = 0.0; 
      leituraPotenciaA = 0.0; 
      leituraPotenciaB = 0.0; 
    }
    if (leituraCorrenteA < 0.3) { 
      leituraCorrenteA = 0.0; 
      leituraPotenciaA = 0.0; 
    }
    if (leituraCorrenteB < 0.3) { 
      leituraCorrenteB = 0.0; 
      leituraPotenciaB = 0.0; 
    }

    // ATUALIZA AS VARIÁVEIS GLOBAIS
    tensao = leituraTensao;
    correnteA = leituraCorrenteA;
    potenciaA = leituraPotenciaA;
    correnteB = leituraCorrenteB;
    potenciaB = leituraPotenciaB;

    // ATUALIZA A LEITURA NO DISPLAY
    display.clearDisplay();
    display.setTextSize(1);
    
    display.setCursor(0, 0);
    display.print("Rede: "); display.print(tensao, 1); display.println(" V");
    
    display.setCursor(0, 10);
    display.print("AP IP: 192.168.4.1");
    
    display.drawLine(0, 20, 128, 20, WHITE); 
    
    display.setCursor(0, 25);
    display.print("FASE A: "); display.print(correnteA, 2); display.println(" A");
    display.print("Pot A : "); display.print(potenciaA, 1); display.println(" W");

    display.setCursor(0, 47);
    display.print("FASE B: "); display.print(correnteB, 2); display.println(" A");
    display.print("Pot B : "); display.print(potenciaB, 1); display.println(" W");
    
    display.display();

    // ENVIO DE DADOS AO THINGSPEAK
    if (millis() - ultimoEnvioNuvem >= 15000) {
      ultimoEnvioNuvem = millis();
      if (WiFi.status() == WL_CONNECTED) {
        ThingSpeak.setField(1, (float)tensao);
        ThingSpeak.setField(2, (float)correnteA);
        ThingSpeak.setField(3, (float)potenciaA);
        ThingSpeak.setField(4, (float)correnteB);
        ThingSpeak.setField(5, (float)potenciaB);
        ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
      }
    }

  
    vTaskDelay(pdMS_TO_TICKS(1500));
  }
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  // INICIALIZAÇÃO DO DISPLAY OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Falha ao iniciar OLED"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);

  // INICIALIZAÇÃO DO WI-FI
  display.setCursor(0, 20);
  display.println("Conectando Wi-Fi...");
  display.display();
  
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(120);
  wifiManager.autoConnect("SIMEE_Config"); 

  // MODO HÍBRIDO DE ALTA ESTABILIDADE
  WiFi.mode(WIFI_AP_STA);
  
  int canalAtual = WiFi.channel() > 0 ? WiFi.channel() : 1;
  WiFi.softAP("SIMEE_Dashboard", "simee1234", canalAtual, 0, 8); 
  
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);

  // INICIALIZAÇÃO DO THINGSPEAK
  client.setTimeout(2500);
  ThingSpeak.begin(client);

  // CALIBRAÇÃO DOS SENSORES DE TENSÃO E CORRENTE
  emonFaseA.voltage(pinoTensao, 92.6, 1.7);
  emonFaseA.current(pinoCorrenteA, 12.5);
  
  emonFaseB.voltage(pinoTensao, 92.6, 1.7);
  emonFaseB.current(pinoCorrenteB, 12.5);

  // ESTABILIZAÇÃO DO FILTRO NO DISPLAY
  display.clearDisplay();
  display.setCursor(0, 20);
  display.println("Estabilizando ADC...");
  display.display();
  
  for (int i = 0; i < 15; i++) {
    emonFaseA.calcIrms(1480);
    emonFaseB.calcIrms(1480);
    emonFaseA.calcVI(20, 500);
    emonFaseB.calcVI(20, 500);
    yield();
  }

  // INICIALIZAÇÃO DO SERVIDOR WEB LOCAL
  server.on("/", enviarHTML);
  server.on("/api/dados", enviarDadosJSON);
  server.begin();

  // DISPARA AS TAREFAS DOS SENSORES + OLED ISOLADAS NO NUCLEO 0
  xTaskCreatePinnedToCore(
    tarefaSensoresENuvem,
    "SensoresNuvem",
    10000,
    NULL,
    1,                     
    NULL,                  
    0                 
  );
}

void loop() {
  
  server.handleClient();
  delay(2);
}