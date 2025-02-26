# 📌 Sistema de Monitoramento de Temperatura e Bem-Estar

## 📖 Sobre o Projeto

Este projeto visa desenvolver um sistema de baixo custo para monitoramento de temperatura e bem-estar, utilizando sensores e dispositivos embarcados. O sistema é capaz de monitorar temperatura e umidade, emitir alertas visuais e sonoros em casos de temperaturas extremas e possui suporte para integração futura com inteligência artificial para detecção de mal-estar.

## 🚀 Funcionalidades

- 📊 **Monitoramento de temperatura e umidade**
- 🔔 **Alertas sonoros e visuais para temperaturas extremas**
- 🎛 **Interface de exibição dos dados via display OLED**
- 💡 **Feedback visual através de LED RGB e matriz de LED**
- 📷 **Suporte para futura integração com câmeras**
- ☁️ **Possibilidade de integração com serviços de IA na nuvem**

## 🛠 Tecnologias Utilizadas

- **Microcontrolador**: Raspberry Pi Pico W
- **Sensores**:
  - DHT22 (temperatura e umidade)
  - Simulação de câmera (para futura integração)
- **Atuadores**:
  - Buzzer (alerta sonoro)
  - LED RGB e Matriz de LED (alerta visual)
- **Protocolos de Comunicação**:
  - I2C (para comunicação com o display OLED e sensores)
  - SPI (para futura integração com câmera Arducam OV2640)
- **Linguagem de Programação**:
  - C/C++ (utilizando o SDK do Raspberry Pi Pico)

## 🔧 Estrutura do Projeto

```
📂 Projeto
│── 📂 img            # Esquemas e diagramas de hardware
│── 📂 lib            # Bibliotecas auxiliares
│── 📂 ref            # Artigos de referência utilizados
│── main.c            # Código-fonte
│── README.md         # Documento principal
│── CMakeLists.txt    # Configuração do CMake para build
```

## 🏗 Instalação e Configuração

### 📥 Pré-requisitos

- Raspberry Pi Pico W
- Sensor DHT22
- Display OLED SSD1306
- 2 Botões, Joystick, LED RGB, Matriz de LED e Buzzer
- Cabo micro USB para programação
- **Software**:
  - Visual Studio Code + Extensão Raspberry Pi Pico
  - CMake
  - SDK do Raspberry Pi Pico

### ⚙️ Passos para Configuração

1. **Clone este repositório**:

    ```bash
   git clone https://github.com/matheusssilva991/projeto_final_embarcatech.git.
   ```

2. **Configure o ambiente de desenvolvimento** seguindo as instruções do SDK do Raspberry Pi Pico.
3. **Compile e envie o código** para o microcontrolador:

   ```bash
   mkdir build && cd build
   cmake -G Ninja ..
   Ninja
   ```

4. **Transfira o arquivo UF2** gerado para o Raspberry Pi Pico.

## 🧪 Testes

Os testes incluem:

- Simulação de valores de temperatura e umidade
- Testes funcionais dos alertas visuais e sonoros
- Testes de interação com os botões físicos para alternância de cômodos e modos

## Demonstração

A seguir, um vídeo demonstrando o funcionamento do projeto:

[![Vídeo de demonstração](https://drive.google.com/file/d/1ZNhqvEIdUILCuW7tRM4jyrmGYjl9HgOJ/view?usp=sharing)](https://drive.google.com/file/d/1ZNhqvEIdUILCuW7tRM4jyrmGYjl9HgOJ/view?usp=sharing)

## 🔮 Futuras Melhorias

- 📷 Implementação real da câmera Arducam OV2640
- ☁️ Integração com serviços de IA na nuvem para detecção de mal-estar
- 📡 Comunicação remota via Wi-Fi para monitoramento em tempo real

## 📜 Licença

Este projeto é distribuído sob a licença MIT. Consulte o arquivo `LICENSE` para mais informações.

## 🤝 Contribuição

Sinta-se à vontade para contribuir! Caso tenha sugestões ou melhorias, abra uma issue ou faça um pull request.

---
✉️ Para dúvidas ou sugestões, entre em contato! 🚀

## 🤝 Equipe

Membros da equipe de desenvolvimento do projeto:
<table>
  <tr>
    <td align="center">
      <a href="https://github.com/matheusssilva991">
        <img src="https://github.com/matheusssilva991.png" width="100px;" alt="Foto de Matheus Santos Silva no GitHub"/><br>
        <b>Matheus Santos Silva (matheusssilva991)</b>
        <p>Desenvolvedor Back-end - NestJS</p>
      </a>
    </td>
  <tr>
</table>
