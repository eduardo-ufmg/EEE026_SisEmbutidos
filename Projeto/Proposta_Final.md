## Objetivo
Garantir, ao usuário, formas seguras e práticas de substituir a chave mecânica no uso de uma fechadura.

## Motivação

Substituir a chave mecânica evita que o usuário:
- Fique sem recursos para usar a fechadura.
- Precise estar presente para conhecer seu estado.
- Tenha a segurança facilmente sobrepassada.

## Detalhamento

### Formas de manipular a fechadura:
- **RFID 13.56 MHz** (mais popular do que aqueles identificados somente como NFC)
- **Página web** acessada por senha
- **Senha em teclado numérico** com limite de tentativas
- **Chave elétrica** no lado de dentro da casa

### Não adaptaremos uma fechadura com chave mecânica porque:
- Uma fechadura eletrônica com chave mecânica é cara.
- "Eletronizar" uma fechadura convencional é inviável para nossos recursos, tempo e esforços.

### Em caso de falta de energia elétrica na rede:
O sistema será mantido por uma bateria.

### Página web:
A página web monitora o estado da fechadura, cadastra e reporta etiquetas cadastradas e apresenta um registro das ações.

### Lista de materiais:
- **Placa de prototipação ESP32**:
  - \[1\] [Eletrogate](https://www.eletrogate.com/modulo-wifi-esp32-bluetooth-30-pinos)
  - \[2\] [MakerHero](https://www.makerhero.com/produto/modulo-wifi-esp32-bluetooth/)
- **Módulo teclado numérico**: **Já temos**
- **Módulo relé**: **Já temos**
- **Tranca eletrônica solenóide**: 
  - \[1\] [Eletrogate](https://www.eletrogate.com/mini-trava-eletrica-solenoide) 
  - \[2\] [MakerHero](https://www.makerhero.com/produto/mini-trava-eletrica-solenoide-12v/)
- **Aumentador de tensão**:
  - \[1\] [Eletrogate](https://www.eletrogate.com/regulador-de-tensao-ajustavel-mt3608-auto-boost-step-up)
  - \[2\] [MakerHero](https://www.makerhero.com/produto/conversor-boost-dc-step-up/)
- **Bateria LiPo**: **Já temos**
- **Carregador de bateria LiPo**: **Já temos**
- **Fonte de tensão**: **Já temos**
- **Leitor RFID 13.56 MHz**: **Já temos**
- **Expansor I2C**:
  - \[1\] [Eletrogate](https://www.eletrogate.com/ci-pcf8574-expansor-de-portas-i2c)
  - \[2\] [MakerHero](https://www.makerhero.com/produto/ci-pcf8574-expansor-de-portas-i2c/)
- **Chave elétrica**: **Já temos**
- **Fios**: **Já temos**
- **Resistores**: **Já temos**
- **Capacitores**: **Já temos**
- **Protoboard**: **Já temos**

### Recursos de software:
- Arduino IDE
- API nativa de RTOS
- ngrok
- Biblioteca ESPAsyncWebServer
- Biblioteca AsyncTCP
- Biblioteca MFRC522
- Biblioteca Keypad
- Biblioteca ArduinoJson
- Git para controle de versão
