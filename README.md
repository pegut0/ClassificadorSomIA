# Classificador de Sons em Tempo Real com ESP32 e IA

Este projeto implementa um dispositivo autônomo capaz de capturar áudio do ambiente, enviá-lo para um servidor local e usar um modelo de Inteligência Artificial (TensorFlow/Keras) para classificar o som em tempo real. O resultado é exibido em um display OLED.

## 🚀 Funcionalidades

- **Classificação em Tempo Real:** Captura 5 segundos de áudio e envia para análise.
- **Servidor Inteligente:** Um servidor em Python (Flask) que executa um modelo de rede neural convolucional (CNN) para classificar os sons.
- **Filtros Avançados:** O servidor possui filtros para rejeitar silêncio e ruídos de fundo constantes (como ventiladores), aumentando a precisão.
- **Display Informativo:** Um display OLED mostra o status do dispositivo e o resultado da classificação.
- **Processamento de Sinal Embarcado:** O ESP32 aplica ganho e filtros de ruído no áudio antes do envio para otimizar a qualidade.

## 🛠️ Componentes Necessários

| Componente | Descrição | Imagem de Exemplo |
| :--- | :--- | :--- |
| **ESP32 Dev Kit** | O microcontrolador principal do projeto. | ![Imagem de um ESP32](https://i.imgur.com/tG2T2mU.png) |
| **Microfone I2S INMP441** | Microfone digital de alta qualidade para captura de áudio. | ![Imagem de um microfone INMP441](https://i.imgur.com/bY3gZkL.jpg) |
| **Display OLED I2C 0.96"** | Display de 128x64 pixels para exibir informações. | ![Imagem de um display OLED 128x64](https://i.imgur.com/uQyY2qA.jpg) |
| **Cabos Jumper** | Para conectar os componentes. | ![Imagem de cabos jumper](https://i.imgur.com/j5L8g3w.png) |
| **Protoboard (Opcional)** | Facilita a organização das conexões de GND. | ![Imagem de uma protoboard](https://i.imgur.com/ysL4g2v.png) |

## 🔌 Montagem do Circuito

Conecte os componentes ao ESP32 conforme a tabela e o diagrama abaixo.

**Dica:** Use uma protoboard para criar uma linha de `GND` comum e facilitar as conexões.

| Módulo | Pino do Módulo | Pino no ESP32 |
| :--- | :---: | :---: |
| **Microfone I2S** | VDD | 3.3V |
| | GND | GND |
| | SCK | GPIO 14 |
| | WS | GPIO 15 |
| | SD | GPIO 32 |
| | L/R | GND |
| **Display OLED** | VCC | 3.3V |
| | GND | GND |
| | SDA | GPIO 21 |
| | SCL | GPIO 22 |


## ⚙️ Configuração do Software

O projeto é dividido em duas partes: o **servidor** (que roda no seu computador) e o **firmware** (que roda no ESP32).

### 1. Configuração do Servidor (Python)

O servidor é responsável por receber o áudio e executar o modelo de IA.

**Pré-requisitos:**
- Python 3.8 ou superior instalado.

**Passos:**

1.  **Clone o Repositório:**
    ```bash
    git clone [https://github.com/seu-usuario/seu-repositorio.git](https://github.com/seu-usuario/seu-repositorio.git)
    cd seu-repositorio/servidor
    ```

2.  **Crie um Ambiente Virtual:**
    ```bash
    # Para Windows
    python -m venv venv
    .\venv\Scripts\activate

    # Para macOS/Linux
    python3 -m venv venv
    source venv/bin/activate
    ```

3.  **Instale as Dependências:**
    Crie um arquivo chamado `requirements.txt` dentro da pasta `servidor` com o seguinte conteúdo:
    ```
    Flask
    tensorflow
    numpy
    librosa
    soundfile
    ```
    Em seguida, instale as bibliotecas:
    ```bash
    pip install -r requirements.txt
    ```

4.  **Execute o Servidor:**
    Certifique-se de que o arquivo do modelo (`classificador_som_otimizado.keras`) está na mesma pasta.
    ```bash
    python server.py 
    ```
    O terminal mostrará o endereço de IP local no qual o servidor está rodando (ex: `192.168.18.103`). Anote este IP!

### 2. Configuração do ESP32 (Arduino IDE)

O firmware do ESP32 captura o áudio e o envia para o servidor.

**Passos:**

1.  **Configure a Arduino IDE:** Se for sua primeira vez, configure a Arduino IDE para programar o ESP32. Siga um [guia de instalação](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html).

2.  **Instale as Bibliotecas:**
    Na Arduino IDE, vá em **Ferramentas > Gerenciar Bibliotecas...** e instale as seguintes bibliotecas:
    - `ArduinoJson` by Benoit Blanchon
    - `Adafruit GFX Library` by Adafruit
    - `Adafruit SSD1306` by Adafruit

3.  **Abra e Configure o Código:**
    - Abra o arquivo `.ino` do ESP32 na Arduino IDE.
    - Altere as seguintes linhas com suas informações:
      ```cpp
      // WiFi
      const char* WIFI_SSID = "NOME_DA_SUA_REDE_WIFI";
      const char* WIFI_PASSWORD = "SENHA_DA_SUA_REDE_WIFI";

      // Servidor Flask
      const char* SERVER_HOST = "192.168.18.103"; // COLOQUE O IP DO SEU SERVIDOR AQUI!
      ```

4.  **Envie o Código:**
    - Conecte o ESP32 ao seu computador.
    - Na Arduino IDE, selecione a placa correta (ex: "ESP32 Dev Module") e a porta COM correta.
    - Clique no botão de "Carregar" (seta para a direita).

## 🕹️ Como Usar

1.  Ligue o ESP32. O display mostrará "Iniciando...".
2.  Aguarde ele se conectar ao WiFi. O display mostrará "WiFi OK!" seguido do IP do ESP32.
3.  Quando o display mostrar "Pronto!", o dispositivo está pronto para uso.
4.  Pressione o botão `BOOT` na placa do ESP32.
5.  O display mostrará "Gravando...". O dispositivo irá capturar 5 segundos de áudio.
6.  Em seguida, ele enviará o áudio e mostrará "Aguardando Servidor...".
7.  O resultado da classificação aparecerá no display (ex: "Cachorro") com o nível de confiança. Se o som não for reconhecido, ele mostrará "Nao reconhecido".

---
*Este projeto foi desenvolvido com o auxílio do Gemini. Última atualização: Julho de 2025.*
