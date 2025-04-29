<h1>
  <p align="center" width="100%">
    <img width="30%" src="https://softex.br/wp-content/uploads/2024/09/EmbarcaTech_logo_Azul-1030x428.png">
  </p>
</h1>

# ✨Tecnologias
Esse projeto foi desenvolvido com as seguintes tecnologias.
- Placa Raspberry Pi Pico W
- Raspberry Pi Pico SDK
- WokWi
- C/C++

# 💻Projeto
Projeto Desenvolvido durante a residência em microcontrolados e sistemas embarcados para estudantes de nível superior ofertado pela CEPEDI e SOFTEX, polo Juazeiro-BA, na Universidade Federal do Vale do São Francisco (UNIVASF).

# 🚀Como rodar
### **Softwares Necessários**
1. **VS Code** com a extensão **Raspberry Pi Pico** instalada.
2. **CMake** e **Ninja** configurados.
3. **SDK do Raspberry Pi Pico** corretamente configurado.

### **Clonando o Repositório**
Para começar, clone o repositório no seu computador:
```bash
git clone https://github.com/DevMaic/Ohmimetro-Raspberry_Pi_PicoW
cd Ohmimetro-Raspberry_Pi_PicoW
```
---


### **Execução na Placa BitDogLab**
#### **1. Conectando os resistores na protoboard**
1. Posicionar um resistor de 10k em uma protoboard, esse será o resistor de referência.
2. Conectar em série com o resistor de referência o resistor ao qual se deseja aferir.
#### **2. Conexões na BitDogLab**
1. O pino 28 é utilizado como entrada do ADC.
2. Conecte o pino de 3.3V na ponta livre do resistor de referência.
3. Conecte o pino GND na ponta livre do resistor que se deseja aferir
4. Conecte o pino 28 na trilha da protoboard que liga o resistor de referência ao resistor a ser aferido.
#### **3. Observando os resultados`**
1. No display observe a leitura que mostra o valor numérico da resistência.
2. Também no display observe o código de cores sendo exibido por extenso.
3. Nos leds mais centrais da matriz de leds (11, 12, 13) oberserve o código de cores.
   