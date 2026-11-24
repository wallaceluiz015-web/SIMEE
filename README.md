# SIMEE — Sistema Inteligente de Monitoramento e Eficiência Energética

Firmware embarcado desenvolvido na disciplina **ARA0029 – Eletricidade Aplicada** do curso de Engenharia da Computação — Centro Universitário Estácio de Belém (Belém-PA).

## Arquitetura
- **Microcontrolador:** ESP32-WROOM-32 (FreeRTOS Dual-Core)
- **Sensores:** 2× SCT-013 (100 A / 50 mA) e 1× ZMPT101B (0–250 V AC isolado)
- **Interface Local:** Display OLED 0,96" I2C (SSD1306) e Dashboard Web 100% offline em SVG (`192.168.4.1`, modo Wi-Fi híbrido `AP + STA`)
- **Telemetria em Nuvem:** Integração via API REST com a plataforma ThingSpeak

## Equipe
- Elrick Vinícius Pinheiro de Almeida Leite
- Joeni Abreu Franco
- Masaru Nagasaka Neto
- Wallace Luiz Santos dos Santos
- Yasmin Vitória dos Santos da Silva

**Orientador:** Prof. Sergio Henrique Monte Santo Andrade
