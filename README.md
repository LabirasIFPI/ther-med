# TherMed ❄️💊 - Sistema de Monitoramento de Termolábeis

## Visão Geral
Este projeto monitora a temperatura de medicamentos termolábeis utilizando a placa **BitDogLab**, que integra um **Raspberry Pi Pico W (RP2040)**. O sistema dispara **alarmes locais** e **notifica via Telegram** através de uma API desenvolvida em **Java**.

## Funcionalidades
- **Medição de temperatura:** Utilizando o sensor **DHT22**.
- **Alarmes locais:** Indica situações críticas através de LEDs, buzzer e display.
- **Notificação remota:** Envio de alertas via **bot do Telegram**.
- **Interface local:** Controle via **joystick** e **botões**.
- **Exibição no display:** Mostra temperatura em tempo real no **OLED SSD1306**.

## Hardware Utilizado
- **Placa BitDogLab** (RP2040 com Wi-Fi)
- **Sensor DHT22** (Temperatura e Umidade)
- **Joystick** (Navegação e ajustes)
- **2 Botões** (Interação com o sistema)
- **Matriz de LEDs 5050 e Buzzer(Alarmes locais)**
- **Display OLED SSD1306** (Mostra informações)

## Software Utilizado
- **Firmware (RP2040)**: Desenvolvido em **C** com **Pico SDK**
- **Backend**: API em **Java (Spring Boot)** para gerenciar e enviar alertas
- **Banco de Dados**: PostgreSQL (para armazenar dispositivos e alarmes registrados no sistema TherMed)
- **Comunicação**: Protocolo HTTP e API REST

## Como Funciona
1. O **sensor DHT22** faz a leitura da temperatura periodicamente.
2. O **RP2040 processa os dados** e exibe no **display OLED**.
3. Se a temperatura ultrapassar o limite:
   - Um **alarme local** é ativado (LEDs e buzzer).
   - Um **alerta é enviado** via bot do **Telegram**.
4. O usuário pode navegar pela configuração de limites de temperatura usando o **joystick** e os **botões**.

## Instalação e Configuração
### Firmware (RP2040)
1. Clone este repositório:
   ```sh
   git clone https://github.com/LabirasIFPI/ther-med.git
   cd ther-med/
   ```
2. Compile com o **Pico SDK**:
   ```sh
   mkdir build && cd build
   cmake ..
   make
   ```
3. Envie o firmware para o **RP2040**:
   ```sh
   cp thermed-pico.uf2 /media/pi/RPI-RP2
   ```

### Backend (API em Java)
Para mais informações sobre a API, consulte-a em https://github.com/LabirasIFPI/thermed-api

## Uso
- **Monitoramento local:** A temperatura é exibida no display OLED.
- **Navegação:** Use o **joystick** e os **botões** para acessar o menu de configuração de temperaturas necessárias para disparar um alarme.
- **Alertas:** Se a temperatura sair da faixa segura, um alarme local é ativado e uma notificação é enviada para o Telegram.

## Autor
Xamã Cardoso Mendes Santos

