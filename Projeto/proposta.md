# Proposta de Projeto: Detector de Quedas para Pessoas com Mobilidade Reduzida

## Descrição do Projeto

A segunda proposta de projeto trata-se de um detector de quedas, que deve ser pequeno, vestível e relativamente confortável, com boa eficiência energética para ser leve e necessitar de pouca recarga. Deve possuir pelo menos conexão à internet para enviar alarmes ou acionar contatos de emergência. O projeto inicial é composto por um acelerômetro de 3 DoF e um ESP8266 alimentado por pilhas, baseado no método descrito em [1].

A motivação do projeto é criar um dispositivo eficiente para ajudar idosos e pessoas com mobilidade reduzida, para as quais quedas podem causar graves problemas. O projeto visa principalmente auxiliar na chamada de ajuda o mais rápido possível em caso de queda, diminuindo os riscos de danos permanentes.

## Objetivo

Implementar um sistema embutido capaz de detectar e reportar, via rede sem fio, a queda de seu usuário.

## Objetivos de Composição

O sistema deve ser capaz de:
- Detectar uma queda através da análise dos sinais de seus sensores.
- Conectar-se a uma rede sem fio adequada, sempre que necessário.
- Transmitir, por esta rede, o aviso quanto à queda.
- Ficar ligado, sem interrupções ou recarga, por todo o período ativo de seu usuário.

## Estimativa de Componentes

- **ESP8266**: Seu modo de sono profundo tem consumo inferior ao do ESP32.
- **MPU6050**: Sua qualidade é, por estimativa baseada em experiência, suficiente; seu custo, bem inferior ao de módulos mais robustos, é atrativo.
- **Fonte 5 V**: Para prototipagem, podemos usar uma fonte AC-DC.
- **Protoboard**: Para prototipagem, as conexões podem ser temporárias.
- **Fios Rígidos**: Mais convenientes para conexões intra-placa.
- **Regulador 5-3 V**: Dado o baixo consumo do acelerômetro, tende a ser mais eficiente.

## Estimativa de Tarefas

- Implementação do modo de sono profundo.
- Conexões esporádicas para verificação da rede.
- Conexão sob demanda com a rede.
- Transmissão de aviso pela rede.
- Configuração do acelerômetro para sono profundo.
- Detecção de queda enquanto em sono profundo.
- Acionamento do ESP pela interrupção disparada pelo acelerômetro.
- Transmissão do aviso quando queda é detectada.
- Retomada do sono profundo.

## Referências

[1] LIN, Dongha; PARK, Chulho; KIM, Nam Ho; KIM, Sang-Hoon; YU, Yun Seop. Fall Detection Algorithm Using 3-Axis Acceleration: Combination with Simple Threshold and Hidden Markov Model. Journal Of Applied Mathematics, [s. l.], 17 set. 2014. Disponível em: [https://onlinelibrary.wiley.com/doi/10.1155/2014/896030](https://onlinelibrary.wiley.com/doi/10.1155/2014/896030). Acesso em: 10 out. 2024.
